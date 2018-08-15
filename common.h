#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/pci.h>
#define enter() printk(KERN_INFO "ENTER %s\n", __FUNCTION__);
#define leave() printk(KERN_INFO "LEAVE %s\n", __FUNCTION__);
