#include "SoftwareSerial.h"
#include <LiquidCrystal.h>
#include <RFID.h>
#include <SPI.h>
#include <Servo.h>

#define SS_PIN1 8
#define RST_PIN1 42

#define SS_PIN2 49
#define RST_PIN2 43

#define RXTSOP 40      
#define TXIR 10  


RFID RC522_1(SS_PIN1, RST_PIN1);
RFID RC522_2(SS_PIN2, RST_PIN2);

Servo servo_entrata;
Servo servo_uscita;
SoftwareSerial s(11,12); //Rx, Tx
LiquidCrystal lcd(41, 7, 5, 4, 3, 2);

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

  pinMode(TXIR, OUTPUT);               // Imposta il Pin come uscita
  pinMode(RXTSOP, INPUT);    
  TCCR2A = _BV(WGM21) | _BV(COM2A0) | _BV(COM2B0); // Timer impost. 
                                                   // a 01010010
  TCCR2B = _BV(CS20);                  // No prescaler
  OCR2A = 210;                         // Il pin 11 è settato per emettere 
                                       // un segnale a 38 khz
  OCR2B = 210;                         // Il pin 3 è settato per emettere 
                                       // un segnale a 38 khz

  delay(5000);
  
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
     posti_tot--;
     stampaSuDisplay();
     Serial.println("Qualcuno è entrato");
     digitalWrite(46, LOW);
     digitalWrite(47, HIGH);
     servo_entrata.write(18);
     delay(2000);
     digitalWrite(46, HIGH);
     digitalWrite(47, LOW);
     servo_entrata.write(98);
   }
   else if(n == 3400){
    posti_tot++;
    stampaSuDisplay();
    Serial.println("Qualcuno è uscito");
    digitalWrite(44, LOW);
    digitalWrite(45, HIGH);
    servo_uscita.write(18);
    delay(2000);
    digitalWrite(44, HIGH);
    digitalWrite(45, LOW);
    servo_uscita.write(98);
  }
 }

  int res = digitalRead(RXTSOP);
  delay(200);
  if(res == 0)
    Serial.println("Abbassa sbarra");
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
