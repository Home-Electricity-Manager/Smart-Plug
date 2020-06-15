/*
 * Code for the Energy Boosters' Teams Smart Plug Multi Socket on the NodeMCU 0.9 Module
 * Developed by Archit Jain
 * 
 * Maximum Number of Possible Sockets for the ESP8266 (NodeMCU) is 8
 * owing to a limited number of gpio pins on the dev board
 * 
 * The Code is Largely Independent of the Board Specifics other than
 * the Wifi Library, Webserver and Pin Bindings
*/

//Include Required Libraries
#include "servertext.h"             //HTML for the Web Server
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
#define aio_serverport  0000
#define aio_username    ""
#define aio_key         ""
#define po_ts_feed      ""
#define subs_feed       ""

//NTP Server
#define ntp_server          "time.nist.gov"     //NTP Server
#define offset              19800               //+05:30 GMT
#define auto_update_int     85600               //AutoSync from NTP Server Everyday

//Pin Bindings
#define ind_led             D4                  //LED for Physical Indication of Device (LED Active LOW)
#define mqtt_pub_led        D4                  //LED for Publish Success Indication
#define ap_control_pin      D8                  //For Externaly Controlling AP Enable Disable
#define dclk                D5                  //Output FF's Clock
#define rclk                D0                  //Clock Input for Register 
#define sel0                D1                  //Select Pins
#define sel1                D2                  //For MUX and Decoder
#define sel2                D3                  //Select Pins
#define serial_out          D7                  //Serial Output to Registers for Appliance Control
#define curr_inp            A0                  //Analog Pin for Current Measurement
static const int sel_pins[3] = {sel0, sel1, sel2};

//Intervals and Value Constants
#define max_sockets                 8
#define serial_baud_rate            115200                            //Serial Communication Baud Rate
#define webserver_port              80                                //Port Number that the Server will Listen on
#define sockets                     8                                 //Number of Appliances connected to the Node (Maximum 8)
#if sockets > 1
  #define socket_pins               int(ceil(log(sockets)/log(2)))    //Number of Pins required for socket selection
#else
  #define socket_pins               1
#endif
#define pub_str_size                (sockets + 1)*10                  //Length of the Publish String
#define conn_wp                     20                                //Wait Period for WiFi Connection in seconds
#define voltage                     230                               //Assumed Constant Voltage
#define current_samples             1480                              //Number of Samples, Arguement to calcIrms
#define spie                        10                                //Serial Print and Cloud Publish Interval for Energy Values 
#define spic                        20                                //Serial Print and Time Force Update Interval for Config Values
#define default_delay_interval      1000                              //Default Interval Variable used for millis delay in milliseconds
const int curr_calib[max_sockets] = {5, 5, 5, 5, 5, 5, 5, 5};         //Current Calibration Factors

//Soft  Access Point
const char *softSSID = "smartswitch";       //Credentials for Master Access Point
const char *softPASS = "smartswitch";       //For the function to work, the password should be more than 8 chars and should begin with a char too
IPAddress softAP_ip(192,168,5,1);
IPAddress softAP_gateway(192,168,5,1);      //IP config for the soft Access Point
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
bool sch_status[sockets] = {1,0,0,1,0,0,1,1};             //Preset the appliance to stay off
unsigned long current_millis;               //For Millis delay of Control AP functionality
unsigned long last_millis = 0;
//

//Class Object Declarations
WiFiClient client;

//MQTT Client Class Object
Adafruit_MQTT_Client mqtt_client_object(&client, aio_server, aio_serverport, aio_username, aio_key);
Adafruit_MQTT_Publish po_ts_object(&mqtt_client_object, po_ts_feed);
Adafruit_MQTT_Subscribe subscribe_object = Adafruit_MQTT_Subscribe(&mqtt_client_object, subs_feed);

//Web Server Class Object, listens to requests on port specified port (HTTP)
ESP8266WebServer server(webserver_port);

//Energy Meter Class Objects for Current Measurement from Multiple Sockets
EnergyMonitor curr[sockets];

//UDP Object and NTP Object for NTP Syncing
WiFiUDP ntp_udp_client;
NTPClient time_object(ntp_udp_client, ntp_server, offset, auto_update_int);
//

//Function Prototypes

//Web Server Handling Functions
void handle_root();
void handle_notfound();
void handle_disconnect_wifi();
void handle_connect_wifi();
void handle_wifi_login();

//Data Acquisition and Control Functions
void spie_rep();
int append_value_to_po_ts(double, bool, char*, int);
void mux_select_write(int);
void dec_select_write(int);

//Network and Soft AP Connection Functions
void sta_setup(char*, char*);
void mqtt_connect();
void ap_button_ontrigger();
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
  
  pinMode(sel0, OUTPUT);
  pinMode(sel1, OUTPUT);
  pinMode(sel2, OUTPUT);
  pinMode(dclk, OUTPUT);
  pinMode(rclk, OUTPUT);
  pinMode(serial_out, OUTPUT);
   
  digitalWrite(ind_led, HIGH);        //Initially WiFi Not Connected and MQTT not publishing  
  
  // Begin Serial Communication with Arduino IDE
  Serial.begin(serial_baud_rate);
  Serial.printf("\nSuccesfully began Serial Communication With SoC at Baud %d", serial_baud_rate);
  Serial.printf("\n#Appliance Sockets: %d, Pins: %d", sockets, socket_pins);
  
  //Begin WiFi Setup
  WiFi.mode(WIFI_AP_STA);   //Declares the WiFi Mode as Station plus Access Point
  //WiFi Access Point Mode Moved to an External function declared as begin_ap();
//  enable_ap ? begin_ap() : true ;
  
  //HTML Server and Handles
  server.begin();
  Serial.printf("\nStarting HTTPS WebServer on port %d", webserver_port);
  server.on("/", HTTP_GET, handle_root);
  server.on("/disconnect_wifi", HTTP_POST, handle_disconnect_wifi);
  server.on("/connect_wifi", HTTP_POST, handle_connect_wifi);
  server.on("/wifi_login", handle_wifi_login);
  server.onNotFound(handle_notfound);

  //Current Measurement Setup
  for(int socket = 0; socket < sockets; socket++)
    curr[socket].current(curr_inp, curr_calib[socket]);

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
    delay(default_delay_interval);
  }
  if(::sflag == conn_wp)
  /*
   * If sflag reaches TIMEOUT, Connection not made
   * Stop connection, Open Soft AP and set enable_ap flag 
  */
  {
    Serial.print("\nWarning : Couldnt Connect to Network");
    WiFi.disconnect();
    begin_ap();
    enable_ap = true;
    delay(default_delay_interval);
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

  //Set all selection pins to LOW
  for(int i = 0; i < 3; i++)
    digitalWrite(sel_pins[i], LOW);

  //Set D-FF's clock LOW
  digitalWrite(dclk, LOW);

  //WiFi connection LED Indication (Connected), and Force Time Update every "spic" seconds or if time_synced flag not set 
  if(WiFi.status() == WL_CONNECTED)
  {
    digitalWrite(ind_led, LOW);
    if(time_object.getEpochTime() % spic == 0)
    {
      ret = time_object.update();
      !::time_synced && ret ? ::time_synced = true : ret ;   //Set Flag to true if previously false
    }
  }
  else
    digitalWrite(ind_led, HIGH); //LED Indication not Connected
    
  //Access Point Control, Allows User to Press an External button to switch the Soft Access Point on or off.
  if(digitalRead(ap_control_pin) == HIGH)
    ap_button_ontrigger();
  
  //Serial Print and Publish Energy and Time Data Every spie seconds 
  if(time_object.getSeconds() % spie == 0 && pflag == 0)
  { 
    /*
     *conf Flag to Print Config Data
     *Flag Set here as measurements introduce delay (~2000 ms)
     *and hence comparison with spic fails later
    */
    bool conf = false;
    if(time_object.getEpochTime() % spic == 0)
      conf = true;

    //Index for Appending Value to end of Publish String, Initiate string to Null
    int append_index = 0;
    po_ts[append_index] = '\0';

    //Get Current timestamp
    timestamp = time_object.getEpochTime();
    //Append TimeStamp to Publish String
    append_index = append_value_to_po_ts(timestamp, true, po_ts, append_index);

    //Collect and Serial Print Energy Data
    Serial.print("\n\nEnergy Data : ");
    Serial.printf("\nTimeStamp : %d", timestamp);
    
    long int m = millis();
    for(int i = 0; i < sockets; i++)
    {
      mux_select_write(i);
      Serial.println();
      Serial.println(((analogRead(curr_inp)+1)*3.3)/1024);
      curr_raw = curr[i].calcIrms(current_samples);
      power = voltage * curr_raw;
      append_index = append_value_to_po_ts(power, false, po_ts, append_index);
      Serial.printf("\nDevice %d (W): %f", i, power);
    }
    Serial.printf("\nMeasurement and Appending Time (ms): %d", millis() - m); 
    Serial.print("\nPublish String:\n");
    Serial.print(po_ts);

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
        delay(default_delay_interval/5);
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

    //Get NEw Subscription from Feed and 
    //Control Appliances, then
    //Print Config Data,
    //Do this after Power has been collected
    //Every spic seconds
    if(conf)
    {
      mqtt_client_object.ping();  //Ping to Keep MQTT Connection Alive
      
      //Read Subscription
      Adafruit_MQTT_Subscribe *schedule;
      schedule = mqtt_client_object.readSubscription(2000);
      if (schedule == &subscribe_object)
      {
        packet = (char*)subscribe_object.lastread;
        Serial.print("\nPacket Received from Control Feed:\n");
        Serial.print(packet);
      }
      
      //Appliance Output Control
      for(int device = 0; device < max_sockets; device++)
      {
        //Write the Status of the device to Output Pin
        digitalWrite(serial_out, sch_status[device]);
        delay(0.1);

        digitalWrite(rclk, LOW);
        digitalWrite(rclk, HIGH);
        delay(1);
        digitalWrite(rclk, LOW);
        delay(1);
      }
      //Pulse DFF Clock
      digitalWrite(dclk, HIGH);
      delay(1);
      digitalWrite(dclk, LOW);
      delay(1);
      digitalWrite(serial_out, LOW);
      
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
        Serial.printf("\nStations Connected to AP: %d", WiFi.softAPgetStationNum());
      }
      else
        Serial.print("\nSoft AP: Inactive");
      Serial.print("\nTime Synced : ");
      ::time_synced ? Serial.print("Yes") : Serial.print("No");
      Serial.print("\nMQTT Connection : ");
      mqtt_client_object.connected() ? Serial.print("Alive") : Serial.print("Dead");  

      
    }
    //plag used to display config data and read subscription only ONCE every spic seconds
    pflag = 1;
  }
  else if(time_object.getSeconds() % spie != 0)
    pflag = 0;
    
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
  digitalWrite(ind_led,HIGH);
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
    //Serial.print(p);
    
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
// Appends the passed value in the right format to the passed Char Array (Passed as Pointer po)
{
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

void mux_select_write(int val)
/*
 * Writes the Select Pins to the Input Multiplexer
 * Integer val is the index of the input to be selected
*/
{
  
  for(int m = 0; m < socket_pins; m++)
  {
    int comp = pow(2, m);
    if((val & comp) != 0)
      digitalWrite(sel_pins[m], HIGH);
    else
      digitalWrite(sel_pins[m], LOW);
  }
}

void dec_select_write(int val)
/*
 * Writes the Select Pins to the Output Decoder/Demux
 * Integer val is the index of the output to be selected
*/
{
  
}

void sta_setup(char *s, char* p)    
/* 
  *  For Setting up STA mode WiFi connections via user input.
  *  Uses the HTML Server through the Soft Access Point to get WiFi network creds from 
  *  user and setup the STA Mode Connection
*/
{
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
    digitalWrite(ind_led, LOW);
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

void ap_button_ontrigger()
/*
 * When Control Button Pressed 
 * Acts as Daemon on Button Press only, not on enable_ap flag
 */
{
  ::current_millis = millis();
  if(::current_millis > ::last_millis + 3*default_delay_interval)  
  /*
   * If Button pressed atleast after 3*default_delay_interval enable change
   * Prevents Burst Presses or Fluctuation Errors
   */
  {
    enable_ap = !enable_ap;
    ::last_millis = ::current_millis;
    if(enable_ap)
      begin_ap();
    else
      close_ap();
  }
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
  Serial.print("\nAdafruit MQTT Connecting");
  while((ret = mqtt_client_object.connect()) != 0 && r != -2)
  //if WiFi connected and MQTT Connection failed, retry in 1 second
  {
    Serial.println();
    Serial.print("\nRetrying MQTT connection in 1 second...");
    delay(default_delay_interval/2);
    mqtt_client_object.disconnect();
    delay(default_delay_interval/2);
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
