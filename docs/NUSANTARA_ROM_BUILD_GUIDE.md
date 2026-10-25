# Panduan Lengkap Build ROM Nusantara Project (Realme C15 - MT6765)
### Terintegrasi Endfield Kernel, KernelSU-Next, dan Kali NetHunter

Dokumen ini berisi panduan komprehensif, arsitektur device tree, konfigurasi local manifest, serta langkah-langkah optimasi untuk mengompilasi **Nusantara Project ROM** pada perangkat **Realme C15 / C12 (`RMX2185` / `RMX2180`, Helio G35 MT6765)**.

---

## 1. Arsitektur Pohon Sumber Daya (Trees Architecture)

Untuk menghasilkan build ROM yang stabil dan selaras dengan kernel Endfield, kita menghubungkan tree berikut:

| Komponen | Repositori / Sumber | Jalur di Source ROM |
| :--- | :--- | :--- |
| **ROM Source** | Nusantara Project (Android 10 `10.0` / Android 11 `11.0`) | Direktori Root ROM |
| **Device Tree** | `techyminati/device_realme_RMX2185` (atau `Frostleaft07`) | `device/realme/RMX2185` |
| **Vendor Tree** | `Frostleaft07/vendor_realme_RMX2185` (atau `techyminati`) | `vendor/realme/RMX2185` |
| **Kernel Tree** | `qrevebnation/android_kernel_realme_mt6765` (Branch `wip`) | `kernel/realme/mt6765` |

---

## 2. Konfigurasi Local Manifest (`local_manifests`)

Buat berkas manifest di direktori source ROM: `.repo/local_manifests/realme_mt6765.xml`:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<manifest>
    <!-- Device Tree -->
    <project name="Frostleaft07/device_realme_RMX2185" path="device/realme/RMX2185" remote="github" revision="ten" />

    <!-- Vendor Tree -->
    <project name="Frostleaft07/vendor_realme_RMX2185" path="vendor/realme/RMX2185" remote="github" revision="ten" />

    <!-- Kernel Tree (Endfield Kernel with KernelSU-Next & NetHunter) -->
    <project name="qrevebnation/android_kernel_realme_mt6765" path="kernel/realme/mt6765" remote="github" revision="wip" />
</manifest>
```

> [!NOTE]
> Sesuaikan nilai `revision` (misal `ten` untuk Android 10 atau `eleven` untuk Android 11) sesuai branch yang tersedia di repositori masing-masing tree.

---

## 3. Peningkatan & Optimasi Penting (Improvements)

Berikut adalah optimasi yang bisa Anda terapkan di pohon ROM (`device/realme/RMX2185`):

### A. Kompilasi Modul Wi-Fi & Bluetooth Secara Native
Pada file `BoardConfig.mk` di device tree, pastikan modul Wi-Fi dan Bluetooth dikompilasi langsung menyatu ke `/vendor/lib/modules/`:

```makefile
# Native MTK Connectivity Modules
TARGET_KERNEL_MODULES := \
    wmt_drv.ko \
    wmt_chrdev_wifi.ko \
    wlan_drv_gen4m.ko \
    bt_drv.ko \
    gps_drv.ko \
    fmradio_drv.ko

# Symbolic link alias untuk inisialisasi vendor MT6765
BOARD_VENDOR_KERNEL_MODULES += $(TARGET_KERNEL_MODULES)
```
*Manfaat*: Tidak memerlukan trik modul overlay atau bind-mount lagi di AnyKernel3. Wi-Fi dan Bluetooth akan langsung aktif secara alami saat pertama kali boot.

---

### B. Penggunaan Sistem File Modern (EROFS & F2FS)
Realme C15 menggunakan storage eMMC 5.1. Dengan mengaktifkan EROFS pada partisi sistem/vendor dan F2FS pada data, performa I/O akan meningkat drastis:

Di `BoardConfig.mk`:
```makefile
# Sistem File Cepat & Hemat Ruang
BOARD_SYSTEMIMAGE_FILE_SYSTEM_TYPE := erofs
BOARD_VENDORIMAGE_FILE_SYSTEM_TYPE := erofs
BOARD_PRODUCTIMAGE_FILE_SYSTEM_TYPE := erofs
BOARD_USERDATAIMAGE_FILE_SYSTEM_TYPE := f2fs

# Kompresi EROFS
BOARD_EROFS_COMPRESSOR := lz4
```

---

### C. Transisi SELinux ke Mode `Enforcing` (Lolos Play Integrity)
Di `BoardConfig.mk`:
```makefile
# Setel SELinux ke Enforcing untuk lolos verifikasi Play Integrity / Perbankan / Gojek
BOARD_KERNEL_CMDLINE += androidboot.selinux=enforcing
```
*Manfaat*: Status `Enforcing` membuat perangkat Anda memenuhi standar `MEETS_DEVICE_INTEGRITY` secara alami.

---

### D. Out-of-the-Box Kali NetHunter & KernelSU-Next Prebuilt
Di file `device.mk`, Anda bisa menyertakan aplikasi KernelSU-Next Manager langsung sebagai aplikasi sistem:

```makefile
# Prebuilt KernelSU-Next Manager APK
PRODUCT_PACKAGES += \
    KernelSU-Next

# Aturan Chroot NetHunter di init.rc
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/rootdir/etc/init.nethunter.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/hw/init.nethunter.rc
```

Isi dari `init.nethunter.rc`:
```init
on post-fs-data
    # Menyiapkan direktori chroot Kali NetHunter
    mkdir /data/local/nhsystem 0755 root root
    mkdir /data/local/nhsystem/kalifs 0755 root root

on boot
    # Perizinan USB Gadget untuk HID Attack / BadUSB
    chmod 0660 /config/usb_gadget/g1/functions/hid.usb0
    chown system system /config/usb_gadget/g1/functions/hid.usb0
```

---

## 4. Langkah-Langkah Build ROM Nusantara

### Langkah 1: Persiapan Lingkungan Build (Ubuntu/Debian)
```bash
sudo apt update && sudo apt install -y \
    bc bison build-essential ccache curl flex g++-multilib gcc-multilib git \
    git-lfs gnupg gperf imagemagick lib32readline-dev lib32z1-dev libelf-dev \
    liblz4-tool libncurses5 libncurses5-dev libsdl1.2-dev libssl-dev libxml2 \
    libxml2-utils lzop pngcrush rsync schedtool squashfs-tools xsltproc zip zlib1g-dev
```

### Langkah 2: Inisialisasi Repositori Nusantara Project
```bash
mkdir -p ~/nusantara && cd ~/nusantara
repo init -u https://github.com/Nusantara-ROM/android_manifest.git -b 10.0 --depth=1
```

### Langkah 3: Tambahkan Manifest Realme MT6765 & Sinkronisasi
Salin konfigurasi XML pada **Bagian 2** ke:
`~/.repo/local_manifests/realme_mt6765.xml`

Lalu lakukan sinkronisasi source code:
```bash
repo sync -c --no-clone-bundle --no-tags --optimized-fetch --prune --force-sync -j$(nproc --all)
```

### Langkah 4: Menyiapkan Lingkungan Kompilasi & Cache
```bash
source build/envsetup.sh
export USE_CCACHE=1
export CCACHE_EXEC=/usr/bin/ccache
ccache -M 50G
```

### Langkah 5: Memilih Target Perangkat (Lunch) & Mulai Build
```bash
lunch nad_RMX2185-userdebug
mka nad -j$(nproc --all)
```

---

## 5. Hasil Akhir (Build Output)
File zip ROM yang siap di-flash akan berada di direktori:
```
out/target/product/RMX2185/Nusantara_v*.zip
```

### Cara Flash:
1. Masuk ke **TWRP / OrangeFox Recovery**.
2. Lakukan **Wipe Dalvik, Cache, System, Vendor, Data**.
3. Flash file `Nusantara_v*.zip`.
4. (Opsional) Reboot ke System. Semua fitur (Wi-Fi, Bluetooth, Audio, Kamera, KernelSU-Next, dan NetHunter) akan langsung aktif secara bawaan!
