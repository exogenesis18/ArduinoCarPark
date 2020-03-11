#include "SoftwareSerial.h"
#include <RFID.h>
#include <SPI.h>

#define SS_PIN1 9
#define RST_PIN 8


SoftwareSerial s(10,11); //Rx, Tx
RFID RC522(SS_PIN1, RST_PIN);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  s.begin(9600);
  SPI.begin(); 
  RC522.init();   //inizializzo il mio rfid reader
  
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  if ( RC522.isCard()){
   if ( RC522.readCardSerial()){
    s.print(leggiCodice());
    Serial.println(leggiCodice());
    delay(15000);
   }
  }
}

int leggiCodice(){
  
  //viene costruita la stringa del codice
  int content = 0, c = 10000;
  for(int i=0;i < 4;i++){
    content+= RC522.serNum[i] * c;
    c = c * 10000;
  }

  if(content < 0)
    content = content * -1;
    
  return content;
}
