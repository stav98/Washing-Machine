#include "WiFi.h"
#include "ESPAsyncWebServer.h"
//#include "SPIFFS.h"

//--------------- Λειτουργία client --------------
const char* client_ssid = "SV6GMP";
const char* client_password = "!stav_prev_agl35!";

//--------------- Λειτουργία AP ------------------
//const char* ap_ssid     = "ESP32-Access-Point";
//const char* ap_password = "123456789";

AsyncWebServer server(80);

void scanNetworks() {
 
  int numberOfNetworks = WiFi.scanNetworks();
 
  Serial.print("Number of networks found: ");
  Serial.println(numberOfNetworks);
 
  for (int i = 0; i < numberOfNetworks; i++) {
 
    Serial.print("Network name: ");
    Serial.println(WiFi.SSID(i));
 
    Serial.print("Signal strength: ");
    Serial.println(WiFi.RSSI(i));
 
    Serial.print("MAC address: ");
    Serial.println(WiFi.BSSIDstr(i));
 
    Serial.print("Encryption type: ");
    //String encryptionTypeDescription = translateEncryptionType(WiFi.encryptionType(i));
    //Serial.println(encryptionTypeDescription);
    Serial.println(WiFi.encryptionType(i));
    Serial.println("-----------------------");
 
  }
}

String out1="aaa";

void notFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "<H1>Not found</H1>");
}

// Replaces placeholder with LED state value
/*String processor(const String& var){
  Serial.println(var);
  if(var == "STATE"){
    if(digitalRead(ledPin)){
      ledState = "ON";
    }
    else{
      ledState = "OFF";
    }
    Serial.print(ledState);
    return ledState;
  }
  return String();
}*/

String processor(const String& var){
  //Serial.println(var);
  if(var == "STATE"){
    out1 = String(volts);
    return out1;
  }
  return String();
}

void initWebServer()
     {
      //--- Στατική IP μόνο για clients ------------------------------------
      IPAddress local_IP(192, 168, 42, 121);
      IPAddress gateway(192, 168, 42, 5);
      IPAddress subnet(255, 255, 255, 0);
      IPAddress primaryDNS(192, 168, 42, 5);   //optional
      IPAddress secondaryDNS(8, 8, 8, 8); //optional
      if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) 
         {
          Serial.println("STA Failed to configure");
         }
      //--------------------------------------------------------------------
      
      //--- Λειτουργία client ----------------------------------------------
      WiFi.begin(client_ssid, client_password);
      while (WiFi.status() != WL_CONNECTED) 
            {
             delay(1000);
             Serial.println("Connecting to WiFi..");
            }
      Serial.println(WiFi.localIP());

/*
      //--- Λειτουργία AP. Διεύθυνση 192.168.4.1 --------------------------- 
      WiFi.mode(WIFI_AP);
      WiFi.softAP(ap_ssid, ap_password, 1, 0, 2); //int channel, int ssid_hidden [0,1], int max_connection [1-4]
      //IPAddress IP = WiFi.softAPIP();
      //Serial.print("AP IP address: ");
      //Serial.println(IP);*/

      //Ζητήθηκε η αρχική σελίδα / 
      server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
               {
                request->send(SPIFFS, "/index.html", String(), false, processor);
                //request->send(SPIFFS, "/index.html", "text/html");
                //Serial.print("web running on core ");
                //Serial.println(xPortGetCoreID());
               });
      server.onNotFound(notFound);

      server.on("/volts", HTTP_GET, [](AsyncWebServerRequest *request)
               {   
                String a = "<val id=\"vol\">" + String(volts) + "</val>" + "<val id=\"amp\">" + String(amperes) + "</val>" + "<val id=\"psutemp\">" + String(psu_temp) + "</val>";
                request->send(200, "text/plain", a);
               });

      // Route to load style.css file
      server.on("/jquery.min.js", HTTP_GET, [](AsyncWebServerRequest *request)
               {
                request->send(SPIFFS, "/jquery.min.js", "text/javascript");
               });
      // Route to load style.css file
      /*server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request)
               {
                request->send(SPIFFS, "/style.css", "text/css");
               });

      // Route to set GPIO to HIGH
      server.on("/on", HTTP_GET, [](AsyncWebServerRequest *request)
               {
                digitalWrite(ledPin, HIGH);    
                request->send(SPIFFS, "/index.html", String(), false, processor);
               });
  
      // Route to set GPIO to LOW
      server.on("/off", HTTP_GET, [](AsyncWebServerRequest *request)
               {
                digitalWrite(ledPin, LOW);    
                request->send(SPIFFS, "/index.html", String(), false, processor);
               });

      // Start server*/
      server.begin();
     }
