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
}

boolean onOff=false;

int loopCounter=0;

void loop() {
  
  if (++loopCounter>=1000) loopCounter=0;

  float destNow=analogRead(PIN_POTI_ANALOG);
  destNow=destNow*(destMax-destMin)/1024+destMin+0.1;
  float destTempOn=(int)destNow;
  float destTempOff=destTempOn+1;  

  Serial.print("dest = ");
  Serial.println(destTempOn);
  
  if ((loopCounter%20)==0) { // every 2s
    onewire1.readValues();

    if (onOff && myTemp>destTempOff) onOff=false;
    else if (!onOff && myTemp<destTempOn) onOff=true;
    
    if (onOff) Serial.println("on");
    else Serial.println("off");
  
    digitalWrite(PIN_LED, onOff);
  }
  delay(100);
}
