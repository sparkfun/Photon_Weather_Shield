/******************************************************************************
  SparkFun_Photon_Weather_Basic_Soil.ino
  SparkFun Photon Weather Shield basic example with soil moisture and temp
  Joel Bartlett @ SparkFun Electronics
  Original Creation Date: May 18, 2015
  This sketch prints the temperature, humidity, barrometric preassure, altitude,
  soil moisture, and soil temperature to the Seril port. Upload this sketch
  after attaching a soil moisture and or soil temperature sensor to test your
  connections.

  Hardware Connections:
	This sketch was written specifically for the Photon Weather Shield,
	which connects the HTU21D and MPL3115A2 to the I2C bus by default.
  If you have an HTU21D and/or an MPL3115A2 breakout,	use the following
  hardware setup:
      HTU21D ------------- Photon
      (-) ------------------- GND
      (+) ------------------- 3.3V (VCC)
       CL ------------------- D1/SCL
       DA ------------------- D0/SDA

    MPL3115A2 ------------- Photon
      GND ------------------- GND
      VCC ------------------- 3.3V (VCC)
      SCL ------------------ D1/SCL
      SDA ------------------ D0/SDA

    Soil Moisture Sensor ----- Photon
        GND ------------------- GND
        VCC ------------------- D5
        SIG ------------------- A1

    DS18B20 Temp Sensor ------ Photon
        VCC (Red) ------------- 3.3V (VCC)
        GND (Black) ----------- GND
        SIG (White) ----------- D4


  Development environment specifics:
  	IDE: Particle Dev
  	Hardware Platform: Particle Photon
                       Particle Core

  This code is beerware; if you see me (or any other SparkFun
  employee) at the local, and you've found our code helpful,
  please buy us a round!
  Distributed as-is; no warranty is given.
*******************************************************************************/
#include "SparkFun_MPL3115A2.h"
#include "HTU21D.h"
#include "OneWire.h"
#include "spark-dallas-temperature.h"

#define ONE_WIRE_BUS D4
#define TEMPERATURE_PRECISION 11
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

int SOIL_MOIST = A1;
int SOIL_MOIST_POWER = D5;

//Run I2C Scanner to get address of DS18B20(s)
//(found in the Firmware folder in the Photon Weather Shield Repo)
DeviceAddress inSoilThermometer =
{0x28, 0xD5, 0xBE, 0x5F, 0x06, 0x00, 0x00, 0x4F};//Waterproof temp sensor address
/***********REPLACE THIS ADDRESS WITH YOUR ADDRESS*************/

float humidity = 0;
float tempf = 0;
double InTempC = 0;//original temperature in C from DS18B20
float soiltempf = 0;//converted temperature in F from DS18B20
float pascals = 0;
float altf = 0;
float baroTemp = 0;
int soilMoisture = 0;

int count = 0;

HTU21D htu = HTU21D();//create instance of HTU21D Temp and humidity sensor
MPL3115A2 baro = MPL3115A2();//create instance of MPL3115A2 barrometric sensor

void update18B20Temp(DeviceAddress deviceAddress, double &tempC);//predeclare to compile

//---------------------------------------------------------------
void setup()
{
    // DS18B20 initialization
    sensors.begin();
    sensors.setResolution(inSoilThermometer, TEMPERATURE_PRECISION);

    Serial.begin(9600);   // open serial over USB

    pinMode(SOIL_MOIST_POWER, OUTPUT);
    digitalWrite(SOIL_MOIST_POWER, LOW);

	while(! htu.begin())
    {
	    Serial.println("HTU21D not found");
	    delay(500);
	  }
    Serial.println("HTU21D OK");

	while(! baro.begin())
    {
      Serial.println("MPL3115A2 not found");
      delay(500);
    }
    Serial.println("MPL3115A2 OK");

    //baro.setModeBarometer();
    //Serial.println(baro.IIC_Read(0x0C), HEX);

    baro.setModeAltimeter();

    //MPL3115A2 Settings
    //baro.setModeBarometer();//Set to Barometer Mode
    baro.setModeAltimeter();//Set to altimeter Mode

    baro.setOversampleRate(7); // Set Oversample to the recommended 128
    baro.enableEventFlags(); //Necessary register calls to enble temp, baro ansd alt
}
//---------------------------------------------------------------
void loop()
{
      //Get readings from all sensors
      calcWeather();
      //Rather than use a delay, keeping track of a counter allows the photon to
      // still take readings and do work in between printing out data.
      count++;
      //alter this number to change the amount of time between each reading
      if(count == 5)
      {
         printInfo();
         count = 0;
      }
}
//---------------------------------------------------------------
void printInfo()
{
  //This function prints the weather data out to the default Serial Port

      //Take the temp reading from each sensor and average them.
      Serial.print("Temp:");
      Serial.print((tempf+baroTemp)/2);
      Serial.print("F, ");

      //Or you can print each temp separately
      /*Serial.print("HTU21D Temp: ");
      Serial.print(tempf);
      Serial.print("F, ");
      Serial.print("Baro Temp: ");
      Serial.print(baroTemp);
      Serial.print("F, ");*/


      Serial.print("Humidity:");
      Serial.print(humidity);
      Serial.print("%, ");

      Serial.print("Pressure:");
      Serial.print(pascals);
      Serial.print("Pa, ");

      Serial.print("Altitude:");
      Serial.print(altf);
      Serial.print("ft. ");

      Serial.print("Soil_Temp:");
      Serial.print(soiltempf);
      Serial.print("F, ");

      Serial.print("Soil_Mositure:");
      Serial.println(soilMoisture);//Mositure Content is expressed as an analog
      //value, which can range from 0 (completely dry) to the value of the
      //materials' porosity at saturation. The sensor tends to max out between
      //3000 and 3500.
}
//---------------------------------------------------------------
void getSoilTemp()
{
    //get temp from DS18B20
    sensors.requestTemperatures();
    update18B20Temp(inSoilThermometer, InTempC);
    soiltempf = (InTempC * 9)/5 + 32;
}
//---------------------------------------------------------------
void getSoilMositure()
{
    /*We found through testing that leaving the soil moisture sensor powered
    all the time lead to corrosion of the probes. Thus, this port breaks out
    Digital Pin D5 as the power pin for the sensor, allowing the Photon to
    power the sensor, take a reading, and then disable power on the sensor,
    giving the sensor a longer lifespan.*/
    digitalWrite(SOIL_MOIST_POWER, HIGH);
    delay(200);
    soilMoisture = analogRead(SOIL_MOIST);
    delay(100);
    digitalWrite(SOIL_MOIST_POWER, LOW);

}
//---------------------------------------------------------------
void update18B20Temp(DeviceAddress deviceAddress, double &tempC)
{
  tempC = sensors.getTempC(deviceAddress);
}
//---------------------------------------------------------------
void getTempHumidity()
{
    float temp = 0;

    temp = htu.readTemperature();
    tempf = (temp * 9)/5 + 32;

    humidity = htu.readHumidity();
}
//---------------------------------------------------------------
void getBaro()
{
  baroTemp = baro.readTempF();//get the temperature in F

  pascals = baro.readPressure();//get pressure in Pascals

  altf = baro.readAltitudeFt();//get altitude in feet
}
//---------------------------------------------------------------
void calcWeather()
{
    getSoilTemp();
    getTempHumidity();
    getBaro();
    getSoilMositure();
}
