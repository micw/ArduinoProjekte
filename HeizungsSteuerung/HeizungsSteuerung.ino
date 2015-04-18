/************************************************************************
 *
 * Heizungssteuerung mit Hysterese, einstellbarer Schalttemperatur und
 * Display mit Anzeige Schalttemperatur + aktuelle Temperatur
 *
 * Hardware:
 * 1. Arduino (getestet mit Uno und Nano-China-Billig-Nachbau)
 *
 * 2. Temperatursensor DS18S20 oder DS1820, siehe http://datasheets.maximintegrated.com/en/ds/DS18S20.pdf
 * - GND angeschlossen
 * - +5V angeschlossen
 * - Data <-> Digital Pin 9
 * - Data <-> 4,7 kOhm <-> +5V  (2kOhm tut's auch)
 *
 * 3. Potentiometer, 10kOhm (oder mehr), siehe http://www.arduino.cc/en/tutorial/potentiometer
 * - Äußere Kontakte an GND und +5V
 * - Schleifkontakt an Analog Pin 0
 *
 * 4. 433 MHz-Sender http://www.homecontrol4.me/de/?id=bauanleitung
 * - GND angeschlossen
 * - +5V angeschlossen
 * - Data <-> Digital Pin 8
 *
 ************************************************************************/


#include <OneWire.h>
#include <DS18X20.h>
#include <OneWireListener.h>

const int PIN_SENSOR=9;
const int PIN_LED=13;
const int PIN_POTI_ANALOG=0;


OneWireListener onewire1=OneWireListener(PIN_SENSOR);

const int destMin=4;
const int destMax=30;

int destTemp=25;
float myTemp=destTemp;

void receiveDS18x20Temperature(int sensorNumber, unsigned int sensorId, float temperature) {
  Serial.print("Temp ");
  Serial.print(sensorId);
  Serial.print(" = ");
  Serial.println(temperature);
  myTemp=temperature;
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_LED, OUTPUT);  
  
  onewire1.setDS18x20TemperatureCallBack(receiveDS18x20Temperature);
  onewire1.initialize();
  delay(100);
  onewire1.readValues();
  delay(1000);
}

boolean onOff=false;


float destOld=0;
bool onOffOld=false;

int loopCounter=-1;
void loop() {
  bool changed=false;
  
  if (++loopCounter>=1000) loopCounter=0;
  if ((loopCounter%50)==0) { // every 5s
    onewire1.readValues();
    changed=true;
  }

  float destNow=analogRead(PIN_POTI_ANALOG);
  destNow=destNow*(destMax-destMin)/1024+destMin+0.1;
  float destTempOn=(int)destNow;
  float destTempOff=destTempOn+1;  

  if (destOld!=destTempOn) {
    destOld=destTempOn;
    changed=true;
  }

  if (onOff && myTemp>destTempOff) onOff=false;
  else if (!onOff && myTemp<destTempOn) onOff=true;
  
  if (onOffOld!=onOff) {
    onOffOld=onOff;
    changed=true;
  }

  if (changed) { // on change 10s -> send data + switch
    Serial.print("dest = ");
    Serial.println(destTempOn);
    
    if (onOff) Serial.println("on");
    else Serial.println("off");
  
    digitalWrite(PIN_LED, onOff);
  }
    
  delay(100);
}
