# Panduan Porting & Arsitektur KernelSU Linux 4.9 (Realme MT6765)

Dokumen ini adalah rekam jejak teknis resmi agar pengembang dan AI asisten tidak tersesat saat melakukan *build*, *maintenance*, atau *rebase* kernel di masa mendatang.

---

## 1. Spesifikasi Target Perangkat & Kernel
- **Perangkat**: Realme MT6765 / Helio P35 & G35 (Codenames: `RMX2185`, `RMX2180`, `Nightmare`).
- **Versi Kernel**: Linux 4.9.190 non-GKI (AArch64).
- **Target OS**: Android 10 (Realme UI 1.0) hingga custom ROM / GSI Android 11–14 (mendukung EROFS & F2FS).
- **Versi KernelSU**: KernelSU-Next v30000 / SukiSU-Ultra (Support Dual Manager Hash: Official KernelSU & SukiSU Ultra).

---

## 2. Status Pencapaian Kompilasi
- **Tahap Kompilasi Kernel**: **100% SELESAI & SUKSES** (`Build Kernel` PASSED).
- **Driver KernelSU**: Semua modul (`core`, `hook`, `infra`, `manager`, `policy`, `runtime`, `selinux`, `sulog`, `supercall`) berhasil dikompilasi tanpa error.
- **Dukungan KPM**: Shims `set_memory.h` terpasang.
- **Dukungan Packaging**: AnyKernel3 terkonfigurasi untuk menyerap `Image.gz-dtb` MT6765.

---

## 3. Daftar Kompatibilitas Lengkap (The Linux 4.9 Backport Layer)

Karena KernelSU modern dirancang untuk GKI (Linux 5.10+ / 6.1+), modifikasi berikut telah dipasang secara permanen di repositori:

### A. Header Shims Baru
| Lokasi File | Fungsi & Alasan |
| :--- | :--- |
| `include/linux/compiler_types.h` | Mengarahkan ke `<linux/compiler.h>`, mendefinisikan makro `fallthrough`. |
| `include/linux/minmax.h` | Forward ke makro min/max di `kernel.h`. |
| `include/linux/pgtable.h` | Forward ke `<asm/pgtable.h>`. |
| `include/linux/sched/*.h` | Sched split di Linux 4.11 di-shim kembali ke `<linux/sched.h>` (`task_stack.h`, `task.h`, `signal.h`, `user.h`, `coredump.h`, dll). |
| `include/uapi/linux/mount.h` | Di Linux 4.9 flag mount ada di `<uapi/linux/fs.h>` dan `<linux/mount.h>`. Header shim meneruskan ke keduanya. |
| `arch/arm64/include/asm/set_memory.h` | Di Linux 4.9 ARM64, `set_memory_ro/rw/x` berada di `<asm/cacheflush.h>`. Shim ini meneruskan ke `<asm/cacheflush.h>` untuk syarat KPM SukiSU. |
| `include/linux/set_memory.h` | Meneruskan ke `<asm/set_memory.h>`. |

### B. Compatibility Layer (`drivers/kernelsu/include/compat.h`)
- Diinjeksi otomatis ke seluruh kompilasi KernelSU via flag `-include $(KSU_KERNEL_DIR)/include/compat.h` di `drivers/kernelsu/Kbuild`.
- Menyediakan:
  - `strncpy_from_user_nofault`, `copy_from_user_nofault`, `copy_to_user_nofault`.
  - `ksu_kernel_read_compat` (adaptasi API `kernel_read` 4.9 yang hanya menerima 3 argumen).
  - `typedef unsigned int __poll_t;` dan `#include <uapi/linux/eventpoll.h>` untuk konstanta `EPOLL*`.
  - `selinux_inode(inode)` -> `((struct inode_security_struct *)(inode)->i_security)`.
  - `selinux_cred(cred)` -> `((struct task_security_struct *)(cred)->security)`.
  - `strscpy_pad()` implementation.
  - `fallthrough` macro fallback.

### C. Modul VFS (`drivers/kernelsu/infra/file_wrapper.c`)
- Callback modern VFS (`iopoll`, `fadvise`, `mmap_supported_flags`, `remap_file_range`) dibungkus version guard.
- Di Linux 4.9, fallback ke `clone_file_range` dan `dedupe_file_range`.
- Anonymous inode wrapper menggunakan `anon_inode_getfile()` native Linux 4.9.

### D. LSM Hook Engine (`drivers/kernelsu/hook/lsm_hook.c`)
- Linux 4.9 menggunakan list standar `struct list_head` (dengan `.next` dan `.prev`), bukan `hlist_head` / `hlist_node`.
- Field `.lsm` tidak ada di struktur 4.9 sehingga tidak diakses.

### E. Syscall Invocation (`drivers/kernelsu/include/util.h`)
- Pada ARM64 Linux 4.9, pemanggilan syscall langsung menerima parameter skalar (`sys_close(fd)`, `sys_setns(fd, nstype)`, `sys_unshare(flags)`), bukan pointer `pt_regs *`.
- Dibuat makro `ksyscall_0` hingga `ksyscall_6` agar memanggil fungsi dengan jumlah parameter yang presisi.

### F. Seccomp Subsystem (`drivers/kernelsu/policy/app_profile.c`)
- Field `current->seccomp.filter_count` hanya ada di Linux >= 5.9 (di-guard).
- `seccomp_filter_release` dipetakan ke `put_seccomp_filter(tsk)` native Linux 4.9.

### G. SELinux Engine (`drivers/kernelsu/selinux/sepolicy.c` & `rules.c`)
- Menggunakan database global `policydb` langsung (bukan `struct selinux_policy`).
- Penanganan `struct flex_array` untuk `db->te_avtab.htable`, `type_val_to_struct_array`, dan `sym_val_to_name`.
- Implementasi `add_filename_trans()` native Linux 4.9 menggunakan `struct filename_trans` dan `hashtab_insert(..., 3 params)`.

### H. Alokasi Image & CI Packaging (`.github/workflows/main.yml`)
- Defconfig MT6765 menghasilkan `Image.gz-dtb` (`CONFIG_BUILD_ARM64_APPENDED_KERNEL_IMAGE_NAME="Image.gz-dtb"`).
- CI menyalin `cp ~/out/arch/arm64/boot/Image* $ANYKERNEL/` sehingga AnyKernel3 dapat mengemas kernel zip dengan benar.

---

## 4. Konfigurasi Wajib Defconfig (`endfield_defconfig`)
```kconfig
CONFIG_KSU=y
CONFIG_KPROBES=y
CONFIG_KRETPROBES=y
CONFIG_HAVE_SYSCALL_TRACEPOINTS=y
CONFIG_KALLSYMS=y
CONFIG_KALLSYMS_ALL=y
CONFIG_EXT4_FS=y
CONFIG_SCHED_DEBUG=y # WAJIB: Mencegah kernel panic rodata fault saat boot mtkPowerManager
```

---
*Dokumentasi ini dibuat otomatis sebagai referensi definitif kelangsungan proyek Endfield Kernel Realme MT6765.*
