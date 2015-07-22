/******************************************************************************
  SparkFun_Photon_Weather_Basic_Soil.ino
  SparkFun Photon Weather Shield basic example with soil moisture and temp
  Joel Bartlett @ SparkFun Electronics
  Original Creation Date: May 18, 2015
  This sketch prints the temperature, humidity, barrometric preassure, altitude,
  to the Seril port.

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

  Development environment specifics:
  	IDE: Particle Dev
  	Hardware Platform: Particle Photon
                       Particle Core

  This code is beerware; if you see me (or any other SparkFun
  employee) at the local, and you've found our code helpful,
  please buy us a round!
  Distributed as-is; no warranty is given.

  Weather Underground Upload sections: Dan Fein @ Weather Underground
  Weather Underground Upload Protocol:
  http://wiki.wunderground.com/index.php/PWS_-_Upload_Protocol
  Sign up at http://www.wunderground.com/personal-weather-station/signup.asp

*******************************************************************************/
#include "HTU21D.h"
#include "SparkFun_MPL3115A2.h"
#include "math.h"

float humidity = 0;
float humTempF = 0;  //humidity sensor temp reading, fahrenheit
float humTempC = 0;  //humidity sensor temp reading, celsius
float baroTempF = 0; //barometer sensor temp reading, fahrenheit
float baroTempC = 0; //barometer sensor temp reading, celsius
float tempF = 0;     //Average of the sensors temperature readings, fahrenheit
float tempC = 0;     //Average of the sensors temperature readings, celsius
float dewptF = 0;
float dewptC = 0;
float pascals = 0;
float inches = 0;

//Wunderground Vars
//char SERVER[] = "rtupdate.wunderground.com"; // Realtime update server
char SERVER [] = "weatherstation.wunderground.com"; //standard server
char WEBPAGE [] = "GET /weatherstation/updateweatherstation.php?";

//Station Identification
char ID [] = "xxxxxx"; //Your station ID here
char PASSWORD [] = "xxxxxx"; //your Weather Underground password here

TCPClient client;

HTU21D htu = HTU21D();//create instance of HTU21D Temp and humidity sensor
MPL3115A2 baro = MPL3115A2();//create instance of MPL3115A2 barometric sensor

//---------------------------------------------------------------
void setup()
{
    Serial.begin(9600);   // open serial over USB at 9600 baud

    //Initialize both on-board sensors
    while(! htu.begin()){
  	    Serial.println("HTU21D not found");
  	    delay(1000);
  	}
  	Serial.println("HTU21D OK");

  	while(! baro.begin()) {
          Serial.println("MPL3115A2 not found");
          delay(1000);
     }
     Serial.println("MPL3115A2 OK");

     //MPL3115A2 Settings
     baro.setModeBarometer();//Set to Barometer Mode
     //baro.setModeAltimeter();//Set to altimeter Mode

     baro.setOversampleRate(7); // Set Oversample to the recommended 128
     baro.enableEventFlags(); //Necessary register calls to enble temp, baro ansd alt

}
//---------------------------------------------------------------
void loop()
{
      //Get readings from all sensors
      calcWeather();

      //Print to console
      printInfo();

      //Send data to Weather Underground
      Serial.println("connecting...");

      if (client.connect(SERVER, 80)) {
      Serial.println("Connected");
      //Say Hello
      client.print(WEBPAGE);
      client.print("ID=");
      client.print(ID);
      client.print("&PASSWORD=");
      client.print(PASSWORD);
      client.print("&dateutc=now"); //can use instead of RTC if sending in real time
      //Data send
      client.print("&tempf=");
      client.print(tempF);
      client.print("&dewptf=");
      client.print(dewptF);
      client.print("&humidity=");
      client.print(humidity);
      client.print("&baromin=");
      client.print(inches);
      //Close it up
      client.print("&action=updateraw");//Standard update
      //client.print("&softwaretype=SparkPhoton&action=updateraw&realtime=1&rtfreq=30");//Rapid Fire
      client.println();
      Serial.println("Upload complete");
      }else {
      Serial.println(F("Connection failed"));
      return;
      }

      //If you want to power down between sends to save power (ie batteries).
      System.sleep(SLEEP_MODE_DEEP,120); //sleep measured in seconds
}
//---------------------------------------------------------------
void printInfo()
{
//This function prints the weather data out to the default Serial Port

    //Take the temp reading from each sensor and average them.
    Serial.print("Temp:");
    Serial.print(tempF);
    Serial.print("F, ");

    //Or you can print each temp separately
    Serial.print("HTU21D Temp: ");
    Serial.print(humTempF);
    Serial.print("F, ");
    Serial.print("Baro Temp: ");
    Serial.print(baroTempF);
    Serial.print("F, ");

    Serial.print("Dew Point:");
    Serial.print(dewptF);
    Serial.println("F, ");

    Serial.print("Humidity:");
    Serial.print(humidity);
    Serial.print("%, ");

    Serial.print("Pressure:");
    Serial.print(pascals);
    Serial.print("Pa, ");
    Serial.print(inches);
    Serial.print("in, ");
}
//---------------------------------------------------------------
void getTempHumidity()
{
    float temp = 0;

    humTempC = htu.readTemperature();
    humTempF = (humTempC * 9)/5 + 32;

    humidity = htu.readHumidity();
}
//---------------------------------------------------------------
void getBaro()
{
  baroTempF = baro.readTempF();//get the temperature in F
  baroTempC = baro.readTemp();//get the temperature in C

  pascals = baro.readPressure();//get pressure in Pascals
  inches = baro.readPressure()* 0.0002953; // Calc for converting Pa to inHg (for Wunderground)
}
//---------------------------------------------------------------
void calcWeather()
{
    getTempHumidity();
    getBaro();
    tempF=((humTempF+baroTempF)/2);
    tempC=((humTempC+baroTempC)/2);
    getDewPoint();
}
//---------------------------------------------------------------
void getDewPoint()
{
    dewptC = dewPoint(tempC, humidity);
    dewptF = (dewptC * 9.0)/ 5.0 + 32.0;
}
//---------------------------------------------------------------
// dewPoint function from NOAA
// reference (1) : http://wahiduddin.net/calc/density_algorithms.htm
// reference (2) : http://www.colorado.edu/geography/weather_station/Geog_site/about.htm
//---------------------------------------------------------------
double dewPoint(double celsius, double humidity)
{
	// (1) Saturation Vapor Pressure = ESGG(T)
	double RATIO = 373.15 / (273.15 + celsius);
	double RHS = -7.90298 * (RATIO - 1);
	RHS += 5.02808 * log10(RATIO);
	RHS += -1.3816e-7 * (pow(10, (11.344 * (1 - 1/RATIO ))) - 1) ;
	RHS += 8.1328e-3 * (pow(10, (-3.49149 * (RATIO - 1))) - 1) ;
	RHS += log10(1013.246);

  // factor -3 is to adjust units - Vapor Pressure SVP * humidity
	double VP = pow(10, RHS - 3) * humidity;

  // (2) DEWPOINT = F(Vapor Pressure)
	double T = log(VP/0.61078);   // temp var
	return (241.88 * T) / (17.558 - T);
}
//---------------------------------------------------------------
