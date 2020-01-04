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
//#include<ESP8266WiFiMulti.h>
//

//Constants Declarations
const char *softSSID = "Master";    //Credentials for Master Access Point
const char *softPASS = "password";  //For the function to work, the password should be more than 8 chars and should begin with a char too

IPAddress softAP_ip(192,168,0,1);         //IP config for the soft Access Point
IPAddress softAP_gateway(192,168,0,1);
IPAddress softAP_subnet(255,255,255,0);
 
int sflag = 0;
int cflag = 0;

const long int offset = 19800;
//

//Class Object Declarations
//ESP8266WifiMulti WiFimulti_Object;
ESP8266WebServer server(80);

EnergyMonitor curr;

WiFiUDP ntp_udp_client;
NTPClient time_object(ntp_udp_client, "asia.pool.ntp.org", offset);
//

//Function Prototypes
void sta_setup(char*, char*);

void handle_root();
void handle_notfound();
void handle_disconnect_wifi();
void handle_connect_wifi();
void handle_wifi_login();
//

//Setup
void setup() {

  //Pin Setup
  pinMode(D4, OUTPUT);      //LED Indication for WiFi Connection
  digitalWrite(D4, HIGH);
  pinMode(A0, INPUT);
  
  // Variables
  int flag = 0;
  
  // Begin Serial Communication
  Serial.begin(115200);
  Serial.println("\nBegin Serial Communication With NodeMCU");

  //Begin WiFi Connections
  WiFi.mode(WIFI_AP_STA);
  //Begin WiFi Access Point Mode
  WiFi.softAPConfig(softAP_ip, softAP_gateway, softAP_subnet);
  bool ret = WiFi.softAP(softSSID, softPASS, 1, false, 6);
  if (ret == true)
  {
    Serial.print("\nSoft Access Point Started with IP: ");
    Serial.print(WiFi.softAPIP());
  }
  //Attempts WiFi Connection to the last connected network
  WiFi.begin(); 

  //HTML Server and Handles
  server.begin();
  Serial.print("\nHTTPS Server Started");
  server.on("/", HTTP_GET, handle_root);
  server.on("/disconnect_wifi", HTTP_POST, handle_disconnect_wifi);
  server.on("/connect_wifi", HTTP_POST, handle_connect_wifi);
  server.on("/wifi_login", handle_wifi_login);
  server.onNotFound(handle_notfound);

  //Current Measurement Setup
  curr.current(1, 4.8);
}
//

//Main Loop
void loop() {
  server.handleClient();            //Handle Incoming HTTP requests from Clients
  
  double curr_raw;
  if(WiFi.status() == WL_CONNECTED) //WiFi connection LED Indication
      digitalWrite(D4, LOW);
  else
    digitalWrite(D4, HIGH);
  if(time_object.getSeconds() % 10 == 0 && cflag == 0)
  {  
    curr_raw = analogRead(A0);
    curr_raw = curr.calcIrms(1480);
    Serial.println();
    Serial.print(time_object.getFormattedTime());
    Serial.println();
    Serial.print(curr_raw);
    Serial.print("\nNo. of Station Connected to AP: ");
    Serial.print(WiFi.softAPgetStationNum());
    cflag = 1;
  }
  else if(time_object.getSeconds() % 10 != 0)
    cflag = 0;
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
    delay(500);
    Serial.print(".");
    sflag++;
    if(sflag > 40)
      break;
  }
  if(sflag < 40)
  {
    digitalWrite(D4, LOW);
    Serial.print("\nConnection Established! IP: ");
    Serial.print(WiFi.localIP());
  }
  else
    Serial.print("\nConnection couldn't be Established");
  sflag = 0;
}

//
