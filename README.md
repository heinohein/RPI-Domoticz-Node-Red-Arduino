## **Project Raspberry PI with Domoticz, MQTT/Mosquitto, Node-Red/Node-Red dashboard, Arduino ESP32 with temperature sensors and Relays**

On my Raspberry PI 3B+ is installed Domoticz,  Mosquitto as the MQTT broker, Node-Red and the Node-Red dashboard. 
To the Arduino ESP32 are attached 0/1x DHT11 temperature+humidity sensor,  0/1/2x DS18B20 temperature sensor(s) and 0/1/2 relay(s).
All communication between Domoticz, the Arduino and some processes programmed in Node-Red are, where needed, processed in Node-Red and then distributed by Mosquitto.
The communicatio between The Raspberry PI and the Arduino Esp is done over Wifi.

The communication between Domoticz, Node-Red and The Arduino ESP32 is maintained by messages with a layout defined by the Domoticz software. These 3 applications maintain between
eachother application to application information exchange, like the OSI model Layer 7.
A more detailed desciption is available in the document "Project Raspberry PI-Domoticz-Node-Red-Arduino.docx" and on detail niveau in the sketch for the ESP32
and the Node-flow. 
An impression of the implementation in Node-Red can be found in the Node-Red* pictures. The Node flow is exported in Node-Red to a file available hereafter.
Hints of the installation in Domoticz can be found in the Domoticz* files and pictures.
The sketch for the Arduino Esp32 can be found as ESP32_MQTT_DHT11_DS18B20_V2.3.ino.
 


## **The following files are available:**


- Project Raspberry PI-Domoticz-Node-Red-Arduino.docx                |project documentation

- ESP32-Node-Red-overview.jpg                                        |Screenprint of the Node-Red flows
- Node-Red-Dashboard.png                                             |Screenprint of the dashboad in Node-Red
- 20200615 Node_Red_V1.2_flows.json                                  |Exported Node flow out from Node-Red
- Node-Red-Log.txt                                                   |Example logfile of Node-Red
|
- Domoticz-Setup.png                                                 |Screenprint of tab Setup/Hardware in Domoticz
- Domoticz-Devices.png                                               |Screenprint of tab Setup/Devices in Domoticz
- Domoticz-Switches.jpg                                              |Screenprint of tab Switches in Domoticz
- Dump domoticz.db for Node-Red.xlsx                                 |Partial dump of domoticz.db made with SQLite. 
|PLEASE READ THE WARNING IN THE Project*.docx FILE. 
- Domoticz-Log.png                                                   |Screenprint of tab Setup/Log showing incoming messages in Domoticz

- ESP32_MQTT_DHT11_DS18B20_V2.3.ino                                  |Sketch for the Arduino Esp32 with very many comments and explanations


## **Prerequisites:**

- This is a Raspberry PI, Arduino Esp32 and Wifi based project.
- You need a fully working Raspberry PI, I used a model 3B+. It runs Raspbian Stretch november 2018. 
- On the RPI you must install Domoticz, Node-Red and Node-Red ui (dashboard). Many installation manuals can be found on the internet. 
- It is strongly advised that you are somewhat familiar with Linux and now how to operate the RPI, Domoticz, Node-Red and Linux commands and tools.
- The RPI must be connected to the local TCP-IP network. Currently I have connected my RPI with a cable but initially I used a Wifi connected solution. Both working fine. 
- You need an Arduino Esp32 to run the sketch. I run this project on a ESP-WROOM 32 devellopment board type ESP-32S. Any Esp32 board should do the job.
- You must know how to connect a sensor to the Esp32, by breadboard + jumper cables or by soldering. Please see details of the connections in the Arduino sketch.
- You must have installed a recent version of the Arduino IDE. In this IDE you must have installed with Preferences the latest ESP32 board managers package.
  Information on how to can be found on the internet. I use a 7 year old PC running Windows 7 for these jobs.
- Some knowledge of TCPIP networks is needed. You must be able to find the IP number of your RPI and update the Esp32 sketch. Normally the DHCP functions 
  of your ADSL modem are helpfull but free tools to be found on the internet like Angry IP Scanner will find the information as well.
- If you connect your RPI to a network by cable it is advised to use a fixed IP address. In the DHCP part of your ADSL modem you must be able to set this. You need this IP adress to visit the
  Domoticz webpages, the Node-Red web pages and The Node Red dashboard web pages. It is therefore easy to have a never changing IP-number.
- You must be able to update and compile the sketch and download it into your Arduino ESP32.
- I use a standaard 230V to 5V powerconverter plus a USB to USB micro adapater cable to move the Esp32 to PC-independant locations. However if I want to inspect
  the debugging output from the sketch I use a portable PC.



## **Installing:**

-You must create in Domoticz all necessary dummy devices, see "Domoticz-Devices.png"
-You must create a map ESP32_MQTT_DHT11_DS18B20_V2.3 and copy the file "ESP32_MQTT_DHT11_DS18B20_V2.3.ino" into it to compile this sketch.
-You must change the sketch before compiling it. You must enter the IP address of the RPI and your WIFI login credentials. The device numbers from your Domoticz installation should be updated.
-Possibly you want to change the used GPIO ports on the Esp32. All information to be changed is located in declarations in the top of the sketch.
-In the Arduino IDE you need to install:
- the pubsubclient-master.zip library, see also https://pubsubclient.knolleary.net
- the ArduinoJson-5.13.5.zip library
- the OneWire-master.zip library by Paul Stoffregen
- the DallasTemperature-3.8.0 library.
- the Adafruit_Sensor-master.zip library 
- All these libraries can be downloaded from Github.

-You must use the function "Node-Red/Import nodes" of Node-Red to import the file "20200615 Node_Red_V1.2_flows.json" to restore the flows. 
You must change in "setup idx numbers, switches" all the numbers of the idx.. global variables according to your setup in Domoticz, see above, press Deploy and run this function once
It is advised that you change in the flow the connection of the node "Filter RELAY0 or 1" to "process T0 or T1, RELAY0, 1" 4-5 or 6-7 if you use DS18B20 sensor(s) and relays and press Deploy.


## **Author:**

heinohein


**The sketch is based on the work of:**

  DHT11 part is based on sketch by Rui Santos, Complete project details at 
  https://randomnerdtutorials.com/complete-guide-for-dht11dht22-humidity-and-temperature-sensor-with-arduino/
  also Written by ladyada, public domain
  To read from the DHT sensor, we’ll use the DHT library from Adafruit. To use this library you also need 
  to install the Adafruit Unified Sensor library. 

  DS18B20 part is based on an example from the Dallas Temperature library. See webpage by by Rui Santos, 
  Complete project details at 
  https://randomnerdtutorials.com/guide-for-ds18b20-temperature-sensor-with-arduino/
  To interface with the DS18B20 temperature sensor, you need to install the One Wire library by Paul Stoffregen 
  and the Dallas Temperature library.

-  For installing Domoticz see https://www.domoticz.com/wiki/Raspberry_Pi
-  For help with installing the Mosquitto MQTT broker: https://www.domoticz.com/wiki/MQTT for instructions
-  To get kick-started in Node-Red see http://www.steves-internet-guide.com/installing-node-red/ and see his many brilliant video's and explanations
-  For further installation help on Node-Red see https://nodered.org/docs/getting-started/windows
-  For help with the Arduino IDE and installing libraries and boards see https://www.arduino.cc/en/guide/windows
-  For help with proramminglanguages like JS and C++, see https://www.w3schools.com/ and http://www.cplusplus.com
-  More details on DHT11 ao, see https://www.circuitbasics.com/how-to-set-up-the-dht11-humidity-sensor-on-an-arduino/
-  More details on DS18B20, see https://randomnerdtutorials.com/guide-for-ds18b20-temperature-sensor-with-arduino/


They are credited for all there work and thanked for their example code, which gave me ideas for this project

The idea's, the documentation and the software, all files are published under GNU General Public License Version 3.
