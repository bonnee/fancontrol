#ifndef BUTTON_H
#include <Arduino.h>

class Button
{
	const byte pin;
	int state;
	unsigned long buttonDownMs;

  public:
	Button(byte attachTo) : pin(attachTo)
	{
	}

	void setup();
	bool loop();
};
#endif