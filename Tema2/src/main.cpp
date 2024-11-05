#include <Arduino.h>

#define BAUD 28800

#define LED_ROSU 11
#define LED_VERDE 10
#define LED_ALBASTRU 9

#define BUTON_START 2
#define BUTON_DIFICULTATE 3

#define BACKSPACE 8

// Timpurile și intervalele utilizate în joc
unsigned long timpTastare[3] = {5000, 4000, 3000}; // Limite de timp în funcție de dificultate
unsigned long timpJoc = 30000;                     // Durata totală a jocului în ms
unsigned long timpDebounce = 300;                  // Timp pentru debouncing
unsigned long timpIncepere = 3000;                 // Durata numărătorii inverse la începutul jocului
unsigned long momentApasareDificultate = 0, momentApasareStart = 0, momentIncepereJoc = 0;
unsigned long momentUltimaClipire = 0, momentSelectareCuvant = 0;

char cuvant[30] = "";                              // Cuvântul introdus de jucător
char cuvantTinta[30];                              // Cuvântul țintă de scris
char dictionar[10][30] = {"robot", "masina", "calculator", "cuvant", "lumina", "energie", "fizica", "tastatura", "copac", "programare"};
char numeDificultati[3][10] = {"\nUsor\n", "\nMediu\n", "\nGreu\n"};

int dificultate = 0, stareLed = 0, clipiri = 0, scor = 0, index = 0;
bool repaus = true, rulare = false;

// Functie pentru setarea culorii LED-ului RGB
void setRGB(int valRosu, int valVerde, int valAlbastru) {
  analogWrite(LED_ROSU, valRosu);
  analogWrite(LED_VERDE, valVerde);
  analogWrite(LED_ALBASTRU, valAlbastru);
}

// Functie pentru verificarea cuvantului introdus
int verificaCuvant(const char *cuvantDeVerificat) {
  int lungime = strlen(cuvantDeVerificat);
  if (strncmp(cuvantDeVerificat, cuvantTinta, lungime) != 0)
    return -1; // Cuvantul nu corespunde
  return lungime == strlen(cuvantTinta) ? 1 : 0; // Cuvantul este corect doar daca lungimile coincid
}

// Functie pentru finalizarea jocului
void finalizare() {
  Serial.print("\nTerminat!\nScor: ");
  Serial.println(scor);
  setRGB(100, 100, 100); // LED alb pentru stare de repaus
}

// Verifica daca timpul jocului a expirat
void verificaStare() {
  if (rulare && millis() - momentIncepereJoc > timpJoc) {
    repaus = true;
    rulare = false;
    finalizare();
  }
}

// Functie pentru numaratoarea inversa de inceput
void incepere() {
  if (millis() - momentUltimaClipire > timpIncepere / 6) {
    momentUltimaClipire = millis();
    setRGB(100 * stareLed, 100 * stareLed, 100 * stareLed);
    stareLed = !stareLed;
    if (!(clipiri % 2)) {
      Serial.println(3 - clipiri / 2);
    }
    clipiri++;
  }

  if (clipiri == 6) {
    // Alege un cuvant tinta aleator
    strcpy(cuvantTinta, dictionar[random(10)]);
    Serial.println(cuvantTinta);
    momentSelectareCuvant = millis();
    setRGB(0, 100, 0); // LED verde pentru rundă activa
    clipiri++;
  }
}

// Functie pentru a initia jocul
void startJoc() {
  momentIncepereJoc = millis();
  scor = 0;
  momentUltimaClipire = millis();
  clipiri = 0;
  strcpy(cuvant, ""); // Reseteaza cuvantul introdus
}

// Verifica butonul de dificultate pentru a schimba nivelul
void verificareButonDificultate() {
  if (repaus && millis() - momentApasareDificultate > timpDebounce && digitalRead(BUTON_DIFICULTATE) == LOW) {
    momentApasareDificultate = millis();
    dificultate = (dificultate + 1) % 3;
    Serial.println(numeDificultati[dificultate]);
  }
}

// Verifica butonul Start/Stop pentru a incepe sau opri jocul
void verificareButonStart() {
  if (millis() - momentApasareStart > timpDebounce && digitalRead(BUTON_START) == LOW) {
    momentApasareStart = millis();
    repaus = !repaus;
    rulare = !rulare;
    if (rulare) {
      startJoc();
    } else {
      finalizare();
    }
  }
}

// Functie pentru a citi o litera introdusa de jucator
void citireLitera() {
  char litera = Serial.read();
  if (litera == BACKSPACE && index > 0) {
    cuvant[--index] = '\0';
  } else {
    cuvant[index++] = litera;
    cuvant[index] = '\0';
  }
}

// Functie pentru a selecta un cuvant nou dupa verificarea corectitudinii
void alegereCuvantNou(int verdict) {
  if (verdict) {
    scor++;
    Serial.println("\n");
  } else {
    Serial.println("\nTimp expirat!\n");
  }
  strcpy(cuvant, "");
  index = 0;
  strcpy(cuvantTinta, dictionar[random(10)]);
  Serial.println(cuvantTinta);
  momentSelectareCuvant = millis();
}

void setup() {
  Serial.begin(BAUD);
  pinMode(LED_ROSU, OUTPUT);
  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_ALBASTRU, OUTPUT);
  pinMode(BUTON_START, INPUT_PULLUP);
  pinMode(BUTON_DIFICULTATE, INPUT_PULLUP);

  setRGB(100, 100, 100); // LED alb pentru stare de repaus

  attachInterrupt(digitalPinToInterrupt(BUTON_START), verificareButonStart, FALLING);
  attachInterrupt(digitalPinToInterrupt(BUTON_DIFICULTATE), verificareButonDificultate, FALLING);

  Serial.println("\nSetup complet\n");
}

void loop() {
  verificaStare(); // Verifica daca jocul trebuie oprit

  if (rulare) {
    if (millis() - momentIncepereJoc <= timpIncepere) {
      incepere(); // Numaratoarea inversa de inceput
    } else {
      if (Serial.available()) {
        citireLitera(); // Citeste si verifica litera introdusa

        int verdict = verificaCuvant(cuvant);
        if (verdict == 1) {
          alegereCuvantNou(1); // Selecteaza un nou cuvant daca e corect
        } else if (verdict == -1) {
          setRGB(100, 0, 0); // LED rosu pentru cuvant gresit
        } else {
          setRGB(0, 100, 0); // LED verde pentru cuvant corect
        }
      }
      if (millis() - momentSelectareCuvant > timpTastare[dificultate]) {
        alegereCuvantNou(0); // Timpul de tastare expirat, selecteaza un nou cuvant
      }
    }
  }
}
