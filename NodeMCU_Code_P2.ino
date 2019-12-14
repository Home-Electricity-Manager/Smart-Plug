#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUDP.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

#define WiFi_LED 13
#define server_LED 12
#define relay 4
#define Amp_Inp A0

//Adafruit Server details
#define AIO_Server    "io.adafruit.com"
#define AIO_Port      1883
#define AIO_ID        "Archit149"
#define AIO_Key       "2619ba57dee340489754ca6dac5de74b"

//WiFi Credentials
char *ssid = "Python";
char *pass = "oneplusmotorolla1+";

//Adafruit Server and Client initiations
WiFiClient client;

Adafruit_MQTT_Client Client_Obj(&client, AIO_Server, AIO_Port, AIO_ID, AIO_Key);
Adafruit_MQTT_Publish Publish_Obj_Ts = Adafruit_MQTT_Publish(&Client_Obj, AIO_ID "/feeds/test.test-time-stamp");
Adafruit_MQTT_Publish Publish_Obj_p = Adafruit_MQTT_Publish(&Client_Obj, AIO_ID "/feeds/test.test-power");
//Adafruit_MQTT_Subscribe Sub_Obj = Adafruit_MQTT_Subscribe(&Client_Obj, AIO_ID "/feeds/onoff");

//NTPClient and Offset info
const long int Offset = 19800;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", Offset);

//Current Measurements
float raw;

void MQTT_connect();
void setup()
{
  pinMode(WiFi_LED, OUTPUT);
  pinMode(server_LED, OUTPUT);
  pinMode(Amp_Inp, INPUT);
  Serial.begin(115200);
  WiFi.begin(ssid, pass);
  Serial.println();
  Serial.print("Connecting to WiFi: ");
  Serial.print(ssid);
  int i = 0;
  while ((WiFi.status() != WL_CONNECTED) && i < 50)
  {
    digitalWrite(WiFi_LED, HIGH);
    delay(100);
    digitalWrite(WiFi_LED, LOW);
    delay(100);
    Serial.print(".");
    i++;
  }
  if (i < 50)
  {
    Serial.println("Successfully Connected");
    digitalWrite(WiFi_LED, HIGH);
    Serial.println("IP Address: ");
    Serial.println(WiFi.localIP());
  }
  else
  {
    Serial.println("Connection Failed");
    digitalWrite(WiFi_LED, LOW);
    while (1)
    {}
  }
  timeClient.begin();
  timeClient.update();
//  Client_Obj.subscribe(&Sub_Obj);
}


void loop() {
  MQTT_connect();
  raw = analogRead(Amp_Inp);
  char rel;
  
//  raw/=1023;
//  raw*=3.3;
//  raw=(raw*1000-2500)*66;
  Serial.println(timeClient.getFormattedTime());
  Serial.println(raw);
  
  if(timeClient.getSeconds()%15==0)
  {
    Publish_Obj_Ts.publish((uint32_t)timeClient.getEpochTime());
  }
  delay(1000);
}

void MQTT_connect()
{
  if (Client_Obj.connected())
  {
    digitalWrite(server_LED, HIGH);
    return;
  }
  Serial.print("Connecting to Server...");
  while ( Client_Obj.connect() != 0)
  {
    Client_Obj.disconnect();
    Serial.println("Retrying in 2 seconds");
    digitalWrite(server_LED, HIGH);
    delay(1000);
    digitalWrite(server_LED, LOW);
    delay(1000);
  }
  Serial.println("Connected");
  digitalWrite(server_LED, HIGH);
}
