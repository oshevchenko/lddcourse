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
	{ 0x683cfe8d, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0x6b06fdce, __VMLINUX_SYMBOL_STR(delayed_work_timer_fn) },
	{ 0x32eec6bd, __VMLINUX_SYMBOL_STR(param_ops_int) },
	{ 0x170476cb, __VMLINUX_SYMBOL_STR(seq_release) },
	{ 0x47275ef0, __VMLINUX_SYMBOL_STR(seq_read) },
	{ 0xbc78ef3c, __VMLINUX_SYMBOL_STR(seq_lseek) },
	{ 0x6bc3fbc0, __VMLINUX_SYMBOL_STR(__unregister_chrdev) },
	{ 0x8c03d20c, __VMLINUX_SYMBOL_STR(destroy_workqueue) },
	{ 0xacc9c85e, __VMLINUX_SYMBOL_STR(remove_proc_entry) },
	{ 0x4752cef8, __VMLINUX_SYMBOL_STR(proc_create_data) },
	{ 0x50f0a941, __VMLINUX_SYMBOL_STR(__register_chrdev) },
	{ 0xa6bbd805, __VMLINUX_SYMBOL_STR(__wake_up) },
	{ 0x4f8b5ddb, __VMLINUX_SYMBOL_STR(_copy_to_user) },
	{ 0x43a53735, __VMLINUX_SYMBOL_STR(__alloc_workqueue_key) },
	{ 0x167e7f9d, __VMLINUX_SYMBOL_STR(__get_user_1) },
	{ 0x9d378f70, __VMLINUX_SYMBOL_STR(queue_delayed_work_on) },
	{ 0xfb578fc5, __VMLINUX_SYMBOL_STR(memset) },
	{ 0xd2b09ce5, __VMLINUX_SYMBOL_STR(__kmalloc) },
	{ 0x91715312, __VMLINUX_SYMBOL_STR(sprintf) },
	{ 0x26278b7b, __VMLINUX_SYMBOL_STR(mutex_lock_interruptible) },
	{ 0x1afab522, __VMLINUX_SYMBOL_STR(mutex_unlock) },
	{ 0xb1903e77, __VMLINUX_SYMBOL_STR(seq_printf) },
	{ 0xe5afcd1b, __VMLINUX_SYMBOL_STR(seq_open) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0xbdfb6dbb, __VMLINUX_SYMBOL_STR(__fentry__) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "88BF02EB0878FEB0075D54D");
