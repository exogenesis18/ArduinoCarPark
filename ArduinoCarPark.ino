//Corrispondono ai pin D1 e D2 del modulo wifi
#define SS_PIN 4
#define RST_PIN 5

#include <SPI.h>
#include <MFRC522.h>
#include <ESP8266WiFi.h>
#include <FirebaseArduino.h>
#include <LinkedList.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <SoftwareSerial.h>

//SS permette la comunicazione via SPI, il secondo permette il reset
MFRC522 mfrc522(SS_PIN, RST_PIN);
SoftwareSerial s(3, 1);

#define FIREBASE_HOST "arduinocarpark-database.firebaseio.com"
#define FIREBASE_AUTH "NKb6SKiqBLxTyzmbQBgEuycPckt28FBdlMJSv3hP"
#define WIFI_SSID "prova"
#define WIFI_PASSWORD "auri1998"

//Da questo client prendo l'ora
const long utcOffsetInSeconds = 3600;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

LinkedList<String> codici = LinkedList<String>();
LinkedList<String> accessi = LinkedList<String>();
String codici_effettuato_accesso[1000];
int num_codici;
int num_accessi;
int indice;
int indice_acc = 1;

/*                ***********************Setup*****************************             */
void setup(){
  Serial.begin(9600);   //inizio la comunicazione seriale (mi serve per il monitor seriale)
  s.begin(9600);
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
  //inizializzo la comunicazione con il server che fornisce l'ora
  timeClient.begin();

  //vedo quanti utenti sono registrati e salvo i loro codici in un array;
  int i = 1;
  Serial.println();
  Serial.println("Lista dei codici di utenti registrati:");
  while(1){
    String stringa = Firebase.getString("ID clienti registrati/" + String(i) + "/codice");
    if (Firebase.success() && !stringa.equals("")){
      codici.add(stringa);
      Serial.println(codici.get(i-1));
    }
    else
      break;
    i++;
  }
  //salvo il numero di codici
  num_codici = codici.size();
  do Firebase.setInt("Posti/posti occupati", 0);
  while(Firebase.failed());

  //inizializzo gli accessi
  Firebase.remove("ID clienti che hanno fatto accesso/");
  
}

/*                ***********************Loop*****************************          */
void loop(){
  
  //controlla se viene appoggiato un badge
  if ( ! mfrc522.PICC_IsNewCardPresent())
    return;
  //cerca di leggere il badge
  if ( ! mfrc522.PICC_ReadCardSerial()) 
    return;
    
  //viene letto il codice e mostrato su monitor
  String content = leggiCodice();

  //controlla che il codice scannerizzato sia all'interno della lista dei codici che hanno già fatto accesso, se sì esce
  for(int i = 0; i < num_accessi; i++){
    if(accessi.get(i).equals(content.substring(1))){
       Serial.println("Arrivederci :)");
       accessi.remove(i);
       num_accessi--;
       int c = 0;
       while(codici_effettuato_accesso){
        if(codici_effettuato_accesso[c].equals(content.substring(1))){
          codici_effettuato_accesso[c] = "ok";
          salvaOra("uscita", c+1);
          calcolaPrezzo(c+1);
          break;
        }
        c++;
       }

       aggiornaPosti(false);

       delay(5000);
       return;
    }
  }
  int posti_tot;
  do posti_tot = Firebase.getInt("Posti/posti totali");
  while(Firebase.failed());
  int posti_occ;
  do posti_occ = Firebase.getInt("Posti/posti occupati");
  while(Firebase.failed());
  if(posti_occ >= posti_tot){
    Serial.println("Parcheggio pieno");
    delay(5000);
    return;
  }
  
  //controlla che il codice scannerizzato sia all'interno della lista dei codici che ha il permesso di accedere
  bool esiste = false;
  for(int i = 0; i < num_codici; i++){
    if(codici.get(i).equals(content.substring(1))){
       esiste = true;
       indice = i+1;
    }
  }
  if(esiste == false){
    Serial.println("Accesso negato");
    delay(5000);
    return;
  }
  
  //se la stringa corrisponde ad una dei codici che hanno accesso, l'accesso viene accordato, altrimenti non viene accordato
  s.write(1);
  accessi.add(content.substring(1));
  num_accessi++;
  aggiornaPosti(true);
    
  digitalWrite(D3,LOW);
  digitalWrite(D8,LOW);
  digitalWrite(D4,HIGH);
  delay(100); 
  digitalWrite(D3,HIGH);
  delay(5000);
  digitalWrite(D4,LOW);
    
  //stampa sul monitor
  Serial.println("Accesso consentito"); 
    
  //vengono mandati nel database i dati relativi all'accesso
  String a = content.substring(1);
  do Firebase.setString("ID clienti che hanno fatto accesso/" + String(indice_acc) + "/codice", a);
  while(Firebase.failed());
  salvaOra("arrivo", indice_acc);
  codici_effettuato_accesso[indice_acc - 1] = content.substring(1);
  Serial.print("Ciao ");
  do Serial.print(Firebase.getString("ID clienti registrati/" + String(indice) + "/nome"));
  while(Firebase.failed());
  Serial.println(" :)");
  indice_acc++;
  
  delay(5000);
  return;
}

/*                    **************Funzioni Ausiliarie************************    */  
void aggiornaPosti(bool entra){
  do Firebase.setInt("Posti/posti occupati", Firebase.getInt("Posti/posti occupati") + (entra == true ? 1 : -1));
  while(Firebase.failed());
  Serial.print("Posti occupati adesso: ");
  do Serial.print(Firebase.getInt("Posti/posti occupati"));
  while(Firebase.failed());
  Serial.print("/");
  do Serial.println(Firebase.getInt("Posti/posti totali"));
  while(Firebase.failed());
}

void salvaOra(String stringa, int indice){
  timeClient.update();
  do Firebase.setInt("ID clienti che hanno fatto accesso/" + String(indice) + "/timestamp " + stringa, timeClient.getEpochTime());
  while(Firebase.failed());
  
}

void calcolaPrezzo(int indice){
  int secondiTot;
  do secondiTot = Firebase.getInt("ID clienti che hanno fatto accesso/" + String(indice) + "/timestamp uscita") -
  Firebase.getInt("ID clienti che hanno fatto accesso/" + String(indice) + "/timestamp arrivo");
  while(Firebase.failed());

  do Firebase.setInt("ID clienti che hanno fatto accesso/" + String(indice) + "/prezzo", (secondiTot/60+1));
  while(Firebase.failed());
  
}

String leggiCodice(){
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

  return content;
}
