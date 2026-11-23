// ==========================================
//      ESP32 (BLYNK IOT BRIDGE)
//      File: ESP32_Blynk_Bridge.ino
// ==========================================

#define BLYNK_TEMPLATE_ID "YOUR_BLYNK_TEMPLATE_ID"
#define BLYNK_TEMPLATE_NAME "Prepaid Electricity"
#define BLYNK_AUTH_TOKEN "YOUR_BLYNK_AUTH_TOKEN"

#define BLYNK_PRINT Serial
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

char ssid[] = "YOUR_WIFI_SSID";
char pass[] = "YOUR_WIFI_PASSWORD";

// ESP32 uses HardwareSerial (Serial2)
#define RXP2 16
#define TXP2 17
HardwareSerial arduinoLink(2); 

WidgetLED statusLed(V4);
float pendingReloadAmount = 0;
bool notificationSent = false; 

void setup() {
  Serial.begin(9600);
  
  // Initialize Serial2 for ESP32 <-> Arduino communication
  arduinoLink.begin(9600, SERIAL_8N1, RXP2, TXP2);
  
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
}

void loop() {
  Blynk.run();

  // --- RECEIVE DATA ---
  if (arduinoLink.available()) {
    String data = arduinoLink.readStringUntil('\n');
    data.trim();

    // FORMAT: Voltage,Current,WhUsed,Balance,Pesos,Power
    int index1 = data.indexOf(',');
    int index2 = data.indexOf(',', index1 + 1);
    int index3 = data.indexOf(',', index2 + 1);
    int index4 = data.indexOf(',', index3 + 1);
    int index5 = data.indexOf(',', index4 + 1); 

    if (index5 > 0) { 
      String voltageStr = data.substring(0, index1);
      String currentStr = data.substring(index1 + 1, index2);
      String whUsedStr = data.substring(index2 + 1, index3);
      String balanceStr = data.substring(index3 + 1, index4);
      String pesosStr = data.substring(index4 + 1, index5); 
      String powerStr = data.substring(index5 + 1); 

      float voltage = voltageStr.toFloat();
      float current = currentStr.toFloat();
      float whUsed = whUsedStr.toFloat();
      float balance = balanceStr.toFloat();
      float remainingPesos = pesosStr.toFloat();
      float power = powerStr.toFloat(); 

      // --- WIDGET UPDATES ---
      Blynk.virtualWrite(V1, balance); 
      Blynk.virtualWrite(V2, balance); 
      Blynk.virtualWrite(V7, remainingPesos); 
      Blynk.virtualWrite(V8, voltage);        
      Blynk.virtualWrite(V9, current);        
      Blynk.virtualWrite(V10, whUsed);        
      Blynk.virtualWrite(V11, power);         

      // Status LED Logic
      if (balance > 0) { 
         statusLed.on();
         Blynk.setProperty(V4, "color", "#23C48E"); 
         Blynk.setProperty(V4, "label", "ACTIVE");
      } else {
         statusLed.on();
         Blynk.setProperty(V4, "color", "#D3435C"); 
         Blynk.setProperty(V4, "label", "OFF");
      }
      
      // NOTIFICATION LOGIC
      if (balance <= 0 && !notificationSent) {
          Blynk.logEvent("zero_balance", "⚠️ ALERT: Power has been cut! Your balance is 0 Watts.");
          notificationSent = true;
      }
      else if (balance > 0) {
          notificationSent = false;
      }
    }
  }
}

// --- COMMANDS ---

BLYNK_WRITE(V5) { // Switch
  int val = param.asInt();
  if (val == 1) {
      arduinoLink.println("SWITCH:1");
  } else {
      arduinoLink.println("SWITCH:0");
  }
}

BLYNK_WRITE(V3) { // Input: Updates the pending reload amount
  pendingReloadAmount = param.asFloat();
}

BLYNK_WRITE(V6) { // Submit Button: Sends the ADD command
  if (param.asInt() == 1 && pendingReloadAmount > 0) {
    arduinoLink.print("ADD:");
    arduinoLink.println(pendingReloadAmount);

    pendingReloadAmount = 0;
    Blynk.virtualWrite(V3, 0); // Reset the input field
  }
}
