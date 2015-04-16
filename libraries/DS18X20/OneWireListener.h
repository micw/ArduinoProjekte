/*
 * OneWireListener.h
 *
 *  Created on: 10.10.2013
 *      Author: mwyraz
 */

#ifndef ONEWIRELISTENER_H_
#define ONEWIRELISTENER_H_

#if ARDUINO >= 100
#include "Arduino.h"       // for delayMicroseconds, digitalPinToBitMask, etc
#else
#include "WProgram.h"      // for delayMicroseconds
#include "pins_arduino.h"  // for digitalPinToBitMask, etc
#endif

#include "DS18X20.h"

// callback(int sensorNumber, unsigned int sensorId, float temperature)
typedef void (*DS18x20TemperatureCallBack)(int, unsigned int, float);

class OneWireListener {
public:
	OneWireListener(int pin);
	void setDS18x20TemperatureCallBack(DS18x20TemperatureCallBack callback);
	int initialize();
	void readValues();
private:
	int pin;
	DS18x20TemperatureCallBack ds18x20Callback;
	int ds18x20Count;
	DS18X20_List *ds18x20;
};

#endif /* ONEWIRELISTENER_H_ */
