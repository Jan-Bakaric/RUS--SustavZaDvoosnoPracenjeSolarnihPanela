#include <Servo.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>

// Definicija pinova
const int LDR1Pin = A0;  // Pin na koji je spojen prvi LDR
const int LDR2Pin = A1;  // Pin na koji je spojen drugi LDR
const int LDR3Pin = A2;  // Pin na koji je spojen treći LDR
const int LDR4Pin = A3;  // Pin na koji je spojen četvrti LDR

const int servo1Pin = 9; // PWM pin za prvi servo
const int servo2Pin = 10; // PWM pin za drugi servo

const int buttonPin = 2;  // Pin na koji je spojeno tipkalo (prekid)
const int ledPin = 13;    // Pin za LED-icu

Servo servo1;
Servo servo2;

// Varijable za očitanje LDR-a
int ldr1Value = 0;
int ldr2Value = 0;
int ldr3Value = 0;
int ldr4Value = 0;

// Varijable za stanje sustava i debounce logiku
volatile bool systemOn = false;
volatile bool toggleRequested = false;
volatile unsigned long lastInterruptTime = 0;
const unsigned long debounceDelay = 200; 

void setup() {
  // Pokretanje serijske komunikacije
  Serial.begin(9600);

  // Inicijalizacija servo-a
  servo1.attach(servo1Pin);
  servo2.attach(servo2Pin);
  
  // Postavljanje početnih pozicija servo-a
  servo1.write(90); // 90 je početni položaj (središnji)
  servo2.write(90); // 90 je početni položaj (središnji)

  pinMode(buttonPin, INPUT_PULLUP); 
  pinMode(ledPin, OUTPUT);

  // Postavljanje prekida
  attachInterrupt(digitalPinToInterrupt(buttonPin), toggleSystem, FALLING);
}

void loop() {
  // Provjerava je li traženo mijenjanje stanja sustava (ON/OFF)
  if (toggleRequested) {
    systemOn = !systemOn;
    toggleRequested = false;

    if (systemOn) {
      Serial.println("Sustav uključen");
    } else {
      Serial.println("Sustav isključen");
      enterSleepMode(); 
    }
  }

  // LED-ica pokazuje je li sustav uključen
  digitalWrite(ledPin, systemOn ? HIGH : LOW);

  if (systemOn) {
    // Očitavanje vrijednosti s LDR-a
    ldr1Value = analogRead(LDR1Pin);
    ldr2Value = analogRead(LDR2Pin);
    ldr3Value = analogRead(LDR3Pin);
    ldr4Value = analogRead(LDR4Pin);

    // Ispisivanje očitanih LDR vrijednosti u serijski monitor
    Serial.print("LDR1 Value: ");
    Serial.print(ldr1Value);
    Serial.print("\t");
    
    Serial.print("LDR2 Value: ");
    Serial.print(ldr2Value);
    Serial.print("\t");

    Serial.print("LDR3 Value: ");
    Serial.print(ldr3Value);
    Serial.print("\t");

    Serial.print("LDR4 Value: ");
    Serial.println(ldr4Value);
    
    // Logika za praćenje panela s LDR-ima (pomicanje servo-a)
    // Koriste se sve četiri LDR vrijednosti za donošenje odluka o pomicanju servo-a
    // Što je LDR otpor manji, to je svjetlost jača, pa rotiranje servo-a ovisi o tome

    // Ako je svjetlost na LDR1 veća od ostalih, pomakni servo1 lijevo, servo2 desno -> gore - desno u simulaciji
    if (ldr1Value < ldr2Value && ldr1Value < ldr3Value && ldr1Value < ldr4Value) {
      servo1.write(0);  // Okretanje servo-a 1 u lijevo
      servo2.write(180); // Okretanje servo-a 2 u desno
    }
    // Ako je svjetlost na LDR2 veći od ostalih, pomakni servo1 desno, servo2 lijevo -> dolje - lijevo u simulaciji
    else if (ldr2Value < ldr1Value && ldr2Value < ldr3Value && ldr2Value < ldr4Value) {
      servo1.write(180); // Okretanje servo-a 1 u desno
      servo2.write(0);  // Okretanje servo-a 2 u lijevo
    }
    // Ako je svjetlost na LDR3 veći od ostalih, pomakni oba servo-a u desno -> dolje - desno u simulaciji
    else if (ldr3Value < ldr1Value && ldr3Value < ldr2Value && ldr3Value < ldr4Value) {
      servo1.write(180); // Okretanje servo-a 1 u desno
      servo2.write(180); // Okretanje servo-a 2 u desno
    }
    // Ako je svjetlost na LDR4 veći od ostalih, pomakni oba servo-a u lijevo -> gore - lijevo u simulaciji
    else if (ldr4Value < ldr1Value && ldr4Value < ldr2Value && ldr4Value < ldr3Value) {
      servo1.write(0);  // Okretanje servo-a 1 u lijevo
      servo2.write(0);  // Okretanje servo-a 2 u lijevo
    }
    // Ako su svi LDR-ovi jednaki, postavi servo-e u srednji položaj
    else {
      servo1.write(90); // Središnji položaj servo-a 1
      servo2.write(90); // Središnji položaj servo-a 2
    }
  } else {
    // Sustav je isključen – servo-i idu u sredinu radi sigurnosti
    servo1.write(90);
    servo2.write(90);
  }

  // Pauza od 1 sekunde
  delay(1000);
}

// Funkcija koja se poziva kod prekida
void toggleSystem() {
  unsigned long currentTime = millis();
  if (currentTime - lastInterruptTime > debounceDelay) {
    toggleRequested = true;
    lastInterruptTime = currentTime;
  }
}

// Funkcija za ulazak u sleep mode
void enterSleepMode() {
  ADCSRA &= ~(1 << ADEN); // Isključi ADC (Analog to Digital Converter) - zbog funkcije analogRead()

  set_sleep_mode(SLEEP_MODE_PWR_DOWN); 
  sleep_enable();                      
  sleep_mode();                        

  sleep_disable();                     
  ADCSRA |= (1 << ADEN);               // Uključi ADC
}
