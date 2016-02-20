/************************************************************************
 *
 * Heizungssteuerung mit Hysterese, einstellbarer Schalttemperatur und
 * Display mit Anzeige Schalttemperatur + aktuelle Temperatur
 *
 * Der im Arduino eingebaute Watchdog wird verwendet um sicherzustellen, dass die Steuerung
 * (sollte sie jemals abstürzen), nie mehr als 8s hängen bleibt.
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
 *    Elro Funksteckdose https://bitbucket.org/fuzzillogic/433mhzforarduino/wiki/Home 
 * - GND angeschlossen
 * - +5V angeschlossen
 * - Data <-> Digital Pin 8
 *
 * 5. (optional) extende Status-Leuchte http://www.arduino.cc/en/Tutorial/Blink?from=Tutorial.BlinkingLED
 * - GND angeschlossen
 * - LED+ <-> 220 Ohm <-> PinDigital Pin 13
 *
 * 6. (optional) LDC-Display
 *     http://www.dfrobot.com/index.php?route=product/product&keyword=160&product_id=135
 *     http://www.geeetech.com/wiki/index.php/Serial_I2C_1602_16%C3%972_Character_LCD_Module
 *     Neuer Treiber!
 *     https://bitbucket.org/fmalpartida/new-liquidcrystal/wiki/Home
 *     http://forum.arduino.cc/index.php?topic=158312.0
 *
 * - Display GND <-> GND
 * - Display VCC <-> +5V
 * - Display SDA <-> Analog Pin 4
 * - Display SCL <-> Analog Pin 5
 *
 * 7. (optional) ESP8266 als Serial->Wifi Adapter (siehe Sketch ESP8266_Serial2WifiSender)
 *    Sendet gemessene Temperaturen, Solltemperatur und Heizungs-Status als UDP-Broadcasts ins WLAN
 * - GND an Power Regulator AMS1117
 * - 3.3V an Power Regulator AMS1117
 * - CH_PD via 10kOhm Pullup an 3,3V (zum Booten benötigt)
 * - RX via 3kOhm an GND
 * - RX via 2kOhm an D2
 * - es geht auch 3,3/2 oder 3,3/2,2 kOhm (Spannungsteiler 5V->3V)
 *
 * 8. (optional) Außen-Temperatursensor DS18S20 oder DS1820, siehe http://datasheets.maximintegrated.com/en/ds/DS18S20.pdf
 * - GND angeschlossen
 * - +5V angeschlossen
 * - Data <-> Digital Pin 10
 * - Data <-> 4,7 kOhm <-> +5V  (2kOhm tut's auch)
 *
 ************************************************************************/


#include "avr/wdt.h" // Watchdog
#include <OneWire.h>
#include <DS18X20.h>
#include <OneWireListener.h>
#include <RemoteTransmitter.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>


const int PIN_ESP8266_RX=2;
const int PIN_SENSOR_INNEN=9;
const int PIN_SENSOR_AUSSEN=10;
const int PIN_SENDER=8;
const int PIN_LED=13;
const int PIN_POTI_ANALOG=0;
const unsigned short SWITCH_SYSTEM_CODE=30;
const unsigned char SWITCH_CHANNEL='A';


OneWireListener onewire1=OneWireListener(PIN_SENSOR_INNEN);
OneWireListener onewire2=OneWireListener(PIN_SENSOR_AUSSEN);
ElroTransmitter switchTransmitter=ElroTransmitter(PIN_SENDER);
LiquidCrystal_I2C  lcd(0x3F,2,1,0,4,5,6,7); // LCD Address is 0x27 or 0x3F


const int destMin=4;
const int destMax=30;

int destTemp=25;
float myTempInnen=destTemp;
float myTempAussen=NAN;

void receiveDS18x20TemperatureInnen(int sensorNumber, unsigned int sensorId, float temperature) {
  Serial.print("Temp (innen) ");
  Serial.print(sensorId);
  Serial.print(" = ");
  Serial.println(temperature);
  myTempInnen=temperature;
  sendTemperatureViaUdp(sensorId,temperature);
}
void receiveDS18x20TemperatureAussen(int sensorNumber, unsigned int sensorId, float temperature) {
  Serial.print("Temp (aussen) ");
  Serial.print(sensorId);
  Serial.print(" = ");
  Serial.println(temperature);
  myTempAussen=temperature;
  sendTemperatureViaUdp(sensorId,temperature);
}

// ESP8266 wifi send stuff
SoftwareSerial espSender(-1, PIN_ESP8266_RX); // RX, TX
const char marker[]="@UDP ";
void sendUdpMessage(byte* message, int length) {
  Serial.println("Sending udp message");
  espSender.print(marker);
  for (int i=0;i<length;i++) {
    int c=message[i];
    if (c<0x10) espSender.print(0);
    espSender.print((int)c,HEX);
  }
  espSender.println();
}


void setup() {
  // Initialize SoftwareSerial for ESP Wifi Sender at D2
  pinMode(PIN_ESP8266_RX, OUTPUT);
  espSender.begin(57600);
  
  delay(1000); // dem Board kurz Zeit geben
  wdt_enable(WDTO_8S); // Watchdog muss alle 8 Sekunden resettet werden
  Serial.begin(115200);
  pinMode(PIN_LED, OUTPUT);  
  
  lcd.begin(16,2);
  lcd.setBacklightPin(3,POSITIVE);
  lcd.setBacklight(HIGH); 
  
  onewire1.setDS18x20TemperatureCallBack(receiveDS18x20TemperatureInnen);
  onewire1.initialize();
  onewire2.setDS18x20TemperatureCallBack(receiveDS18x20TemperatureAussen);
  onewire2.initialize();
  
  wdt_reset(); // Watchdock resetten
  delay(1000); // dem Sensor kurz Zeit geben
  wdt_reset(); // Watchdock resetten
}

byte sensorMessageNum=0;
void sendStatusViaUdp(boolean onOff, float destTemp) {
    byte msg[6];
    msg[0]='b';
    msg[ 1]=onOff?1:0;
    msg[ 2]=SWITCH_SYSTEM_CODE;
    msg[ 3]=SWITCH_CHANNEL;
    msg[ 4]=(byte)destTemp;
    msg[ 5]=sensorMessageNum++;
    sendUdpMessage(msg,6);
}
void sendTemperatureViaUdp(unsigned int sensorId, float temperature) {
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
    sendUdpMessage(msg,6);
}

boolean onOff=false;


float destOld=0;
bool onOffOld=false;
bool showAussenTemp=false;

int loopCounter=-1;
void loop() {
  wdt_reset(); // Watchdock resetten
  bool changed=false; // relevant für Display
  bool switchChanged=false; // relevant für Display
  if (loopCounter<0) switchChanged=true; // beim 1. Aufruf Schaltbefehl sofort senden
  
  if (++loopCounter>=32000 || loopCounter<0) loopCounter=0; // Überlauf verhindern
  if ((loopCounter%300)==10) { // every 30s
    onewire1.readValues();
    onewire2.readValues();
    changed=true;
  }
  if ((loopCounter%100)==0) { // every 10s
    showAussenTemp=!showAussenTemp;
    changed=true;
  }

  float destNow=analogRead(PIN_POTI_ANALOG);
  destNow=destNow*(destMax-destMin)/1024+destMin+0.1;
  float destTempOn=(int)destNow;
  float destTempOff=destTempOn+1;  

  if (destOld!=destTempOn) {
    destOld=destTempOn;
    changed=true;
    switchChanged=true;
  }

  if (onOff && myTempInnen>destTempOff) onOff=false;
  else if (!onOff && myTempInnen<destTempOn) onOff=true;
  
  if (onOffOld!=onOff) {
    onOffOld=onOff;
    changed=true;
  }

  if (changed) { // Änderung -> Display+StatusLED updaten
    sendStatusViaUdp(onOff,destTempOn);
    
    Serial.print("dest = ");
    Serial.println(destTempOn);
    
    if (onOff) Serial.println("on");
    else Serial.println("off");
  
    digitalWrite(PIN_LED, onOff);
    
    char displayMsg[16];
    char tmpIstStr[4];
    lcd.setCursor(0,0);
    if (isnan(myTempAussen) || !showAussenTemp) {
      dtostrf(myTempInnen,5,1,tmpIstStr);
      sprintf(displayMsg, "Innen:  %s+C ", tmpIstStr);
    } else {
      dtostrf(myTempAussen,5,1,tmpIstStr);
      sprintf(displayMsg, "Aussen: %s+C ", tmpIstStr);
    }
    displayMsg[13]=0xDF; //°
    lcd.print(displayMsg);
    lcd.setCursor(0,1);
    if (onOff) {
      sprintf(displayMsg, "MIN: %2d+C  Ein  ", (int)destTempOn);
    }
    else {
      sprintf(displayMsg, "MIN: %2d+C  Aus  ", (int)destTempOn);
    }
    displayMsg[7]=0xDF; //°
    lcd.print(displayMsg);
  }
  
  if (switchChanged||((loopCounter%300)==0)) { // Schaltbefehl bei Änderung Status senden und alle 30s wiederholen (stellt sicher, dass die Heizung nicht an oder aus bleibt)
    noInterrupts();
    switchTransmitter.sendSignal(SWITCH_SYSTEM_CODE,SWITCH_CHANNEL,onOff);
    interrupts();
  }

  delay(100);
}
