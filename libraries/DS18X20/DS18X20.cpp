/*
By R. Plag, December 2009

Change by M.Wyraz: Changed name from DS18S20 to DS18X20 and added Support for DS18B20

This file implements the class DS18X20_List which provides easy access
to a set of DS1820/DS18S20DS18S20/DS18B20 1-Wire temperature sensors. The class is based
on the OneWire library by Jim Studt and Tom Pollard (see OneWire.cpp). 

Usage is explained in ds18s20_demo.pde.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <DS18X20.h>
#include <Arduino.h>


void ds18x20_print_temperature(float t) 
{ 
  int Tc_100=t*100; 
  int Whole = abs(Tc_100 / 100); // Vorzeichenlosen Werte erzeugen 
  int Fract = abs(Tc_100 % 100); // Vorzeichenlosen Werte erzeugen 
  if (abs(Tc_100) != Tc_100)     // Bei negativem Übergabewert Minuszeichen für Ausgabe setzen 
    Serial.print("-"); 
  Serial.print(Whole); 
  Serial.print("."); 
  if (Fract < 10)                // e.g. t=20.05 => Fract=5 => '0' missing
    Serial.print("0");
  Serial.print(Fract); 
  } 
// ------------------------------------------------------------------
// ------------------------------------------------------------------
// ------------------------------------------------------------------
// ------------------------------------------------------------------
DS18X20_List::DS18X20_List(unsigned char p) : OneWire(p)
{
  last_convert=0;
  count=0;
  int max_loops=255;
  while ((count<MAX_DS18X20) && (OneWire::search(ds18x20[count].addr) && (max_loops--))) 
    {
    if ( OneWire::crc8( ds18x20[count].addr, 7) != ds18x20[count].addr[7]) 
      {
      continue; //Serial.print("CRC is not valid!\n");
      }
  
    if ( ds18x20[count].addr[0] != 0x10 && ds18x20[count].addr[0] != 0x28) 
      {
      continue; //Serial.print("Device is not a DS18X20 family device.\n");
      }

	ds18x20[count].type=ds18x20[count].addr[0];
    ds18x20[count].id=(ds18x20[count].addr[1]<<8)+ds18x20[count].addr[7];
    count++;
    }
  OneWire::reset_search();
  convert();
}
// ------------------------------------------------------------------
void DS18X20_List::update()
{
  unsigned long cur=millis(); // 32bit
  unsigned long elapsed=0;
  if (cur<last_convert)
    {
    elapsed=(0xffffffff-last_convert);
    elapsed+=cur;
    }
  else
    elapsed=cur-last_convert;
  
  if (elapsed<1000) return;

  read();
  convert();
}
// ------------------------------------------------------------------
void DS18X20_List::convert()
{
  for (int i=0;i<count;i++)
    {
    OneWire::reset();
    OneWire::select(ds18x20[i].addr);
    OneWire::write(0x44,0);    
    }
  last_convert=millis();
}
// ------------------------------------------------------------------
void DS18X20_List::read()
{
  for (int i=0;i<count;i++) read(i);
}
// ------------------------------------------------------------------
// read scratchpad and reconstruct hi precision temperature value in degrees celsius
void DS18X20_List::read(unsigned char no)
    {
    if (no>=count) return;

    ds18x20[no].tempValid=false;
    ds18x20[no].temp=-299;

    int present = OneWire::reset();
    OneWire::select(ds18x20[no].addr);    
    OneWire::write(0xBE);         // read scratchpad
    if (present==0) {
      return;
      }

    unsigned char *d=ds18x20[no].data;
    
    bool allBytesFF=true;
    for ( int i=0;i<9;i++) { // read all 9 bytes from scratchpad
      d[i] = OneWire::read();
	  if (d[i]!=0xFF)  allBytesFF=false;
	}
	if ( allBytesFF) return; // Sensor data invalid (all 0xFF)
      
    if (ds18x20[no].type==0x10) { // Type 0x10 (DS1820/DS18S20)
		
		if( d[1] == 0 )
		{
		  ds18x20[no].temp = (int) d[0] >> 1;
		} else {
		  ds18x20[no].temp = -1 * (int) (0x100-d[0]) >> 1;
		}
		ds18x20[no].temp -= 0.25;

		// improve resolution of temperature
		float hi_precision = (int) d[7] - (int) d[6];
		hi_precision = hi_precision / (int) d[7];
		ds18x20[no].temp += hi_precision;
	}
    else if (ds18x20[no].type==0x28) { // Type 0x28 (DS18B20)
		// Resolution is 12 Bits, no "magic" to achieve higher resolution
		int value=(int) ((d[1] << 8) | (d[0]));
		if( d[1] & 0x80 != 0 )
		{
		  value = -1 * (int) (0x10000-value);
		}
		ds18x20[no].temp=value/16.0f; // 4 Bits fraction
	}
	// Temperature is valid if it's in the Sensor's range and if it's not 85.0000 (this is the power on value)
    ds18x20[no].tempValid=(ds18x20[no].temp>=-55 && ds18x20[no].temp<=125) && (ds18x20[no].temp!=85.0f);
}
// ------------------------------------------------------------------
bool DS18X20_List::has_temp(unsigned char i)
{
  if (i>=count) return false;
  return ds18x20[i].tempValid;
}
// ------------------------------------------------------------------
float DS18X20_List::get_temp(unsigned char i)
{
  if (i>=count) return -299;
  return ds18x20[i].temp;
}
// ------------------------------------------------------------------
float DS18X20_List::get_temp_by_id(unsigned int id)
{
  for (int i=0;i<count;i++)
    if (ds18x20[i].id==id)
      return ds18x20[i].temp;
  return 85.0;
}
// ------------------------------------------------------------------
unsigned int DS18X20_List::get_id(unsigned char no)
{
  if (no>=count) return 0;
  return ds18x20[no].id;
}
// ------------------------------------------------------------------



