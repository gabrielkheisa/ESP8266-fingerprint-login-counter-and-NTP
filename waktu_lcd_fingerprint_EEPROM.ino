#include <Wire.h>

#include <LiquidCrystal_I2C.h>

#include <Adafruit_Fingerprint.h>

#include <NTPClient.h>

#include <WiFiUdp.h>

#include <ESP8266WiFi.h>

#include <SoftwareSerial.h>

#include <EEPROM.h>

#define doorLock 16
int jam_mtbs = 1000 * 3600 * 1; // update time and put login amount to EEPROM for every 1 hour
long jam_lasttime;

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7);

SoftwareSerial mySerial(12, 14);

const long utcOffsetInSeconds = 7 * 60 * 60 + 1;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "time.google.com", utcOffsetInSeconds);
/* Put your SSID & Password */
const char * ssid = "<Name>"; // Enter SSID here
const char * password = "<Password>"; //Enter Password here
IPAddress local_ip(192, 168, 1, 89);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

Adafruit_Fingerprint finger = Adafruit_Fingerprint( & mySerial);

int jumlahlogin;
const int eeAddress = 175; // Where counter data stored in EEPROM

void setup() {
  Serial.begin(9600);
  //EEPROM initialization
  EEPROM.begin(512);
  EEPROM.get(eeAddress, jumlahlogin); // Retrieve amount of scan from EEPROM in case of blackout

  //EEPROM.put(175, 0); //pakek address 175 sadja
  //EEPROM.commit();

  //inisialisasi Wi-Fi --------------------------------------------
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  //WiFi.config(local_ip, gateway, subnet); //--> To override DHCP
  //-------------------------------------------------------
  //WE define our LCD 16 columns and 2 rows
  lcd.begin(16, 2); // for 16 x 2 LCD module
  lcd.setBacklightPin(3, POSITIVE);
  lcd.setBacklight(HIGH);
  lcd.clear();

  lcd.setCursor(0, 0); //set koordinat LCD
  lcd.print("Booting up");
  delay(100);
  lcd.setCursor(0, 1);
  lcd.print("DoorLock");
  delay(100);

  delay(10000);
  Serial.println(timeClient.update());

  lcd.clear();

  finger.begin(57600);
  pinMode(doorLock, OUTPUT);
  finger.getTemplateCount();
  if (finger.verifyPassword()) {
    Serial.println("Found fingerprint sensor!");
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    while (1) {
      delay(1);
    }

    //pinMode(doorLock, OUTPUT);
    digitalWrite(doorLock, HIGH);
    lcd.setCursor(0, 0); //set koordinat LCD
    lcd.print("Gabriel Kheisa");
    delay(100);
    lcd.setCursor(0, 1);
    lcd.print("Fingerprint");
    delay(100);
  }
}

void loop() {
  getFingerprintID();

  //update jam pakek millis() biar ga ngelag wifinya
  if (millis() > jam_lasttime + jam_mtbs) {
    timeClient.update();
    lcd.clear();
    jam_lasttime = millis();
    EEPROM.begin(512);
    EEPROM.put(eeAddress, jumlahlogin);
    EEPROM.commit();
  }
}

uint8_t getFingerprintID() {
  uint8_t p = finger.getImage();
  switch (p) {
  case FINGERPRINT_OK:
    Serial.println("Image taken");
    break;
  case FINGERPRINT_NOFINGER:
    Serial.println("No finger detected");

    digitalWrite(doorLock, HIGH);

    Serial.println(timeClient.getFormattedTime());

    //biar ga ngelag, update nya tiap 5 menit sekali
    /*
      if (
        (int)timeClient.getSeconds() == 30 && (int)timeClient.getMinutes() % 5 == 0)
        timeClient.update();
    */

    //Write your text:
    Serial.println((String) timeClient.getFormattedTime());

    lcd.setCursor(0, 1);
    lcd.print((String) timeClient.getFormattedTime());
    lcd.setCursor(8, 1);
    lcd.print("     ");
    lcd.setCursor(13, 1);
    lcd.print(jumlahlogin);

    if ((int) timeClient.getSeconds() % 5 == 0) {
      lcd.setCursor(0, 0);
      lcd.print("@arduino_craft   ");
    } else {
      lcd.setCursor(0, 0); //we start writing from the first row first column
      lcd.print("Gabriel Kheisa   "); //16 characters poer line
    }

    return p;
  case FINGERPRINT_PACKETRECIEVEERR:
    Serial.println("Communication error");
    return p;
  case FINGERPRINT_IMAGEFAIL:
    Serial.println("Imaging error");
    return p;
  default:
    Serial.println("Unknown error");
    return p;
  }

  // OK success!

  p = finger.image2Tz();
  switch (p) {
  case FINGERPRINT_OK:
    Serial.println("Image converted");
    break;
  case FINGERPRINT_IMAGEMESS:
    Serial.println("Image too messy");
    return p;
  case FINGERPRINT_PACKETRECIEVEERR:
    Serial.println("Communication error");
    return p;
  case FINGERPRINT_FEATUREFAIL:
    Serial.println("Could not find fingerprint features");
    return p;
  case FINGERPRINT_INVALIDIMAGE:
    Serial.println("Could not find fingerprint features");
    return p;
  default:
    Serial.println("Unknown error");
    return p;
  }

  // OK converted!---------------------------------------------
  p = finger.fingerFastSearch();
  if (p == FINGERPRINT_OK) { //jika fingerprint terdeteksi
    Serial.println("Found a print match!");

    digitalWrite(doorLock, LOW);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Selamat Datang");
    delay(100);
    lcd.setCursor(0, 1);
    lcd.print("Gabriel Kheisa ");
    for (int i = 5; i > 0; i--) {
      delay(1000);
    }
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Terima Kasih");
    delay(100);
    lcd.setCursor(0, 1);
    lcd.print("Gabriel Kheisa");
    delay(100);
    digitalWrite(doorLock, HIGH);
    //jumlah login dan EEPROM
    jumlahlogin++;
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_NOTFOUND) { //jika fingerprint salah
    Serial.println("Did not find a match");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Jari Salah");
    delay(100);
    lcd.setCursor(0, 1);
    lcd.print("Coba Lagi");
    delay(2000);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Gabriel Kheisa ");
    delay(100);
    lcd.setCursor(0, 1);
    lcd.print("Fingerprint    ");
    delay(100);
    return p;
  } else {
    Serial.println("Unknown error");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("??");
    delay(100);
    return p;
  }

  // found a match!
  Serial.print("Found ID #");
  Serial.print(finger.fingerID);
  Serial.print(" with confidence of ");
  Serial.println(finger.confidence);

  return finger.fingerID;
}
