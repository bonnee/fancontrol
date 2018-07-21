#include "temp.h"

void Temp::setup()
{
	sensor.setup(pin);
	wait_start = millis();
}

bool Temp::loop()
{
	if (millis() - wait_start >= (unsigned int)sensor.getMinimumSamplingPeriod())
	{
		wait_start = millis();

		temp = sensor.getTemperature();
		if (isnan(temp))
		{
			// If there's an error in the sensor, wait 5 seconds to let the communication reset
			wait_start += 5000;
			sensor.setup(pin);
		}
		return true;
	}
	return false;
}

double Temp::get_temp()
{
	return temp;
}