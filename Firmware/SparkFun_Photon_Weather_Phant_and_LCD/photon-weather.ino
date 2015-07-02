#pragma SPARK_NO_PREPROCESSOR
#include "Adafruit_MPL3115A2.h"
#include "HTU21D.h"
#include "SparkFun-Spark-Phant.h"
#include "Particle-OneWire.h"
#include "particle-dallas-temperature.h"

#define ONE_WIRE_BUS D4
#define TEMPERATURE_PRECISION 11
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

int WDIR = A0;
int RAIN = D2;
int WSPEED = D3;
int SOIL_MOIST = A1;
int SOIL_MOIST_POWER = D5;

//Run I2C Scanner to get address of DS18B20(s)
//DeviceAddress inSoilThermometer = {0x28, 0x6F, 0xD1, 0x5E, 0x06, 0x00, 0x00, 0x76};//Proto Address
DeviceAddress inSoilThermometer = {0x28, 0xD5, 0xBE, 0x5F, 0x06, 0x00, 0x00, 0x4F};//Redboard Address

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
float humidity = 0; // [%]
float tempf = 0; // [temperature F]
float rainin = 0; // [rain inches over the past hour)] -- the accumulated rainfall in the past 60 min
volatile float dailyrainin = 0; // [rain inches so far today in local time]
double InTempC = 0;//original temperature in C from DS18B20
float soiltempf = 0;//converted temperature in F from DS18B20
float pascals = 0;
float altf = 0;
float baroTemp = 0;
long lastWindCheck = 0;
unsigned int soilMoisture = 0;

// volatiles are subject to modification by IRQs
volatile long lastWindIRQ = 0;
volatile byte windClicks = 0;
volatile unsigned long raintime, rainlast, raininterval, rain;

////////////PHANT STUFF/////////////////////////////////////////////////////////////////////
const char server[] = "data.sparkfun.com";
const char publicKey[] = "Jx9gp6aw9Ji17qr4dYYR";
const char privateKey[] = "gzeZMw0keqC1zYo2Gxxy";
Phant phant(server, publicKey, privateKey);
/////////////////////////////////////////////////////////////////////////////////////////

HTU21D htu = HTU21D();//create instance of HTU21D Temp and humidity sensor
Adafruit_MPL3115A2 baro = Adafruit_MPL3115A2();//create instance of MPL3115A2 barrometric sensor

//Declare all functions, won't compile without these
void update18B20Temp(DeviceAddress deviceAddress, double &tempC);
int postToPhant();
void printInfo();
void getTempHumidity();
void getSoilTemp();
void getBaro();
void calcWeather();
float get_wind_speed();
int get_wind_direction();
void rainIRQ();
void wspeedIRQ();
void printLCD();
void clearScreen();
void toggleReverseMode();
void toggleSplash();
void setBacklight(byte duty);
void setBaud(byte baud);
void restoreDefaultBaud();
void setX(byte posX);
void setY(byte posY);
void setHome();
void demo();
void setPixel(byte x, byte y, byte set);
void drawLine(byte x1, byte y1, byte x2, byte y2, byte set);
void drawBox(byte x1, byte y1, byte x2, byte y2, byte set);
void drawCircle(byte x, byte y, byte rad, byte set);
void eraseBlock(byte x1, byte y1, byte x2, byte y2);
void getSoilMositure();

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
    Serial1.begin(115200);  //Serial port for Graphic LCD
    //setBaud(2);

    pinMode(WSPEED, INPUT_PULLUP); // input from wind meters windspeed sensor
    pinMode(RAIN, INPUT_PULLUP); // input from wind meters rain gauge sensor

    pinMode(SOIL_MOIST_POWER, OUTPUT);
    digitalWrite(SOIL_MOIST_POWER, LOW);

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

  seconds = 0;
  lastSecond = millis();

  // attach external interrupt pins to IRQ functions
  attachInterrupt(D2, rainIRQ, FALLING);
  attachInterrupt(D3, wspeedIRQ, FALLING);

  // turn on interrupts
  interrupts();

  if(Time.month() < 3 || Time.month() > 10)//Daylight Savings Time
  Time.zone(-7);//Denver time zone
  else
  Time.zone(-6);//Denver time zone DST
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

    //Report all readings every second
    calcWeather();
    printInfo();
    printLCD();
    postToPhant();
  }
    //delay(600000);//post every 10 minutes
    delay(10000);//every 10 seconds
}
//---------------------------------------------------------------
int postToPhant()
{
    pascals = pascals/3377;

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
void printInfo()
{
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
    /*Serial.print(",windgustmph=");
    Serial.print(windgustmph, 1);
    Serial.print(",windgustdir=");
    Serial.print(windgustdir);
    Serial.print(",windspdmph_avg2m=");
    Serial.print(windspdmph_avg2m, 1);
    Serial.print(",winddir_avg2m=");
    Serial.print(winddir_avg2m);
    Serial.print(",windgustmph_10m=");
    Serial.print(windgustmph_10m, 1);
    Serial.print(",windgustdir_10m=");
    Serial.print(windgustdir_10m);*/
    Serial.print("Temp:");
    Serial.print((tempf+baroTemp)/2);
    Serial.print("F, ");
    Serial.print("Humidity:");
    Serial.print(humidity);
    Serial.print("%, ");
    //Serial.print("Baro Temp: ");
    //Serial.print(baroTemp);
    //Serial.print("F, ");
    Serial.print("Pressure:");
    Serial.print(pascals/3377);
    Serial.print("Inches(Hg), ");
    Serial.print("Altitude:");
    Serial.print(altf);
    Serial.print("ft., ");
    Serial.print("Rain:");
    Serial.print(rainin, 2);
    Serial.print("in., ");
    //Serial.print("Daily Rain:");
    //Serial.print(dailyrainin, 2);
    //Serial.print("in., ");
    Serial.print("Soil Temp:");
    Serial.print(soiltempf);
    Serial.print("F, ");
    Serial.print("Soil Mositure:");
    Serial.println(soilMoisture);
}
//---------------------------------------------------------------
void printLCD()
{
    clearScreen();
    delay(200);
    setHome();
    delay(200);
    Serial1.println(Time.timeStr());
    Serial1.println();
    delay(200);

    Serial1.print("Wind Dir: ");
    switch (winddir)
    {
      case 0:
        Serial1.println("North");
        break;
      case 1:
        Serial1.println("NE");
        break;
      case 2:
        Serial1.println("East");
        break;
      case 3:
        Serial1.println("SE");
        break;
      case 4:
        Serial1.println("South");
        break;
      case 5:
        Serial1.println("SW");
        break;
      case 6:
        Serial1.println("West");
        break;
      case 7:
        Serial1.println("NW");
        break;
      default:
        Serial1.println("No Wind");
        // if nothing else matches, do the
        // default (which is optional)
    }
    delay(100);

    Serial1.print("Wind Speed: ");
    Serial1.print(windspeedmph, 1);
    Serial1.println("mph");
    Serial1.println();
    delay(100);
    /*Serial1.print(",windgustmph=");
    Serial1.print(windgustmph, 1);
    Serial1.print(",windgustdir=");
    Serial1.print(windgustdir);
    Serial1.print(",windspdmph_avg2m=");
    Serial1.print(windspdmph_avg2m, 1);
    Serial1.print(",winddir_avg2m=");
    Serial1.print(winddir_avg2m);
    Serial1.print(",windgustmph_10m=");
    Serial1.print(windgustmph_10m, 1);
    Serial1.print(",windgustdir_10m=");
    Serial1.print(windgustdir_10m);*/
    Serial1.print("Temp: ");
    Serial1.print((tempf+baroTemp)/2);
    Serial1.println("F");
    delay(100);

    Serial1.print("Humidity: ");
    Serial1.print(humidity);
    Serial1.println("%");
    Serial1.println();
    delay(100);
    //Serial1.print("Baro Temp: ");
    //Serial1.print(baroTemp);
    //Serial1.print("F, ");


    Serial1.print("Soil Temp: ");
    Serial1.print(soiltempf);
    Serial1.println("F");
    delay(100);
    Serial1.print("Soil Moisture: ");
    Serial1.println(soilMoisture);
    Serial1.println();
    delay(100);

    Serial1.print("Rain: ");
    Serial1.print(rainin, 2);
    Serial1.println("in.");
    Serial1.println();
    delay(100);

    Serial1.print("Altitude: ");
    Serial1.print(altf);
    Serial1.println("ft.");
    delay(100);

    Serial1.print("Pressure: ");
    Serial1.print(pascals/3377);
    Serial1.println("in.(Hg)");
    Serial1.println();
    delay(100);


    //Serial1.print("Daily Rain:");
    //Serial1.print(dailyrainin, 2);
    //Serial1.print("in., ");

    //Serial1.flush();


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










//LCD


//-------------------------------------------------------------------------------------------
void clearScreen()
{
  //clears the screen, you will use this a lot!
  Serial1.write(0x7C);
  Serial1.write(0x00); //CTRL @
  //can't send LCD.write(0) or LCD.write(0x00) because it's interprestted as a NULL
}
//-------------------------------------------------------------------------------------------
void toggleReverseMode()
{
  //Everything that was black is now white and vise versa
  Serial1.write(0x7C);
  Serial1.write(0x12); //CTRL r
}
//-------------------------------------------------------------------------------------------
void toggleSplash()
{
  //turns the splash screen on and off, the 1 second delay at startup stays either way.
  Serial1.write(0x7C);
  Serial1.write(0x13); //CTRL s
}
//-------------------------------------------------------------------------------------------
void setBacklight(byte duty)
{
  //changes the back light intensity, range is 0-100.
  Serial1.write(0x7C);
  Serial1.write(0x02); //CTRL b
  Serial1.write(duty); //send a value of 0 - 100
}
//-------------------------------------------------------------------------------------------
void setBaud(byte baud)
{
  //changes the baud rate.
  Serial1.write(0x7C);
  Serial1.write(0x07); //CTRL g
  Serial1.write(baud); //send a value of 49 - 54
  delay(100);

/*
“1” = 4800bps - 0x31 = 49
“2” = 9600bps - 0x32 = 50
“3” = 19,200bps - 0x33 = 51
“4” = 38,400bps - 0x34 = 52
“5” = 57,600bps - 0x35 = 53
“6” = 115,200bps - 0x36 = 54
*/

  //these statements change the SoftwareSerial1 baud rate to match the baud rate of the LCD.
  if(baud == 49)
  {
	Serial1.end();
	Serial1.begin(4800);
  }
  if(baud == 50)
  {
	Serial1.end();
	Serial1.begin(9600);
  }
  if(baud == 51)
  {
	Serial1.end();
	Serial1.begin(19200);
  }
  if(baud == 52)
  {
	Serial1.end();
	Serial1.begin(38400);
  }
  if(baud == 53)
  {
	Serial1.end();
	Serial1.begin(57600);
  }
  if(baud == 54)
  {
	Serial1.end();
	Serial1.begin(115200);
  }
}
//-------------------------------------------------------------------------------------------
void restoreDefaultBaud()
{
//This function is used to restore the default baud rate in case you change it
//and forget to which rate it was changed.


Serial1.end();//end the transmission at whatever the current baud rate is

//cycle through every other possible buad rate and attemp to change the rate back to 115200
Serial1.begin(4800);
Serial1.write(0x7C);
Serial1.write(0x07);
Serial1.write(54);//set back to 115200
Serial1.end();

Serial1.begin(9600);
Serial1.write(0x7C);
Serial1.write(0x07);
Serial1.write(54);//set back to 115200
Serial1.end();

Serial1.begin(19200);
Serial1.write(0x7C);
Serial1.write(0x07);
Serial1.write(54);//set back to 115200
Serial1.end();

Serial1.begin(38400);
Serial1.write(0x7C);
Serial1.write(0x07);
Serial1.write(54);//set back to 115200
Serial1.end();

Serial1.begin(57600);
Serial1.write(0x7C);
Serial1.write(0x07);
Serial1.write(54);//set back to 115200
Serial1.end();

Serial1.begin(115200);
delay(10);
Serial1.write(0x7C);
Serial1.write(0x00); //clearScreen
Serial1.print("Baud restored to 115200!");
delay(5000);

}
//-------------------------------------------------------------------------------------------
void demo()
{
  //Demonstartes all the capabilities of the LCD
  Serial1.write(0x7C);
  Serial1.write(0x04);//CTRL d
}
//-------------------------------------------------------------------------------------------
void setX(byte posX) //0-127 or 0-159 pixels
{
  //Set the X position
  Serial1.write(0x7C);
  Serial1.write(0x18);//CTRL x
  Serial1.write(posX);

//characters are 8 pixels tall x 6 pixels wide
//The top left corner of a char is where the x/y value will start its print
//For example, if you print a char at position 1,1, the bottom right of your char will be at position 7,9.
//Therefore, to print a character in the very bottom right corner, you would need to print at the coordinates
//x = 154 , y = 120. You should never exceed these values.


// Here we have an example using an upper case 'B'. The star is where the character starts, given a set
//of x,y coordinates. # represents the blocks that make up the character, and _ represnets the remaining
//unused bits in the char space.
//    *###__
//    #   #_
//    #   #_
//    ####__
//    #   #_
//    #   #_
//    ####__
//    ______
}
//-------------------------------------------------------------------------------------------
void setY(byte posY)//0-63 or 0-127 pixels
{
  //Set the y position
  Serial1.write(0x7C);
  Serial1.write(0x19);//CTRL y
  Serial1.write(posY);

}
//-------------------------------------------------------------------------------------------
void setHome()
{
  Serial1.write(0x7C);
  Serial1.write(0x18);
  Serial1.write((byte)0);//set x back to 0

  Serial1.write(0x7C);
  Serial1.write(0x19);
  Serial1.write(0x00);//set y back to 0
}
//-------------------------------------------------------------------------------------------
void setPixel(byte x, byte y, byte set)
{
  Serial1.write(0x7C);
  Serial1.write(0x10);//CTRL p
  Serial1.write(x);
  Serial1.write(y);
  Serial1.write(0x01);
  delay(10);
}
//-------------------------------------------------------------------------------------------
void drawLine(byte x1, byte y1, byte x2, byte y2, byte set)
{
  //draws a line from two given points. You can set and reset just as the pixel function.
  Serial1.write(0x7C);
  Serial1.write(0x0C);//CTRL l
  Serial1.write(x1);
  Serial1.write(y1);
  Serial1.write(x2);
  Serial1.write(y2);
  Serial1.write(0x01);
  delay(10);

}
//-------------------------------------------------------------------------------------------
void drawBox(byte x1, byte y1, byte x2, byte y2, byte set)
{
  //draws a box from two given points. You can set and reset just as the pixel function.
  Serial1.write(0x7C);
  Serial1.write(0x0F);//CTRL o
  Serial1.write(x1);
  Serial1.write(y1);
  Serial1.write(x2);
  Serial1.write(y2);
  Serial1.write(0x01);
  delay(10);

}
//-------------------------------------------------------------------------------------------
void drawCircle(byte x, byte y, byte rad, byte set)
{
//draws a circle from a point x,y with a radius of rad.
//Circles can be drawn off-grid, but only those pixels that fall within the
//display boundaries will be written.
  Serial1.write(0x7C);
  Serial1.write(0x03);//CTRL c
  Serial1.write(x);
  Serial1.write(y);
  Serial1.write(rad);
  Serial1.write(0x01);
  delay(10);

}
//-------------------------------------------------------------------------------------------
void eraseBlock(byte x1, byte y1, byte x2, byte y2)
{
  //This is just like the draw box command, except the contents of the box are erased to the background color
  Serial1.write(0x7C);
  Serial1.write(0x05);//CTRL e
  Serial1.write(x1);
  Serial1.write(y1);
  Serial1.write(x2);
  Serial1.write(y2);
  delay(10);

}
