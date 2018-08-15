#include "common.h"
#include "pcicfg.h"
#include "pcibox.h"

// always remember to free pcicfg_box

pcicfg_box_t *get_pcicfg_box(int domain,
                              int busnr,
                              int device,
                              int fn,
                              uint32_t ctrl_addr,
                              uint32_t status_addr) {
    pcicfg_t *pcicfg = NULL;
    pcicfg_box_t *pcicfg_box =
        (pcicfg_box_t *)kmalloc(sizeof(pcicfg_box_t), GFP_KERNEL);
    enter();
    if (!pcicfg_box) {
        printk(KERN_ERR "No memory for a box!!!!\n");
        leave()
        return NULL;
    }
    
    pcicfg_box->inited = 0;
    
    if (!(pcicfg = get_pcicfg(domain, busnr, device, fn))) {
        printk(KERN_ERR "can n1ot initialize this pcicfg box\n");
        kfree(pcicfg_box);
        leave()
        return NULL;
    } else {
        pcicfg_box->pcicfg_space = pcicfg;
        pcicfg_box->control_addr = ctrl_addr;
        pcicfg_box->status_addr = status_addr;
        if (pcicfg_read_dword(pcicfg, ctrl_addr, &pcicfg_box->control) != YEAH) {
            printk(KERN_WARNING "read control register failed\n");
            kfree(pcicfg_box);
            leave()
            return NULL;
        }
        if (pcicfg_read_dword(pcicfg, status_addr, &pcicfg_box->status) != YEAH) {
            printk(KERN_WARNING "read status register failed\n");
            kfree(pcicfg_box);
            leave()
            return NULL;
        }
        pcicfg_box->inited = INITED;
    }
    leave()
    return pcicfg_box;
}


static int box_check(pcicfg_box_t *box) {
    enter();
    if (!box || box->inited != INITED) {
        printk(KERN_ERR "pcicfg_box can not be used!!!!\n");
        leave();
        return -ENOBOX;
    }
    leave();
    return 1;
}


int pcicfg_box_read_byte(pcicfg_box_t *pcicfg_box, int where, uint8_t *val) {
    enter();
    if (!box_check(pcicfg_box) || !val) {
        leave();
        return -EREAD;
    }
    leave();
    return pcicfg_read_byte(pcicfg_box->pcicfg_space, where, val);
}

int pcicfg_box_read_word(pcicfg_box_t *pcicfg_box, int where, uint16_t *val) {
    enter();
    if (!box_check(pcicfg_box) || !val) {
        leave();
        return -EREAD;
    }
    leave();
    return pcicfg_read_word(pcicfg_box->pcicfg_space, where, val);
}

int pcicfg_box_read_dword(pcicfg_box_t *pcicfg_box, int where, uint32_t *val) {
    enter();
    if (!box_check(pcicfg_box) || !val) {
        leave();
        return -EREAD;
    }
    leave();
    return pcicfg_read_dword(pcicfg_box->pcicfg_space, where, val);
}

int pcicfg_box_read_qword(pcicfg_box_t *pcicfg_box, int where, uint64_t *val) {
    enter();
    if (!box_check(pcicfg_box) || !val) {
        leave();
        return -EREAD;
    }
    leave();
    return pcicfg_read_qword(pcicfg_box->pcicfg_space, where, val);
}

int pcicfg_box_write_byte(pcicfg_box_t *pcicfg_box, int where, uint8_t val) {
    enter();
    if (!box_check(pcicfg_box)) {
        leave();
        return -EREAD;
    }
    leave();
    return pcicfg_write_byte(pcicfg_box->pcicfg_space, where, val);
}

int pcicfg_box_write_word(pcicfg_box_t *pcicfg_box, int where, uint16_t val) {
    enter();
    if (!box_check(pcicfg_box)) {
        leave();
        return -EWRITE;
    }
    leave();
    return pcicfg_write_word(pcicfg_box->pcicfg_space, where, val);
}

int pcicfg_box_write_dword(pcicfg_box_t *pcicfg_box, int where, uint32_t val) {
    enter();
    if (!box_check(pcicfg_box)) {
        leave();
        return -EWRITE;
    }
    leave();
    return pcicfg_write_dword(pcicfg_box->pcicfg_space, where, val);
}

int pcicfg_box_write_qword(pcicfg_box_t *pcicfg_box, int where, uint64_t val) {
    enter();
    if (!box_check(pcicfg_box)) {
        leave();
        return -EWRITE;
    }
    leave();
    return pcicfg_write_qword(pcicfg_box->pcicfg_space, where, val);
}

void pcicfg_box_free(pcicfg_box_t *box) {
    enter();
    if (!box_check(box)) {
        printk(KERN_WARNING "You can not free an uninitialized box\n");
        leave();
        return;
    }
    pcicfg_free(box->pcicfg_space);
    kfree(box);
    box = NULL;
    leave();
}
