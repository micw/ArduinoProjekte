#ifndef DS18X20_HH
#define DS18X20_HH

#include "OneWire.h"

#define MAX_DS18X20 5

void ds18x20_print_temperature(float t);

struct DS18X20
{
  unsigned char data[12];
  unsigned char addr[8];
  unsigned char type;
  float temp;
  bool tempValid;
  unsigned int id;
};

class DS18X20_List : public OneWire
{
  unsigned long last_convert;
  DS18X20 ds18x20[MAX_DS18X20];
public:
  int count;     // number of available sensors. May need to adjust MAX_DS18X20 (see above)

  DS18X20_List(unsigned char pin);// searches for available DS18X20 sensors 
  
  void update(); // calls read() and convert(), but only if the last conversion 
                 // has finished. You can call update() as often as you like to 
                 // keep the measured temperatures uptodate. 

  void convert();// issues a "convert" on all sensors. Sensors will measure
                 // temperature simultaneously and hence need to be 
                 // powered by a dedicated 5V line. Otherwise, the power 
                 // provided by the data line will not suffice.

  void read(unsigned char no); // read single sensor
  void read();   // transfers the converted temperature values from all sensors
                 // into arduino. Need to call convert() before (and wait for 
                 // end of conversion). Better use update() instead of convert()
                 // and read().

  float get_temp(unsigned char i);      // get temp from i-th sensor
  bool has_temp(unsigned char i);      // check the presence of a (valid) temperature) for the i-th sensor

  float get_temp_by_id(unsigned int id); // get temp for sensor with ID 'id'

  unsigned int get_id(unsigned char i); // get ID of i-th
};

// ------------------------------------------------------------------
#endif


