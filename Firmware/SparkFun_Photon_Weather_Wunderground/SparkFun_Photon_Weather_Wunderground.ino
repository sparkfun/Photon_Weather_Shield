/******************************************************************************
  SparkFun_Photon_Weather_Wunderground.ino
  SparkFun Photon Weather Shield basic example
  Joel Bartlett @ SparkFun Electronics
  Original Creation Date: May 18, 2015
  Updated August 21, 2015
  This sketch prints the temperature, humidity, and barometric pressure OR
  altitude to the Serial port.

  The library used in this example can be found here:
  https://github.com/sparkfun/SparkFun_Photon_Weather_Shield_Particle_Library

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

//---------------------------------------------------------------

  Weather Underground Upload sections: Dan Fein @ Weather Underground
  Weather Underground Upload Protocol:
  http://wiki.wunderground.com/index.php/PWS_-_Upload_Protocol
  Sign up at http://www.wunderground.com/personal-weather-station/signup.asp

*******************************************************************************/
#include "SparkFun_Photon_Weather_Shield_Library.h"
#include "math.h"   //For Dew Point Calculation

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

//char SERVER[] = "rtupdate.wunderground.com";        //Rapidfire update server - for multiple sends per minute
char SERVER [] = "weatherstation.wunderground.com";   //Standard server - for sends once per minute or less
char WEBPAGE [] = "GET /weatherstation/updateweatherstation.php?";

//Station Identification
char ID [] = "xxxxxx"; //Your station ID here
char PASSWORD [] = "xxxxxx"; //your Weather Underground password here

TCPClient client;

//Create Instance of HTU21D or SI7021 temp and humidity sensor and MPL3115A2 barometric sensor
Weather sensor;

//---------------------------------------------------------------
void setup()
{
    Serial.begin(9600);   // open serial over USB at 9600 baud

    //Initialize the I2C sensors and ping them
    sensor.begin();

    /*You can only receive acurate barrometric readings or acurate altitiude
    readings at a given time, not both at the same time. The following two lines
    tell the sensor what mode to use. You could easily write a function that
    takes a reading in one made and then switches to the other mode to grab that
    reading, resulting in data that contains both acurate altitude and barrometric
    readings. For this example, we will only be using the barometer mode. Be sure
    to only uncomment one line at a time. */
    sensor.setModeBarometer();//Set to Barometer Mode
    //baro.setModeAltimeter();//Set to altimeter Mode

    //These are additional MPL3115A2 functions the MUST be called for the sensor to work.
    sensor.setOversampleRate(7); // Set Oversample rate
    //Call with a rate from 0 to 7. See page 33 for table of ratios.
    //Sets the over sample rate. Datasheet calls for 128 but you can set it
    //from 1 to 128 samples. The higher the oversample rate the greater
    //the time between data samples.


    sensor.enableEventFlags(); //Necessary register calls to enble temp, baro ansd alt

}
//---------------------------------------------------------------
void loop()
{
      //Get readings from all sensors
      getWeather();

      //Print to console
      printInfo();

      //Send data to Weather Underground
      sendToWU();

      //Power down between sends to save power, measured in seconds.
      System.sleep(SLEEP_MODE_DEEP,300);  //for Particle Photon
      //Spark.sleep(SLEEP_MODE_DEEP,300);   //for Spark Core
}
//---------------------------------------------------------------
void printInfo()
{
//This function prints the weather data out to the default Serial Port

  Serial.print("Temp:");
  Serial.print(tempF);
  Serial.print("F, ");

  Serial.print("Humidity:");
  Serial.print(humidity);
  Serial.print("%, ");

  Serial.print("Baro_Temp:");
  Serial.print(baroTempF);
  Serial.print("F, ");

  Serial.print("Humid_Temp:");
  Serial.print(humTempF);
  Serial.print("F, ");

  Serial.print("Pressure:");
  Serial.print(pascals/100);
  Serial.print("hPa, ");
  Serial.print(inches);
  Serial.println("in.Hg");
  //The MPL3115A2 outputs the pressure in Pascals. However, most weather stations
  //report pressure in hectopascals or millibars. Divide by 100 to get a reading
  //more closely resembling what online weather reports may say in hPa or mb.
  //Another common unit for pressure is Inches of Mercury (in.Hg). To convert
  //from mb to in.Hg, use the following formula. P(inHg) = 0.0295300 * P(mb)
  //More info on conversion can be found here:
  //www.srh.noaa.gov/images/epz/wxcalc/pressureConversion.pdf

  //If in altitude mode, print with these lines
  //Serial.print("Altitude:");
  //Serial.print(altf);
  //Serial.println("ft.");

}
//---------------------------------------------------------------
void sendToWU()
{
  Serial.println("connecting...");

  if (client.connect(SERVER, 80)) {
  Serial.println("Connected");
  client.print(WEBPAGE);
  client.print("ID=");
  client.print(ID);
  client.print("&PASSWORD=");
  client.print(PASSWORD);
  client.print("&dateutc=now");      //can use 'now' instead of time if sending in real time
  client.print("&tempf=");
  client.print(tempF);
  client.print("&dewptf=");
  client.print(dewptF);
  client.print("&humidity=");
  client.print(humidity);
  client.print("&baromin=");
  client.print(inches);
  client.print("&action=updateraw");    //Standard update rate - for sending once a minute or less
  //client.print("&softwaretype=Particle-Photon&action=updateraw&realtime=1&rtfreq=30");  //Rapid Fire update rate - for sending multiple times per minute, specify frequency in seconds
  client.println();
  Serial.println("Upload complete");
  delay(300);                         //Without the delay it goes to sleep too fast and the send is unreliable
  }else{
    Serial.println(F("Connection failed"));
  return;
  }
}
//---------------------------------------------------------------
void getWeather()
{
  // Measure Relative Humidity from the HTU21D or Si7021
  humidity = sensor.getRH();

  // Measure Temperature from the HTU21D or Si7021
  humTempC = sensor.getTemp();
  humTempF = (humTempC * 9)/5 + 32;
  // Temperature is measured every time RH is requested.
  // It is faster, therefore, to read it from previous RH
  // measurement with getTemp() instead with readTemp()

  //Measure the Barometer temperature in F from the MPL3115A2
  baroTempC = sensor.readBaroTemp();
  baroTempF = (baroTempC * 9)/5 + 32; //convert the temperature to F

  //Measure Pressure from the MPL3115A2
  pascals = sensor.readPressure();
  inches = pascals * 0.0002953; // Calc for converting Pa to inHg (Wunderground expects inHg)

  //If in altitude mode, you can get a reading in feet with this line:
  //float altf = sensor.readAltitudeFt();

  //Average the temperature reading from both sensors
  tempC=((humTempC+baroTempC)/2);
  tempF=((humTempF+baroTempF)/2);

  //Calculate Dew Point
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
