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
 * - Display GND <-> GND
 * - Display VCC <-> +5V
 * - Display SDA <-> Analog Pin 4
 * - Display SCL <-> Analog Pin 5
 *
 ************************************************************************/


#include "avr/wdt.h" // Watchdog
#include <OneWire.h>
#include <DS18X20.h>
#include <OneWireListener.h>
#include <RemoteTransmitter.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>


const int PIN_SENSOR=9;
const int PIN_SENDER=8;
const int PIN_LED=13;
const int PIN_POTI_ANALOG=0;
const unsigned short SWITCH_SYSTEM_CODE=30;
const unsigned char SWITCH_CHANNEL='A';


OneWireListener onewire1=OneWireListener(PIN_SENSOR);
ElroTransmitter switchTransmitter=ElroTransmitter(PIN_SENDER);
LiquidCrystal_I2C lcd=LiquidCrystal_I2C(0x27,16,2);  // set the LCD address to 0x27 for a 16 chars and 2 line display


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
  wdt_enable(WDTO_8S); // Watchdog muss alle 8 Sekunden resettet werden
  Serial.begin(115200);
  pinMode(PIN_LED, OUTPUT);  
  
  lcd.init();
  lcd.backlight();
  
  onewire1.setDS18x20TemperatureCallBack(receiveDS18x20Temperature);
  onewire1.initialize();
  wdt_reset(); // Watchdock resetten
  delay(1000); // dem Sensor kurz Zeit geben
  wdt_reset(); // Watchdock resetten
}

boolean onOff=false;


float destOld=0;
bool onOffOld=false;

int loopCounter=-1;
void loop() {
  wdt_reset(); // Watchdock resetten
  bool changed=false; // relevant für Display
  bool switchChanged=false; // relevant für Display
  if (loopCounter<0) switchChanged=true; // beim 1. Aufruf Schaltbefehl sofort senden
  
  if (++loopCounter>=32000 || loopCounter<0) loopCounter=0; // Überlauf verhindern
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
    switchChanged=true;
  }

  if (onOff && myTemp>destTempOff) onOff=false;
  else if (!onOff && myTemp<destTempOn) onOff=true;
  
  if (onOffOld!=onOff) {
    onOffOld=onOff;
    changed=true;
  }

  if (changed) { // Änderung -> Display+StatusLED updaten
    Serial.print("dest = ");
    Serial.println(destTempOn);
    
    if (onOff) Serial.println("on");
    else Serial.println("off");
  
    digitalWrite(PIN_LED, onOff);
    
    char tmpIstStr[4];
    dtostrf(myTemp,2,1,tmpIstStr);
    char displayMsg[16];
    lcd.setCursor(0,0);
    sprintf(displayMsg, "IST: %s +C", tmpIstStr);
    displayMsg[10]=0xDF; //°
    lcd.printstr(displayMsg);
    lcd.setCursor(0,1);
    if (onOff) {
      sprintf(displayMsg, "MIN: %2d.0 +C Ein", (int)destTempOn);
    }
    else {
      sprintf(displayMsg, "MIN: %2d.0 +C Aus", (int)destTempOn);
    }
    displayMsg[10]=0xDF; //°
    lcd.printstr(displayMsg);
  }
  
  if (switchChanged||((loopCounter%300)==0)) { // Schaltbefehl bei Änderung Status senden und alle 30s wiederholen (stellt sicher, dass die Heizung nicht an oder aus bleibt)
    noInterrupts();
    switchTransmitter.sendSignal(SWITCH_SYSTEM_CODE,SWITCH_CHANNEL,onOff);
    interrupts();
  }

  delay(100);
}
