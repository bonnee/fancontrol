#include "temp.h"

Temp::Temp(int attachTo)
{
	pin = attachTo;
	wait_start = 0;
}

void Temp::setup()
{
	sensor.setup(pin);
}

int Temp::loop(unsigned long now)
{
	if (sensor.getStatus() == 0)
	{
		wait_start = now;
		double temp = sensor.getTemperature();

		/* Sometimes I get a checksum error from my DHT-22.
         To avoid exceptions I check if the reported temp is a number.
         This should work only with the "getStatus() == 0" above, but it gave me errors anyway, So I doublecheck */
		if (!isnan(temp))
		{
			/*dtemp = round(temp * 2.0) / 2.0;
			ctemp = round(temp);*/
			return temp;
		}
	}
	else
	{
		// If there's an error in the sensor, wait 5 seconds to let the communication reset
		wait_start += 5000;
		sensor.setup(pin);
	}
	return -1;
}