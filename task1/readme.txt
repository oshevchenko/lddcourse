В репозиторій я додав тільки папки з потрібними файлами, весь код ядра не копіював. За основу взяв https://wiki.openwrt.org/toh/tp-link/tl-mr3020
В kernels\mips-linux-2.6.31\drivers\:
- додати "source "drivers/misc-modules/Kconfig"" в Kconfig
- додати "obj-$(CONFIG_MISC_MODULES)		+= misc-modules/" в Makefile
В kernels\mips-linux-2.6.31\drivers\misc-modules\:
- створити Kconfig;
- відредагувати Makefile.