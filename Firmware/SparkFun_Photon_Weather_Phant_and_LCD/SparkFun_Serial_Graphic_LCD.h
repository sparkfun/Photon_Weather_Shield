/**********************************************
Graphic Serial LCD Libary Header File
Joel Bartlett
SparkFun Electronics
9-25-13

Updated for the Particle IDE on 7-2-15
**********************************************/

#ifndef LCD_h
#define LCD_h

#include "application.h"


class LCD
{
	public:
	LCD();
	void print(char Str[78]);
	void print(int num);
	void print(double doub);
	void print(float flo);
	void print(byte by);
	void print(long num);
	//void print(string str);
	void print(unsigned int num);
	void print(unsigned long num);

	void println(char Str[78]);
	void println(int num);
	void println(double doub);
	void println(float flo);
	void println(byte by);
	void println(long num);
	//void println(string str);
	void println(unsigned int num);
	void println(unsigned long num);
	void println();

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


	private:




};

#endif
