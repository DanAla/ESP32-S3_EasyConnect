#include "ESP32S3_EasyConnect.h"
#include <stdarg.h>

ESP32S3_EasyConnect EasyConnect;

ESP32S3_EasyConnect::ESP32S3_EasyConnect() 
  : server(80),
    webSocket(81),
    telnetServer(23) {
  // Initialize with default values
  config.deviceName = "ESP32-S3-Device";
  config.theme = "dark";
  config.enableOTA = true;
  config.enableTelnet = true;
  config.telnetPort = 23;
  config.updateInterval = 5000;
  config.customParam1 = "";
  config.customParam2 = "";
  config.customParam3 = 0;
  config.customParam4 = 0.0;
  
  // Initialize telnet clients
  for (int i = 0; i < MAX_TELNET_CLIENTS; i++) {
    telnetClients[i].connected = false;
    telnetClients[i].lastActivity = 0;
  }
}

bool ESP32S3_EasyConnect::begin(const char* deviceName) {
  Serial.begin(115200);
  
  // Initial startup message to Serial only (Telnet not ready yet)
  Serial.println("\n");
  Serial.println("🚀 Starting ESP32-S3 EasyConnect Framework v1.2.0");
  Serial.println("📡 With Telnet server and WebSocket support!");
  Serial.println("==============================================");
  
  // Initialize filesystem
  if (!LittleFS.begin(true)) {
    Serial.println("❌ LittleFS Mount Failed");
    return false;
  }
  
  // Load configuration
  if (!loadConfig()) {
    Serial.println("⚠️ Using default configuration");
  }
  
  // Set device name if provided
  if (deviceName != nullptr) {
    config.deviceName = deviceName;
  }
  
  // WiFiManager setup
  wifiManager.setTimeout(180);
  wifiManager.setConfigPortalTimeout(180);
  wifiManager.setAPCallback([](WiFiManager* mgr) {
    Serial.println("📱 Entered Configuration Mode");
    Serial.print("IP Address: ");
    Serial.println(WiFi.softAPIP());
  });
  
  // Custom parameters in WiFiManager
  WiFiManagerParameter custom_deviceName("name", "Device Name", config.deviceName.c_str(), 40);
  WiFiManagerParameter custom_theme("theme", "Theme (light/dark)", config.theme.c_str(), 10);
  WiFiManagerParameter custom_telnet("telnet", "Enable Telnet (0/1)", config.enableTelnet ? "1" : "0", 2);
  
  wifiManager.addParameter(&custom_deviceName);
  wifiManager.addParameter(&custom_theme);
  wifiManager.addParameter(&custom_telnet);
  
  // Attempt to connect to saved network or start configuration portal
  bool res = wifiManager.autoConnect(config.deviceName.c_str());
  
  if (!res) {
    logln("❌ Failed to connect and hit timeout");
    delay(3000);
    ESP.restart();
  } else {
    logln("✅ WiFi Connected!");
    log("IP Address: ");
    logln(WiFi.localIP().toString());
    isConnected = true;
    
    if (onConnectedCallback != nullptr) {
      onConnectedCallback();
    }
  }
  
  // Update config with WiFiManager parameters
  config.deviceName = custom_deviceName.getValue();
  config.theme = custom_theme.getValue();
  config.enableTelnet = (String(custom_telnet.getValue()) == "1");
  saveConfig();
  
  // Setup telnet server if enabled
  if (config.enableTelnet) {
    setupTelnet();
  }
  
  // Setup web server and WebSocket
  setupWebServer();
  setupWebSocket();
  
  // Setup ElegantOTA
  if (config.enableOTA) {
    ElegantOTA.begin(&server, otaUsername, otaPassword);
    logln("✅ OTA Updates enabled at /update");
  }
  
  server.begin();
  logln("✅ HTTP server started on port 80");
  logln("✅ WebSocket server started on port 81");
  
  deviceUptime = millis();
  return true;
}

void ESP32S3_EasyConnect::loop() {
  server.handleClient();
  webSocket.loop();
  ElegantOTA.loop();
  
  // Update uptime
  deviceUptime = millis();
  
  // Handle Telnet connections and data
  if (config.enableTelnet) {
    handleTelnet();
  }
  
  // Handle WiFi reconnection
  if (WiFi.status() != WL_CONNECTED) {
    if (isConnected) {
      isConnected = false;
      logln("❌ WiFi disconnected");
      if (onDisconnectedCallback != nullptr) {
        onDisconnectedCallback();
      }
    }
    
    if (millis() - lastReconnectAttempt > 10000) {
      logln("🔄 Attempting WiFi reconnection...");
      WiFi.reconnect();
      lastReconnectAttempt = millis();
    }
  } else if (!isConnected) {
    isConnected = true;
    logln("✅ WiFi reconnected");
    if (onConnectedCallback != nullptr) {
      onConnectedCallback();
    }
  }
  
  // Send periodic updates via WebSocket
  if (millis() - lastUpdate > config.updateInterval) {
    sendDeviceStatus();
    lastUpdate = millis();
  }
}

void ESP32S3_EasyConnect::setupTelnet() {
  telnetServer.begin();
  telnetServer.setNoDelay(true);
  telnetEnabled = true;
  
  log("✅ Telnet server started on port ");
  logln(String(config.telnetPort));
  logln("💡 Connect using: telnet " + WiFi.localIP().toString());
}

void ESP32S3_EasyConnect::handleTelnet() {
  // Check for new connections
  if (telnetServer.hasClient()) {
    bool connectionAccepted = false;
    
    for (int i = 0; i < MAX_TELNET_CLIENTS; i++) {
      if (!telnetClients[i].connected) {
        if (telnetClients[i].client) {
          telnetClients[i].client.stop();
        }
        
        telnetClients[i].client = telnetServer.available();
        telnetClients[i].connected = true;
        telnetClients[i].lastActivity = millis();
        
        // Send welcome message
        String welcome = "\r\n";
        welcome += "┌────────────────────────────────────────┐\r\n";
        welcome += "│       ESP32-S3 EasyConnect Telnet     │\r\n";
        welcome += "│              Framework v1.2.0         │\r\n";
        welcome += "└────────────────────────────────────────┘\r\n";
        welcome += "Device: " + config.deviceName + "\r\n";
        welcome += "IP: " + WiFi.localIP().toString() + "\r\n";
        welcome += "Free Heap: " + String(ESP.getFreeHeap()) + " bytes\r\n";
        welcome += "Uptime: " + String(deviceUptime / 1000) + "s\r\n";
        welcome += "Connected clients: " + String(getTelnetClientCount()) + "/" + String(MAX_TELNET_CLIENTS) + "\r\n";
        welcome += "Type 'help' for available commands\r\n";
        welcome += "----------------------------------------\r\n";
        welcome += "> ";
        
        telnetClients[i].client.print(welcome);
        connectionAccepted = true;
        
        log("🔌 Telnet client connected from: ");
        logln(telnetClients[i].client.remoteIP().toString());
        break;
      }
    }
    
    if (!connectionAccepted) {
      // No free slots, reject connection
      WiFiClient client = telnetServer.available();
      client.print("❌ Maximum telnet clients reached (" + String(MAX_TELNET_CLIENTS) + "). Try again later.\r\n");
      client.stop();
      logln("⚠️ Telnet connection rejected - maximum clients reached");
    }
  }
  
  // Handle data from connected clients
  for (int i = 0; i < MAX_TELNET_CLIENTS; i++) {
    if (telnetClients[i].connected && telnetClients[i].client.connected()) {
      // Check for incoming data
      while (telnetClients[i].client.available()) {
        String command = telnetClients[i].client.readStringUntil('\n');
        command.trim();
        
        if (command.length() > 0) {
          telnetClients[i].lastActivity = millis();
          
          log("📨 Telnet command from ");
          log(telnetClients[i].client.remoteIP().toString());
          log(": ");
          logln(command);
          
          // Handle built-in commands
          if (command == "help" || command == "?") {
            String help = "Available commands:\r\n";
            help += "  help, ?       - Show this help\r\n";
            help += "  status        - Show device status\r\n";
            help += "  restart       - Restart device\r\n";
            help += "  factoryreset  - Factory reset\r\n";
            help += "  clients       - Show connected clients\r\n";
            help += "  wifi          - Show WiFi info\r\n";
            help += "  memory        - Show memory usage\r\n";
            help += "  config        - Show current configuration\r\n";
            help += "  clear, cls    - Clear screen\r\n";
            help += "  disconnect    - Disconnect this session\r\n";
            help += "Custom commands can be added via callback\r\n";
            help += "> ";
            telnetClients[i].client.print(help);
            
          } else if (command == "status") {
            String status = "Device Status:\r\n";
            status += "  Name: " + config.deviceName + "\r\n";
            status += "  Uptime: " + String(deviceUptime / 1000) + "s\r\n";
            status += "  Free Heap: " + String(ESP.getFreeHeap()) + " bytes\r\n";
            status += "  WiFi: " + String(WiFi.SSID()) + " (" + String(WiFi.RSSI()) + " dBm)\r\n";
            status += "  IP: " + WiFi.localIP().toString() + "\r\n";
            status += "  Telnet clients: " + String(getTelnetClientCount()) + "/" + String(MAX_TELNET_CLIENTS) + "\r\n";
            status += "> ";
            telnetClients[i].client.print(status);
            
          } else if (command == "restart") {
            telnetClients[i].client.print("🔄 Restarting device...\r\n");
            delay(1000);
            restartDevice();
            
          } else if (command == "factoryreset") {
            telnetClients[i].client.print("🗑️ Factory reset...\r\n");
            delay(1000);
            factoryReset();
            
          } else if (command == "clients") {
            String clients = "Connected Telnet Clients:\r\n";
            for (int j = 0; j < MAX_TELNET_CLIENTS; j++) {
              if (telnetClients[j].connected && telnetClients[j].client.connected()) {
                clients += "  " + String(j+1) + ". " + telnetClients[j].client.remoteIP().toString() + 
                          " (active " + String((millis() - telnetClients[j].lastActivity) / 1000) + "s ago)\r\n";
              }
            }
            clients += "> ";
            telnetClients[i].client.print(clients);
            
          } else if (command == "wifi") {
            String wifiInfo = "WiFi Information:\r\n";
            wifiInfo += "  SSID: " + String(WiFi.SSID()) + "\r\n";
            wifiInfo += "  IP: " + WiFi.localIP().toString() + "\r\n";
            wifiInfo += "  MAC: " + String(WiFi.macAddress()) + "\r\n";
            wifiInfo += "  RSSI: " + String(WiFi.RSSI()) + " dBm\r\n";
            wifiInfo += "  Channel: " + String(WiFi.channel()) + "\r\n";
            wifiInfo += "> ";
            telnetClients[i].client.print(wifiInfo);
            
          } else if (command == "memory") {
            String memInfo = "Memory Information:\r\n";
            memInfo += "  Free Heap: " + String(ESP.getFreeHeap()) + " bytes\r\n";
            memInfo += "  Min Free Heap: " + String(ESP.getMinFreeHeap()) + " bytes\r\n";
            memInfo += "  Max Alloc Heap: " + String(ESP.getMaxAllocHeap()) + " bytes\r\n";
            memInfo += "  PSRAM Size: " + String(ESP.getPsramSize()) + " bytes\r\n";
            memInfo += "  Free PSRAM: " + String(ESP.getFreePsram()) + " bytes\r\n";
            memInfo += "> ";
            telnetClients[i].client.print(memInfo);
            
          } else if (command == "config") {
            String configInfo = "Current Configuration:\r\n";
            configInfo += "  Device Name: " + config.deviceName + "\r\n";
            configInfo += "  Theme: " + config.theme + "\r\n";
            configInfo += "  OTA Enabled: " + String(config.enableOTA ? "Yes" : "No") + "\r\n";
            configInfo += "  Telnet Enabled: " + String(config.enableTelnet ? "Yes" : "No") + "\r\n";
            configInfo += "  Update Interval: " + String(config.updateInterval) + "ms\r\n";
            configInfo += "  Custom1: " + config.customParam1 + "\r\n";
            configInfo += "  Custom2: " + config.customParam2 + "\r\n";
            configInfo += "  Custom3: " + String(config.customParam3) + "\r\n";
            configInfo += "  Custom4: " + String(config.customParam4) + "\r\n";
            configInfo += "> ";
            telnetClients[i].client.print(configInfo);
            
          } else if (command == "clear" || command == "cls") {
            // Clear screen (ANSI escape codes)
            telnetClients[i].client.print("\033[2J\033[H"); // Clear screen and move to home
            telnetClients[i].client.print("> ");
            
          } else if (command == "disconnect") {
            telnetClients[i].client.print("👋 Disconnecting...\r\n");
            telnetClients[i].client.stop();
            telnetClients[i].connected = false;
            
          } else {
            // Pass command to custom callback if set
            if (telnetCommandCallback != nullptr) {
              telnetCommandCallback(command, telnetClients[i].client);
            } else {
              telnetClients[i].client.print("❌ Unknown command. Type 'help' for available commands.\r\n> ");
            }
          }
        }
      }
      
      // Check for client timeout (10 minutes)
      if (millis() - telnetClients[i].lastActivity > 600000) {
        log("⏰ Telnet client timeout: ");
        logln(telnetClients[i].client.remoteIP().toString());
        telnetClients[i].client.print("⏰ Connection timeout. Goodbye!\r\n");
        telnetClients[i].client.stop();
        telnetClients[i].connected = false;
      }
    } else {
      // Client disconnected
      if (telnetClients[i].connected) {
        log("🔌 Telnet client disconnected: ");
        logln(telnetClients[i].client.remoteIP().toString());
        telnetClients[i].connected = false;
      }
    }
  }
}

void ESP32S3_EasyConnect::broadcastTelnet(String message) {
  if (!config.enableTelnet) return;
  
  for (int i = 0; i < MAX_TELNET_CLIENTS; i++) {
    if (telnetClients[i].connected && telnetClients[i].client.connected()) {
      telnetClients[i].client.print(message);
    }
  }
}

void ESP32S3_EasyConnect::sendToTelnet(String message) {
  if (!config.enableTelnet) return;
  broadcastTelnet(message);
}

// Enhanced logging system
void ESP32S3_EasyConnect::log(String message) {
  Serial.print(message);
  sendToTelnet(message);
}

void ESP32S3_EasyConnect::logln(String message) {
  Serial.println(message);
  sendToTelnet(message + "\r\n");
}

void ESP32S3_EasyConnect::logf(const char* format, ...) {
  char buffer[256];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);
  
  Serial.print(buffer);
  sendToTelnet(buffer);
}

bool ESP32S3_EasyConnect::loadConfig() {
  File file = LittleFS.open(configFile, "r");
  if (!file) {
    logln("❌ Failed to open config file for reading");
    return false;
  }
  
  size_t size = file.size();
  std::unique_ptr<char[]> buf(new char[size]);
  file.readBytes(buf.get(), size);
  file.close();
  
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, buf.get());
  
  if (error) {
    logln("❌ Failed to parse config file");
    return false;
  }
  
  config.deviceName = doc["deviceName"] | "ESP32-S3-Device";
  config.theme = doc["theme"] | "dark";
  config.enableOTA = doc["enableOTA"] | true;
  config.enableTelnet = doc["enableTelnet"] | true;
  config.telnetPort = doc["telnetPort"] | 23;
  config.updateInterval = doc["updateInterval"] | 5000;
  config.customParam1 = doc["customParam1"] | "";
  config.customParam2 = doc["customParam2"] | "";
  config.customParam3 = doc["customParam3"] | 0;
  config.customParam4 = doc["customParam4"] | 0.0;
  
  logln("✅ Configuration loaded successfully");
  return true;
}

bool ESP32S3_EasyConnect::saveConfig() {
  DynamicJsonDocument doc(1024);
  
  doc["deviceName"] = config.deviceName;
  doc["theme"] = config.theme;
  doc["enableOTA"] = config.enableOTA;
  doc["enableTelnet"] = config.enableTelnet;
  doc["telnetPort"] = config.telnetPort;
  doc["updateInterval"] = config.updateInterval;
  doc["customParam1"] = config.customParam1;
  doc["customParam2"] = config.customParam2;
  doc["customParam3"] = config.customParam3;
  doc["customParam4"] = config.customParam4;
  
  File file = LittleFS.open(configFile, "w");
  if (!file) {
    logln("❌ Failed to open config file for writing");
    return false;
  }
  
  serializeJson(doc, file);
  file.close();
  
  logln("✅ Configuration saved successfully");
  return true;
}

void ESP32S3_EasyConnect::setupWebServer() {
  // Serve static files from LittleFS
  server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
  
  // REST API endpoints
  server.on("/", HTTP_GET, [this]() { handleRoot(); });
  server.on("/api/status", HTTP_GET, [this]() { handleAPIStatus(); });
  server.on("/api/config", HTTP_GET, [this]() { handleAPIConfig(); });
  server.on("/api/config", HTTP_POST, [this]() { handleAPIConfig(); });
  server.on("/api/system", HTTP_POST, [this]() { handleAPISystem(); });
  server.on("/api/scan", HTTP_GET, [this]() { handleAPIScan(); });
  server.onNotFound([this]() { handleNotFound(); });
}

void ESP32S3_EasyConnect::setupWebSocket() {
  webSocket.begin();
  webSocket.onEvent([this](uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
    this->webSocketEvent(num, type, payload, length);
  });
}

void ESP32S3_EasyConnect::handleRoot() {
  server.send(200, "text/html", 
    "<html>"
    "<head><title>ESP32-S3 EasyConnect</title></head>"
    "<body>"
    "<h1>ESP32-S3 EasyConnect Framework</h1>"
    "<p>Device is running. Access the dashboard at <a href='/index.html'>/index.html</a></p>"
    "<p>OTA Updates: <a href='/update'>/update</a></p>"
    "<p>API Status: <a href='/api/status'>/api/status</a></p>"
    "</body>"
    "</html>");
}

void ESP32S3_EasyConnect::handleAPIStatus() {
  DynamicJsonDocument doc(512);
  
  doc["device"]["name"] = config.deviceName;
  doc["device"]["chipId"] = String((uint32_t)ESP.getEfuseMac(), HEX);
  doc["device"]["flashSize"] = ESP.getFlashChipSize();
  doc["device"]["freeHeap"] = ESP.getFreeHeap();
  doc["device"]["sdkVersion"] = ESP.getSdkVersion();
  doc["device"]["uptime"] = deviceUptime;
  
  doc["wifi"]["connected"] = isWiFiConnected();
  doc["wifi"]["ssid"] = WiFi.SSID();
  doc["wifi"]["rssi"] = WiFi.RSSI();
  doc["wifi"]["ip"] = WiFi.localIP().toString();
  doc["wifi"]["mac"] = WiFi.macAddress();
  
  doc["system"]["uptime"] = deviceUptime;
  doc["system"]["restartReason"] = ESP.getResetReason();
  doc["system"]["telnetEnabled"] = config.enableTelnet;
  doc["system"]["telnetClients"] = getTelnetClientCount();
  
  // Add custom data if callback is set
  if (customDataCallback != nullptr) {
    customDataCallback(doc);
  }
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void ESP32S3_EasyConnect::handleAPIConfig() {
  if (server.method() == HTTP_GET) {
    DynamicJsonDocument doc(1024);
    
    doc["deviceName"] = config.deviceName;
    doc["theme"] = config.theme;
    doc["enableOTA"] = config.enableOTA;
    doc["enableTelnet"] = config.enableTelnet;
    doc["telnetPort"] = config.telnetPort;
    doc["updateInterval"] = config.updateInterval;
    doc["customParam1"] = config.customParam1;
    doc["customParam2"] = config.customParam2;
    doc["customParam3"] = config.customParam3;
    doc["customParam4"] = config.customParam4;
    
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
    
  } else if (server.method() == HTTP_POST) {
    String body = server.arg("plain");
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, body);
    
    if (error) {
      server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
      return;
    }
    
    if (doc.containsKey("deviceName")) config.deviceName = doc["deviceName"].as<String>();
    if (doc.containsKey("theme")) config.theme = doc["theme"].as<String>();
    if (doc.containsKey("enableOTA")) config.enableOTA = doc["enableOTA"];
    if (doc.containsKey("enableTelnet")) config.enableTelnet = doc["enableTelnet"];
    if (doc.containsKey("telnetPort")) config.telnetPort = doc["telnetPort"];
    if (doc.containsKey("updateInterval")) config.updateInterval = doc["updateInterval"];
    if (doc.containsKey("customParam1")) config.customParam1 = doc["customParam1"].as<String>();
    if (doc.containsKey("customParam2")) config.customParam2 = doc["customParam2"].as<String>();
    if (doc.containsKey("customParam3")) config.customParam3 = doc["customParam3"];
    if (doc.containsKey("customParam4")) config.customParam4 = doc["customParam4"];
    
    saveConfig();
    
    if (onConfigChangedCallback != nullptr) {
      onConfigChangedCallback();
    }
    
    server.send(200, "application/json", "{\"status\":\"Configuration updated\"}");
  }
}

void ESP32S3_EasyConnect::handleAPISystem() {
  String action = server.arg("action");
  
  if (action == "restart") {
    server.send(200, "application/json", "{\"status\":\"Restarting...\"}");
    delay(1000);
    restartDevice();
  } else if (action == "factoryReset") {
    server.send(200, "application/json", "{\"status\":\"Factory reset...\"}");
    delay(1000);
    factoryReset();
  } else {
    server.send(400, "application/json", "{\"error\":\"Invalid action\"}");
  }
}

void ESP32S3_EasyConnect::handleAPIScan() {
  int n = WiFi.scanNetworks();
  DynamicJsonDocument doc(2048);
  JsonArray networks = doc.createNestedArray("networks");
  
  for (int i = 0; i < n; ++i) {
    JsonObject network = networks.createNestedObject();
    network["ssid"] = WiFi.SSID(i);
    network["rssi"] = WiFi.RSSI(i);
    network["encryption"] = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "open" : "secured";
    network["channel"] = WiFi.channel(i);
  }
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void ESP32S3_EasyConnect::handleNotFound() {
  server.send(404, "application/json", "{\"error\":\"Endpoint not found\"}");
}

void ESP32S3_EasyConnect::webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      logf("[%u] WebSocket Disconnected!\n", num);
      break;
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
        logf("[%u] WebSocket Connected from %d.%d.%d.%d\n", num, ip[0], ip[1], ip[2], ip[3]);
        sendDeviceStatus();
      }
      break;
    case WStype_TEXT:
      {
        String message = String((char*)payload);
        logf("[%u] WebSocket Received: %s\n", num, message.c_str());
        
        // Handle WebSocket commands
        if (message == "getStatus") {
          sendDeviceStatus();
        } else if (message == "toggleTheme") {
          config.theme = (config.theme == "dark") ? "light" : "dark";
          saveConfig();
          sendDeviceStatus();
        } else {
          // Pass to custom callback
          if (webSocketCommandCallback != nullptr) {
            webSocketCommandCallback(message, num);
          }
        }
      }
      break;
  }
}

void ESP32S3_EasyConnect::sendDeviceStatus() {
  DynamicJsonDocument doc(512);
  
  doc["type"] = "status";
  doc["wifi"]["connected"] = isWiFiConnected();
  doc["wifi"]["ssid"] = WiFi.SSID();
  doc["wifi"]["rssi"] = WiFi.RSSI();
  doc["wifi"]["ip"] = WiFi.localIP().toString();
  doc["system"]["freeHeap"] = ESP.getFreeHeap();
  doc["system"]["uptime"] = deviceUptime;
  doc["config"]["theme"] = config.theme;
  doc["config"]["deviceName"] = config.deviceName;
  doc["telnet"]["enabled"] = config.enableTelnet;
  doc["telnet"]["clients"] = getTelnetClientCount();
  
  String jsonString;
  serializeJson(doc, jsonString);
  webSocket.broadcastTXT(jsonString);
}

void ESP32S3_EasyConnect::broadcastWebSocket(String message) {
  webSocket.broadcastTXT(message);
}

void ESP32S3_EasyConnect::restartDevice() {
  logln("🔄 Restarting device...");
  delay(1000);
  ESP.restart();
}

void ESP32S3_EasyConnect::factoryReset() {
  logln("🗑️ Performing factory reset...");
  
  // Clear WiFi credentials
  wifiManager.resetSettings();
  
  // Delete config file
  LittleFS.remove(configFile);
  
  // Disconnect all telnet clients
  disconnectTelnetClients();
  
  delay(1000);
  ESP.restart();
}

bool ESP32S3_EasyConnect::isWiFiConnected() {
  return WiFi.status() == WL_CONNECTED;
}

// Callback setters
void ESP32S3_EasyConnect::onConnected(void (*callback)()) {
  onConnectedCallback = callback;
}

void ESP32S3_EasyConnect::onDisconnected(void (*callback)()) {
  onDisconnectedCallback = callback;
}

void ESP32S3_EasyConnect::onConfigChanged(void (*callback)()) {
  onConfigChangedCallback = callback;
}

void ESP32S3_EasyConnect::setCustomDataCallback(void (*callback)(JsonDocument&)) {
  customDataCallback = callback;
}

void ESP32S3_EasyConnect::onTelnetCommand(void (*callback)(String, WiFiClient&)) {
  telnetCommandCallback = callback;
}

void ESP32S3_EasyConnect::onWebSocketCommand(void (*callback)(String, uint8_t)) {
  webSocketCommandCallback = callback;
}

int ESP32S3_EasyConnect::getTelnetClientCount() {
  int count = 0;
  for (int i = 0; i < MAX_TELNET_CLIENTS; i++) {
    if (telnetClients[i].connected && telnetClients[i].client.connected()) {
      count++;
    }
  }
  return count;
}

void ESP32S3_EasyConnect::disconnectTelnetClients() {
  for (int i = 0; i < MAX_TELNET_CLIENTS; i++) {
    if (telnetClients[i].connected) {
      telnetClients[i].client.print("🔌 Server shutting down for maintenance. Goodbye!\r\n");
      telnetClients[i].client.stop();
      telnetClients[i].connected = false;
    }
  }
}

void ESP32S3_EasyConnect::printDebugInfo() {
  logln("\n=== ESP32-S3 EasyConnect Debug Info ===");
  log("Device Name: "); logln(config.deviceName);
  log("WiFi Status: "); logln(isWiFiConnected() ? "Connected" : "Disconnected");
  log("IP Address: "); logln(WiFi.localIP().toString());
  log("Free Heap: "); logln(String(ESP.getFreeHeap()) + " bytes");
  log("Theme: "); logln(config.theme);
  log("Telnet Enabled: "); logln(config.enableTelnet ? "Yes" : "No");
  log("Telnet Clients: "); logln(String(getTelnetClientCount()) + "/" + String(MAX_TELNET_CLIENTS));
  log("Uptime: "); logln(String(deviceUptime / 1000) + " seconds");
  logln("====================================\n");
}

String ESP32S3_EasyConnect::getIPAddress() {
  return WiFi.localIP().toString();
}

unsigned long ESP32S3_EasyConnect::getUptime() {
  return deviceUptime;
}

DeviceConfig ESP32S3_EasyConnect::getConfig() {
  return config;
}

void ESP32S3_EasyConnect::setConfig(const DeviceConfig& newConfig) {
  config = newConfig;
  saveConfig();
}
