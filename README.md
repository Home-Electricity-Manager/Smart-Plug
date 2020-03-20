The original idea was to make a master plug for each room and several slave plugs connected to the master through a 
WiFi access point created using the master plug. But due to Network Stack issues and issues pertaining to maintain time in 
all the smart plugs, I'm dropping this idea dor the time being.

**Description of libraries Used**

*servertext.h*

Contains the required HTML for the WiFi Connection feature
The Master acts as a HTML Server and is used to connect to the required Network using 
a web interface.

*ESP8266WiFi.h*

Library used by ESP8266 module to give WiFi functionality to the Board.

*ESP8266WebServer.h*

Library required to bring the web server functionality for the Module.

*EmonLib.h*

Used to calculate the current drawn

*NTPClient.h*

Uses UDP protocol to connect to a NTP Server. IS used to sync time with the master plug.
**Currently having Issues with syncing time with a server. The issue is resolved when the flexible WiFi Connection feature isnt used, i.e. when I preset the WiFi SSID and password. Cannot seem to make it work otherwise. Suspected problem is with the UDP Connection**

*WiFiUDP.h*

Used for setting up the udp connection required for the NTPClient Server Commuincation.

**PROGRAM DESCRIPTION**

**Preprocessing and Declarations**

The NodeMCU has two WiFi Modes, the Station Mode and the Access Point Mode
Station mode is used to connect to our home network and the Access Point mode makes the 
master an Access Point through which the WebServer will be accessed. The following code 
describes the basic config of the access point and other constants that are used in the later functions
~~~
//Constants Declarations
const char *softSSID = "Master";    //Credentials for Master Access Point
const char *softPASS = "password";  //For the function to work, the password should be more than 8 chars and should begin with a char too

IPAddress softAP_ip(192,168,0,1);         //IP config for the soft Access Point
IPAddress softAP_gateway(192,168,0,1);
IPAddress softAP_subnet(255,255,255,0);

int sflag = 0;
int cflag = 0;

const long int offset = 19800;
~~~
Objects that will be used are declared as follows
~~~
//Class Object Declarations
//ESP8266WifiMulti WiFimulti_Object;
ESP8266WebServer server(80);

EnergyMonitor curr;

WiFiUDP ntp_udp_client;
NTPClient time_object(ntp_udp_client, "asia.pool.ntp.org", offset);
//
~~~
ESP8266WebServer class is used for the methods of the webserver. 80 declares the network 
port used for the webserver, which is default for HTML

EnergyMonitor class contains the methods used to calculate the RMS value of Current

WiFiUDP class declares an object used for the UDP connection

NTPClient class uses a constructor taking in the UDP object, NTPServer and the Time Offset 
that is declred in the constants above

**Function Descriptions**
The Following Functions are used in the Program:
~~~
//Function Prototypes
void sta_setup(char*, char*);

void handle_root();
void handle_notfound();
void handle_disconnect_wifi();
void handle_connect_wifi();
void handle_wifi_login();
//
~~~
Other than this, the void setup and loop do the same thing as in an Arduino

*void sta_setup()*

This function is used to dynamically connect to a home wifi network using the WebServer.

*void handle_root ...... handle_wifi_login()*

Used for the behavior of the webserver and redirecting webpages
**There is an Outstanding Issue with this. When Incorrect Login info is passed throught the server, the webpage isn't returned soon enough and the browser considers it a time out and hence an error is produced. Haven't figured out a way to delay the redirection as of now**


The Setup function is described below:
~~~
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
  WiFi.mode(WIFI_AP_STA);   //Declares the WiFi Mode as Station plus Access Point
  
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
~~~
pinModes are used to declare the I/O pins on the NodeMCU as Input or output
The digital pin D4 is connected to a status LED that indicates the Status of the station
mode connection i.e connection to the Home Network.
The Analog pin A0 is used to read the output from the current sensor and it's sensed value is
used to calculate the RMS current.

Serial.begin is used begin serial communication with our Arduino IDE.

softApConfig configures the soft access point with the given IP config details declared before
if the method returns true then it prints so on the serial monitor

After the soft Access Point is started the WiFi.begin() statement attempts STA 
mode connection to the last known WiFi network

Next, the HTML server is started and handles are defined using server.on method

curr.current confiures the sapling cycles and calibration factor based off the hardware.

