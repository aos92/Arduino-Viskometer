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

const int TOMBOL_DEC = 6; // Pin tombol untuk pengurangan
const int TOMBOL_INC = 7;  // Pin tombol untuk penambahan
const int TOMBOL_MENU = 8;  // Pin tombol untuk menu
const int RELAY_HEATER = 4; // Pin Relay Heater
const int RELAY_MOTOR = 5; // Pin Relay Motor DC
const int PWM_MOTOR = 9; // Pin kecepatan Motor DC
const int SENSOR_SUHU = 10; // Pin sensor DS18B20
const int SENSOR_VOLT = A0; // Pin sensor voltage
const int SENSOR_ARUS = A1; // Pin sensor INA169
const int SENSOR_SPEED = 2; // Hubungkan dengan pin A sensor


volatile int COUNTER = 0; // Menghitung jumlah pulsa
volatile unsigned long PREV_TIME = 0; // Waktu sebelumnya untuk penghitungan RPM


OneWire oneWire(SENSOR_SUHU); // Inisialisasi objek OneWire untuk sensor suhu
DallasTemperature sensor(&oneWire); // Inisialisasi objek DallasTemperature untuk sensor suhu

int resolusi = 11;
int rpm = 0;
float suhu = 0.0;
float volt = 0.0;
float ampere = 0.0;
float torsi = 0.0;
float kcpSudut = 0.0;
float viskositas = 0.0;

const byte MENU_UTAMA = 0; // Konstanta untuk menu utama
const byte MENU_SET_SUHU_IDEAL = 1; // Konstanta untuk menu pengaturan suhu maksimum
const byte MENU_SET_SUHU_HISTERESIS = 2; // Konstanta untuk menu pengaturan suhu histeresis
const byte MENU_SET_STATUS_HEATER = 3; // Konstanta untuk menu memulai atau menghentikan heater
const byte MENU_SET_MOTOR_AKTIF = 4; // Konstanta untuk menu pengaturan waktu motor aktif
const byte MENU_SET_PWM_MOTOR = 5; // Konstanta untuk menu pengaturan PWM motor
const byte MENU_SET_STATUS_MOTOR = 6; // Konstanta untuk menu memulai atau menghentikan motor
const byte MENU_TIMEOUT = 7; // Konstanta untuk menu timeout
byte menuSekarang = MENU_UTAMA; // Variabel untuk menyimpan menu saat ini

int tombolState = 0; // Variabel untuk menyimpan status tombol
int panjangTekanSet = 200; // Variabel untuk panjang tekan tombol yang ditetapkan
int panjangTekan = panjangTekanSet; // Variabel untuk panjang tekan tombol
int waktuTekanMulai = 0; // Waktu tombol mulai ditekan
int waktuTekanStop = 0; // Waktu tombol dilepas
boolean tombolFlag = false; // Variabel flag untuk mengontrol tombol ditekan atau tidak
unsigned long masukMenu; // Waktu saat masuk ke menu
unsigned long waktuKeluarMenu = 60000; // Waktu untuk keluar dari menu (timeout)


unsigned long memulaiMotor; // Waktu mulai motor aktif
boolean motor = false; //  status motor apakah sedang aktif atau tidak
boolean heater = false; // Status pemanas aktif atau tidak

float suhuMaksimum = 25.0;
float suhuHisteresis = 1.0;
unsigned long setWaktuMotor = 3000;
int setPwmMotor = 50;

//float baca_A, baca_B, nilaiADC_A, nilaiADC_B;

//////////// Fungsi untuk melakukan setup awal ///////////
void setup() {
  lcd.begin(16, 2);
  lcd.backlight();

  // Set pin relay heater sebagai OUTPUT dan matikan relay
  pinMode(RELAY_HEATER, OUTPUT);
  digitalWrite(RELAY_HEATER, LOW);

  // Set pin relay motor sebagai OUTPUT dan matikan relay
  pinMode(RELAY_MOTOR, OUTPUT);
  digitalWrite(RELAY_MOTOR, LOW);

  // Set pin tombol DEC, INC, dan MENU sebagai INPUT
  pinMode(TOMBOL_DEC, INPUT);
  pinMode(TOMBOL_INC, INPUT);
  pinMode(TOMBOL_MENU, INPUT);

  // Atur status awal tombol DEC, INC, dan MENU sebagai HIGH (pull-up internal)
  digitalWrite(TOMBOL_DEC, HIGH);
  digitalWrite(TOMBOL_INC, HIGH);
  digitalWrite(TOMBOL_MENU, HIGH);

  // Konfigurasi pin sebagai input
  pinMode(SENSOR_SPEED, INPUT);

  // Mulai komunikasi dengan sensor suhu
  sensor.begin();
  // Set resolusi sensor suhu
  sensor.setResolution(resolusi);
  // Delay untuk stabilisasi sensor
  delay(250 / (1 << (12 - resolusi)));

  // Attach interrupt untuk pin A
  attachInterrupt(digitalPinToInterrupt(SENSOR_SPEED), countPulse, RISING);

}

/////////// Loop utama program ////////////
void loop() {
  // Mendapatkan waktu saat ini dalam milidetik
  unsigned long currentMillis = millis();

  // Mengambil nilai waktuSekarang sebelum perhitungan
  unsigned long  waktuSekarang = currentMillis;

  //handleTimeout();
  float baca_A, baca_B, nilaiADC_A, nilaiADC_B;
  //int jumlahPulsa = counter;

  /////////////////////// Pengecekan ///////////////////////

  sensor.requestTemperatures();
  suhu = sensor.getTempCByIndex(0);
  delay(150 / (1 << (12 - resolusi)));


  baca_A = analogRead(SENSOR_VOLT); // Ubah nilai analog menjadi tegangan
  nilaiADC_A = (baca_A * 5.0) / 1023.0;
  volt = nilaiADC_A / (7500.0 / (30000.0 + 7500.0));


  baca_B = analogRead(SENSOR_ARUS); // Ubah nilai analog menjadi arus
  nilaiADC_B = (baca_B * 5.0) / 1023.0;
  // Is = (Vout x 1k) / (RS x RL)
  ampere = (nilaiADC_B * 100) / (10 * 10);

  if (millis() - PREV_TIME >= 1000) {
    noInterrupts(); // Menonaktifkan interupsi sementara
    rpm = (COUNTER * 60.0) / 100.0; // dist plate 100 bole
    COUNTER = 0;
    PREV_TIME = millis();
    interrupts(); // Mengaktifkan interupsi kembali
  }

  ////////////////////// Tindakan //////////////////////

  // Pemeriksaan suhu untuk pemanas
  if (heater) {
    if (suhuMaksimum - suhuHisteresis > suhu) {
      heaterON(); // Aktifkan pemanas
    } else if (suhu > suhuMaksimum) {
      heaterOFF(); // Matikan pemanas
    }
  } else {
    heaterOFF(); // Jika pemanas tidak aktif, pastikan mati
  }

  // Pemeriksaan suhu terlampaui
  if (suhu > suhuMaksimum && !motor) {
    motor = true; // Aktifkan motor
    memulaiMotor = waktuSekarang; // Catat waktu mulai motor aktif
    startMotor(); // Memulai motor
  }

  // Pemeriksaan waktu motor aktif terlampaui
  if (motor && (waktuSekarang - memulaiMotor) > setWaktuMotor) {
    motor = false; // Nonaktifkan motor
    stopMotor(); // Menghentikan motor
  }

  /////////////////// Kalkulasi ///////////////////

  kcpSudut = (rpm * 2 * PI) / 60.0; // Menghitung kecepatan sudut dalam radian per detik

  torsi = (volt * ampere) / ((2 * PI * rpm) / 60.0); // Menghitung torsi berdasarkan arus, RPM, dan tegangan input

  // Menghitung perbedaan torsi (T)
  float T = torsi - 0.5;

  // Menghitung viskositas menggunakan rumus
  viskositas = (T * 60) / (4 * PI * PI * rpm * 0.05) * (1 / (0.03 * 0.03) - 1 / (0.03 * 0.04));

  /*
    // inisialisasi nilai-nilai parameter
    TL = torsi; // Torsi pada kondisi ada beban (Nm)
    TNL = 0.0; // Torsi pada kondisi tanpa beban (Nm)
    NL = rpm; // Kecepatan poros pada kondisi ada beban (mtr)
    h = 0.05; // Ketinggian lapisan cairan (mtr)
    Ri = 0.03; // Jari-jari silinder dalam (mtr)
    Ro = 0.04; // Jari-jari silinder luar (mtr)
  */

  ////////////////////// Menu //////////////////////

  // Cek apakah sudah melewati waktu keluar menu dan menu saat ini bukan MENU_UTAMA
  if (menuSekarang != MENU_UTAMA && (waktuSekarang - masukMenu) > waktuKeluarMenu) {
    // Jika ya, atur waktu keluar menu
    aturWaktuKeluarMenu();
  }

  // Pemilihan aksi berdasarkan menu saat ini
  switch (menuSekarang) {
    case MENU_UTAMA:
      // Tampilkan menu utama
      menuUtama();
      break;
    case MENU_SET_SUHU_IDEAL:
      // Atur suhu maksimum
      aturSuhuIdeal();
      break;
    case MENU_SET_SUHU_HISTERESIS:
      // Atur suhu minimum
      aturSuhuHistetesis();
      break;
    case MENU_SET_STATUS_HEATER:
      aturStatusHeater(); // Panggil fungsi menu heater On/Off
      break;
    case MENU_SET_MOTOR_AKTIF:
      // Atur waktu motor aktif
      aturMotorAktif();
      break;
    case MENU_SET_PWM_MOTOR:
      // Atur PWM motor
      aturPWMMotor();
      break;
    case MENU_SET_STATUS_MOTOR:
      // Mulai atau hentikan motor
      aturStatusMotor();
      break;
    case MENU_TIMEOUT:
      // Atur waktu keluar menu
      aturWaktuKeluarMenu();
      break;
    default:
      break;
  }
}

// Fungsi untuk menampilkan menu utama
void menuUtama() {
  // Dapatkan panjang tombol ditekan
  panjangTekan = dapatkanPanjangTekan();
  // Delay untuk menghindari bounce pada tombol
  delay(50);

  // Cek apakah tombol ditekan dengan panjang tertentu
  if (panjangTekan > panjangTekanSet) {
    // Ganti menu saat ini ke MENU_ATUR_SUHU_MAKS
    menuSekarang = MENU_SET_SUHU_IDEAL;
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
  lcd.print(suhu);
  lcd.setCursor(5, 0);
  lcd.print(" C  ");

  lcd.setCursor(9, 0);
  lcd.print(rpm);
  lcd.setCursor(12, 0);
  lcd.print(" RPM");

  lcd.setCursor(0, 1);
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
void aturSuhuIdeal() {
  while (menuSekarang == MENU_SET_SUHU_IDEAL) {
    if (millis() - masukMenu > waktuKeluarMenu) menuSekarang = MENU_TIMEOUT;

    lcd.setCursor(0, 0);
    lcd.print("SET suhu Max:");
    lcd.setCursor(0, 1);
    lcd.print(suhuMaksimum, 1);
    lcd.print(" C ");
    lcd.setCursor(8, 1);
    lcd.print(" <<");
    lcd.print(suhuHisteresis, 1);
    lcd.print(" C");

    if (digitalRead(TOMBOL_DEC) == LOW) {
      suhuMaksimum = suhuMaksimum - 0.1;
      delay(250);
    }
    if (digitalRead(TOMBOL_INC) == LOW) {
      suhuMaksimum = suhuMaksimum + 0.1;
      delay(250);
    }

    if (suhuMaksimum < 1) suhuMaksimum = 1;
    if (suhuMaksimum > 98) suhuMaksimum = 98;

    if (digitalRead(TOMBOL_MENU) == LOW) {
      delay(250);
      lcd.clear();
      menuSekarang = MENU_SET_SUHU_HISTERESIS; // Ganti ke menu pengaturan suhu minimum
    }
  }
  delay(100);
}

// Fungsi untuk mengatur suhu minimum
void aturSuhuHistetesis() {
  while (menuSekarang == MENU_SET_SUHU_HISTERESIS) {
    if (millis() - masukMenu > waktuKeluarMenu) menuSekarang = MENU_TIMEOUT;

    lcd.setCursor(0, 0);
    lcd.print("SET Hys. Suhu:");
    lcd.setCursor(0, 1);
    lcd.print(suhuMaksimum, 1);
    lcd.print(" C ");
    lcd.setCursor(8, 1);
    lcd.print(">> ");
    lcd.print(suhuHisteresis, 1);
    lcd.print(" C");

    if (digitalRead(TOMBOL_DEC) == LOW) {
      suhuHisteresis = suhuHisteresis - 0.1;
      delay(250);
    }
    if (digitalRead(TOMBOL_INC) == LOW) {
      suhuHisteresis = suhuHisteresis + 0.1;
      delay(250);
    }

    if (suhuHisteresis < 0.1) suhuHisteresis = 0.1;
    if (suhuHisteresis > 2) suhuHisteresis = 2;

    if (digitalRead(TOMBOL_MENU) == LOW) {
      delay(250);
      lcd.clear();
      menuSekarang = MENU_SET_STATUS_HEATER; // Ganti ke menu pengaturan heater on off
    }
  }
  delay(100);
}

// Fungsi untuk mengatur heater in off
void aturStatusHeater() {
  while (menuSekarang == MENU_SET_STATUS_HEATER) {
    if (millis() - masukMenu > waktuKeluarMenu) {
      menuSekarang = MENU_TIMEOUT;
      break;
    }

    lcd.setCursor(0, 0);
    if (heater) {
      lcd.print("Heater ON!");
    } else {
      lcd.print("Heater off!");
    }
    lcd.setCursor(0, 1);
    lcd.print("OFF(-) ON(+)");

    if (digitalRead(TOMBOL_INC) == LOW) {
      heater = true;
      heaterON(); // Mengaktifkan pemanas
      delay(250);
    }

    if (digitalRead(TOMBOL_DEC) == LOW) {
      heater = false;
      heaterOFF(); // Menonaktifkan pemanas
      delay(250);
    }

    if (digitalRead(TOMBOL_MENU) == LOW) {
      delay(250);
      lcd.clear();
      menuSekarang = MENU_SET_MOTOR_AKTIF; // Ganti ke menu pengaturan waktu motor aktif
    }
  }
  delay(100);
}
// Fungsi menyalakan heatet
void heaterON() {
  digitalWrite(RELAY_HEATER, HIGH);
}
// Fungsi mematikan heatet
void heaterOFF() {
  digitalWrite(RELAY_HEATER, LOW);
}

// Fungsi untuk mengatur waktu motor aktif
void aturMotorAktif() {
  while (menuSekarang == MENU_SET_MOTOR_AKTIF) {
    if (millis() - masukMenu > waktuKeluarMenu) menuSekarang = MENU_TIMEOUT;

    lcd.setCursor(0, 0);
    lcd.print("SET Mot Aktif:");
    lcd.setCursor(0, 1);
    lcd.print(" +/- : ");
    lcd.print(setWaktuMotor, 1);
    lcd.print(" detik");

    if (digitalRead(TOMBOL_DEC) == LOW) {
      setWaktuMotor = setWaktuMotor - 10;
      delay(250);
    }
    if (digitalRead(TOMBOL_INC) == LOW) {
      setWaktuMotor = setWaktuMotor + 10;
      delay(250);
    }

    if (setWaktuMotor < 10) setWaktuMotor = 10;
    if (setWaktuMotor > 60) setWaktuMotor = 60;

    if (digitalRead(TOMBOL_MENU) == LOW) {
      delay(250);
      lcd.clear();
      setWaktuMotor = setWaktuMotor * 1000; //ms
      menuSekarang = MENU_SET_PWM_MOTOR; // Ganti ke menu pengaturan PWM motor
    }
  }
  delay(100);
}

// Fungsi untuk mengatur PWM motor
void aturPWMMotor() {
  while (menuSekarang == MENU_SET_PWM_MOTOR) {
    if (millis() - masukMenu > waktuKeluarMenu) menuSekarang = MENU_TIMEOUT;

    lcd.setCursor(0, 0);
    lcd.print("SET pwm Mot:");
    lcd.setCursor(0, 1);
    lcd.print("+/-: ");
    lcd.print(setPwmMotor, 1);
    lcd.print(" %");

    if (digitalRead(TOMBOL_DEC) == LOW) {
      setPwmMotor = setPwmMotor - 10;
      delay(250);
    }
    if (digitalRead(TOMBOL_INC) == LOW) {
      setPwmMotor = setPwmMotor + 10;
      delay(250);
    }

    if (setPwmMotor < 10) setPwmMotor = 10;
    if (setPwmMotor > 100) setPwmMotor = 100;

    if (digitalRead(TOMBOL_MENU) == LOW) {
      delay(250);
      lcd.clear();
      menuSekarang = MENU_SET_STATUS_MOTOR; // Ganti ke menu untuk memulai atau menghentikan motor
    }
  }
  delay(100);
}

// Fungsi untuk memulai atau menghentikan motor
void aturStatusMotor() {
  while (menuSekarang == MENU_SET_STATUS_MOTOR) {
    if (millis() - masukMenu > waktuKeluarMenu) {
      menuSekarang = MENU_TIMEOUT;
      break;
    }

    lcd.setCursor(0, 0);
    if (motor) {
      lcd.print("Motor ON!");
    } else {
      lcd.print("Motor off!");
    }
    lcd.setCursor(0, 1);
    lcd.print("STOP(-) START(+)");

    if (digitalRead(TOMBOL_INC) == LOW) {
      motor = true;
      startMotor(); // Memulai motor
      delay(250);
    }

    if (digitalRead(TOMBOL_DEC) == LOW) {
      motor = false;
      stopMotor(); // Menghentikan motor
      delay(250);
    }

    if (digitalRead(TOMBOL_MENU) == LOW) {
      delay(250);
      lcd.clear();
      menuSekarang = MENU_UTAMA; // Kembali ke menu utama
    }
  }
  delay(100);
}

// Fungsi untuk memulai motor
void startMotor() {
  digitalWrite(RELAY_MOTOR, HIGH); // Gerak kan motor dengan menyalakan relay
  // Mengatur kecepatan motor berdasarkan setPwmMotor (0-100)
  int nilaiPWM = map(setPwmMotor, 0, 100, 0, 255); // Ubah nilai setPwmMotor (0-100) ke skala (0-255)
  analogWrite(PWM_MOTOR, nilaiPWM);
}

// Fungsi untuk menghentikan motor
void stopMotor() {
  digitalWrite(RELAY_MOTOR, LOW); // Hentikan motor dengan mematikan relay
  int nilaiPWM = 0;
  analogWrite(PWM_MOTOR, nilaiPWM);
}

// Fungsi untuk mengatasi timeout pada menu
void aturWaktuKeluarMenu() {
  // Misalnya, kembali ke menu utama
  menuSekarang = MENU_UTAMA;
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print("Timeout!");
  lcd.setCursor(0, 1);
  lcd.print("Kembali ke Menu Utama");
  delay(3000); // Tahan pesan timeout selama 3 detik sebelum kembali ke menu utama
  masukMenu = millis(); // Atur ulang variabel masukMenu ke waktu saat ini
}

/*
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
*/

// Fungsi untuk mendapatkan panjang tombol ditekan
int dapatkanPanjangTekan() {
  // Baca status tombol
  tombolState = digitalRead(TOMBOL_MENU);

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
  COUNTER++;
}
