#ifndef servertext_h
#define servertext_h


const char *HTMLroot = "<!DOCKTYPE html>"
"<head>"
  "<h1 align=\"center\"> Master Smart Plug 1 </h1>" 
"</head>"
"<body>"
  "<form action = \"/Connect_WiFi\" method=\"POST\">"
  "<input type =\"submit\" value=\"Connect to WiFi\">" 
"</body>";


const char *HTMLroot_conn = "<!DOCTYPE html>"
"<head>" 
  "<h1 align=\"centre\"> Master Smart Plug 1 </h1>" 
"</head>"
  "<h3> WiFi Connected </h3>" 
"<body>"
  "<form action = \"/Connect_WiFi\" method=\"POST\">"
  "<input type =\"submit\" value=\"Connect to Other WiFi\">"
"</body>";

  
const char *HTMLconnect_WiFi = "<!DOCTYPE html>"
"<head>" 
  "<h1><centre> Master Smart Plug 1 <centre></h1>" 
"</head>" 
"<body>" 
  "<form action = \" WiFi_login\" method=\"POST\">"
  "<input type =\"text\" name=\"SSID\" placeholder=\"SSID\">"
    "</br>" 
  "<input type =\"password\" name=\"pass\" placeholder=\"Password\">"
    "</br>"
  "<input type =\"submit\" value=\"Connect\">"
"</head>";


#endif
