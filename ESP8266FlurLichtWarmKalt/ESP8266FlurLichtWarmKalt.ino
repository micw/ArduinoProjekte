/**
 * D0 - output ok
 * D5 - output ok
 * D6 - output ok
 * D7 - output ok
 *
 * D3 - output ok
 * D4 - output ok, internal LED is inverse
 *
 * D3 - pullup input ok, bootet nicht wenn LOW
 * D4 - pullup input ok, bootet nicht wenn LOW
 * 
 * D1 - pullup input ok
 * D2 - pullup input ok
*/

#define SMARTIO_DEBUG 1
#include <smartio.h>
#include <smartio_mqtt.h>
#include <private_wifi.h>
#include <ext_RemoteReceiver.h>

Bus bus;
//Button b1(&bus,D1,1);
//Button b2(&bus,D2,2);
DoubleButton b(&bus,D1,D2,1,2,3);
DimmableCWLed led1(D0,D5);
DimmableCWLed led2(D6,D7);

Mqtt mqtt(WIFI_SSID,WIFI_PASSWORD,MQTT_HOST,MQTT_PORT,MQTT_USERNAME,MQTT_PASSWORD,"light/desktop/alive");

//RemoteReceiver elroSwitch;

unsigned long num=0;

void callback(unsigned long chan, unsigned int state) {
  interrupts();
  Serial.print(num++);
  Serial.print(" ");
  Serial.print(chan);
  Serial.print(" ");
  Serial.print(state);
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  
  analogWriteFreq(400);

  pinMode(D3,INPUT);
  RemoteReceiver::init(D3,2,callback);
  RemoteReceiver::enable();

  bus.debug=true;

  mqtt.connect("light/desktop/light1/",led1);

/*
  mqtt.property(
    "light/desktop/light1/state/get",[]() { return led1.get_state() },
    "light/desktop/light1/state/set",[](int value) { led1.set_state() });*/
/*
  mqtt.watch("light/desktop/light1/state/get",[]() { return led1.get_state()?1:0; });
  mqtt.on("light/desktop/light1/state/set",[](float value) { led1.set_state(value!=0); });
  */

  /*
  mqtt.watch("light/desktop/light1/dim/get",[]() { return led1.get_dim_value(); });
  mqtt.on("light/desktop/light1/dim/set",[](int value) { led1.set_dim_value(value); });

  mqtt.watch("light/desktop/light1/fade/get",[]() { return (int) (((led1.get_fade_value())/1023.0*347)+153); });
  mqtt.on("light/desktop/light1/fade/set",[](int value) { led1.set_fade_value((int) ((value-153)/347.0*1023)); });

  mqtt.watch("light/desktop/light2/state/get",[]() { return led2.get_state()?1:0; });
  mqtt.on("light/desktop/light2/state/set",[](int value) { led2.set_state(value!=0); });
  
  mqtt.watch("light/desktop/light2/dim/get",[]() { return led2.get_dim_value(); });
  mqtt.on("light/desktop/light2/dim/set",[](int value) { led2.set_dim_value(value); });

  mqtt.watch("light/desktop/light2/fade/get",[]() { return (int) (((led2.get_fade_value())/1023.0*347)+153); });
  mqtt.on("light/desktop/light2/fade/set",[](int value) { led2.set_fade_value((int) ((value-153)/347.0*1023)); });
  */

  bus.on(Button::TYPE,1,Button::ACTION_KLICK,[]() { led1.toggle(); });
  bus.on(Button::TYPE,1,Button::ACTION_HOLD,[]() { led1.dim_start(); });
  bus.on(Button::TYPE,1,Button::ACTION_RELEASE,[]() { led1.dim_stop(); });

  bus.on(Button::TYPE,2,Button::ACTION_KLICK,[]() { led2.toggle(); });
  bus.on(Button::TYPE,2,Button::ACTION_HOLD,[]() { led2.dim_start(); });
  bus.on(Button::TYPE,2,Button::ACTION_RELEASE,[]() { led2.dim_stop(); });

  bus.on(Button::TYPE,3,Button::ACTION_HOLD,[]() { led1.fade_start(); led2.fade_start(); });
  bus.on(Button::TYPE,3,Button::ACTION_RELEASE,[]() { led1.fade_stop(); led2.fade_stop(); });
  
  /*
  pinMode(D1,INPUT_PULLUP);
  pinMode(D2,INPUT);
  */
}

bool state=false;

void loop() {
  MainLoop::loop();
  /*
  bool newstate=digitalRead(D3);
  if (state!=newstate) {
    Serial.printf("%i -> %i",state,newstate\n);
    newstate=state;
  }
  */
  /*
  Serial.print(analogRead(D1));
  Serial.print("   ");
  Serial.println(analogRead(D2));
  delay(10);
  */
}
