
/*
Branchements 
 ESP8266 <-------> COLORDUINO
      D1 <-------> SCL  
      D2 <-------> SDA
      GND<-------> GND
      VU <-------> VDD
               --> RXD (not used)
               --> TXD (not used)
               --> GND (indifferent avec autre GND)
      RST<-------> DTR                
*/


#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include "WiFiConnect.h"
#include "WiFiConnectParam.h"
#include <WiFiClientSecure.h>
#include <ezTime.h>
#include <time.h>
#include <Wire.h> 
#include <EEPROM.h>


WiFiConnect wc;
//String buf = loadFromEEPROM();
//WiFiConnectParam wp = WiFiConnectParam("lieu","Code du marégraphe de référence (data.shom.fr)?",94,"Code du marégraphe de référence (data.shom.fr)?") ;


String ville = "94"; //94: boucau bayonne 

const char* host = "services.data.shom.fr";
const int httpsPort = 443;

DynamicJsonDocument doc(16384);
int x = 255;
int refreshTideTime=0;
tm heurepleinemer;

void configModeCallback(WiFiConnect *mWiFiConnect) {
  Serial.println("Entering Access Point");
}

void startWiFi(boolean showParams = true) {


 
  wc.setDebug(true);

  wc.setAPName("Marégramme - Biarritz");

 // wc.addParameter(&wp);
  
  wc.setAPCallback(configModeCallback);

  // wc.resetSettings(); //helper to remove the stored wifi connection, comment out after first upload and re upload

    /*
       AP_NONE = Continue executing code
       AP_LOOP = Trap in a continuous loop - Device is useless
       AP_RESET = Restart the chip
       AP_WAIT  = Trap in a continuous loop with captive portal until we have a working WiFi connection
    */
    if (!wc.autoConnect()) { // try to connect to wifi
      /* We could also use button etc. to trigger the portal on demand within main loop */
      wc.startConfigurationPortal(AP_WAIT); //if not connected show the configuration portal
   //   saveToEEPROM(String(wp.getValue()));
      ESP.restart();
    }
}

void setup() {
  Serial.begin(115200);

  startWiFi();

    
  waitForSync();

Serial.println(now());
  
  Wire.begin();

  transmit(x);
  transmit(refreshTide());
}

void loop() {
   events();

//Serial.println(lieu);
time_t duree = now() - makeTime(heurepleinemer.tm_hour,heurepleinemer.tm_min,heurepleinemer.tm_sec,heurepleinemer.tm_mday,heurepleinemer.tm_mon+1,heurepleinemer.tm_year+1900);

    delay(0);

//  Serial.println(wp.getValue());

  
      transmit(int(duree/60/4));
//      Serial.println("tranmission simple");
//      Serial.println(int(duree/60));
//      Serial.println(int(duree/60/4)%100);
      if (int(duree/60/4)%100==0) {
        transmit(refreshTide());
//        Serial.println("transmission avec rafraichissement");
//        Serial.println(int(duree/60));
      }
    
    delay(1000);  

    // Wifi Dies? Start Portal Again
    if (WiFi.status() != WL_CONNECTED) {
      if (!wc.autoConnect()) wc.startConfigurationPortal(AP_WAIT);
    }
  
   
}

void transmit(int x) {
  Wire.beginTransmission(0x09);  // transmit to device #9 / transmission sur l arduino secondaire a l adresse 0x09 (=9 en decimale)
  Wire.write(x);                 // sends x / envoi de la valeur de x
  Wire.endTransmission();        // stop transmitting / arret de la transmission
  Serial.println(x);
}


int refreshTide() {
   
  WiFiClientSecure client;


  Serial.print("connecting to ");
  Serial.println(host);
  
  client.setInsecure();

  if (!client.connect(host, httpsPort)) {
    Serial.println("connection failed");
    x = 255;
    return x;
  }

  time_t debut = now()-3600*13;

  String url = "/maregraphie/observation/json/"+ville+"?sources=1&dtStart=" + dateTime(debut,RFC3339) + "&dtEnd=" + dateTime(RFC3339) + "&interval=10"; // ou intervalle 5
 // Serial.print("requesting URL: ");
 // Serial.println(url);

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: BuildFailureDetectorESP8266\r\n" +
               "Connection: close\r\n\r\n");

 // Serial.println("request sent");


  // Check HTTP status
char status[32] = {0};
  client.readBytesUntil('\r', status, sizeof(status));
  if (strcmp(status, "HTTP/1.1 200 OK") != 0) {
    Serial.print(F("Unexpected response: "));
    Serial.println(status);
    x = 255 ;
    client.stop();
    return x;
  }
  
    // Skip HTTP headers
  char endOfHeaders[] = "\r\n\r\n";
  if (!client.find(endOfHeaders)) {
    Serial.println(F("Invalid response"));
    x = 255 ;
    client.stop();
    return x;
  }

  // Allocate the JSON document
  // Use arduinojson.org/v6/assistant to compute the capacity.

 

 
  // Parse JSON object
  DeserializationError error = deserializeJson(doc, client);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    x = 255 ;
    client.stop();
    return x;
  }
 
yield();
  
int i=0;

strptime(doc["data"][0]["timestamp"],"%Y-%m-%d %H:%M:%S",&heurepleinemer);
double hauteur=doc["data"][0]["value"].as<double>();
String verif = doc["data"][0]["timestamp"].as<String>();
while (i < doc["data"].size()+1) {
if (doc["data"][i]["value"].as<double>()>hauteur){
strptime(doc["data"][i]["timestamp"],"%Y/%m/%d %H:%M:%S",&heurepleinemer);
verif = doc["data"][i]["timestamp"].as<String>();

hauteur=doc["data"][i]["value"].as<double>();
}
  i++;
}

time_t duree = now() - makeTime(heurepleinemer.tm_hour,heurepleinemer.tm_min,heurepleinemer.tm_sec,heurepleinemer.tm_mday,heurepleinemer.tm_mon+1,heurepleinemer.tm_year+1900);

x = int(duree/60/4);

doc.clear();
doc.garbageCollect();

return x;
}

/*
String loadFromEEPROM() {
char mychar0,mychar1,mychar2,mychar3;
  EEPROM.begin(512);
  EEPROM.get(0,  mychar0);
  EEPROM.get(1,  mychar1);
  EEPROM.get(2,  mychar2);
  EEPROM.get(3,  mychar3);
  EEPROM.end();

  Serial.println("Code du marégraphe de référence en mémoire:");
  Serial.println(String(mychar0)+String(mychar1)+String(mychar2)+String(mychar3));
  String resultat = String(mychar0)+String(mychar1)+String(mychar2)+String(mychar3);
  return resultat;
}


void saveToEEPROM(String lieumaregraphe) {
  EEPROM.begin(512);
  char mychar0 = lieumaregraphe.charAt(0);
  EEPROM.put(0, mychar0);
  char mychar1 = lieumaregraphe.charAt(1);
  EEPROM.put(1, mychar1);
  char mychar2 = lieumaregraphe.charAt(2);
  EEPROM.put(2, mychar2);
  char mychar3 = lieumaregraphe.charAt(3);
  EEPROM.put(3, mychar3);
  EEPROM.commit();
  EEPROM.end();
}

*/
