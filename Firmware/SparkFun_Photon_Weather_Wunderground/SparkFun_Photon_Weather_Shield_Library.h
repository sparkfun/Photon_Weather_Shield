/*
 SparkFun Photon Weather Shield Particle Library
 Compiled and Updated for Particle By: Joel Bartlett
 SparkFun Electronics
 Date: August 05, 2015

 This library combines multiple libraries together to make development on the SparkFun
 Photon Weather Shield easier. There are two versions of the Weather Shield, one containing
 the HTU21D Temp and Humidity sensor and one containing the Si7021-A10. Shortages of the HTU21D
 IC combined with contractual obligations led to the two versions existing. Thus, to make it
 easier on you, the end user, we have written this library to automatically detect which version
 you have without the need for you to have to figure it out and pick a corresponding library.
 The MPL3115A2 library was added to avoid the need to import two separate libraries for
 the shield.

 This library is based on the following libraries:

 MPL3115A2 Barometric Pressure Sensor Library
 By: Nathan Seidle
 SparkFun Electronics
 Date: September 24th, 2013
 https://github.com/sparkfun/MPL3115A2_Breakout

 Spark Core HTU21D Temperature / Humidity Sensor Library
 By: Romain MP
 https://github.com/romainmp/HTU21D

 Arduino Si7010 relative humidity + temperature sensor
 Jakub Kaminski, 2014
 https://github.com/teoqba/Si7020

 This Library is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This Library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 For a copy of the GNU General Public License, see
 <http://www.gnu.org/licenses/>.
 */

#include "application.h"//needed for all Particle libraries

#ifndef SparkFun_Photon_Weather_Shield_Library_h
#define SparkFun_Photon_Weather_Shield_Library_h

/****************Si7021 & HTU21D Definitions***************************/

#define ADDRESS      0x40

#define TEMP_MEASURE_HOLD  0xE3
#define HUMD_MEASURE_HOLD  0xE5
#define TEMP_MEASURE_NOHOLD  0xF3
#define HUMD_MEASURE_NOHOLD  0xF5
#define TEMP_PREV   0xE0

#define WRITE_USER_REG  0xE6
#define READ_USER_REG  0xE7
#define SOFT_RESET  0xFE

#define HTRE        0x02
#define _BV(bit) (1 << (bit))

#define CRC_POLY 0x988000 // Shifted Polynomial for CRC check

// Error codes
#define I2C_TIMEOUT 	998
#define BAD_CRC		999

/****************MPL3115A2 Definitions************************************/
#define MPL3115A2_ADDRESS 0x60 // Unshifted 7-bit I2C address for sensor

#define STATUS     0x00
#define OUT_P_MSB  0x01
#define OUT_P_CSB  0x02
#define OUT_P_LSB  0x03
#define OUT_T_MSB  0x04
#define OUT_T_LSB  0x05
#define DR_STATUS  0x06
#define OUT_P_DELTA_MSB  0x07
#define OUT_P_DELTA_CSB  0x08
#define OUT_P_DELTA_LSB  0x09
#define OUT_T_DELTA_MSB  0x0A
#define OUT_T_DELTA_LSB  0x0B
#define WHO_AM_I   0x0C
#define F_STATUS   0x0D
#define F_DATA     0x0E
#define F_SETUP    0x0F
#define TIME_DLY   0x10
#define SYSMOD     0x11
#define INT_SOURCE 0x12
#define PT_DATA_CFG 0x13
#define BAR_IN_MSB 0x14
#define BAR_IN_LSB 0x15
#define P_TGT_MSB  0x16
#define P_TGT_LSB  0x17
#define T_TGT      0x18
#define P_WND_MSB  0x19
#define P_WND_LSB  0x1A
#define T_WND      0x1B
#define P_MIN_MSB  0x1C
#define P_MIN_CSB  0x1D
#define P_MIN_LSB  0x1E
#define T_MIN_MSB  0x1F
#define T_MIN_LSB  0x20
#define P_MAX_MSB  0x21
#define P_MAX_CSB  0x22
#define P_MAX_LSB  0x23
#define T_MAX_MSB  0x24
#define T_MAX_LSB  0x25
#define CTRL_REG1  0x26
#define CTRL_REG2  0x27
#define CTRL_REG3  0x28
#define CTRL_REG4  0x29
#define CTRL_REG5  0x2A
#define OFF_P      0x2B
#define OFF_T      0x2C
#define OFF_H      0x2D

/****************Si7021 & HTU21D Class**************************************/
class Weather
{
public:
	// Constructor
	Weather();

	void  begin();

	// Si7021 & HTU21D Public Functions
	float getRH();
	float readTemp();
	float getTemp();
	float readTempF();
	float getTempF();
	void  heaterOn();
	void  heaterOff();
	void  changeResolution(uint8_t i);
	void  reset();
	uint8_t  checkID();

    //MPL3115A2 Public Functions
	float readAltitude(); // Returns float with meters above sealevel. Ex: 1638.94
	float readAltitudeFt(); // Returns float with feet above sealevel. Ex: 5376.68
	float readPressure(); // Returns float with barometric pressure in Pa. Ex: 83351.25
	float readBaroTemp(); // Returns float with current temperature in Celsius. Ex: 23.37
	float readBaroTempF(); // Returns float with current temperature in Fahrenheit. Ex: 73.96
	void setModeBarometer(); // Puts the sensor into Pascal measurement mode.
	void setModeAltimeter(); // Puts the sensor into altimetery mode.
	void setModeStandby(); // Puts the sensor into Standby mode. Required when changing CTRL1 register.
	void setModeActive(); // Start taking measurements!
	void setOversampleRate(byte); // Sets the # of samples from 1 to 128. See datasheet.
	void enableEventFlags(); // Sets the fundamental event flags. Required during setup.

private:
	//Si7021 & HTU21D Private Functions
	uint16_t makeMeasurment(uint8_t command);
	void     writeReg(uint8_t value);
	uint8_t  readReg();

	//MPL3115A2 Private Functions
	void toggleOneShot();
	byte IIC_Read(byte regAddr);
	void IIC_Write(byte regAddr, byte value);
};

#endif
