/**********************************************
Graphic Serial LCD Libary Main File
Joel Bartlett
SparkFun Electronics
9-25-13

Updated for the Particle IDE on 7-2-15
**********************************************/
#include "SparkFun_Serial_Graphic_LCD.h"

LCD::LCD()
{
	Serial1.begin(115200);

}
//-------------------------------------------------------------------------------------------
void LCD::print(char Str[78])//26 characters is the length of one line on the LCD
{
	Serial1.print(Str);
	//if you need to print longer strings, change the size of this array here and in the .h file
}
//-------------------------------------------------------------------------------------------
void LCD::print(int num)//can't convert ints to strings so this is just for printing ints
{
	Serial1.print(num);
}
//-------------------------------------------------------------------------------------------
void LCD::print(double doub)//26 characters is the length of one line on the LCD
{
	Serial1.print(doub);
	//if you need to print longer strings, change the size of this array here and in the .h file
}
//-------------------------------------------------------------------------------------------
void LCD::print(float flo)//can't convert ints to strings so this is just for printing ints
{
	Serial1.print(flo);
}
//-------------------------------------------------------------------------------------------
void LCD::print(byte by)//26 characters is the length of one line on the LCD
{
	Serial1.print(by);
	//if you need to print longer strings, change the size of this array here and in the .h file
}
//-------------------------------------------------------------------------------------------
void LCD::print(long num)//can't convert ints to strings so this is just for printing ints
{
	Serial1.print(num);
}
//-------------------------------------------------------------------------------------------
/*void LCD::print(string str)//can't convert ints to strings so this is just for printing ints
{
	Serial1.print(str);
}*/
//-------------------------------------------------------------------------------------------
void LCD::print(unsigned int num)//26 characters is the length of one line on the LCD
{
	Serial1.print(num);
	//if you need to print longer strings, change the size of this array here and in the .h file
}
//-------------------------------------------------------------------------------------------
void LCD::print(unsigned long num)//can't convert ints to strings so this is just for printing ints
{
	Serial1.print(num);
}
//-------------------------------------------------------------------------------------------
void LCD::println(char Str[78])//26 characters is the length of one line on the LCD
{
	Serial1.println(Str);
	//if you need to print longer strings, change the size of this array here and in the .h file
}
//-------------------------------------------------------------------------------------------
void LCD::println(int num)//can't convert ints to strings so this is just for printing ints
{
	Serial1.println(num);
}
//-------------------------------------------------------------------------------------------
void LCD::println(double doub)//26 characters is the length of one line on the LCD
{
	Serial1.println(doub);
	//if you need to print longer strings, change the size of this array here and in the .h file
}
//-------------------------------------------------------------------------------------------
void LCD::println(float flo)//can't convert ints to strings so this is just for printing ints
{
	Serial1.println(flo);
}
//-------------------------------------------------------------------------------------------
void LCD::println(byte by)//26 characters is the length of one line on the LCD
{
	Serial1.println(by);
	//if you need to print longer strings, change the size of this array here and in the .h file
}
//-------------------------------------------------------------------------------------------
/*void LCD::println(string str)//can't convert ints to strings so this is just for printing ints
{
	Serial1.println(str);
}*/
//-------------------------------------------------------------------------------------------
void LCD::println(long num)//can't convert ints to strings so this is just for printing ints
{
	Serial1.println(num);
}
//-------------------------------------------------------------------------------------------
void LCD::println(unsigned int num)//26 characters is the length of one line on the LCD
{
	Serial1.println(num);
	//if you need to print longer strings, change the size of this array here and in the .h file
}
//-------------------------------------------------------------------------------------------
void LCD::println(unsigned long num)//can't convert ints to strings so this is just for printing ints
{
	Serial1.println(num);
}
//-------------------------------------------------------------------------------------------
void LCD::println()//can't convert ints to strings so this is just for printing ints
{
	Serial1.println();
}

//-------------------------------------------------------------------------------------------
void LCD::clearScreen()
{
  //clears the screen, you will use this a lot!
  Serial1.write(0x7C);
  Serial1.write((byte)0); //CTRL @
  //can't send LCD.write(0) or LCD.write(0x00) because it's interprestted as a NULL
}
//-------------------------------------------------------------------------------------------
void LCD::toggleReverseMode()
{
  //Everything that was black is now white and vise versa
  Serial1.write(0x7C);
  Serial1.write(0x12); //CTRL r
}
//-------------------------------------------------------------------------------------------
void LCD::toggleSplash()
{
  //turns the splash screen on and off, the 1 second delay at startup stays either way.
  Serial1.write(0x7C);
  Serial1.write(0x13); //CTRL s
}
//-------------------------------------------------------------------------------------------
void LCD::setBacklight(byte duty)
{
  //changes the back light intensity, range is 0-100.
  Serial1.write(0x7C);
  Serial1.write(0x02); //CTRL b
  Serial1.write(duty); //send a value of 0 - 100
}
//-------------------------------------------------------------------------------------------
void LCD::setBaud(byte baud)
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

  //these statements change the Serial 1 baud rate to match the baud rate of the LCD.
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
void LCD::restoreDefaultBaud()
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

delay(100);

Serial1.begin(9600);
Serial1.write(0x7C);
Serial1.write(0x07);
Serial1.write(54);//set back to 115200
Serial1.end();

delay(100);

Serial1.begin(19200);
Serial1.write(0x7C);
Serial1.write(0x07);
Serial1.write(54);//set back to 115200
Serial1.end();

delay(100);

Serial1.begin(38400);
Serial1.write(0x7C);
Serial1.write(0x07);
Serial1.write(54);//set back to 115200
Serial1.end();

delay(100);

Serial1.begin(57600);
Serial1.write(0x7C);
Serial1.write(0x07);
Serial1.write(54);//set back to 115200
Serial1.end();

delay(100);

Serial1.begin(115200);
delay(10);
Serial1.write(0x7C);
Serial1.write((byte)0); //clearScreen
Serial1.print("Baud restored to 115200!");
delay(5000);

}
//-------------------------------------------------------------------------------------------
void LCD::demo()
{
  //Demonstartes all the capabilities of the LCD
  Serial1.write(0x7C);
  Serial1.write(0x04);//CTRL d
}
//-------------------------------------------------------------------------------------------
void LCD::setX(byte posX) //0-127 or 0-159 pixels
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
void LCD::setY(byte posY)//0-63 or 0-127 pixels
{
  //Set the y position
  Serial1.write(0x7C);
  Serial1.write(0x19);//CTRL y
  Serial1.write(posY);

}
//-------------------------------------------------------------------------------------------
void LCD::setHome()
{
  Serial1.write(0x7C);
  Serial1.write(0x18);
  Serial1.write((byte)0);//set x back to 0

  Serial1.write(0x7C);
  Serial1.write(0x19);
  Serial1.write((byte)0);//set y back to 0
}
//-------------------------------------------------------------------------------------------
void LCD::setPixel(byte x, byte y, byte set)
{
  Serial1.write(0x7C);
  Serial1.write(0x10);//CTRL p
  Serial1.write(x);
  Serial1.write(y);
  Serial1.write(0x01);
  delay(10);
}
//-------------------------------------------------------------------------------------------
void LCD::drawLine(byte x1, byte y1, byte x2, byte y2, byte set)
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
void LCD::drawBox(byte x1, byte y1, byte x2, byte y2, byte set)
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
void LCD::drawCircle(byte x, byte y, byte rad, byte set)
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
void LCD::eraseBlock(byte x1, byte y1, byte x2, byte y2)
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
