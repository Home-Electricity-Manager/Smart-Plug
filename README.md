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
**Currently having Issues with syncing time with a server. The issue is resolved when the flexible WiFi Connection feature isnt used, i.e. when I preset the WiFi SSID and password. Cannot seem to make it work otherwise**




