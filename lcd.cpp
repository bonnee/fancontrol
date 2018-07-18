#include "lcd.h"

LCD::LCD(byte din, byte clk, byte cs)
{
	lc = LedControl(din, clk, cs, CASCADE);
}

void LCD::setup(int brightness)
{
	lc.clearDisplay(0);
	lc.shutdown(0, false);
	lc.setIntensity(0, brightness);

	write("Fan Ctrl");
}

void LCD::write(const char *buf)
{
	int i = 0;
	while (i < strlen(buf))
	{
		//If not last then you can do i + 1
		if (i < strlen(buf) - 1 && buf[i + 1] == '.')
		{
			lc.setChar(0, i, buf[i], true);
			i += 2;
		}
		else
		{
			lc.setChar(0, i, buf[i], false);
			i++;
		}
	}
}

void LCD::update(double temp, double target, double duty, bool t_mode)
{
	char buf[LEN * 2];

	if (t_mode)
	{
		sprintf(buf, "Set %4.1fC", target);
	}
	else
	{
		if (duty == 0)
			sprintf(buf, "%4.1fC Off", temp);
		else
			sprintf(buf, "%4.1fC %3u", temp, (int)duty);
	}

	write(buf);
}
