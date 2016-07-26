#include <Adafruit_NeoPixel.h>

/******************************************************************************
 * @file    saeco_hack_example.ino
 * @author  Rémi Pincent - INRIA
 * @date    27/05/2016
 *
 * @brief Make your coffee from anywhere on a SEACO INTELIA coffee machine. 
 * In order to be functional, coffee machine must be hacked : 
 * https://github.com/OpHaCo/smart_coffee_machine
 * remark : data echanges over MQTT not optimized - transmitted strings
 *
 * Project : smart_coffee_machine - saeco intelia hack
 * Contact:  Rémi Pincent - remi.pincent@inria.fr
 *
 * Revision History:
 * https://github.com/OpHaCo/smart_coffee_machine
 * 
 * LICENSE :
 * project_name (c) by Rémi Pincent
 * project_name is licensed under a
 * Creative Commons Attribution-NonCommercial 3.0 Unported License.
 *
 * You should have received a copy of the license along with this
 * work.  If not, see <http://creativecommons.org/licenses/by-nc/3.0/>.
 *****************************************************************************/

/**************************************************************************
 * Include Files
 **************************************************************************/
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <saeco_intelia.h>
#include <saeco_status.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

/**************************************************************************
 * Macros
 **************************************************************************/
/** Connect a teensy daugther boad thru UART
 *  to get precise coffee machine statuses 
 */
#define ENABLE_STATUS

/** Status frame start bytes */
#define FRAME_START_BYTES (uint16_t) 0x00FF

/**************************************************************************
 * Manifest Constants
 **************************************************************************/
static const char* _ssid = "wifi_ssid";
static const char* _password = "wifi_key";
static const char* _mqttServer = "mqtt_server_address";

/**************************************************************************
 * Type Definitions
 **************************************************************************/

/**************************************************************************
 * Global Variables
 **************************************************************************/

/**************************************************************************
 * Local Functions Declarations
 **************************************************************************/
static void onMQTTMsgReceived(char* topic, byte* payload, unsigned int length);
static void onSmallCoffeeBtnPress(uint32_t dur);
static void onPowerBtnPress(uint32_t dur);
static void setupWifi(void);
static void setupOTA(void);
static void setupSaecoCoffeeMachine(void);
static void setupMQTT(void);
static void setupMQTTTopics(void);
static void setupMQTTSubs(void);
static void mqttSubscribe(const char* topic);
static void setupMQTTConnection(void);
static void getCoffeeMachineStatus(void);

/**************************************************************************
 * Static Variables
 **************************************************************************/
/** filled lated with mac address */
static String _coffeeMachineName = "saeco_intelia";
static String _coffeeMachineTopicPrefix = "/amiqual4home/machine_place/";

static String _coffeeMachineOnBtnPressTopic;
static String _coffeeMachineAliveTopic;
static String _coffeeMachineStatusTopic;

static String _coffeeMachinePowerTopic;
static String _coffeeMachineSmallCoffeeTopic;
static String _coffeeMachineCoffeeTopic;
static String _coffeeMachineTeaTopic;
static String _coffeeMachineCleanTopic;
static String _coffeeMachineBrewTopic;

static String _onCoffeeMachinePowerBtnTopic;
static String _onCoffeeMachineSmallCoffeeBtnTopic;
static String _onCoffeeMachineCoffeeBtnTopic;
static String _onCoffeeMachineTeaBtnTopic;
static String _onCoffeeMachineCleanBtnTopic;
static String _onCoffeeMachineBrewBtnTopic;

static WiFiClient _wifiClient;
static PubSubClient _mqttClient(_mqttServer, 1883, onMQTTMsgReceived, _wifiClient);

static ESaecoStatus     _machineStatus     = OUT_OF_ENUM_SAECO_STATUS;

/**************************************************************************
 * Global Functions Defintions
 **************************************************************************/
void setup() {
  
  Serial.begin(115200);
  delay(10);

  Serial.println("\nstarting saeco_hack_example"); 

  uint8_t* mac[6]; 
  String macAdd= WiFi.macAddress();                
  Serial.print("WIFI mac address : ");
  Serial.println(macAdd);

  #ifdef ENABLE_STATUS
  Serial.println("Coffee machine status enabled over Rx using Teensy daughterboard");
  #endif

  setupWifi();
  setupMQTT();
  setupOTA();
  setupSaecoCoffeeMachine();
}

void loop() {
  _mqttClient.loop();

  #ifdef ENABLE_STATUS
  getCoffeeMachineStatus();
  #endif
  
  saecoIntelia_update();
  
  if (!_mqttClient.connected()){
    connectMQTT();
  }
  ArduinoOTA.handle();
}
 /**************************************************************************
 * Local Functions Definitions
 **************************************************************************/
static void onMQTTMsgReceived(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message on topic ");
  Serial.print(topic);
  Serial.print(" - length = ");
  Serial.println(length);

  if(strcmp(topic, _coffeeMachinePowerTopic.c_str()) == 0)
  {
    saecoIntelia_power();
  }
  else if(strcmp(topic, _coffeeMachineSmallCoffeeTopic.c_str()) == 0)
  {
    saecoIntelia_smallCup();
  }
  else if(strcmp(topic, _coffeeMachineCoffeeTopic.c_str()) == 0)
  {
    saecoIntelia_bigCup();
  }
  else if(strcmp(topic, _coffeeMachineTeaTopic.c_str()) == 0)
  {
    saecoIntelia_teaCup();
  }
  else if(strcmp(topic, _coffeeMachineCleanTopic.c_str()) == 0)
  {
    saecoIntelia_clean();
  }
  else if(strcmp(topic, _coffeeMachineBrewTopic.c_str()) == 0)
  {
    saecoIntelia_brew();
  }
}

static void onBrewBtnPress(uint32_t dur)
{
  Serial.print("brew button pressed during "); 
  Serial.print(dur);
  Serial.println("ms");

  if (!_mqttClient.publish(_onCoffeeMachineBrewBtnTopic.c_str(), String(dur).c_str())) {
    Serial.print("Publish failed on topic ");
    Serial.println(_onCoffeeMachineBrewBtnTopic);
  }
}

static void onSmallCoffeeBtnPress(uint32_t dur)
{  
  Serial.print("small coffee button pressed during "); 
  Serial.print(dur);
  Serial.println("ms");

  if (!_mqttClient.publish(_onCoffeeMachineSmallCoffeeBtnTopic.c_str(), String(dur).c_str())) {
    Serial.print("Publish failed on topic ");
    Serial.println(_onCoffeeMachineSmallCoffeeBtnTopic.c_str());
  }
}

static void onCoffeeBtnPress(uint32_t dur)
{  
  Serial.print("coffee button pressed during "); 
  Serial.print(dur);
  Serial.println("ms");

  if (!_mqttClient.publish(_onCoffeeMachineCoffeeBtnTopic.c_str(), String(dur).c_str())) {
    Serial.print("Publish failed on topic ");
    Serial.println(_onCoffeeMachineCoffeeBtnTopic.c_str());
  }
}

static void onPowerBtnPress(uint32_t dur)
{  
  Serial.print("power button pressed during "); 
  Serial.print(dur);
  Serial.println("ms");

  if (!_mqttClient.publish(_onCoffeeMachinePowerBtnTopic.c_str(), String(dur).c_str())) {
    Serial.print("Publish failed on topic ");
    Serial.println(_onCoffeeMachinePowerBtnTopic.c_str());
  }
}

static void onTeaBtnPress(uint32_t dur)
{  
  Serial.print("tea button pressed during "); 
  Serial.print(dur);
  Serial.println("ms");

  if (!_mqttClient.publish(_onCoffeeMachineTeaBtnTopic.c_str(), String(dur).c_str())) {
    Serial.print("Publish failed on topic ");
    Serial.println(_onCoffeeMachineTeaBtnTopic.c_str());
  }
}

static void onCleanBtnPress(uint32_t dur)
{  
  Serial.print("clean button pressed during "); 
  Serial.print(dur);
  Serial.println("ms");

  if (!_mqttClient.publish(_onCoffeeMachineCleanBtnTopic.c_str(), String(dur).c_str())) {
    Serial.print("Publish failed on topic ");
    Serial.println(_onCoffeeMachineCleanBtnTopic.c_str());
  }
}

static void setupWifi(void)
{
  uint8_t mac[6];
  WiFi.macAddress(mac);
  
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(_ssid);
  
  WiFi.begin(_ssid, _password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    printf(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  
  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }  
}

static void setupOTA(void)
{
  //Set actions for OTA
  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("OTA initialized");
  
}

static void setupSaecoCoffeeMachine()
{
  TsCoffeeBtnPins coffeePins =
  {
    ._u8_smallCupBtnPin      = 16,
    ._u8_bigCupBtnPin        = 4,
    ._u8_teaCupBtnPin        = NOT_USED_COFFEE_PIN,
    ._u8_powerBtnPin         = 14,
    ._u8_coffeeBrewBtnPin    = 13,
    ._u8_hiddenBtnPin        = NOT_USED_COFFEE_PIN,
    ._u8_cleanBtnPin         = NOT_USED_COFFEE_PIN,
    ._u8_onSmallCupBtnPin    = 5,
    ._u8_onBigCupBtnPin      = 2,
    ._u8_onTeaCupBtnPin      = 10,
    ._u8_onPowerBtnPin       = 12,
    ._u8_onCoffeeBrewBtnPin  = 9,
    ._u8_onHiddenBtnPin      = NOT_USED_COFFEE_PIN,
    /** A0 cannot be configured as digital input */
    // ._u8_onCleanBtnPin       = A0
    ._u8_onCleanBtnPin       = NOT_USED_COFFEE_PIN
  };
  
  
  TsButtonPressCb coffeeBtnPressCb = {
    ._pf_onSmallCupBtnPress    = &onSmallCoffeeBtnPress,
    ._pf_onBigCupBtnPress      = &onCoffeeBtnPress,
    ._pf_onTeaCupBtnPress      = &onTeaBtnPress,
    ._pf_onPowerBtnPress       = &onPowerBtnPress,
    ._pf_onCoffeeBrewBtnPress  = &onBrewBtnPress,
    ._pf_onHiddenBtnPress      = NULL,
    ._pf_onCleanBtnPress       = &onCleanBtnPress,
  };
  saecoIntelia_init(&coffeePins, &coffeeBtnPressCb); 
  Serial.println("Saeco coffee machine initialized");
}

static void setupMQTT(void)
{
  setupMQTTTopics();
  connectMQTT();
}

static void setupMQTTTopics(void)
{
  _coffeeMachineTopicPrefix += _coffeeMachineName;

  _coffeeMachineStatusTopic= _coffeeMachineTopicPrefix + '/' + "status";
  _coffeeMachineOnBtnPressTopic= _coffeeMachineTopicPrefix + '/' + "on_button_press";
  _coffeeMachineAliveTopic= _coffeeMachineTopicPrefix + '/' + "alive";
  
  _coffeeMachinePowerTopic = _coffeeMachineTopicPrefix + '/' + "power";
  _coffeeMachineSmallCoffeeTopic = _coffeeMachineTopicPrefix + '/' + "smallCoffee";
  _coffeeMachineCoffeeTopic = _coffeeMachineTopicPrefix + '/' + "coffee";
  _coffeeMachineTeaTopic = _coffeeMachineTopicPrefix + '/' + "tea";
  _coffeeMachineCleanTopic = _coffeeMachineTopicPrefix + '/' + "clean";
  _coffeeMachineBrewTopic = _coffeeMachineTopicPrefix + '/' + "brew";
  
  _onCoffeeMachinePowerBtnTopic = _coffeeMachineOnBtnPressTopic + "/power"; 
  _onCoffeeMachineSmallCoffeeBtnTopic = _coffeeMachineOnBtnPressTopic + "/smallCoffee";
  _onCoffeeMachineCoffeeBtnTopic = _coffeeMachineOnBtnPressTopic + "/coffee";
  _onCoffeeMachineTeaBtnTopic = _coffeeMachineOnBtnPressTopic + "/tea";
  _onCoffeeMachineCleanBtnTopic = _coffeeMachineOnBtnPressTopic + "/clean";
  _onCoffeeMachineBrewBtnTopic = _coffeeMachineOnBtnPressTopic + "/brew";
}

static void setupMQTTSubs(void)
{
  mqttSubscribe(_coffeeMachinePowerTopic.c_str());
  mqttSubscribe(_coffeeMachineSmallCoffeeTopic.c_str());
  mqttSubscribe(_coffeeMachineCoffeeTopic.c_str());
  mqttSubscribe(_coffeeMachineTeaTopic.c_str());
  mqttSubscribe(_coffeeMachineCleanTopic.c_str());
  mqttSubscribe(_coffeeMachineBrewTopic.c_str());
}

static void mqttSubscribe(const char* topic)
{
  if(_mqttClient.subscribe (topic))
  {
    Serial.print("Subscribe ok to ");
    Serial.println(topic);
  }
  else {
    Serial.print("Subscribe ko to ");
    Serial.println(topic);
  }
  /** if mqtt client loop not called when several mqtt api calls are done.
   * At some point, mqtt client api calls fail
   * */
  _mqttClient.loop();
}

static void connectMQTT(void)
{
  Serial.print("Connecting to ");
  Serial.print(_mqttServer);
  Serial.print(" as ");
  Serial.println(_coffeeMachineName);

  while (true)
  {
    if (_mqttClient.connect(_coffeeMachineName.c_str())) {
      setupMQTTSubs();
      Serial.println("Connected to MQTT broker");
      Serial.print("send alive message to topic ");
      Serial.println(_coffeeMachineAliveTopic.c_str());
      if (!_mqttClient.publish(_coffeeMachineAliveTopic.c_str(), "alive")) {
        Serial.println("Publish failed");
      }
      return;
    }
    else
    {
      Serial.println("connection to MQTT failed\n");
    }
    delay(2000);
  }
}

static void getCoffeeMachineStatus(void)
{
  ESaecoStatus newMachineStatus;
  
  if( Serial.available() == 0 )
    return;

  if(Serial.read() != ((FRAME_START_BYTES >> 8) & 0xFF))
    return;

  while(Serial.available() == 0);
 
  if(Serial.read() != FRAME_START_BYTES  & 0xFF)
    return;
  
  while(Serial.available() == 0);
  newMachineStatus = (ESaecoStatus) Serial.read();
 
  if(newMachineStatus != _machineStatus)
  {
    _machineStatus = newMachineStatus;
;
    Serial.println("frame received");
    Serial.print("status = ");
    Serial.print(_machineStatus);

    if (!_mqttClient.publish(_coffeeMachineStatusTopic.c_str(), String(_machineStatus).c_str())) {
      Serial.print("Publish failed on topic ");
      Serial.println(_coffeeMachineStatusTopic.c_str());
    }
  }
}






