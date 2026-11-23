// ==========================================
//      ARDUINO UNO (SMART PREPAID METER)
//      File: Arduino_Meter.ino
// ==========================================
#include <SoftwareSerial.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <Adafruit_INA219.h> 

// --- PINS ---
SoftwareSerial espSerial(2, 3); // RX=2, TX=3
LiquidCrystal_I2C lcd(0x27, 16, 2); 

const int houseLedPin1 = 9;   
const int houseLedPin2 = 10;
const int houseLedPin3 = 11;
const int houseLedPin4 = 12; 
const int houseLedPin5 = 13; 
const int redLedPin = 8;     
const int relayPin = 7;  

// --- SENSOR OBJECT ---
Adafruit_INA219 ina219; 

// --- TIMING, STATE, AND SCALING VARIABLES ---
double balance = 0.0;        // System starts with zero balance
float accumulatedWh = 0.0; 
long previousMillis = 0;   
const float DEDUCTION_SCALAR = 10000.0; 
bool isAppOn = true;         // Tracks the state of the Blynk Switch

// --- THRESHOLDS ---
const float WARNING_LIMIT = 1.0; 

void setup() {
  Serial.begin(9600);    
  espSerial.begin(9600); 
  
  pinMode(houseLedPin1, OUTPUT);
  pinMode(houseLedPin2, OUTPUT); 
  pinMode(houseLedPin3, OUTPUT); 
  pinMode(houseLedPin4, OUTPUT); 
  pinMode(houseLedPin5, OUTPUT); 
  pinMode(relayPin, OUTPUT);
  pinMode(redLedPin, OUTPUT);

  // --- INA219 Setup ---
  if (!ina219.begin()) {
    lcd.clear(); lcd.print("INA219 Error");
    while (1) { delay(10); } 
  }
  
  previousMillis = millis();

  // Initial state (Inverted Logic: HIGH = OFF, LOW = ON)
  digitalWrite(relayPin, LOW); 
  digitalWrite(houseLedPin1, HIGH); 
  digitalWrite(houseLedPin2, HIGH);
  digitalWrite(houseLedPin3, HIGH);
  digitalWrite(houseLedPin4, HIGH); 
  digitalWrite(houseLedPin5, HIGH); 
  digitalWrite(redLedPin, LOW); // Red LED ON (Warning State)

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("Smart Meter");
  lcd.setCursor(0,1);
  lcd.print("System Ready");
  delay(2000);
}

void loop() {
  // --- TIMING CALCULATION ---
  long currentMillis = millis();
  long deltaTime = currentMillis - previousMillis;
  previousMillis = currentMillis; 
  
  // 1. CHECK STATUS
  bool systemActive = isAppOn && (balance > 0);

  // --- READ SENSOR DATA ---
  float voltage = ina219.getBusVoltage_V();
  float current = ina219.getCurrent_mA() / 1000.0; 
  float power = ina219.getPower_mW() / 1000.0;     
  
  float remainingPesos = balance * 2.5; 
  
  if (systemActive) {
    digitalWrite(relayPin, HIGH); 
    
    // --- CALCULATE DEDUCTION ---
    float deduction = (power * deltaTime) / DEDUCTION_SCALAR;
    float currentWh = (power * deltaTime) / 3600000.0; 
    accumulatedWh += currentWh;
    balance = balance - deduction;
    
    // --- TURN ON LEDS (Inverted Logic: LOW = ON) ---
    digitalWrite(houseLedPin1, LOW); 
    digitalWrite(houseLedPin2, LOW); 
    digitalWrite(houseLedPin3, LOW); 
    digitalWrite(houseLedPin4, LOW);
    digitalWrite(houseLedPin5, LOW);
       
    // --- WARNING LOGIC ---
    if (balance <= WARNING_LIMIT) {
        lcd.setCursor(0, 1); lcd.print("Balance LOW!    ");
        for(int i = 0; i < 2; i++){
            digitalWrite(redLedPin, LOW); delay(200);
            digitalWrite(redLedPin, HIGH); delay(200);
        }
    } else {
       digitalWrite(redLedPin, HIGH); 
       lcd.setCursor(0, 1); lcd.print("Usage: Normal   ");
       delay(500); 
    }
    
    lcd.setCursor(0, 0); lcd.print("Rem: "); lcd.print(balance); lcd.print("U");

    // --- SEND DATA TO ESP32 ---
    espSerial.print(voltage); espSerial.print(",");
    espSerial.print(current); espSerial.print(",");
    espSerial.print(accumulatedWh); espSerial.print(",");
    espSerial.print(balance); espSerial.print(",");
    espSerial.print(remainingPesos); espSerial.print(",");
    espSerial.println(power); 

    // Pause to allow Arduino to listen for commands
    delay(500); 

  } else {
    // --- STOPPED STATE ---
    digitalWrite(relayPin, LOW); 
    digitalWrite(houseLedPin1, HIGH); 
    digitalWrite(houseLedPin2, HIGH); 
    digitalWrite(houseLedPin3, HIGH); 
    digitalWrite(houseLedPin4, HIGH); 
    digitalWrite(houseLedPin5, HIGH); 
    digitalWrite(redLedPin, LOW); 
    
    // Send 0s to ESP32
    espSerial.print(voltage); espSerial.print(","); 
    espSerial.print(0.0); espSerial.print(",");
    espSerial.print(accumulatedWh); espSerial.print(","); 
    espSerial.print(0.0); espSerial.print(",");
    espSerial.print(0.0); espSerial.print(",");
    espSerial.println(0.0);
    
    lcd.clear();
    if (balance <= 0) {
      lcd.setCursor(0, 0); lcd.print("EMPTY UNITS");
      lcd.setCursor(0, 1); lcd.print("PLEASE RELOAD");
    } else {
      lcd.setCursor(0, 0); lcd.print("SYSTEM PAUSED");
    }
    
    // Pause to allow Arduino to listen for commands
    delay(1000); 
  }

  // --- 3. LISTEN FOR COMMANDS ---
  while (espSerial.available() > 0) {
    delay(10); 
    String command = espSerial.readStringUntil('\n');
    command.trim(); 
    
    if (command.startsWith("ADD:")) {
       float moneyInput = command.substring(4).toFloat();
       if (moneyInput > 0) {
         balance += (moneyInput * 0.4); 
         lcd.clear(); lcd.print("Loading..."); delay(1000);
       }
    }
    else if (command.startsWith("SWITCH:")) {
       if (command.indexOf("1") > 0) {
           isAppOn = true;
       } else {
           isAppOn = false;
       }
    }
  }
}
