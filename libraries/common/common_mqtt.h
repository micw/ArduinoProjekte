#include <PubSubClient.h>

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

char MQTT_CLIENT_ID[15];

void setup_mqtt() {
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  byte mac[6];
  WiFi.macAddress(mac);
  sprintf(MQTT_CLIENT_ID, "ESP8266_%02X%02X%02X", mac[3], mac[4], mac[5]);
}

void connect_mqtt(char* MQTT_TOPIC) {
  // Loop until we're reconnected
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD)) {
      Serial.println("connected");
      if (MQTT_TOPIC && mqttClient.subscribe(MQTT_TOPIC,1)) {
        Serial.print("Subscribed to ");
        Serial.println(MQTT_TOPIC);
      }
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
