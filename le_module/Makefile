obj-m:=le_module.o
KERNEL_SOURCE=/home/laurentf/Documents/ensea/git/ENSEA_socfpga_de10_TP/emb/linux-socfpga
CFLAGS_MODULE=-fno-pic

all:
	make -C $(KERNEL_SOURCE) M=$(PWD) modules

clean:
	make -C $(KERNEL_SOURCE) M=$(PWD) clean

install:
	make -C $(KERNEL_SOURCE) M=$(PWD) install