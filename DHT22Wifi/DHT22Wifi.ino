/****************************************
 * 
 * Kombination eines ESP8266 mit einem DHT22 Feuchte- und
 * Temperatursensor
 * 
 * ESP Pinout: https://mcuoneclipse.files.wordpress.com/2014/11/esp8266-pins.png
 * 
 * Stromversorgung via 3,3V Power Regulator AMS1117
 * ESP8266:
 * - GND an Power Regulator
 * - 3.3V an Power Regulator
 * - CH_PD via 10kOhm Pullup an 3,3V (zum Booten benötigt)
 * 
 * DHT22:
 * - GND an Power Regulator
 * - 3.3V an Power Regulator
 * - Data an ESP8266 GPIO 2
 * 
 */

#include "DHT.h"
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <private_wifi.h>

#define DHTPIN 2
#define DHTTYPE DHT22

const int sensorId = 0x0012;

const int send_port = 5000;
WiFiUDP Udp;

IPAddress BROADCAST_IP(192,168,1,255);

byte sensorMessageNum=0;

// Initialize DHT sensor.
// Note that older versions of this library took an optional third parameter to
// tweak the timings for faster processors.  This parameter is no longer needed
// as the current DHT reading algorithm adjusts itself to work on faster procs.
DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);
  Serial.println("Waiting 5s");
  delay(5000);
  Serial.println("Initializing");
  dht.begin();

  WiFi.mode(WIFI_STA); // disable hotspot
  WiFi.begin(WIFI_SSID,WIFI_PASSWORD);
}

byte msg[6];

void loop() {
 // Wait a few seconds between measurements.
  delay(10000);

  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
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

  int tempEncoded;
  if (t>0) tempEncoded =(int) (t*100+0.5);
  else tempEncoded =(int) (t*100-0.5);

  int humidityEncoded=(int) (h*100+0.5);

  byte * temperatureBytes = (byte *) &tempEncoded;
  byte * humidityBytes = (byte *) &humidityEncoded;

  msg[0]='a';
  msg[ 1]=(byte) (sensorId & 0xff);
  msg[ 2]=(byte) (sensorId>>8 & 0xff);
  msg[ 3]=temperatureBytes[0];
  msg[ 4]=temperatureBytes[1];
  msg[ 5]=sensorMessageNum++;

  Udp.beginPacket(BROADCAST_IP, send_port);
  Udp.write(msg,6);
  Udp.endPacket();

  msg[0]='e';
  msg[ 3]=humidityBytes[0];
  msg[ 4]=humidityBytes[1];
  msg[ 5]=sensorMessageNum++;

  Udp.beginPacket(BROADCAST_IP, send_port);
  Udp.write(msg,6);
  Udp.endPacket();
  
}
