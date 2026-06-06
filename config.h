#ifndef CONFIG_H
#define CONFIG_H

// --- Hardware Pins ---
#define DOOR_RELAY_PIN 16    // Change if needed
#define LIGHT_RELAY_PIN 17   // Change if needed
#define BUTTON_PIN 0         // Onboard IO0

// --- Relay Logic (Flip these to LOW if your relay triggers on LOW) ---
#define RELAY_ON HIGH
#define RELAY_OFF LOW

// --- Networking ---
const char* ssid = "your_wifi_name";
const char* password = "your_wifi_password";

// --- Security ---
const String ACCESS_PIN = "set_your_door_access_pin"; // Set your door PIN here

#endif
