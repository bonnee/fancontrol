#include "config.h"

void Config::setup()
{
	// Check if saved bytes have the same "version" and loads them. Otherwise it will load the default values.
	if (EEPROM.read(CONFIG_START + 0) == CONFIG_VERSION[0] &&
		EEPROM.read(CONFIG_START + 1) == CONFIG_VERSION[1] &&
		EEPROM.read(CONFIG_START + 2) == CONFIG_VERSION[2])
		for (unsigned int t = 0; t < sizeof(storage); t++)
			*((char *)&storage + t) = EEPROM.read(CONFIG_START + t);
}

void Config::write()
{
	for (unsigned int t = 0; t < sizeof(storage); t++)
		EEPROM.update(CONFIG_START + t, *((char *)&storage + t));
}

double Config::getTarget()
{
	return storage.target;
}

void Config::setTarget(double new_target)
{
	storage.target = new_target;
}