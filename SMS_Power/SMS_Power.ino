// Power loss sms notification
// condac 2017

#include "config.h" // Copy config.h.template to config.h and replace number with your phone number


// Using TinyGSM library
// Select your modem:
//#define TINY_GSM_MODEM_SIM800
#define TINY_GSM_MODEM_SIM900
// #define TINY_GSM_MODEM_A6
// #define TINY_GSM_MODEM_A7
// #define TINY_GSM_MODEM_M590
// #define TINY_GSM_MODEM_ESP8266

#define SerialMon Serial

#include <SoftwareSerial.h>

SoftwareSerial SerialAT(7, 8); // RX, TX
#define POWER_IN A5
#define GSM_POWER_STATUS 12
#define GSM_POWER_BUTTON 9
#define LED 13
#define TINY_GSM_DEBUG SerialMon

#include <TinyGsmClient.h>
TinyGsm modem(SerialAT);

// Module baud rate
uint32_t rate = 19200;

unsigned long powerlosstime;
bool powerstatus;
bool powernotified = false;

void initModem() {
  if( !digitalRead(GSM_POWER_STATUS) ) {
    SIM900power();
    
  }
  modem.init();
  
  SerialMon.println("Wait for network");
  while (!modem.waitForNetwork()) {
    SerialMon.println("Wait for network loop");
    delay(1000);    
  }
  SerialMon.println("Send AT");
  SerialAT.write("AT\r");
  delay(1000);
  if (SerialAT.available()) {
    SerialMon.write(SerialAT.read());
  }
  if (SerialAT.available()) {
    SerialMon.write(SerialAT.read());
  }
  if (SerialAT.available()) {
    SerialMon.write(SerialAT.read());
  }
  SerialMon.println("Send GSM mode");
  SerialAT.write("AT+CSCS=\"GSM\"\r"); // must set this mode manual or it fail to use the send sms
  delay(500);
  if (SerialAT.available()) {
    SerialMon.write(SerialAT.read());
  }
  if (SerialAT.available()) {
    SerialMon.write(SerialAT.read());
  }
  if (SerialAT.available()) {
    SerialMon.write(SerialAT.read());
  }
  SerialMon.println(modem.getModemInfo());
  delay(1000);
  int csq = modem.getSignalQuality();
  
  DBG("Signal quality:", csq);
  delay(1000);
}
void setup() {
  // Set Pinmodes
  pinMode(POWER_IN, INPUT);
  pinMode(GSM_POWER_STATUS, INPUT);
  pinMode(GSM_POWER_BUTTON, OUTPUT);
  pinMode(LED, OUTPUT);
  // Set console baud rate
  SerialMon.begin(19200);
  SerialAT.begin(rate);
  SerialMon.println(F("*Starting*"));
  // do all init stuff for modem
  initModem();
  // Access AT commands from Serial Monitor
  SerialMon.println(F("*Started*"));
  powerstatus = digitalRead(POWER_IN);
  if (powerstatus) {
    bool res = modem.sendSMS(PHONE_NR, String("Arduino boot. Power ON") );
    DBG("SMS:", res ? "OK" : "fail");
  } else {
    bool res = modem.sendSMS(PHONE_NR, String("Arduino boot. Power OFF") );
    DBG("SMS:", res ? "OK" : "fail");
  }
  

}

void loop() {
  digitalWrite(LED, powerstatus);
  if (SerialAT.available()) {
    SerialMon.write(SerialAT.read());
  }
  if (SerialMon.available()) {
    byte b = SerialMon.read();
    if ( b == '*') {
      SerialAT.write(0x1a);
    } 
    else if( b=='/') {
      bool res;
      res = modem.sendSMS(PHONE_NR, String("Hello from house.") );
      DBG("SMS:", res ? "OK" : "fail");

    }else {
      SerialAT.write(b);
    }
  }
  // put your main code here, to run repeatedly:
  if(digitalRead(GSM_POWER_STATUS) ) {
    //Serial.println("GSM Power True");
  }else {
    delay(1000);
    if(!digitalRead(GSM_POWER_STATUS) ) {
      initModem();
      bool res = modem.sendSMS(PHONE_NR, String("GSM modem rebooted") );
      DBG("SMS:", res ? "OK" : "fail");
    }
  }
  if(digitalRead(POWER_IN) ) {
    if (!powerstatus) {
      // Power is restored
      powerstatus = true;
      unsigned long blackouttime = millis() - powerlosstime;
      blackouttime = blackouttime /1000;
      if (powernotified) {
        // Long power failure, but arduino is still alive
        if(!digitalRead(GSM_POWER_STATUS) ) {
          initModem();
          String stringOne = "GSM modem rebooted and power restored. ";
          String stringThree = stringOne + blackouttime;
          bool res = modem.sendSMS(PHONE_NR, String( stringThree ) );
          DBG("SMS:", res ? "OK" : "fail");
          Serial.println("Sent long powerrestore sms gsm reboot");
        } else {
          String stringOne = "Power restored. ";
          String stringThree = stringOne + blackouttime;
          bool res = modem.sendSMS(PHONE_NR, String( stringThree ) );
          DBG("SMS:", res ? "OK" : "fail");
          Serial.println("Sent long powerrestore sms ");
        }
        
      } else {
        // Short power failure. 
        if(!digitalRead(GSM_POWER_STATUS) ) {
          initModem();
          String stringOne = "GSM modem rebooted and short power failure. ";
          String stringThree = stringOne + blackouttime;
          bool res = modem.sendSMS(PHONE_NR, String( stringThree ) );
          DBG("SMS:", res ? "OK" : "fail");
          Serial.println("Sent short powerrestore sms gsm reboot");
        } else {
          String stringOne = "Short power failure restored. ";
          String stringThree = stringOne + blackouttime;
          bool res = modem.sendSMS(PHONE_NR, String( stringThree ) );
          DBG("SMS:", res ? "OK" : "fail");
          Serial.println("Sent short powerrestore sms ");
        }
      }
    }
    //Serial.println("Power True");
  }else {
    delay(100);
    if(!digitalRead(POWER_IN) ) {
      
      if (powerstatus) {
        // Power is down and was up prevuisly 
        powerlosstime = millis();
        powerstatus = false;
        powernotified = false;
        Serial.println("Power False");
      }
      
    }
    
  }
  if (!powerstatus) {
    unsigned long compare = millis() - powerlosstime;
    if (compare > 5000) { // wait 5s before sending power fail sms incase it is just a short power failure. 
      if (!powernotified) {
        powernotified = true;
        bool res = modem.sendSMS(PHONE_NR, String("Power failed.") );
        DBG("SMS:", res ? "OK" : "fail");
        Serial.println("Sent powerloss sms");
      }
      
    }
  }
}

void SIM900power() {
  // press powerbutton
  digitalWrite(GSM_POWER_BUTTON, HIGH);
  delay(1500);
  digitalWrite(GSM_POWER_BUTTON, LOW);
  delay(1000);
}
