/*
   Catatan:
   Pompa dan flow 1 untuk cairan basa
   Pompa dan flow 2 untuk cairan asam
   Pompa dan flow 3 & 4 untuk cairan nutrisi A dan B, bebas gunakan yang mana saja
*/
#define BLYNK_PRINT Serial
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_ADS1015.h>
Adafruit_ADS1115 ads(0x48);
LiquidCrystal_I2C lcd(0x27, 16, 2);

//inisialisasi aplikasi blynk
char auth[] = "qc2iK1ZHWwBWfRXbPd2Fi-PkPq9wnrXO"; //ganti dengan Auth Token dari aplikasi blynk yang masuk ke email
char ssid[] = "CucuMbah"; //ganti dengan nama wifi anda
char pass[] = "Embuh1133335"; //ganti dengan password wifi anda

BlynkTimer timer;

//konfigurasi baca pH dan PPM
int16_t ADC_Ph, ADC_PPM;

/* konfigurasi pin relay kontrol pompa */
const int Pompa1 = 3;  // D0
const int Pompa2 = 1;  // D1
const int Pompa3 = 16; // D2
const int Pompa4 = 14; // D3

/* konfigurasi pin flow meter */
//D6 D7 D8 D9
const int FlowSensor1 = 12; int FlowSensor1State = 0; float CountFlow1 = 0; //flow sensor1
const int FlowSensor2 = 13; int FlowSensor2State = 0; float CountFlow2 = 0; //flow sensor2
const int FlowSensor3 = 15; int FlowSensor3State = 0; float CountFlow3 = 0; //flow sensor3
const int FlowSensor4 = 2;  int FlowSensor4State = 0; float CountFlow4 = 0; //flow sensor4

boolean tes1, tes2, tes3, tes4, onFlow, sistemOn;
uint16_t mLiter1, mLiter2, mLiter3, mLiter4;
bool onFlowPH = false; bool onFlowPPM = false; bool onNutrisiA = false; bool onNutrisiB = false;

//interval waktu sistem bekerja
long lastRead = 0;
const long interval = 60000; //28800000 = 8 jam


void sendSensor() {
  //kirim nilai pH dan PPM ke blynk
  ADC_Ph = ads.readADC_SingleEnded(0);
  ADC_PPM = ads.readADC_SingleEnded(1);

  float Ph = ADC_Ph * 0.1875 * 3.5 / 1000;
  int PPM = ADC_PPM * 0.1875 / 2;

  Blynk.virtualWrite(V2, Ph);
  Blynk.virtualWrite(V3, PPM);

  if (Ph < 6.00 && Ph > 7.00) {
    Blynk.notify("Motor PH menyala");
  } else {
    Blynk.notify("PH Normal");
  }

  if (PPM < 540) {
    Blynk.notify("Motor Nutrisi Menyala");
  } else  if (PPM >= 840) {
    Blynk.notify(String("Nutrisi Terlalu Pekat: ") + PPM + " PPM");
  }

  //misal agar angkanya sama antara blynk dan alat, baris ini di un-komen
  sistemOn = true;

}

void setup() {
  //inisialisasi I/O dan sensor
  pinMode(Pompa1, OUTPUT); digitalWrite(Pompa1, HIGH);  pinMode(Pompa2, OUTPUT); digitalWrite(Pompa2, HIGH);
  pinMode(Pompa3, OUTPUT); digitalWrite(Pompa3, HIGH);  pinMode(Pompa4, OUTPUT); digitalWrite(Pompa4, HIGH);
  pinMode(FlowSensor1, INPUT); pinMode(FlowSensor3, INPUT); pinMode(FlowSensor3, INPUT); pinMode(FlowSensor4, INPUT);

  ads.begin();
  // ads.setGain(GAIN_TWOTHIRDS);  // 2/3x gain +/- 6.144V  1 bit = 3mV      0.1875mV (default)

  lcd.init();
  lcd.init();
  lcd.backlight();
  lcd.setCursor(5, 0); lcd.print("Welcome");
  lcd.setCursor(0, 1); lcd.print("Smart Hidroponik");

  Blynk.begin(auth, ssid, pass, "blynk-cloud.com", 8080);
  timer.setInterval(interval, sendSensor); //interval pengiriman data 1 menit
}

void loop() {
  Blynk.run();
  timer.run();

  float Ph = ADC_Ph * 0.1875 * 3.5 / 1000;
  int PPM = ADC_PPM * 0.1875 / 2;

  //misal agar angkanya sama antara blynk dan alat, 3 baris ini di komen
  //if (millis() - lastRead >= interval) {
    //lastRead = millis();
    ///sistemOn = true;
  //}

  if (sistemOn == true) {
    //timePrevious = millis();
    ADC_Ph = ads.readADC_SingleEnded(0);
    ADC_PPM = ads.readADC_SingleEnded(1);

    lcd.clear();
    lcd.setCursor(0, 0); lcd.print("Nilai PH :"); lcd.print(Ph);
    lcd.setCursor(0, 1); lcd.print("Kadar PPM:"); lcd.print(PPM);
    onFlowPH = true;
    onFlow = true;
    sistemOn = false;
  }

  if (onFlow == true) {

    if (onFlowPH == true) {
      if (Ph < 6.0) {
        bacaFlowPhA();
      } else if (Ph > 7.0) {
        bacaFlowPhB();
      } else {
        digitalWrite(Pompa1, HIGH);
        digitalWrite(Pompa2, HIGH);
        onFlowPH = false;
      }
    }

    if (onFlowPH == false) {
      onFlowPPM = true;
    }

    if (onFlowPPM == true) {
      if (PPM < 560) {
        bacaFlowPPM();
      } else {
        digitalWrite(Pompa3, HIGH);
        digitalWrite(Pompa4, HIGH);
        onFlowPPM = false;
      }
    }
  } else {
    digitalWrite(Pompa1, HIGH);
    digitalWrite(Pompa2, HIGH);
    digitalWrite(Pompa3, HIGH);
    digitalWrite(Pompa4, HIGH);
  }
  //end of loop
}

// keluarkan cairan basa
void bacaFlowPhA() {
  FlowSensor1State = digitalRead(FlowSensor1);

  if (FlowSensor1State == HIGH) {
    tes1 = true;
  }

  if (tes1) {
    if (FlowSensor1State == LOW) {
      CountFlow1 += 1;
      tes1 = false;
    }
  }

  mLiter1 = CountFlow1 * 450 / 1000;

  if (mLiter1 >= 60) { //mL flow PH cairan basa
    digitalWrite(Pompa1, HIGH);
    CountFlow1 = 0;
    onFlowPH = false;
  } else {
    digitalWrite(Pompa1, LOW); digitalWrite(Pompa2, HIGH); digitalWrite(Pompa3, HIGH); digitalWrite(Pompa4, HIGH);
  }

  //lcd.setCursor(10, 0); lcd.print("U"); lcd.print(mLiter1);
  //end void bacaFLowPhA
}

// keluarkan cairan asam
void bacaFlowPhB() {
  FlowSensor2State = digitalRead(FlowSensor2);

  if (FlowSensor2State == HIGH) {
    tes2 = true;
  }

  if (tes2) {
    if (FlowSensor2State == LOW) {
      CountFlow2 += 1;
      tes2 = false;
    }
  }

  mLiter2 = CountFlow2 * 450 / 1000;

  if (mLiter2 >= 60) { //mL flow PH cairan asam
    digitalWrite(Pompa2, HIGH);
    CountFlow2 = 0;
    onFlowPH = false;
  } else {
    digitalWrite(Pompa2, LOW); digitalWrite(Pompa1, HIGH); digitalWrite(Pompa3, HIGH); digitalWrite(Pompa4, HIGH);
  }

  //lcd.setCursor(10, 1); lcd.print("D"); lcd.print(mLiter2);
  //end void bacaFLowPhB
}

//keluarkan nutrisi
void bacaFlowPPM() {
  if (onNutrisiB == false) {
    onNutrisiA = true;
  }

  if (onNutrisiA == true) {
    FlowSensor3State = digitalRead(FlowSensor3);

    if (FlowSensor3State == HIGH) {
      tes3 = true;
    }

    if (tes3) {
      if (FlowSensor3State == LOW) {
        CountFlow3 += 1;
        tes3 = false;
      }
    }

    mLiter3 = CountFlow3 * 450 / 1000;

    if (mLiter3 >= 100) { //mL flow nutrisi A
      digitalWrite(Pompa3, HIGH);
      CountFlow3 = 0;
      mLiter3 = 0;
      delay(100);
      onNutrisiA = false;
      onNutrisiB = true;
    } else {
      digitalWrite(Pompa3, LOW); digitalWrite(Pompa1, HIGH); digitalWrite(Pompa2, HIGH); digitalWrite(Pompa4, HIGH);
    }
    ///lcd.setCursor(14, 0); lcd.print("A"); lcd.print(mLiter3);
  }

  if (onNutrisiB == true) {
    FlowSensor4State = digitalRead(FlowSensor4);

    if (FlowSensor4State == HIGH) {
      tes4 = true;
    }

    if (tes4) {
      if (FlowSensor4State == LOW) {
        CountFlow4 += 1;
        tes4 = false;
      }
    }

    mLiter4 = CountFlow4 * 450 / 1000;

    if (mLiter4 >= 100) { //mL flow nutrisi B
      digitalWrite(Pompa4, HIGH);
      CountFlow4 = 0;
      mLiter4 = 0;
      onNutrisiB = false;
      onFlowPPM = false;
      onFlow = false;
      delay(1000);
    } else {
      digitalWrite(Pompa4, LOW); digitalWrite(Pompa1, HIGH); digitalWrite(Pompa2, HIGH); digitalWrite(Pompa3, HIGH);
    }
    //lcd.setCursor(14, 1); lcd.print("B"); lcd.print(mLiter4);
  }

  //end void baca FlowPPM
}
