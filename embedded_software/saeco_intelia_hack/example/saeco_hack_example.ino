#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#include "saeco_intelia.h"

const char* ssid = "ssid";
const char* password = "pass";

ESP8266WebServer server(80);

const int led = 13;

void handleRoot() {
  digitalWrite(led, 1);
  server.send(200, "text/plain", "hello from esp8266!");
  digitalWrite(led, 0);
}

void handleCoffee() {
 server.send(200, "text/plain", "hello from esp8266!");
 saecoIntelia_smallCup();
}

void handlePower() {
 server.send(200, "text/plain", "hello from esp8266!");
 saecoIntelia_power();
}

void handleNotFound(){
  digitalWrite(led, 1);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  digitalWrite(led, 0);
}

void onSmallCoffeeBtnPress(uint32_t dur)
{
  Serial.print("small coffeed pressed during ");
  Serial.println(dur);
}

void setup(void){
Serial.begin(115200);
  
TsCoffeeBtnPins coffeePins =
{
  ._u8_smallCupBtnPin = 16,
  ._u8_bigCupBtnPin   = NOT_USED_COFFEE_PIN,
  ._u8_teaCupBtnPin= NOT_USED_COFFEE_PIN,
  ._u8_powerBtnPin = 5,
  ._u8_coffeeBrewBtnPin= NOT_USED_COFFEE_PIN,
  ._u8_hiddenBtnPin= NOT_USED_COFFEE_PIN,
  ._u8_onSmallCupBtnPin= 4,
  ._u8_onBigCupBtnPin= NOT_USED_COFFEE_PIN,
  ._u8_onTeaCupBtnPin= NOT_USED_COFFEE_PIN,
  ._u8_onPowerBtnPin= NOT_USED_COFFEE_PIN,
  ._u8_onCoffeeBrewBtnPin= NOT_USED_COFFEE_PIN,
  ._u8_onHiddenBtnPin= NOT_USED_COFFEE_PIN,
};


TsButtonPressCb coffeeBtnPressCb = {
  ._pf_onSmallCupBtnPress = &onSmallCoffeeBtnPress,
  ._pf_onBigCupBtnPress = NULL,
  ._pf_onTeaCupBtnPress= NULL,
  ._pf_onPowerBtnPress= NULL,
  ._pf_onCoffeeBrewBtnPress= NULL,
  ._pf_onHiddenBtnPress = NULL,
};

saecoIntelia_init(&coffeePins, &coffeeBtnPressCb);

  pinMode(led, OUTPUT);
  digitalWrite(led, 0);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);

  server.on("/power", handlePower);
  server.on("/coffee", handleCoffee);

  server.on("/inline", [](){
    server.send(200, "text/plain", "this works as well");
  });

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
}

void loop(void){
  server.handleClient();
  saecoIntelia_update();
}
