#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <PubSubClient.h>

#include "saeco_intelia.h"

const char* ssid =     "ssid";
const char* password = "pwd";

const char* mqtt_server =           "ip";
const int   mqtt_server_port =      1883;

const char* mqtt_topic_subscribed = "Saeco incoming";
const char* mqtt_topic_publish =    "Saeco outgoing";

const char* mqtt_msg_handlePower =          "Saeco turn on";
const char* mqtt_msg_handleSmallCoffeeCup = "Saeco make small coffee cup";
const char* mqtt_msg_handleBigCoffeeCup =   "Saeco make big coffee cup";


ESP8266WebServer server(80);
WiFiClient espClient;
PubSubClient mqtt_client(espClient); //mqtt client

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

void setup_saecoInit()
{
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
}

void setup_connectWifi()
{
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
}

void setup_server(){
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

void callback(char* topic, byte* payload, unsigned int length)
{
  Serial.print("Mosquitto message arrived [");
  Serial.print(topic);
  Serial.print("]");
  char* message;
  for (int i =0;i<length;i++){
    char receivedChar = (char)payload[i];
    message = message + (char)payload[i];
    Serial.print(receivedChar);  
  }
  Serial.println();

  if (message == mqtt_msg_handlePower) {
    Serial.println("Received Mosquitto message to handle power");
    saecoIntelia_power();
    mqtt_client.publish(mqtt_topic_publish,"Coffe machine turned on");
  } else if (message == mqtt_msg_handleSmallCoffeeCup) {
    Serial.println("Received Mosquitto message to make a small coffee cup");
    saecoIntelia_smallCup();
    mqtt_client.publish(mqtt_topic_publish,"Made a small cup of coffee");
  } 
  /* big cup not yet implemented
   * else if (message == mqtt_msg_handleBigCoffeeCup){
    Serial.println("Received Mosquitto message to make a big coffee cup");
    saecoIntelia_bigCup();
    mqtt_client.publish(mqtt_topic_publish,"Made a large cup of coffee");
  }*/
}

void reconnectMosquitto()
{
  while (!mqtt_client.connected()) {
    //Connecting to Mosquitto server
    /* if the mosquitto server uses user and password, use the following definition
    //mqtt_client.connect("Saeco machine à café", const char* user, const char* pass)*/
    if (mqtt_client.connect("Saeco machine à café")){
      Serial.println("Connected to Mosquitto server");
      mqtt_client.subscribe(mqtt_topic_subscribed);  
    } else {
      Serial.print("Failed connection, rc=");
      Serial.print(mqtt_client.state());
      Serial.println("Trying again in 5 seconds");
      delay(5000);  
    }
  }
}

void setup(void){
  Serial.begin(115200);
  setup_saecoInit();
  setup_connectWifi();
  setup_server();

  //setup mosquitto client
  mqtt_client.setServer(mqtt_server,mqtt_server_port);
  mqtt_client.setCallback(callback);
}

void loop(void){
  server.handleClient();
  saecoIntelia_update();
  if (!mqtt_client.connected()){
    reconnectMosquitto();
  }
  mqtt_client.loop();
}
