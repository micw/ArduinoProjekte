/**
 * Ein ESP8622 NodeMCU Develompent Board, welches via WIFI empfangene Schaltsignale
 * für Elro-Transmitter (Funktsteckdosen) via 433 MHz Sender übermittelt.
 * http://frightanic.com/iot/comparison-of-esp8266-nodemcu-development-boards/
 * 
 * Achtung, Pins beachten. Pin D1 entspricht im Sketch Digital5, siehe:
 * http://cdn.frightanic.com/blog/wp-content/uploads/2015/09/esp8266-nodemcu-dev-kit-v2-pins.png
 * 
 * Die Schaltsignale werden als UDP-Broadcast an UDP_RECEIVE_PORT gesendet.
 * Format:
 * @SWITCH SC CH on|off
 *   SC = Systemcode 0...31
 *   CH = Channel A..E
 *   on|off = Schaltzustand
 * 
 * 1. 433 MHz Transmitter
 * - VCC und GND verbinden
 * - DATA an ESP D1 (=Digital5)
 * 
 * 2. DS18x20 http://datasheets.maximintegrated.com/en/ds/DS18S20.pdf
 * - VCC und GND verbinden
 * - DATA an ESP D2 (=Digital4)
 * - Data <-> 4,7 kOhm <-> +5V  (2kOhm tut's auch)
 *   
 */

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <private_wifi.h>
#include "RemoteTransmitter.h"

#define UDP_RECEIVE_PORT 5001
#define RADIO_TRANSMITTER_PIN 5

WiFiUDP Udp;

ElroTransmitter switchTransmitter(RADIO_TRANSMITTER_PIN);

void setup() {
  wdt_enable(WDTO_8S); // Watchdog muss alle 8 Sekunden resettet werden
  Serial.begin(115200);
  Serial.println("Initializing");
  WiFi.begin(WIFI_SSID,WIFI_PASSWORD);
  wdt_reset(); // Watchdock resetten
  Serial.println("Starting");
}

const size_t maxMsgSize=50;
char msg[maxMsgSize+2];

boolean parseUdpMessage(char msg[],int len) {
  
  // @SWITCH 13 A on
  char *ptr;
  ptr = strtok(msg, " ");
  if (!ptr || strcmp(ptr, "@SWITCH")!=0) return false;
  ptr = strtok(NULL, " ");
  if (!ptr) return false;

  char *end;
  long _systemCode = strtol(ptr, &end, 10);
  if (end == ptr || *end != '\0' || _systemCode<0 || _systemCode>31) return false;
  int systemCode=_systemCode;

  ptr = strtok(NULL, " ");
  if (!ptr || strlen(ptr)!=1) return false;
  char channel=ptr[0];
  if (channel<'A'||channel>'E') return false;

  ptr = strtok(NULL, " \r\n");
  if (!ptr) return false;
  bool switchState;
  if (strcmp(ptr,"on")==0) switchState=1;
  else if (strcmp(ptr,"off")==0) switchState=0;
  else return false;

  Serial.print("Sending switch signal: ");
  Serial.print(systemCode);
  Serial.print(" ");
  Serial.print(channel);
  Serial.print(" ");
  Serial.println(ptr);

  noInterrupts();
  switchTransmitter.sendSignal(systemCode,channel,switchState);
  interrupts();
  
  Serial.println("Signal sent.");

  return true;
}

void receiveUdpMessage() {
  if(WiFi.status() == WL_CONNECTED) {
      if(!Udp) { 
          Serial.print("Listening on UDP ");
          Serial.println(UDP_RECEIVE_PORT);
          Udp.begin(UDP_RECEIVE_PORT);
      }
  }
  if(!Udp) return;
  int packetSize=Udp.parsePacket();
  if (packetSize>maxMsgSize) {
    Serial.print("Message too big, discarding ");
    Serial.print(Udp.available());
    Serial.println(" bytes");
    Udp.flush();
    return;
  }
  if (packetSize) {
    int len=Udp.read(msg,maxMsgSize);
    if (!parseUdpMessage(msg,len)) {
      Serial.print("Ignored invalid message: ");
      Serial.println(msg);
    }
  }
}

void loop() {
  wdt_reset(); // Watchdock resetten
  receiveUdpMessage();
}

