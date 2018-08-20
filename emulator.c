#include <linux/kthread.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/smp.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/string.h>

// #include "instance.h"
#include "large_header.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Dicridon Xiong");
MODULE_DESCRIPTION("A NVM emulator");
MODULE_VERSION("0.1");

static char *mode = "wr";
module_param(mode, charp, 0);
MODULE_PARM_DESC(mode, "specify an emulating mode: w, r, wr");

struct task_struct *kthread;
HABox_t *HA0;

void delay(void *d) {
    uint64_t delay_time = (*((uint64_t *)d));
    int cpu = get_cpu();
    put_cpu();
    printk(KERN_INFO "SMP on cpu %d, access number: %lld\n", cpu, delay_time);
    
    mdelay(delay_time / 2 + 2000);
}

int emulator(void* mode) {
    uint64_t counter = 0;
    uint64_t delay_count = 100;
    int cpu = get_cpu();
    int err = 0;
    put_cpu();
    if (cpu != 1) {
        printk(KERN_WARNING "Emulation result may not be correct since emulator is not on cpu 0\n");
        return -1;
    } else {
        printk(KERN_INFO "Emulation started\n");
    }
    
    HA0 = get_HAbox(XEON_DOMAIN, 0, 0);
    HA_box_freeze(HA0);
    HA_box_reset_ctls(HA0);
    HA_box_reset_ctrs(HA0);
    HA_box_clear_overflow(HA0);
    HA_disable_overflow(HA0, 0);

    if (strcmp((char*)mode, "wr") == 0) {
        printk(KERN_INFO "read and write emulation\n");
        HA_choose_event(HA0, 0, &HA_event_remote_access);        
    }
    else if (strcmp((char*)mode, "w") == 0) {
        printk(KERN_INFO "only write emulation\n");
        HA_choose_event(HA0, 0, &HA_event_remote_writes);
    }
    else if (strcmp((char*)mode, "r") == 0) {
        printk(KERN_INFO "only read emulation\n");
        HA_choose_event(HA0, 0, &HA_event_remote_reads);        
    }
    else {
        printk(KERN_INFO "Unknown option, default is wr emulation\n");
        HA_choose_event(HA0, 0, &HA_event_remote_access);        
    }


    HA_enable(HA0, 0);        // pair0 in HA0 in socket0 will monitor remote reads
    HA_box_unfreeze(HA0);
    msleep(10);

    while(1) {
        HA_box_freeze(HA0);
        HA_read_counter(HA0, 0, &counter);
        if (counter >= 1000) {
	    delay_count = counter;
	    printk(KERN_INFO "delay count is %lld\n", delay_count);
            err = smp_call_function_single(12, delay, &delay_count, 1);
        }
        // printk(KERN_INFO "REMOTE READS %lld\n", counter);
	if (err != 0) {
            printk(KERN_WARNING "sending smp failed\n");
	    err = 0;
	    msleep(2000);
	}
        HA_reset_ctr(HA0, 0);
        // disable overflow only disable PMI interrupt, must clear overflow signal
        // manually
        HA_box_clear_overflow(HA0);
        HA_box_unfreeze(HA0);
        if (kthread_should_stop()) {
            printk(KERN_INFO "Signal received, thread ends\n");
            do_exit(0);
        }
        msleep(10);
    }
    return 0;
}

static int __init start_emulator(void) {
    if (strcmp("wr", mode) != 0 && strcmp("r", mode) != 0 && strcmp("w", mode) != 0) {
        printk(KERN_WARNING "Invalid option %s\n", mode);
        printk(KERN_INFO "insmod emulator.ko w/r/wr\n");
        return -1;
    }
    
    kthread = kthread_create(emulator, mode, "Emulator");

    if (!kthread) {
        printk(KERN_ERR "kernel thread creation failed\n");
        return -1;
    }
    // cpu 1 on socket 0, client should run on socket 1
    kthread_bind(kthread, 1); 
    wake_up_process(kthread);
    printk(KERN_INFO "module installed\n");
    return 0;
}

static void __exit terminate_emulator(void) {
    kthread_stop(kthread);
    free_HAbox(HA0);
    printk(KERN_INFO "module removed\n");
}

module_init(start_emulator);
module_exit(terminate_emulator);
