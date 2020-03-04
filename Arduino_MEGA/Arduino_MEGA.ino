#include "SoftwareSerial.h"

SoftwareSerial s(10,11); //Rx, Tx

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  s.begin(9600);
  
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
}

void loop() {
  // put your main code here, to run repeatedly:
   if(s.available() > 0){

      //float val = ArduinoUno.parseFloat();

      Serial.println(s.read());
   }
}
