/*
 * Code for the Energy Boosters' Teams Master Smart Plug on the NodeMCU 0.9 Module
*/

//Include Required Libraries
#include "servertext.h"             // HTML for the Web Server
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>       
#include <EmonLib.h>
#include <NTPClient.h>
#include <WiFiUDP.h>
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>
//

//Constants Declarations
#define aio_server      "io.adafruit.com"
#define aio_serverport  1883
#define aio_username    "Archit149"
#define aio_key         "2619ba57dee340489754ca6dac5de74b"
#define po_ts_feed      "Archit149/feeds/energymonitor.tv-po-ts"
#define subs_feed       "Archit149/feeds/energymonitor.schedule"

#define ind_led D4                         //LED for Physical Indication of Device
#define pub_led D0                         //LED for Publish Success Indication
#define curr_inp A0                        //Analog Pin for Current Measurement
#define conn_wp 20                         //Wait Period for WiFi Connection in seconds
#define voltage 230                        //Assumed Constant Voltage
#define curr_calib 5                       //Current Calibration Factor
#define spie 30                            //Serial Print and Cloud Publish Interval for Energy Values 
#define spic 300                           //Serial Print and Time Force Update Interval for Config Values
const char *softSSID = "smartswitch";      //Credentials for Master Access Point
const char *softPASS = "smartswitch";      //For the function to work, the password should be more than 8 chars and should begin with a char too
IPAddress softAP_ip(192,168,5,1);
IPAddress softAP_gateway(192,168,5,1);     //IP config for the soft Access Point
IPAddress softAP_subnet(255,255,255,0);
            //Flags used for System Operations
int sflag = 0;                             //sflag: used in setup for WiFi Connection Time Out and later in HTTPS Server Functions to check for connection success
int pflag = 0;                             //pflag: used to make sure power measurement is run one time only and based on current time (timestamp)
bool ret;                                  //For Various Connection Result Return Values 
bool time_synced = false;                  //Keeps Track of Whether Time is Set Correctly or not
const long int offset = 19800;             //+05:30 GMT
const long int auto_update_int = 85600;    //AutoSync from NTP Server Everyday

long int timestamp;
double curr_raw;
double power;
char po_ts[20];                            //Contains Combined value of Power and TimeStamp
//

//Class Object Declarations
WiFiClient client;

Adafruit_MQTT_Client mqtt_object(&client, aio_server, aio_serverport, aio_username, aio_key);
Adafruit_MQTT_Publish po_ts_object(&mqtt_object, po_ts_feed);

ESP8266WebServer server(80);

EnergyMonitor curr;

WiFiUDP ntp_udp_client;
NTPClient time_object(ntp_udp_client, "time.nist.gov", offset, auto_update_int);
//

//Function Prototypes
void handle_root();
void handle_notfound();
void handle_disconnect_wifi();
void handle_connect_wifi();
void handle_wifi_login();

void sta_setup(char*, char*);
void begin_ap();
void mqtt_connect();
//

//Setup
void setup() 
{
  //Pin Setup
  pinMode(ind_led, OUTPUT);      //LED Indication for WiFi Connection
  pinMode(pub_led, OUTPUT);      //LED Indication for MQTT Publish
  digitalWrite(ind_led, HIGH);   //WiFi Not Connected
  digitalWrite(pub_led, HIGH);   //Not Publishing 
  pinMode(curr_inp, INPUT);      //Current Measurement
  
  // Begin Serial Communication with Arduino IDE
  Serial.begin(115200);
  Serial.print("\nSuccesfully began Serial Communication With NodeMCU\n");

  //Begin WiFi Mode
  WiFi.mode(WIFI_AP_STA);   //Declares the WiFi Mode as Station plus Access Point

  //WiFi Access Point Mode Moved to an External function declared as begin_ap();
  begin_ap();
  
  //HTML Server and Handles
  server.begin();
  Serial.print("\nStarting HTTPS WebServer");
  server.on("/", HTTP_GET, handle_root);
  server.on("/disconnect_wifi", HTTP_POST, handle_disconnect_wifi);
  server.on("/connect_wifi", HTTP_POST, handle_connect_wifi);
  server.on("/wifi_login", handle_wifi_login);
  server.onNotFound(handle_notfound);

  //Current Measurement Setup
  curr.current(curr_inp, curr_calib);

  // Time Keeping Begins
  time_object.begin();

  po_ts[20] = '\0';
  po_ts[15] = '.';
  po_ts[10] = ',';

  WiFi.begin();
  Serial.print("\nConnecting to Last Known Network");
  while(WiFi.status() != WL_CONNECTED && ::sflag < conn_wp)
  {
    Serial.print(".");
    delay(1000);
    ::sflag++;
  }
  if(sflag == conn_wp)
  {
    Serial.print("\nWarning : Couldnt Connect to Network");
    WiFi.disconnect();
    delay(1000);
  }
  else
  {
    Serial.print("\nConnection Successful to: ");
    Serial.print(WiFi.SSID());
    Serial.print("\nIP Address: ");
    Serial.print(WiFi.localIP());
    ret = time_object.update();
    ret ? Serial.print("\nTime Sync Successful") :  Serial.print("\nWarning : Time Sync Failed");
    ret ? ::time_synced = true : ::time_synced = false;
  }
  ::sflag = 0;
  mqtt_connect();
}
//

//Main Loop
void loop() {
      
  server.handleClient();            //Handle Incoming HTTP requests from Clients

  if(WiFi.status() == WL_CONNECTED) //WiFi connection LED Indication
  {
    digitalWrite(D4, LOW);
    if(time_object.getEpochTime() % spic == 0)
    {
      ret = time_object.update();
      !::time_synced && ret ? ::time_synced = true : ret ;   //Set Flag to true if previously false
    }
  }
  else
    digitalWrite(D4, HIGH);

  //Serial Print and Publish Energy and Time Data Every spie seconds 
  if(time_object.getSeconds() % spie == 0 && pflag == 0)
  { 
    //Print Config Data Every spic seconds
    if(time_object.getEpochTime() % spic == 0)
    {
      mqtt_object.ping();
      Serial.print("\n\nConfig Data :");
      Serial.print("\nWiFi Connected : ");
      if(WiFi.status() == WL_CONNECTED) 
      {
        Serial.print("Yes"); 
        Serial.print("\nSSID : ");
        Serial.print(WiFi.SSID());
        Serial.print("\nIP : ");
        Serial.print(WiFi.localIP());
      }
      else
        Serial.print("No");
      
      Serial.print("\nStations Connected to AP: ");
      Serial.print(WiFi.softAPgetStationNum());
      Serial.print("\nTime Synced : ");
      ::time_synced ? Serial.print("Yes") : Serial.print("No");
      Serial.print("\nMQTT Connection : ");
      mqtt_object.connected() ? Serial.print("Alive") : Serial.print("Dead");  
    }
    
    curr_raw = curr.calcIrms(1480);
    power = voltage*curr_raw;
    timestamp = time_object.getEpochTime();
    
    int i, p, t;
    t = timestamp;
    for(i = 9; i >= 0; i--)
    {
      po_ts[i] = t % 10 + 48;
      t = t - t % 10;
      t /= 10;
    }
    for(i = 3; i > -5; i--)
    {
      p = power / pow(10, i);
      p = p % 10;
      if (i >= 0)
        po_ts[-i+14] = p + 48;
      else
        po_ts[-i+15] = p + 48;
    }
    po_ts[20] = '\0';
    
    Serial.print("\n\nEnergy Data : ");
    Serial.print("\nTimeStamp : ");
    Serial.print(timestamp);
    Serial.print("\nCurrent(A) : ");
    Serial.print(curr_raw);
    Serial.print("\nPower (W) : ");
    Serial.print(power);
    Serial.println();
    if(mqtt_object.connected() && time_synced)
    {
      bool res;
      res = po_ts_object.publish(po_ts);
      if(res)
      {
        digitalWrite(pub_led, LOW);
        delay(200);
        digitalWrite(pub_led, HIGH);
        delay(200);
      }
    }
    else
    {
      if(time_synced)
      {
        Serial.print("\nMQTT Connection Lost.. Reconnecting");
        mqtt_connect();
      }
    }
    pflag = 1;
  }
  else if(time_object.getSeconds() % spie != 0)
    pflag = 0;
}
//

// Function Definitions
void handle_root()
{
  if( WiFi.status() == WL_CONNECTED )
    server.send(200, "text/html", html_root_conn); 
  else
    server.send(200, "text/html", html_root); 
}

void handle_disconnect_wifi()
{
  WiFi.disconnect();
  digitalWrite(D4,HIGH);
  Serial.println("WiFi Disconnected\n");
  server.sendHeader("Location", "/");
  server.send(303);
}

void handle_connect_wifi()
{
  if(sflag == 1)
      server.send(200, "text/html", html_connect_wifi_tryagain);
  else
      server.send(200, "text/html", html_connect_wifi);
  sflag = 0;
}

void handle_wifi_login()
{
  if(server.hasArg("ssid") && server.hasArg("pass"))
  {
    const String ssid_user(server.arg("ssid"));   // Recieve SSID and password from the User using the web application
    const String pass_user(server.arg("pass"));

    int i = 0;
    while(ssid_user[i] != '\0')       //Parsing the SSID and storing it to a char pointer
      i++;
    char* s = new char[i+1];
    for(int j=0; j<i; j++)
    {
      if(ssid_user[j]=='+')
        s[j] = ' ';
      else
        s[j] = ssid_user[j];
      s[j+1] = '\0';
    }
    Serial.print("\n\nRequest Received for STA WiFi Connection : ");
    Serial.print("\nSSID: ");
    Serial.print(s);

    i = 0;
    while(pass_user[i] != '\0')       //Parsing the password and storing it to a char pointer
      i++;
    char* p = new char[i+1];
    for(int j=0; j<i; j++)
    {
      if(pass_user[j]=='+')
        p[j] = ' ';
      else
        p[j] = pass_user[j];
      p[j+1] = '\0';
    }
    Serial.print("\nPass: ");
    Serial.print(p);
    
    sta_setup(s, p);                  //Setup WiFi connection using the given credentials
    
    if(WiFi.status() == WL_CONNECTED) //Redirects to home page if Connectd successfully
    {
      server.sendHeader("Location", "/");
      server.send(303);
    }
    else                              //Redirect to connect page if cannot connect. 
    {                                 //BUG 1
      sflag = 1;
      server.sendHeader("Location", "/connect_wifi");
      server.send(303);
    }
    
    delete[] p;                       // Delete dynamically allocated memory
    delete[] s;
  }
  else
  {
    sflag = 1;
    server.sendHeader("Location", "/connect_wifi");
    server.send(303);
  }
}

void handle_notfound()
{
  server.sendHeader("Location", "/");
  server.send(303);
}

void sta_setup(char *s, char* p)    
{
  /* 
  *  For Setting up STA mode WiFi connections via user input.
  *  Uses the HTML Server through the Soft Access Point to get WiFi network creds from 
  *  user and setup the STA Mode Connection
  */
  WiFi.begin(s, p);
  Serial.println("\nConnecting via STA mode");
  while(WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.print(".");
    sflag++;
    if(sflag > conn_wp)
      break;
  }
  if(sflag < conn_wp)
  {
    digitalWrite(D4, LOW);
    Serial.print("\nConnection Established! IP: ");
    Serial.print(WiFi.localIP());
    if(!::time_synced)
    {
      ret = time_object.update();
      ret ? ::time_synced = true : ::time_synced = false;
    }
    mqtt_connect();
  }
  else
    Serial.print("\nConnection couldn't be Established");
  sflag = 0;
}

void begin_ap()
{
  WiFi.softAPConfig(softAP_ip, softAP_gateway, softAP_subnet);
  ret = WiFi.softAP(softSSID, softPASS);
  if (ret == true)
  {
    Serial.print("\nSoft Access Point Started with IP: ");
    Serial.print(WiFi.softAPIP());
  }
}

void mqtt_connect()
{
  int r = 1;
  if(mqtt_object.connected())
    return;
  if(WiFi.status() != WL_CONNECTED)
    r = -2;
  Serial.print("\nAdafruit MQTT");
  while((ret = mqtt_object.connect()) != 0 && r != -2)
  {
    Serial.println();
    Serial.print("\nRetrying MQTT connection in 1 second...");
    delay(750);
    mqtt_object.disconnect();
    delay(750);
    r--;
    if( r < 0 )
      break;
    else
      continue;
  }
  r >= 0 ? Serial.print("\nMQTT Connected"): Serial.print("\nWarning : Cannot Connect to MQTT Service, data would not be published");
}
//
