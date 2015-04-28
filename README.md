# ESP WiFiSwitch
This sketch is running a web server for configuring WiFI if can't connect or for controlling of one GPIO to switch a light/LED
While a WiFi config is not set or can't connect:
  http://server_ip will give a config page with 
While a WiFi config is set:
  http://server_ip/gpio -> Will display the GIPIO state and a switch form for it
  http://server_ip/gpio?state=0 -> Will change the GPIO directly and display the above aswell
  http://server_ip/cleareeprom -> Will reset the WiFi setting and rest to configure mode as AP
server_ip is the IP address of the ESP8266 module, will be 
printed to Serial when the module is connected.
For several snippets used the credit goes to:
- https://github.com/esp8266
- https://github.com/chriscook8/esp-arduino-apboot
- And the whole Arduino and ESP8266 comunity
