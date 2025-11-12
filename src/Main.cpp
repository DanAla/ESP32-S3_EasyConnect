/**
 * ESP32-S3 EasyConnect Advanced Example
 * Demonstrates all framework features with sensor simulation
 */

#include <ESP32S3_EasyConnect.h>

// Sensor simulation variables
float temperature = 23.5;
float humidity = 65.2;
float pressure = 1013.25;
int ledState = 0;
unsigned long lastSensorUpdate = 0;
unsigned long lastBroadcast = 0;

void setup() {
  // Initialize framework with custom device name
  EasyConnect.begin("AdvancedSensorDevice");
  
  // Set up all available callbacks
  EasyConnect.onConnected(onWifiConnected);
  EasyConnect.onDisconnected(onWifiDisconnected);
  EasyConnect.onConfigChanged(onConfigChanged);
  EasyConnect.setCustomDataCallback(addCustomData);
  EasyConnect.onTelnetCommand(handleTelnetCommand);
  EasyConnect.onWebSocketCommand(handleWebSocketCommand);
  
  // Configure custom parameters
  DeviceConfig config = EasyConnect.getConfig();
  config.customParam1 = "Sensor Unit";
  config.customParam2 = "Room 101";
  config.customParam3 = 1;
  config.customParam4 = 1.5;
  EasyConnect.setConfig(config);
  
  // Setup LED pin for demonstration
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, ledState);
  
  EasyConnect.logln("✅ Advanced example initialized!");
  EasyConnect.logln("📊 Sensor simulation started");
  EasyConnect.logln("🔧 Custom callbacks registered");
}

void loop() {
  // Required: Handle framework operations
  EasyConnect.loop();
  
  // Update sensor readings every 2 seconds
  if (millis() - lastSensorUpdate > 2000) {
    updateSensors();
    lastSensorUpdate = millis();
  }
  
  // Broadcast status every 30 seconds
  if (millis() - lastBroadcast > 30000) {
    EasyConnect.broadcastTelnet("📢 System broadcast: Uptime " + String(EasyConnect.getUptime() / 1000) + "s, Temp: " + String(temperature) + "°C\r\n");
    EasyConnect.broadcastWebSocket("Broadcast: System running for " + String(EasyConnect.getUptime() / 1000) + " seconds");
    lastBroadcast = millis();
  }
  
  delay(100);
}

void onWifiConnected() {
  EasyConnect.logln("🎉 WiFi Connected!");
  EasyConnect.log("📱 Access dashboard: http://");
  EasyConnect.logln(EasyConnect.getIPAddress());
  EasyConnect.log("🔌 Telnet access: telnet ");
  EasyConnect.logln(EasyConnect.getIPAddress());
  
  // Broadcast connection event
  EasyConnect.broadcastTelnet("🎉 Device connected to WiFi: " + String(WiFi.SSID()) + "\r\n");
}

void onWifiDisconnected() {
  EasyConnect.logln("❌ WiFi Disconnected!");
  EasyConnect.broadcastTelnet("❌ WiFi connection lost!\r\n");
}

void onConfigChanged() {
  EasyConnect.logln("⚙️ Configuration changed - reloading settings");
  
  DeviceConfig config = EasyConnect.getConfig();
  EasyConnect.log("🔧 New device name: ");
  EasyConnect.logln(config.deviceName);
  EasyConnect.log("🎨 New theme: ");
  EasyConnect.logln(config.theme);
  
  EasyConnect.broadcastTelnet("⚙️ Configuration updated. Device: " + config.deviceName + ", Theme: " + config.theme + "\r\n");
}

void addCustomData(JsonDocument& doc) {
  // Add custom sensor data to API responses
  JsonObject sensors = doc.createNestedObject("sensors");
  sensors["temperature"] = temperature;
  sensors["humidity"] = humidity;
  sensors["pressure"] = pressure;
  sensors["ledState"] = ledState;
  
  JsonObject location = doc.createNestedObject("location");
  location["unit"] = EasyConnect.getConfig().customParam1;
  location["room"] = EasyConnect.getConfig().customParam2;
}

void handleTelnetCommand(String command, WiFiClient& client) {
  if (command == "sensors") {
    client.print("📊 Current Sensor Readings:\r\n");
    client.print("  Temperature: " + String(temperature) + " °C\r\n");
    client.print("  Humidity: " + String(humidity) + " %\r\n");
    client.print("  Pressure: " + String(pressure) + " hPa\r\n");
    client.print("  LED State: " + String(ledState ? "ON" : "OFF") + "\r\n");
    client.print("> ");
    
  } else if (command == "led on") {
    ledState = 1;
    digitalWrite(LED_BUILTIN, ledState);
    client.print("💡 LED turned ON\r\n");
    client.print("> ");
    EasyConnect.broadcastTelnet("💡 LED state changed to ON\r\n");
    
  } else if (command == "led off") {
    ledState = 0;
    digitalWrite(LED_BUILTIN, ledState);
    client.print("💡 LED turned OFF\r\n");
    client.print("> ");
    EasyConnect.broadcastTelnet("💡 LED state changed to OFF\r\n");
    
  } else if (command == "toggle") {
    ledState = !ledState;
    digitalWrite(LED_BUILTIN, ledState);
    client.print("💡 LED toggled to " + String(ledState ? "ON" : "OFF") + "\r\n");
    client.print("> ");
    EasyConnect.broadcastTelnet("💡 LED toggled to " + String(ledState ? "ON" : "OFF") + "\r\n");
    
  } else if (command == "reboot") {
    client.print("🔄 Rebooting device...\r\n");
    delay(1000);
    ESP.restart();
    
  } else if (command.startsWith("set temp ")) {
    String value = command.substring(9);
    temperature = value.toFloat();
    client.print("🌡️ Temperature set to " + String(temperature) + " °C\r\n");
    client.print("> ");
    
  } else if (command.startsWith("set hum ")) {
    String value = command.substring(8);
    humidity = value.toFloat();
    client.print("💧 Humidity set to " + String(humidity) + " %\r\n");
    client.print("> ");
    
  } else {
    client.print("❌ Unknown custom command: '" + command + "'\r\n");
    client.print("💡 Available custom commands: sensors, led on, led off, toggle, reboot, set temp X, set hum X\r\n");
    client.print("> ");
  }
}

void handleWebSocketCommand(String command, uint8_t clientNum) {
  if (command == "getSensors") {
    String sensorData = "{\"type\":\"sensorData\",\"temperature\":" + String(temperature) + 
                       ",\"humidity\":" + String(humidity) + ",\"pressure\":" + String(pressure) + 
                       ",\"ledState\":" + String(ledState) + "}";
    EasyConnect.broadcastWebSocket(sensorData);
    
  } else if (command == "toggleLED") {
    ledState = !ledState;
    digitalWrite(LED_BUILTIN, ledState);
    String response = "{\"type\":\"ledState\",\"state\":" + String(ledState) + "}";
    EasyConnect.broadcastWebSocket(response);
    EasyConnect.broadcastTelnet("💡 WebSocket: LED toggled to " + String(ledState ? "ON" : "OFF") + "\r\n");
    
  } else if (command.startsWith("setTemperature:")) {
    String value = command.substring(15);
    temperature = value.toFloat();
    String response = "{\"type\":\"temperatureSet\",\"value\":" + String(temperature) + "}";
    EasyConnect.broadcastWebSocket(response);
    
  } else {
    EasyConnect.log("❌ Unknown WebSocket command: ");
    EasyConnect.logln(command);
  }
}

void updateSensors() {
  // Simulate realistic sensor readings with some noise
  temperature += (random(-10, 11) / 10.0);
  humidity += (random(-5, 6) / 10.0);
  pressure += (random(-20, 21) / 10.0);
  
  // Keep values in realistic ranges
  temperature = constrain(temperature, 15.0, 35.0);
  humidity = constrain(humidity, 30.0, 80.0);
  pressure = constrain(pressure, 980.0, 1040.0);
  
  // Log sensor updates occasionally
  static unsigned long lastLog = 0;
  if (millis() - lastLog > 10000) {
    EasyConnect.logf("📊 Sensors - Temp: %.1f°C, Hum: %.1f%%, Press: %.1fhPa\n", temperature, humidity, pressure);
    lastLog = millis();
  }
  
  // Send sensor updates via WebSocket
  String sensorUpdate = "{\"type\":\"sensorUpdate\",\"temperature\":" + String(temperature) + 
                       ",\"humidity\":" + String(humidity) + ",\"pressure\":" + String(pressure) + "}";
  EasyConnect.broadcastWebSocket(sensorUpdate);
}
