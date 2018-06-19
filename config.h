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
	// Shady hack that exposes target as a read-only variable.
	// Needed to pass target as reference in PID declaration.
	const double &target;

	Config() : target(storage.target) {}

	void setTarget(double new_target);
	void setup();
	void write();
};
#endif