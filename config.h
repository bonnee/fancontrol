#ifndef CONFIG_H
#include <EEPROM.h>

// Change this if you want your current settings to be overwritten.
#define CONFIG_VERSION "f01"
// Where to store config data in EEPROM
#define CONFIG_START 32

// Settings
struct StoreStruct
{
	char version[4];
	double target;
};

class Config
{
	StoreStruct storage = {
		CONFIG_VERSION,
		40};

  public:
	Config() {}

	void set_target(double new_target);
	double get_target();

	void setup();
	void flush();
};
#endif