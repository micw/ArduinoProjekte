/**
 * Ein ESP8622 NodeMCU Develompent Board, welches Sensorwerte als UDP-Broadcast via Wifi verschickt.
 * http://frightanic.com/iot/comparison-of-esp8266-nodemcu-development-boards/
 * 
 * Achtung, Pins beachten. Pin D1 entspricht im Sketch Digital5, siehe:
 * http://cdn.frightanic.com/blog/wp-content/uploads/2015/09/esp8266-nodemcu-dev-kit-v2-pins.png
 * 
 * DS18x20 http://datasheets.maximintegrated.com/en/ds/DS18S20.pdf
 * - VCC und GND verbinden
 * - DATA an ESP D2 (=Digital4)
 * - Data <-> 4,7 kOhm <-> +5V  (2kOhm tut's auch)
 *   
 */

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <private_wifi.h>
#include <OneWireListener.h>

#define UDP_SEND_PORT 5000
#define PIN_SENSOR1 4
#define SENSOR_CHECK_INTERVAL 30000

WiFiUDP Udp;
IPAddress BROADCAST_IP(255,255,255,255);

OneWireListener onewire1=OneWireListener(PIN_SENSOR1);

byte sensorMessageNum=0;
void receiveDS18x20Temperature(int sensorNumber, unsigned int sensorId, float temperature) {
    // Note: interrupts are disabled. You can re-enable them if needed.

    int tempEncoded;
    if (temperature>0) tempEncoded =(int) (temperature*100+0.5);
    else tempEncoded =(int) (temperature*100-0.5);

    byte * temperatureBytes = (byte *) &tempEncoded;

    byte msg[6];
    msg[0]='a';
    msg[ 1]=(byte) (sensorId & 0xff);
    msg[ 2]=(byte) (sensorId>>8 & 0xff);
    msg[ 3]=temperatureBytes[0];
    msg[ 4]=temperatureBytes[1];
    msg[ 5]=sensorMessageNum++;

    if (Udp) {
      Udp.beginPacket(BROADCAST_IP, UDP_SEND_PORT);
      Udp.write(msg,6);
      Udp.endPacket();
      delay(1000);
    }

    Serial.print(sensorId,HEX);
    Serial.print(" = ");
    Serial.print(temperature);
    Serial.print(" = ");
    Serial.print(tempEncoded);
    Serial.println();
}

void setup() {
  wdt_enable(WDTO_8S); // Watchdog muss alle 8 Sekunden resettet werden
  Serial.begin(115200);
  Serial.println("Initializing");
  WiFi.begin(WIFI_SSID,WIFI_PASSWORD);
  wdt_reset(); // Watchdock resetten
  int sensorCount=0;
  onewire1.setDS18x20TemperatureCallBack(receiveDS18x20Temperature);
  sensorCount=onewire1.initialize();
  wdt_reset(); // Watchdock resetten
  Serial.println("Starting");
}

long nextRead=0;
void readSensorValues() {
  unsigned long currentMillis = millis();
  if (currentMillis>nextRead) {
    nextRead=currentMillis+SENSOR_CHECK_INTERVAL;
    Serial.println("Reading sensors");
    onewire1.readValues();
    Serial.println("Done");
    
  }
}

void loop() {
  wdt_reset(); // Watchdock resetten
  if(WiFi.status() == WL_CONNECTED) {
    readSensorValues();
  } else {
    Serial.println("Waiting for Wifi connection ...");
  }
  delay(SENSOR_CHECK_INTERVAL);
}

