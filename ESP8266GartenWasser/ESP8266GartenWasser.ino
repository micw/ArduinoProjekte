/****************************************
 * 
 * ESP8266 als Steuerung unserer Garten-Bewässerung
 * 
 * Aufbau:
 * ESP8266 basiertes Dev-Board (NodeMCU)
 * 
 * Solid-State-Relais CX240D5 an Pins D1 und D2
 * - diese schalten je ein Magnetventil Hunter [TBD]
 * 
 * Es wird eine Verbindung zum MQTT-Broker aufgebaut. Alle Topica haben water/MAC/ als Präfix, wobei MAC die letzten 6 Zeichen der MAC-Adresse sind
 * Beispiel (MAC: CAFE01)
 * water/CAFEBA/alive - ist 1, wenn der ESP verbunden ist. Wird mittels Last will and testamtent auf 0 gesetzt, wenn die Verindung abbricht
 * water/CAFEBA/set1 - Schalten von Wasser 1 (0 oder 1)
 * water/CAFEBA/set2 - Schalten von Wasser 2 (0 oder 1)
 * water/CAFEBA/get1 - Status von Wasser 1 (0 oder 1)
 * water/CAFEBA/get2 - Status von Wasser 2 (0 oder 1)
 * 
 * Status-LED zeigt MQTT-Verbindung an
 * Optionale 2 Status-LEDs zeigen Wasser an/aus an
 * 
 */

#include <esp_pins.h>
#include <common_wifi.h>
#include <common_mqtt.h>

// Settings Variante 1
//#define WATER1 D1
//#define WATER2 D2
//#define STATUS D6

// Settings Variante 2 (RGB LED, 6=Grün, 7=Blau, 8=Rot)
#define WATER1 4
#define WATER2 5
#define STATUS D8
#define STATUS_W1 D6
#define STATUS_W2 D7


char topicWaterAlive[19];
char topicWaterSet1[18];
char topicWaterSet2[18];
char topicWaterGet1[18];
char topicWaterGet2[18];

const char* MQTT_FALSE = "0";
const char* MQTT_TRUE = "1";

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  if (length<1) return;
  boolean state=payload[0]=='1';
  if (strcmp(topicWaterSet1,topic)==0) {
      mqttClient.publish(topicWaterGet1,state?MQTT_TRUE:MQTT_FALSE,1);
      digitalWrite(WATER1, state?HIGH:LOW);
      #if defined STATUS_W1
        digitalWrite(STATUS_W1, state?HIGH:LOW);
      #endif
  }
  if (strcmp(topicWaterSet2,topic)==0) {
      mqttClient.publish(topicWaterGet2,state?MQTT_TRUE:MQTT_FALSE,1);
      digitalWrite(WATER2, state?HIGH:LOW);
      #if defined STATUS_W2
        digitalWrite(STATUS_W2, state?HIGH:LOW);
      #endif
  }
}

void setup() {
  Serial.begin(115200);
  wdt_enable(WDTO_8S); // Watchdog muss alle 8 Sekunden resettet werden

  // Pins auf Output setzten und Wasser ausschalten
  pinMode(WATER1, OUTPUT);
  pinMode(WATER2, OUTPUT);
  pinMode(STATUS, OUTPUT);
  digitalWrite(WATER1, LOW);
  digitalWrite(WATER2, LOW);
  digitalWrite(STATUS, LOW);
  #if defined STATUS_W1
    pinMode(STATUS_W1, OUTPUT);
    digitalWrite(STATUS_W1, LOW);
  #endif
  #if defined STATUS_W2
    pinMode(STATUS_W2, OUTPUT);
    digitalWrite(STATUS_W2, LOW);
  #endif

  byte mac[6];
  WiFi.macAddress(mac);

  sprintf(topicWaterAlive, "water/%02X%02X%02X/alive", mac[3], mac[4], mac[5]);
  sprintf(topicWaterSet1, "water/%02X%02X%02X/set1", mac[3], mac[4], mac[5]);
  sprintf(topicWaterGet1, "water/%02X%02X%02X/get1", mac[3], mac[4], mac[5]);
  sprintf(topicWaterSet2, "water/%02X%02X%02X/set2", mac[3], mac[4], mac[5]);
  sprintf(topicWaterGet2, "water/%02X%02X%02X/get2", mac[3], mac[4], mac[5]);
  
  wdt_reset(); // Watchdog resetten
  setup_wifi();
  wdt_reset(); // Watchdog resetten
  setup_mqtt();
  wdt_reset(); // Watchdog resetten
  mqttClient.setCallback(mqttCallback);
}

void loop() {
  wdt_reset(); // Watchdog resetten
  while (!mqttClient.connected()) {
    digitalWrite(WATER1, LOW);
    digitalWrite(WATER2, LOW);
    digitalWrite(STATUS, LOW);
    #if defined STATUS_W1
      digitalWrite(STATUS_W1, LOW);
    #endif
    #if defined STATUS_W2
      digitalWrite(STATUS_W2, LOW);
    #endif
  
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD, topicWaterAlive, 1, 1, MQTT_FALSE)) {
      Serial.println("connected");
      mqttClient.publish(topicWaterAlive,MQTT_TRUE,1);
      mqttClient.publish(topicWaterGet1,MQTT_FALSE,1);
      mqttClient.publish(topicWaterGet2,MQTT_FALSE,1);
      if (mqttClient.subscribe(topicWaterSet1,1)) {
        Serial.print("Subscribed to ");
        Serial.println(topicWaterSet1);
      }
      if (mqttClient.subscribe(topicWaterSet2,1)) {
        Serial.print("Subscribed to ");
        Serial.println(topicWaterSet2);
      }
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
      wdt_reset(); // Watchdog resetten
    }
    return;
  }
  digitalWrite(STATUS, HIGH);
  mqttClient.loop();
}
