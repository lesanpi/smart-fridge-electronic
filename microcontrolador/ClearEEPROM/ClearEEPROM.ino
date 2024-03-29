#include <EEPROM.h>

void setup() {
  EEPROM.begin(512);
  // write a 0 to all 512 bytes of the EEPROM
  for (int i = 0; i < EEPROM.length(); i++) { EEPROM.write(i, 0); }

  // turn the LED on when we're done
  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);
  EEPROM.end();
}

void loop() {}
// /dev/cu.usbserial-0001