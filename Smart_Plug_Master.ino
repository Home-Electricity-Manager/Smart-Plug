/*
 * Code for the Energy Boosters' Teams Master Smart Plug
*/

//Required Libraries
#include<string.h>
#include "servertext.h"
#include<ESP8266WebServer.h>
#include<ESP8266WiFi.h>
//#include<ESP8266WiFiMulti.h>
//

//Constants Declarations
const char *softSSID = "Master";    //Credentials for Master Access Point
const char *softPASS = "password";  //For the function to work, the password should be more than 8 chars and should begin with a char too

char *ssid = "alok jain";
char *pass = "9910138138";
//

//Class Declarations
//ESP8266WifiMulti WiFimulti_Object;
ESP8266WebServer server(80);
//

//Function Prototypes
void LAN_setup(char*, char*);

void handle_root();
void handle_notroot();
void handle_connect_WiFi();
void handle_WiFi_login();
//

//Setup
void setup() {
  // Variables
  int flag = 0;
  
  // Begin Serial Communication
  Serial.begin(115200);
  Serial.println("\nBegin Serial Communication With NodeMCU");

  //Begin WiFi Connections
  //Begin WiFi Access Point Mode
  WiFi.mode(WIFI_AP_STA);
  bool ret = WiFi.softAP(softSSID, softPASS);
  if (ret == true)
  {
    Serial.print("\nSoft Access Point Started with IP: ");
    Serial.print(WiFi.softAPIP());
  }
  
  server.begin();
  Serial.print("\nHTTPS Server Started");
  server.on("/",HTTP_GET, handle_root);
  server.on("/Connect_WiFi",HTTP_POST, handle_connect_WiFi);
  server.on("/WiFi_login", handle_WiFi_login);
  server.onNotFound(handle_notroot);
  
  LAN_setup(ssid, pass); 
}
//

//Main Loop
void loop() {
  server.handleClient();        // Handle Incoming HTTP requests from Clients
}
//

// Function Definitions
void LAN_setup(char *ssid, char* pass)    
/* 
 *  For Setting up STA mode WiFi connections
 *  Sets up an HTML Server and uses the Access Point to get WiFi network creds from 
 *  user and setup the STA Mode Connection
*/
{
  WiFi.begin(ssid, pass);
  Serial.println("\nConnecting via STA mode");
  while(WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print("..");
  }
  Serial.print("Connection Established! IP: ");
  Serial.print(WiFi.localIP());
  //WiFimulti_Object.addAP();
}

void handle_root()
{
  if(WiFi.status() == WL_CONNECTED)
    server.send(200, "text/html", HTMLroot_conn); 
  else
    server.send(200, "text/html", HTMLroot); 
}

void handle_connect_WiFi()
{
  server.send(200, "text/html", HTMLconnect_WiFi);
}

void handle_WiFi_login()
{
  
  if(server.hasArg("SSID") && server.hasArg("pass"))
  {
//    LAN_setup();
    std::string ssid_user("");
    std::string pass_user("");
    strcpy( ssid_user, server.arg("SSID"));
//    ssid_pass += server.arg("pass");
    
    if(WiFi.status() == WL_CONNECTED)
    {
      server.sendHeader("Location", "/");
      server.send(303);
    }
    else
    {
      server.send(303);
    }
  }
  else
  {
    
  }
}

void handle_notroot()
{
  server.send(404, "text/plain", "Not Found");
}
//
