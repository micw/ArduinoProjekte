/************************************************************************
 *
 * Sensor-Arduino, welcher seine Daten via ESP8266 funkt (siehe Sketch ESP8266_Serial2WifiSender)
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
 * 3. ESP8266 als Serial->Wifi Adapter (siehe Sketch ESP8266_Serial2WifiSender)
 *    Sendet gemessene Temperaturen, Solltemperatur und Heizungs-Status als UDP-Broadcasts ins WLAN
 * - GND an Power Regulator AMS1117
 * - 3.3V an Power Regulator AMS1117
 * - CH_PD via 10kOhm Pullup an 3,3V (zum Booten benötigt)
 * - RX via 3kOhm an GND
 * - RX via 2kOhm an D2
 * - es geht auch 3,3/2 oder 3,3/2,2 kOhm (Spannungsteiler 5V->3V)
 *
 * 4. Optional: Photo-Wiederstand an Analog Pin1 zur Helligkeitsmessung
 * - +5V Photo-Wiederstand <-> Analog In 1 <-> 10kOhm Wiederstand <-> GND
 * - Pin D5 auf GND, um den Sketch anzuzeigen, dass es einen Brightness-Sensor gibt
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
#include <SoftwareSerial.h>
#include "Timer.h"

const int PIN_ESP8266_RX=2;
const int PIN_SENSOR1=9;
const int PIN_SENSOR2=10;
const int PIN_SENDER=2;
const int PIN_LED=13;
const int SENSOR_CHECK_INTERVAL=30000;

const int PIN_BRIGTHNESS_SENSOR_AVAILABLE=5;


boolean BRIGHTNESS_SENSOR_AVAILABLE=false;
const byte BRIGTHNESS_SENSOR_ID=1;
const int PIN_BRIGHTNESS_ANALOG=1;
const int PIN_BRIGHTNESS_LED=7;
// Unterhalb welcher Helligkeit soll die LED an gehen?
// In meinem Setup fällt die Helligkeit während der Dämmerung (im Oktober) innerhalb von 10 Minuten von 200 auf 100
const int BRIGHTNESS_THRESOLD=120;


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

    sendUdpMessage(msg, 6);
    delay(2000);

    Serial.print(sensorId,HEX);
    Serial.print(" = ");
    Serial.print(temperature);
    Serial.print(" = ");
    Serial.print(tempEncoded);
    Serial.println();
}

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

void timerReadSensorValues()
{
    Serial.print("FREE RAM: ");
    Serial.println(getFreeRam ());
    
    if (BRIGHTNESS_SENSOR_AVAILABLE)
    {
      Serial.println("Read values from Brightness");
      wdt_reset(); // Watchdock resetten
      int brightness=getBrightness();
  
      byte msg[5];
      msg[0]='d';
      msg[ 1]=BRIGTHNESS_SENSOR_ID;
      msg[ 2]=(byte) (brightness & 0xff);
      msg[ 3]=(byte) (brightness>>8 & 0xff);
      msg[ 4]=sensorMessageNum++;
  
      sendUdpMessage(msg, 5);
  
      digitalWrite(PIN_BRIGHTNESS_LED, brightness<BRIGHTNESS_THRESOLD);
      delay(1000);
      
      Serial.print("Brightness = ");
      Serial.print(brightness);
      Serial.println();
    }
  
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
  
    // Initialize SoftwareSerial for ESP Wifi Sender at D2
    pinMode(PIN_ESP8266_RX, OUTPUT);
    espSender.begin(57600);

    pinMode(PIN_BRIGTHNESS_SENSOR_AVAILABLE, INPUT_PULLUP);

    wdt_reset(); // Watchdock resetten
    delay(500);
    wdt_reset(); // Watchdock resetten
    // Wenn PIN_BRIGTHNESS_AVAILABLE auf GND geschaltet ist, wird der Brightness-Sensor aktiviert
    BRIGHTNESS_SENSOR_AVAILABLE=(digitalRead(PIN_BRIGTHNESS_SENSOR_AVAILABLE)==LOW);

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

    if (BRIGHTNESS_SENSOR_AVAILABLE)
    {
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
    }
    else
    {
      Serial.println("Brightness sensor disabled.");
    }


    // Timer: Sensoren zyklisch auslesen
    timer.every(SENSOR_CHECK_INTERVAL,timerReadSensorValues);

    Serial.println("Setup finished.");
}

void loop()
{
    wdt_reset(); // Watchdock resetten
    timer.update();
}

