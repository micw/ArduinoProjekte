/****************************************
 * 
 * Kombination eines ESP8266 mit einem DHT22 Feuchte- und
 * Temperatursensor, sendet per MQTT-Topic
 * 
 * ESP Pinout: https://mcuoneclipse.files.wordpress.com/2014/11/esp8266-pins.png
 * 
 * DHT22:
 * - GND an GND
 * - 3.3V an 3.3V
 * - Data an D2
 * 
 */

#include <esp_pins.h>
#include <common_wifi.h>
#include <common_mqtt.h>
#include "DHT.h"

#define DHTPIN D2
#define DHTTYPE DHT22

char topicTemp[19];
char topicTempIdx[22];
char topicHumid[20];

// Initialize DHT sensor.
DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);
  wdt_enable(WDTO_8S); // Watchdog muss alle 8 Sekunden resettet werden

  dht.begin();
  wdt_reset(); // Watchdog resetten
  setup_wifi();
  wdt_reset(); // Watchdog resetten
  setup_mqtt();
  wdt_reset(); // Watchdog resetten

  byte mac[6];
  WiFi.macAddress(mac);
  sprintf(topicTemp, "sensor/%02X%02X%02X/temp", mac[3], mac[4], mac[5]);
  sprintf(topicTempIdx, "sensor/%02X%02X%02X/tempIdx", mac[3], mac[4], mac[5]);
  sprintf(topicHumid, "sensor/%02X%02X%02X/humid", mac[3], mac[4], mac[5]);
}

char payload[10];

void loop() {
  wdt_reset(); // Watchdog resetten
  connect_mqtt(0);

  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(t, h, false);

  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.print(" %\t");
  Serial.print("Temperature: ");
  Serial.print(t);
  Serial.print(" *C \t");
  Serial.print("Heat index: ");
  Serial.print(hic);
  Serial.print(" *C ");
  Serial.println();

  
  sprintf(payload,"%d.%02d",(int)t,(int)(t*10)%10);
  mqttClient.publish(topicTemp,payload);

  sprintf(payload,"%d.%02d",(int)h,(int)(h*10)%10);
  mqttClient.publish(topicHumid,payload);

  sprintf(payload,"%d.%02d",(int)hic,(int)(hic*10)%10);
  mqttClient.publish(topicTempIdx,payload);

  delay(5000);
}

