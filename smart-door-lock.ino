// this project impelemented by Khudhur on 1 Jun 2026
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <esp_task_wdt.h> // Official ESP32 Hardware Watchdog Library
#include "config.h"
#include "webpage.h"

// Watchdog Timeout constant (5 seconds is standard for network devices)
const int WDT_TIMEOUT_SECONDS = 5;

AsyncWebServer server(80);
Preferences preferences;

bool lightState = false;
String currentSSID;
String currentPASS;

// Global string to cache the pre-scanned network list as JSON text
String scannedNetworksJSON = "[]";

unsigned long lastWiFiCheck = 0;
const unsigned long wifiCheckInterval = 30000; 

unsigned long bootTime = 0;
const unsigned long apFallbackTimeout = 300000; // 5 Minutes before AP mode

bool apModeActive = false;
unsigned long apStartTime = 0;
const unsigned long apActiveTimeout = 300000;  

void unlockDoor() {
  digitalWrite(DOOR_RELAY_PIN, RELAY_ON);
  digitalWrite(LIGHT_RELAY_PIN, RELAY_ON);
  lightState = true;
  delay(1000); 
  digitalWrite(DOOR_RELAY_PIN, RELAY_OFF);
}

void IRAM_ATTR onButtonPress() {
  lightState = !lightState;
  digitalWrite(LIGHT_RELAY_PIN, lightState ? RELAY_ON : RELAY_OFF);
}

// Function to scan networks BEFORE launching the AP
void performPreFlightScan() {
  Serial.println("\n[SCANNER] Performing pre-flight Wi-Fi scan...");
  
  WiFi.disconnect(true);
  delay(100);
  
  int n = WiFi.scanNetworks(false, false, false, 300); 
  
  String json = "[";
  for (int i = 0; i < n; ++i) {
    String currentSSID = WiFi.SSID(i);
    // Check if SSID exists and is not already in our JSON string
    if (currentSSID.length() > 0 && json.indexOf("\"" + currentSSID + "\"") == -1) {
      json += "\"" + currentSSID + "\"";
      if (i < n - 1) json += ",";
    }
  }
  
  // Cleanup trailing comma if necessary
  if (json.endsWith(",")) json.remove(json.length() - 1);
  json += "]";
  
  scannedNetworksJSON = json;
  WiFi.scanDelete();
  
  Serial.printf("[SCANNER] Found %d networks. Snapshot cached.\n", n);
}

void startAPMode() {
  // 1. Scan the environment while the radio is unencumbered
  performPreFlightScan();

  Serial.println("[SYSTEM] Activating Backup Captive AP...");
  WiFi.mode(WIFI_AP);
  WiFi.softAP("Smart Room Access", password); 
  
  MDNS.begin("door");
  apModeActive = true;
  apStartTime = millis();
  
  Serial.print("-> Secure AP Live! Portal IP: ");
  Serial.println(WiFi.softAPIP());
}

void setup() {
  Serial.begin(115200);
  bootTime = millis();

  // --- HARDWARE WATCHDOG INITIALIZATION ---
  // Define configuration properties for the hardware watchdog timer
  esp_task_wdt_config_t wdt_config = {
      .timeout_ms = WDT_TIMEOUT_SECONDS * 1000,
      .idle_core_mask = (1 << portNUM_PROCESSORS) - 1, // Monitor system loops
      .trigger_panic = true // Force full hardware reboot if triggered
  };
  esp_task_wdt_init(&wdt_config);
  esp_task_wdt_add(NULL); // Subscribe the current main Arduino execution thread
  Serial.println("[WATCHDOG] Hardware supervisor armed successfully.");

  pinMode(DOOR_RELAY_PIN, OUTPUT);
  pinMode(LIGHT_RELAY_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  digitalWrite(DOOR_RELAY_PIN, RELAY_OFF);
  digitalWrite(LIGHT_RELAY_PIN, RELAY_OFF);

  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), onButtonPress, FALLING);

  preferences.begin("wifi-config", false);
  currentSSID = preferences.getString("ssid", ssid);
  currentPASS = preferences.getString("pass", password);

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(currentSSID.c_str(), currentPASS.c_str());

  // --- Web Server Endpoints ---

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });

  // Serve the pre-compiled layout snapshot to the client interface
  server.on("/scan", HTTP_GET, [](AsyncWebServerRequest *request){
    if (!apModeActive) {
      request->send(403, "application/json", "[]");
      return;
    }
    request->send(200, "application/json", scannedNetworksJSON);
  });

  server.on("/wifi", HTTP_GET, [](AsyncWebServerRequest *request){
    if (apModeActive) {
      request->send_P(200, "text/html", wifi_html);
    } else {
      request->send(403, "text/plain", "Forbidden: Use 'Smart Room Access' AP.");
    }
  });

  server.on("/save-wifi", HTTP_POST, [](AsyncWebServerRequest *request){
    if (!apModeActive) {
      request->send(403, "text/plain", "Forbidden");
      return;
    }
    
    if (request->hasParam("ssid", true) && request->hasParam("pass", true)) {
      String newSSID = request->getParam("ssid", true)->value();
      String newPASS = request->getParam("pass", true)->value();
      
      preferences.putString("ssid", newSSID);
      preferences.putString("pass", newPASS);
      
      request->send(200, "text/plain", "Configuration saved! Resetting device...");
      
      delay(2000);
      preferences.end();
      ESP.restart(); 
    } else {
      request->send(400, "text/plain", "Bad Submission Profile");
    }
  });

  server.on("/unlock", HTTP_POST, [](AsyncWebServerRequest *request){
    if(request->hasParam("pin", true) && request->getParam("pin", true)->value() == ACCESS_PIN){
      unlockDoor();
      request->send(200, "text/plain", "Unlocked Success");
    } else {
      request->send(401, "text/plain", "Unauthorized: Wrong PIN");
    }
  });

  server.on("/light/toggle", HTTP_POST, [](AsyncWebServerRequest *request){
    lightState = !lightState;
    digitalWrite(LIGHT_RELAY_PIN, lightState ? RELAY_ON : RELAY_OFF);
    request->send(200, "text/plain", lightState ? "Light ON" : "Light OFF");
  });

  server.begin();
  ArduinoOTA.begin();
}

void loop() {
  // Feed the hardware watchdog timer every cycle to prove the code is executing normally
  esp_task_wdt_reset();

  ArduinoOTA.handle();
  unsigned long currentMillis = millis();

  if (!apModeActive) {
    if (currentMillis - lastWiFiCheck >= wifiCheckInterval) {
      lastWiFiCheck = currentMillis;
      
      if (WiFi.status() == WL_CONNECTED) {
        static bool logLatch = false;
        if (!logLatch) {
          Serial.print("[SUCCESS] Local network interface bound. IP: ");
          Serial.println(WiFi.localIP());
          MDNS.begin("door");
          logLatch = true;
        }
      } else {
        if (currentMillis - bootTime >= apFallbackTimeout) {
          startAPMode();
        }
      }
    }
  } 
  else {
    if (currentMillis - apStartTime >= apActiveTimeout) {
      Serial.println("[SYSTEM] Timeout reached. Re-entering scanner sequence...");
      WiFi.softAPdisconnect(true);
      
      // Force status fallback parameters to scan cleanly
      WiFi.mode(WIFI_STA);
      WiFi.begin(currentSSID.c_str(), currentPASS.c_str());
      
      apModeActive = false;
      bootTime = millis(); 
    }
  }
}