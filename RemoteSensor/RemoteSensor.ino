/************************************************************************
 *
 * Sensor-Arduino, welcher seine Daten via 433MHz funkt
 *
 * Der im Arduino eingebaute Watchdog wird verwendet um sicherzustellen, dass die Software
 * (sollte sie jemals abstürzen), nie mehr als 8s hängen bleibt.
 * 
 * Hardware:
 * 1. Arduino (getestet mit Nano-China-Billig-Nachbau)
 *
 * 2. Temperatursensor DS18S20 oder DS1820, siehe http://datasheets.maximintegrated.com/en/ds/DS18S20.pdf
 * - GND angeschlossen
 * - +5V angeschlossen
 * - Data <-> Digital Pin 9
 * - Data <-> 4,7 kOhm <-> +5V  (2kOhm tut's auch)
 * - TODO: Alternativ kann auch via ParasitePOwer mit 2 Adern angeschlossen werden
 * - An Pin 10 wird ebnso nach Temperatursensoren gesucht
 * 
 * Beim Start wird die Anzahl der gefundenen Sensoren via LED ausgegeben:
 * - langes leuchten
 * - 1x kurz blinken pro gefundenen Sensor an Digital Pin 9
 * - langes leuchten
 * - 1x kurz blinken pro gefundenen Sensor an Digital Pin 10
 * - langes leuchten
 * 
 * 3. 433 MHz-Sender http://www.homecontrol4.me/de/?id=bauanleitung
 *    Elro Funksteckdose https://bitbucket.org/fuzzillogic/433mhzforarduino/wiki/Home 
 * - GND angeschlossen
 * - +5V angeschlossen
 * - Data <-> Digital Pin 2
 * 
 * 4. Optional: Photo-Wiederstand an Analog Pin1 zur Helligkeitsmessung
 * - +5V Photo-Wiederstand <-> Analog In 1 <-> 10kOhm Wiederstand <-> GND
 *  
 * 5. Optional zu 4.: Dämmerungs-geschaltete LED an Digital Pin 7
 * - LED zwischen Digital Pin 7 und GND.
 * - Maximal 40 mA!!! Bei höheren Lasten Transistor verwenden!!!
 *
 ************************************************************************/


#include "avr/wdt.h" // Watchdog
#include <OneWire.h>
#include <DS18X20.h>
#include <OneWireListener.h>
#include <VirtualWire.h> // 433MHz Lib
#include "Timer.h"

const int PIN_SENSOR1=9;
const int PIN_SENSOR2=10;
const int PIN_SENDER=2;
const int PIN_LED=13;
const int SENSOR_CHECK_INTERVAL=30000;

#define BRIGHTNESS_SENSOR
#ifdef BRIGHTNESS_SENSOR
const byte BRIGTHNESS_SENSOR_ID=1;
const int PIN_BRIGHTNESS_ANALOG=1;
const int PIN_BRIGHTNESS_LED=7;
// Unterhalb welcher Helligkeit soll die LED an gehen?
// In meinem Setup fällt die Helligkeit während der Dämmerung (im Oktober) innerhalb von 10 Minuten von 200 auf 100
const int BRIGHTNESS_THRESOLD=120;
#endif


Timer timer=Timer();
OneWireListener onewire1=OneWireListener(PIN_SENSOR1);
OneWireListener onewire2=OneWireListener(PIN_SENSOR2);

void ledBlinkNumber(int number)
{
  digitalWrite(PIN_LED, true);
  delay(1000);
  digitalWrite(PIN_LED, false);
  delay(1000);
  for (int i=0;i<number;i++)
  {
          if (i>0) delay(500);
          digitalWrite(PIN_LED, true);
          delay(200);
          digitalWrite(PIN_LED, false);
  }
  delay(1000);
}

byte sensorMessageNum=0;
void receiveDS18x20Temperature(int sensorNumber, unsigned int sensorId, float temperature) {
    // Note: interrupts are disabled. You can re-enable them if needed.

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

    vw_send((uint8_t *)msg, 6);
    vw_send((uint8_t *)msg, 6);


    Serial.print(sensorId,HEX);
    Serial.print(" = ");
    Serial.print(temperature);
    Serial.print(" = ");
    Serial.print(tempEncoded);
    Serial.println();
}

#ifdef BRIGHTNESS_SENSOR
int getBrightness()
{
    // Get average of 50 Measures
    int brightnessCount=50;
    long brightnessSum=0;
    for (int i=0;i<brightnessCount;i++) {
      brightnessSum+=(1024-analogRead(PIN_BRIGHTNESS_ANALOG));
      delay(10);
    }
    wdt_reset(); // Watchdock resetten
    
    long brightness=brightnessSum/brightnessCount;
    return (int) brightness;
}
#endif

void timerReadSensorValues()
{
    Serial.print("FREE RAM: ");
    Serial.println(getFreeRam ());
    
#ifdef BRIGHTNESS_SENSOR
    Serial.println("Read values from Brightness");

    wdt_reset(); // Watchdock resetten

    int brightness=getBrightness();

    byte msg[5];
    msg[0]='d';
    msg[ 1]=BRIGTHNESS_SENSOR_ID;
    msg[ 2]=(byte) (brightness & 0xff);
    msg[ 3]=(byte) (brightness>>8 & 0xff);
    msg[ 4]=sensorMessageNum++;

    vw_send((uint8_t *)msg, 5);
    vw_send((uint8_t *)msg, 5);

    digitalWrite(PIN_BRIGHTNESS_LED, brightness<BRIGHTNESS_THRESOLD);
    
    Serial.print("Brightness = ");
    Serial.print(brightness);
    Serial.println();
#endif
  
    Serial.println("Read values from OneWire");

    digitalWrite(PIN_LED, true);
    onewire1.readValues();
    onewire2.readValues();
    digitalWrite(PIN_LED, false);
}

int getFreeRam () {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}



void setup() {
    wdt_enable(WDTO_8S); // Watchdog muss alle 8 Sekunden resettet werden
  
    Serial.begin(115200);
    Serial.println("Setup started.");
    pinMode(PIN_LED, OUTPUT);  

    int sensorCount;
    onewire1.setDS18x20TemperatureCallBack(receiveDS18x20Temperature);
    sensorCount=onewire1.initialize();
    wdt_reset(); // Watchdock resetten
    ledBlinkNumber(sensorCount);
  
    onewire2.setDS18x20TemperatureCallBack(receiveDS18x20Temperature);
    sensorCount=onewire2.initialize();
    wdt_reset(); // Watchdock resetten
    ledBlinkNumber(sensorCount);
  
    // 1x lang blinken
    wdt_reset(); // Watchdock resetten
    ledBlinkNumber(0);
    
    wdt_reset(); // Watchdock resetten
    delay(1000); // dem Sensor kurz Zeit geben
    wdt_reset(); // Watchdock resetten

#ifdef BRIGHTNESS_SENSOR
    Serial.println("Initializing brightness sensor + led.");
    pinMode(PIN_BRIGHTNESS_LED, OUTPUT);  
    digitalWrite(PIN_BRIGHTNESS_LED, true);
    delay(1000);
    digitalWrite(PIN_BRIGHTNESS_LED, false);
    wdt_reset(); // Watchdock resetten
    delay(1000);
    digitalWrite(PIN_BRIGHTNESS_LED, true);
    delay(1000);
    digitalWrite(PIN_BRIGHTNESS_LED, false);
    wdt_reset(); // Watchdock resetten
    delay(1000);

    // Initialwert
    digitalWrite(PIN_BRIGHTNESS_LED, getBrightness()<BRIGHTNESS_THRESOLD);
    
#endif

    
    // VirtualWire
    vw_set_tx_pin(PIN_SENDER);
    vw_setup(1000);

    // Timer: Sensoren zyklisch auslesen
    timer.every(SENSOR_CHECK_INTERVAL,timerReadSensorValues);

    Serial.println("Setup finished.");
}

void loop()
{
    wdt_reset(); // Watchdock resetten
    timer.update();
}

