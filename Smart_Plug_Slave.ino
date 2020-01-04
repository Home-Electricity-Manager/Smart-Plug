/*
 * Code for the Energy Boosters' Team's Slave smart Plug on the NodeMCU 0.9
 */

//Include Required Libraries
#include <ESP8266WiFi.h>
#include <EmonLib.h>
#include <NTPClient.h>
#include <WiFiUDP.h>


//Constants Declarations
const char *ssid = "alok jain";    //Credentials for Connection to Master Access Point
const char *pass = "9910138138";  

WiFiUDP ntpClient;
NTPClient time_object(ntpClient, "pool.ntp.org", 19800);

void setup() {
  // put your setup code here, to run once:
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  while(WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
  }
  time_object.begin();
  time_object.update();
}

void loop() {
  // put your main code here, to run repeatedly:
  Serial.println(time_object.getFormattedTime());
}
