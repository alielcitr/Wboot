# Wboot Manager v3.3 🚀

**Wboot**, GRUB 2.14 tabanlı, dinamik ISO/imaj tarama yeteneğine sahip gelişmiş bir çoklu başlatma (Multi-Boot) yönetim sistemidir. Özel olarak C dilinde geliştirilmiş `isoscan` GRUB modülü sayesinde disklerdeki `.iso`, `.img`, `.vhd`, `.vhdx` ve `.wim` imajlarını otomatik olarak tespit eder ve herhangi bir ek yapılandırmaya gerek kalmadan önyükleme menüsüne ekler[cite: 1, 3].

---

## 📸 Öne Çıkan Özellikler

* 🔍 **Otomatik & Dinamik İmaj Taraması:** Disk bölümlerindeki ISO, VHD, WIM ve IMG dosyalarını otomatik algılar.
* 🐧 **Geniş Linux Dağıtım Desteği:** `casper`, `live` ve standart Linux boot yapılandırmalarını otomatik algılar (`vmlinuz` & `initrd` eşleştirmesi).
* 🪟 **Windows PE & WIM Boot:** `wimboot` entegrasyonu ile `boot.wim` imajlarını doğrudan başlatabilir.
* 💾 **Zengin Dosya Sistemi Desteği:** FAT, NTFS, exFAT, BTRFS, XFS, UDF ve ISO9660 dosya sistemlerini destekler[cite: 1].
* 🛡️ **Güvenli Manuel Giriş:** Özel dizin tırmanma (Directory Traversal - `..`) koruması ile manuel ISO yolu tanımlama[cite: 1].
* ⚡ **Yüksek Performanslı C Modülü:** Taramaları hızlı tamamlamak için derinlik sınırı (`ISOSCAN_MAX_DEPTH`) ve gereksiz dizin atlama (Windows, Node Modules, Temp vb.) algoritmaları.

---

## 🛠️ Proje Bileşenleri

| Dosya | Tür | Açıklama |
| :--- | :--- | :--- |
| **`isoscan.c`** | C Kaynak Kodu | GRUB 2.14 için yazılmış özel tarama modülü. |
| **`isoscan.mod`** | GRUB Modülü | `isoscan.c` dosyasının derlenmiş GRUB modülü hali. |
| **`grub.cfg`** | Konfigürasyon | Wboot ana menü yapısı, imaj mount mekanizmaları ve önyükleme mantığı[cite: 1]. |

---

## ⚙️ Derleme (Build) Notları

> ⚠️ **Önemli Derleme Notu:** `isoscan.c` kaynak kodu **GRUB 2.14** (ve üzeri) kaynak ağacı ve API başlık dosyaları (headers) ile uyumlu olacak şekilde geliştirilmiştir[cite: 3].

`isoscan.mod` modülünü GRUB 2.14 kaynak kodları ile derlemek için:

1. `isoscan.c` dosyasını GRUB 2.14 kaynak kod dizinindeki `grub-core/commands/` altına kopyalayın[cite: 3].
2. `grub-core/Makefile.core.def` dosyasına `isoscan` modül tanımını ekleyin.
3. Standard GRUB derleme adımlarını çalıştırarak modülü oluşturun:

```sh
cd /grub-2.14
mkdir build-efi
cd build-efi
../configure   --with-platform=efi   --target=x86_64   --disable-werror
make -j$(nproc)

./grub-mkimage \
  -d ./grub-core \
  -o Wboot.efi \
  -p /EFI/Wboot \
  -O x86_64-efi \
  boot linux multiboot multiboot2 \
  normal configfile search loopback \
  part_gpt part_msdos fat ext2 ntfs iso9660 exfat \
  isoscan terminal gfxterm gfxterm_background gfxmenu \
  all_video video video_fb efi_gop efi_uga \
  font png jpeg gettext \
  echo cat ls reboot halt test \
  extcmd minicmd help loadenv regexp \
  blocklist cmp hexdump memrw \
  datetime date time \
  eval read set \
  search_fs_file search_fs_uuid search_label \
  true false sleep disk mdraid mdraid09 mdraid1x \
  lvm raid dm_nv cryptodisk luks gcry_rijndael \
  gcry_sha256 gcry_sha512 password_pbkdf2 \
  progress keylayouts at_keyboard usb_keyboard \
  usb usbserial_common usbserial_pl2303 usbserial_ftdi \
  ehci ohci uhci \
  memdisk tar cpio \
  isoscan \
  gui_mouse 
