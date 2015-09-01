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
 By: Jakub Kaminski, 2014
 https://github.com/teoqba/ADDRESS

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

 #include "SparkFun_Photon_Weather_Shield_Library.h"

 //Initialize
 Weather::Weather(){}

 void Weather::begin(void)
{
  Wire.begin();

  uint8_t ID_Barro = IIC_Read(WHO_AM_I );
  uint8_t ID_Temp_Hum = checkID();

  int x,y = 0;

  if (ID_Barro == 0xC4)//Ping WhoAmI register
    x = 1;
  else
  	x = 0;

  if(ID_Temp_Hum == 0x15)//Ping CheckID register
    y = 1;
  else if(ID_Temp_Hum == 0x32)
    y = 2;
  else
    y = 0;

  if(x == 1 && y == 1)
  {
    Serial.println("MPL3115A2 Found");
    Serial.println("Si7021 Found");
  }
  else if(x == 1 && y == 2)
  {
    Serial.println("MPL3115A2 Found");
    Serial.println("HTU21D Found");
  }
  else if(x == 0 && y == 1)
  {
    Serial.println("MPL3115A2 NOT Found");
    Serial.println("Si7021 Found");
  }
  else if(x == 0 && y == 2)
  {
    Serial.println("MPL3115A2 NOT Found");
    Serial.println("HTU21D Found");
  }
  else if(x == 1 && y == 0)
  {
    Serial.println("MPL3115A2 Found");
    Serial.println("No Temp/Humidity Device Detected");
  }
  else
  	Serial.println("No Devices Detected");
}

/****************Si7021 & HTU21D Functions**************************************/


float Weather::getRH()
{
	// Measure the relative humidity
	uint16_t RH_Code = makeMeasurment(HUMD_MEASURE_NOHOLD);
	float result = (125.0*RH_Code/65536)-6;
	return result;
}

float Weather::readTemp()
{
	// Read temperature from previous RH measurement.
	uint16_t temp_Code = makeMeasurment(TEMP_PREV);
	float result = (175.25*temp_Code/65536)-46.85;
	return result;
}

float Weather::getTemp()
{
	// Measure temperature
	uint16_t temp_Code = makeMeasurment(TEMP_MEASURE_NOHOLD);
	float result = (175.25*temp_Code/65536)-46.85;
	return result;
}
//Give me temperature in fahrenheit!
float Weather::readTempF()
{
  return((readTemp() * 1.8) + 32.0); // Convert celsius to fahrenheit
}

float Weather::getTempF()
{
  return((getTemp() * 1.8) + 32.0); // Convert celsius to fahrenheit
}


void Weather::heaterOn()
{
	// Turns on the ADDRESS heater
	uint8_t regVal = readReg();
	regVal |= _BV(HTRE);
	//turn on the heater
	writeReg(regVal);
}

void Weather::heaterOff()
{
	// Turns off the ADDRESS heater
	uint8_t regVal = readReg();
	regVal &= ~_BV(HTRE);
	writeReg(regVal);
}

void Weather::changeResolution(uint8_t i)
{
	// Changes to resolution of ADDRESS measurements.
	// Set i to:
	//      RH         Temp
	// 0: 12 bit       14 bit (default)
	// 1:  8 bit       12 bit
	// 2: 10 bit       13 bit
	// 3: 11 bit       11 bit

	uint8_t regVal = readReg();
	// zero resolution bits
	regVal &= 0b011111110;
	switch (i) {
	  case 1:
	    regVal |= 0b00000001;
	    break;
	  case 2:
	    regVal |= 0b10000000;
	    break;
	  case 3:
	    regVal |= 0b10000001;
	  default:
	    regVal |= 0b00000000;
	    break;
	}
	// write new resolution settings to the register
	writeReg(regVal);
}

void Weather::reset()
{
	//Reset user resister
	writeReg(SOFT_RESET);
}

uint8_t Weather::checkID()
{
	uint8_t ID_1;

 	// Check device ID
	Wire.beginTransmission(ADDRESS);
	Wire.write(0xFC);
	Wire.write(0xC9);
	Wire.endTransmission();

    Wire.requestFrom(ADDRESS,1);

    ID_1 = Wire.read();

    return(ID_1);
}

uint16_t Weather::makeMeasurment(uint8_t command)
{
	// Take one ADDRESS measurement given by command.
	// It can be either temperature or relative humidity
	// TODO: implement checksum checking

	uint16_t nBytes = 3;
	// if we are only reading old temperature, read olny msb and lsb
	if (command == 0xE0) nBytes = 2;

	Wire.beginTransmission(ADDRESS);
	Wire.write(command);
	Wire.endTransmission();
	// When not using clock stretching (*_NOHOLD commands) delay here
	// is needed to wait for the measurement.
	// According to datasheet the max. conversion time is ~22ms
	 delay(100);

	Wire.requestFrom(ADDRESS,nBytes);
	//Wait for data
	int counter = 0;
	while (Wire.available() < nBytes){
	  delay(1);
	  counter ++;
	  if (counter >100){
	    // Timeout: Sensor did not return any data
	    return 100;
	  }
	}

	unsigned int msb = Wire.read();
	unsigned int lsb = Wire.read();
	// Clear the last to bits of LSB to 00.
	// According to datasheet LSB of RH is always xxxxxx10
	lsb &= 0xFC;
	unsigned int mesurment = msb << 8 | lsb;

	return mesurment;
}

void Weather::writeReg(uint8_t value)
{
	// Write to user register on ADDRESS
	Wire.beginTransmission(ADDRESS);
	Wire.write(WRITE_USER_REG);
	Wire.write(value);
	Wire.endTransmission();
}

uint8_t Weather::readReg()
{
	// Read from user register on ADDRESS
	Wire.beginTransmission(ADDRESS);
	Wire.write(READ_USER_REG);
	Wire.endTransmission();
	Wire.requestFrom(ADDRESS,1);
	uint8_t regVal = Wire.read();
	return regVal;
}

/****************MPL3115A2 Functions**************************************/
//Returns the number of meters above sea level
//Returns -1 if no new data is available
float Weather::readAltitude()
{
	toggleOneShot(); //Toggle the OST bit causing the sensor to immediately take another reading

	//Wait for PDR bit, indicates we have new pressure data
	int counter = 0;
	while( (IIC_Read(STATUS) & (1<<1)) == 0)
	{
		if(++counter > 600) return(-999); //Error out after max of 512ms for a read
		delay(1);
	}

	// Read pressure registers
	Wire.beginTransmission(MPL3115A2_ADDRESS);
	Wire.write(OUT_P_MSB);  // Address of data to get
	Wire.endTransmission(false); // Send data to I2C dev with option for a repeated start. THIS IS NECESSARY and not supported before Arduino V1.0.1!
	if (Wire.requestFrom(MPL3115A2_ADDRESS, 3) != 3) { // Request three bytes
		return -999;
	}

	byte msb, csb, lsb;
	msb = Wire.read();
	csb = Wire.read();
	lsb = Wire.read();

	// The least significant bytes l_altitude and l_temp are 4-bit,
	// fractional values, so you must cast the calulation in (float),
	// shift the value over 4 spots to the right and divide by 16 (since
	// there are 16 values in 4-bits).
	float tempcsb = (lsb>>4)/16.0;

	float altitude = (float)( (msb << 8) | csb) + tempcsb;

	return(altitude);
}

//Returns the number of feet above sea level
float Weather::readAltitudeFt()
{
  return(readAltitude() * 3.28084);
}

//Reads the current pressure in Pa
//Unit must be set in barometric pressure mode
//Returns -1 if no new data is available
float Weather::readPressure()
{
	//Check PDR bit, if it's not set then toggle OST
	if(IIC_Read(STATUS) & (1<<2) == 0) toggleOneShot(); //Toggle the OST bit causing the sensor to immediately take another reading

	//Wait for PDR bit, indicates we have new pressure data
	int counter = 0;
	while(IIC_Read(STATUS) & (1<<2) == 0)
	{
		if(++counter > 600) return(-999); //Error out after max of 512ms for a read
		delay(1);
	}

	// Read pressure registers
	Wire.beginTransmission(MPL3115A2_ADDRESS);
	Wire.write(OUT_P_MSB);  // Address of data to get
	Wire.endTransmission(false); // Send data to I2C dev with option for a repeated start. THIS IS NECESSARY and not supported before Arduino V1.0.1!
	if (Wire.requestFrom(MPL3115A2_ADDRESS, 3) != 3) { // Request three bytes
		return -999;
	}

	byte msb, csb, lsb;
	msb = Wire.read();
	csb = Wire.read();
	lsb = Wire.read();

	toggleOneShot(); //Toggle the OST bit causing the sensor to immediately take another reading

	// Pressure comes back as a left shifted 20 bit number
	long pressure_whole = (long)msb<<16 | (long)csb<<8 | (long)lsb;
	pressure_whole >>= 6; //Pressure is an 18 bit number with 2 bits of decimal. Get rid of decimal portion.

	lsb &= 0b00110000; //Bits 5/4 represent the fractional component
	lsb >>= 4; //Get it right aligned
	float pressure_decimal = (float)lsb/4.0; //Turn it into fraction

	float pressure = (float)pressure_whole + pressure_decimal;

	return(pressure);
}

float Weather::readBaroTemp()
{
	if(IIC_Read(STATUS) & (1<<1) == 0) toggleOneShot(); //Toggle the OST bit causing the sensor to immediately take another reading

	//Wait for TDR bit, indicates we have new temp data
	int counter = 0;
	while( (IIC_Read(STATUS) & (1<<1)) == 0)
	{
		if(++counter > 600) return(-999); //Error out after max of 512ms for a read
		delay(1);
	}

	// Read temperature registers
	Wire.beginTransmission(MPL3115A2_ADDRESS);
	Wire.write(OUT_T_MSB);  // Address of data to get
	Wire.endTransmission(false); // Send data to I2C dev with option for a repeated start. THIS IS NECESSARY and not supported before Arduino V1.0.1!
	if (Wire.requestFrom(MPL3115A2_ADDRESS, 2) != 2) { // Request two bytes
		return -999;
	}

	byte msb, lsb;
	msb = Wire.read();
	lsb = Wire.read();

	toggleOneShot(); //Toggle the OST bit causing the sensor to immediately take another reading

    //Negative temperature fix by D.D.G.
    uint16_t foo = 0;
    bool negSign = false;

    //Check for 2s compliment
	if(msb > 0x7F)
	{
        foo = ~((msb << 8) + lsb) + 1;  //2’s complement
        msb = foo >> 8;
        lsb = foo & 0x00F0;
        negSign = true;
	}

	// The least significant bytes l_altitude and l_temp are 4-bit,
	// fractional values, so you must cast the calulation in (float),
	// shift the value over 4 spots to the right and divide by 16 (since
	// there are 16 values in 4-bits).
	float templsb = (lsb>>4)/16.0; //temp, fraction of a degree

	float temperature = (float)(msb + templsb);

	if (negSign) temperature = 0 - temperature;

	return(temperature);
}

//Give me temperature in fahrenheit!
float Weather::readBaroTempF()
{
  return((readBaroTemp() * 9.0)/ 5.0 + 32.0); // Convert celsius to fahrenheit
}

//Sets the mode to Barometer
//CTRL_REG1, ALT bit
void Weather::setModeBarometer()
{
  byte tempSetting = IIC_Read(CTRL_REG1); //Read current settings
  tempSetting &= ~(1<<7); //Clear ALT bit
  IIC_Write(CTRL_REG1, tempSetting);
}

//Sets the mode to Altimeter
//CTRL_REG1, ALT bit
void Weather::setModeAltimeter()
{
  byte tempSetting = IIC_Read(CTRL_REG1); //Read current settings
  tempSetting |= (1<<7); //Set ALT bit
  IIC_Write(CTRL_REG1, tempSetting);
}

//Puts the sensor in standby mode
//This is needed so that we can modify the major control registers
void Weather::setModeStandby()
{
  byte tempSetting = IIC_Read(CTRL_REG1); //Read current settings
  tempSetting &= ~(1<<0); //Clear SBYB bit for Standby mode
  IIC_Write(CTRL_REG1, tempSetting);
}

//Puts the sensor in active mode
//This is needed so that we can modify the major control registers
void Weather::setModeActive()
{
  byte tempSetting = IIC_Read(CTRL_REG1); //Read current settings
  tempSetting |= (1<<0); //Set SBYB bit for Active mode
  IIC_Write(CTRL_REG1, tempSetting);
}

//Call with a rate from 0 to 7. See page 33 for table of ratios.
//Sets the over sample rate. Datasheet calls for 128 but you can set it
//from 1 to 128 samples. The higher the oversample rate the greater
//the time between data samples.
void Weather::setOversampleRate(byte sampleRate)
{
  if(sampleRate > 7) sampleRate = 7; //OS cannot be larger than 0b.0111
  sampleRate <<= 3; //Align it for the CTRL_REG1 register

  byte tempSetting = IIC_Read(CTRL_REG1); //Read current settings
  tempSetting &= 0b11000111; //Clear out old OS bits
  tempSetting |= sampleRate; //Mask in new OS bits
  IIC_Write(CTRL_REG1, tempSetting);
}

//Enables the pressure and temp measurement event flags so that we can
//test against them. This is recommended in datasheet during setup.
void Weather::enableEventFlags()
{
  IIC_Write(PT_DATA_CFG, 0x07); // Enable all three pressure and temp event flags
}

//Clears then sets the OST bit which causes the sensor to immediately take another reading
//Needed to sample faster than 1Hz
void Weather::toggleOneShot(void)
{
  byte tempSetting = IIC_Read(CTRL_REG1); //Read current settings
  tempSetting &= ~(1<<1); //Clear OST bit
  IIC_Write(CTRL_REG1, tempSetting);

  tempSetting = IIC_Read(CTRL_REG1); //Read current settings to be safe
  tempSetting |= (1<<1); //Set OST bit
  IIC_Write(CTRL_REG1, tempSetting);
}


// These are the two I2C functions in this sketch.
byte Weather::IIC_Read(byte regAddr)
{
  // This function reads one byte over IIC
  Wire.beginTransmission(MPL3115A2_ADDRESS);
  Wire.write(regAddr);  // Address of CTRL_REG1
  Wire.endTransmission(false); // Send data to I2C dev with option for a repeated start. THIS IS NECESSARY and not supported before Arduino V1.0.1!
  Wire.requestFrom(MPL3115A2_ADDRESS, 1); // Request the data...
  return Wire.read();
}

void Weather::IIC_Write(byte regAddr, byte value)
{
  // This function writes one byte over IIC
  Wire.beginTransmission(MPL3115A2_ADDRESS);
  Wire.write(regAddr);
  Wire.write(value);
  Wire.endTransmission(true);
}
