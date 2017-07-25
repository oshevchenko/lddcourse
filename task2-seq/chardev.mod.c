#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0x75999ebb, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0xd5dea03a, __VMLINUX_SYMBOL_STR(seq_release) },
	{ 0xa219133c, __VMLINUX_SYMBOL_STR(seq_read) },
	{ 0x8fa79405, __VMLINUX_SYMBOL_STR(seq_lseek) },
	{ 0x6bc3fbc0, __VMLINUX_SYMBOL_STR(__unregister_chrdev) },
	{ 0x8b7f6335, __VMLINUX_SYMBOL_STR(remove_proc_entry) },
	{ 0xd2475303, __VMLINUX_SYMBOL_STR(proc_create_data) },
	{ 0xe23149a4, __VMLINUX_SYMBOL_STR(__register_chrdev) },
	{ 0xc3aaf0a9, __VMLINUX_SYMBOL_STR(__put_user_1) },
	{ 0x167e7f9d, __VMLINUX_SYMBOL_STR(__get_user_1) },
	{ 0xdb7305a1, __VMLINUX_SYMBOL_STR(__stack_chk_fail) },
	{ 0xfcca6c0b, __VMLINUX_SYMBOL_STR(try_module_get) },
	{ 0x91715312, __VMLINUX_SYMBOL_STR(sprintf) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0xb0384ade, __VMLINUX_SYMBOL_STR(module_put) },
	{ 0xab6b0577, __VMLINUX_SYMBOL_STR(seq_printf) },
	{ 0x4503d64e, __VMLINUX_SYMBOL_STR(seq_open) },
	{ 0xbdfb6dbb, __VMLINUX_SYMBOL_STR(__fentry__) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "65DCE652629E5C7BDEB2A04");
