/*
 * Code for the Energy Boosters' Teams Master Smart Plug on the NodeMCU 0.9 Module
*/

//Include Required Libraries
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

int sflag = 0;
//

//Class Object Declarations
//ESP8266WifiMulti WiFimulti_Object;
ESP8266WebServer server(80);
//

//Function Prototypes
void sta_setup(char*, char*);

void handle_root();
void handle_notroot();
void handle_disconnect_wifi();
void handle_connect_wifi();
void handle_wifi_login();
//

//Setup
void setup() {
  //Pin Setup
  pinMode(D0, OUTPUT);
  digitalWrite(D0, HIGH);
  
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
  server.on("/", HTTP_GET, handle_root);
  server.on("/disconnect_wifi", HTTP_POST, handle_disconnect_wifi);
  server.on("/connect_wifi", HTTP_POST, handle_connect_wifi);
  server.on("/wifi_login", handle_wifi_login);
  server.onNotFound(handle_notroot);

  WiFi.begin(); 
}
//

//Main Loop
void loop() {
  if(WiFi.status() == WL_CONNECTED)
    digitalWrite(D0, LOW);
  else
    digitalWrite(D0, HIGH);
  server.handleClient();        // Handle Incoming HTTP requests from Clients
}
//

// Function Definitions
void sta_setup(char *s = ssid, char* p = pass)    
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
    digitalWrite(D0, LOW);
    delay(1000);
    digitalWrite(D0, HIGH);
    Serial.print(".");
    sflag++;
    if(sflag > 50)
      break;
  }
  if(sflag < 50)
  {
    digitalWrite(D0, LOW);
    Serial.print("Connection Established! IP: ");
    Serial.print(WiFi.localIP());
  }
  else
    Serial.print("Connection couldn't be Established ");
  sflag = 0;
  //WiFimulti_Object.addAP();
}

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
  digitalWrite(D0,HIGH);
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
  if(server.hasArg("SSID") && server.hasArg("pass"))
  {
    std::string ssid_user("");
    std::string pass_user("");
//    strcpy( ssid_user, server.arg("SSID"));
//    ssid_pass += server.arg("pass");
    sta_setup();
    if(WiFi.status() == WL_CONNECTED)
    {
      server.sendHeader("Location", "/");
      server.send(303);
    }
    else
    {
      sflag = 1;
      server.sendHeader("Location", "/connect_wifi");
      server.send(303);
    }
  }
  else
  {
    sflag = 1;
    server.sendHeader("Location", "/connect_wifi");
    server.send(303);
  }
}

void handle_notroot()
{
  server.sendHeader("Location", "/");
  server.send(303);
}
//
