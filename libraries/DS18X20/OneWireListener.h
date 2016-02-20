/*
 * OneWireListener.h
 *
 *  Created on: 10.10.2013
 *      Author: mwyraz
 */

#ifndef ONEWIRELISTENER_H_
#define ONEWIRELISTENER_H_

#include "Arduino.h"       // for delayMicroseconds, digitalPinToBitMask, etc

#include <OneWire.h>
#include <DallasTemperature.h>

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
	OneWire *oneWire;
	DallasTemperature *dallasTemperature;
	DeviceAddress addr;
};

#endif /* ONEWIRELISTENER_H_ */
