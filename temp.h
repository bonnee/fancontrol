#ifndef TEMP_H
#include <DHT.h> // https://github.com/markruys/arduino-DHT

class Temp
{
	DHT sensor;
	int pin;

	unsigned long wait_start;
	bool waiting;

  public:
	Temp(int attachTo);
	void setup();
	int loop(unsigned long now);
};
#endif