/*********
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

   11-5-2020 BME220 removed and replaced by DHT11

   This sketch sends a temperature including humidity message (if a DHT11 is used) to domoticz. The return 
   message from domoticz is transformed in node-red to an input message for this sketch. 
   The input message flashes a led to "acknowlegde" the receiving of the temperature and humidity 
   message by domoticz.
   For test purposes together with the temperature and humidity message also a switch on message is send 
   to domoticz. 
   The switch message shows in domoticz often with 5 seconds delay a lightbulb switched on. After a timeout 
   the sketch sends a switch off message, after which the lightbulb extinguishes.
   The temperature/humidity and the switch sensors must be predefined in domoticz at setup/hardware and 
   setup/devices as dummy devices. see https://www.domoticz.com/wiki/MQTT for instructions
   
nano /var/log/mosquitto/mosquitto.log:
1589195865: New client connected from 192.168.2.28 as ESP8266Client (c1, k15).
                                      checked         ************* checked oke
15-5-2020 V1.0 hfmv initial version
19-5-2020 V1.1 hfmv all messages now topic domoticz/in and domoticz/output55, now static defined
                    Json processing of messages
                    led flashes when sending a switch message to domoticz
21-5-2020 V2.0 hfmv support for DS18B20 temperature sensor added
24-5-2020 V2.1 hfmv procoll with MQTT broker changed, relay 1 and 2 added
3-6-2020  V2.2 hfmv fieldnames and procedure names changes SAVED
5-6-2020  V2.3 hfmv retest IDX and relay messages
*********/
#define VERSION "V2.3"
                                             //**************************************************************************
const char* ssid = "";                       // fill in your SSID/Password combination
const char* password = "";
const char* mqtt_server = "192.168.2.100";   // Fill in your MQTT Broker IP address
const int  ledIdx=54;                        // Fill in your idx in domoticz for the Led switch indicator use 2 or 3 digits
const int  dht11TempHumIdx=55;               // Fill in your idx in domoticz for the DHT11 temperature/humidity device
const int  ds18b20TempIdx[]={53,58};         // Fill in your idx number(s)in domoticz for the DS18B20 temperature device(s)
const int  relay0Idx=56;                     // Fill in your idx in domoticz for the relay 0 device
const int  relay1Idx=57;                     // Fill in your idx in domoticz for the relay 1 device
                                             //**************************************************************
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include <OneWire.h>
#include <DallasTemperature.h>
#define ONE_WIRE_BUS 4             // Data wire is plugged into port 4 on the Arduino

// The black wire of a DS18B20 sensor must be connected to GND pin
// The red wire must be connected to the 5V pin
// The yellow wire must be connected to pin P4. P4 must also be connected with a 4700 Ohm resistor to 5V (eg pull up).
// If you use 2x DS18B20 sensors all colered wires must be connected in parallel the yellow ones share the one pull up resistor

#include "DHT.h"
#define DHTPIN 5                   // Digital pin connected to the DHT sensor

#define RELAY0PIN 18               // Output pins for relays
#define RELAY1PIN 19
#define RELAYON LOW                // LOW/HIGH or HIGH/LOW depending on relay hardware
#define RELAYOFF HIGH
int relay0Status=0;                // 0=relay open (OFF), 1=relay closed (ON), 
int relay1Status=0;
int swDomRelay0=0;                 //0=idle, incoming message: 1=idx off, 2= idx on, 3=IDX off, 4=IDX on
int swDomRelay1=0;                  

// ESP8266 note: use pins 3, 4, 5, 12, 13 or 14 --
// Pin 15 can work but DHT must be disconnected during program upload.

// Uncomment whatever type you're using!
#define DHTTYPE DHT11              // DHT 11
//#define DHTTYPE DHT22            // DHT 22  (AM2302), AM2321
//#define DHTTYPE DHT21            // DHT 21 (AM2301)

// NOTE ATTENTION: this is for a 3 pin "Keyes" version of the DHT11
// housing sensor up 
// Connect pin 1 (on the left marked S) to whatever your DHTPIN 
// Connect pin 2 (in the middle ) of the sensor to +5V
// Connect pin 3 (on the right marked -) of the sensor to GROUND

// Initialize DHT sensor.
DHT dht(DHTPIN, DHTTYPE);

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

                                             
char toDomoticz[]   = "domoticz/in";          
char fromDomoticz[] = "domoticz/output55";                 // topic for reduced number of messages generated by domoticz to this esp32
 
char dht11TempHum[] = "{\"idx\":5. ,\"nvalue\":0,\"svalue\":\"28.00;70.00;1\"}"; // send Temp+ Hum message to domoticz/in 55
char dht11Ack[]     = "{\"idx\":.5 ,\"nvalue\":1,\"nr\":15}";    // confirm received by domoticz of TempHum message
char db18B20Temp[]  = "{\"idx\":.  ,\"nvalue\":0,\"svalue\":\"28.00;1\"}"; // send Temp message to domoticz/in for 53/58
char ds18b20Ack[]   = "{\"idx\":. ,\"nvalue\":1,\"nr\":15}";    // confirm received by domoticz of Temp message

char ledStatus[]    = "{\"idx\":5. ,\"nvalue\":1}";        // send Led on/off status to domoticz/in 54 
char ledAck[]       = "{\"idx\":.4 ,\"nvalue\":1}";        // confirm received by domoticz of Led status
char relayIDX[]     = "{\"IDX\":.  ,\"nvalue\":1}";        // Receive from Node-Red Set relay1 or 2 is on/off 
char relayStatus[]  = "{\"idx\":.  ,\"nvalue\":1}";        // send relay1 or 2 on/off status to domoticz/in 56/57
char relayAck[]     = "{\"idx\":.  ,\"nvalue\":1}";        // confirm received by domoticz of relay message

char otherIDX[]     = "{\"IDX\":.  ,\"nvalue\":1}";        // receive from Node-Red not defined device nr idx is on/off 
char otherStatus[]  = "{\"idx\":.  ,\"nvalue\":1}";        // send on/off status to domoticz from not defined device nr idx 
char otherAck[]     = "{\"idx\":.  ,\"nvalue\":1}";        // confirm received by domoticz of message not defined device nr idx 

const int posIdxTempHum=7;                                 // to fill in idx number in the messages
const int posIdxTempHumAck=7;  
const int posIdxTemp=7;
const int posIdxTempAck=7;
const int posIdxLed=7;
const int posIdxLedAck=7;
const int posIDXRelay=7;
const int posIdxRelay=7;
const int posIdxRelayAck=7;
const int posIDXOther=7;
const int posIdxOther=7;
const int posIdxOtherAck=7;

const int posTemp=sizeof(dht11TempHum)-16;                 // to fill in reported value in the messages
const int posHum=sizeof(dht11TempHum)-10;
const int posT=sizeof(db18B20Temp)-10;
const int posLed=sizeof(ledStatus)-3;
const int posLedAck=sizeof(ledAck)-3; 
const int posRelay=sizeof(relayAck)-3;
const int posRelayAck=sizeof(relayAck)-3;
const int posOther=sizeof(otherStatus)-3; 
const int posOtherAck=sizeof(otherAck)-3; 

int messageIdx = 0;                                        // extracted info fields from a input message
int messageIDX = 0;
int nvalue = 0;
int otherIDXValue = 0;                                     // save values incoming IDX message
int otherNvalue=0;

int messageNr = 0;  

WiFiClient espClient;
PubSubClient client(espClient);

char msg[50];                          // workfields
int value = 0;
float temperature = 0;
float humidity = 0;
int messageCnt=0;                      //counter for debug
int swDHTOK=1;                         //=1 DHT is working oke, 0=error, stop prosessing or not present,=2 error found keep trying

// LED Pin
const int LEDPIN = 2;
unsigned long lastLed=0;               //time led was last switched on
unsigned long deltaLed=1500ul;         //time led on after to be switched off 
unsigned long delayLed=1000ul;         //no led on/off change after

unsigned long lastFlash=0;             //time led was switched on for flash
unsigned long deltaFlash=200ul;        //time led is to be switched on during flash
unsigned long delayFlash=500ul;        //no led on/off change after

unsigned long lastLedMess=0;           //time last led message was send 
unsigned long delayLedMess=7000ul;     //time before next led message can be send
int swLedMess=0;                       //0=idle, 2=request to send on message, 1= request to send off message  
int requestLed=0;                      //0=idle, 1=request led off, 2=led on 
int requestFlash=0;                    //0=idle, 1=request led flash

int swDomLed=0;                        //0=idle, incoming message: 1=off, 2=on
int swDomTempHum=0;                    //0=idle, incoming message: 1=off, 2=on
int swDomTemp180=0;                    //0=idle, incoming message: 1=off, 2=on
int swDomTemp181=0;                    //0=idle, incoming message: 1=off, 2=on
int swDomOther=0;                      //0=idle, incoming message: 1=idx off, 2= idx on, 3=IDX off, 4=IDX on

unsigned long lastTemp=0;              //time last temperature message(s) were send to Domoticz
unsigned long delayTemp=30000ul;       //time between Domoticz messages

unsigned long lastRelay=0;             //time last Relay Ack message
unsigned long delayRelay=1000ul;       //time between Relay Ack messages

unsigned long lastOther=0;             //time last Other Ack message
unsigned long delayOther=1000ul;       //time between Other Ack messages

DallasTemperature sensors(&oneWire);   // Pass our oneWire reference to Dallas Temperature. 
int numberOfDevices;                   // Number of temperature devices found
DeviceAddress tempDeviceAddress;       // We'll use this variable to store a found device address


//***************************************************************************************************
void setup() 
{
  Serial.begin(115200);
  Serial.println();
  Serial.print("ESP_MQTT_DHT11/DS18B20 ");
  Serial.println (VERSION);
  
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  if (swDHTOK==1)
  {
    dht.begin();                                   // start DHT sensor readings and test if it works
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    float f = dht.readTemperature(true);

    // Check if any reads failed
    if (isnan(h) || isnan(t) || isnan(f)) 
    {
      Serial.println(F("Failed to read from DHT sensor!"));
      swDHTOK=0;                                   //set to stop processing in loop
    } else
    {
      Serial.println("DHT initially working");
    }
  } else
  { 
    Serial.println("No DHT configured");
  }

  pinMode(LEDPIN, OUTPUT);
  pinMode(RELAY0PIN, OUTPUT);
  pinMode(RELAY1PIN, OUTPUT);

  prepareMessages();                             // prepare and print messages to check parameters settings
 
  sensors.begin();                               // Start up the library for DS18B20
  
  numberOfDevices = sensors.getDeviceCount();    // Grab a count of devices on the wire
  Serial.print("Locating one wire devices...");  // locate devices on the wire bus
  Serial.print("Found ");
  Serial.print(numberOfDevices, DEC);
  Serial.println(" devices.");
  for(int i=0;i<numberOfDevices; i++)            // Loop through each device, print out address              
  {
    if(sensors.getAddress(tempDeviceAddress, i)) // Search the wire for address
    {
      Serial.print("Found device ");
      Serial.print(i, DEC);
      Serial.print(" with address: ");
      printAddress(tempDeviceAddress);
      Serial.println();
    } else 
    {
      Serial.print("Found ghost device at ");
      Serial.print(i, DEC);
      Serial.print(" but could not detect address. Check power and cabling");
    }
  }
}


void setup_wifi() {
//***************
  delay(10);

  Serial.println();                              // connecting to a WiFi network
  Serial.print("Connecting to ");
  Serial.print(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}


// process incomming message. Minimal processing here, all actions are done in the loop
void callback(char* topic, byte* message, unsigned int length)
//************************************************************************************* 
{
  Serial.print("Message in, topic: ");           // debug print incomming message
  Serial.print(topic);
  Serial.print(" Message: ");
  String messageTemp;
  for (int i = 0; i < length; i++) 
  {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();
  
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(messageTemp);  // split Json string
  if(!root.success()) 
  {
    Serial.println("parseObject() failed");
    return;
  }

  messageIdx = root["idx"];                      // extract info fields
  messageIDX = root["IDX"];
  nvalue = root["nvalue"];
  messageNr = root["nr"];
/*   
  Serial.print("idx ");
  Serial.print(messageIdx);
  Serial.print(" IDX ");
  Serial.println(messageIDX);
*/  

  if (messageIdx==dht11TempHumIdx)               //55
  {
    if (swDomTempHum==0)                         // is this the first request to inform Domoticz ?
    {
      swDomTempHum=nvalue+1;                     // therefore value is now 1 or 2
    } else
    {
      Serial.print("swDomTempHum problem ");
      Serial.println(swDomTempHum);
    }
  } else if (messageIdx==ds18b20TempIdx[0])      //53
  {
    if (swDomTemp180==0)                         // is this the first request to inform Domoticz ?
    {
      swDomTemp180=nvalue+1;
    } else
    {
      Serial.print("swDomTemp180 problem ");
      Serial.println(swDomTemp180); 
    }
  } else if (messageIdx==ds18b20TempIdx[1])      //58
  {
    if (swDomTemp181==0)                         // is this the first request to inform Domoticz ?
    {
      swDomTemp181=nvalue+1;
    } else
    {
      Serial.print("swDomTemp181 problem ");
      Serial.println(swDomTemp181);
    }
  } else if (messageIdx==ledIdx)                 //54
  {
    if (swDomLed==0)                             // is this the first request to inform Domoticz ?
    {
      swDomLed=nvalue+1;
    } else
    {
      Serial.print("swDomLed problem ");
      Serial.print(nvalue+1);
      Serial.println(swDomLed);
    }
  } else if (messageIdx==relay0Idx)              //56
  {
    if (swDomRelay0==0)                          // is this the first request to inform Domoticz ?
    {
      swDomRelay0=nvalue+1;                      //0=idle, incoming message: 1=idx off, 2= idx on, 3=IDX off, 4=IDX on
    } else
    {
      Serial.print("swDomRelay0 problem ");
      Serial.println(swDomRelay0);
    }
  } else if (messageIdx==relay1Idx)              //57
  {
    if (swDomRelay1==0)                          // is this the first request to inform Domoticz ?
    {
      swDomRelay1=nvalue+1;
    } else
    {
      Serial.print("swDomRelay1 problem ");
      Serial.println(swDomRelay1);
    }
  } else if (messageIDX==relay0Idx)              //56 and IDX message
  {
    if (swDomRelay0==0)                          // is this the first request to inform Domoticz ?
    {
      swDomRelay0=nvalue+3;
    } else
    {
      Serial.print("swDomRelay0 problem ");
      Serial.println(swDomRelay0);
    }
  } else if (messageIDX==relay1Idx)              //57 and IDX message
  {
    if (swDomRelay1==0)                          // is this the first request to inform Domoticz ?
    {
      swDomRelay1=nvalue+3;
    } else
    {
      Serial.print("swDomRelay1 problem ");
      Serial.println(swDomRelay1);
    }
  } else if (messageIDX != NULL)                 //0=idle, incoming message: 1=idx off, 2= idx on, 3=IDX off, 4=IDX on
  {
    if (swDomOther==0)
    {
      swDomOther=nvalue+3;
      otherIDXValue=messageIDX;                  //extra save, unnesesary?
      otherNvalue=nvalue+3;
    } else
    {
      Serial.print("sw IDX Other problem ");
      Serial.println(swDomOther);
    } 
  } else if (messageIdx != NULL)
  {
    if (swDomOther==0)
    {
      swDomOther=nvalue+1;
    } else
    {
      Serial.print("sw idx Other problem ");
      Serial.println(swDomOther);
    }
  } else
  {
    Serial.print(messageIdx);                       //should never print this
    Serial.print(" ");
    Serial.print(nvalue);
    Serial.print(" ");
    Serial.println(messageNr);
    Serial.println("incoming message skipped");
    return;
  }
} // end callback

//***************************************************************************************
//***************************************************************************************
void loop() 
{
  if (!client.connected())                       //check connection to MQTT broker 
  {
    reconnect();
  }
  client.loop();
  
  if (swDomTempHum>0)                            //complete processing incoming messages
  {
    requestFlash=1;                              //request a flash led
    swDomTempHum=0;                              //0=idle
  }
  if (swDomTemp180>0)
  {
    requestFlash=1;                              //request a flash led
    swDomTemp180=0;                                                           
  }  
  if (swDomTemp181>0)
  {
    requestFlash=1;
    swDomTemp181=0;                                  
  }  
  if (swDomRelay0==1 || swDomRelay0==2)
  {
    relayOnOff(relay0Idx,swDomRelay0);           //switch relay0 on/off, store status
    requestFlash=1;                              //request Led flash
    swDomRelay0=0;                               //0=idle
  }
  if (swDomRelay1==1 || swDomRelay1==2)
  {
    relayOnOff(relay1Idx,swDomRelay1);
    requestFlash=1;
    swDomRelay1=0;                                 
  }
  if (swDomOther==1 || swDomOther==2)
  {
    requestFlash=1;                               
    swDomOther=0;                                //0=idle                                 
  }
  if (swDomLed>0)
  {
    swDomLed=0;                                  //0=idle                                 
  }
  if (swDomTempHum>0)
  {
    swDomTempHum=0;                              // 0=idle
  }
  if (swDomTemp180>0)
  {
    swDomTemp180=0;                              // 0=idle
  }
  if (swDomTemp181>0)                 
  {
    swDomTemp181=0;                 
  }
  
  unsigned long now = millis();
  if (now >= lastTemp)                           //check if it is time to send new temperature readings 
  {
    if (swDHTOK>=1)
    {
      buildTempHumMessage();
      if (swDHTOK==1)
      {
        client.publish(toDomoticz, dht11TempHum);  // send result to domoticz
        requestLed=2;                                //request Led on
        swLedMess=2;                                 //request send led message on
      }
    }
    if (numberOfDevices>0)                       // are there any DS18B20 temperature sensor? 
    {
      for(int i=0;i<numberOfDevices; i++) 
      {
        buildTempMessage(i);                     // send temperature only message(s) to domoticz      
        client.publish(toDomoticz, db18B20Temp);
        requestLed=2;                                //request Led on
        swLedMess=2;                                 //request send led message on
      }
    }
    lastTemp = millis()+ delayTemp;
  }  
  if (now >= lastRelay)                          //check if time to relay Ack message to Domoticz is expired
  {
    if (swDomRelay0>0)                           //0=idle, incoming message:3=IDX off, 4=IDX on; send Status message
    {
      relayOnOff(relay0Idx,swDomRelay0);
      buildRelayStatus(relay0Idx,swDomRelay0);
      client.publish(toDomoticz, relayStatus);   //send relay is on/off message to domoticz
      lastRelay=millis()+ delayRelay;
      swDomRelay0=0;                             //0=idle
      requestLed=2;                              //request Led on
      swLedMess=2;                               //request send led message on
    }
    if (swDomRelay1>0)                     
    {
      relayOnOff(relay1Idx,swDomRelay1);
      buildRelayStatus(relay1Idx,swDomRelay1);
      client.publish(toDomoticz, relayStatus);   
      lastRelay=millis()+ delayRelay;
      swDomRelay1=0;
      requestLed=2;                              
      swLedMess=2;                                                      
    }
  }
  if (now >= lastOther)                          //check if time to send other status message to Domoticz is expired
  {
    if (swDomOther>2)                            //0=idle, incoming message: 1=idx off, 2= idx on, 3=IDX off, 4=IDX on
    {
      buildOtherStatus(otherIDXValue, otherNvalue);
      client.publish(toDomoticz, otherStatus);   //send other status message to domoticz
      lastOther=millis()+ delayOther;
      swDomOther=0;                              // 0=idle
      requestLed=2;                              //request Led on
      swLedMess=2;                               //request send led message on
      otherIDXValue=0;                           //0=idle
    }      
  }  
  
  if (now >= lastLed)                            //check if time to switch led on/off is expired
  {
    if (requestLed==2)                          
    {
      digitalWrite(LEDPIN, HIGH);
      Serial.println("led ON");
      lastLed=millis()+ deltaLed;
      lastFlash=lastLed+                         // no Flash during led "period"
          delayLed;                              //no led on/off change after
      requestLed=1;                              //1=request to switch led off
    } else if (requestLed==1)
    {
      digitalWrite(LEDPIN, LOW);
      Serial.println("led OFF");
      lastLed=millis()+ delayLed;
      lastFlash=millis()+
          delayLed;                              //no led on/off change after led was off
      requestLed=0;
    }
  }

  if (now >= lastLedMess)                        //check if time to send led messages to Domoticz is expired
  {
    if (swLedMess==2)                            // check if domoticz needs to be informed/updated
    {
      buildledStatus(ledIdx, 1);
      client.publish(toDomoticz, ledStatus);     //send led status message to domoticz
      lastLedMess=millis()+ delayLedMess;
      swLedMess=1;
    } else if (swLedMess==1)
    {
      buildledStatus(ledIdx, 0);
      client.publish(toDomoticz, ledStatus);     //send led status message to domoticz
      lastLedMess=millis()+ delayLedMess;
      swLedMess=0;
    } 
  }

  if (requestFlash>0 && now >= lastFlash)        //check if time to Flash the led is expired
  {
      digitalWrite(LEDPIN, HIGH);                // flash led to indate send message to Domoticz
      delay(deltaFlash);
      digitalWrite(LEDPIN, LOW);
      Serial.println("flash");
      lastFlash=millis()+ delayFlash;
      requestFlash=0;
  }
//  delay(1000);                                 // minimal wait
//  Serial.print(millis()/1000);
//  Serial.println(" end loop");
}  // end loop

//**********************************************************************
//**********************************************************************
//**********************************************************************
void buildledStatus(int messageIdx, int messageStatus)
{
  char idxString[8];                             //prepare messages to domoticz
  char *p1;
  char *p2;
  
  itoa (messageIdx, idxString, 10);              //convert int to char 54
  p1=idxString;
  p2=ledStatus+posIdxLed;
  *p2++=*p1++;                                   //copy 2 bytes derefferenced
  *p2=*p1;
  if (messageStatus==1)
  {
    ledStatus[posLed]='1'; 
  } else if (messageStatus==0)
  {
    ledStatus[posLed]='0';
  } else
  {  
    ledStatus[posLed]='?';
  }
  Serial.println(ledStatus);       
}


void buildledAck(int messageIdx, int messageStatus)
{
  char idxString[8];                             //prepare messages to domoticz
  char *p1;
  char *p2;
  
  itoa (messageIdx, idxString, 10);              //convert int to char 54
  p1=idxString;
  p2=ledAck+posIdxLedAck;
  *p2++=*p1++;                                   //copy 2 bytes derefferenced
  *p2=*p1;
  if (messageStatus==1)
  {
    ledAck[posLedAck]='1'; 
  } else if (messageStatus==0)
  {
    ledAck[posLedAck]='0';
  } else
  {  
    ledAck[posLedAck]='?';
  }
  Serial.println(ledAck);          
}


//**********************************************************************
void buildOtherStatus(int messageIdx, int messageStatus)
{
  char idxString[8];                             //prepare messages to domoticz
  char *p1;
  char *p2;
  
  itoa (messageIdx, idxString, 10);              //convert int to char 54
  p1=idxString;
  p2=otherStatus+posIdxOther;
  *p2++=*p1++;                                   //copy 2 bytes derefferenced
  *p2=*p1;
  if (messageStatus==4)                          //0=idle, incoming message: 1=idx off, 2= idx on, 3=IDX off, 4=IDX on
  {
    otherStatus[posOther]='1'; 
  } else if (messageStatus==3)
  {
    otherStatus[posOther]='0';
  } else
  {  
    otherStatus[posOther]='?';
  }
  Serial.println(otherStatus);       
}


void buildOtherAck(int messageIdx, int messageStatus)
{
  char idxString[8];                             //prepare messages to domoticz
  char *p1;
  char *p2;
  
  itoa (messageIdx, idxString, 10);              //convert int to char 54
  p1=idxString;
  p2=otherAck+posIdxOtherAck;
  *p2++=*p1++;                                   //copy 2 bytes derefferenced
  *p2=*p1;
  if (messageStatus==1)
  {
    otherAck[posOtherAck]='1'; 
  } else if (messageStatus==0)
  {
    otherAck[posOtherAck]='0';
  } else
  {  
    otherAck[posOtherAck]='?';
  }
  Serial.println(otherAck);        
}


//**********************************************************************
// send the temperature and humidity measurements from the DHT11 to domoticz
void buildTempHumMessage()
{
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) 
  {
    if (swDHTOK==1)
    {
      Serial.println(F("Failed to read from DHT sensor!"));
      delay(1000);
      swDHTOK=2;                                   //set to stop processing in loop, but keep trying
      return;
    } else                                         // was 2 and stays 2
    {
      return;
    }
  }

  swDHTOK=1;                                       //read is (now) oke
  // Compute heat index in Fahrenheit (the default)
  float hif = dht.computeHeatIndex(f, h);
  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(t, h, false);
    
  Serial.print(++messageCnt);
  Serial.print(F(" Humidity: "));
  Serial.print(h);
  Serial.print(F("%  Temperature: "));
  Serial.print(t);
  Serial.print(F("°C "));
  Serial.print(f);
  Serial.print(F("°F  Heat index: "));
  Serial.print(hic);
  Serial.print(F("°C "));
  Serial.print(hif);
  Serial.println(F("°F"));
 
  // Convert the values to a char array
  char tempString[8];
  char *p1;       
  char *p2;       

  itoa (dht11TempHumIdx, tempString, 10);        //convert int to char 55
  p1=tempString;
  p2=dht11TempHum+posIdxTempHum;
  *p2++=*p1++;                                   //copy 2 bytes derefferenced
  *p2=*p1;
  
  dtostrf(t, 1, 2, tempString);                  //convert double to char string
  p1 = tempString;
  p2 = dht11TempHum+posTemp;
  while(*p1 != '\0')                             // dereferenced byte 
  {
    *p2++ = *p1++;                               // copy 1 byte dereferenced)
  }

  char humString[8];
  dtostrf(h, 1, 2, humString);
  //Serial.print("Humidity: ");
  //Serial.println(humString);
  p1 = humString;
  p2 = dht11TempHum+posHum;
  while(*p1 != '\0')
  {
    *p2++ = *p1++;
  }
  Serial.println(dht11TempHum);                  //output={"idx":55, "nvalue":0,"svalue":"23.00;33.00;1"}
}


//**********************************************************************
// send the temperature measurements from the DS18B20 to domoticz
void buildTempMessage(int numberOfDevice)
{
  sensors.requestTemperatures();                 // Send the command to get temperatures
  
  // Loop through each device, print out temperature data, send message to domoticz
  float tempC=0;                                 // work fields
  char tempString[8];
  char idxString[8];
  char *p1;                                       
  char *p2;

  // Search the wire for address
  if (sensors.getAddress(tempDeviceAddress, numberOfDevice))
  {
    Serial.print("Temperature for device: ");
    Serial.print(numberOfDevice,DEC);            // Output the device ID
    Serial.print(" ");
 
    tempC = sensors.getTempC(tempDeviceAddress);
    Serial.print("Temp C: ");
    Serial.print(tempC);
    Serial.print(" Temp F: ");
    Serial.println(DallasTemperature::toFahrenheit(tempC)); // Converts tempC to Fahrenheit
        
    itoa (ds18b20TempIdx[numberOfDevice], idxString, 10);   //convert idx number to char
    p1=idxString;
    p2=db18B20Temp+posIdxTemp;
    *p2++=*p1++;                                 //copy 2 bytes derefferenced
    *p2=*p1;
        
    dtostrf(tempC, 1, 2, tempString);            // Convert the temperature to a char array
    p1 = tempString;
    p2 = db18B20Temp+posT;
    while(*p1 != '\0')                           // dereferenced byte 
    {
      *p2++ = *p1++;                             // copy 1 byte dereferenced)
    }
    Serial.println(db18B20Temp);                 //output={"idx":53 ,"nvalue":0,"svalue":"24.19;1"}
  }
}


//**********************************************************************
// send relayAck status message to Domoticz
void buildRelayStatus(int relayIdx,int relayNewStatus)
{
  char *p1;
  char *p2;
  char idxString[8];
  itoa (relayIdx, idxString, 10);            //56 or 57 
  p1=idxString;
  p2=relayStatus+posIdxRelay;
  *p2++=*p1++;                               //copy 2 bytes derefferenced
  *p2=*p1;
  if (relayNewStatus==2 || relayNewStatus==4)
  {
    relayStatus[posRelay]='1';                    
  } else if (relayNewStatus==1 || relayNewStatus==3)
  {
    relayStatus[posRelay]='0';                    
  } else
  {
    relayStatus[posRelay]='?';
  }
  Serial.println(relayStatus);
}

void relayOnOff(int relayIdx,int relayNewStatus)
{
  if (relayIdx==relay0Idx)
  {
     if (relayNewStatus==2 || relayNewStatus==4)
     {
        digitalWrite(RELAY0PIN, RELAYON);
        relay0Status=1;
     } else if (relayNewStatus==1 || relayNewStatus==3)
     {
        digitalWrite(RELAY0PIN, RELAYOFF);
        relay0Status=0;
     } else
     {
        digitalWrite(RELAY0PIN, RELAYOFF);
        relay0Status=9;                         //error, should never happen
     }
  }
  if (relayIdx==relay1Idx)
  {
     if (relayNewStatus==2 || relayNewStatus==4)
     {
        digitalWrite(RELAY1PIN, RELAYON);
        relay1Status=1;
     } else if (relayNewStatus==1 || relayNewStatus==3)
     {
        digitalWrite(RELAY1PIN, RELAYOFF);
        relay1Status=0;
     } else
     {
        digitalWrite(RELAY1PIN, RELAYOFF);
        relay1Status=9;                         //error, should never happen
     }
  }
  Serial.print("Relays ");
  Serial.print(relay0Status);
  Serial.print(" ");
  Serial.println(relay1Status);
}

void buildRelayAck(int relayIdx,int relayNewStatus)
{
  char *p1;
  char *p2;
  char idxString[8];
  itoa (relayIdx, idxString, 10);            //56 or 57 
  p1=idxString;
  p2=relayAck+posIdxRelay;
  *p2++=*p1++;                               //copy 2 bytes derefferenced
  *p2=*p1;
  if (relayNewStatus==2)
  {
    relayAck[posRelay]='1';                    
  } else if (relayNewStatus==1)
  {
    relayAck[posRelay]='0';                    
  } else
  {
    relayAck[posRelay]='?';
  }
  Serial.println(relayAck);
}


// Debug: prepare and print messages to check parameters settings
void prepareMessages()
{
  buildTempHumMessage(); //55
  //buildTempHumAck(); //55
  buildTempMessage(0); //53,58
  //buildTempAck(); //53,58
  buildledStatus(ledIdx, 0); //54
  buildledAck(ledIdx, 0); //54
  buildRelayStatus(relay0Idx, 0); //56,57
  buildRelayAck(relay0Idx, 0); //56,57
  buildOtherStatus(11, 0); //other
  buildOtherAck(12, 0); //other
}


// function to print a DS18B20 temperature sensor device address
void printAddress(DeviceAddress deviceAddress) 
{
  for (uint8_t i = 0; i < 8; i++) 
  {
    if (deviceAddress[i] < 16) 
      Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}

void reconnect() 
{
  // Loop until we're reconnected
  while (!client.connected()) 
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client"))         // ESP8266 name checked with mosquitto.log is correct 
    {
      Serial.println("connected");
      client.subscribe(fromDomoticz);            // Subscribe to topic is done here
    } else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
