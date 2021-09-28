// ---------------------
// Default Libraries
// ---------------------

#include <Streaming.h>
#include <iomanip>



// ------------------
// WiFi Libraries
// ------------------

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <ESP8266HTTPClient.h>

#include <WiFiClientSecureBearSSL.h>



// --------------------------------
// Json Deserialising Libraries
// --------------------------------

#include <ArduinoJson.h>
#include <StreamUtils.h>



// ---------------------------
// 7-Seg Display Libraries
// ---------------------------

#include <TM1638.h>



// --------------------------
// OLED Display Libraries
// --------------------------

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>



// ----------- WiFi Connection & Host Site Connection -----------

const char* SSID = "ssid";           // CHANGE THIS
const char* PASSWORD = "password";  // CHANGE THIS

const uint8_t FINGERPRINT[20] = {0xa4, 0x2f, 0x75, 0xe1, 0xd3, 0x2a, 0x99, 0x88, 0x22, 0x5b, 0x6b, 0x53, 0x69, 0x3a, 0x86, 0xea, 0x05, 0x9a, 0x5b, 0x8d}; // sha-1 fingerprint
const char* HOST_SITE = "https://api.coronatracker.com/v3/stats/worldometer/global";  // site to connect to

ESP8266WiFiMulti WiFiMulti;   // create object of ESP8266WiFiMulti called WiFiMulti



// ----------- 7 Segment Display Initialization -----------
TM1638 ledkey_module(D5, D6, D7, true, 7);



// ----------- OLED Display Initialization -----------
#define OLED_RESET -1

#define OLED_SCREEN_I2C_ADDRESS 0x3C

Adafruit_SSD1306 display(OLED_RESET);



// ----------- Button Reading -----------
int buttonPin = D3;
int BUTTON_COUNT = 0;



// ----------- Initialize Globals ---------
int COUNT = 0;
int TOTAL_CONFIRMED_CASES;    // creating the variables that will contain the data scraped from the JSON
int TOTAL_ACTIVE_CASES;       // taken from the covid api
int TOTAL_DEATHS;
int TOTAL_RECOVERED;
int NEW_CASES;
int TOTAL_NEW_DEATHS;
int CASE_PER_MILLION;
const char* DATE_UPDATED;



// ----------- Function for connecting to wifi -----------

void wifiConnect()
{// this function handles connection to the wifi network(my phone hotspot in this case)

  // for loop counting down from 3
  for (uint8_t t = 4; t > 0; t--) {
    Serial.printf("[SETUP] WAIT %d...\n", t);
    Serial.flush(); // waits for outgoing serial transmission to finish before continuing
    delay(1000);  // 1 second
  }
  // connect to wifi
  
    WiFi.mode(WIFI_STA);
    WiFiMulti.addAP(SSID, PASSWORD);


}



// ----------- function for retrieving data from host site -----------

void getWebData()
{// this function handles connection to the host site, sending the get request
 // and deserialising the JSON data and storing in respective variables

  // wait for connection
  if ((WiFiMulti.run() == WL_CONNECTED))
  {
    //create object of WiFiClientSecure called client
    std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
    Serial.print("WiFi Connected");
    client->setFingerprint(FINGERPRINT);  // set the clients fingerprint to the one 
                                          // in globvar FINGERPRINT

    HTTPClient https; // create object of HTTPClient called https

    Serial.print("[HTTPS] begin...\n");
    if (https.begin(*client, HOST_SITE))  // if https connection is successful run code
    {
      Serial.print("[HTTPS] GET...\n");
      // start connection
      int httpCode = https.GET();

      // httpCode will be negative on error
      if (httpCode > 0)
      {
        Serial.printf("[HTTPS] GET... Code, %d\n", httpCode); // returns the status code

        // file found on host site
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
        {

          const char * json = https.getString().c_str();  // store data returned in variable json
          //Serial.println(json); // error checking
          const size_t capacity = JSON_OBJECT_SIZE(8) + 145; // set memory needed to deserialise JSON data
          DynamicJsonDocument doc(capacity);  // create a DynamicJsonDocument object of name doc with size of capacity
          DeserializationError error = deserializeJson(doc, json);  // error checking

          TOTAL_CONFIRMED_CASES = doc["totalConfirmed"];    // inputting data recieved into respective variables
          TOTAL_ACTIVE_CASES = doc["totalActiveCases"];
          TOTAL_DEATHS = doc["totalDeaths"];
          TOTAL_RECOVERED = doc["totalRecovered"];
          NEW_CASES = doc["totalNewCases"];
          TOTAL_NEW_DEATHS= doc["totalNewDeaths"];
          CASE_PER_MILLION = doc["totalCasesPerMillionPop"];
          DATE_UPDATED = doc["created"];

          if (error)  // if there is an error run code

          {
            Serial.print(F("deserializeJson() failed with code ")); // print code that the deserialisation failed with
            Serial.println(error.c_str());
            delay(10000); // 10 seconds
             return;
          }
        }
      }else // if httpCode < 0 run code
      {
        Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str()); // output http status code returned
      }

      https.end();  // close https connection

    }else // if failure to connect run code
    {
      Serial.printf("[HTTPS] Unable to connect\n");
    }
  }

}



// ----------- function for setting up the OLED screen -----------

void oledSetup()
{

  display.begin(SSD1306_SWITCHCAPVCC, OLED_SCREEN_I2C_ADDRESS);

  display.display();
  delay(2000);

  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1); // 21 chars per line
  display.setTextColor(WHITE);

}



// ----------- function for displaying respective data on the oled screen and 7 seg display -----------

void dataDisplay()
{

  // make sure display is reset each time its required
  display.clearDisplay();
  display.setCursor(0, 0);

  switch (BUTTON_COUNT)
  {
    case 0:
    {
      display << "COVID-19 WORLDWIDE" << endl
      << "Press button to" << endl
      << "switch between data" << endl
      << "to display";
      display.display();
      
      ledkey_module.clearDisplay();
      ledkey_module.setDisplayToString("HELLO", 1, false);
      
      break;
    }
    case 1:
    {
      display << " COVID-19 WORLDWIDE" << endl
      << "Total Cases: " << endl
      << TOTAL_CONFIRMED_CASES;
      display.display();

      ledkey_module.clearDisplay();
      ledkey_module.setDisplayToDecNumber(TOTAL_CONFIRMED_CASES, 1, false);
      
      break;
    }
    case 2:
    {
      display << " COVID-19 WORLDWIDE" << endl
      << "Active Cases: " << endl
      << TOTAL_ACTIVE_CASES;
      display.display();

      ledkey_module.clearDisplay();
      ledkey_module.setDisplayToDecNumber(TOTAL_ACTIVE_CASES, 1, false);
      
      break;
    }
    case 3:
    {
      display << " COVID-19 WORLDWIDE" << endl
      << "Total Deaths: " << endl
      << TOTAL_DEATHS;
      display.display();

      ledkey_module.clearDisplay();
      ledkey_module.setDisplayToDecNumber(TOTAL_DEATHS, 1, false);
      
      break;
    }
    case 4:
    {
      display << " COVID-19 WORLDWIDE" << endl
      << "Total Recovered: " << endl
      << TOTAL_RECOVERED;
      display.display();

      ledkey_module.clearDisplay();
      ledkey_module.setDisplayToDecNumber(TOTAL_RECOVERED, 1, false);
      
      break;
    }
    case 5:
    {
      display << " COVID-19 WORLDWIDE" << endl
      << "Cases Today: " << endl
      << NEW_CASES;
      display.display();

      ledkey_module.clearDisplay();
      ledkey_module.setDisplayToDecNumber(NEW_CASES, 1, false);
      
      break;
    }
    case 6:
    {
      display << " COVID-19 WORLDWIDE" << endl
      << "Cases per Million: " << endl
      << CASE_PER_MILLION;
      display.display();

      ledkey_module.clearDisplay();
      ledkey_module.setDisplayToDecNumber(CASE_PER_MILLION, 1, false);
      
      break;
    }
    case 7:
    {
      Serial.println(BUTTON_COUNT);
      BUTTON_COUNT = 0;
      break;
    }
  }
  

}



// ----------- basic setup function -----------

void setup() {

  Serial.begin(115200); // begin Serial communication on 115200

  // setup the 7 segment display and check its working by displaying hello
  ledkey_module.clearDisplay();
  ledkey_module.setupDisplay(true, 2);
  ledkey_module.setDisplayToString("HELLO", 0, true); 

  oledSetup();  // runs function oledSetup

  pinMode(buttonPin, INPUT_PULLUP); // read button input
  
  Serial.println();
  Serial.println();
  Serial.println();

  wifiConnect();  // run function wifiConnect

}



// ----------- basic loop function -----------

void loop() {

  // one check per loop to check if button has been pressed
  int lastBtnState = 1;  // default state is 1
  int state = digitalRead(buttonPin);
  if (state != lastBtnState)
  {
    lastBtnState = state;
    if (state == 0)
    {
      BUTTON_COUNT +=1;
    }
  }

  dataDisplay();  // runs function oledDisplay

  // clear 7 seg display and then display total worldwide confirmed covid cases
//  ledkey_module.clearDisplay();
//  ledkey_module.setDisplayToDecNumber(TOTAL_CONFIRMED_CASES, 1, false);


  // checks the host site for data updates every 30 seconds (1 loop = 1 second~)
  if (COUNT % 30 == 0)
  {
    
    getWebData(); // run function getWebData 
    if (COUNT > 0) // as to not get spammed with "wait 30s before next check" on first loop
    {
    Serial.println("Wait 30s before next check...");  // wait 30 as to not send too frequent requests
    }
  }

  // small if statement to prevent serial being spammed every loop with the same data
  if (TOTAL_CONFIRMED_CASES > 0 && COUNT % 15 == 0) // every 15 loops run code and only if theres data stored
  {
    Serial.println();
    Serial.println();
    Serial.println();
    Serial.println();
    
    Serial.print("Total Confirmed: ");
    Serial.println(TOTAL_CONFIRMED_CASES);

    Serial.print("Total Active: ");
    Serial.println(TOTAL_ACTIVE_CASES);

    Serial.print("Total Deaths: ");
    Serial.println(TOTAL_DEATHS);

    Serial.print("Total Recovered: ");
    Serial.println(TOTAL_RECOVERED);

    Serial.print("Total New Cases: ");
    Serial.println(NEW_CASES);

    Serial.print("Total New Deaths: ");
    Serial.println(TOTAL_NEW_DEATHS);

    Serial.print("Cases Per Million: ");
    Serial.println(CASE_PER_MILLION);

    Serial.print("Date Updated: ");
    Serial.println(DATE_UPDATED);
  }
  COUNT++; // increment count after each loop
  delay(1000); // 1 second
}
