/*
 * Code for the Energy Boosters' Teams Smart Plug Single Socket on the NodeMCU 0.9 Module
 * Developed by Archit Jain
*/

//Include Required Libraries
#include "servertext.h"             // HTML for the Web Server
#include <ESP8266WiFi.h>            //WiFI Control Library
#include <ESP8266WebServer.h>       //WebServer for Network Connection
#include <EmonLib.h>                //Open Source Energy monitor Library
#include <NTPClient.h>              //NTP Client for Time Sync
#include <WiFiUDP.h>                //Used for Adafruit MQTT
#include <Adafruit_MQTT.h>          //Adafruit MQTT
#include <Adafruit_MQTT_Client.h>   //Adafruit MQTT Client
//

//Constants Declarations

//MQTT Broker Values
#define aio_server      "io.adafruit.com"
#define aio_serverport  0000 //Ommitted
#define aio_username    "//Ommitted"
#define aio_key         "//Ommitted"
#define po_ts_feed      "//Ommitted"
#define subs_feed       "//Ommitted"

//Pin Bindings
#define ind_led             D4                  //LED for Physical Indication of Device
#define mqtt_pub_led        D4                  //LED for Publish Success Indication
#define ap_control_pin      D8                  //For Externaly Controlling AP Enable Disable
#define op_select           D0                  //For Relay Control
#define curr_inp            A0                  //Analog Pin for Current Measurement

//Intervals and Value Constants
#define conn_wp                     20          //Wait Period for WiFi Connection in seconds
#define voltage                     230         //Assumed Constant Voltage
#define curr_calib                  5           //Current Calibration Factor
#define spie                        30          //Serial Print and Cloud Publish Interval for Energy Values 
#define spic                        300         //Serial Print and Time Force Update Interval for Config Values
#define millis_delay_interval       1000        //Default Interval Variable used for millis delay in milliseconds
#define offset                      19800       //+05:30 GMT
#define auto_update_int             85600       //AutoSync from NTP Server Everyday
#define pub_str_size                20          //Length of the Publish String

//Soft  Access Point
const char *softSSID = "smartswitch";           //Credentials for Master Access Point
const char *softPASS = "smartswitch";           //For the function to work, the password should be more than 8 chars and should begin with a char too
IPAddress softAP_ip(192,168,5,1);
IPAddress softAP_gateway(192,168,5,1);          //IP config for the soft Access Point
IPAddress softAP_subnet(255,255,255,0);

//Flags
int sflag = 0;                              //sflag: flag used in setup for WiFi Connection Time Out and later in HTTPS Server Functions to check for connection success
int pflag = 0;                              //pflag: flag used to make sure power measurement is run one time only and based on current time (timestamp)
bool ret;                                   //For Various Connection Result Return Values 
bool time_synced = false;                   //Keeps Track of Whether Time is Set or not
bool enable_ap = false;                     //To Enable AP for WiFi Configuration

//Measurements and Control Values
long int timestamp;                         //Timestamp Measurement
double curr_raw;                            //Current Measurement on specified Pin
double power;                               //Power Measured using Current and Voltage
char po_ts[pub_str_size];                   //Contains Combined value of Power and TimeStamp
char* packet;                               //Packet Received from Subscription
bool sch_status = false;                    //Preset the appliance to stay off
unsigned long current_millis;               //For Millis Implementation
unsigned long last_millis = 0;
//

//Class Object Declarations
WiFiClient client;

//MQTT Client Class Object
Adafruit_MQTT_Client mqtt_client_object(&client, aio_server, aio_serverport, aio_username, aio_key);
Adafruit_MQTT_Publish po_ts_object(&mqtt_client_object, po_ts_feed);
Adafruit_MQTT_Subscribe subscribe_object = Adafruit_MQTT_Subscribe(&mqtt_client_object, subs_feed);

//Web Server Class Object, listens to requests on port 80 (HTTP)
ESP8266WebServer server(80);

//Energy Meter Class Object for Current Measurement
EnergyMonitor curr;

//UDP Object and NTP Object for NTP Syncing
WiFiUDP ntp_udp_client;
NTPClient time_object(ntp_udp_client, "time.nist.gov", offset, auto_update_int);
//

//Function Prototypes

//Web Server Handling Functions
void handle_root();
void handle_notfound();
void handle_disconnect_wifi();
void handle_connect_wifi();
void handle_wifi_login();

//String Append and Format Function
int append_value_to_po_ts(double, bool, char*, int);

//Network and Soft AP Connection Functions
void sta_setup(char*, char*);
void mqtt_connect();
bool begin_ap();
bool close_ap();
//

//Setup
void setup() 
{
  //Pin Setup
  pinMode(ind_led, OUTPUT);           //LED Indication for WiFi Connection and MQTT Publishing
  pinMode(curr_inp, INPUT);           //Current Measurement Pin
  pinMode(ap_control_pin, INPUT);     //Controlling AP Externally
  digitalWrite(ind_led, HIGH);        //Initially WiFi Not Connected and MQTT not publishing
  
  // Begin Serial Communication with Arduino IDE
  Serial.begin(115200);
  Serial.print("\nSuccesfully began Serial Communication With NodeMCU\n");
  
  //Begin WiFi Setup
  WiFi.mode(WIFI_AP_STA);   //Declares the WiFi Mode as Station plus Access Point
  //WiFi Access Point Mode Moved to an External function declared as begin_ap();
//  enable_ap ? begin_ap() : true ;
  
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

  //WiFi Connection Begins
  WiFi.begin();   //Tries Connection to Last Known Network
  Serial.print("\nConnecting to Last Known Network");
  while(WiFi.status() != WL_CONNECTED && ::sflag < conn_wp)
  //Waits for Connection to be made till TIMEOUT = conn_wp, uses sflag
  {
    Serial.print(".");
    ::sflag++;
    delay(1000);
  }
  if(::sflag == conn_wp)
  //If sflag reaches TIMEOUT, Connection not made
  //Stop connection, Open Soft AP and set enable_ap flag
  {
    Serial.print("\nWarning : Couldnt Connect to Network");
    WiFi.disconnect();
    begin_ap();
    enable_ap = true;
    delay(1000);
  }
  else
  //If sflag not TIMED OUT, Connection Made
  //Print SSID and IP, Close Soft AP 
  //enable_ap flag not required to be set to false, as loop doesn't act as daemon for enable_ap (starts only when button is pressed)
  //Attempt Time Sync from NTP and Set time_synced flag appropriately
  {
    Serial.print("\nConnection Successful to: ");
    Serial.print(WiFi.SSID());
    Serial.print("\nIP Address: ");
    Serial.print(WiFi.localIP());
    close_ap();
    ret = time_object.update();
    ret ? Serial.print("\nTime Sync Successful") :  Serial.print("\nWarning : Time Sync Failed");
    ret ? ::time_synced = true : ::time_synced = false;
  }
  ::sflag = 0;    //Reset Sflag for Other Uses
  
  //Attempt MQTT Connection
  mqtt_connect();
  mqtt_client_object.subscribe(&subscribe_object);
}
//

//Main Loop
void loop() {

  //Handle Incoming HTTP requests from Clients
  server.handleClient();

  //WiFi connection LED Indication (Connected), and Force Time Update every "spic" seconds or if time_synced flag not set 
  if(WiFi.status() == WL_CONNECTED)
  {
    digitalWrite(D4, LOW);
    if(time_object.getEpochTime() % spic == 0)
    {
      ret = time_object.update();
      !::time_synced && ret ? ::time_synced = true : ret ;   //Set Flag to true if previously false
    }
  }
  else
    digitalWrite(D4, HIGH); //LED Indication not Connected
    
  //Access Point Control, Allows User to Press an External button to switch the Soft Access Point on or off.
  if(digitalRead(ap_control_pin) == HIGH) 
  //If Control Button Pressed? Acts as Daemon on Button Press only, not on enable_ap flag
  {
    ::current_millis = millis();
    if(::current_millis > ::last_millis + 3*millis_delay_interval)  
    //If Button pressed atleast after 3*millis_delay_interval enable change
    //Prevents Burst Presses or Fluctuation Errors
    {
      enable_ap = !enable_ap;
      ::last_millis = ::current_millis;
      if(enable_ap)
        begin_ap();
      else
        close_ap();
    }
  }
  
  //Serial Print and Publish Energy and Time Data Every spie seconds 
  if(time_object.getSeconds() % spie == 0 && pflag == 0)
  { 
    //Index for Appending Value to end of Publish String, Initiate string to Null
    int append_index = 0;
    po_ts[append_index] = '\0';

    //Measurement using Emonlib
    curr_raw = curr.calcIrms(1480);
    power = voltage*curr_raw;
    //Get Current timestamp
    timestamp = time_object.getEpochTime();

    //Append TimeStamp to Publish String
    append_index = append_value_to_po_ts(timestamp, true, po_ts, append_index);
    //Append Power to Publish String
    append_index = append_value_to_po_ts(power, false, po_ts, append_index);
    Serial.print("\n");
    Serial.print(po_ts);

    //Serial Print Energy Data
    Serial.print("\n\nEnergy Data : ");
    Serial.print("\nTimeStamp : ");
    Serial.print(timestamp);
    Serial.print("\nCurrent(A) : ");
    Serial.print(curr_raw);
    Serial.print("\nPower (W) : ");
    Serial.print(power);
    Serial.println();

    //MQTT Connection and Time Sync Check
    if(mqtt_client_object.connected() && time_synced)
    //If Connected and Time Synced
    //Publish the Publish String to MQTT Broker and Indicate Appropriately
    {
      bool res;
      res = po_ts_object.publish(po_ts);
      if(res)
      {
        digitalWrite(mqtt_pub_led, HIGH);
        delay(200);
        digitalWrite(mqtt_pub_led, LOW);
      }
    }
    else
    //If MQTT not Connected or Time not Synced
    {
      if(time_synced)
      //If not Connected, Attempt Connection
      {
        Serial.print("\nMQTT Connection Lost.. Reconnecting");
        mqtt_connect();
      }
    }
    
    //Print Config Data Every spic seconds,
    //and Check for new subscription
    //Do this after Power has been collected
    //Subscription used to Control Appliance
    if(time_object.getEpochTime() % spic == 0)
    {
      mqtt_client_object.ping();  //Ping to Keep MQTT Connection Alive
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
      if(enable_ap)
      {
        Serial.print("\nSoft AP: Active");
        Serial.print("\nStations Connected to AP: ");
        Serial.print(WiFi.softAPgetStationNum());
      }
      else
        Serial.print("\nSoft AP: Inactive");
      Serial.print("\nTime Synced : ");
      ::time_synced ? Serial.print("Yes") : Serial.print("No");
      Serial.print("\nMQTT Connection : ");
      mqtt_client_object.connected() ? Serial.print("Alive") : Serial.print("Dead");  

      //Read Subscription
      Adafruit_MQTT_Subscribe *schedule;
      schedule = mqtt_client_object.readSubscription(2000);
      if (schedule == &subscribe_object)
      {
        packet = (char*)subscribe_object.lastread;
        Serial.print(packet);
      }
      //End of Subscription Reading and Parsing
    }
    //pfalg used to display config data and read subscription only ONCE every spic seconds
    pflag = 1;
  }
  else if(time_object.getSeconds() % spie != 0)
    pflag = 0;

  //digitalWrite(pin#, sch_status);
}
//

// Function Definitions
void handle_root()
{
  //If Conected to WiFi, take to WiFi connected Page else to Not Connected
  if( WiFi.status() == WL_CONNECTED )
    server.send(200, "text/html", html_root_conn); 
  else
    server.send(200, "text/html", html_root); 
}

void handle_disconnect_wifi()
{
  //Disconnect WiFi and send user to Home
  WiFi.disconnect();
  digitalWrite(D4,HIGH);
  Serial.print("\nWiFi Disconnected");
  server.sendHeader("Location", "/");
  server.send(303);
}

void handle_connect_wifi()
{
  //If connection unsuccesful, send to try again page else continue to connect wifi
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
    {                                 //BUG 1, Connection takes time and browser times out
      sflag = 1;
      server.sendHeader("Location", "/connect_wifi");
      server.send(303);
    }
    
    delete[] p;                       // Delete dynamically allocated memory
    delete[] s;
  }
  else
  //If user doesnt input credentials, redirect to same page
  {
    sflag = 1;
    server.sendHeader("Location", "/connect_wifi");
    server.send(303);
  }
}

void handle_notfound()
{
  //Send to Home when location not Found
  server.sendHeader("Location", "/");
  server.send(303);
}

int append_value_to_po_ts(double val, bool timestamp, char* po, int end_index)
{
  // Appends the passed value in the right format to the passed Char Array (Passed as Pointer po)
  if(timestamp)
  {
    long int t = val;
    int ap, up, down;
    
    up = t / pow(10,5);
    down = t - up * pow(10, 5);
    
    for(end_index = 0; end_index <= 9; end_index++)
    {
      ap = up / pow(10, 4);
      po[end_index] = ap + 48;
      
      up *= 10;
      up = up % 100000;
      up += down / pow(10, 4);

      down *= 10;
      down = down % 100000;
    }
    po[end_index] = '\0';
    return end_index;
  }
  else
  {
    // Append Power Value at the End
    po[end_index] = ',';
    po[end_index + 10] = '\0';
    
    Serial.print(po);
    int p;
    for(int i = 1; i <= 9; i++)
    {
      p = val / pow(10, 3);
      p = p % 10;
      if(i == 5)
      {
        po[end_index + i] = '.';
        i++;
      }
      po[end_index + i] = p + 48;
      val *= 10;
    }
    return end_index + 10;
  }
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

bool begin_ap()
{
  //Begin Soft AP when called and return true if successful
  WiFi.softAPConfig(softAP_ip, softAP_gateway, softAP_subnet);
  ret = WiFi.softAP(softSSID, softPASS);
  if (ret == true)
  {
    Serial.print("\nSoft Access Point Started with IP: ");
    Serial.print(WiFi.softAPIP());
  }
  return ret;
}

bool close_ap()
{
  //Close Soft AP when called and return True if successfully closed
  ret = WiFi.softAPdisconnect(true);
  if(ret)
    Serial.print("\nAccess Point: Closed");
  else
    Serial.print("\nCannot Close Access Point try again");
  return ret;
}

void mqtt_connect()
{
  int r = 1;
  if(mqtt_client_object.connected())  //return nothing if already Connected
    return;
  if(WiFi.status() != WL_CONNECTED)   //Set flag if WiFi not connected
    r = -2;
  Serial.print("\nAdafruit MQTT");
  while((ret = mqtt_client_object.connect()) != 0 && r != -2)
  //if WiFi connected and MQTT Connection failed, retry in 1 second
  {
    Serial.println();
    Serial.print("\nRetrying MQTT connection in 1 second...");
    delay(500);
    mqtt_client_object.disconnect();
    delay(500);
    r--;    //Retry only once then exit from retry loop
    if( r < 0 )
      break;
    else
      continue;
  }
  //Serial Description of Connection Result
  r >= 0 ? Serial.print("\nMQTT Connected"): Serial.print("\nWarning : Cannot Connect to MQTT Service, data would not be published");
}
//
