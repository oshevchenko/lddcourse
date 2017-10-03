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
	{ 0x32eec6bd, __VMLINUX_SYMBOL_STR(param_ops_int) },
	{ 0x64a2ca84, __VMLINUX_SYMBOL_STR(register_netdev) },
	{ 0x870e13d3, __VMLINUX_SYMBOL_STR(alloc_netdev_mqs) },
	{ 0x3fc0ec76, __VMLINUX_SYMBOL_STR(free_netdev) },
	{ 0x68423999, __VMLINUX_SYMBOL_STR(unregister_netdev) },
	{ 0xde7bfa45, __VMLINUX_SYMBOL_STR(netif_rx) },
	{ 0xf21dfa6a, __VMLINUX_SYMBOL_STR(napi_complete_done) },
	{ 0x6128b5fc, __VMLINUX_SYMBOL_STR(__printk_ratelimit) },
	{ 0x53aa46ae, __VMLINUX_SYMBOL_STR(__netdev_alloc_skb) },
	{ 0xb593cbeb, __VMLINUX_SYMBOL_STR(netif_receive_skb) },
	{ 0xb224a4cc, __VMLINUX_SYMBOL_STR(eth_type_trans) },
	{ 0xcb02430d, __VMLINUX_SYMBOL_STR(skb_put) },
	{ 0xdb7305a1, __VMLINUX_SYMBOL_STR(__stack_chk_fail) },
	{ 0xfe890d2a, __VMLINUX_SYMBOL_STR(arp_send) },
	{ 0x69acdf38, __VMLINUX_SYMBOL_STR(memcpy) },
	{ 0x37a0cba, __VMLINUX_SYMBOL_STR(kfree) },
	{ 0xed132595, __VMLINUX_SYMBOL_STR(netif_napi_add) },
	{ 0x3b475b41, __VMLINUX_SYMBOL_STR(ether_setup) },
	{ 0xccca000a, __VMLINUX_SYMBOL_STR(kmem_cache_alloc_trace) },
	{ 0x7875fbf2, __VMLINUX_SYMBOL_STR(kmalloc_caches) },
	{ 0x3874b097, __VMLINUX_SYMBOL_STR(consume_skb) },
	{ 0xccb487c2, __VMLINUX_SYMBOL_STR(__napi_schedule) },
	{ 0x6bf1c17f, __VMLINUX_SYMBOL_STR(pv_lock_ops) },
	{ 0xe259ae9e, __VMLINUX_SYMBOL_STR(_raw_spin_lock) },
	{ 0x287fcd4f, __VMLINUX_SYMBOL_STR(skb_push) },
	{ 0x7758015e, __VMLINUX_SYMBOL_STR(netif_tx_wake_queue) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0x1916e38c, __VMLINUX_SYMBOL_STR(_raw_spin_unlock_irqrestore) },
	{ 0x680ec266, __VMLINUX_SYMBOL_STR(_raw_spin_lock_irqsave) },
	{ 0xbdfb6dbb, __VMLINUX_SYMBOL_STR(__fentry__) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "229908D2D2B55A0E6ADDC9E");
