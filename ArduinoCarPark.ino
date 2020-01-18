//Corrispondono ai pin D1 e D2 del modulo wifi
#define SS_PIN 4
#define RST_PIN 5

#include <SPI.h>
#include <MFRC522.h>
#include <ESP8266WiFi.h>
#include <FirebaseArduino.h>
#include <Servo.h> 

//SS permette la comunicazione via SPI, il secondo permette il reset
MFRC522 mfrc522(SS_PIN, RST_PIN);

#define FIREBASE_HOST "arduinocarpark-database.firebaseio.com"
#define FIREBASE_AUTH "NKb6SKiqBLxTyzmbQBgEuycPckt28FBdlMJSv3hP"
#define WIFI_SSID "prova"
#define WIFI_PASSWORD "auri1998"

void setup(){
  Serial.begin(115200);   //inizio la comunicazione seriale (mi serve per il monitor)
  SPI.begin();      //inizio la comunicazione via SPI
  mfrc522.PCD_Init();   //inizializzo il mio rfid reader
  
  //sono i pin non utilizzati dal modulo wifi
  digitalWrite(D3,HIGH);
  digitalWrite(D8,LOW);
  digitalWrite(D4,LOW);
  pinMode(D3,OUTPUT);
  pinMode(D8,OUTPUT);
  pinMode(D4,OUTPUT);
   
  //connessione al mio wifi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("connecting"); 
  //finché non è ancora connesso, stampa dei puntini...
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  //a questo punto si è connesso
  Serial.println();
  Serial.print("connected: "); 
  Serial.println(WiFi.localIP());
  
  //inizializzo la comunicazione col mio database
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
}

void loop(){

  Serial.println(Firebase.getString("/ID clienti registrati/Aurora"));
  //controlla se viene appoggiato un badge
  if ( ! mfrc522.PICC_IsNewCardPresent())
    return;
  //cerca di leggere il badge
  if ( ! mfrc522.PICC_ReadCardSerial()) 
    return;
  
  //creato oggetto json
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& valueObject = jsonBuffer.createObject();
    
  //viene mostrato l'UID sul monitor
  Serial.println();
  Serial.print("UID tag: ");
  //viene costruita la stringa del codice
  String content= "";
  byte letter;
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
    content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  content.toUpperCase();
  Serial.println();
  //se la stringa corrisponde ad una dei codici che hanno accesso, l'accesso viene accordato, altrimenti non viene accordato
  if ((content.substring(1) == ("A7 50 80 63"))){
    digitalWrite(D3,LOW);
    digitalWrite(D8,LOW);
    digitalWrite(D4,HIGH);
    delay(100); 
    digitalWrite(D3,HIGH);
    delay(5000);
    digitalWrite(D4,LOW);
    
    //stampa sul monitor
    Serial.println("Access Granted :)"); 
    Serial.println(content.substring(1));
    
    //viene mandato il valore nel database
    String a = content.substring(1);
    Firebase.pushString("ID clienti che hanno fatto accesso", a);
        
    Serial.println("Welcome in our ArduinoCarPark");
    delay(1000);
    Serial.println(" ");
  } 
  else
    Serial.println("Access Denied :(");
  
  delay(5000);
  return;
}
