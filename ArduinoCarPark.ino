//Corrispondono ai pin D1 e D2 del modulo wifi

#include <SPI.h>
#include <ESP8266WiFi.h>
#include <FirebaseArduino.h>
#include <LinkedList.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <SoftwareSerial.h>

#define FIREBASE_HOST "arduinocarpark-database.firebaseio.com"
#define FIREBASE_AUTH "NKb6SKiqBLxTyzmbQBgEuycPckt28FBdlMJSv3hP"
#define WIFI_SSID "prova"
#define WIFI_PASSWORD "auri1998"

SoftwareSerial s(D2, D3);

//Da questo client prendo l'ora
const long utcOffsetInSeconds = 3600;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

LinkedList<String> codici = LinkedList<String>(); //vengono salvati tutti i codici che hanno accesso al sistema
int indice_utente; //è il numero che identifica un utente
String accessi[200]; //vengono salvati tutti i codici che fanno accesso al parcheggio
int indice_acc = 1; //è il numero che identifica l'accesso da parte di un utente
String msg;

unsigned long previousMillis = 0; //will store last time LED was updated
unsigned long interval = 10000; //interval at which to blink (milliseconds)
unsigned long currentMillis;

/*                ***********************Setup*****************************             */
void setup(){
  Serial.begin(115200);   //inizio la comunicazione seriale (mi serve per il monitor seriale)
  s.begin(9600);
  SPI.begin();      //inizio la comunicazione via SPI
  
   
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
    String stringa;
    do{
      stringa = Firebase.getString("ID clienti registrati/" + String(i) + "/codice");
    }
    while(Firebase.failed());
    if (Firebase.success() && !stringa.equals("")){
      codici.add(stringa);
      Serial.println(codici.get(i-1));
    }
    else
      break;
    i++;
  }

  //recupero le informazioni in caso di reset spontaneo del sistema (rendo il sistema robusto)
  int j = 0, posti = 0;
  while(1){
    String codice_accesso_rec;
    do{
      codice_accesso_rec = Firebase.getString("ID clienti che hanno fatto accesso/" + String(j+1) + "/codice");
    }
    while(Firebase.failed());
    if (Firebase.success() && !codice_accesso_rec.equals("")){
      if(Firebase.getInt("ID clienti che hanno fatto accesso/" + String(j+1) + "/timestamp uscita") != 0)
        accessi[j] = "ok";
      else{
        accessi[j] = codice_accesso_rec;
        posti++;
      }
    }
    else
      break;
    j++;
  }

  indice_acc = j+1;

  do Firebase.setInt("Posti/posti occupati", posti);
  while(Firebase.failed());
  
}

/*                ***********************Loop*****************************          */
void loop(){

  delay(10);
  
  if(millis() - previousMillis > interval) {
    previousMillis = millis(); //save the last time you blinked the LED
    //fa un controllo delle richieste di pagamento
    for(int i = 1; i < indice_acc; i++){
      if(Firebase.getBool("ID clienti che hanno fatto accesso/" + String(i) + "/uscita/richiesta") == true){
        salvaOra("richiesta", i, true);
        calcolaPrezzo(i);
        Firebase.setBool("ID clienti che hanno fatto accesso/" + String(i) + "/uscita/richiesta", false);
      }
      if(Firebase.getInt("ID clienti che hanno fatto accesso/" + String(i) + "/timestamp intenzione pagamento") != 0){
        int tempo_passato = Firebase.getInt("ID clienti che hanno fatto accesso/" + String(i) + "/timestamp intenzione pagamento") - 
        Firebase.getInt("ID clienti che hanno fatto accesso/" + String(i) + "/uscita/timestamp richiesta");
        if(tempo_passato > 30){
          int prezzo_base = Firebase.getInt("ID clienti che hanno fatto accesso/" + String(i) + "/uscita/prezzo");
          Firebase.setInt("ID clienti che hanno fatto accesso/" + String(i) + "/uscita/supplemento",
          (prezzo_base + (1+tempo_passato/30) < 20 ? (1+tempo_passato/30) : 20 - prezzo_base));
          Firebase.remove("ID clienti che hanno fatto accesso/" + String(i) + "/timestamp intenzione pagamento");
        }
      }
      delay(1);
    }
  }
  //controlla se viene appoggiato un badge
  if(s.available() > 0){
    while(s.available() > 0){
      msg = msg + String(s.read());
      delay(500);
    }
  }
  else
    return;

  String content = msg;
  msg = "";
  Serial.println(content);

  //controlla che il codice scannerizzato sia all'interno della lista dei codici che hanno già fatto accesso, se sì esce
  for(int i = 0; i < indice_acc; i++){
    if(accessi[i].equals(content)){
      bool pagato;
      do pagato = Firebase.getBool("ID clienti che hanno fatto accesso/" + String(i+1) + "/pagato");
      while(Firebase.failed());
      if(pagato == false){
        Serial.println("Non hai pagato");
        return;
      }
       s.write(0x22);
       Serial.println("Arrivederci :)");
       accessi[i] = "ok";
       salvaOra("uscita", i+1, false);

       aggiornaPosti(false);

       return;
    }
  }

  //controlla che ci siano posti disponibili
  int posti_tot;
  do posti_tot = Firebase.getInt("Posti/posti totali");
  while(Firebase.failed());
  int posti_occ;
  do posti_occ = Firebase.getInt("Posti/posti occupati");
  while(Firebase.failed());
  if(posti_occ >= posti_tot){
    Serial.println("Parcheggio pieno");
    return;
  }
  
  //controlla che il codice scannerizzato sia all'interno della lista dei codici che ha il permesso di accedere
  bool esiste = false;
  for(int i = 0; i < codici.size(); i++){
    if(codici.get(i).equals(content)){
       esiste = true;
       indice_utente = i+1;
    }
  }
  if(esiste == false){
    Serial.println("Accesso negato");
    return;
  }
  
  //se la stringa corrisponde ad una dei codici che hanno accesso, l'accesso viene accordato, altrimenti non viene accordato
  //stampa 1 se entra
  s.write(0x11);
  accessi[indice_acc - 1] = content;
  aggiornaPosti(true);
    
  //stampa sul monitor
  Serial.println("Accesso consentito"); 
    
  //vengono mandati nel database i dati relativi all'accesso
  do Firebase.setString("ID clienti che hanno fatto accesso/" + String(indice_acc) + "/codice", content);
  while(Firebase.failed());
  salvaOra("arrivo", indice_acc, false);
  do Firebase.setBool("ID clienti che hanno fatto accesso/" + String(indice_acc) + "/pagato", false);
  while(Firebase.failed());
  do Firebase.setBool("ID clienti che hanno fatto accesso/" + String(indice_acc) + "/uscita/richiesta", false);
  while(Firebase.failed());
  Serial.print("Ciao ");
  do Serial.print(Firebase.getString("ID clienti registrati/" + String(indice_utente) + "/nome"));
  while(Firebase.failed());
  Serial.println(" :)");
  indice_acc++;
 
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

void salvaOra(String stringa, int indice, bool richiesta){
  timeClient.update();
  if(richiesta == false){
    do Firebase.setInt("ID clienti che hanno fatto accesso/" + String(indice) + "/timestamp " + stringa, timeClient.getEpochTime());
    while(Firebase.failed());
  }
  else{
    do Firebase.setInt("ID clienti che hanno fatto accesso/" + String(indice) + "/uscita/timestamp " + stringa, timeClient.getEpochTime());
    while(Firebase.failed());
  }
  
}

void calcolaPrezzo(int indice){
  int secondiTot;
  do secondiTot = Firebase.getInt("ID clienti che hanno fatto accesso/" + String(indice) + "/uscita/timestamp richiesta") -
  Firebase.getInt("ID clienti che hanno fatto accesso/" + String(indice) + "/timestamp arrivo");
  while(Firebase.failed());

  do Firebase.setInt("ID clienti che hanno fatto accesso/" + String(indice) + "/uscita/prezzo", (secondiTot/60+1));
  while(Firebase.failed());
  
}
