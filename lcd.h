#ifndef LCD_H
#include <string.h>
#include <LedControl.h> //https://github.com/giech/LedControl

#define CASCADE 1
#define LEN 7

class LCD
{
	LedControl lc = LedControl(0, 0, 0, 0);

	/**
    	Writes to the LCD

    	@param buf Input string.
		@param dots position of the dots to illuminate.
	*/
	void write(const char *buf);

  public:
	LCD(byte din, byte clk, byte cs);

	void setup(int intensity);

	/**
    	Updates the LCD with updated parameters

    	@param temp current temperature.
    	@param target target temperature.
		@param duty the current fan speed (in %).
    	@param t_mode wether the display should be in target mode 
		(Displaying the target) or normal mode (displaying temp 
		and duty)
	*/
	void update(double temp, double target, double duty, bool t_mode);
};
#endif