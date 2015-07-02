/*
Use this sketch to find the address(es) of any DS18B20 Temperature sensors
you have attached to your Photon or Photon Weather Shield
*/

#include "OneWire/OneWire.h"

OneWire ds = OneWire(D4);  // on pin 10 (a 4.7K resistor is necessary)
unsigned long lastUpdate = 0;
void setup() {
  Serial.begin(9600);
    while(!Serial.available()) SPARK_WLAN_Loop();
}

void loop() {

 unsigned long now = millis();
    if((now - lastUpdate) > 3000)
    {
        lastUpdate = now;
        byte i;
        byte present = 0;
        byte addr[8];

      if ( !ds.search(addr)) {
        Serial.println("No more addresses.");
        Serial.println();
        ds.reset_search();
        //delay(250);
        return;
      }
            // the first ROM byte indicates which chip
      switch (addr[0]) {
        case 0x10:
          Serial.println("Chip = DS18S20");  // or old DS1820
          break;
        case 0x28:
          Serial.println("Chip = DS18B20");
          break;
        case 0x22:
          Serial.println("Chip = DS1822");
          break;
        default:
          Serial.println("Device is not a DS18x20 family device.");
          return;
      }


      Serial.print("ROM = ");
      Serial.print("0x");
        Serial.print(addr[0],HEX);
      for( i = 1; i < 8; i++) {
        Serial.print(", 0x");
        Serial.print(addr[i],HEX);
      }

      if (OneWire::crc8(addr, 7) != addr[7]) {
          Serial.println("CRC is not valid!");
          return;
      }


    Serial.println();
      ds.reset();

    }
}
