#include <OneWire.h>
#include <DallasTemperature.h>
#include <Ethernet.h>

#define DS18S20_PIN 8
#define SWITCH_PIN 8

OneWire DS18S20_WIRE(DS18S20_PIN);
DallasTemperature sensors(&DS18S20_WIRE);

// mac: CA:FE:FE:ED:00:01 (range free to use)
byte mac[] = { 0xCA, 0xFE, 0xFE, 0xED, 0x00, 0x01 }; 

EthernetServer server(80);

void setup() {
  Serial.begin(115200);
  sensors.begin();
  sensors.setResolution(12); // 9=0.5°C, 10=0.25°C,... max 12 
  Ethernet.begin(mac);
  server.begin();

  pinMode(SWITCH_PIN,INPUT_PULLUP);
}

void loop() {
  EthernetClient client = server.available();
  if (client) {
    while (client.connected()) {
      while (client.available()) {
        client.read();
      }
      sensors.requestTemperatures(); // Send the command to get temperatures
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/json");
      client.println("Connection: close");
      client.println("Refresh: 30");
      client.println();
      client.println("{");
      client.print("  temp: ");
      client.print(sensors.getTempCByIndex(0));
      client.println(",");
      client.print("  switch: ");
      client.println(digitalRead(SWITCH_PIN));
      client.println("}");
      
      delay(50);
      client.stop();
    }
  }
}
