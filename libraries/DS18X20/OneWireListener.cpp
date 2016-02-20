/*
 * OneWireListener.cpp
 *
 *  Created on: 10.10.2013
 *      Author: mwyraz
 */

#include "OneWireListener.h"

OneWireListener::OneWireListener(int pin)
{
	this->pin=pin;
	this->ds18x20Callback=0;
	this->oneWire=0;
	this->dallasTemperature=0;
}

void OneWireListener::setDS18x20TemperatureCallBack(DS18x20TemperatureCallBack callback)
{
	this->ds18x20Callback=callback;
}

int OneWireListener::initialize()
{
	if (ds18x20Callback)
	{
		Serial.print("Initializing OneWire for DS18X12 on pin ");
		Serial.println(pin);

		oneWire=new OneWire(pin);
		dallasTemperature=new DallasTemperature(oneWire);
		dallasTemperature->begin();
		dallasTemperature->setResolution(12);
		dallasTemperature->setWaitForConversion(false);
		int sensorCount=dallasTemperature->getDeviceCount();
		Serial.print("  ");
		Serial.print(sensorCount);
		Serial.println(" sensors found:");

	    for (int ds18x20Num=0;ds18x20Num<sensorCount;ds18x20Num++)
	    {
			Serial.print("    ");
	    	if (dallasTemperature->getAddress(addr,ds18x20Num)) {
				Serial.print((addr[1]<<8)+addr[7],HEX);
			} else {
				Serial.print("Unknown address");
			}
			Serial.println();
	    }
		return sensorCount;
	}
	return 0;
}

void OneWireListener::readValues()
{
	dallasTemperature->requestTemperatures();
	int sensorCount=dallasTemperature->getDeviceCount();
    for (int ds18x20Num=0;ds18x20Num<sensorCount;ds18x20Num++)
    {
    	if (!dallasTemperature->getAddress(addr,ds18x20Num)) {
    	// Sensor unreachable
    		continue;
    	}
    	float temperature = dallasTemperature->getTempC(addr);
    	if (!(temperature>=-55 && temperature<=125) && (temperature!=85.0f)) {
    		// Temperature is only valid if it's in the Sensor's range and if it's not 85.0000 (this is the power on value)
    		continue;
    	}
      	unsigned int sensorId=(addr[1]<<8)+addr[7];
      	/*
		Serial.print(ds18x20Num);
		Serial.print(" ");
		Serial.print(sensorId);
		Serial.print(" ");
		Serial.print(temperature);
		Serial.println();
		*/
		ds18x20Callback(ds18x20Num,sensorId,temperature);
    }
}
