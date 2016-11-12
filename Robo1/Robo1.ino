/* Sweep
 by BARRAGAN <http://barraganstudio.com>
 This example code is in the public domain.

 modified 8 Nov 2013
 by Scott Fitzgerald
 http://www.arduino.cc/en/Tutorial/Sweep
*/

#include <Servo.h>

int[] tPos1=[65,100,180];
int[] tPos2=[65,100,180];

Servo gelenk1;
Servo gelenk2;
Servo gelenk3;

float pos1=10;
float pos2=60;
float pos3=60;

void setup() {
  Serial.begin(115200);
  gelenk1.attach(3);
  gelenk2.attach(4);
  gelenk3.attach(5);
  gelenk1.write(pos1);
  gelenk2.write(pos2);
  gelenk3.write(pos3);
}

void loop() {
  if(Serial.available())
  {
    String s=Serial.readString();
    if (s.length()>=2) {
      if (s.substring(0,2).equals("1+")) pos1+=5;
      if (s.substring(0,2).equals("1-")) pos1-=5;
      if (s.substring(0,2).equals("2+")) pos2+=5;
      if (s.substring(0,2).equals("2-")) pos2-=5;
      if (s.substring(0,2).equals("3+")) pos3+=5;
      if (s.substring(0,2).equals("3-")) pos3-=5;
    }
    if (s.length()>=3) {
      if (s.substring(0,3).equals("1++")) pos1+=25;
      if (s.substring(0,3).equals("1--")) pos1-=25;
      if (s.substring(0,3).equals("2++")) pos2+=25;
      if (s.substring(0,3).equals("2--")) pos2-=25;
      if (s.substring(0,3).equals("3++")) pos3+=25;
      if (s.substring(0,3).equals("3--")) pos3-=25;
    }
    Serial.println("1 = "+String(pos1));
    Serial.println("2 = "+String(pos2));
    Serial.println("3 = "+String(pos3));
  }
  gelenk1.write(pos1);
  gelenk2.write(pos2);
  gelenk3.write(pos3);
}

