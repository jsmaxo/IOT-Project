#define BLYNK_TEMPLATE_ID "TMPL6wIMtHVxK"
#define BLYNK_TEMPLATE_NAME "Water Quality Monitering"
#define BLYNK_AUTH_TOKEN "14w9sZ6T-fkPtHi-OMflubE9ACcKKpMJ"

// Comment this out to disable prints and save space
#define BLYNK_PRINT Serial
#include <BlynkSimpleEsp32.h>
#include "secret.h"
#include <WiFi.h>

#include <WiFiClient.h>
#include <LiquidCrystal_I2C.h>

// LCD configuration
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = WIFI_SSID ;
char pass[] = WIFI_PASS ;



char auth[] = BLYNK_AUTH_TOKEN;

namespace pin {
    const byte tds_sensor = 34;
}

namespace device {
    float aref = 3.3; // Vref, this is for 3.3v compatible controller boards, for Arduino use 5.0v.
}

namespace sensor {
    float ec = 0;
    unsigned int tds = 0;
    float ecCalibration = 1;
}

void setup() {
    Serial.begin(115200); // Debugging on hardware Serial 0
    Blynk.begin(auth, ssid, pass);
    // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(1, 0);
  lcd.print("Water Quality");
  lcd.setCursor(3, 1);
  lcd.print("Monitoring   ");
  lcd.setCursor(12, 0);  // Adjust the position for the heart symbol 
  
  delay(2000);
  lcd.clear();

}

void loop() {
    Blynk.run();
    readTdsQuick();
    delay(1000);
}

void readTdsQuick() {

    float temperature = 25.0; // assume room temp (or use DS18B20 later)

    // Read the raw analog value and convert to voltage
    float sum = 0;
    for (int i = 0; i < 10; i++) {
        sum += analogRead(pin::tds_sensor);
        delay(10);
    }
    float avg = sum / 10.0;
    float rawEc = avg * device::aref / 4096.0;
    
    // Debugging: Print the raw analog value
    Serial.print(F("Raw Analog Value: "));
    Serial.println(rawEc);

    // Adjust this offset based on the sensor's dry reading (without immersion)
    float offset = 0.14; // Set this to the observed raw analog value in air

    // Apply calibration and offset compensation
    float tempCoefficient = 1.0 + 0.02 * (temperature - 25.0);
    sensor::ec = ((rawEc / tempCoefficient) * sensor::ecCalibration) - offset;

    // If the EC is below zero after adjustment, set it to zero
    if (sensor::ec < 0) sensor::ec = 0;

    // Convert voltage value to TDS value using a cubic equation
    sensor::tds = (133.42 * pow(sensor::ec, 3) - 255.86 * sensor::ec * sensor::ec + 857.39 * sensor::ec) * 0.5;

    // Debugging: Print the TDS and EC values
    Serial.print(F("TDS: "));
    Serial.println(sensor::tds);
    Serial.print(F("EC: "));
    Serial.println(sensor::ec, 2);
    lcd.setCursor(0, 0);
    lcd.print("TDS: ");

    lcd.setCursor(4,0);
    lcd.print(sensor::tds);
    lcd.print(" ppm");

    lcd.setCursor(0,1);
    lcd.print("EC: ");

    lcd.setCursor(4,1);
    lcd.print(sensor::ec);
    lcd.print(" mS/cm");

    // Send data to Blynk virtual pins
    Blynk.virtualWrite(V2, sensor::tds);
    Blynk.virtualWrite(V3, sensor::ec);
}