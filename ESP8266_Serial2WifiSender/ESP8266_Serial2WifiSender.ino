/*
 * Ein EPS8622 als serieller Sender, welcher Datenpakete, die in einem bestimmten Format seriell eingehen
 * via UDP-Broadcast ins Netz schickt.
 * Dieser ersetzt die 433MHz Sender, die ich bisher verwende.
 * 
 * ESP Pinout: https://mcuoneclipse.files.wordpress.com/2014/11/esp8266-pins.png
 * 
 * Stromversorgung via 3,3V Power Regulator AMS1117
 * ESP8266:
 * - GND an Power Regulator
 * - 3.3V an Power Regulator
 * - CH_PD via 10kOhm Pullup an 3,3V (zum Booten benötigt)
 * 
 * Serielle Anbindung an Arduino:
 * - Arduino mit einem Spannungsteiler Digial Pin2 -> 2k -> ESP RX -> 3 k -> GND
 * - Arduino mit GND an ESP GND
 * - 3.3V-Regler an ESP VCC
 * - 3.3V-Regler -> 10k -> ESP CH_PH
 * - Stromversorgung über Arduino:
 *   - 3.3V Power Regulator an Arduino 5V (Low-Drop-Regulator)
 * - Stromversorgung über Netzteil:
 *   - 3.3V+GND Power Regulator an Netzteil
 *
 * Kommunikation:
 * - seriellen Eingang des ESP verwenden
 * - Message-Format:
 *   @UDP CAFEBABE
 *   where CAFEBABE is the hex-encoded message
 */

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <private_wifi.h>

const int send_port = 5000;
WiFiUDP Udp;

IPAddress BROADCAST_IP(255,255,255,255);

const char marker[]="@UDP ";
const char markerLength=strlen(marker);
const int msgMaxLength=1023;
byte msgBuffer[msgMaxLength];


void setup() {
  Serial.begin(57600);
  Serial.println("Initializing");
  WiFi.begin(WIFI_SSID,WIFI_PASSWORD);
}

/****
 * Serial Read Line with fixed memory footprint 
 */
const int maxSerialLineLength=2+markerLength+2*msgMaxLength; // Enough for the marker and the message and a little extra space (required to detect if line is too long)
char serialLineBuffer[maxSerialLineLength+1]; // 1 extra char for NULL-terminator
int serialLineBufferPos=0;
char* serialReadLn() {
  char character;

  while(Serial.available()) {
      character = Serial.read();
      if (character==13) continue; // skip \r
      serialLineBuffer[serialLineBufferPos++]=character; // add character to bufefr
      if ((character==10) || (serialLineBufferPos==maxSerialLineLength)) { // \n = new Line or max line length reached
        if (character==10) serialLineBufferPos--; // overwrite newline
        serialLineBuffer[serialLineBufferPos]=0;
        serialLineBufferPos=0;
        return serialLineBuffer;
      }
  }
  return NULL;
}

/**
 * Returns a hex byte at the offset or -1
 */
int parseHexByte(char* string, int offset) {
  int result=0;
  for (int i=0;i<2;i++) {
    char c=string[offset+i];
    result=result<<4;
    if(c >= '0' && c <= '9') result+=(byte)(c - '0');
    else if(c >= 'A' && c <= 'F') result+=(byte)(c - 'A'+10);
    else if(c >= 'a' && c <= 'f') result+=(byte)(c - 'a'+10);
    else return -1;
  }
  return result;
}

void loop() {
  // Read a line
  char* line=serialReadLn();
  // If line starts with marker:
  if (line && strncmp(line,marker,markerLength)==0) {

    // Calculate message length
    int msgLength=(strlen(line)-markerLength+1)/2;

    // Message too long for buffer?
    if (msgLength>msgMaxLength) {
      Serial.print("Message too long (");
      Serial.print(msgLength);
      Serial.print(" bytes, max is ");
      Serial.print(msgMaxLength);
      Serial.print(" bytes): ");
      Serial.print(line);
      return;
    }

    // Build the message
    for (int i=0;i<msgLength;i++) {
      int value=parseHexByte(line,markerLength+2*i);
      if (value<0) {
        Serial.print("Invalid udp message: ");
        Serial.println(line);
        return;
      }
      msgBuffer[i]=(char) value;
    }

    Serial.print("Sending: ");
    Serial.println((char*) (line + markerLength));

    // Broadcast UDP message
    Udp.beginPacket(BROADCAST_IP, send_port);
    Udp.write(msgBuffer,msgLength);
    Udp.endPacket();
    
  }
}


