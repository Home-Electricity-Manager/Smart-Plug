/*
 * Code for the Energy Boosters' Teams Master Smart Plug on the NodeMCU 0.9 Module
*/

//Include Required Libraries
#include "servertext.h"             // HTML for the Web Server
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>       
#include <EmonLibUser.h>            // EmonLib with Bias Value Customised to calibrate with hardware.
#include <NTPClient.h>
#include <WiFiUDP.h>
//

//Constants Declarations
#define ind_led D4                         //LED for Physical Indication of Device
#define curr_inp A0                        //Analog Pin for Current Measurement
#define conn_wp 20                         //Wait Period for WiFi Connection in seconds
#define voltage 230                        //Assumed Constant Voltage
const char *softSSID = "smartswitch";      //Credentials for Master Access Point
const char *softPASS = "smartswitch";      //For the function to work, the password should be more than 8 chars and should begin with a char too
IPAddress softAP_ip(192,168,5,1);
IPAddress softAP_gateway(192,168,5,1);     //IP config for the soft Access Point
IPAddress softAP_subnet(255,255,255,0);
int sflag = 0;
int pflag = 0;                             //Flags used for System Operations
int wflag = 0;
bool ret;                                  //For Various Connection Result Return Values 
bool time_synced = false;                  //Keeps Track of Whether Time is Set Correctly or not
const long int offset = 19800;             //+05:30 GMT
const long int auto_update_int = 85600;    //AutoSync from NTP Server Everyday

double curr_raw;
double power;
float curr_calib = 4.8;
//

//Class Object Declarations
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
//void close_ap();
//

//Setup
void setup() {

  //Pin Setup
  pinMode(ind_led, OUTPUT);      //LED Indication for WiFi Connection
  digitalWrite(ind_led, HIGH);   //WiFi Not Connected
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
}
//

//Main Loop
void loop() {
  
  if(::wflag == 0)
  {
    WiFi.begin();
    Serial.print("\nConnecting to Last Known Network");
    while(WiFi.status() != WL_CONNECTED && ::wflag < conn_wp)
    {
      Serial.print(".");
      delay(1000);
      ::wflag++;
    }
    if(::wflag == conn_wp)
    {
      Serial.print("\nWarning : Couldnt Connect to Network");
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
  }
    
  if(WiFi.status() == WL_CONNECTED) //WiFi connection LED Indication
  {
    digitalWrite(D4, LOW);
    if(time_object.getSeconds() == 0)
    {
      ret = time_object.update();
      !::time_synced && ret ? ::time_synced = true : ret ;   //Set Flag to true if previously false
    }
  }
  else
  {
    digitalWrite(D4, HIGH);
  }
  
  server.handleClient();            //Handle Incoming HTTP requests from Clients

  //Serial Print Energy and Time Data Every 10 sec
  if(time_object.getSeconds() % 10 == 0 && pflag == 0)
  { 
    //Print Config Data Every 300 sec
    if(time_object.getMinutes() % 5 == 0 && time_object.getSeconds() == 0)
    {
      Serial.print("\n\nConfig Data :");
      Serial.print("\nWiFi Connected : ");
      if(WiFi.status() == WL_CONNECTED) 
      {
        Serial.print("YES"); 
        Serial.print("\nSSID : ");
        Serial.print(WiFi.SSID());
        Serial.print("\nIP : ");
        Serial.print(WiFi.localIP());
      }
      else
        Serial.print("NO");
      
      Serial.print("\nStations Connected to AP: ");
      Serial.print(WiFi.softAPgetStationNum());
    }
    
    curr_raw = curr.calcIrms(400);
    power = voltage*curr_raw;
    Serial.print("\n\nEnergy Data : ");
    Serial.print("\nTimeStamp : ");
    Serial.print(time_object.getEpochTime());
    Serial.print("\nCurrent(A) : ");
    Serial.print(curr_raw);
    Serial.print("\nPower (W) : ");
    Serial.print(power);
    pflag = 1;
  }
  else if(time_object.getSeconds() % 10 != 0)
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
 // WiFi.disconnect(true);
 // delay(5000);
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
//
