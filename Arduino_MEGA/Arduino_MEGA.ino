#include "SoftwareSerial.h"
#include <LiquidCrystal.h>
#include <RFID.h>
#include <SPI.h>
#include <Servo.h>

#define SS_PIN1 9
#define RST_PIN1 13

#define SS_PIN2 49
#define RST_PIN2 8

RFID RC522_1(SS_PIN1, RST_PIN1);
RFID RC522_2(SS_PIN2, RST_PIN2);

Servo servo_entrata;
Servo servo_uscita;
SoftwareSerial s(10,11); //Rx, Tx
LiquidCrystal lcd(12, 7, 5, 4, 3, 2);

int posti_tot = 10;
int p = 0;

unsigned long previousMillis = 0; //will store last time LED was updated
unsigned long interval = 5000; //interval at which to blink (milliseconds)
unsigned long currentMillis;

/*                ***********************Setup*****************************             */

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  SPI.begin();   
  RC522_1.init();
  RC522_2.init();
  servo_entrata.attach(48);
  servo_entrata.write(98);
  servo_uscita.attach(53);
  servo_uscita.write(98);
  lcd.begin(16, 2);
  lcd.print("Posti liberi:");
  lcd.setCursor(0, 1);
  lcd.print("10/10");
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
 if(millis() - previousMillis > interval) {
  previousMillis = millis(); //save the last time you blinked the LED
  if ((RC522_1.isCard() && RC522_1.readCardSerial()) || (RC522_2.isCard() && RC522_2.readCardSerial())) {
    Serial.println(leggiCodice());
    s.print(leggiCodice());
   }
  delay(50);
 }
 
 if(s.available() > 0){
   int n = s.read();
   n = n * 100;
   if(n == 1700){
     stampaSuDisplay();
     Serial.println("Qualcuno è entrato");
     digitalWrite(46, LOW);
     digitalWrite(47, HIGH);
     servo_entrata.write(18);
     posti_tot--;
     delay(2000);
     digitalWrite(46, HIGH);
     digitalWrite(47, LOW);
     servo_entrata.write(98);
   }
   else if(n == 3400){
    stampaSuDisplay();
    Serial.println("Qualcuno è uscito");
    digitalWrite(44, LOW);
    digitalWrite(45, HIGH);
    servo_uscita.write(18);
    posti_tot++;
    delay(2000);
    digitalWrite(44, HIGH);
    digitalWrite(45, LOW);
    servo_uscita.write(98);
  }
 }
}

/*                    **************Funzioni Ausiliarie************************    */

int leggiCodice(){
  
  //viene costruita la stringa del codice
  int content = 0, c = 10000;
  for(int i=0;i < 4;i++){
    if(RC522_1.readCardSerial())
      content+= RC522_1.serNum[i] * c;
    else
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
