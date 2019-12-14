#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUDP.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

//Server Deets
#define ser "io.adafruit.com"
#define id  "Archit149"
#define key "2619ba57dee340489754ca6dac5de74b"
#define port 1883

//WiFi Creds
char *ssid = "alok jain";
char *pass = "9910138138";

//Server ini
WiFiClient client;

const long int Offset = 19800;
WiFiUDP ntpUDP;
NTPClient timeC(ntpUDP, "pool.ntp.org", Offset);

Adafruit_MQTT_Client ID1 (&client, ser, port, id, key);
Adafruit_MQTT_Publish pub_ob (&ID1, id "/feeds/test.test-state");

void setup() {
  // put your setup code here, to run once:
  pinMode(A0, INPUT);
  pinMode(4, OUTPUT);
  Serial.begin(115200);
  WiFi.begin(ssid, pass);
  while((WiFi.status() != WL_CONNECTED))
  {
    delay(100);
    Serial.print(".");
  }
  timeC.begin();
  timeC.update();
}

char rel = '1';

void loop() 
{
  MQTT_Connect();
  if ( Serial.available() ) 
  {
      rel = Serial.read();
      if (rel == '0')
        digitalWrite(4, HIGH);
      if (rel == '1')
        digitalWrite(4, LOW);
  }
  int a;
  a = (analogRead(A0)/1023)*;
  char pub[100];
  char ts[20];
  int i=0;
  long int t = timeC.getEpochTime();
  while(t != 0)
  {
    int m = t % 10;
    ts[i] = pub[i];
    i++;
  }
  Serial.print(pub);
  pub_ob.publish(pub);
}

void MQTT_Connect()
{
  if( ID!.connect() !=0 )
  {
    return();
  }
  Serial.print("Connecting to server..");
  while( ID1.connect() != 0 )
  {
    ID1.disconnect();
    delay(1000);
  }
  Serial.print("Connected");
}
