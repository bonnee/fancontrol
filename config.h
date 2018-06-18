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
} storage = { // Default values
	CONFIG_VERSION,
	40};

class Config
{
  public:
	Config();
	double getTarget();
	void setTarget(double new_target);
	void setup();
	void write();
};
#endif