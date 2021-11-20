//For Dashboard please go at: https://io.adafruit.com/aizaz/dashboards/coursework 
//Name: Muhammad Aizaz Afzal
//Student Number: 200261713
//Aston University

/*****LIBRARIES*****/

#include <RFID.h>
#include <ESP8266WiFi.h>
#include <SPI.h>
#include <WiFiUdp.h>
#include "rgb_lcd.h"
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

/*****DEFINING MACROS*****/

#define SlaveSelect_PIN 0
#define Reset_PIN       2
#define LED             15
#define AnalogPin       A0
#define ssid            "ABG_Wireless" // Enter your hotspot/WiFi SSID
#define password        ""             // Enter your hotspot/WiFi Password
#define TASK1L          100
#define TASK2L          200
#define TASK3L          300
#define TIMEOUTVAL      250
#define LINES           8
#define LEN             150
#define SOCKET          8888
#define BUFFERLEN       255
#define PORT            80
/*********************************** Adafruit IO defines*********************************************/
#define NOPUBLISH                           // If publishing once in more than 10 second intervals
#define ADASERVER     "io.adafruit.com"     // Adafruit Server
#define ADAPORT       8883                  // Encrypted Port
#define ADAUSERNAME   "aizaz"               // Adafruit Username
#define ADAKEY        "aio_zsar12bJ6iZJ4iLeQSAMZmJEptDb" // Adafruit key 

/*****INITIALIZING VARIABLES*****/

int CurrentCard = 0;
int LastCard = 0;
int count = 0;
int tempcount = 0;
float Temp;
float TEMP_ARRAY[10];       // To store last 10 temperature values
float lower = 36.5;         // Default value for lower limit of temperature
float upper = 38;           // Default value for upper limit of temperature
unsigned long TASK1LC = 0;
unsigned long TASK2LC = 0;
unsigned long TASK3LC = 0;
String uid[12];             // To store the RFID UID
char RFID_NUM[] = {"\n"};
char Location[] = "Birmingham Museum Main Enterance";
char message [13];          // character string of 12 characters with 13 being null character
char dashboard_address[] = "https://io.adafruit.com/aizaz/dashboards/coursework";
char inputbuf[LINES][LEN] = {};
char inBUFFER[BUFFERLEN];
char outBUFFER[BUFFERLEN];

/******************************** Global instances / variables***************************************/
WiFiClientSecure client;    // Class instance for the MQTT server
WiFiServer SERVER(PORT);    // Port used by HTTP
WiFiClient Client;          //
rgb_lcd lcd;
WiFiUDP UDP;
RFID myrfid(SlaveSelect_PIN, Reset_PIN);          // create a class instance of the rgb_lcd class
static const char *fingerprint PROGMEM = "59 3C 48 0A B1 8B 39 4E 0D 58 50 47 9A 13 55 60 CC A0 1D AF";
// create an instance of the Adafruit MQTT class. This requires the client, server, portm username and the Adafruit key
Adafruit_MQTT_Client MQTT(&client, ADASERVER, ADAPORT, ADAUSERNAME, ADAKEY);

/******************************** Feeds *************************************************************/
//Feeds we publish to
Adafruit_MQTT_Publish TEMP = Adafruit_MQTT_Publish(&MQTT, ADAUSERNAME "/feeds/temperature");
Adafruit_MQTT_Publish LOCATION = Adafruit_MQTT_Publish(&MQTT, ADAUSERNAME "/feeds/device-location");
Adafruit_MQTT_Publish UID = Adafruit_MQTT_Publish(&MQTT, ADAUSERNAME "/feeds/rfid");
Adafruit_MQTT_Publish CHANGE_LOWER_LMT = Adafruit_MQTT_Publish(&MQTT, ADAUSERNAME "/feeds/lower-limit");
Adafruit_MQTT_Publish CHANGE_UPPER_LMT = Adafruit_MQTT_Publish(&MQTT, ADAUSERNAME "/feeds/upper-limit");
// Subscribed Feeds
Adafruit_MQTT_Subscribe LOWER_LMT = Adafruit_MQTT_Subscribe(&MQTT, ADAUSERNAME "/feeds/lower-limit");
Adafruit_MQTT_Subscribe UPPER_LMT = Adafruit_MQTT_Subscribe(&MQTT, ADAUSERNAME "/feeds/upper-limit");


/******FUNCTION PROTOTYPES******/

int checkRFID(void);
void MQTTconnect (void);
void Subscription(void);
void Publishing(void);
void Webpage (void);
void servepage (void);
void Broadcast (void);

/*****SETUP FUNCTION STARTS HERE*****/

void setup()                  // the setup function is called once on startup
{
  Serial.begin(115200);       // initialise serial, for communication with the host pc
  while (!Serial)             // while the serial port is not ready
  {
    ;                         // do nothing
  }
  delay(500);
  Serial.print("Attempting to connect to ");    // Inform of us connecting
  Serial.print(ssid);                           // print the ssid over serial

  WiFi.begin(ssid, password);                   // attemp to connect to the access point SSID with the password

  while (WiFi.status() != WL_CONNECTED)         // whilst we are not connected
  {
    delay(500);                                 // wait for 0.5 seconds (500ms)
    Serial.print(".");                          // print a .
  }
  Serial.print("\n");                           // print a new line

  Serial.println("Succesfully connected");      // let us now that we have now connected to the access point
  Serial.print("IP:  ");                        // print the IP address
  Serial.println(WiFi.localIP());               // In the same way, the println function prints all four IP address bytes
  Serial.println("");
  pinMode(LED, OUTPUT); // pin 15 set as output pin
  lcd.begin(16, 2);
  lcd.setRGB(0, 255, 0);
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());                    // update the display with the local (i.e. board) IP address
  client.setFingerprint(fingerprint);
  MQTT.subscribe(&LOWER_LMT);                   // Subscribing to MQTT Feeds
  MQTT.subscribe(&UPPER_LMT);
  SPI.begin();
  myrfid.init();
  MQTTconnect();
  Serial.println("");
  SERVER.begin();                               // Trying to connect to HTTP server
  Serial.println("HTTP based webpage connected");
  Serial.print("Please access webpage at http://");
  Serial.println(WiFi.localIP());
  Serial.println("");
  if (UDP.begin(SOCKET))
  {
    Serial.println("UDP Port opened successfully!!!");
  }
  else
    ;
}

/*****LOOP FUNCTION STARTS HERE*****/

void loop()
{
  count = tempcount;
  count = count % 10; //runs from 0 to 9 only
  tempcount = count;
  lcd.setCursor(0, 0);
  lcd.print("Scan RFID Card");
  Subscription();    // Subscription to MQTT Feeds
  unsigned long current_millis = millis();
  if ((current_millis - TASK1LC) >= TASK1L)
  {
    TASK1LC = millis();
    Publishing();    // Check RFID and Publishing on dashboard every 100 ms
  }
  if ((current_millis - TASK2LC) >= TASK2L)
  {
    TASK2LC = millis();
    Webpage();        // Establishing onnection to webpage using HTTP every 200 ms
  }
  if ((current_millis - TASK3LC) >= TASK3L)
  { TASK3LC = millis();
    Broadcast();     // UDP broadcasting every 300 ms
  }
}

/******************************* MQTT connect *******************************************************/
void MQTTconnect ()
{
  unsigned char tries = 0;
  // Stop if already connected.
  if ( MQTT.connected() )
  {
    return;
  }
  Serial.print("Connecting to MQTT... ");
  while ( MQTT.connect() != 0 )                                    // while we are
  {
    Serial.println("Will try to connect again in five seconds");   // inform user
    MQTT.disconnect();                                             // disconnect
    delay(5000);                                                   // wait 5 seconds
    tries++;
    if (tries == 3)
    {
      Serial.println("problem with communication, forcing WDT for reset");
      while (1)
      {
        ;   // forever do nothing
      }
    }
  }
  Serial.println("MQTT succesfully connected!");
  Serial.print("Please access dashboard at ");
  Serial.println(dashboard_address);
}

void Publishing()
{
  if (checkRFID())
  {
    tempcount++;
    TEMP.publish(Temp);
    LOCATION.publish(Location);
    UID.publish(message);
#ifdef NOPUBLISH
    if ( !MQTT.ping() )
    {
      MQTT.disconnect();
    }
#endif
  }
}

int checkRFID() // Body of checkRFID function
{
  CurrentCard = myrfid.isCard();
  if (CurrentCard && !LastCard)
  {
    if (myrfid.readCardSerial())
    {
      for (int i = 0; i < 4; i++) // Loop for comparing UID of employee
      {
        RFID_NUM[i] = myrfid.serNum[i];
      }
      sprintf(message, "%d%d%d%d", RFID_NUM[0], RFID_NUM[1], RFID_NUM[2], RFID_NUM[3]);
      uid[count] = {message};
      lcd.clear();
      Temp = analogRead(AnalogPin);
      Temp = (Temp / 51.2) + 25;
      TEMP_ARRAY[count] = Temp;
      lcd.print("Temp: ");
      lcd.setCursor(6, 0);
      lcd.print(Temp);
      lcd.setCursor(11, 0);
      lcd.print("*C");
      count++;
      if (Temp > lower && Temp < upper)
      {
        digitalWrite(LED, HIGH);
        lcd.setCursor(0, 1);
        lcd.print("Door opening...");
        delay(1000);
        lcd.clear();
        digitalWrite(LED, LOW );
        return 1;
      }
      else
      {
        lcd.setCursor(0, 1);
        lcd.print("Self Isolate!");
        delay(1000);
        lcd.clear();
        return 1;
      }
    }
  }
  LastCard = CurrentCard;
  myrfid.halt();
  return 0;
}
void servepage ()
{
  String reply;
  reply += "<!DOCTYPE HTML>";                                  // start of actual HTML file
  reply += "<html>";                                           // start of html
  reply += "<head>";                                           // the HTML header
  reply += "<title>AIZAZ's IoT Coursework</title>";            // page title as shown in the tab
  reply += "</head>";
  reply += "<body>";                                           // start of the body
  reply += "&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;";
  reply += "<h1>AntiSars</h1>";                                // heading text
  reply += "Device Location:  ";
  reply += Location;
  reply += "<br>";
  reply += "<br>";
  reply += "<button><a href=""/>Refresh</a></button>";
  reply += "<br>";
  reply += "<button><a href=""/?upper++"">Increase Upper Limit (+0.5)</a></button>";
  reply += "&emsp;&emsp;";
  reply += "<button><a href=""/?upper--"">Decrease Upper Limit (-0.5)</a></button>";
  reply += "<br>";
  reply += "<button><a href=""/?lower++"">Increase Lower Limit (+0.5)</a></button>";
  reply += "&emsp;&emsp;";
  reply += "<button><a href=""/?lower--"">Decrease Lower Limit (-0.5)</a></button>";
  reply += "<br>";
  for (count = 0; count < 10 ; count++)  // Printing values from last 10 access attempts
  {
    reply += "<br>";
    reply += "--------------------------------------------------------------------------";
    reply += "<br>";
    reply += "Temperature: ";
    reply += TEMP_ARRAY[count];
    reply += "&emsp;&emsp;";
    reply += "UID: ";
    reply += uid[count];
    reply += "<br>";
    reply += "--------------------------------------------------------------------------";
  }
  reply += "</body>";                                          // end of body
  reply += "</html>";                                          // end of HTML
  Client.printf("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: %u\r\n\r\n%s", reply.length(), reply.c_str());
}

void Webpage ()
{
  int timecnt = 0;
  int i;
  Client = SERVER.available();
  if (Client)
  {
    while (!Client.available())
    {
      delay(1);
      timecnt++;
      if (timecnt >= TIMEOUTVAL)
      {
        return;
      }
    }
    for (i = 0; i < LINES; i++)
    {
      Client.readStringUntil('\r').toCharArray(&inputbuf[i][0], LEN);
    }
    if (!strcmp(inputbuf[0], "GET /?upper++ HTTP/1.1"))
    {
      upper += 0.5;
      CHANGE_UPPER_LMT.publish(upper);
    }
    if (!strcmp(inputbuf[0], "GET /?upper-- HTTP/1.1"))
    {
      upper -= 0.5;
      CHANGE_UPPER_LMT.publish(upper);
    }
    if (!strcmp(inputbuf[0], "GET /?lower-- HTTP/1.1"))
    {
      lower -= 0.5;
      CHANGE_LOWER_LMT.publish(lower);
    }
    if (!strcmp(inputbuf[0], "GET /?lower++ HTTP/1.1"))
    {
      lower += 0.5;
      CHANGE_LOWER_LMT.publish(lower);
    }
    Client.flush();
    servepage();
    Client.stop();
  }
}

void Broadcast ()
{
  int packetsize = 0;
  packetsize = UDP.parsePacket();
  if (packetsize)
  {
    UDP.read(inBUFFER, BUFFERLEN);
    inBUFFER[packetsize] = '\0';
    Serial.print("Received ");
    Serial.print(packetsize);
    Serial.println(" bytes");
    Serial.println("Contents");
    Serial.println(inBUFFER);
    Serial.print("From IP ");
    Serial.println(UDP.remoteIP());
    Serial.print("From port ");
    Serial.println(UDP.remotePort());
    UDP.beginPacket(UDP.remoteIP(), UDP.remotePort());
    strcpy(outBUFFER, WiFi.localIP().toString().c_str());
    UDP.write(outBUFFER);
    UDP.endPacket();
  }
}

void Subscription()
{
  Adafruit_MQTT_Subscribe *subscription;                   // create a subscriber object instance
  while ( subscription = MQTT.readSubscription(500) )      // Read a subscription and wait for max of 500 milliseconds.
  {
    if (subscription == &LOWER_LMT)                        // if the subscription we have receieved matches the one we are after
    {
      lower = atof((char *)LOWER_LMT.lastread);
    }
    if (subscription == &UPPER_LMT)                        // if the subscription we have receieved matches the one we are after
    {
      upper = atof((char *)UPPER_LMT.lastread);
    }
  }
}
