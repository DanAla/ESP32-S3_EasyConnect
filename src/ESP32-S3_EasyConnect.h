/**
 * ESP32-S3 EasyConnect Framework
 * Integrated framework with WiFiManager, ElegantOTA, Web Dashboard, REST API, and Telnet
 * Author: ESP32 Framework
 * Version: 1.2.0
 */

#ifndef ESP32S3_EASYCONNECT_H
#define ESP32S3_EASYCONNECT_H

#include <WiFi.h>
#include <WebServer.h>
#include <WiFiManager.h>
#include <ElegantOTA.h>
#include <ArduinoJson.h>
#include <WebSocketsServer.h>
#include <LittleFS.h>

// Default configuration structure
struct DeviceConfig {
  String deviceName;
  String theme;
  bool enableOTA;
  bool enableTelnet;
  int telnetPort;
  int updateInterval;
  String customParam1;
  String customParam2;
  int customParam3;
  float customParam4;
};

// Telnet client management
struct TelnetClient {
  WiFiClient client;
  bool connected;
  unsigned long lastActivity;
};

class ESP32S3_EasyConnect {
private:
  // Core components
  WebServer server;
  WebSocketsServer webSocket;
  WiFiManager wifiManager;
  WiFiServer telnetServer;
  
  // Configuration
  DeviceConfig config;
  const char* configFile = "/config.json";
  const char* otaUsername = "admin";
  const char* otaPassword = "admin123";
  
  // Device status
  bool isConnected = false;
  unsigned long lastUpdate = 0;
  unsigned long lastReconnectAttempt = 0;
  unsigned long deviceUptime = 0;
  
  // Telnet management
  static const int MAX_TELNET_CLIENTS = 3;
  TelnetClient telnetClients[MAX_TELNET_CLIENTS];
  bool telnetEnabled = false;
  
  // Callback function pointers
  void (*onConnectedCallback)() = nullptr;
  void (*onDisconnectedCallback)() = nullptr;
  void (*onConfigChangedCallback)() = nullptr;
  void (*customDataCallback)(JsonDocument&) = nullptr;
  void (*telnetCommandCallback)(String, WiFiClient&) = nullptr;
  void (*webSocketCommandCallback)(String, uint8_t) = nullptr;

public:
  ESP32S3_EasyConnect();
  
  // Core initialization
  bool begin(const char* deviceName = "ESP32-S3-Device");
  void loop();
  
  // Configuration management
  bool loadConfig();
  bool saveConfig();
  DeviceConfig getConfig();
  void setConfig(const DeviceConfig& newConfig);
  
  // Web interface setup
  void setupWebServer();
  void setupWebSocket();
  
  // Telnet server setup
  void setupTelnet();
  void handleTelnet();
  void broadcastTelnet(String message);
  void sendToTelnet(String message);
  
  // Logging system (Serial + Telnet)
  void log(String message);
  void logln(String message);
  void logf(const char* format, ...);
  
  // API Endpoints
  void handleRoot();
  void handleAPIStatus();
  void handleAPIConfig();
  void handleAPISystem();
  void handleAPIScan();
  void handleNotFound();
  
  // WebSocket events
  void webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length);
  
  // Utility functions
  void sendDeviceStatus();
  void restartDevice();
  void factoryReset();
  bool isWiFiConnected();
  
  // Callback setters
  void onConnected(void (*callback)());
  void onDisconnected(void (*callback)());
  void onConfigChanged(void (*callback)());
  void setCustomDataCallback(void (*callback)(JsonDocument&));
  void onTelnetCommand(void (*callback)(String, WiFiClient&));
  void onWebSocketCommand(void (*callback)(String, uint8_t));
  
  // Developer utilities
  void printDebugInfo();
  String getIPAddress();
  unsigned long getUptime();
  
  // Telnet utilities
  int getTelnetClientCount();
  void disconnectTelnetClients();
  
  // WebSocket broadcast
  void broadcastWebSocket(String message);
};

// Global instance for easy access
extern ESP32S3_EasyConnect EasyConnect;

#endif
