#ifndef TEMP_H
#include <DHT.h> // https://github.com/markruys/arduino-DHT

class Temp
{
	DHT sensor;
	byte pin;

	unsigned long wait_start;

  public:
	Temp(byte attachTo);
	void setup();
	int loop(unsigned long now);
};
#endif