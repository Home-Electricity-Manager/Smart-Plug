#ifndef servertext_h
#define servertext_h


const char *html_root = "<!DOCKTYPE html>"
"<head>"
  "<h1 align=\"center\"> Master Smart Plug 1 </h1>" 
"</head>"
"<body>"
  "<form align=\"center\" action = \"/connect_wifi\" method=\"POST\">"
  "<input align=\"center\" type =\"submit\" value=\"Connect to WiFi\">"
  "</form>"
"</body>";


const char *html_root_conn = "<!DOCTYPE html>"
"<head>" 
  "<h1 align=\"center\"> Master Smart Plug 1 </h1>" 
"</head>"
"<body>"
  "<h3 align=\"center\"> WiFi Connected </h3>" 
  "<form align=\"center\" action = \"/connect_wifi\" method=\"POST\">"
  "<input align=\"center\" type =\"submit\" value=\"Connect to Other WiFi\">"
  "</form>"
  "</br>"
  
  "<form align=\"center\" action = \"/disconnect_wifi\" method=\"POST\">"
  "<input align=\"center\" type =\"submit\" value=\"Disconnect WiFi\">"
  "</form>"
"</body>";

  
const char *html_connect_wifi = "<!DOCTYPE html>"
"<head>" 
  "<h1 align=\"center\"> Master Smart Plug 1 </h1>" 
"</head>" 
"<body>" 
  "<h3 align=\"center\"> Enter WiFi Login Credentials </h3>"
  "<form align=\"center\" action = \"/wifi_login\" method=\"POST\">"
  "<input align=\"center\" type =\"text\" name=\"SSID\" placeholder=\"SSID\">"
    "</br>" 
  "<input align=\"center\" type =\"password\" name=\"pass\" placeholder=\"Password\">"
    "</br>"
  "<input align=\"center\" type =\"submit\" value=\"Connect\">"
    "</br>"
  "</form>"
  "</br>"
  
  "<form align=\"center\" action = \"/\" method=\"POST\">"
  "<input align=\"center\" type =\"submit\" value=\"Home\">"
  "</form>"
"</head>";


const char *html_connect_wifi_tryagain = "<!DOCTYPE html>"
"<head>" 
  "<h1 align=\"center\">Master Smart Plug 1 </h1>" 
"</head>" 
"<body>" 
  "<h3 align=\"center\"> Couldn't Connect to WiFi, try again </h3>"
  "<form align=\"center\" action = \" wifi_login\" method=\"POST\">"
  "<input align=\"center\" type =\"text\" name=\"SSID\" placeholder=\"SSID\">"
    "</br>" 
  "<input align=\"center\" type =\"password\" name=\"pass\" placeholder=\"Password\">"
    "</br>"
  "<input align=\"center\" type =\"submit\" value=\"Connect\">"
    "</br>"
  "</form>"
  "</br>"
  "<form align=\"center\" action = \"/\" method=\"POST\">"
  "<input align=\"center\" type =\"submit\" value=\"Home\">"
  "</form>"
"</head>";


#endif
