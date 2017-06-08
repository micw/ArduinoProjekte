/**
 * Steuerung der Badbeleuchtung (12V LED-Stripes)
 * 
 * Hardware:
 * - NodeMCU Devboard
 * - 12V Spannungsquelle. Diese muss mit GND am NodeMCU-GND hängen, sonst sterben die MOSFET in Sekunden!
 *   12+ an Pluspol der LED-Stripes
 * - IRLZ34N MOSFET Pins (von vorne betrachtet von links nach rechts): G D S
 *   G: direkt an Digital-Out des NodeMCU (kein Wiederstand nötig) sowie mit 10k gegen GND
 *   D: Liefert die geschaltete Masse der Leuchstreifen. Minuspol der Stripes hier anschließen
 *   S: an GND
 *   - an D1: Akzeptbeleuchtung
 *   - an D2: Badschrank links
 *   - an D3: Badschrank rechts
 * - In den beiden Schränken je ein Türschalter (Geschlossen bei geöffneter Tür)
 *   - ein Pol an GND, den anderen an:
 *   - an D6: Schalter Badschrank links
 *   - an D7: Schalter Badschrank rechts
 * - 12->5V Spannungsregler, um den NodeMCU vom LED-Travo mit versorgen zu lassen
 * 
 * Die MOSTFET können einige Ampere schalten (auch per PWM). Da die Eingänge aber nur mit 3,3V getrieben werden,
 * sollte man es nicht übertreiben bzw. ab und zu die Temperatur checken. Kühlblech könnte nicht schaden.
 * Idee: einen DS18S20 an die Mosfets hängen zur Selbstüberwachung ;-)
 * 
 * Funktionen:
 * - Ziel-Helligkeit wird per MQTT übergeben. Beim Start ist diese 0
 * - Akzeptbeleuchtung hat immer Ziel-Helligkeit
 * - Schränke haben Zielhelligkeit, wenn sie geschlossen sind und 100% wenn sie geöffnet sind
 * - Zwischen Helligkeiten wird sanft gefadet
 */

#define LED_AKZENT     D1
#define LED_SCHRANK1   D2
#define LED_SCHRANK2   D3
#define BTN_SCHRANK1   D6
#define BTN_SCHRANK2   D7

#define STATUS_INITIAL 0
#define HELLIGKEIT_INITIAL 0
#define HELLIGKEIT_SCHRANK_OFFEN 1023

#define FADE_UP_SPEED_MS 500
#define FADE_DOWN_SPEED_MS 1500


// Schließer: LOW, Öffner: HIGH
#define BUTTON_PRESSED_VALUE HIGH
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <private_wifi.h>
#include <PubSubClient.h>

#define MQTT_CLIENT_ID "BadLicht"
#define MQTT_TOPIC "badlicht/set/#"

WiFiClient espClient;
PubSubClient client(espClient);

char responseTopic[128];

int statusAkzent=STATUS_INITIAL;
int statusSchrank=STATUS_INITIAL;

int helligkeitZielAkzent=HELLIGKEIT_INITIAL;
int helligkeitZielSchrank=HELLIGKEIT_INITIAL;

struct LedFade {
  int helligkeit_start;
  int helligkeit_ist;
  int helligkeit_soll;
  unsigned long zeit_start;
  unsigned long zeit_ende;
};

LedFade AKZENT = { HELLIGKEIT_INITIAL,HELLIGKEIT_INITIAL,HELLIGKEIT_INITIAL,0,0 };
LedFade SCHRANK1 = { HELLIGKEIT_INITIAL,HELLIGKEIT_INITIAL,HELLIGKEIT_INITIAL,0,0 };
LedFade SCHRANK2 = { HELLIGKEIT_INITIAL,HELLIGKEIT_INITIAL,HELLIGKEIT_INITIAL,0,0 };

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Received ");
  Serial.print(topic);
  Serial.print(": ");

  payload[length] = '\0';
  String s = String((char*)payload);
  int value=s.toInt();

  Serial.println(value);
  
  if (value<0) value=0;
  else if (value>1023) value=1023;

  String command=String(topic).substring(strlen(MQTT_TOPIC)-1);

  if (command.equals("schrankbr")) {
    helligkeitZielSchrank=value;
  } else if (command.equals("schrank")) {
    statusSchrank=value>0;
  } else if (command.equals("akzentbr")) {
    helligkeitZielAkzent=value;
  } else if (command.equals("akzent")) {
    statusAkzent=value>0;
  } else {
    Serial.print("Unkown command: ");
    Serial.println(command);
  }
  sendStatus();
}

unsigned long nextStatusUpdate=millis()+60000;

void sendStatus() {
  Serial.println("Updating status");

  char payload[5];

  String(statusSchrank).toCharArray(payload,5);
  client.publish("badlicht/state/schrank",payload);
  String(helligkeitZielSchrank).toCharArray(payload,5);
  client.publish("badlicht/state/schrankbr",payload);
  String(statusAkzent).toCharArray(payload,5);
  client.publish("badlicht/state/akzent",payload);
  String(helligkeitZielAkzent).toCharArray(payload,5);
  client.publish("badlicht/state/akzentbr",payload);
}

void setup() {     
  Serial.begin(115200);
  wdt_enable(WDTO_8S); // Watchdog muss alle 8 Sekunden resettet werden
  pinMode(LED_AKZENT, OUTPUT);
  pinMode(LED_SCHRANK1, OUTPUT);
  pinMode(LED_SCHRANK2, OUTPUT);
  pinMode(BTN_SCHRANK1, INPUT_PULLUP);
  pinMode(BTN_SCHRANK2, INPUT_PULLUP);

  analogWriteFreq(200);

  performFades();
  wdt_reset(); // Watchdog resetten

  setup_wifi();
  client.setServer(MQTT_HOST, MQTT_PORT);
  client.setCallback(callback);
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID,WIFI_PASSWORD);
  wdt_reset(); // Watchdock resetten

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}


void performFades() {
  performFade(LED_AKZENT,AKZENT);
  performFade(LED_SCHRANK1,SCHRANK1);
  performFade(LED_SCHRANK2,SCHRANK2);
}

void performFade(int pin, LedFade &fade) {

  unsigned long now = millis();

  int oldValue=fade.helligkeit_ist;

  if (fade.helligkeit_ist!=fade.helligkeit_soll) {
    double progress=((double) now-fade.zeit_start)/(fade.zeit_ende-fade.zeit_start);
    if (progress>1) {
      fade.helligkeit_ist=fade.helligkeit_soll;
    } else {
      double ist=fade.helligkeit_start+(fade.helligkeit_soll-fade.helligkeit_start)*progress;
      fade.helligkeit_ist=ist;
    }
    delay(10);
  }

  analogWrite(pin, fade.helligkeit_ist);

  if(now>nextStatusUpdate) {
    nextStatusUpdate=now+60000;
    sendStatus();
  }
  
}

void fadeTo(LedFade &fade, int soll) {
  if (fade.helligkeit_soll==soll) return;
  fade.helligkeit_start=fade.helligkeit_ist;
  fade.helligkeit_soll=soll;
  unsigned long now = millis();
  fade.zeit_start=now;
  if (fade.helligkeit_ist<fade.helligkeit_soll) fade.zeit_ende=now+FADE_UP_SPEED_MS;
  else fade.zeit_ende=now+FADE_DOWN_SPEED_MS;
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD)) {
      Serial.println("connected");
      if (client.subscribe(MQTT_TOPIC,1)) {
        Serial.print("Subscribed to ");
        Serial.println(MQTT_TOPIC);
      }
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void loop() {
  wdt_reset(); // Watchdog resetten

  fadeTo(AKZENT,statusAkzent==0?0:helligkeitZielAkzent);

  if (digitalRead(BTN_SCHRANK1)==BUTTON_PRESSED_VALUE) {
    fadeTo(SCHRANK1,statusSchrank==0?0:helligkeitZielSchrank);
  } else {
    fadeTo(SCHRANK1,HELLIGKEIT_SCHRANK_OFFEN);
  }
  if (digitalRead(BTN_SCHRANK2)==BUTTON_PRESSED_VALUE) {
    fadeTo(SCHRANK2,statusSchrank==0?0:helligkeitZielSchrank);
  } else {
    fadeTo(SCHRANK2,HELLIGKEIT_SCHRANK_OFFEN);
  }

  performFades();

  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}
