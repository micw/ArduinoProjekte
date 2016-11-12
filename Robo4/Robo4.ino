/* Sweep
 by BARRAGAN <http://barraganstudio.com>
 This example code is in the public domain.

 modified 8 Nov 2013
 by Scott Fitzgerald
 http://www.arduino.cc/en/Tutorial/Sweep
*/

#include <Servo.h>

int tPos0[]={10,60,60};
int tPos1[]={65,100,180};
int tPos2[]={0,180,85};
int tPos3[]={0,150,85};

Servo gelenk1;
Servo gelenk2;
Servo gelenk3;

float pos1=tPos0[0];
float pos2=tPos0[1];
float pos3=tPos0[2];

float vmax=0.7;

void setup() {
  Serial.begin(115200);
  gelenk1.attach(3);
  gelenk2.attach(4);
  gelenk3.attach(5);
  gelenk1.write(pos1);
  gelenk2.write(pos2);
  gelenk3.write(pos3);
}

void gotoPos(int tPos[]) {
  float delta1=tPos[0]-pos1;
  float delta2=tPos[1]-pos2;
  float delta3=tPos[2]-pos3;
  float deltaMax=max(abs(delta1),max(abs(delta2),abs(delta3)));
  float stepCount=deltaMax/vmax;
  float speed1=delta1/stepCount;
  float speed2=delta2/stepCount;
  float speed3=delta3/stepCount;

  for (float s=0;s<stepCount;s++) {
    pos1+=speed1;
    pos2+=speed2;
    pos3+=speed3;
    gelenk1.write(pos1);
    gelenk2.write(pos2);
    gelenk3.write(pos3);
    delay(5);
  }
  pos1=tPos[0];
  pos2=tPos[1];
  pos3=tPos[2];
  gelenk1.write(pos1);
  gelenk2.write(pos2);
  gelenk3.write(pos3);
  delay(5);
  
}

void loop() {
  gotoPos(tPos1);
  delay(2500);
  gotoPos(tPos2);
  gotoPos(tPos3);
  delay(1000);
}

