#include <ArduinoJson.h>
#include <WiFiManager.h>
#include <ESP8266SSDP.h>
#include <ESP8266WebServer.h>
#include <IRremoteESP8266.h>
#include "ESP8266FactoryReset.h"
#include "FeedbackLED.h"

#define LED_PIN           12 //BUILTIN_LED
#define FACTORY_RESET_PIN 14
#define IR_EMITTER_PIN    4

ESP8266FactoryReset reset(FACTORY_RESET_PIN);
FeedbackLED  feedback(LED_PIN);
ESP8266WebServer HTTP(80);
IRsend irsend(IR_EMITTER_PIN);

#define MAX_COMMAND_SIZE 256;

void send() {
    Serial.println("Send!!!!!");
    if (HTTP.hasArg("v")) {
      String v = HTTP.arg("v");
      int size = v.length()+1;
      char data[size];
      v.toCharArray(data, size);
      char* str = data;
      size_t count = 0;
      while(*str) if (*str++ == ',') ++count;
      ++count;
      unsigned int cmds[count];
      size_t cmd_cnt = 0;
      char* token = strtok(data, ",");
      while (token != NULL) {
        cmds[cmd_cnt] = atoi(token);;
        cmd_cnt++;
        token = strtok(NULL, ",");
      }
      Serial.print("sending....");
      irsend.sendGC(cmds, count);
      Serial.println("done");
    }
    HTTP.send(200, "text/plain", "Sent!\n");
}

void setup() {
  Serial.begin(115200);
  Serial.println("");
  Serial.println("Startup!");

  reset.setArmedCB([](){feedback.blink(FEEDBACK_LED_FAST);}); // blink fast when reset button is pressed
  reset.setDisarmedCB([](){feedback.off();});                 // if its released before ready time turn the LED off
  reset.setReadyCB([](){feedback.on();});                     // once its ready, put the LED on solid, when button is released, reset!
  reset.setup(); // handle reset held at startup
  
  // setup wifi, blink let slow while connecting and fast if portal activated.
  feedback.blink(FEEDBACK_LED_SLOW);
  WiFiManager wifi;
  wifi.setAPCallback([](WiFiManager *){feedback.blink(FEEDBACK_LED_FAST);});
  String ssid = "STFramework" + String(ESP.getChipId());
  wifi.autoConnect(ssid.c_str(), NULL);
  feedback.off();

  Serial.print("Starting HTTP...");
  HTTP.on("/index.html", HTTP_GET, [](){
    HTTP.send(200, "text/plain", "I'm a Thing!");
  });
  HTTP.on("/description.xml", HTTP_GET, [](){
    SSDP.schema(HTTP.client());
  });
  HTTP.on("/send",  HTTP_GET, &send);

  HTTP.begin();
  Serial.println("done");

  Serial.print("Starting SSDP...");
  SSDP.setSchemaURL("description.xml");
  SSDP.setHTTPPort(80);
  SSDP.setName("Thing");
  SSDP.setSerialNumber(ESP.getChipId());
  SSDP.setURL("index.html");
  SSDP.setModelName("Thing");
  SSDP.setModelNumber("0.1");
  SSDP.setDeviceType("urn:Liebman:device:IRRemote:1");
  SSDP.setModelURL("http://www.zod.com");
  SSDP.setManufacturer("Christopher B. Liebman");
  SSDP.setManufacturerURL("http://www.zod.com");
  SSDP.begin();
  Serial.println("done");

  pinMode(IR_EMITTER_PIN, OUTPUT);
  irsend.begin();
}

void loop() {
  HTTP.handleClient();
  reset.loop(); // handle reset from main loop
  delay(1);
}

 
