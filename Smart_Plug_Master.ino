/*
 * Code for the Energy Boosters' Teams Master Smart Plug
*/

//Required Libraries
#include<ESP8266WiFi.h>
//#include<ESP8266WiFiMulti.h>
//#include<ESP8266mDNS.h>
//

//Constants Declarations
const char *softSSID = "Master";    //Credentials for Master Access Point
const char *softPASS = "password";  //For the function to work, the password should be more than 8 chars and should begin with a char too

const char *ssid = "alok jain";
const char *pass = "9910138138";
//

//Class Declarations
//ESP8266WifiMulti WiFimulti_Object;
//

//Function Prototypes
void LAN_setup();
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
    Serial.print("Soft Access Point Started with IP: ");
    Serial.print(WiFi.softAPIP());
  }
//  if(!WiFi.begin())                   //Checks for the presence of Previous Network Connection                                   //and Connects. Otherwise Calls LAN_setup()
    LAN_setup();
   
/*  Serial.println("\nStarting mDNS Responder");
  while(!MDNS.begin("mastersp"))
  {
    flag++;
    if(flag > 50)
      break;
  }
  flag > 50 ? Serial.println("\nCould not begin mDNS server, Consider Reset") : Serial.println("\nmDNS Responder Active");
  flag = 0; 
*/
  
}
//

//Main Loop
void loop() {
 
}
//

// Function Definitions
void LAN_setup()    
/* 
 *  Setting up for the first time OR when the Master has been reset
 *  Sets up an HTML Server and uses the Access Point to setup the STA Mode Connection
*/
{
  WiFi.begin(ssid, pass);
  Serial.println("Connecting via STA mode..");
  while(WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print("..");
  }
  Serial.print("Connection Established! IP: ");
  Serial.print(WiFi.localIP());
  //WiFimulti_Object.addAP();
}
//
