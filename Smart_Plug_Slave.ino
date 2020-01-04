/*
 * Code for the Energy Boosters' Team's Slave smart Plug on the NodeMCU 0.9
 */

//Include Required Libraries
#include <ESP8266WiFi.h>
#include <EmonLib.h>
#include <NTPClient.h>
#include <WiFiUDP.h>


//Constants Declarations
const char *ssid = "Master";    //Credentials for Connection to Master Access Point
const char *pass = "password";  

void setup() {
  // put your setup code here, to run once:
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  while(WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  
}
