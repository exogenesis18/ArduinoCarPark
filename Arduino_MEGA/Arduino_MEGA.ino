#include "SoftwareSerial.h"
#include <LiquidCrystal.h>
#include <RFID.h>
#include <SPI.h>
#include <Servo.h>

#define SS_PIN1 8
#define RST_PIN1 5

#define SS_PIN2 10
#define RST_PIN2 5

RFID RC522_1(SS_PIN1, RST_PIN1);
RFID RC522_2(SS_PIN2, RST_PIN2);

Servo servo_entrata;
Servo servo_uscita;
SoftwareSerial s(11,12); //Rx, Tx
LiquidCrystal lcd(41, 7, 9, 4, 3, 2);

int posti_tot = 20;
int p = 0;

unsigned long previousMillis1 = 0; //will store last time LED was updated
unsigned long previousMillis2 = 0;
unsigned long interval = 5000; //interval at which to blink (milliseconds)
unsigned long currentMillis;

/*                ***********************Setup*****************************             */

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  SPI.begin();   
  RC522_1.init();
  RC522_2.init();
  servo_entrata.attach(37);
  servo_entrata.write(98);
  servo_uscita.attach(24);
  servo_uscita.write(98);
  lcd.begin(16, 2);
  lcd.print("Posti liberi:");
  lcd.setCursor(0, 1);
  lcd.print("20/20");
  analogWrite(6, 20);
  s.begin(9600);
  pinMode(46, OUTPUT);
  digitalWrite(46, HIGH);
  pinMode(47, OUTPUT);
  pinMode(44, OUTPUT);
  digitalWrite(44, HIGH);
  pinMode(45, OUTPUT);
  
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
}

/*                ***********************Loop*****************************          */

void loop() {

  if (RC522_1.isCard() && RC522_1.readCardSerial()) {
    if(millis() - previousMillis1 > interval) {
      previousMillis1 = millis(); //save the last time you blinked the LED
      Serial.println(leggiCodice1());
       s.print(leggiCodice1());
    }
    RC522_1.halt();
  }
  if (RC522_2.isCard() && RC522_2.readCardSerial()){
    if(millis() - previousMillis2 > interval) {
       previousMillis2 = millis(); //save the last time you blinked the LED
        Serial.println(leggiCodice2());
        s.print(leggiCodice2());
    }
    RC522_2.halt();
  }
  delay(50);
 
 if(s.available() > 0){
   int n = s.read();
   if(n == 17){
     posti_tot--;
     stampaSuDisplay();
     Serial.println("Qualcuno è entrato");
     digitalWrite(46, LOW);
     digitalWrite(47, HIGH);
     servo_entrata.write(18);
     delay(5000);
     servo_entrata.write(98);
     digitalWrite(46, HIGH);
     digitalWrite(47, LOW);
   }
   else if(n == 34){
    posti_tot++;
    stampaSuDisplay();
    Serial.println("Qualcuno è uscito");
    digitalWrite(44, LOW);
    digitalWrite(45, HIGH);
    servo_uscita.write(178);
    delay(5000);
    servo_uscita.write(98);
    digitalWrite(44, HIGH);
    digitalWrite(45, LOW);
  }
 }
}

/*                    **************Funzioni Ausiliarie************************    */

int leggiCodice1(){
  
  //viene costruita la stringa del codice
  int content = 0, c = 10000;
  for(int i=0;i < 4;i++){
    content+= RC522_1.serNum[i] * c;
    c = c * 10000;
  }
  if(content < 0)
    content = content * -1;
    
  return content;
}

int leggiCodice2(){
  
  //viene costruita la stringa del codice
  int content = 0, c = 10000;
  for(int i=0;i < 4;i++){
    content+= RC522_2.serNum[i] * c;
    c = c * 10000;
  }
  if(content < 0)
    content = content * -1;
    
  return content;
}

void stampaSuDisplay(){
  if(posti_tot < 10){
    lcd.setCursor(1, 1);
    lcd.print(' ');
   }
  lcd.setCursor(0, 1);
  lcd.print(posti_tot);
}
