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
