/**
 * LED PWM mit HomeWizard Funkschalter
 * D3 - input - 433 MHZ Transmitter
 * D5 - output Mosfet (led)
*/

#include <ESP8266WiFi.h>
#include <ext_HomeWizard.h>

void setup() {
  Serial.begin(115200);

  // Wifi off - https://github.com/esp8266/Arduino/issues/644#issuecomment-174932102
  WiFi.mode(WIFI_OFF);
  WiFi.forceSleepBegin();
  delay(1);
  
  //analogWriteFreq(8000);

  pinMode(D3,INPUT);
  pinMode(D5,OUTPUT);

  digitalWrite(D5,0);

  HomeWizard::startReceiving(D3);
}

int led_state=0;

void loop() {
  HW_DATAGRAM data;
  if (HomeWizard::receiveData(&data))
  {
    Serial.print("Switch: 0x"); Serial.println(data.id, HEX);
    Serial.print("Unit: "); Serial.println(data.unit, DEC);
    Serial.print("State: "); Serial.println(data.state ? "ON" : "OFF");
    Serial.print("All? "); Serial.println(data.all ? "Yes" : "No");
    
    if (data.id==0xB2AFFF && data.unit==14) {
      if (data.state) {
        led_state=0;
      } else {
        led_state++;
        if (led_state>3) led_state=1;
      }
      Serial.println(led_state);
      switch(led_state) {
        case 0:
          digitalWrite(D5,0);
          break;
        case 1:
          digitalWrite(D5,1);
          break;
        case 2:
          analogWrite(D5,400);
          break;
        case 3:
          analogWrite(D5,50);
          break;
      }
    }
  }
}
