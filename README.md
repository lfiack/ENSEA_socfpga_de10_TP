# ENSEA_socfpga_de10_TP
TP SoCFPGA sur DE10

## Outils à installer 

```bash
yay -S ncurses flex bison openssl dkms libelf systemd-libs pciutils libmpc autoconf bc
```

```bash
yay -S extra/debootstrap extra/qemu-user-static extra/qemu-user-static-binfmt
```

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
cd u-boot
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

> TODO captures d'écran

https://github.com/zangman/de10-nano/blob/master/docs/Building-the-Kernel.md#kernel-options

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

## Création de la carte SD

Dans le dossier ```emb```:

```bash
mkdir sdcard
cd sdcard
```

Créez une image vide de 2GB :

```bash
sudo dd if=/dev/zero of=sdcard.img bs=2G count=1
```

Rendre l'image visible comme un device.

```bash
sudo losetup --show -f sdcard.img
```

### Partitionner l'image

To run Embedded Linux on the DE10-Nano, we need 3 partitions as shown in the table below. Partition numbers have to be exactly as shown below. File sizes also have to be exactly as shown, except for the Root Filesystem which can be increased to take up all the remaining space on the SD card. In our case, we're using a 1GB image file so, we'll have the Root Filesystem take up around 750MB.

Note that you should create them in the exact order listed when using fdisk.

> Suggestion: If you are creating an image and writing to an SD card several times because you are trying some experimental features, it's better to keep the file size low like 1GB for the entire SD card. This makes it faster to write the image to the SD Card.

| Order | Partition              | Partition Type | Partition Number | Last Sector | FS Type       | FS Hex Code |
| ----- | ---------------------- | -------------- | ---------------- | ----------- | ------------- | ----------- |
| 1     | U-Boot and SPL         | primary        | 3                | +1M         | Altera Custom | a2          |
| 2     | Kernel and Device Tree | primary        | 1                | +254M       | fat32         | b           |
| 3     | Root Filesystem        | primary        | 2                | _default_   | ext4          | 83          |


Utilisez l'outil ```fdisk``` pour partitionner le fichier.

```bash
sudo fdisk /dev/loop0
```

Appuyez sur `p` puis `Entrée` pour voir la liste des partitions:

```bash
Welcome to fdisk (util-linux 2.42.1).
Changes will remain in memory only, until you decide to write them.
Be careful before using the write command.

Device does not contain a recognized partition table.
Created a new DOS (MBR) disklabel with disk identifier 0x4a4d3a1e.

Command (m for help): p
Disk /dev/loop0: 2 GiB, 2147479552 bytes, 4194296 sectors
Units: sectors of 1 * 512 = 512 bytes
Sector size (logical/physical): 512 bytes / 512 bytes
I/O size (minimum/optimal): 512 bytes / 512 bytes
Disklabel type: dos
Disk identifier: 0x4a4d3a1e

Command (m for help): 
```

#### Partition du bootloader

As you can see, there are no partitions at the moment. Let's create them as per the table above. For the first partition, you will need to type in the following commands in the fdisk prompt. For example, the first step is `n` followed by `enter`.

1. `n`, `enter`
2. `p`, `enter`
3. `3`, `enter`
4. `enter`
5. `+1M`, `enter`

If you typed everything correctly, it should look as shown below:

```bash
Command (m for help): n
Partition type
   p   primary (0 primary, 0 extended, 4 free)
   e   extended (container for logical partitions)
Select (default p): p
Partition number (1-4, default 1): 3
First sector (2048-4194295, default 2048): 
Last sector, +/-sectors or +/-size{K,M,G,T,P} (2048-4194295, default 4194295): +1M

Created a new partition 3 of type 'Linux' and of size 1 MiB.

Command (m for help): 
```

You can see that it assigned it the default filesystem of `Linux`. We need to change that to `Altera Custom`. This is not a standard filesystem, so we'll need to manually assign the hex code `a2`. For this, enter the following commands:

1. `t`, `enter`
2. `a2`, `enter`

```bash
Command (m for help): t
Selected partition 3
Hex code (type L to list all codes): a2
Changed type of partition 'Linux' to 'unknown'.
```

#### Kernel and Device Tree partition

For the next partition, type the following commands:

1. `n`, `enter`
2. `p`, `enter`
3. `1`, `enter`
4. `enter`
5. `+254M`, `enter`

The output should look like:

```bash
Command (m for help): n
Partition type
   p   primary (1 primary, 0 extended, 3 free)
   e   extended (container for logical partitions)
Select (default p): p
Partition number (1,2,4, default 1): 1
First sector (4096-4194295, default 4096): 
Last sector, +/-sectors or +/-size{K,M,G,T,P} (4096-4194295, default 4194295): +254M

Created a new partition 1 of type 'Linux' and of size 254 MiB.
```

Again, let's change the filesystem type. Type the following commands:

1. `t`, `enter`
2. `1`,`enter`
3. `b`, `enter`

```bash
Command (m for help): t
Partition number (1,3, default 3): 1
Hex code (type L to list all codes): b

Changed type of partition 'Linux' to 'W95 FAT32'.

Command (m for help):
```

#### Root Partition

For the last partition, we will assign whatever space remains in the image file. Here are the commands:

1. `n`, `enter`
2. `p`, `enter`
3. `2`, `enter`
4. `enter`
5. `enter`

```bash
Command (m for help): n
Partition type
   p   primary (2 primary, 0 extended, 2 free)
   e   extended (container for logical partitions)
Select (default p): p
Partition number (2,4, default 2): 2
First sector (524288-4194295, default 524288): 
Last sector, +/-sectors or +/-size{K,M,G,T,P} (524288-4194295, default 4194295): 

Created a new partition 2 of type 'Linux' and of size 1,7 GiB.
```

We will keep the default `Linux` partition type for this.


#### Writing the partition table

Check that the partitions are created as expected. Here is what I see when I type in `p`, `enter`:

```bash
Command (m for help): p
Disk /dev/loop0: 2 GiB, 2147479552 bytes, 4194296 sectors
Units: sectors of 1 * 512 = 512 bytes
Sector size (logical/physical): 512 bytes / 512 bytes
I/O size (minimum/optimal): 512 bytes / 512 bytes
Disklabel type: dos
Disk identifier: 0x4a4d3a1e

Device       Boot  Start     End Sectors  Size Id Type
/dev/loop0p1        4096  524287  520192  254M  b W95 FAT32
/dev/loop0p2      524288 4194295 3670008  1,7G 83 Linux
/dev/loop0p3        2048    4095    2048    1M a2 unknown
```

The partitions created so far haven't been written to the image file yet. So let's put them in with the command `w`, `enter`:

```bash
Command (m for help): w
The partition table has been altered.
Calling ioctl() to re-read partition table.
Re-reading the partition table failed.: Invalid argument

The kernel still uses the old table. The new table will be used at the next reboot or after you run partprobe(8) or kpartx(8).
```

The error in the message tells us that the partitions haven't been loaded by the kernel, which is indeed the case if you type:

```bash
ls /dev/loop0*
```

you will see we only have one device `/dev/loop0` and the partitions are not visible.

To access them for mounting and writing the contents, we have to run

```bash
sudo partprobe /dev/loop0
```

Now if you type:

```bash
ls /dev/loop0*
```

You should see the partitions:

```bash
/dev/loop0  /dev/loop0p1  /dev/loop0p2  /dev/loop0p3
```

### Creating the file systems

Lets create the fat and ext4 filesystems:

```bash
# Partition 1 is FAT
sudo mkfs -t vfat /dev/loop0p1

# Partition 2 is Linux
sudo mkfs.ext4 /dev/loop0p2
```


### Writing to the partitions

Now we'll populate the various partitions.

#### Bootloader partition

The bootloader partition is a binary partition which needs to be written in raw format. We don't have to mount it, so we'll just use the `dd` command to write it directly:

```bash
cd $DEWD
cd sdcard
sudo dd if=../u-boot/u-boot-with-spl.sfp of=/dev/loop0p3 bs=64k seek=0 oflag=sync
```

#### Kernel and Device Tree partition

This is a fat partition, so we'll need to mount it first and copy the files:

```bash
cd $DEWD
cd sdcard
mkdir -p fat

# Mount the fat partition.
sudo mount /dev/loop0p1 fat

# Copy the kernel image.
sudo cp ../linux-socfpga/arch/arm/boot/zImage fat

# Copy the de0 device tree.
sudo cp ../linux-socfpga/arch/arm/boot/dts/intel/socfpga/socfpga_cyclone5_de0_nano_soc.dtb fat

# Create the extlinux config file for the bootloader.
echo "LABEL Linux Default" > extlinux.conf
echo "    KERNEL ../zImage" >> extlinux.conf
echo "    FDT ../socfpga_cyclone5_de0_nano_soc.dtb" >> extlinux.conf
echo "    APPEND root=/dev/mmcblk0p2 rw rootwait earlyprintk console=ttyS0,115200n8" >> extlinux.conf

# Copy it into the extlinux folder.
sudo mkdir -p fat/extlinux
sudo cp extlinux.conf fat/extlinux

# Unmount the partition.
sudo umount fat
```

> **Note**: One thing to call out is that we are using the device tree for the DE0. The reasons for this are explained in the appendix.

#### Root Filesystem partition

> Note: If your rootfs is Archlinux ARM, then jump to [this section](./Archlinux-ARM-Root-File-System.md) to continue with the rootfs setup instead of the steps below.

For the final partition, here are the steps:

```bash
cd $DEWD
cd sdcard
mkdir -p ext4

# Mount the ext4 partition.
sudo mount /dev/loop0p2 ext4

# Extract the rootfs archive.
cd ext4
sudo tar -xvf ../../rootfs.tar.bz2

# Unmount the partition.
cd ..
sudo umount ext4
```

Quite straightfoward.

### Writing to SD Card

The hard work is done. Now all that's left is to write the `sdcard.img` file to an actual SD Card and we're ready to boot.

#### Writing in Linux

If you are not using Virtualbox for your Debian OS or if you have access to the SD Card device directly in the virtual machine, then you can use the following command to write the image file directly to the SD Card.

> **WARNING** - Be extremely careful with the command below. If you type the wrong device, there are no warnings, it will wipe your system clean.

```bash
cd $DEWD
cd sdcard

# Identify your SD Card device.
lsblk

# Write to the correct device (Ex: /dev/sdb).
sudo dd if=sdcard.img of=/dev/sdb bs=64K status=progress
```