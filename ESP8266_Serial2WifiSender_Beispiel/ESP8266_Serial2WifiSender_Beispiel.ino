/**
 * Beispiel-Anwendung für den ESP8266_SerialWifiSender
 * 
 * Die RX-Leitung ist mitteles eines 2:3 Spannungsteilers
 * am D2 des Arduino angeschlossen.
 * 
 */

#include <SoftwareSerial.h>

SoftwareSerial espSender(-1, 2); // RX, TX

const char udpMarker[]="@UDP ";
void sendUdpMessage(byte* message, int length) {
  espSender.print(udpMarker);
  for (int i=0;i<length;i++) {
    int c=message[i];
    if (c<0x10) espSender.print(0);
    espSender.print((int)c,HEX);
  }
  espSender.println();
}

void setup() {
  Serial.begin(115200);
  Serial.println("Initializing");
  pinMode(2, OUTPUT);
  espSender.begin(57600);
  delay(1000);
  sendUdpMessage((byte*) 'Starting ...3',5);
  delay(1000);
  sendUdpMessage((byte*) 'Starting ...2',5);
  delay(1000);
  sendUdpMessage((byte*) 'Starting ...1',5);
}

byte seq;
void loop() {
  String str="Msg # "+String(seq)+" ";
  int length=16; // enough for message + padding to 16 bytes for better hexdump display on the receiver side
  byte msg[length];
  for (int i=0;i<length;i++) {
    msg[i]=' ';
  }
  str.getBytes(msg,str.length());
  
  msg[length-2]=seq; // raw value to test if 0...255 are transmitted correctly
  msg[length-1]='\n';
  sendUdpMessage(msg,length);
  
  delay(500);
  seq++;
}
