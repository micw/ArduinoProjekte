/*
 * LedCodes.cpp
 *
 *  Created on: 10.10.2013
 *      Author: mwyraz
 */

#include "LedCodes.h"

void LedCodes::number(int ledPin, int number)
{
  digitalWrite(ledPin, true);
  delay(1000);
  digitalWrite(ledPin, false);
  delay(1000);
  for (int i=0;i<number;i++)
  {
	  if (i>0) delay(500);
	  digitalWrite(ledPin, true);
	  delay(200);
	  digitalWrite(ledPin, false);
  }
  delay(1000);
  digitalWrite(ledPin, true);
  delay(1000);
  digitalWrite(ledPin, false);
}

