/*
 * I2CUtils.h
 *
 *  Created on: 14 avr. 2013
 *      Author: mythril
 */

#ifndef I2CUTILS_H_
#define I2CUTILS_H_

#include <Arduino.h>
#include <Wire.h>
#include <config.h>

#define I2C_ACK					0
#define I2C_DATA_TOO_LONG		1
#define I2C_NACK_BAD_ADDRESS	2
#define I2C_NACK_BAD_DATA		3
#define I2C_OTHER_ERROR			4

#define cbi(sfr, bit)   (_SFR_BYTE(sfr) &= ~_BV(bit))
#define sbi(sfr, bit)   (_SFR_BYTE(sfr) |= _BV(bit))

class I2CUtils {
public:
	I2CUtils();

	byte scan();

	void fastSpeed(boolean fast);
	void pullup(boolean activate);

	boolean isError(byte code);
	boolean isOk(byte code);

#if defined(DEBUG)
	void printReturnCode(byte code);
#endif
};

extern I2CUtils i2cUtils;

#endif /* I2CUTILS_H_ */
