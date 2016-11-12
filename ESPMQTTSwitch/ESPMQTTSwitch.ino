/**
 * Ein ESP8622 NodeMCU Develompent Board, welches via WIFI/MQTT empfangene Schaltsignale
 * für Elro-Transmitter (Funktsteckdosen) via 433 MHz Sender übermittelt.
 * http://frightanic.com/iot/comparison-of-esp8266-nodemcu-development-boards/
 * 
 * Achtung, Pins beachten. Pin D1 entspricht im Sketch Digital5, siehe:
 * http://cdn.frightanic.com/blog/wp-content/uploads/2015/09/esp8266-nodemcu-dev-kit-v2-pins.png
 * 
 * 1. 433 MHz Transmitter
 * - VCC und GND verbinden
 * - DATA an ESP D1 (=Digital5)
 *   
 */

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <private_wifi.h>
#include <PubSubClient.h>
#include "RemoteTransmitter.h"

#define MQTT_CLIENT_ID "ElroSwitch"
#define MQTT_TOPIC "elroswitch/set/"
#define MQTT_STATE_TOPIC "elroswitch/state/"
#define RADIO_TRANSMITTER_PIN 5

ElroTransmitter switchTransmitter(RADIO_TRANSMITTER_PIN);

WiFiClient espClient;
PubSubClient client(espClient);

char responseTopic[128];
int switchStates[160]; // 0..31 * A..E


void doSwitch(int systemCode, char channel, int onoffAsInt) {

  int switchNum=(channel-'A')*32+systemCode;

  if (onoffAsInt==0 || onoffAsInt==1) { // SET
    switchStates[switchNum]=onoffAsInt;
  } else { // GET
    onoffAsInt=switchStates[switchNum];
  }

  if (onoffAsInt!=0 && onoffAsInt!=1) { // UNKNOWN
    return;
  }

  bool onoff=(onoffAsInt==1);

  switchTransmitter.sendSignal(systemCode,channel,onoff);

  String _topic=String(MQTT_STATE_TOPIC);
  _topic.concat(systemCode);
  _topic.concat("/");
  _topic.concat(channel);
  _topic.toCharArray(responseTopic,128);

  client.publish(responseTopic,onoff?"1":"0");
  Serial.print("Published ");
  Serial.print(responseTopic);
  Serial.print(": ");
  Serial.println(onoff?"1":"0");
  
}


const long switchUpdateDelay = 30000;
unsigned long switchUpdateNextMillis = 0;

void updateSwitches() {
  if (switchUpdateNextMillis>millis()) return;
  if (switchUpdateNextMillis>0) {
    Serial.println("Update switch states");
    for (char channel='A'; channel<='E'; channel++) {
      for (int systemCode=0;systemCode<=31;systemCode++) {
        doSwitch(systemCode,channel,2); // 2=UNKNOWN - use switchStates
        wdt_reset(); // Watchdock resetten
      }
    }
  }
  switchUpdateNextMillis=millis()+switchUpdateDelay;
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Received ");
  Serial.print(topic);
  Serial.print(": ");

  if (length!=1) {
    Serial.print(" ERR payload size != 1 byte: ");
    Serial.println(length);
    return;
  }

  bool onoff=payload[0]=='1';
  Serial.println(onoff?"ON":"OFF");

  char *ptr;
  ptr = strtok(topic, MQTT_TOPIC);
  if (!ptr) {
    Serial.println("ERR unexpected topic");
    return;
  }
  
  char *end;
  long _systemCode = strtol(ptr, &end, 10);
  if (end == ptr) {
    Serial.println("ERR unexpected topic end after system code");
    return;
  }
  if (_systemCode<0 || _systemCode>31) {
    Serial.println("ERR system code must be between 0..31");
    return;
  }
  int systemCode=_systemCode;
  
  ptr = strtok(NULL, "/");
  if (!ptr || strlen(ptr)!=1) {
    Serial.println("ERR unexpected topic, expected /CHANNEL after system code");
  }
  char channel=ptr[0];
  if (channel<'A'||channel>'E') {
    Serial.println("ERR channel must be between A..E");
  }

  doSwitch(systemCode,channel,onoff?1:0);
}

void setup() {
  wdt_enable(WDTO_8S); // Watchdog muss alle 8 Sekunden resettet werden
  for (int i=0;i<160;i++) switchStates[i]=2; // 2 == UNKNOWN
  Serial.begin(115200);
  setup_wifi();
  client.setServer(MQTT_HOST, MQTT_PORT);
  client.setCallback(callback);
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID,WIFI_PASSWORD);
  wdt_reset(); // Watchdock resetten

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD)) {
      Serial.println("connected");
      if (client.subscribe(MQTT_TOPIC,1)) {
        Serial.print("Subscribed to ");
        Serial.println(MQTT_TOPIC);
      }
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void loop() {
  wdt_reset(); // Watchdock resetten
  
  updateSwitches();
  
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}

