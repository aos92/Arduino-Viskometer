# Arduino-Viskometer

# Sistem Pengukuran Viskositas Menggunakan Arduino

Repository ini berisi skrip Arduino untuk sistem pengukuran viskositas menggunakan berbagai sensor dan komponen. Sistem ini dirancang untuk mengukur viskositas dengan menganalisis suhu, RPM motor, dan torsi. Skrip Arduino dalam repository ini memberikan fungsionalitas untuk mengatur pengaturan, mengontrol pemanas dan motor, serta menampilkan informasi pada layar LCD.

## Daftar Isi
- [Diagram-](#diagram)
- [Pendahuluan](#pendahuluan)
- [Komponen](#komponen)
- [Koneksi](#koneksi)
- [Instalasi](#instalasi)
- [Penggunaan](#penggunaan)
- [Rumus](#rumus)
- [Fitur-](#fitur)
- [Kontribusi](#kontribusi)
- [Lisensi](#lisensi)

## Diagram

Berikut adalah diagram alur kerja atau diagram aliran yang menggambarkan bagaimana sistem viskometer Arduino ini berfungsi:

```plaintext
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
  |------[Timeout]--> Penanganan Timeout
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
```

## Pendahuluan

Proyek ini bertujuan untuk menciptakan sistem pengukuran viskositas menggunakan Arduino. Sistem ini menggunakan sensor suhu, pengukuran RPM, dan perhitungan torsi untuk menentukan viskositas dari suatu cairan. Skrip Arduino menyediakan antarmuka pengguna melalui layar LCD dan tombol-tombol untuk mengontrol berbagai parameter sistem.

## Komponen

- Papan Arduino
- Sensor suhu (DS18B20)
- Sensor RPM
- Sensor torsi
- Modul relay (untuk pengendalian pemanas dan motor)
- Layar LCD (LiquidCrystal_I2C)

## Koneksi

Berikut adalah skema koneksi komponen pada proyek ini:

1. Sensor Suhu DS18B20 dihubungkan ke pin `10`.
2. Sensor ACS612 (sensor arus) dihubungkan ke pin `A0`.
3. Sensor encoder dihubungkan ke pin `2`.
4. Tombol DEC dihubungkan ke pin `6`.
5. Tombol INC dihubungkan ke pin `7`.
6. Tombol MENU dihubungkan ke pin `8`.
7. Relay Heater dihubungkan ke pin `4`.
8. Relay Motor DC dihubungkan ke pin `5`.
9. Kecepatan Motor DC dihubungkan ke pin `9`.
10. LCD menggunakan komunikasi I2C dengan alamat `0x27`.

Pastikan Anda mengikuti skema koneksi dengan benar untuk memastikan komponen berfungsi sesuai harapan.

## Instalasi

1. Clone atau unduh repository ini ke komputer lokal Anda.
2. Unggah skrip Arduino (`viskometer.ino`) ke papan Arduino menggunakan Arduino IDE.
3. Hubungkan sensor dan komponen yang diperlukan sesuai instruksi kabel pada komentar kode.

## Penggunaan

1. **Instalasi Hardware**

   - Sambungkan sensor suhu DS18B20 ke pin yang telah ditentukan pada papan Arduino.
   - Hubungkan tombol penambahan (INC), tombol pengurangan (DEC), dan tombol menu ke pin yang sesuai pada papan Arduino.
   - Sambungkan relay pemanas (heater) dan relay motor ke pin yang telah ditentukan.
   - Pastikan komponen terhubung sesuai dengan skema koneksi yang disediakan.

2. **Instalasi Perangkat Lunak**

   - Unduh skrip Arduino yang terlampir (`viskometer.ino`).
   - Buka skrip tersebut menggunakan Arduino IDE.
   - Pilih model papan Arduino yang sesuai di dalam Arduino IDE.
   - Unggah skrip ke papan Arduino Anda.

3. **Interaksi dengan Sistem**

   - Setelah papan Arduino dihidupkan, layar LCD akan menampilkan informasi suhu, RPM, arus, dan viskositas aktual.
   - Anda dapat menggunakan tombol penambahan (INC) dan tombol pengurangan (DEC) untuk berinteraksi dengan menu pengaturan seperti pengaturan suhu maksimum, suhu histeresis, waktu motor aktif, dan PWM motor.
   - Tombol menu dapat digunakan untuk beralih antara menu pengaturan dan menu utama.

4. **Memulai dan Menghentikan Motor serta Pemanas**

   - Di menu utama, Anda dapat memulai atau menghentikan motor dengan tombol start (tombol INC) dan stop (tombol DEC).
   - Anda dapat mengaktifkan atau menonaktifkan pemanas dengan memilih menu "Heater ON/OFF."

## Rumus

Skrip ini melibatkan rumus untuk menghitung viskositas menggunakan torsi, RPM, dan parameter lainnya. Rumusnya dijelaskan sebagai berikut:

```
μ = (T * 60) / (4 * π^2 * rpm * h) * (1 / (Ri^2) - 1 / (Ri * Ro))
```

- `μ` adalah viskositas cairan dalam unit cP (centipoise).
- `T` adalah perbedaan torsi antara kondisi tanpa beban dan dengan beban (Nm).
- `rpm` adalah kecepatan rotasi poros dalam RPM (revolusi per menit).
- `h` adalah ketinggian lapisan cairan dalam meter.
- `Ri` adalah jari-jari silinder dalam dalam meter.
- `Ro` adalah jari-jari silinder luar dalam meter.

Contoh: Jika `T` adalah 0.5 Nm, `rpm` adalah 200, `h` adalah 0.05 m, `Ri` adalah 0.03 m, dan `Ro` adalah 0.04 m, maka viskositas cairan akan dihitung.

Anda dapat mengganti nilai-nilai variabel tersebut dalam skrip untuk menghitung viskositas cairan dengan rumus di atas.

## Fitur

Selain fungsi utama, skrip ini juga memiliki beberapa fitur tambahan, antara lain:

- Pengaturan suhu maksimum dan suhu histeresis untuk mengendalikan pemanas.
- Pengaturan waktu motor aktif dan PWM motor untuk mengatur durasi dan kecepatan motor.
- Mulai/hentikan motor secara manual.
- Tampilan waktu timeout saat tidak ada interaksi dengan tombol.

## Kontribusi

Jika Anda ingin berkontribusi pada pengembangan skrip ini, silakan buat _pull request_ atau ajukan _issue_ di repositori ini.

## Lisensi

Proyek ini dilisensikan di bawah Lisensi Umum GNU, Versi 3.0. Untuk informasi lebih lanjut, lihat berkas [LICENSE](LICENSE).

Anda diperbolehkan berkontribusi pada proyek ini atau menggunakannya untuk tujuan Anda sendiri!
```

