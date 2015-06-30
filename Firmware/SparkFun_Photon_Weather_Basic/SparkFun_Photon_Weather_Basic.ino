#pragma SPARK_NO_PREPROCESSOR
#include "Adafruit_MPL3115A2.h"
#include "HTU21D.h"

float humidity = 0; // [%]
float tempf = 0; // [temperature F]
float pascals = 0;
float altf = 0;
float baroTemp = 0;

int count = 0;

HTU21D htu = HTU21D();//create instance of HTU21D Temp and humidity sensor
Adafruit_MPL3115A2 baro = Adafruit_MPL3115A2();//create instance of MPL3115A2 barrometric sensor

//Declare all functions, won't compile without these
void printInfo();
void getTempHumidity();
void getBaro();
void calcWeather();

//---------------------------------------------------------------
void setup()
{
    Serial.begin(9600);   // open serial over USB at 9600 baud

    //Initialize both on-board sensors
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

}
//---------------------------------------------------------------
void loop()
{
      //Get readings from all sensors
      calcWeather();
      //Rather than use a delay, keeping track of a counter allows the photon to still
      //take readings and do work in between printing out data.
      count++;
      if(count == 10)//alter this number to change the amount of time between each reading
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
    Serial.print(pascals/3377);
    Serial.print("Inches(Hg), ");

    Serial.print("Altitude:");
    Serial.print(altf);
    Serial.println("ft.");

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
  float temp = 0;

  temp = baro.getTemperature();
  baroTemp = (temp * 9)/5 + 32;

  pascals = baro.getPressure();
  // Our weather page presents pressure in Inches (Hg)
  // Use http://www.onlineconversion.com/pressure.htm for other units
  //Serial.print(pascals/3377); Serial.println(" Inches (Hg)");

  float altm = baro.getAltitude();

  altf = altm * 3.2808;
  //Serial.print(altm); Serial.println(" meters");

}
//---------------------------------------------------------------
void calcWeather()
{
    getTempHumidity();
    getBaro();
}
//---------------------------------------------------------------
