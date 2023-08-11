/*

  # ========================================================================
  #                     GNU GENERAL PUBLIC LICENSE
  #                        Version 3, 29 June 2007
  # ========================================================================
  # Copyright (C) 2023 ASEP OMAN SOMANTRI
  #
  # This program is free software: you can redistribute it and/or modify
  # it under the terms of the GNU General Public License as published by
  # the Free Software Foundation, either version 3 of the License, or
  # (at your option) any later version.
  #
  # This program is distributed in the hope that it will be useful,
  # but WITHOUT ANY WARRANTY; without even the implied warranty of
  # MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  # GNU General Public License for more details.
  #
  # You should have received a copy of the GNU General Public License
  # along with this program. If not, see <https://www.gnu.org/licenses/>.
  # ========================================================================

  # DIAGRAM KETJA

  Mulai
  |
  Inisialisasi
  |
  Membaca Sensor
  |
  Pengaturan Pemanas
  |
  Membaca RPM dan Torsi
  |
  Kalkulasi Viskositas
  |
  Pemeriksaan Motor Aktif
  |-------[Ya]------> Aktifkan Motor
  |
  |-------[Tidak]---+
  |                  |
  V                  |
  Pemeriksaan Waktu Motor Aktif
  |-------[Ya]------> Matikan Motor
  |
  |-------[Tidak]---+
  |                  |
  V                  |
  Tampilan Menu
  |
  Interaksi Tombol
  |------[Timeout]--> Timeout Handling
  |
  Memulai/Menghentikan Motor
  |------[Start]----> Mulai Motor
  |                  |
  |-----[Stop]------> Hentikan Motor
  |
  Memulai/Menghentikan Heater
  |------[Heater ON]--> Hidupkan Pemanas
  |                  |
  |---[Heater OFF]---> Matikan Pemanas
  |
  Mengatur Suhu Maksimum dan Minimum
  |------[Atur Suhu Maksimum]--> Atur Suhu Maksimum
  |                  |
  |---[Atur Suhu Minimum]---> Atur Suhu Minimum
  |
  Mengatur Waktu Motor Aktif dan PWM Motor
  |------[Atur Waktu Motor Aktif]--> Atur Waktu Motor Aktif
  |                  |
  |-----[Atur PWM Motor]---> Atur PWM Motor
  |
  Akhir

*/

#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2); // Alamat I2C LCD dan ukuran karakter

#define tmblDEC 6 // Pin tombol untuk pengurangan
#define tmblINC 7  // Pin tombol untuk penambahan
#define tmblMENU 8  // Pin tombol untuk menu

#define relayHeater 4 // Pin Relay Heater
#define relayMotor 5 // Pin Relay Motor DC
#define kecepatsnMotor 9 // Pin kecepatan Motor DC

const int pinSensorSUHU = 10; // Pin sensor DS18B20
const int pinSensorArus = A0; // Pin sensor ACS612

const int sensorEncoder = 2; // Hubungkan dengan pin A sensor


// Deklarasi pin sensor
const int sensorPinA = 2; // Hubungkan dengan pin A sensor

volatile int counter = 0; // Menghitung jumlah pulsa
volatile unsigned long prevTime = 0; // Waktu sebelumnya untuk penghitungan RPM

OneWire oneWire(pinSensorSUHU); // Inisialisasi objek OneWire untuk sensor suhu
DallasTemperature sensor(&oneWire); // Inisialisasi objek DallasTemperature untuk sensor suhu
int resolusi = 11; // Resolusi sensor suhu

float suhu, volt, ampere, torsi, kcpSudut, viskositas;
int rpm;
unsigned long setWaktuMotor;
//unsigned long prevTime;

const byte MENU_UTAMA = 0; // Konstanta untuk menu utama
const byte MENU_ATUR_SUHU_MAKS = 1; // Konstanta untuk menu pengaturan suhu maksimum
const byte MENU_ATUR_SUHU_MIN = 2; // Konstanta untuk menu pengaturan suhu minimum
const byte MENU_HEATER_ON_OFF = 3; // Konstanta untuk menu memulai atau menghentikan heater
const byte MENU_ATUR_WAKTU_MOTOR_AKTIF = 4; // Konstanta untuk menu pengaturan waktu motor aktif
const byte MENU_ATUR_PWM_MOTOR = 5; // Konstanta untuk menu pengaturan PWM motor
const byte MENU_START_STOP = 6; // Konstanta untuk menu memulai atau menghentikan motor
const byte MENU_TIMEOUT = 7; // Konstanta untuk menu timeout
byte menuSaatIni = MENU_UTAMA; // Variabel untuk menyimpan menu saat ini

int tombolState = 0; // Variabel untuk menyimpan status tombol
int panjangTekanSet = 500; // Variabel untuk panjang tekan tombol yang ditetapkan
int panjangTekan = panjangTekanSet; // Variabel untuk panjang tekan tombol
int waktuTekanMulai = 0; // Waktu tombol mulai ditekan
int waktuTekanStop = 0; // Waktu tombol dilepas
boolean tombolFlag = false; // Variabel flag untuk mengontrol tombol ditekan atau tidak
unsigned long masukMenu; // Waktu saat masuk ke menu
unsigned long waktuKeluarMenu = 60000; // Waktu untuk keluar dari menu (timeout)

unsigned long memulaiMotor; // Waktu mulai motor aktif
boolean motorBerjalan = false; // Variabel untuk melacak status motor apakah sedang berjalan atau tidak

boolean heaterAktif = false; // Status pemanas aktif atau tidak

float suhuMaksimum = 25.0;
float suhuHisteresis = 1.0;
int setPwmMotor = 50;
int waktuMotor = 30;

// inisialisasi nilai-nilai parameter
//float TL = torsi; // Torsi pada kondisi ada beban (Nm)
float TNL = 0.0; // Torsi pada kondisi tanpa beban (Nm)
//float NL = rpm; // Kecepatan poros pada kondisi ada beban (mtr)
float h = 0.05; // Ketinggian lapisan cairan (mtr)
float Ri = 0.03; // Jari-jari silinder dalam (mtr)
float Ro = 0.04; // Jari-jari silinder luar (mtr)

//////////// Fungsi untuk melakukan setup awal ///////////
void setup() {
  lcd.begin(16, 2);
  lcd.backlight();

  // Set pin relay heater sebagai OUTPUT dan matikan relay
  pinMode(relayHeater, OUTPUT);
  digitalWrite(relayHeater, LOW);

  // Set pin relay motor sebagai OUTPUT dan matikan relay
  pinMode(relayMotor, OUTPUT);
  digitalWrite(relayMotor, LOW);

  // Set pin tombol DEC, INC, dan MENU sebagai INPUT
  pinMode(tmblDEC, INPUT);
  pinMode(tmblINC, INPUT);
  pinMode(tmblMENU, INPUT);

  // Atur status awal tombol DEC, INC, dan MENU sebagai HIGH (pull-up internal)
  digitalWrite(tmblDEC, HIGH);
  digitalWrite(tmblINC, HIGH);
  digitalWrite(tmblMENU, HIGH);

  // Konfigurasi pin sebagai input
  pinMode(sensorEncoder, INPUT);

  // Mulai komunikasi dengan sensor suhu
  sensor.begin();
  // Set resolusi sensor suhu
  sensor.setResolution(resolusi);
  // Delay untuk stabilisasi sensor
  delay(250 / (1 << (12 - resolusi)));

  // Attach interrupt untuk pin A
  attachInterrupt(digitalPinToInterrupt(sensorEncoder), countPulse, RISING);

}

/////////// Loop utama program ////////////
void loop() {
  // Mendapatkan waktu saat ini dalam milidetik
  unsigned long currentMillis = millis();

  // Mengambil nilai waktuSekarang sebelum perhitungan
  unsigned long  waktuSekarang = currentMillis;

  handleTimeout();

  int jumlahPulsa = counter;

  /////////////////////// Pengecekan ///////////////////////

  sensor.requestTemperatures();
  suhu = sensor.getTempCByIndex(0);
  delay(150 / (1 << (12 - resolusi)));

  float raw = analogRead(pinSensorArus); // Ubah nilai analog menjadi tegangan
  volt = ((raw / 1024.0) * 5.0) - 2.5;
  ampere = volt / 0.185; // Menghitung arus berdasarkan sensitivitas

  if (millis() - prevTime >= 1000) {
    noInterrupts(); // Menonaktifkan interupsi sementara
    rpm = (jumlahPulsa * 60.0) / 100.0;
    counter = 0;
    prevTime = millis();
    interrupts(); // Mengaktifkan interupsi kembali
  }

  ////////////////////// Tindakan //////////////////////

  // Pemeriksaan suhu untuk pemanas
  if (heaterAktif) {
    if (suhuMaksimum - suhuHisteresis > suhu) {
      heaterON(); // Aktifkan pemanas
    } else if (suhu > suhuMaksimum) {
      heaterOFF(); // Matikan pemanas
    }
  } else {
    heaterOFF(); // Jika pemanas tidak aktif, pastikan mati
  }

  // Pemeriksaan suhu terlampaui
  if (suhu > suhuMaksimum && !motorBerjalan) {
    motorBerjalan = true; // Aktifkan motor
    memulaiMotor = waktuSekarang; // Catat waktu mulai motor aktif
    mulaiMotor(); // Memulai motor
  }

  // Pemeriksaan waktu motor aktif terlampaui
  if (motorBerjalan && (waktuSekarang - memulaiMotor) > setWaktuMotor) {
    motorBerjalan = false; // Nonaktifkan motor
    hentikanMotor(); // Menghentikan motor
  }
            
  /////////////////// Kalkulasi ///////////////////

  kcpSudut = (rpm * 2 * PI) / 60.0; // Menghitung kecepatan sudut dalam radian per detik

  torsi = (volt * ampere) / ((2 * PI * rpm) / 60.0); // Menghitung torsi berdasarkan arus, RPM, dan tegangan input

  // Menghitung perbedaan torsi (T)
  float T = torsi - TNL;

  // Menghitung viskositas menggunakan rumus
  viskositas = (T * 60) / (4 * PI * PI * rpm * h) * (1 / (Ri * Ri) - 1 / (Ri * Ro));

  ////////////////////// Menu //////////////////////

  // Cek apakah sudah melewati waktu keluar menu dan menu saat ini bukan MENU_UTAMA
  if (menuSaatIni != MENU_UTAMA && (waktuSekarang - masukMenu) > waktuKeluarMenu) {
    // Jika ya, atur waktu keluar menu
    aturWaktuKeluarMenu();
  }

  // Pemilihan aksi berdasarkan menu saat ini
  switch (menuSaatIni) {
    case MENU_UTAMA:
      // Tampilkan menu utama
      tampilkanMenuUtama();
      break;
    case MENU_ATUR_SUHU_MAKS:
      // Atur suhu maksimum
      aturSuhuMaksimum();
      break;
    case MENU_ATUR_SUHU_MIN:
      // Atur suhu minimum
      aturSuhuMinimum();
      break;
    case MENU_HEATER_ON_OFF:
      heaterOnOff(); // Panggil fungsi menu heater On/Off
      break;
    case MENU_ATUR_WAKTU_MOTOR_AKTIF:
      // Atur waktu motor aktif
      aturWaktuMotorAktif();
      break;
    case MENU_ATUR_PWM_MOTOR:
      // Atur PWM motor
      aturPWMMotor();
      break;
    case MENU_START_STOP:
      // Mulai atau hentikan motor
      mulaiHentikanMotor();
      break;
    case MENU_TIMEOUT:
      // Atur waktu keluar menu
      aturWaktuKeluarMenu();
      break;
    default:
      //Tampilkan pesan error jika menu tidak terdefinisi
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Error. ");
      lcd.setCursor(0, 1);
      lcd.print("Coba Lagi !!");
      delay(3000);
      // Kembali ke menu utama
      menuSaatIni = MENU_UTAMA;
      lcd.clear();
      break;
  }
}

// Fungsi untuk menampilkan menu utama
void tampilkanMenuUtama() {
  // Dapatkan panjang tombol ditekan
  panjangTekan = dapatkanPanjangTekan();
  // Delay untuk menghindari bounce pada tombol
  delay(50);

  // Cek apakah tombol ditekan dengan panjang tertentu
  if (panjangTekan > panjangTekanSet) {
    // Ganti menu saat ini ke MENU_ATUR_SUHU_MAKS
    menuSaatIni = MENU_ATUR_SUHU_MAKS;
    // Set panjang tombol ke panjangTekanSet untuk menghindari duplikasi aksi
    panjangTekan = panjangTekanSet;
    // Delay untuk menghindari bounce pada tombol
    delay(50);
    // Simpan waktu saat menu dipanggil
    masukMenu = millis();
  }

  // Cek apakah tombol ditekan dengan panjang lebih pendek dari panjangTekanSet
  if (panjangTekan < panjangTekanSet) {
    // Set panjang tombol ke panjangTekanSet untuk menghindari duplikasi aksi
    panjangTekan = panjangTekanSet;
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  //  if (suhuAktual < 10)
  //  lcd.print(" ");
  lcd.print(suhu);
  lcd.setCursor(5, 0);
  lcd.print(" C  ");

  lcd.setCursor(9, 0);
  lcd.print(rpm);
  lcd.setCursor(12, 0);
  lcd.print(" RPM");

  lcd.setCursor(0, 1);
  //  if (ampeteAktual < 10)
  //  lcd.print(" ");
  lcd.print(ampere);
  lcd.setCursor(5, 1);
  lcd.print(" A ");

  lcd.setCursor(8, 1);
  lcd.print(viskositas);
  lcd.setCursor(13, 1);
  lcd.print(" cP");

  delay(500);
}

// Fungsi untuk mengatur suhu maksimum
void aturSuhuMaksimum() {
  while (menuSaatIni == MENU_ATUR_SUHU_MAKS) {
    if (millis() - masukMenu > waktuKeluarMenu) menuSaatIni = MENU_TIMEOUT;

    lcd.setCursor(0, 0);
    lcd.print("SET suhu Max:");
    lcd.setCursor(0, 1);
    lcd.print(suhuMaksimum, 1);
    lcd.print(" C ");
    lcd.setCursor(8, 1);
    lcd.print(" <<");
    lcd.print(suhuHisteresis, 1);
    lcd.print(" C");

    if (digitalRead(tmblDEC) == LOW) {
      suhuMaksimum = suhuMaksimum - 0.1;
      delay(250);
    }
    if (digitalRead(tmblINC) == LOW) {
      suhuMaksimum = suhuMaksimum + 0.1;
      delay(250);
    }

    if (suhuMaksimum < 1) suhuMaksimum = 1;
    if (suhuMaksimum > 98) suhuMaksimum = 98;

    if (digitalRead(tmblMENU) == LOW) {
      delay(250);
      lcd.clear();
      menuSaatIni = MENU_ATUR_SUHU_MIN; // Ganti ke menu pengaturan suhu minimum
    }
  }
  delay(100);
}

// Fungsi untuk mengatur suhu minimum
void aturSuhuMinimum() {
  while (menuSaatIni == MENU_ATUR_SUHU_MIN) {
    if (millis() - masukMenu > waktuKeluarMenu) menuSaatIni = MENU_TIMEOUT;

    lcd.setCursor(0, 0);
    lcd.print("SET Hys. Suhu:");
    lcd.setCursor(0, 1);
    lcd.print(suhuMaksimum, 1);
    lcd.print(" C ");
    lcd.setCursor(8, 1);
    lcd.print(">> ");
    lcd.print(suhuHisteresis, 1);
    lcd.print(" C");

    if (digitalRead(tmblDEC) == LOW) {
      suhuHisteresis = suhuHisteresis - 0.1;
      delay(250);
    }
    if (digitalRead(tmblINC) == LOW) {
      suhuHisteresis = suhuHisteresis + 0.1;
      delay(250);
    }

    if (suhuHisteresis < 0.1) suhuHisteresis = 0.1;
    if (suhuHisteresis > 2) suhuHisteresis = 2;

    if (digitalRead(tmblMENU) == LOW) {
      delay(250);
      lcd.clear();
      menuSaatIni = MENU_HEATER_ON_OFF; // Ganti ke menu pengaturan heater on off
    }
  }
  delay(100);
}

// Fungsi untuk mengatur heater in off
void heaterOnOff() {
  while (menuSaatIni == MENU_HEATER_ON_OFF) {
    if (millis() - masukMenu > waktuKeluarMenu) {
      menuSaatIni = MENU_TIMEOUT;
      break;
    }

    lcd.setCursor(0, 0);
    if (heaterAktif) {
      lcd.print("Heater ON!");
    } else {
      lcd.print("Heater off!");
    }
    lcd.setCursor(0, 1);
    lcd.print("OFF(-) ON(+)");

    if (digitalRead(tmblINC) == LOW) {
      heaterAktif = true;
      heaterON(); // Mengaktifkan pemanas
      delay(250);
    }

    if (digitalRead(tmblDEC) == LOW) {
      heaterAktif = false;
      heaterOFF(); // Menonaktifkan pemanas
      delay(250);
    }

    if (digitalRead(tmblMENU) == LOW) {
      delay(250);
      lcd.clear();
      menuSaatIni = MENU_ATUR_WAKTU_MOTOR_AKTIF; // Ganti ke menu pengaturan waktu motor aktif
    }
  }
  delay(100);
}
// Fungsi menyalakan heatet
void heaterON() {
  digitalWrite(relayHeater, HIGH);
}
// Fungsi mematikan heatet
void heaterOFF() {
  digitalWrite(relayHeater, LOW);
}

// Fungsi untuk mengatur waktu motor aktif
void aturWaktuMotorAktif() {
  while (menuSaatIni == MENU_ATUR_WAKTU_MOTOR_AKTIF) {
    if (millis() - masukMenu > waktuKeluarMenu) menuSaatIni = MENU_TIMEOUT;

    lcd.setCursor(0, 0);
    lcd.print("SET Mot Aktif:");
    lcd.setCursor(0, 1);
    lcd.print(" +/- : ");
    lcd.print(waktuMotor, 1);
    lcd.print(" detik");

    if (digitalRead(tmblDEC) == LOW) {
      waktuMotor = waktuMotor - 10;
      delay(250);
    }
    if (digitalRead(tmblINC) == LOW) {
      waktuMotor = waktuMotor + 10;
      delay(250);
    }

    if (waktuMotor < 10) waktuMotor = 10;
    if (waktuMotor > 60) waktuMotor = 60;

    if (digitalRead(tmblMENU) == LOW) {
      delay(250);
      lcd.clear();
      setWaktuMotor = waktuMotor * 1000; //ms
      menuSaatIni = MENU_ATUR_PWM_MOTOR; // Ganti ke menu pengaturan PWM motor
    }
  }
  delay(100);
}

// Fungsi untuk mengatur PWM motor
void aturPWMMotor() {
  while (menuSaatIni == MENU_ATUR_PWM_MOTOR) {
    if (millis() - masukMenu > waktuKeluarMenu) menuSaatIni = MENU_TIMEOUT;

    lcd.setCursor(0, 0);
    lcd.print("SET pwm Mot:");
    lcd.setCursor(0, 1);
    lcd.print("+/-: ");
    lcd.print(setPwmMotor, 1);
    lcd.print(" %");

    if (digitalRead(tmblDEC) == LOW) {
      setPwmMotor = setPwmMotor - 10;
      delay(250);
    }
    if (digitalRead(tmblINC) == LOW) {
      setPwmMotor = setPwmMotor + 10;
      delay(250);
    }

    if (setPwmMotor < 10) setPwmMotor = 10;
    if (setPwmMotor > 100) setPwmMotor = 100;

    if (digitalRead(tmblMENU) == LOW) {
      delay(250);
      lcd.clear();
      menuSaatIni = MENU_START_STOP; // Ganti ke menu untuk memulai atau menghentikan motor
    }
  }
  delay(100);
}

// Fungsi untuk memulai atau menghentikan motor
void mulaiHentikanMotor() {
  while (menuSaatIni == MENU_START_STOP) {
    if (millis() - masukMenu > waktuKeluarMenu) {
      menuSaatIni = MENU_TIMEOUT;
      break;
    }

    lcd.setCursor(0, 0);
    if (motorBerjalan) {
      lcd.print("Motor ON!");
    } else {
      lcd.print("Motor off!");
    }
    lcd.setCursor(0, 1);
    lcd.print("STOP(-) START(+)");

    if (digitalRead(tmblINC) == LOW) {
      if (!motorBerjalan) {
        motorBerjalan = true;
        mulaiMotor(); // Memulai motor jika belum berjalan
      }
    }

    if (digitalRead(tmblDEC) == LOW) {
      if (motorBerjalan) {
        motorBerjalan = false;
        hentikanMotor(); // Menghentikan motor jika sudah berjalan
      }
    }

    if (digitalRead(tmblMENU) == LOW) {
      delay(250);
      lcd.clear();
      menuSaatIni = MENU_UTAMA; // Kembali ke menu utama
    }
  }
  delay(100);
}

// Fungsi untuk memulai motor
void mulaiMotor() {
  digitalWrite(relayMotor, HIGH); // Gerak kan motor dengan menyalakan relay
  // Mengatur kecepatan motor berdasarkan setPwmMotor (0-100)
  int nilaiPWM = map(setPwmMotor, 0, 100, 0, 255); // Ubah nilai setPwmMotor (0-100) ke skala (0-255)
  analogWrite(kecepatsnMotor, nilaiPWM);
}

// Fungsi untuk menghentikan motor
void hentikanMotor() {
  digitalWrite(relayMotor, LOW); // Hentikan motor dengan mematikan relay
}

// Fungsi untuk mengatasi timeout pada menu
void aturWaktuKeluarMenu() {
  // Lakukan tindakan sesuai kebutuhan saat timeout terjadi
  // Misalnya, kembali ke menu utama
  menuSaatIni = MENU_UTAMA;
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print("Timeout!");
  lcd.setCursor(0, 1);
  lcd.print("Kembali ke Menu Utama");
  delay(3000); // Tahan pesan timeout selama 3 detik sebelum kembali ke menu utama

  masukMenu = millis(); // Atur ulang variabel masukMenu ke waktu saat ini
}

// Menambahkan penanganan timeout pada menu
void handleTimeout() {
  if (millis() - masukMenu > waktuKeluarMenu) {
    menuSaatIni = MENU_TIMEOUT;
    lcd.clear();
    lcd.setCursor(1, 0);
    lcd.print("Kembali ke ->");
    lcd.setCursor(0, 1);
    lcd.print("menu Utama");
    delay(3000);
    masukMenu = millis(); // Atur ulang variabel masukMenu ke waktu saat ini
  }
}

// Fungsi untuk mendapatkan panjang tombol ditekan
int dapatkanPanjangTekan() {
  // Baca status tombol
  tombolState = digitalRead(tmblMENU);

  // Cek apakah tombol ditekan (LOW) dan tombolFlag belum aktif
  if (tombolState == LOW && tombolFlag == false) {
    // Simpan waktu saat tombol mulai ditekan
    waktuTekanMulai = millis();
    // Set tombolFlag menjadi true untuk menandakan tombol sedang ditekan
    tombolFlag = true;
  }

  // Cek apakah tombol dilepas (HIGH) dan tombolFlag aktif
  if (tombolState == HIGH && tombolFlag == true) {
    // Simpan waktu saat tombol dilepas
    waktuTekanStop = millis();
    // Hitung panjang tombol ditekan dalam milidetik
    panjangTekan = waktuTekanStop - waktuTekanMulai;
    // Set tombolFlag menjadi false untuk menandakan tombol telah dilepas
    tombolFlag = false;
  }
  // Kembalikan nilai panjang tombol ditekan
  return panjangTekan;
}

void countPulse() {
  // Callback fungsi ini dipanggil ketika ada pulsa di pin A
  counter++;
}
