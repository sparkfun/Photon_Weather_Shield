/******************************************************************************
  SparkFun_Photon_Weather_Phant_LCD.ino
  SparkFun Photon Weather Shield example sketch demonstarting how to post
  weather data to data.sparkfun.com aka Phant as well as printing current
  weather information to a Serial Graphic LCD via a Bluetooth connection.

  Based on the Wimp Weather Station sketch by: Nathan Seidle
  https://github.com/sparkfun/Wimp_Weather_Station

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

    BlueSMiRF  ------------- Photon Serial 1 Port
        VCC  ------------------- 3.3V (VCC)
        GND -------------------- GND
        RX  -------------------- TX
        TX --------------------- RX

    BlueSMiRF  ----------- Serial Graphic LCD Backpack
        VCC  ------------------- 5V (VCC)
        GND -------------------- GND
        RX  -------------------- TX
        TX --------------------- RX


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
#include "SparkFunPhant.h"
#include "SparkFun_Serial_Graphic_LCD.h"

#define ONE_WIRE_BUS D4
#define TEMPERATURE_PRECISION 11
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

int SOIL_MOIST = A1;
int SOIL_MOIST_POWER = D5;

int WDIR = A0;
int RAIN = D2;
int WSPEED = D3;

//Run I2C Scanner to get address of DS18B20(s)
//(found in the Firmware folder in the Weather Shield Repo)
DeviceAddress inSoilThermometer = {0x28, 0xD5, 0xBE, 0x5F, 0x06, 0x00, 0x00, 0x4F};
/***********REPLACE THIS ADDRESS WITH YOUR ADDRESS*************/

//Global Variables
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
long lastSecond; //The millis counter to see when a second rolls by
byte seconds; //When it hits 60, increase the current minute
byte seconds_2m; //Keeps track of the "wind speed/dir avg" over last 2 minutes array of data
byte minutes; //Keeps track of where we are in various arrays of data
byte minutes_10m; //Keeps track of where we are in wind gust/dir over last 10 minutes array of data

//We need to keep track of the following variables:
//Wind speed/dir each update (no storage)
//Wind gust/dir over the day (no storage)
//Wind speed/dir, avg over 2 minutes (store 1 per second)
//Wind gust/dir over last 10 minutes (store 1 per minute)
//Rain over the past hour (store 1 per minute)
//Total rain over date (store one per day)

byte windspdavg[120]; //120 bytes to keep track of 2 minute average
int winddiravg[120]; //120 ints to keep track of 2 minute average
float windgust_10m[10]; //10 floats to keep track of 10 minute max
int windgustdirection_10m[10]; //10 ints to keep track of 10 minute max
volatile float rainHour[60]; //60 floating numbers to keep track of 60 minutes of rain

//These are all the weather values that wunderground expects:
int winddir = 0; // [0-360 instantaneous wind direction]
float windspeedmph = 0; // [mph instantaneous wind speed]
float windgustmph = 0; // [mph current wind gust, using software specific time period]
int windgustdir = 0; // [0-360 using software specific time period]
float windspdmph_avg2m = 0; // [mph 2 minute average wind speed mph]
int winddir_avg2m = 0; // [0-360 2 minute average wind direction]
float windgustmph_10m = 0; // [mph past 10 minutes wind gust mph ]
int windgustdir_10m = 0; // [0-360 past 10 minutes wind gust direction]
float rainin = 0; // [rain inches over the past hour)] -- the accumulated rainfall in the past 60 min
long lastWindCheck = 0;
volatile float dailyrainin = 0; // [rain inches so far today in local time]

int humidity = 0;
int tempf = 0;
double InTempC = 0;//original temperature in C from DS18B20
float soiltempf = 0;//converted temperature in F from DS18B20
float pascals = 0;
float altf = 0;
float baroTemp = 0;
int soilMoisture = 0;

int count = 0;

// volatiles are subject to modification by IRQs
volatile long lastWindIRQ = 0;
volatile byte windClicks = 0;
volatile unsigned long raintime, rainlast, raininterval, rain;

HTU21D htu = HTU21D();//create instance of HTU21D Temp and humidity sensor
MPL3115A2 baro = MPL3115A2();//create instance of MPL3115A2 barrometric sensor
LCD lcd = LCD();//create instance of the Serial Graphic LCD


////////////PHANT STUFF/////////////////////////////////////////////////////////////////////
const char server[] = "data.sparkfun.com";
const char publicKey[] = "yourPublicKey";
const char privateKey[] = "yourPrivateKey";
Phant phant(server, publicKey, privateKey);
/////////////////////////////////////////////////////////////////////////////////////////

void update18B20Temp(DeviceAddress deviceAddress, double &tempC);//predeclare to compile

//Interrupt routines (these are called by the hardware interrupts, not by the main code)
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void rainIRQ()
// Count rain gauge bucket tips as they occur
// Activated by the magnet and reed switch in the rain gauge, attached to input D2
{
  raintime = millis(); // grab current time
  raininterval = raintime - rainlast; // calculate interval between this and last event

    if (raininterval > 10) // ignore switch-bounce glitches less than 10mS after initial edge
  {
    dailyrainin += 0.011; //Each dump is 0.011" of water
    rainHour[minutes] += 0.011; //Increase this minute's amount of rain

    rainlast = raintime; // set up for next event
  }
}

void wspeedIRQ()
// Activated by the magnet in the anemometer (2 ticks per rotation), attached to input D3
{
  if (millis() - lastWindIRQ > 10) // Ignore switch-bounce glitches less than 10ms (142MPH max reading) after the reed switch closes
  {
    lastWindIRQ = millis(); //Grab the current time
    windClicks++; //There is 1.492MPH for each click per second.
  }
}

//---------------------------------------------------------------
void setup()
{
    // DS18B20 initialization
    sensors.begin();
    sensors.setResolution(inSoilThermometer, TEMPERATURE_PRECISION);

    Serial.begin(9600);   // open serial over USB
    //Serial1.begin() happens in the Serial Graphic LCD library

    pinMode(WSPEED, INPUT_PULLUP); // input from wind meters windspeed sensor
    pinMode(RAIN, INPUT_PULLUP); // input from wind meters rain gauge sensor

    pinMode(SOIL_MOIST_POWER, OUTPUT);//power control for soil moisture
    digitalWrite(SOIL_MOIST_POWER, LOW);//Leave off by defualt

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

    //MPL3115A2 Settings
    //baro.setModeBarometer();//Set to Barometer Mode
    baro.setModeAltimeter();//Set to altimeter Mode

    baro.setOversampleRate(7); // Set Oversample to the recommended 128
    baro.enableEventFlags(); //Necessary register calls to enble temp, baro ansd alt

    seconds = 0;
    lastSecond = millis();

    // attach external interrupt pins to IRQ functions
    attachInterrupt(RAIN, rainIRQ, FALLING);
    attachInterrupt(WSPEED, wspeedIRQ, FALLING);

    // turn on interrupts
    interrupts();

    //Take first reading before heading into loop()
    calcWeather();
    printInfo();
    postToPhant();
}
//---------------------------------------------------------------
void loop()
{
  //Keep track of which minute it is
  if(millis() - lastSecond >= 1000)
  {

    lastSecond += 1000;

    //Take a speed and direction reading every second for 2 minute average
    if(++seconds_2m > 119) seconds_2m = 0;

    //Calc the wind speed and direction every second for 120 second to get 2 minute average
    float currentSpeed = get_wind_speed();
    //float currentSpeed = random(5); //For testing
    int currentDirection = get_wind_direction();
    windspdavg[seconds_2m] = (int)currentSpeed;
    winddiravg[seconds_2m] = currentDirection;
    //if(seconds_2m % 10 == 0) displayArrays(); //For testing

    //Check to see if this is a gust for the minute
    if(currentSpeed > windgust_10m[minutes_10m])
    {
      windgust_10m[minutes_10m] = currentSpeed;
      windgustdirection_10m[minutes_10m] = currentDirection;
    }

    //Check to see if this is a gust for the day
    if(currentSpeed > windgustmph)
    {
      windgustmph = currentSpeed;
      windgustdir = currentDirection;
    }

    if(++seconds > 59)
    {
      seconds = 0;

      if(++minutes > 59) minutes = 0;
      if(++minutes_10m > 9) minutes_10m = 0;

      rainHour[minutes] = 0; //Zero out this minute's rainfall amount
      windgust_10m[minutes_10m] = 0; //Zero out this minute's gust
    }

    //Get readings from all sensors
    calcWeather();
    //Rather than use a delay, keeping track of a counter allows the photon to
    // still take readings and do work in between printing out data.
    count++;
    //alter this number to change the amount of time between each reading
    if(count == 5)//post roughly every 10 minutes
    {
       printInfo();
       printLCD();
       postToPhant();
       count = 0;
    }
  }
}
//---------------------------------------------------------------
void printInfo()
{
  //This function prints the weather data out to the default Serial Port
      Serial.print("Wind Dir:");
      switch (winddir)
      {
        case 0:
          Serial.print("North");
          break;
        case 1:
          Serial.print("NE");
          break;
        case 2:
          Serial.print("East");
          break;
        case 3:
          Serial.print("SE");
          break;
        case 4:
          Serial.print("South");
          break;
        case 5:
          Serial.print("SW");
          break;
        case 6:
          Serial.print("West");
          break;
        case 7:
          Serial.print("NW");
          break;
        default:
          Serial.print("No Wind");
          // if nothing else matches, do the
          // default (which is optional)
      }
      Serial.print(" Wind Speed:");
      Serial.print(windspeedmph, 1);
      Serial.print("mph, ");

      Serial.print("Rain:");
      Serial.print(rainin, 2);
      Serial.print("in., ");

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
int postToPhant()
{
    phant.add("altf", altf);
    phant.add("barotemp", baroTemp);
    phant.add("humidity", humidity);
    phant.add("pascals", pascals);
    phant.add("rainin", rainin);
    phant.add("soiltempf", soiltempf);
    phant.add("soilmoisture", soilMoisture);
    phant.add("tempf", tempf);
    phant.add("winddir", winddir);
    phant.add("windspeedmph", windspeedmph);

    TCPClient client;
    char response[512];
    int i = 0;
    int retVal = 0;

    if (client.connect(server, 80))
    {
        Serial.println("Posting!");
        client.print(phant.post());
        delay(1000);
        while (client.available())
        {
            char c = client.read();
            //Serial.print(c);
            if (i < 512)
                response[i++] = c;
        }
        if (strstr(response, "200 OK"))
        {
            Serial.println("Post success!");
            retVal = 1;
        }
        else if (strstr(response, "400 Bad Request"))
        {
            Serial.println("Bad request");
            retVal = -1;
        }
        else
        {
            retVal = -2;
        }
    }
    else
    {
        Serial.println("connection failed");
        retVal = -3;
    }
    client.stop();
    return retVal;

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
//Read the wind direction sensor, return heading in degrees
int get_wind_direction()
{
  unsigned int adc;

  adc = analogRead(WDIR); // get the current reading from the sensor

  // The following table is ADC readings for the wind direction sensor output, sorted from low to high.
  // Each threshold is the midpoint between adjacent headings. The output is degrees for that ADC reading.
  // Note that these are not in compass degree order! See Weather Meters datasheet for more information.

  if(adc > 2270 && adc < 2290) return (0);//North
  if(adc > 3220 && adc < 3240) return (1);//NE
  if(adc > 3890 && adc < 3910) return (2);//East
  if(adc > 3780 && adc < 3800) return (3);//SE

  if(adc > 3570 && adc < 3590) return (4);//South
  if(adc > 2790 && adc < 2810) return (5);
  if(adc > 1580 && adc < 1610) return (6);
  if(adc > 1930 && adc < 1950) return (7);

  return (-1); // error, disconnected?
}
//---------------------------------------------------------------
//Returns the instataneous wind speed
float get_wind_speed()
{
  float deltaTime = millis() - lastWindCheck; //750ms

  deltaTime /= 1000.0; //Covert to seconds

  float windSpeed = (float)windClicks / deltaTime; //3 / 0.750s = 4

  windClicks = 0; //Reset and start watching for new wind
  lastWindCheck = millis();

  windSpeed *= 1.492; //4 * 1.492 = 5.968MPH

  /* Serial.println();
   Serial.print("Windspeed:");
   Serial.println(windSpeed);*/

  return(windSpeed);
}
//---------------------------------------------------------------
void calcWeather()
{
    getSoilTemp();
    getTempHumidity();
    getBaro();
    getSoilMositure();

    //Calc winddir
    winddir = get_wind_direction();

    //Calc windspeed
    windspeedmph = get_wind_speed();

    //Calc windgustmph
    //Calc windgustdir
    //Report the largest windgust today
    windgustmph = 0;
    windgustdir = 0;

    //Calc windspdmph_avg2m
    float temp = 0;
    for(int i = 0 ; i < 120 ; i++)
      temp += windspdavg[i];
    temp /= 120.0;
    windspdmph_avg2m = temp;

    //Calc winddir_avg2m
    temp = 0; //Can't use winddir_avg2m because it's an int
    for(int i = 0 ; i < 120 ; i++)
      temp += winddiravg[i];
    temp /= 120;
    winddir_avg2m = temp;

    //Calc windgustmph_10m
    //Calc windgustdir_10m
    //Find the largest windgust in the last 10 minutes
    windgustmph_10m = 0;
    windgustdir_10m = 0;
    //Step through the 10 minutes
    for(int i = 0; i < 10 ; i++)
    {
      if(windgust_10m[i] > windgustmph_10m)
      {
        windgustmph_10m = windgust_10m[i];
        windgustdir_10m = windgustdirection_10m[i];
      }
    }

    //Total rainfall for the day is calculated within the interrupt
    //Calculate amount of rainfall for the last 60 minutes
    rainin = 0;
    for(int i = 0 ; i < 60 ; i++)
      rainin += rainHour[i];
}
//---------------------------------------------------------------
void printLCD()
{
    lcd.clearScreen();
    delay(100);
    lcd.setHome();
    delay(100);

    //For anything that won't work with the library, you can still use Seral1
    Serial1.println(Time.timeStr());
    Serial1.println();
    delay(100);

    lcd.print("Wind Dir: ");
    switch (winddir)
    {
      case 0:
        lcd.println("North");
        break;
      case 1:
        lcd.println("NE");
        break;
      case 2:
        lcd.println("East");
        break;
      case 3:
        lcd.println("SE");
        break;
      case 4:
        lcd.println("South");
        break;
      case 5:
        lcd.println("SW");
        break;
      case 6:
        lcd.println("West");
        break;
      case 7:
        lcd.println("NW");
        break;
      default:
        lcd.println("No Wind");
        // if nothing else matches, do the
        // default (which is optional)
    }
    delay(100);

    lcd.print("Wind Speed: ");
    lcd.print(windspeedmph);
    lcd.println("mph");
    lcd.println();
    delay(100);
    /*lcd.print(",windgustmph=");
    lcd.print(windgustmph, 1);
    lcd.print(",windgustdir=");
    lcd.print(windgustdir);
    lcd.print(",windspdmph_avg2m=");
    lcd.print(windspdmph_avg2m, 1);
    lcd.print(",winddir_avg2m=");
    lcd.print(winddir_avg2m);
    lcd.print(",windgustmph_10m=");
    lcd.print(windgustmph_10m, 1);
    lcd.print(",windgustdir_10m=");
    lcd.print(windgustdir_10m);*/
    lcd.print("Temp: ");
    lcd.print((tempf+baroTemp)/2);
    lcd.println("F");
    delay(100);

    lcd.print("Humidity: ");
    lcd.print(humidity);
    lcd.println("%");
    lcd.println();
    delay(100);
    //lcd.print("Baro Temp: ");
    //lcd.print(baroTemp);
    //lcd.print("F, ");


    lcd.print("Soil Temp: ");
    lcd.print(soiltempf);
    lcd.println("F");
    delay(100);
    lcd.print("Soil Moisture: ");
    lcd.println(soilMoisture);
    lcd.println();
    delay(100);

    lcd.print("Rain: ");
    lcd.print(rainin);
    lcd.println("in.");
    lcd.println();
    delay(100);

    lcd.print("Altitude: ");
    lcd.print(altf);
    lcd.println("ft.");
    delay(100);

    lcd.print("Pressure: ");
    lcd.print(pascals);
    lcd.println("Pa");
    lcd.println();
    delay(100);


    //lcd.print("Daily Rain:");
    //lcd.print(dailyrainin, 2);
    //lcd.print("in., ");
}
//---------------------------------------------------------------
