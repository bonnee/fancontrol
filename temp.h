#ifndef TEMP_H
#include <Arduino.h>
#include <DHT.h> // https://github.com/markruys/arduino-DHT

class Temp
{
	DHT sensor;
	byte pin;
	double temp;

	unsigned long wait_start;

  public:
	Temp(byte attach_to) : pin(attach_to) {}
	void setup();
	bool loop();

	double get_temp();
};
#endif