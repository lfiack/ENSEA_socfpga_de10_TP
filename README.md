# ENSEA_socfpga_de10_TP
TP SoCFPGA sur DE10

## Outils à installer 

yay -S ncurses flex bison openssl dkms libelf systemd-libs pciutils libmpc autoconf bc

yay -S extra/debootstrap extra/qemu-user-static extra/qemu-user-static-binfmt

> Optional dependencies for debootstrap
>    gnupg: check release signatures [installed]
>    debian-archive-keyring: check release signatures for Debian
>    debian-ports-archive-keyring: check release signatures for Debian Ports
>    ubuntu-keyring: check release signatures for Ubuntu

## What do we need for Embedded Linux?

There are a few steps we need to go through to build a working embedded linux OS:

* Device ROM - Every embedded device, including the DE10-Nano, has some instructions that get executed as soon as the device gets powered on. These instructions point the device to read the preloader binary from a specific storage location such as SD Card, eMMC etc. There's nothing for us to do here, it's a ROM.
* Preloader - The preloader is a binary that is provided by the manufacturer which in our case is Intel/Altera. This does some basic setup before it hands over to the Bootloader. In our case, we'll have the Preloader combined with the bootloader.
* Bootloader - The bootloader does some hardware initialization before it hands over to the Kernel to initialize the OS.
* Kernel - The kernel is the heart of the OS and contains all the information about the hardware on the board.
* RootFS - The root filesystem is the location where we work and write programs etc.

Toutes les étapes nécessaires à la création de la carte SD seront fait dans le dossier ```emb``` :

```bash
mkdir emb
cd emb
```

Si vous travaillez avec git (et c'est recommandé) il faudra que l'outil ignore ce dossier, car il sera particulièrement volumineux.

Créez un fichier ```.gitignore``` (au même niveau que emb) et ajoutez-y la ligne suivante :

```bash
emb/
```

```bash
wget https://developer.arm.com/-/media/Files/downloads/gnu/15.2.rel1/binrel/arm-gnu-toolchain-15.2.rel1-x86_64-arm-none-linux-gnueabihf.tar.xz

tar -xvf arm-gnu-toolchain-15.2.rel1-x86_64-arm-none-linux-gnueabihf.tar.xz

rm arm-gnu-toolchain-15.2.rel1-x86_64-arm-none-linux-gnueabihf.tar.xz

export CROSS_COMPILE=$PWD/arm-gnu-toolchain-15.2.rel1-x86_64-arm-none-linux-gnueabihf/bin/arm-none-linux-gnueabihf-
```

## Building the Universal Bootloader (U-Boot)

### Getting the sources

```bash
git clone https://github.com/u-boot/u-boot.git
```

Récupérez la dernière version stable :

```bash
#git tag
git checkout v2026.04
```

### Configuration

#### Configure U-Boot to flash FPGA automatically at boot time

Éditez le fichier ```include/config_distro_bootcmd.h```, cherchez les lignes suivantes:

```C
	BOOT_TARGET_DEVICES(BOOTENV_DEV)                                  \
	\
	"distro_bootcmd=" BOOTENV_SET_SCSI_NEED_INIT                      \
		BOOTENV_SET_NVME_NEED_INIT                                \
		BOOTENV_SET_IDE_NEED_INIT                                 \
		BOOTENV_SET_VIRTIO_NEED_INIT                              \
		BOOTENV_SET_EXTENSION_NEED_INIT                           \
		"for target in ${boot_targets}; do "                      \
			"run bootcmd_${target}; "                         \
		"done\0"
```

Et modifiez ```distro_bootcmd``` de la manière suivante:

```C
	BOOT_TARGET_DEVICES(BOOTENV_DEV)                                  \
	\
	"distro_bootcmd= " \
		"if test -e mmc 0:1 u-boot.scr; then " \
		"echo --- Found u-boot.scr ---; " \
		"fatload mmc 0:1 0x2000000 u-boot.scr; " \
		"source 0x2000000; " \
		"elif test -e mmc 0:1 soc_system.rbf; then " \
		"echo --- Programming FPGA ---; " \
		"fatload mmc 0:1 0x2000000 soc_system.rbf; " \
		"fpga load 0 0x2000000 0x700000; " \
		"else " \
		"echo u-boot.scr and soc_system.rbf not found in fat.; " \
		"fi; " \
```

Explications ici : https://github.com/zangman/de10-nano/blob/master/docs/Building-the-Universal-Bootloader-U-Boot.md#configure-u-boot-to-flash-fpga-automatically-at-boot-time

#### Assign a permanent mac address to the ethernet device

Dans le dossier u-boot :

```bash
make -C tools gen_eth_addr
tools/gen_eth_addr
```

Vous obtiendrez une adresse MAC selon le format suivant :

```bash
22:d6:5c:3d:93:4b
```

Copiez-là et sauvegardez-la quelque part.

Ouvrir le fichier suivant ```include/configs/socfpga_common.h``` et cherchez les lignes suivantes :

```C
#define CFG_EXTRA_ENV_SETTINGS \
	"fdtfile=" CONFIG_DEFAULT_FDT_FILE "\0" \
	"bootm_size=0xa000000\0" \
	"kernel_addr_r="__stringify(CONFIG_SYS_LOAD_ADDR)"\0" \
	"fdt_addr_r=0x02000000\0" \
	"scriptaddr=0x02100000\0" \
	"pxefile_addr_r=0x02200000\0" \
	"ramdisk_addr_r=0x02300000\0" \
	"socfpga_legacy_reset_compat=1\0" \
	BOOTENV
```

Et ajoutez la ligne ```ethaddr```. Remplacez l'adresse MAC par celle que vous avez sauvegardé précédemment.
N'oubliez pas le ```\0``` et le ```\```.

```C
#define CFG_EXTRA_ENV_SETTINGS \
	"fdtfile=" CONFIG_DEFAULT_FDT_FILE "\0" \
	"bootm_size=0xa000000\0" \
	"kernel_addr_r="__stringify(CONFIG_SYS_LOAD_ADDR)"\0" \
	"fdt_addr_r=0x02000000\0" \
	"scriptaddr=0x02100000\0" \
	"pxefile_addr_r=0x02200000\0" \
	"ramdisk_addr_r=0x02300000\0" \
	"socfpga_legacy_reset_compat=1\0" \
	"ethaddr=22:d6:5c:3d:93:4b\0" \
	BOOTENV
```

#### Finish the configuration

Préparer la configuration par défaut :

```bash
make ARCH=arm socfpga_de10_nano_defconfig
```

La configuration par défaut devrait suffire. Vous pouvez aller voir la configuration manuelle avec la commande suivante :

```bash
make ARCH=arm menuconfig
```

### Compilation

```bash
make ARCH=arm -j 8
```

Si la compilation s'est bien déroulée, vous devriez avoir un fichier ```u-boot-with-spl.sfp```.
Il contient le bootloader combiné au secondary program loader (spl).

## Building the Kernel

### Télécharger le Kernel

Dans le dossier ```emb/```:

```bash
git clone https://github.com/altera-opensource/linux-socfpga.git
cd linux-socfpga
#git branch -a
git checkout socfpga-7.0
```

### Configurer le kernel

```bash
make ARCH=arm socfpga_defconfig
```

```bash
make ARCH=arm menuconfig
```

TODO captures d'écran

```bash
make ARCH=arm LOCALVERSION=zImage -j 8
```

## Building Debian Root File System

### Debootstrap First stage

Dans le dossier ```emb/```:

```bash
sudo mkdir rootfs

sudo debootstrap --arch=armhf --foreign bullseye rootfs
```

### Second stage

```bash
sudo cp /usr/bin/qemu-arm-static rootfs/usr/bin/

sudo chroot rootfs /usr/bin/qemu-arm-static /bin/bash -i

/debootstrap/debootstrap --second-stage
```

### Configuration

1. Toujours dans le terminal _chrooté_ :

```bash
apt install vim -y
```

2. Editez le fichier ```/etc/hostname``` et changez le nom en ```de10-<vos initiales>```.

3. Mettez un mot de passe :

```passwd```

Rien n'apparaît quand vous tapez votre mot de passe, c'est normal.

4. Copiez les lignes suivantes dans le fichier ```/etc/fstab``` :

```bash
none		    /tmp	tmpfs	defaults,noatime,mode=1777	0	0
/dev/mmcblk0p2	/	    ext4	defaults	                0	1
```

5. Activez la liaison série en tapant la commande suivante :

```bash
systemctl enable serial-getty@ttyS0.service
```

6. Configurez et installez les locales :

```bash
apt install locales -y
export PATH=$PATH:/usr/sbin
dpkg-reconfigure locales
```

Ajoutez la locale en_US.UTF-8.

7. Dans le fichier ```/etc/network/interfaces```, sous la ligne ```source-directory /etc/network/interfaces.d```, ajoutez : 


```bash
auto lo eth0
iface lo inet loopback

allow-hotplug eth0
iface eth0 inet dhcp
```

> TODO : Remplacé end0 par eth0 à tester

> TODO : skip sources.list (https://github.com/zangman/de10-nano/blob/master/docs/Debian-Root-File-System.md#configuration)

8. Installez un serveur ssh :

```bash
apt install openssh-server -y
```

Ajoutez (ou décommentez) la ligne ```PermitRootLogin yes``` dans le fichier ```/etc/ssh/sshd_config```.

9. Le reste :

```
apt install haveged -y
apt install net-tools build-essential device-tree-compiler -y
```

### Nettoyage

```bash
apt clean
rm /usr/bin/qemu-arm-static
exit
```

### Création d'une tarbal

Dans le dossier ```emb```:

```bash
cd rootfs
sudo tar -cjpf ../rootfs.tar.bz2 .
cd ..
```