/**
 * IotWebConf03CustomParameters.ino -- IotWebConf is an ESP8266/ESP32
 *   non blocking WiFi/AP web configuration library for Arduino.
 *   https://github.com/prampec/IotWebConf 
 *
 * Copyright (C) 2020 Balazs Kelemen <prampec+arduino@gmail.com>
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 * Hardware setup for this example:
 *   - An LED is attached to LED_BUILTIN pin with setup On=LOW.
 *   - [Optional] A push button is attached to pin D2, the other leg of the
 *     button should be attached to GND.
 * ***** COMMENTS FOR IOTWEBCONF START WITH "// --"
 */
// Wire Master Writer
// by devyte
// based on the example of the same name by Nicholas Zambetti <http://www.zambetti.com>
// Demonstrates use of the Wire library
// Writes data to an I2C/TWI slave device
// Refer to the "Wire Slave Receiver" example for use with this
// This example code is in the public domain.
//  COMMENTS FOR WIRE TRANSMISSION START WITH "// ** "
/*
 * RTC DS3231
 * 
 * COMMENTS START WITH "// oo"
 * 
 */

// --
#include <IotWebConf.h>
#include <IotWebConfUsing.h> // This loads aliases for easier class names.
#include <IotWebConfOptionalGroup.h>

// **
#include <Wire.h>
#include <PolledTimeout.h>

// oo RTC
#include <microDS3231.h>
MicroDS3231 rtc;
int MIN, HOUR, DAY, MONTH, YEAR;
boolean needToResetTime = false ;

// -- Initial name of the Thing. Used e.g. as SSID of the own Access Point.
const char thingName[] = "Maregramme";

// -- Initial password to connect to the Thing, when it creates an own Access Point.
const char wifiInitialApPassword[] = "1234abcd";
#define STRING_LEN 128

// -- Configuration specific key. The value should be modified if config structure was changed.
#define CONFIG_VERSION "v1"

// -- When CONFIG_PIN is pulled to ground on startup, the Thing will use the initial
//      password to buld an AP. (E.g. in case of lost password)
#define CONFIG_PIN D2

// -- Status indicator pin .
//      First it will light up (kept LOW), on Wifi connection it will blink,
//      when connected to the Wifi it will turn off (kept HIGH).
#define STATUS_PIN LED_BUILTIN

// ** Wire transmission settings
#define SDA_PIN 4
#define SCL_PIN 5
const int16_t I2C_MASTER = 0x42;
const int16_t I2C_COLORDUINO = 0x09; // for COLORDUINO (can be changed by software in colorduino)
// const int16_t I2C_CLOCK = 0x57; // for DS3231 (one of the two available addresses)
byte message = 0; // init

// -- Method declarations.
void handleRoot();
// -- Callback methods.
void configSaved();
bool formValidator(iotwebconf::WebRequestWrapper* webRequestWrapper);

// --
DNSServer dnsServer;
WebServer server(80);

// -- 
char portParamValue[STRING_LEN];  // to choose port
char dateRTCValue[STRING_LEN]; 
char timeRTCValue[STRING_LEN]; 
char dstParamValue[STRING_LEN];


// -- port options
static char portValues[][STRING_LEN] = { "94", "160" };
static char portNames[][STRING_LEN] = { "Bayonne", "Concarneau" };

// --
IotWebConf iotWebConf(thingName, &dnsServer, &server, wifiInitialApPassword, CONFIG_VERSION);
// -- You can also use namespace formats e.g.: iotwebconf::TextParameter
//IotWebConfTextParameter stringParam = IotWebConfTextParameter("String param", "stringParam", stringParamValue, STRING_LEN);
// -- We can add a legend to the separator
IotWebConfParameterGroup choosePortGroup = IotWebConfParameterGroup("p_settings", "Port settings");
IotWebConfSelectParameter portParam = IotWebConfSelectParameter("Choose port", "choosePort", portParamValue, STRING_LEN, (char*)portValues, (char*)portNames, sizeof(portValues) / STRING_LEN, STRING_LEN);

iotwebconf::OptionalParameterGroup setDateTimeGroup = iotwebconf::OptionalParameterGroup("AdjustDate", "Set date / time", needToResetTime); 
iotwebconf::TextParameter dateRTCParam = iotwebconf::TextParameter("Set Date dd/mm/yyyy", "dateRTC", dateRTCValue, STRING_LEN);
iotwebconf::TextParameter timeRTCParam = iotwebconf::TextParameter("Set Time hh:mm (24h)", "timeRTC", timeRTCValue, STRING_LEN);
iotwebconf::CheckboxParameter dstParam = iotwebconf::CheckboxParameter("DST ON", "timeDst", dstParamValue, STRING_LEN,  false);

// -- An instance must be created from the class defined above.
iotwebconf::OptionalGroupHtmlFormatProvider optionalGroupHtmlFormatProvider;

void setup() 
{
  Serial.begin(115200);
  Serial.println();
  Serial.println("Starting up...");

// **  
  Wire.begin(SDA_PIN, SCL_PIN, I2C_MASTER); // join i2c bus (address optional for master)

// --  
  choosePortGroup.addItem(&portParam);

  setDateTimeGroup.addItem(&dateRTCParam);
  setDateTimeGroup.addItem(&timeRTCParam);
  setDateTimeGroup.addItem(&dstParam);
  
  iotWebConf.setStatusPin(STATUS_PIN);
  iotWebConf.setConfigPin(CONFIG_PIN);
  iotWebConf.addParameterGroup(&choosePortGroup);
  iotWebConf.setHtmlFormatProvider(&optionalGroupHtmlFormatProvider);
  iotWebConf.addParameterGroup(&setDateTimeGroup);
  iotWebConf.setConfigSavedCallback(&configSaved);
  iotWebConf.setFormValidator(&formValidator);
  iotWebConf.getApTimeoutParameter()->visible = true;

  // -- Initializing the configuration.
  iotWebConf.init();

  // -- Set up required URL handlers on the web server.
  server.on("/", handleRoot);
  server.on("/config", []{ iotWebConf.handleConfig(); });
  server.onNotFound([](){ iotWebConf.handleNotFound(); });

  Serial.println("Ready.");

  // oo  
  if (rtc.lostPower()) {            // will be executed if the RTC lost power
    Serial.println("lost power!");
    rtc.setTime(BUILD_SEC, BUILD_MIN, BUILD_HOUR, BUILD_DAY, BUILD_MONTH, BUILD_YEAR);
    needToResetTime = true;
  }
}

void loop() 
{
// -- doLoop should be called as frequently as possible.
  iotWebConf.doLoop();

// **
  using periodic = esp8266::polledTimeout::periodicMs;
  static periodic nextPing(1000);
  if (nextPing) {
    Wire.beginTransmission(I2C_COLORDUINO); // transmit to device #9
    Wire.write(message); // sends one byte 0-256 : 0-250 is (minutes since high tides divided by 3), 251*255 is tide coefficient indication
    Wire.endTransmission(); // stop transmitting

// oo
  Serial.print(rtc.getHours());
  Serial.print(":");
  Serial.print(rtc.getMinutes());
  Serial.print(":");
  Serial.print(rtc.getSeconds());
  Serial.print(" ");
  Serial.print(rtc.getDay());
  Serial.print(" ");
  Serial.print(rtc.getDate());
  Serial.print("/");
  Serial.print(rtc.getMonth());
  Serial.print("/");
  Serial.println(rtc.getYear());

  }
}


/**
 * Handle web requests to "/" path.
 */
void handleRoot()
{
  // -- Let IotWebConf test and handle captive portal requests.
  if (iotWebConf.handleCaptivePortal())
  {
    // -- Captive portal request were already served.
    return;
  }

 // -- Reverse Lookup for Port name
  int cle;
  for (int i=0; i < (sizeof(portValues) / STRING_LEN) ; i++) {
     if ( atoi((char*)portValues[i])== atoi(portParamValue) ) { 
      cle = i ;
      }
   }
   
  String s = "<!DOCTYPE html><html lang=\"en\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/>";
  s += "<title>Tide-o-meter Parameters</title></head><body>";
  s += "<p style=\"text-align:center\"><span style=\"font-family:Lucida Sans Unicode,Lucida Grande,sans-serif\"><strong>Tide shown for </strong></p>";
  s += "<p style=\"text-align:center\"><span style=\"font-family:Lucida Sans Unicode,Lucida Grande,sans-serif\"><span style=\"font-size:18px\"><strong><span style=\"color:#ffffff\"><span style=\"background-color:#2980b9\">&nbsp;";
  s += (char*)portNames[cle]; 
  s += "&nbsp; </span></span></strong></span></p></br></br></br>";
  s += "<p style=\"text-align:center\"><span style=\"font-family:Lucida Sans Unicode,Lucida Grande,sans-serif\">Go to <a href='config'>configure page</a> to change settings (wifi, tide location, date/time).";
  s += "</body></html>\n";

  server.send(200, "text/html", s);
}

void configSaved()
{
  Serial.println("Configuration was updated.");
  Serial.println(timeRTCValue);
  if (needToResetTime == true) {
    HOUR = String(timeRTCValue).substring(0,2).toInt();
    MIN = String(timeRTCValue).substring(3,5).toInt();
    DAY = String(dateRTCValue).substring(0,2).toInt();
    MONTH = String(dateRTCValue).substring(3,5).toInt();
    YEAR = String(dateRTCValue).substring(6,10).toInt();
    rtc.setTime(0, MIN, HOUR, DAY, MONTH, YEAR);
    needToResetTime == false;
  }
  
}

bool formValidator(iotwebconf::WebRequestWrapper* webRequestWrapper)
{
  Serial.println("Validating form.");
  bool valid = true;

// validate dd/mm/yyyy
  int l = webRequestWrapper->arg(dateRTCParam.getId()).length(); 
  DAY = webRequestWrapper->arg(dateRTCParam.getId()).substring(0,2).toInt(); 
  MONTH = webRequestWrapper->arg(dateRTCParam.getId()).substring(3,5).toInt(); 
  YEAR = webRequestWrapper->arg(dateRTCParam.getId()).substring(6,10).toInt(); 
 
  if (l != 10 || DAY < 0 || DAY > 31 || MONTH < 0 || MONTH > 12 || YEAR < 2022 )
  {
    dateRTCParam.errorMessage = "Please provide a date in dd/mm/yyyy format!";
    valid = false;
  }

// validate hh:mm  
  l = webRequestWrapper->arg(timeRTCParam.getId()).length(); 
  HOUR = webRequestWrapper->arg(timeRTCParam.getId()).substring(0,2).toInt(); 
  MIN = webRequestWrapper->arg(timeRTCParam.getId()).substring(3,5).toInt(); 

  if (l != 5 || HOUR < 0 || HOUR > 23 || MIN < 0 || MIN > 59 )
  {
    timeRTCParam.errorMessage = "Please provide a time in hh:mm format!";
    valid = false;
  }

  if (l>0) {
      needToResetTime = true;
    }
/*
  int l = webRequestWrapper->arg(stringParam.getId()).length();
  if (l < 3)
  {
    stringParam.errorMessage = "Please provide at least 3 characters for this test!";
    valid = false;
  }
*/
  return valid;
}
