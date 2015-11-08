/*
 * LedCodes.h
 *
 *  Created on: 10.10.2013
 *      Author: mwyraz
 */

#ifndef LEDCODES_H_
#define LEDCODES_H_

#if ARDUINO >= 100
#include "Arduino.h"       // for delayMicroseconds, digitalPinToBitMask, etc
#else
#include "WProgram.h"      // for delayMicroseconds
#include "pins_arduino.h"  // for digitalPinToBitMask, etc
#endif

class LedCodes {
public:
	static void number(int ledPin, int number);
};

#endif /* LEDCODES_H_ */
