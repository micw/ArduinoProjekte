/* Sweep
 by BARRAGAN <http://barraganstudio.com>
 This example code is in the public domain.

 modified 8 Nov 2013
 by Scott Fitzgerald
 http://www.arduino.cc/en/Tutorial/Sweep
*/

#include <Servo.h>

Servo servoDreh;
Servo servoHochRunter;

int DREH_UBER_BECHER=170;
int HOCHRUNTER_IN_BECHER=140;
int HOCHRUNTER_UBER_BECHER1=110;
int HOCHRUNTER_UBER_BECHER2=90;

int DREH_VOR_GEBLAESE=35;
int HOCHRUNTER_VOR_GEBLAESE1=90;
int HOCHRUNTER_VOR_GEBLAESE2=65;


void moveTo(Servo servo, int dest, int delayMs) {
  int ist=servo.read();
  int dir=(ist>dest)?-1:1;
  while (ist!=dest) {
    ist+=dir;
    servo.write(ist);
    delay(delayMs);
  }
}

void setup() {
  pinMode(3, OUTPUT);
  digitalWrite(3, 0);

  servoHochRunter.attach(5);
  servoHochRunter.write(90);
  delay(1000);

  servoDreh.attach(4);
  servoDreh.write(DREH_UBER_BECHER);
  delay(1000);
}

void loop() {
  moveTo(servoDreh,DREH_UBER_BECHER,3);

  // Eintauchen
  moveTo(servoHochRunter,HOCHRUNTER_IN_BECHER,10);
  delay(1000);

  // Abtropfen
  moveTo(servoHochRunter,HOCHRUNTER_UBER_BECHER2,10);
  delay(1000);

  // Abschütteln
  for (int i=0;i<3;i++) {
    servoHochRunter.write(HOCHRUNTER_UBER_BECHER1);
    delay(20);
    moveTo(servoHochRunter,HOCHRUNTER_UBER_BECHER2,10);
  }
  delay(500);

  // Gebläse
  moveTo(servoHochRunter,HOCHRUNTER_VOR_GEBLAESE1,10);
  digitalWrite(3, 1); // An
  moveTo(servoDreh,DREH_VOR_GEBLAESE,10);
  // Hoch/Runter
  for (int i=0;i<3;i++) {
    moveTo(servoHochRunter,HOCHRUNTER_VOR_GEBLAESE2,15);
    moveTo(servoHochRunter,HOCHRUNTER_VOR_GEBLAESE1,15);
  }
  digitalWrite(3, 0); // Aus
}

