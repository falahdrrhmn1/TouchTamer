<p align="center">
  <img src="assets/icon.png" width="96" alt="Ikon TouchTamer">
</p>

<h1 align="center">TouchTamer</h1>

<p align="center">
  Gestur touchpad untuk Windows: atur brightness dan volume dengan menggeser jari, dan pindahkan jendela dengan lima jari.
</p>

<p align="center">
  <a href="README.md">English</a> · <b>Bahasa Indonesia</b>
</p>

---

TouchTamer adalah program kecil yang berjalan di latar belakang untuk laptop Windows. Gesturnya sama di semua aplikasi, termasuk di desktop kosong.

| Yang ingin dilakukan | Caranya |
|---|---|
| Mengatur **brightness** | Tahan `Ctrl`+`Shift`, lalu geser naik atau turun dengan **2 jari** di **sisi kiri** touchpad |
| Mengatur **volume** | Tahan `Ctrl`+`Shift`, lalu geser naik atau turun dengan **2 jari** di **sisi kanan** touchpad |
| **Memindahkan jendela** | Arahkan kursor ke jendela, lalu geser dengan **5 jari** |
| Mengubah **ukuran jendela** | `Alt`+`Atas` dan `Alt`+`Bawah` |
| Mengubah **lebar jendela** | `Alt`+`Kanan` dan `Alt`+`Kiri` |

Programnya hanya satu file kecil, sekitar 130 KB. RAM khususnya sekitar 2,6 MB, CPU mendekati 0% saat diam, tidak perlu hak administrator, dan tidak pernah terhubung ke internet.

## Daftar isi

- [Apakah cocok untuk laptop saya?](#apakah-cocok-untuk-laptop-saya)
- [Cara memasang](#cara-memasang)
- [Cara memakai](#cara-memakai)
- [Mengubah pengaturan](#mengubah-pengaturan)
- [Menghentikan, menjalankan ulang, atau mematikan saat startup](#menghentikan-menjalankan-ulang-atau-mematikan-saat-startup)
- [Cara menghapus](#cara-menghapus)
- [Kalau ada masalah](#kalau-ada-masalah)
- [Batasan yang sudah diketahui](#batasan-yang-sudah-diketahui)
- [Privasi](#privasi)
- [Membangun dari kode sumber](#membangun-dari-kode-sumber)
- [Lisensi](#lisensi)

## Apakah cocok untuk laptop saya?

> **Perlu diketahui:** sejauh ini TouchTamer baru diuji di satu laptop, yaitu Lenovo IdeaPad Flex 5 14ITL05 dengan Windows 11. Laptop lain seharusnya bisa kalau memenuhi syarat di bawah, tetapi itu belum terbukti. Kalau Anda mencobanya, tolong [buat issue](https://github.com/falahdrrhmn1/TouchTamer/issues) dan ceritakan hasilnya.

| Yang dibutuhkan | Untuk | Cara mengecek |
|---|---|---|
| Windows 10 atau 11, 64-bit | Semua fitur | Tekan `Win`+`Pause`, atau buka **Settings → System → About**. Windows 10 belum diuji |
| **Precision Touchpad** | Semua gestur | Lihat di bawah |
| Touchpad yang bisa membaca **5 jari** | Memindahkan jendela | Installer memberi tahu di Windows 11 24H2 atau lebih baru |
| **Layar bawaan laptop** | Brightness | Monitor eksternal tidak didukung. Gesturnya tidak melakukan apa-apa di sana |

Hotkey `Alt`+panah tetap jalan tanpa touchpad.

### Mengecek touchpad

- **Windows 11:** buka **Settings → Bluetooth & devices → Touchpad**. Touchpad Anda termasuk Precision Touchpad kalau ada bagian **Three-finger gestures** dan **Four-finger gestures**.
- **Windows 10:** buka **Settings → Devices → Touchpad**. Touchpad Anda termasuk Precision Touchpad kalau halamannya bertuliskan **Your PC has a precision touchpad**.

## Cara memasang

### 1. Unduh

1. Buka <https://github.com/falahdrrhmn1/TouchTamer>.
2. Klik tombol hijau **Code**, lalu **Download ZIP**.

Atau pakai tautan langsung ini: [unduh ZIP](https://github.com/falahdrrhmn1/TouchTamer/archive/refs/heads/main.zip).

### 2. Ekstrak ZIP

1. Buka folder **Downloads**.
2. Klik kanan `TouchTamer-main.zip`, lalu pilih **Extract All…** dan **Extract**.
3. Buka folder baru `TouchTamer-main`. Di dalamnya harus ada `install.cmd`.

Jangan jalankan installer langsung dari dalam ZIP tanpa diekstrak, karena file-file yang dibutuhkannya tidak akan ditemukan.

### 3. Jalankan installer

Klik dua kali **`install.cmd`**.

Kalau muncul jendela biru bertuliskan **Windows protected your PC**, klik **More info**, lalu **Run anyway**. Windows menampilkan pesan ini untuk program tanpa tanda tangan digital berbayar, dan TouchTamer memang tidak punya. [Cek file-nya sendiri](#cek-file-sendiri) kalau ingin lebih yakin.

Installer melakukan hal berikut:

- Mengecek versi Windows dan touchpad.
- Menyalin program ke `%LOCALAPPDATA%\Programs\TouchTamer`.
- Menambahkan shortcut supaya TouchTamer jalan saat Anda masuk ke Windows.
- Langsung menjalankan TouchTamer.

Installer tidak meminta hak administrator. Hasilnya tampil di jendela hitam. Tekan tombol apa saja untuk menutupnya.

### 4. Coba

TouchTamer tidak punya jendela atau ikon di tray. Program ini berjalan di latar belakang, dan Anda hanya melihat panel kecil saat ada nilai yang berubah.

1. Tahan `Ctrl`+`Shift`.
2. Tempelkan 2 jari di sisi kiri touchpad, lalu geser ke atas.
3. Panel berisi persen brightness muncul di bagian bawah layar.

### 5. Simpan foldernya

Simpan folder hasil ekstrak kalau nanti ingin memakai `uninstall.cmd`. Boleh juga dihapus, lalu diunduh lagi saat dibutuhkan, atau [hapus TouchTamer secara manual](#menghapus-secara-manual).

### Cek file sendiri

Sidik jari yang seharusnya ada di [`bin/TouchTamer.exe.sha256`](bin/TouchTamer.exe.sha256). Untuk membandingkannya, buka PowerShell di folder hasil ekstrak, lalu jalankan:

```powershell
Get-FileHash .\bin\TouchTamer.exe -Algorithm SHA256
```

Kedua nilainya harus sama. Anda juga bisa membaca kode sumbernya di [`src`](src) dan [membangun programnya sendiri](#membangun-dari-kode-sumber).

## Cara memakai

### Brightness dan volume

1. Tahan `Ctrl`+`Shift`, dan terus tahan selama menggeser.
2. Tempelkan 2 jari di touchpad:
   - **Sisi kiri** untuk brightness.
   - **Sisi kanan** untuk volume.
3. Geser **ke atas untuk menambah** atau **ke bawah untuk mengurangi**.

Perlu diketahui:

- Sisi ditentukan saat geseran dimulai. Setelah itu jari boleh melewati bagian tengah.
- Geseran setinggi touchpad mengubah nilai sebesar 60%. Angka ini [bisa diubah](#mengubah-pengaturan).
- Scroll 2 jari biasa, tanpa `Ctrl`+`Shift`, tidak terpengaruh.

### Memindahkan jendela

1. Arahkan kursor ke jendela yang ingin dipindah.
2. Tempelkan **5 jari** di touchpad.
3. Geser ke arah mana saja. Jendela dan kursor bergerak bersama.
4. Angkat jari untuk selesai.

Perlu diketahui:

- **Memindah jauh:** angkat jari, tempelkan lagi, lalu lanjutkan. Jendelanya tetap sama, karena kursor masih berada di atasnya.
- **Jendela full screen:** jendela kembali dulu ke ukuran normal, lalu mengikuti jari. Ini sama seperti menyeret judul jendela yang sedang maximize.
- **Yang diabaikan:** desktop, taskbar, dan layar penuh tanpa bingkai, seperti game, video, dan browser dalam mode `F11`.
- **Jendela tidak bisa hilang:** bagian atas jendela selalu bisa dijangkau di layar.
- **Tiga dan empat jari:** gestur Windows Anda tetap jalan, karena TouchTamer hanya bereaksi pada lima jari.

### Ukuran jendela: Alt+Atas dan Alt+Bawah

Tombol ini mengubah jendela yang sedang aktif dengan urutan berikut:

| Level | Ukuran |
|---|---|
| Tertinggi | Full screen sungguhan (maximize) |
| 2 | 95% dari layar, di tengah |
| 3 | 90%, di tengah |
| 4 | 75%, di tengah |
| Terendah | 40%, di tengah |

- `Alt`+`Atas` memperbesar jendela. Di level tertinggi, jendela kembali ke level terendah.
- `Alt`+`Bawah` memperkecil jendela. Di level terendah, jendela tetap di situ.
- Persentasenya tidak menghitung taskbar, jadi jendela tidak pernah tertutup olehnya.

### Lebar jendela: Alt+Kanan dan Alt+Kiri

- `Alt`+`Kanan` melebarkan jendela aktif 40 piksel, sampai tepi layar.
- `Alt`+`Kiri` menyempitkannya 40 piksel, sampai minimal 300 piksel.
- Pada jendela full screen, `Alt`+`Kiri` keluar dulu dari full screen, sedangkan `Alt`+`Kanan` tidak melakukan apa-apa.

> **Catatan:** di browser dan File Explorer, `Alt`+`Kiri` dan `Alt`+`Kanan` biasanya berarti Back dan Forward, dan di File Explorer `Alt`+`Atas` berarti naik satu folder. TouchTamer mengambil alih tombol ini selama berjalan. Kalau ingin fungsi aslinya kembali, [matikan hotkey-nya](#mengubah-pengaturan).

## Mengubah pengaturan

1. Tekan `Win`+`R`, tempel baris di bawah, lalu tekan `Enter`:
   ```
   %LOCALAPPDATA%\Programs\TouchTamer
   ```
2. Buka `TouchTamer.ini` dengan Notepad.
3. Ubah nilainya, lalu simpan file.
4. [Jalankan ulang TouchTamer](#menghentikan-menjalankan-ulang-atau-mematikan-saat-startup).

| Pengaturan | Bawaan | Fungsi |
|---|---|---|
| `level_gesture` | `1` | Gestur brightness dan volume. `0` untuk mematikan |
| `gesture_mod` | `ctrl+shift` | Tombol yang ditahan saat gestur itu. Pakai `ctrl`, `alt`, `shift`, atau gabungannya |
| `full_height_pct` | `60` | Perubahan persen untuk geseran setinggi touchpad |
| `move_gesture` | `1` | Memindahkan jendela dengan 5 jari. `0` untuk mematikan |
| `move_speed_pct` | `150` | Kecepatan jendela. Makin besar makin cepat |
| `size_hotkeys` | `1` | `Alt`+`Atas` dan `Alt`+`Bawah`. `0` untuk mematikan |
| `width_hotkeys` | `1` | `Alt`+`Kanan` dan `Alt`+`Kiri`. `0` mengembalikan Back dan Forward ke browser |
| `hide_after_ms` | `850` | Lama panel terlihat, dalam milidetik |
| `alpha` | `245` | Kepekatan panel, dari 40 sampai 255 |
| `volume_native_osd` | `0` | `1` menampilkan overlay volume Windows, bukan panel |
| `debug` | `0` | `1` menulis log diagnosa, `TouchTamer.log`, di samping program |

Memasang versi yang lebih baru tidak menimpa `TouchTamer.ini` Anda.

## Menghentikan, menjalankan ulang, atau mematikan saat startup

**Menghentikan sementara:** buka **Task Manager** (`Ctrl`+`Shift`+`Esc`), cari **TouchTamer touchpad gestures**, lalu klik **End task**.

**Menjalankan lagi:** buka folder pemasangan seperti di atas, lalu klik dua kali `TouchTamer.exe`.

**Menjalankan ulang:** lakukan keduanya. TouchTamer membaca pengaturan saat mulai berjalan.

**Agar tidak ikut jalan saat Windows mulai:** buka **Task Manager → Startup apps**, pilih **TouchTamer**, lalu klik **Disable**.

## Cara menghapus

Klik dua kali **`uninstall.cmd`** di folder hasil ekstrak. Kalau foldernya sudah tidak ada, [unduh lagi](#1-unduh).

Uninstaller menghentikan TouchTamer, menghapus shortcut startup, dan menghapus file program. Brightness dan volume tetap di nilai terakhir.

### Menghapus secara manual

1. Hentikan TouchTamer lewat Task Manager.
2. Tekan `Win`+`R`, ketik `shell:startup`, lalu tekan `Enter`. Hapus shortcut `TouchTamer`.
3. Tekan `Win`+`R`, ketik `%LOCALAPPDATA%\Programs`, lalu tekan `Enter`. Hapus folder `TouchTamer`.

## Kalau ada masalah

| Masalah | Coba ini |
|---|---|
| Tidak terjadi apa-apa sama sekali | Cek apakah **TouchTamer touchpad gestures** ada di Task Manager. Kalau tidak ada, jalankan dari folder pemasangan |
| Gestur tidak jalan, tetapi `Alt`+panah jalan | [Cek touchpad Anda](#mengecek-touchpad). TouchTamer butuh Precision Touchpad |
| Brightness tidak berubah | Brightness hanya jalan di layar bawaan laptop. Volume seharusnya tetap jalan |
| Volume atau brightness berubah dua kali lebih cepat | Mungkin ada salinan lain yang berjalan, atau ada alat lain yang ikut bereaksi. Tutup lewat Task Manager |
| Halaman ikut zoom atau bergulir saat gestur | Windows tetap mengirim scroll ke sebagian aplikasi. Coba `gesture_mod` lain, misalnya `ctrl+alt` |
| Lima jari tidak bereaksi | Touchpad Anda mungkin membaca kurang dari 5 jari, dan installer memperingatkan hal ini di Windows 11 24H2 atau lebih baru. Tempelkan kelima jari dulu sebelum menggeser |
| Jendela tidak bisa dipindah atau diubah ukurannya | Windows memblokir ini untuk jendela yang berjalan sebagai administrator, seperti Task Manager |
| `Alt`+panah tidak jalan | Mungkin ada program lain yang memakai tombol itu. Atur `debug=1`, jalankan ulang, lalu cari `hotkey ... not available` di `TouchTamer.log` |
| Ingin Back dan Forward di browser | Atur `width_hotkeys=0`, lalu jalankan ulang |
| Pengaturan tidak berefek | Simpan file, lalu jalankan ulang TouchTamer. Pengaturan hanya dibaca saat mulai berjalan |
| Installer bilang ada file yang hilang | Ekstrak seluruh ZIP dulu, lalu jalankan `install.cmd` dari folder hasil ekstrak |
| Diblokir Windows atau antivirus | Program ini tidak punya tanda tangan berbayar. [Cek file-nya](#cek-file-sendiri), lalu izinkan kalau Anda percaya |

Masih bermasalah? [Buat issue](https://github.com/falahdrrhmn1/TouchTamer/issues) dengan menyebut model laptop, versi Windows, dan yang sudah dicoba. `TouchTamer.log` juga membantu. Baca dulu sebelum dibagikan, karena bisa memuat nama jenis jendela aplikasi.

## Batasan yang sudah diketahui

- Baru diuji di satu laptop, jadi touchpad lain bisa berperilaku berbeda.
- Panel selalu muncul di layar utama.
- Jendela yang berjalan sebagai administrator tidak bisa dipindah atau diubah ukurannya. TouchTamer sendiri tidak perlu dijalankan sebagai administrator untuk hal lain.
- Gestur tidak jalan di layar masuk Windows dan layar izin (UAC).
- Telapak tangan yang menempel di touchpad bisa terhitung sebagai jari.

## Privasi

- TouchTamer tidak punya kode jaringan dan tidak mengirim apa pun ke mana pun.
- Program ini tidak merekam ketikan. Program hanya mengecek apakah `Ctrl`, `Alt`, dan `Shift` ditahan saat gestur, dan mendaftarkan tombol `Alt`+panah.
- Posisi jari dipakai untuk gestur, lalu dibuang.
- File log mati secara bawaan. Kalau dinyalakan, log tetap di komputer Anda dan berisi gestur, nilai, dan jenis jendela aplikasi, bukan ketikan atau nama dokumen.

## Membangun dari kode sumber

Anda butuh compiler MinGW-w64 64-bit dengan `g++` dan `windres` di `PATH`, misalnya [w64devkit](https://github.com/skeeto/w64devkit). Lalu jalankan ini di folder repo:

```bat
build.cmd
```

Hasilnya `bin\TouchTamer.exe`. Untuk ikut membangun uji:

```bat
build.cmd test
build\test_gesture.exe
python tests\e2e_windows.py
```

Uji end-to-end membuka jendela kosongnya sendiri dan menggerakkan kursor selama sekitar satu menit, jadi jangan sentuh keyboard dan touchpad selama uji berjalan. Uji ini tidak pernah menyentuh jendela Anda yang lain.

| Lokasi | Isi |
|---|---|
| `src/touchtamer.cpp` | Program utama |
| `src/gesture.h` | Logika gestur, dipisah dari Windows supaya bisa diuji |
| `tests/` | Uji unit dan uji end-to-end |
| `scripts/` | Installer dan uninstaller |
| `bin/` | Program siap pakai dan pengaturan bawaannya |

## Lisensi

[MIT](LICENSE). Anda bebas memakai, mengubah, dan membagikannya. TouchTamer diberikan tanpa jaminan apa pun.
