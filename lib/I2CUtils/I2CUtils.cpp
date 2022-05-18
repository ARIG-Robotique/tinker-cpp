/*
 * I2CUtils.cpp
 *
 *  Created on: 14 avr. 2013
 *      Author: mythril
 */

#include "I2CUtils.h"

#include <Wire.h>

I2CUtils::I2CUtils() {
}

byte I2CUtils::scan() {
#if defined(DEBUG)
	Serial.println(" * I2C Scanning...");
#endif
	byte nDevices = 0;
	for (byte address = 1; address < 127; address++) {
		// The i2c_scanner uses the return value of the Write.endTransmisstion to see if
		// a device did acknowledge to the address.
		Wire.beginTransmission(address);
		byte retCode = Wire.endTransmission();
		if (this->isOk(retCode)) {
#if defined(DEBUG)
			Serial.print("    -> Device found at address 0x");
			if (address < 16)
				Serial.print("0");
			Serial.print(address, HEX);
			Serial.println(" !");
#endif
			nDevices++;
		} else if (retCode == I2C_OTHER_ERROR) {
#if defined(DEBUG)
			Serial.print("    -> Unknow error at address 0x");
			if (address < 16)
				Serial.print("0");
			Serial.println(address, HEX);
#endif
		}
	}

	return nDevices;
}

void I2CUtils::fastSpeed(boolean fast) {
	if (!fast) {
		TWBR = ((F_CPU / 100000) - 16) / 2;
	} else {
		TWBR = ((F_CPU / 400000) - 16) / 2;
	}

#if defined(DEBUG)
	Serial.print(" * Speed I2C (0 = 100, 1 = 400) : ");
	Serial.println(fast, DEC);
#endif
}

void I2CUtils::pullup(boolean activate) {
	// NOT YET IMPLEMENTED
}

#if defined(DEBUG)
/*
 * Fonction permettant d'afficher le résultat d'une commande I2C
 */
void I2CUtils::printReturnCode(byte code) {
	switch (code) {
	case I2C_ACK: // OK
		Serial.println("[I2C - OK] Success");
		break;
	case I2C_DATA_TOO_LONG:
		Serial.println("[I2C - ERROR] Data Too long");
		break;
	case I2C_NACK_BAD_ADDRESS:
		Serial.println("[I2C - ERROR] Bad address");
		break;
	case I2C_NACK_BAD_DATA:
		Serial.println("[I2C - ERROR] Bad data");
		break;
	case I2C_OTHER_ERROR:
		Serial.println("[I2C - ERROR] Unknown error");
		break;
	}
}
#endif

/*
 * Contrôle si le code de retour est une erreur
 */
boolean I2CUtils::isError(byte code) {
	return code != I2C_ACK;
}

/*
 * Contrôle si le code de retour est OK
 */
boolean I2CUtils::isOk(byte code) {
	return code == I2C_ACK;
}

I2CUtils i2cUtils = I2CUtils();
