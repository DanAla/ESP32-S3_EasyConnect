# ESP32-S3 EasyConnect Framework

## Table of Contents
- [Overview](#overview)
- [Quick Start](#quick-start)
- [Installation](#installation)
- [Basic Usage](#basic-usage)
- [Advanced Features](#advanced-features)
- [API Reference](#api-reference)
- [Web Dashboard](#web-dashboard)
- [Telnet Commands](#telnet-commands)
- [OTA Updates](#ota-updates)
- [Troubleshooting](#troubleshooting)
- [Complete Examples](#complete-examples)

## Overview

ESP32-S3 EasyConnect is a comprehensive framework that integrates WiFi management, OTA updates, web dashboard, REST API, WebSocket, and Telnet server into a single easy-to-use package.

### Key Features
- ✅ **Auto WiFi Configuration** - Captive portal with network scanning
- ✅ **Web-Based OTA Updates** - Browser-based firmware and filesystem updates
- ✅ **Modern Web Dashboard** - Real-time monitoring with dark/light theme
- ✅ **RESTful API** - JSON-based configuration and status endpoints
- ✅ **WebSocket Support** - Real-time bidirectional communication
- ✅ **Telnet Server** - Parallel serial communication for remote access
- ✅ **JSON Configuration** - Persistent settings storage in LittleFS
- ✅ **Developer Friendly** - Minimal setup code with extensive callbacks

## Quick Start

### Absolute Minimum Code (3 lines)
```cpp
#include <ESP32S3_EasyConnect.h>

void setup() { EasyConnect.begin("MyDevice"); }
void loop() { EasyConnect.loop(); }
```

### Basic Example (5 lines with callbacks)
```cpp
#include <ESP32S3_EasyConnect.h>

void setup() {
  EasyConnect.begin("MyDevice");
  EasyConnect.onConnected([]() { Serial.println("WiFi Connected!"); });
}

void loop() { EasyConnect.loop(); }
```

## Installation

### PlatformIO (Recommended)

1. **Create new project:**
```bash
mkdir my-esp32-project
cd my-esp32-project
pio init --board esp32-s3-devkitc-1
```

2. **Replace `platformio.ini`:**
```ini
[env:esp32-s3-devkitc-1]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
monitor_speed = 115200
lib_deps = 
    tzapu/WiFiManager@^2.0.17
    ayushsharma82/ElegantOTA@^2.2.9
    bblanchon/ArduinoJson@^6.21.3
    links2004/WebSockets@^2.3.6
    lorol/LittleFS_ESP32@^1.0.6
board_build.filesystem = littlefs
```

3. **Project structure:**
```
project/
├── src/
│   ├── main.cpp
│   ├── ESP32S3_EasyConnect.h
│   └── ESP32S3_EasyConnect.cpp
├── data/
│   ├── index.html
│   └── style.css (optional)
└── platformio.ini
```

4. **Upload filesystem:**
```bash
pio run -t uploadfs
```

5. **Upload firmware:**
```bash
pio run -t upload
```

### Arduino IDE
1. Install required libraries:
   - WiFiManager
   - ElegantOTA
   - ArduinoJson
   - WebSockets
   - LittleFS

2. Copy `ESP32S3_EasyConnect.h` and `ESP32S3_EasyConnect.cpp` to your sketch folder

3. Use Arduino ESP32 Filesystem Uploader to upload web files

## Basic Usage

### 1. Minimal Setup
```cpp
#include <ESP32S3_EasyConnect.h>

void setup() {
  EasyConnect.begin("MySmartDevice");
}

void loop() {
  EasyConnect.loop();
  // Your application code here
  delay(100);
}
```

### 2. With Basic Callbacks
```cpp
#include <ESP32S3_EasyConnect.h>

void setup() {
  Serial.begin(115200);
  
  EasyConnect.begin("MyDevice");
  
  EasyConnect.onConnected([]() {
    Serial.println("🎉 Connected to WiFi!");
    Serial.print("📱 Dashboard: http://");
    Serial.println(EasyConnect.getIPAddress());
  });
  
  EasyConnect.onDisconnected([]() {
    Serial.println("❌ WiFi disconnected!");
  });
}

void loop() {
  EasyConnect.loop();
  delay(100);
}
```

### 3. With Configuration
```cpp
#include <ESP32S3_EasyConnect.h>

void setup() {
  EasyConnect.begin("ConfigurableDevice");
  
  EasyConnect.onConfigChanged([]() {
    Serial.println("⚙️ Configuration updated!");
    
    // Get new configuration
    DeviceConfig config = EasyConnect.getConfig();
    Serial.print("Device Name: ");
    Serial.println(config.deviceName);
    Serial.print("Theme: ");
    Serial.println(config.theme);
  });
}

void loop() {
  EasyConnect.loop();
  delay(100);
}
```

## Advanced Features

### 1. Custom Data in API Responses
```cpp
#include <ESP32S3_EasyConnect.h>

// Simulated sensor data
float temperature = 23.5;
float humidity = 65.0;

void setup() {
  EasyConnect.begin("SensorDevice");
  
  // Add custom data to API responses
  EasyConnect.setCustomDataCallback([](JsonDocument& doc) {
    doc["sensors"]["temperature"] = temperature;
    doc["sensors"]["humidity"] = humidity;
    doc["sensors"]["unit"] = "Celsius";
    doc["status"]["lastUpdate"] = millis();
  });
}

void loop() {
  EasyConnect.loop();
  
  // Update sensor readings
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate > 5000) {
    temperature += random(-10, 11) / 10.0;
    humidity += random(-5, 6) / 10.0;
    lastUpdate = millis();
  }
  
  delay(100);
}
```

### 2. Custom Telnet Commands
```cpp
#include <ESP32S3_EasyConnect.h>

int ledState = 0;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  EasyConnect.begin("TelnetDevice");
  
  // Custom telnet commands
  EasyConnect.onTelnetCommand([](String command, WiFiClient& client) {
    if (command == "led on") {
      ledState = 1;
      digitalWrite(LED_BUILTIN, HIGH);
      client.print("💡 LED turned ON\r\n> ");
      
    } else if (command == "led off") {
      ledState = 0;
      digitalWrite(LED_BUILTIN, LOW);
      client.print("💡 LED turned OFF\r\n> ");
      
    } else if (command == "toggle") {
      ledState = !ledState;
      digitalWrite(LED_BUILTIN, ledState);
      client.print("💡 LED toggled to " + String(ledState ? "ON" : "OFF") + "\r\n> ");
      
    } else if (command == "status") {
      client.print("📊 Device Status:\r\n");
      client.print("  LED: " + String(ledState ? "ON" : "OFF") + "\r\n");
      client.print("  Uptime: " + String(millis() / 1000) + "s\r\n");
      client.print("  Free RAM: " + String(ESP.getFreeHeap()) + " bytes\r\n");
      client.print("> ");
      
    } else {
      client.print("❌ Unknown command: '" + command + "'\r\n");
      client.print("💡 Try: led on, led off, toggle, status\r\n> ");
    }
  });
}

void loop() {
  EasyConnect.loop();
  delay(100);
}
```

### 3. WebSocket Custom Commands
```cpp
#include <ESP32S3_EasyConnect.h>

int counter = 0;

void setup() {
  EasyConnect.begin("WebSocketDevice");
  
  // Handle WebSocket commands
  EasyConnect.onWebSocketCommand([](String command, uint8_t clientNum) {
    if (command == "increment") {
      counter++;
      String response = "{\"type\":\"counter\",\"value\":" + String(counter) + "}";
      EasyConnect.broadcastWebSocket(response);
      
    } else if (command == "reset") {
      counter = 0;
      String response = "{\"type\":\"counter\",\"value\":0}";
      EasyConnect.broadcastWebSocket(response);
      
    } else if (command.startsWith("set:")) {
      String value = command.substring(4);
      counter = value.toInt();
      String response = "{\"type\":\"counter\",\"value\":" + String(counter) + "}";
      EasyConnect.broadcastWebSocket(response);
    }
  });
}

void loop() {
  EasyConnect.loop();
  
  // Broadcast counter every 10 seconds
  static unsigned long lastBroadcast = 0;
  if (millis() - lastBroadcast > 10000) {
    String update = "{\"type\":\"autoUpdate\",\"counter\":" + String(counter) + "}";
    EasyConnect.broadcastWebSocket(update);
    lastBroadcast = millis();
  }
  
  delay(100);
}
```

### 4. Advanced Sensor Monitoring Example
```cpp
#include <ESP32S3_EasyConnect.h>
#include <DHT.h>

#define DHT_PIN 4
#define DHT_TYPE DHT22

DHT dht(DHT_PIN, DHT_TYPE);

struct SensorData {
  float temperature;
  float humidity;
  unsigned long lastRead;
  bool error;
} sensorData;

void setup() {
  Serial.begin(115200);
  dht.begin();
  
  EasyConnect.begin("ClimateMonitor");
  
  // Custom API data
  EasyConnect.setCustomDataCallback([](JsonDocument& doc) {
    doc["sensors"]["temperature"] = sensorData.temperature;
    doc["sensors"]["humidity"] = sensorData.humidity;
    doc["sensors"]["lastRead"] = sensorData.lastRead;
    doc["sensors"]["error"] = sensorData.error;
  });
  
  // Telnet commands for sensors
  EasyConnect.onTelnetCommand([](String command, WiFiClient& client) {
    if (command == "read") {
      readSensors();
      client.print("📊 Sensor Readings:\r\n");
      client.print("  Temperature: " + String(sensorData.temperature) + "°C\r\n");
      client.print("  Humidity: " + String(sensorData.humidity) + "%\r\n");
      client.print("  Last Read: " + String((millis() - sensorData.lastRead) / 1000) + "s ago\r\n");
      client.print("> ");
    }
  });
  
  Serial.println("✅ Climate Monitor Started");
}

void loop() {
  EasyConnect.loop();
  
  // Read sensors every 30 seconds
  static unsigned long lastSensorRead = 0;
  if (millis() - lastSensorRead > 30000) {
    readSensors();
    lastSensorRead = millis();
    
    // Broadcast via WebSocket
    String sensorUpdate = "{\"type\":\"sensorUpdate\",\"temperature\":" + 
                         String(sensorData.temperature) + ",\"humidity\":" + 
                         String(sensorData.humidity) + "}";
    EasyConnect.broadcastWebSocket(sensorUpdate);
  }
  
  delay(1000);
}

void readSensors() {
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  
  if (isnan(temp) || isnan(hum)) {
    sensorData.error = true;
    EasyConnect.logln("❌ Failed to read from DHT sensor!");
  } else {
    sensorData.temperature = temp;
    sensorData.humidity = hum;
    sensorData.lastRead = millis();
    sensorData.error = false;
    
    EasyConnect.logf("📊 Temp: %.1f°C, Hum: %.1f%%\n", temp, hum);
  }
}
```

## API Reference

### Core Methods

#### `bool begin(const char* deviceName = "ESP32-S3-Device")`
Initializes the framework.
```cpp
EasyConnect.begin("MyDevice");  // Basic
EasyConnect.begin();           // Uses default name
```

#### `void loop()`
Handles framework operations (must be called in loop).
```cpp
void loop() {
  EasyConnect.loop();
  // Your code
}
```

#### `void log(String message)`
Logs message to both Serial and Telnet.
```cpp
EasyConnect.log("Sensor reading: ");
EasyConnect.logln(String(value));
EasyConnect.logf("Temperature: %.1f°C", temp);
```

### Configuration Methods

#### `DeviceConfig getConfig()`
Returns current configuration.
```cpp
DeviceConfig config = EasyConnect.getConfig();
Serial.println(config.deviceName);
Serial.println(config.theme);
```

#### `void setConfig(const DeviceConfig& newConfig)`
Updates configuration.
```cpp
DeviceConfig newConfig = EasyConnect.getConfig();
newConfig.deviceName = "New Name";
newConfig.theme = "light";
EasyConnect.setConfig(newConfig);
```

#### `bool saveConfig()`
Manually saves configuration to LittleFS.
```cpp
if (EasyConnect.saveConfig()) {
  Serial.println("Config saved");
}
```

### Callback Methods

#### Connection Callbacks
```cpp
EasyConnect.onConnected([]() {
  Serial.println("WiFi connected!");
});

EasyConnect.onDisconnected([]() {
  Serial.println("WiFi disconnected!");
});
```

#### Configuration Callback
```cpp
EasyConnect.onConfigChanged([]() {
  Serial.println("Configuration changed!");
});
```

#### Custom Data Callback
```cpp
EasyConnect.setCustomDataCallback([](JsonDocument& doc) {
  doc["custom"]["myValue"] = 42;
  doc["custom"]["timestamp"] = millis();
});
```

#### Telnet Command Callback
```cpp
EasyConnect.onTelnetCommand([](String command, WiFiClient& client) {
  if (command == "custom") {
    client.print("Custom command executed!\r\n> ");
  }
});
```

#### WebSocket Command Callback
```cpp
EasyConnect.onWebSocketCommand([](String command, uint8_t clientNum) {
  if (command == "refresh") {
    EasyConnect.broadcastWebSocket("{\"type\":\"refresh\"}");
  }
});
```

### Utility Methods

#### `String getIPAddress()`
Returns device IP address.
```cpp
String ip = EasyConnect.getIPAddress();
Serial.println("IP: " + ip);
```

#### `unsigned long getUptime()`
Returns device uptime in milliseconds.
```cpp
unsigned long uptime = EasyConnect.getUptime();
```

#### `void printDebugInfo()`
Prints debug information to Serial and Telnet.
```cpp
EasyConnect.printDebugInfo();
```

#### `void restartDevice()`
Restarts the device.
```cpp
EasyConnect.restartDevice();
```

#### `void factoryReset()`
Performs factory reset (clears WiFi and configuration).
```cpp
EasyConnect.factoryReset();
```

### Telnet Methods

#### `void broadcastTelnet(String message)`
Sends message to all connected Telnet clients.
```cpp
EasyConnect.broadcastTelnet("System broadcast message\r\n");
```

#### `int getTelnetClientCount()`
Returns number of connected Telnet clients.
```cpp
int clients = EasyConnect.getTelnetClientCount();
```

### WebSocket Methods

#### `void broadcastWebSocket(String message)`
Broadcasts message to all WebSocket clients.
```cpp
EasyConnect.broadcastWebSocket("{\"type\":\"update\"}");
```

## Web Dashboard

### Access Points
- **Main Dashboard**: `http://[device-ip]/index.html`
- **OTA Updates**: `http://[device-ip]/update`
- **REST API**: `http://[device-ip]/api/status`

### Dashboard Features
- Real-time device status monitoring
- Sensor data visualization
- Theme toggle (dark/light)
- System controls (restart, factory reset)
- Live log display
- Configuration management

### Customizing the Dashboard
Modify `data/index.html` to add custom controls and visualizations.

## Telnet Commands

### Built-in Commands
```bash
telnet 192.168.1.100  # Replace with your device IP

help        # Show all available commands
status      # Display device status
restart     # Restart the device
factoryreset # Factory reset (clears all settings)
clients     # Show connected Telnet clients
wifi        # Show WiFi information
memory      # Show memory usage
config      # Show current configuration
clear       # Clear the screen (cls also works)
disconnect  # Disconnect current session
```

### Custom Command Example
```cpp
EasyConnect.onTelnetCommand([](String command, WiFiClient& client) {
  if (command == "custom") {
    client.print("This is a custom command!\r\n");
    client.print("Device uptime: " + String(millis() / 1000) + "s\r\n");
    client.print("> ");
  }
});
```

## OTA Updates

### Web-based OTA
1. Access `http://[device-ip]/update`
2. Username: `admin`, Password: `admin123`
3. Upload new firmware or filesystem

### API Endpoints for OTA
- `GET /update` - OTA update page
- `POST /update` - Firmware upload
- `POST /updatefs` - Filesystem upload

### Secure OTA Example
```cpp
void setup() {
  EasyConnect.begin("SecureDevice");
  
  // Change OTA credentials
  // Note: You'll need to modify the framework code for this
  // Currently uses: admin/admin123
}
```

## REST API Reference

### GET `/api/status`
Returns device status and sensor data.
```json
{
  "device": {
    "name": "MyDevice",
    "chipId": "a1b2c3d4",
    "freeHeap": 123456,
    "uptime": 1234567
  },
  "wifi": {
    "connected": true,
    "ssid": "MyWiFi",
    "rssi": -65,
    "ip": "192.168.1.100"
  },
  "sensors": {
    "temperature": 23.5,
    "humidity": 65.2
  }
}
```

### GET `/api/config`
Returns current configuration.
```json
{
  "deviceName": "MyDevice",
  "theme": "dark",
  "enableOTA": true,
  "enableTelnet": true,
  "updateInterval": 5000,
  "customParam1": "value1",
  "customParam2": "value2"
}
```

### POST `/api/config`
Updates configuration.
```json
{
  "deviceName": "NewName",
  "theme": "light",
  "enableOTA": false
}
```

### POST `/api/system?action=restart`
Restarts the device.

### POST `/api/system?action=factoryReset`
Performs factory reset.

### GET `/api/scan`
Scans for available WiFi networks.

## Complete Examples

### Example 1: Smart Home Controller
```cpp
#include <ESP32S3_EasyConnect.h>

// Device states
bool livingRoomLight = false;
bool bedroomLight = false;
int thermostatTemp = 22;

void setup() {
  EasyConnect.begin("SmartHomeController");
  
  // Add device states to API
  EasyConnect.setCustomDataCallback([](JsonDocument& doc) {
    doc["devices"]["livingRoomLight"] = livingRoomLight;
    doc["devices"]["bedroomLight"] = bedroomLight;
    doc["devices"]["thermostat"] = thermostatTemp;
  });
  
  // Telnet commands for home control
  EasyConnect.onTelnetCommand([](String command, WiFiClient& client) {
    if (command == "lights on") {
      livingRoomLight = true;
      bedroomLight = true;
      client.print("💡 All lights turned ON\r\n> ");
      
    } else if (command == "lights off") {
      livingRoomLight = false;
      bedroomLight = false;
      client.print("💡 All lights turned OFF\r\n> ");
      
    } else if (command.startsWith("thermostat ")) {
      String tempStr = command.substring(11);
      thermostatTemp = tempStr.toInt();
      client.print("🌡️ Thermostat set to " + String(thermostatTemp) + "°C\r\n> ");
      
    } else if (command == "status") {
      client.print("🏠 Smart Home Status:\r\n");
      client.print("  Living Room Light: " + String(livingRoomLight ? "ON" : "OFF") + "\r\n");
      client.print("  Bedroom Light: " + String(bedroomLight ? "ON" : "OFF") + "\r\n");
      client.print("  Thermostat: " + String(thermostatTemp) + "°C\r\n");
      client.print("> ");
    }
  });
  
  // WebSocket commands
  EasyConnect.onWebSocketCommand([](String command, uint8_t clientNum) {
    if (command == "toggleLivingRoom") {
      livingRoomLight = !livingRoomLight;
      String response = "{\"type\":\"livingRoomLight\",\"state\":" + 
                       String(livingRoomLight) + "}";
      EasyConnect.broadcastWebSocket(response);
      
    } else if (command == "toggleBedroom") {
      bedroomLight = !bedroomLight;
      String response = "{\"type\":\"bedroomLight\",\"state\":" + 
                       String(bedroomLight) + "}";
      EasyConnect.broadcastWebSocket(response);
    }
  });
}

void loop() {
  EasyConnect.loop();
  
  // Broadcast status every 30 seconds
  static unsigned long lastBroadcast = 0;
  if (millis() - lastBroadcast > 30000) {
    String status = "{\"type\":\"homeStatus\",\"livingRoomLight\":" + 
                   String(livingRoomLight) + ",\"bedroomLight\":" + 
                   String(bedroomLight) + ",\"thermostat\":" + 
                   String(thermostatTemp) + "}";
    EasyConnect.broadcastWebSocket(status);
    lastBroadcast = millis();
  }
  
  delay(1000);
}
```

### Example 2: Industrial Monitor
```cpp
#include <ESP32S3_EasyConnect.h>

struct IndustrialData {
  float voltage = 230.0;
  float current = 15.5;
  float power = 3500.0;
  float frequency = 50.0;
  bool alarm = false;
  unsigned long sampleCount = 0;
} industrialData;

void setup() {
  EasyConnect.begin("IndustrialMonitor");
  
  // Industrial data in API
  EasyConnect.setCustomDataCallback([](JsonDocument& doc) {
    doc["industrial"]["voltage"] = industrialData.voltage;
    doc["industrial"]["current"] = industrialData.current;
    doc["industrial"]["power"] = industrialData.power;
    doc["industrial"]["frequency"] = industrialData.frequency;
    doc["industrial"]["alarm"] = industrialData.alarm;
    doc["industrial"]["samples"] = industrialData.sampleCount;
  });
  
  // Telnet commands for industrial control
  EasyConnect.onTelnetCommand([](String command, WiFiClient& client) {
    if (command == "data") {
      client.print("🏭 Industrial Data:\r\n");
      client.print("  Voltage: " + String(industrialData.voltage) + "V\r\n");
      client.print("  Current: " + String(industrialData.current) + "A\r\n");
      client.print("  Power: " + String(industrialData.power) + "W\r\n");
      client.print("  Frequency: " + String(industrialData.frequency) + "Hz\r\n");
      client.print("  Alarm: " + String(industrialData.alarm ? "ACTIVE" : "INACTIVE") + "\r\n");
      client.print("  Samples: " + String(industrialData.sampleCount) + "\r\n");
      client.print("> ");
      
    } else if (command == "reset alarm") {
      industrialData.alarm = false;
      client.print("🚨 Alarm reset\r\n> ");
      EasyConnect.broadcastTelnet("🚨 Alarm manually reset\r\n");
      
    } else if (command == "simulate alarm") {
      industrialData.alarm = true;
      client.print("🚨 Alarm simulated\r\n> ");
      EasyConnect.broadcastTelnet("🚨 ALARM TRIGGERED!\r\n");
    }
  });
}

void loop() {
  EasyConnect.loop();
  
  // Simulate industrial data changes
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate > 2000) {
    industrialData.voltage += random(-10, 11) / 10.0;
    industrialData.current += random(-5, 6) / 10.0;
    industrialData.power = industrialData.voltage * industrialData.current;
    industrialData.frequency = 50.0 + random(-5, 6) / 10.0;
    industrialData.sampleCount++;
    
    // Random alarm simulation (1% chance)
    if (random(100) == 0) {
      industrialData.alarm = true;
      EasyConnect.broadcastTelnet("🚨 CRITICAL: Power anomaly detected!\r\n");
      EasyConnect.broadcastWebSocket("{\"type\":\"alarm\",\"message\":\"Power anomaly\"}");
    }
    
    // Broadcast data via WebSocket
    String dataUpdate = "{\"type\":\"industrialUpdate\",\"voltage\":" + 
                       String(industrialData.voltage) + ",\"current\":" + 
                       String(industrialData.current) + ",\"power\":" + 
                       String(industrialData.power) + ",\"alarm\":" + 
                       String(industrialData.alarm) + "}";
    EasyConnect.broadcastWebSocket(dataUpdate);
    
    lastUpdate = millis();
  }
  
  delay(100);
}
```

### Example 3: Data Logger with Remote Access
```cpp
#include <ESP32S3_EasyConnect.h>

struct LogEntry {
  unsigned long timestamp;
  float value1;
  float value2;
  String note;
};

std::vector<LogEntry> dataLog;
unsigned long logCounter = 0;

void setup() {
  EasyConnect.begin("DataLogger");
  
  // Add logging data to API
  EasyConnect.setCustomDataCallback([](JsonDocument& doc) {
    doc["logging"]["totalEntries"] = dataLog.size();
    doc["logging"]["lastEntry"] = dataLog.empty() ? 0 : dataLog.back().timestamp;
    doc["logging"]["memoryUsage"] = sizeof(LogEntry) * dataLog.size();
  });
  
  // Telnet commands for data logging
  EasyConnect.onTelnetCommand([](String command, WiFiClient& client) {
    if (command == "log status") {
      client.print("📊 Data Log Status:\r\n");
      client.print("  Total entries: " + String(dataLog.size()) + "\r\n");
      client.print("  Memory used: " + String(sizeof(LogEntry) * dataLog.size()) + " bytes\r\n");
      if (!dataLog.empty()) {
        client.print("  Last entry: " + String(dataLog.back().timestamp) + "\r\n");
      }
      client.print("> ");
      
    } else if (command == "log clear") {
      dataLog.clear();
      client.print("🗑️ Log cleared\r\n> ");
      EasyConnect.broadcastTelnet("🗑️ Data log cleared by remote command\r\n");
      
    } else if (command == "log add") {
      LogEntry entry;
      entry.timestamp = millis();
      entry.value1 = random(1000) / 10.0;
      entry.value2 = random(1000) / 10.0;
      entry.note = "Auto-generated";
      dataLog.push_back(entry);
      client.print("➕ Log entry added: " + String(entry.value1) + ", " + String(entry.value2) + "\r\n> ");
      
    } else if (command == "log last") {
      if (dataLog.empty()) {
        client.print("❌ No entries in log\r\n> ");
      } else {
        auto& entry = dataLog.back();
        client.print("📝 Last log entry:\r\n");
        client.print("  Time: " + String(entry.timestamp) + "\r\n");
        client.print("  Value1: " + String(entry.value1) + "\r\n");
        client.print("  Value2: " + String(entry.value2) + "\r\n");
        client.print("  Note: " + entry.note + "\r\n");
        client.print("> ");
      }
    }
  });
  
  // WebSocket commands for logging
  EasyConnect.onWebSocketCommand([](String command, uint8_t clientNum) {
    if (command == "getLogStats") {
      String stats = "{\"type\":\"logStats\",\"entries\":" + 
                    String(dataLog.size()) + ",\"memory\":" + 
                    String(sizeof(LogEntry) * dataLog.size()) + "}";
      EasyConnect.broadcastWebSocket(stats);
    }
  });
}

void loop() {
  EasyConnect.loop();
  
  // Auto-add log entry every minute
  static unsigned long lastAutoLog = 0;
  if (millis() - lastAutoLog > 60000) {
    LogEntry entry;
    entry.timestamp = millis();
    entry.value1 = random(1000) / 10.0;
    entry.value2 = random(1000) / 10.0;
    entry.note = "Auto-log #" + String(++logCounter);
    dataLog.push_back(entry);
    
    EasyConnect.logln("➕ Auto-log: " + String(entry.value1) + ", " + String(entry.value2));
    lastAutoLog = millis();
  }
  
  delay(1000);
}
```

## Troubleshooting

### Common Issues

#### 1. WiFi Connection Problems
```cpp
// Enable debug output
Serial.begin(115200);
EasyConnect.begin("MyDevice");

// Check connection status
if (!EasyConnect.isWiFiConnected()) {
  Serial.println("WiFi not connected!");
}
```

#### 2. Filesystem Mount Failed
- Ensure LittleFS is properly initialized
- Check if filesystem image was uploaded
- Verify platformio.ini configuration

#### 3. OTA Update Fails
- Check network connectivity
- Verify sufficient flash memory
- Ensure correct firmware file format

#### 4. Telnet Connection Refused
- Verify Telnet is enabled in configuration
- Check firewall settings
- Ensure correct port (default: 23)

### Debugging Tips

1. **Enable Serial Debugging**
```cpp
Serial.begin(115200);
EasyConnect.begin("DebugDevice");
EasyConnect.printDebugInfo();
```

2. **Check Configuration**
```cpp
DeviceConfig config = EasyConnect.getConfig();
Serial.println("Device: " + config.deviceName);
Serial.println("Telnet: " + String(config.enableTelnet));
```

3. **Monitor Network Status**
```cpp
void loop() {
  EasyConnect.loop();
  
  static unsigned long lastCheck = 0;
  if (millis() - lastCheck > 30000) {
    Serial.println("WiFi: " + String(EasyConnect.isWiFiConnected() ? "Connected" : "Disconnected"));
    Serial.println("Telnet clients: " + String(EasyConnect.getTelnetClientCount()));
    lastCheck = millis();
  }
}
```

### Performance Optimization

1. **Adjust Update Intervals**
```cpp
DeviceConfig config = EasyConnect.getConfig();
config.updateInterval = 10000; // 10 seconds instead of 5
EasyConnect.setConfig(config);
```

2. **Disable Unused Features**
```cpp
DeviceConfig config = EasyConnect.getConfig();
config.enableTelnet = false; // Disable Telnet if not needed
config.enableOTA = false;    // Disable OTA in production
EasyConnect.setConfig(config);
```

## File Structure Reference

### Core Files
- `ESP32S3_EasyConnect.h` - Main header file with class definition
- `ESP32S3_EasyConnect.cpp` - Implementation file
- `main.cpp` - Your application code

### Web Files (in data folder)
- `index.html` - Web dashboard
- `style.css` - Optional additional styles

### Configuration Files
- `/config.json` - Automatically created in LittleFS

## Support

For issues and questions:
1. Check this manual first
2. Verify your configuration
3. Enable debug output
4. Check Serial and Telnet logs

## License

BSD3 License - Feel free to use in personal and commercial projects.

---

### Version History
- **v1.0.0** - Initial release with basic features
- **v1.1.0** - Added Telnet server support
- **v1.2.0** - Enhanced WebSocket and callback system

### Contributing
Feel free to contribute to this project by submitting issues or pull requests on the GitHub repository.
