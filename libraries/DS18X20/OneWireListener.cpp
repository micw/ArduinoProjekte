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
	this->ds18x20Count=0;
	this->ds18x20=0;
}

void OneWireListener::setDS18x20TemperatureCallBack(DS18x20TemperatureCallBack callback)
{
	this->ds18x20Callback=callback;
}

int OneWireListener::initialize()
{
	int sensorCount=0;
	if (ds18x20Callback)
	{
		Serial.print("Initializing OneWire for DS18X12 on pin ");
		Serial.println(pin);
		ds18x20=new DS18X20_List(pin);
		ds18x20->update();
		Serial.print("  ");
		Serial.print(ds18x20->count);
		Serial.println(" sensors found:");
		sensorCount+=ds18x20->count;
	    for (int ds18x20Num=0;ds18x20Num<ds18x20->count;ds18x20Num++)
	    {
			Serial.print("    ");
			Serial.print(ds18x20->get_id(ds18x20Num),HEX);
			Serial.println();
	    }
	}
	return sensorCount;
}

void OneWireListener::readValues()
{
	ds18x20->update();
    for (int ds18x20Num=0;ds18x20Num<ds18x20->count;ds18x20Num++)
    {
      if (!ds18x20->has_temp(ds18x20Num)) continue; // Invalid!
      unsigned int sensorId=ds18x20->get_id(ds18x20Num);
      float temperature=ds18x20->get_temp(ds18x20Num);
/*      Serial.print(ds18x20Num);
      Serial.print(" ");
      Serial.print(sensorId);
      Serial.print(" ");
      Serial.print(temperature);
      Serial.println();*/
      ds18x20Callback(ds18x20Num,sensorId,temperature);
    }
}
