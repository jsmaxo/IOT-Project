#define BLYNK_TEMPLATE_ID "TMPL6mhEr_Ynq"
#define BLYNK_TEMPLATE_NAME "Water Level  Monitering"
#define BLYNK_AUTH_TOKEN "Cuu_y00_IVSOTrPvtAUkPTzhgmrhc242"

#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <ESP_Mail_Client.h>
#include "secret.h"
#define SMTP_server "smtp.gmail.com"
#define SMTP_Port 587
#define sender_email EMAIL_SENDER    // sender email id
#define sender_password EMAIL_PASSWORD
#define Recipient_email "nanayakkaraj210@gmail.com"    // receiver email id
#define Recipient_name "ESP32"

// ---------- WiFi ----------
char ssid[] = WIFI_SSID;
char pass[] = WIFI_PASS;

// ---------- Tank distances (cm) ----------
float emptyTankDistance = 12.4;   // distance when tank is empty
float fullTankDistance  = 2;   // distance when tank is full

bool lowAlertSent = false;
bool fullAlertSent = false;
// ---------- Pins (avoid strapping pins) ----------
#define RLED 4
#define BLED 5
#define TRIGPIN 12
#define ECHOPIN 13
#define BUZZER 18
#define relay 23



// ---------- Blynk virtual pins ----------
#define VPIN_LEVEL   V0
#define VPIN_DIST    V1

LiquidCrystal_I2C lcd(0x27, 16, 2);
BlynkTimer timer;
SMTPSession smtp;

float lastDistance = NAN;
int   lastLevel    = -1;

void alert(int ledPin) {
  for (int i = 0; i < 10; i++) {
    digitalWrite(ledPin, HIGH);
    digitalWrite(BUZZER, HIGH);
    delay(500);
    digitalWrite(ledPin, LOW);
    digitalWrite(BUZZER, LOW);
    delay(500);
  }
}

void displayDataLCD(int level, float dist) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Water(%): ");
  lcd.print(level);
  lcd.print("%");

  lcd.setCursor(0, 1);
  lcd.print("Water_H: ");
  lcd.print(dist, 1); // one decimal place
  lcd.print("cm");

}

void measureDistance() {
  // trigger pulse
  digitalWrite(TRIGPIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGPIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGPIN, LOW);

  // echo pulse with timeout (30000 us ≈ 5 m range)
  unsigned long duration = pulseIn(ECHOPIN, HIGH, 30000UL);
  if (duration == 0) {
    Serial.println("Echo timeout");
    return;
  }

  // convert to distance (cm). speed of sound ~0.0343 cm/us -> dist = (duration/2)*0.0343
  float distance = (duration * 0.0343f) / 2.0f;

  // clamp and compute level
  distance = constrain(distance, 0.0f, 400.0f);
  if (distance > (fullTankDistance - 10) && distance < emptyTankDistance) {
    int level = map((int)distance, emptyTankDistance, fullTankDistance, 0, 100);
    level = constrain(level, 0, 100);

    // only update if changed (reduces LCD/Blynk spam)
    if (level != lastLevel || fabs(distance - lastDistance) > 0.2f) {
      float waterlevelheight = emptyTankDistance - distance;
      displayDataLCD(level, waterlevelheight);
      Blynk.virtualWrite(VPIN_LEVEL, level);
      Blynk.virtualWrite(VPIN_DIST, String(waterlevelheight, 1) + " cm");

      Serial.print("waterlevelheight: ");
      Serial.print(waterlevelheight, 1);
      Serial.print(" cm | Level: ");
      Serial.print(level);
      Serial.println("%");

        smtp.debug(1);
        ESP_Mail_Session session;
        session.server.host_name = SMTP_server;
        session.server.port = SMTP_Port;
        session.login.email = sender_email;
        session.login.password = sender_password;
        session.login.user_domain = "";


      if(waterlevelheight <= 1.8 && !lowAlertSent){

              alert(RLED);

              digitalWrite(relay, LOW); // LOW = Motor ON (active LOW relay)
              Serial.println("Motor is ON ");
              digitalWrite(BUZZER, LOW);

              lowAlertSent = true;
              fullAlertSent = false;

                    // Declare the message class
            SMTP_Message message;
            message.sender.name = "Water_Level_Monitering";
            message.sender.email = sender_email;
            message.subject = "Water Level is LOW_Refill Needed";
            message.addRecipient(Recipient_name, Recipient_email);

            // Send simple text message
            String textMsg = "water level in your tank is critically low.  \nCurrent Water Height:  (" + String(waterlevelheight, 1) + " cm)";
            message.text.content = textMsg.c_str();
            message.text.charSet = "us-ascii";
            message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;

            if (!smtp.connect(&session))
              return;

            if (!MailClient.sendMail(&smtp, &message))
              Serial.println("Error sending Email, " + smtp.errorReason());
      }
      else if((emptyTankDistance - fullTankDistance) <= waterlevelheight  && !fullAlertSent){
        
            alert(BLED);
             
            digitalWrite(relay, HIGH); // HIGH = Motor OFF (active HIGH relay)
            Serial.println("Motor is OFF ");

            fullAlertSent = true;
            lowAlertSent = false;

          // Declare the message class
            SMTP_Message message;
            message.sender.name = "Water_Level_Monitering";
            message.sender.email = sender_email;
            message.subject = "Water Tank is FULL_Action Required";
            message.addRecipient(Recipient_name, Recipient_email);

            // Send simple text message
            String textMsg = "Water level in your tank has reached its full capacity. \nCurrent Water Height:  (" + String(waterlevelheight, 1) + " cm)";
            message.text.content = textMsg.c_str();
            message.text.charSet = "us-ascii";
            message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;

            if (!smtp.connect(&session))
              return;

            if (!MailClient.sendMail(&smtp, &message))
              Serial.println("Error sending Email, " + smtp.errorReason());
      }
      else{
        digitalWrite(BLED,LOW);
        digitalWrite(RLED,LOW);
      }

      lastLevel = level;
      lastDistance = distance;
    }
  } else {
    Serial.println("Reading out of calibrated range");
  }
}

void checkBlynkStatus() {
  if (Blynk.connected()) {
    Serial.println("✅ Blynk Connected");
  } else {
    Serial.println("❌ Blynk Not Connected");
    Blynk.connect(5000); // try to reconnect for up to 5s
  }
}

BLYNK_CONNECTED() {
  Blynk.syncVirtual(VPIN_LEVEL);
  Blynk.syncVirtual(VPIN_DIST);
}

void setup() {
  Serial.begin(115200);
  delay(100);

  // pins
  pinMode(RLED, OUTPUT);
  pinMode(BLED, OUTPUT);
  pinMode(TRIGPIN, OUTPUT);
  pinMode(ECHOPIN, INPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(relay, OUTPUT);
  digitalWrite(relay, HIGH);  

  // LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Water Level");
  lcd.setCursor(0, 1);
  lcd.print("Monitering Sys");
  delay(2200);
  lcd.clear();

  // Simplest, robust way: lets Blynk handle Wi-Fi + cloud connect
  Serial.println("Connecting to Wi-Fi & Blynk...");
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass, "blynk.cloud", 80);  // blocks until connected
  Serial.println("Connected.");

  // timers
  timer.setInterval(1000L, measureDistance);   // 0.5s measurements
  timer.setInterval(2000L, checkBlynkStatus); // connection check
}

void loop() {
  Blynk.run();
  timer.run();
}


