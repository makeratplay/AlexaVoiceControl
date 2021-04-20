#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>

#include "SimpleJson.h"
#include "HueBridge.h"

// Rename the credentials.sample.h file to credentials.h and 
// edit it according to your wifi username/password
#include "credentials.h"


HueBridge hueBridge;

void setup()
{
  Serial.begin(115200);
  delay(500);
  connectWifi();
  initHueBridge();
}

void loop()
{
  hueBridge.handle();
}

void initHueBridge()
{
  // setup device name for Amazon Echo
  hueBridge.addDevice("nuclear reactor");
  hueBridge.onSetState(handle_SetState);
  hueBridge.start();
}

void handle_SetState(unsigned char id, bool state, unsigned char bri, short ct, unsigned int hue, unsigned char sat, char mode)
{
  Serial.printf_P("\nhandle_SetState id: %d, state: %s, bri: %d, ct: %d, hue: %d, sat: %d, mode: %s\n", 
    id, state ? "true": "false", bri, ct, hue, sat, mode == 'h' ? "hs" : mode == 'c' ? "ct" : "xy");

  if ( ct == 383 ){
    Serial.println("Warm white");
  }

  if ( hue == 0 && sat == 254 ){
    Serial.println("Red");
  }

  if ( hue == 21845 && sat == 254 ){
    Serial.println("Green");
  }

  if ( hue == 43690 && sat == 254 ){
    Serial.println("Blue");
  }
}

void connectWifi()
{
  // Set WIFI module to STA mode
  WiFi.mode(WIFI_STA);
  Serial.printf("Connecting to %s ", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  // Connected!
  Serial.printf("Wifi connected, SSID: %s, IP address: %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());

  // Setup Multicast DNS https://en.wikipedia.org/wiki/Multicast_DNS 
  // You can open http://nuclearreactor.local in Chrome on a desktop
  Serial.println("Setup MDNS for http://nuclearreactor.local");
  if (!MDNS.begin("nuclearreactor"))
  {
    Serial.println("Error setting up MDNS responder!");
  }
}