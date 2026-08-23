# WBoot

## Linux Kernel-Based Pre-OS Boot, Multiboot, Recovery & Remote Support Platform

WBoot, **UEFI firmware aşamasından başlatılabilen**, Linux kernel'i donanım ve sistem altyapısı olarak kullanan, işletim sistemlerinden bağımsız bir **pre-OS boot, multiboot, recovery, diagnostics ve remote-support platformudur**.

WBoot'un temel yaklaşımı klasik bir bootloader'ın yalnızca işletim sistemi yüklemesi yerine, işletim sistemi başlamadan önce tam teşekküllü bir sistem ortamı sağlamaktır.

Bu ortam;

* Windows
* Linux
* Android
* ISO images
* Virtual disk images
* EFI applications
* Bootable disk images

gibi farklı hedefleri keşfedebilir, yönetebilir ve uygun boot mekanizması üzerinden başlatabilir.

Aynı zamanda işletim sistemi açılmadığında **disk, filesystem, boot configuration ve recovery işlemlerine uzaktan erişim sağlayabilen bağımsız bir teknik müdahale ortamı** olarak çalışabilir.

---

# Core Concept

WBoot'un temel mimarisi:

```text
                         UEFI Firmware
                              │
                              ▼
                     ┌─────────────────┐
                     │   WBoot EFI     │
                     │  EFI/WBoot/*    │
                     └────────┬────────┘
                              │
                              ▼
                       Linux Kernel
                              │
                              ▼
                     WBoot Initramfs
                              │
                              ▼
                    ┌──────────────────┐
                    │      WBoot       │
                    │   Pre-OS Layer   │
                    └────────┬─────────┘
                             │
        ┌────────────────────┼────────────────────┐
        │                    │                    │
        ▼                    ▼                    ▼
     Boot                 Recovery              Remote
   Management            Environment            Support
        │                    │                    │
        ▼                    ▼                    ▼
 Windows / Linux        Disk / Filesystem      SSH / Telnet
 Android / ISO          Boot Repair            VNC / RDP
 VHD / VHDX / IMG       Diagnostics            Relay
 EFI / Images           BusyBox
```

WBoot böylece işletim sistemi ile firmware arasında yalnızca bir yükleyici değil, **tam bir pre-OS execution environment** oluşturur.

---

# Why Linux Kernel?

WBoot, modern donanım desteğini sıfırdan oluşturmak yerine Linux kernel'in yıllar içerisinde geliştirilmiş sürücü ve sistem altyapısından yararlanır.

Linux kernel WBoot'a aşağıdaki temel altyapıları sağlar:

```text
Linux Kernel
│
├── PCI / PCIe
├── USB
├── USB HID
├── Keyboard
├── Mouse
├── Storage
├── NVMe
├── SATA / AHCI
├── Network
├── Ethernet
├── Wi-Fi*
├── Framebuffer
├── DRM / KMS*
├── EFI
├── TPM*
├── TCP/IP
└── Filesystem Support
```

Bu yaklaşım sayesinde WBoot'un amacı;

> her yeni donanım için ayrı bir bootloader sürücüsü geliştirmek değil, Linux kernel'in donanım yeteneklerini kullanarak bunun üzerine güçlü bir boot/recovery katmanı inşa etmektir.

* Donanım, firmware, kernel configuration ve sürücü/firmware desteğine bağlıdır.

---

# Pre-OS Architecture

WBoot normal işletim sisteminden önce çalışan bağımsız bir ortamdır.

Normal boot:

```text
UEFI
 │
 ▼
WBoot
 │
 ▼
Linux Kernel
 │
 ▼
WBoot Environment
 │
 ▼
Selected OS
```

Recovery:

```text
UEFI
 │
 ▼
WBoot
 │
 ▼
Linux Kernel
 │
 ▼
WBoot Recovery
 │
 ├── Disk
 ├── Filesystem
 ├── Boot Configuration
 ├── Kernel / Initramfs
 └── Diagnostics
```

Remote recovery:

```text
Technician
     │
     ▼
Remote Network
     │
     ▼
Relay / NAT
     │
     ▼
WBoot
     │
     ├── Terminal
     ├── Disk
     ├── Filesystem
     ├── Recovery
     └── Boot Manager
```

WBoot'un en önemli özelliklerinden biri, **normal işletim sisteminin çalışıyor olmasına ihtiyaç duymamasıdır**.

---

# UEFI Integration

WBoot, UEFI System Partition üzerinde küçük bir boot alanına yerleştirilebilecek şekilde tasarlanmaktadır.

Örnek:

```text
EFI/
├── Microsoft/
│   └── Boot/
│
├── Boot/
│
└── WBoot/
    ├── wbootx64.efi
    ├── kernel
    ├── initramfs
    └── config/
```

UEFI firmware WBoot'u doğrudan başlatabilir.

Böylece Windows veya Linux'un kendi boot sürecinin çalışmasına gerek kalmadan:

```text
UEFI
 ↓
WBoot
 ↓
Linux Kernel
 ↓
WBoot UI / Recovery / Multiboot
```

zinciri oluşturulabilir.

WBoot bu nedenle işletim sistemi içine kurulmuş sıradan bir recovery uygulamasından farklıdır.

---

# Multiboot

WBoot, yalnızca Windows ve Linux arasında seçim yapmakla sınırlı olmayan genişletilebilir bir boot discovery mimarisi hedeflemektedir.

```text
WBoot
│
├── Windows
│
├── Linux
│   ├── Ubuntu
│   ├── Debian
│   ├── Fedora
│   ├── Arch
│   └── Other Linux
│
├── Android
│
├── ISO Images
│
├── EFI Applications
│
├── Virtual Disk Images
│
└── Other Bootable Images
```

WBoot'un discovery katmanı sistemde bulunan boot edilebilir kaynakları tarayarak bunları tek bir kullanıcı arayüzünde sunmayı hedefler.

---

# Windows Boot & Recovery

WBoot, Windows'un başlamadığı durumlarda Windows kurulumuna bağımsız bir recovery ortamından erişebilmek üzere tasarlanabilir.

Örnek:

```text
UEFI
 │
 ▼
WBoot
 │
 ▼
Linux Kernel
 │
 ▼
Windows Installation Discovery
 │
 ├── EFI
 ├── Windows filesystem
 ├── Boot files
 ├── BCD
 └── Recovery data
```

Bu ortam üzerinden uygun araçlarla:

* Windows boot yapılandırmasının incelenmesi
* EFI dosyalarının kontrolü
* disk/filesystem erişimi
* log analizi
* recovery işlemleri
* boot problemlerinin teşhisi

gerçekleştirilebilir.

WBoot'un Windows'a bağımlı olmaması burada temel avantajdır.

---

# Linux Boot & Recovery

Linux kurulumları da otomatik olarak keşfedilebilir.

```text
Linux Installation
│
├── /boot
│   ├── vmlinuz
│   └── initramfs
│
├── /etc
├── /usr
├── /var
└── filesystem
```

WBoot recovery ortamı Linux filesystem'lerini mount ederek:

* kernel
* initramfs
* boot configuration
* filesystem
* system logs
* configuration files

üzerinde inceleme ve recovery işlemlerine olanak sağlayabilir.

Bu nedenle WBoot, farklı Linux dağıtımları için ortak recovery platformu olarak da kullanılabilecek şekilde tasarlanmaktadır.

---

# Android Boot

Android, WBoot'un multiboot hedeflerinden biri olarak ele alınabilir.

```text
Android
│
├── Boot Image
├── Kernel
├── Initramfs / Ramdisk
├── Android configuration
└── System partitions
```

WBoot'un amacı uygun x86/x86_64 Android kurulumlarını ve desteklenen Android boot image yapılarını keşfedip yönetebilecek bir boot katmanı sağlamaktır.

Android'in donanım ve platforma özel boot zincirleri ayrıca dikkate alınmalıdır.

---

# ISO Boot

WBoot ISO imajlarını da boot discovery sistemine dahil edebilir.

Örnek:

```text
ISO Discovery
│
├── Linux ISO
├── Windows ISO
├── Recovery ISO
├── Diagnostic ISO
└── Custom ISO
```

Bu sayede ayrı bir USB boot ortamı oluşturmadan sistemde bulunan ISO imajlarının uygun yöntemlerle başlatılması hedeflenmektedir.

ISO boot yöntemi imajın yapısına ve hedef işletim sisteminin boot gereksinimlerine bağlıdır.

---

# Virtual Disk Boot

WBoot, sanal disk imajlarını da keşfedilebilir boot kaynakları olarak ele alabilecek şekilde tasarlanmaktadır.

```text
Virtual Disk
│
├── VHD
├── VHDX
├── IMG
└── RAW
```

Genel akış:

```text
Image Discovery
      │
      ▼
Image Inspection
      │
      ▼
Partition / Filesystem Discovery
      │
      ▼
Mount / Access
      │
      ▼
Boot / Recovery
```

Bu yapı özellikle Windows ve Linux sanal disklerinin recovery, inspection ve uygun boot senaryolarında kullanılmasını hedefler.

---

# Disk & Filesystem Management

WBoot'un recovery altyapısının temel bileşenlerinden biri storage yönetimidir.

```text
Storage
│
├── Disk Discovery
├── Partition Discovery
├── Filesystem Detection
├── Mount
├── Unmount
├── Filesystem Inspection
├── Boot Image Discovery
└── Recovery Access
```

Linux kernel'in storage ve filesystem altyapısı bu katmanın temelini oluşturur.

---

# Graphical User Interface

WBoot terminal tabanlı bir recovery sistemi ile sınırlı değildir.

Kendi UI katmanı:

```text
Input Backend
      │
      ▼
Framebuffer / Display
      │
      ▼
Font Renderer
      │
      ▼
Widgets
      │
      ▼
WBoot UI
```

üzerinde çalışır.

UI içerisinde hedeflenen bölümler:

```text
WBoot
│
├── Boot Manager
├── OS Discovery
├── Recovery
├── Disk Manager
├── Terminal
├── Diagnostics
├── Network
├── Remote Support
└── Settings
```

---

# BusyBox Recovery Environment

WBoot recovery kullanıcı alanı BusyBox tabanlı araçlarla küçük ve hızlı tutulabilir.

Örnek:

```text
WBoot Recovery Shell

# ls
# mount
# umount
# dmesg
# ip
# ps
# cat
# cp
# mv
# mkdir
# sh
```

Bu ortam GUI'nin yeterli olmadığı düşük seviyeli sistem müdahaleleri için kullanılabilir.

---

# Boot-Level Remote Support

WBoot'un ayırt edici hedeflerinden biri **işletim sistemi başlamadan önce uzaktan teknik müdahale** sağlayabilmesidir.

Bu yaklaşım:

```text
Normal OS Remote Support
```

modelinden farklıdır.

Normal durumda:

```text
Machine
 │
 ▼
Operating System
 │
 ▼
SSH / RDP / VNC
```

WBoot yaklaşımında:

```text
Machine
 │
 ▼
UEFI
 │
 ▼
WBoot
 │
 ▼
Linux Kernel
 │
 ▼
Remote Support
```

olabilir.

Böylece normal işletim sistemi:

* boot etmiyor olabilir
* filesystem problemi yaşayabilir
* kernel problemi yaşayabilir
* network servisleri başlamıyor olabilir
* kullanıcı alanı bozulmuş olabilir

ancak WBoot çalışıyorsa sistem uzaktan erişilebilir bir recovery ortamına sahip olabilir.

---

# Remote Connectivity

WBoot geliştirme kapsamında remote access senaryoları test edilmektedir.

Hedeflenen/uygulanan uzak erişim mekanizmaları:

```text
                    WBoot
                      │
          ┌───────────┼───────────┐
          │           │           │
          ▼           ▼           ▼
         SSH        Telnet       VNC
          │                       │
          └──────────┬────────────┘
                     ▼
                    RDP
```

NAT arkasındaki sistemler için relay/outbound bağlantı mimarisi de kullanılabilir.

Örnek:

```text
Remote Technician
        │
        ▼
      Relay
        │
        ▼
      Internet
        │
        ▼
       NAT
        │
        ▼
      WBoot
        │
        ├── Shell
        ├── Recovery
        ├── Disk
        ├── Boot Manager
        └── Diagnostics
```

Bu mimari, fiziksel olarak sisteme erişilemeyen veya işletim sistemi açılmayan cihazlarda teknik destek senaryoları için özellikle değerlidir.

### Security

Remote support production ortamında aşağıdaki güvenlik katmanları uygulanmalıdır:

* Strong authentication
* Public-key authentication
* TLS/encrypted transport
* Authorization
* Session isolation
* Access control
* Connection timeout
* Audit logging
* Secure relay authentication
* Optional one-time recovery credentials

WBoot'un recovery erişimi, normal işletim sistemi erişiminden daha yüksek yetkilere sahip olabileceğinden güvenlik kritik bir bileşendir.

---

# Development & QEMU

QEMU, WBoot'un **Linux kernel geliştirme, boot flow testleri ve hata ayıklama ortamı** olarak kullanılmaktadır.

Örnek:

```text
QEMU
 │
 ├── OVMF / UEFI
 ├── q35
 ├── KVM
 ├── Linux Kernel
 ├── WBoot Initramfs
 ├── Virtual Disk
 ├── USB Keyboard
 ├── USB Tablet
 ├── Display
 ├── Serial Console
 └── Virtual Network
```

Örneğin:

```bash
qemu-system-x86_64 \
  -machine q35,accel=kvm,i8042=off \
  -cpu host \
  -m 4096 \
  -smp 4 \
  -drive if=pflash,format=raw,readonly=on,file=OVMF_CODE_4M.fd \
  -drive if=pflash,format=raw,file=OVMF_VARS_local.fd \
  -drive if=ide,format=raw,file=disk.img,index=0,snapshot=on \
  -kernel linux/arch/x86/boot/bzImage \
  -initrd build/wboot-initramfs.cpio.gz \
  -append 'console=ttyS0 console=tty1 boot.debug=1 boot.timeout=3600 boot.remote=1 boot.relay=1' \
  -netdev user,id=n0 \
  -device e1000,netdev=n0 \
  -usb \
  -device usb-kbd \
  -device usb-tablet \
  -display gtk \
  -vga std \
  -serial stdio \
  -no-reboot
```

QEMU burada WBoot'un nihai hedef donanımı değildir.

QEMU:

> **Linux kernel + WBoot geliştirme ve debugging laboratuvarıdır.**

Nihai hedef gerçek UEFI sistemlerinde çalışabilen WBoot ortamıdır.

---

# Development Philosophy

WBoot'un temel tasarım felsefesi:

> **Use the Linux kernel where Linux is already strong, and build the boot, recovery and management environment on top of it.**

Başka bir ifadeyle:

```text
Linux Kernel
      │
      ├── Hardware
      ├── Drivers
      ├── Storage
      ├── Networking
      ├── Filesystems
      └── Input / Display
             │
             ▼
           WBoot
             │
      ┌──────┼───────┐
      ▼      ▼       ▼
     Boot  Recovery Remote
```

WBoot, Linux kernel'i yeniden uygulamak yerine onun mevcut altyapısını kullanarak **boot orchestration ve recovery intelligence** katmanını geliştirmeyi amaçlar.

---

# Target Architecture

Uzun vadeli hedef mimari:

```text
                              UEFI
                                │
                                ▼
                         ┌─────────────┐
                         │    WBoot    │
                         │ EFI Loader  │
                         └──────┬──────┘
                                │
                                ▼
                         Linux Kernel
                                │
                                ▼
                         WBoot Runtime
                                │
              ┌─────────────────┼─────────────────┐
              │                 │                 │
              ▼                 ▼                 ▼
          Boot Manager       Recovery        Remote Support
              │                 │                 │
       ┌──────┼───────┐         │          ┌──────┼──────┐
       │      │       │         │          │      │      │
       ▼      ▼       ▼         ▼          ▼      ▼      ▼
    Windows Linux Android    BusyBox     SSH   VNC    RDP
       │      │       │         │
       └──────┼───────┘         │
              │                 │
       ┌──────┴────────┐        │
       ▼               ▼        ▼
      ISO          VHD/VHDX   Filesystems
       │               │        │
       └───────────────┴────────┘
                       │
                       ▼
                    Recovery
```

---

# Why WBoot?

Modern systems increasingly require more than a minimal bootloader.

WBoot approaches the problem differently:

```text
Traditional Bootloader

UEFI
 ↓
Bootloader
 ↓
OS
```

versus:

```text
WBoot

UEFI
 ↓
Linux Kernel
 ↓
WBoot Pre-OS Environment
 ├── Boot
 ├── Multiboot
 ├── Recovery
 ├── Diagnostics
 ├── Disk Management
 ├── BusyBox
 └── Remote Support
 ↓
Operating System / Image
```

Bu yaklaşım WBoot'u klasik bootloader ile işletim sistemi arasında yeni bir **pre-OS infrastructure layer** olarak konumlandırmayı amaçlamaktadır.

---

# Planned Ecosystem Integration

WBoot yalnızca bağımsız bir proje olarak değil, mevcut Linux sistemlerine entegre edilebilecek bir recovery platformu olarak da tasarlanabilir.

Örneğin:

```text
Ubuntu
 ├── Normal Linux
 └── WBoot Recovery

Debian
 ├── Normal Linux
 └── WBoot Recovery

Fedora
 ├── Normal Linux
 └── WBoot Recovery

Windows
 ├── Normal Windows
 └── WBoot Recovery
```

Bu modelde WBoot, her işletim sisteminin kendi kernel/disk altyapısıyla uyumlu bir recovery katmanı olarak kullanılabilir.

---

# Current Development Status

WBoot aktif geliştirme aşamasındadır.

Mevcut geliştirme kapsamında:

* [x] Linux kernel based runtime
* [x] UEFI/QEMU development environment
* [x] Custom graphical UI
* [x] Framebuffer support
* [x] Keyboard / mouse input
* [x] Font rendering
* [x] Widget system
* [x] Terminal environment
* [x] BusyBox based recovery userspace
* [x] Disk discovery
* [x] Filesystem mounting
* [x] Bootable image discovery
* [x] Recovery environment
* [x] Network support
* [x] Remote access infrastructure
* [x] Remote connection testing
* [x] Relay-based remote access testing
* [ ] Extended Windows recovery integration
* [ ] Extended Linux distribution integration
* [ ] Android boot integration
* [ ] Extended ISO boot support
* [ ] Extended VHD/VHDX boot support
* [ ] Secure Boot integration
* [ ] TPM integration
* [ ] Production-grade remote authentication
* [ ] Hardware validation across multiple physical platforms

---

# Project Goals

WBoot'un uzun vadeli hedefleri:

1. **UEFI-native boot environment**
2. **Linux kernel-based hardware abstraction**
3. **Cross-platform multiboot**
4. **Windows recovery**
5. **Linux recovery**
6. **Android boot/recovery**
7. **ISO boot**
8. **Virtual disk boot**
9. **Filesystem and disk management**
10. **Built-in diagnostics**
11. **BusyBox recovery shell**
12. **Graphical recovery UI**
13. **Boot-level remote support**
14. **NAT/relay remote access**
15. **Linux distribution integration**
16. **Secure recovery infrastructure**

---

# Project Vision

WBoot'un nihai vizyonu:

> **An independent Linux-kernel-based pre-OS environment that can boot, inspect, recover, diagnose and remotely manage modern operating systems and bootable images before the target operating system starts.**

WBoot'un amacı işletim sisteminin yerini almak değil;

**işletim sistemi başlamadan önce sistemin kontrolünü, kurtarılmasını ve yönetilmesini mümkün kılan güvenilir bir altyapı oluşturmaktır.**

---

# Project Status

**Development / Experimental**

WBoot aktif geliştirme ve test aşamasındadır.

Donanım desteği, filesystem davranışı, boot yöntemleri, Android/ISO/virtual-disk uyumluluğu ve remote-access özellikleri kullanılan kernel configuration, firmware, hardware, image formatı ve hedef sistemin boot mimarisine bağlı olarak değişebilir.

---

## WBoot

**UEFI • Linux Kernel • Boot • Multiboot • Recovery • Diagnostics • Remote Support**

**Boot before the OS. Recover before the OS. Manage before the OS.**
