# Dokumentasi Riset & Panduan Optimasi MT6765

Dokumen ini memuat rangkuman temuan teknis dari repositori pengembang kernel MediaTek MT6765 (Helio P35 / G35), seperti `realme-mediatek-dev/android_kernel_realme_mt6765`, `techyminati/RMX2185`, serta patch dari kernel engineer (Sultan Alsawaf, LibXZR).

---

## 1. Arsitektur Wi-Fi & Bluetooth (In-Kernel vs Loadable Module)

* **Permasalahan Default:** Driver konektivitas MediaTek (`wlan_6765_axi.ko`, `bt_drv.ko`, `gps`, `fmradio`) di Android standar dimuat sebagai modul dari `/vendor`. Pada Android 10+ (dynamic partitions/SAR), `/vendor` berstatus read-only sehingga pergantian kernel sering menimbulkan masalah Wi-Fi/BT tidak menyala.
* **Solusi Saat Ini:** Injeksi systemless otomatis melalui AnyKernel3 (`do.systemless=1`) ke `/data/adb/modules/ak3-helper` dan modul Magisk backup terpisah (`WiFi-BT-Fix-Magisk.zip`).
* **Solusi Jangka Panjang:** Mengadopsi patch in-kernel connectivity MTK dari `techyminati` dengan menentukan parameter hardware:
  * `MTK_COMBO_CHIP`
  * `WLAN_CHIP_ID=6765`
  * `WIFI_IP_SET`
  * `WIFI_ECO_VER`
  Hal ini memungkinkan Wi-Fi & BT di-compile langsung menyatu ke `vmlinux` (`=y`) tanpa memerlukan file `.ko` eksternal.

---

## 2. Pembersihan Logspam Ekstrem (Performa & Penghematan Baterai Nyata)

Driver MediaTek bawaan terkenal mencetak ribuan baris log per menit yang membebani CPU:
* **TEEI (Trustonic TEE):** Mematikan logspam TEEI terbukti memotong waktu boot hingga **~2 detik**.
* **MTK WLAN Gen4m & Video:** Membatasi logging level ke error saja mencegah CPU throttling di latar belakang.
* **Binder Debug Mask:** Menyetel `binder_debug_mask=0` menghilangkan latensi IPC Android, mencegah frame drop saat animasi dan audio stutter.

---

## 3. Akselerasi Manajemen Memori (20x Lebih Cepat pada `mremap`)

* **Patch `HAVE_MOVE_PMD` (`mm: speed up mremap by 20x on large regions`):**
  Android ART runtime dan LMK secara intensif memanggil `mremap()`. Linux 4.9 bawaan memindahkan entry page table per PTE. Dengan `HAVE_MOVE_PMD`, kernel memindahkan mapping di level PMD. Waktu alokasi memori besar (1 GB) turun dari **~3.5 ms menjadi ~0.15 ms (~20x lebih cepat)**.

---

## 4. Eliminasi UI Micro-Stutter / Jank

* **Asynchronous Dead Task & MM Freeing (`finish_task_switch`):**
  Menghindari pembebasan task memori yang mati langsung pada CPU performa saat context switch. Pembebasan didelegasikan ke *unbound workqueue* di core hemat daya (LITTLE cluster), menjaga rendering frame 60fps tetap mulus.
* **Prioritas Devfreq Monitor:**
  Menyetel antrean kerja `devfreq` (pengatur frekuensi GPU dan bus RAM) ke `WQ_HIGHPRI` agar clock grafis dan bus memori merespons sentuhan seketika tanpa delay.

---

## 5. Efisiensi Penguncian & Daya (20% Mutex Throughput)

* **Optimized Spinning Loop di ARM64 (`locking/osq`):**
  Menggunakan instruksi `wfe` (*Wait For Event*) saat terjadi perebutan lock (*lock contention*), alih-alih loop berputar (*busy-waiting*) yang membakar baterai dan menghasilkan panas. Throughput mutex meningkat hingga **~20%**.
* **Power Efficient Workqueues & Cpuset Background:**
  Memindahkan task vmstat dan timer berkala ke workqueue hemat daya, serta memaksa `nice=10` pada grup `cpuset: background` agar aplikasi latar belakang tidak mencuri resource CPU dari aplikasi yang sedang dibuka pengguna.

---

## 6. Referensi Pohon Device & Vendor ROM (Untuk Build Custom ROM)
* **Device Tree:** [techyminati/android_device_realme_RMX2185](https://github.com/techyminati/android_device_realme_RMX2185) (simpan di `device/realme/RMX2185`)
* **Vendor Tree:** [techyminati/proprietary_vendor_realme_RMX2185](https://github.com/techyminati/proprietary_vendor_realme_RMX2185) (simpan di `vendor/realme/RMX2185`)

---

## 7. Eksplorasi Tingkat Lanjut (Next-Tier Enhancements & Roadmap)

Peningkatan level mendalam melampaui konfigurasi bawaan untuk performa silikon maksimal:

### 1. Performa UI & Respons Sentuhan Layar (Touch Latency & CPU Boost)
* **Dynamic CPU Input Boost:**
  * Menambahkan pemicu event sentuhan touchscreen (`touch_event`). Saat jari menyentuh layar, frekuensi CPU cluster besar (Cortex-A53 2.3 GHz) langsung dinaikkan ke clock boost selama ~80 milidetik. Menghilangkan jeda respon awal saat berinteraksi dengan aplikasi dan game.
* **Tuning Schedutil / Schedplus Rate Limits:**
  * `up_rate_limit_us = 500`: Frekuensi melompat ke level tertinggi hanya dalam 0.5 milidetik saat beban terdeteksi.
  * `down_rate_limit_us = 20000`: Menahan frekuensi selama 20 milidetik sebelum turun, mencegah frame drop saat scrolling panjang di aplikasi media sosial.
* **Affinity Capacity Margins (WALT/EAS):**
  * Memastikan thread rendering UI utama Android (`RenderThread` dan `SurfaceFlinger`) diprioritaskan berjalan di core 2.3 GHz, bukan tersasar ke core hemat 1.8 GHz.
* **ZRAM Multi-Stream Compression:**
  * Menyetel `max_comp_streams` ke 8 stream (sesuai 8 core CPU) agar seluruh core dapat memampatkan RAM secara simultan tanpa lock contention.

### 2. Storage & I/O Loading Speed (Melampaui Deadline Scheduler)
* **Zen I/O Scheduler atau Maple I/O Scheduler:**
  * Mengintegrasikan scheduler **Zen** atau **Maple** yang dirancang khusus untuk memori flash smartphone eMMC 5.1.
  * Memprioritaskan operasi *read* (buka aplikasi) dan mengelompokkan operasi *write* di latar belakang secara asinkron, memotong jeda waktu tunggu I/O secara drastis saat multitasking berat.
* **Bypass Redundant Software CRC Check pada eMMC:**
  * Chip eMMC modern sudah memiliki mesin *hardware ECC* terintegrasi.
  * Mematikan verifikasi checksum CRC32 software yang redundan pada driver kernel MMC (`CONFIG_MMC_PERF_PROFILING`) mendongkrak sequential read eMMC sebesar **10–15%**.

### 3. Kerapian Sistem & Wi-Fi/BT Built-in (`=y`) Permanen
* **In-Kernel MTK Connectivity:**
  * Meng-hardcode spesifikasi chip MT6765 langsung ke driver `drivers/misc/mediatek/connectivity/`:
    * `MTK_COMBO_CHIP=CONNAC`
    * `WLAN_CHIP_ID=6765`
    * `WIFI_IP_SET=1`
    * `WIFI_ECO_VER=1`
  * Mengubah target kompilasi driver Wi-Fi (`wlan_drv_gen4m`), Bluetooth (`bt_drv`), GPS, dan FM radio menjadi **built-in langsung ke kernel (`=y`)**.
  * **Hasil:** Kernel 100% *self-contained*. File zip kernel dapat di-flash pada ROM apa pun tanpa memerlukan modul vendor atau injeksi Magisk module.

### 4. GPU & Thermal Curve Tuning (PowerVR Rogue GE8320)
* Menghaluskan kurva mitigasi suhu MediaTek dari penurunan drastis seketika pada 42°C menjadi penurunan bertahap (*gradual step-down*), mencegah *sudden FPS drop* saat bermain game.

### 5. Jaringan Tingkat Lanjut
* Mengaktifkan **TCP Fast Open (TFO)** dan **TCP Window Scaling** untuk mempercepat handshake koneksi data internet pada aplikasi dan browser hingga 15–30%.

