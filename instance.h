#ifndef __PCICFG_HA_BOX__
#define __PCICFG_HA_BOX__

#include "common.h"
#include "pcicfg.h"
#include "pcibox.h"

#define XEON_DOMAIN                           (0x0000)


#define SCKT0_HA_BUS                         (0x7F)
#define SCKT1_HA_BUS                         (0xFF)


#define HA_DEVICE                      (0x12)

// HA0
#define HA0_FUNCTION                    (0x01)
// HA1
#define HA1_FUNCTION                    (0x05)

#define HA_PCI_PMON_BOX_CTL            (0xF4)
#define HA_PCI_PMON_BOX_STATUS         (0xF8)
#define HA_PCI_PMON_CTL3               (0xE4)
#define HA_PCI_PMON_CTL2               (0xE0)
#define HA_PCI_PMON_CTL1               (0xDC)
#define HA_PCI_PMON_CTL0               (0xD8)
#define HA_PCI_PMON_CTR3               (0xB8)
#define HA_PCI_PMON_CTR2               (0xB0)
#define HA_PCI_PMON_CTR1               (0xA8)
#define HA_PCI_PMON_CTR0               (0xA0)
#define HA_PCI_PMON_BOX_OPCODEMATCH    (0x48)
#define HA_PCI_PMON_BOX_ADDRMATCH1     (0x48)
#define HA_PCI_PMON_BOX_ADDRMATCH0     (0x40)
    
#define HA_PCI_PMON_BOX_CTL_frz        (1 << 8)
#define HA_PCI_PMON_BOX_CTL_rst_ctrs   (1 << 1)
#define HA_PCI_PMON_BOX_CTL_rst_ctrl   (1)
#define HA_PCI_PMON_BOX_STATUS_ov      (0xf)
#define HA_PCI_PMON_CTRL_thresh        (0xff << 24)
#define HA_PCI_PMON_CTRL_invert        (1 << 23)
#define HA_PCI_PMON_CTRL_en            (1 << 22)
#define HA_PCI_PMON_CTRL_ov_en         (1 << 20)
#define HA_PCI_PMON_CTRL_rst           (1 << 17)
#define HA_PCI_PMON_CTRL_umask         (0xff << 8)
#define HA_PCI_PMON_CTRL_ev_sel        (0xff)




// HAn boxes
typedef struct {
    uint8_t event_code;
    uint8_t umask;
    char name[24];
} event_t;

const event_t HA_event_clock_ticks = {
    .event_code = 0x00,
    .umask = 0x00,
    .name = "clock ticks",
};

const event_t HA_event_remote_reads = {
    .event_code = 0x01,
    .umask = 0x02,
    .name = "remote reads",
};

const event_t HA_event_local_reads = {
    .event_code = 0x01,
    .umask = 0x01,
    .name = "local reads",
};

const event_t HA_event_reads = {
    .event_code = 0x01,
    .umask = 0x01,
    .name = "reads",
};
  
typedef struct {
    pcicfg_box_t *box;
    event_t *event;
} HABox_t;

static HABox_t *__get_HAbox(int domain, uint32_t busnr, uint8_t device,
                            uint8_t fn, uint32_t ctrl_addr,
                            uint32_t status_addr) {

    HABox_t *habox = (HABox_t *)kmalloc(sizeof(HABox_t), GFP_KERNEL);
    pcicfg_box_t *box = NULL;    
    if (!habox) {
        printk(KERN_ERR "Can not get HAbox %x:%x.%x\n", busnr, device, fn);
        return NULL;        
    }

    box = get_pcicfg_box(domain, busnr, device, fn, ctrl_addr, status_addr);

    if (!box) {
        printk(KERN_ERR "Can not get HAbox %x:%x.%x\n", busnr, device, fn);
        kfree(habox);
        return NULL;
    }
    habox->box = box;
    habox->event = &HA_event_clock_ticks;
    return habox;
}

HABox_t *get_HAbox(int domain, uint32_t scktnr, int boxnr) {
    if (domain != XEON_DOMAIN) {
        printk(KERN_ERR "domain not supported\n");
        return NULL;        
    }

    if (scktnr == 0) {
        if (boxnr == 0) {
            return __get_HAbox(domain,
                               SCKT0_HA_BUS,
                               HA_DEVICE,
                               HA0_FUNCTION,
                               HA_PCI_PMON_BOX_CTL,
                               HA_PCI_PMON_BOX_STATUS);
        } else if (boxnr == 1) {
            return __get_HAbox(domain,
                               SCKT0_HA_BUS,
                               HA_DEVICE,
                               HA1_FUNCTION,
                               HA_PCI_PMON_BOX_CTL,
                               HA_PCI_PMON_BOX_STATUS);
        } else {
            printk(KERN_ERR "Invalid box number %u\b", boxnr);
        }
    } else if (scktnr == 1) {
        if (boxnr == 0) {
            return __get_HAbox(domain,
                               SCKT1_HA_BUS,
                               HA_DEVICE,
                               HA0_FUNCTION,
                               HA_PCI_PMON_BOX_CTL,
                               HA_PCI_PMON_BOX_STATUS);
        } else if (boxnr == 1) {
            return __get_HAbox(domain,
                               SCKT1_HA_BUS,
                               HA_DEVICE,
                               HA1_FUNCTION,
                               HA_PCI_PMON_BOX_CTL,
                               HA_PCI_PMON_BOX_STATUS);
        } else {
            printk(KERN_ERR "Invalid box number %u\b", boxnr);
            return NULL;
        }
    } else {
        printk(KERN_ERR "invalid socket number %d\n", scktnr);
        return NULL;
    }
    return NULL;
}

void free_HAbox(HABox_t *habox) {
    if (!habox)
        return;
    pcicfg_box_free(habox->box);
    kfree(habox);
    habox = NULL;
}

int HA_box_freeze(HABox_t *habox) {
    uint32_t freeze_dword;
    if (!habox) {
        printk(KERN_ERR "Why you try to freeze an empty habox???\n");
        return -1;
    }

    if (pcicfg_box_read_dword(habox->box,
                              habox->box->control_addr,
                              &freeze_dword) != YEAH)
        return -1;
    freeze_dword |= HA_PCI_PMON_BOX_CTL_frz;
    if (pcicfg_box_write_dword(habox->box,
                               habox->box->control_addr,
                               freeze_dword) != YEAH)
        return -1;
    return 0;
}

int HA_box_unfreeze(HABox_t *habox) {
    uint32_t unfreeze_dword;
    if (!habox) {
        printk(KERN_ERR "Why you try to unfreeze an empty habox???\n");
        return -1;
    }

    if (pcicfg_box_read_dword(habox->box,
                              habox->box->control_addr,
                              &unfreeze_dword) != YEAH)
        return -1;
    unfreeze_dword &= (~HA_PCI_PMON_BOX_CTL_frz);
    if (pcicfg_box_write_dword(habox->box,
                               habox->box->control_addr,
                               unfreeze_dword) != YEAH)
        return -1;
    return 0;
}

int HA_box_reset_ctls(HABox_t *habox) {
    uint32_t reset_dword;
    if (!habox) {
        printk(KERN_ERR "Why you try to reset an empty habox???\n");
        return -1;
    }

    if (pcicfg_box_read_dword(habox->box,
                              habox->box->control_addr,
                              &reset_dword) != YEAH)
        return -1;
    reset_dword |= HA_PCI_PMON_BOX_CTL_rst_ctrl;
    if (pcicfg_box_write_dword(habox->box,
                               habox->box->control_addr,
                               reset_dword) != YEAH)
        return -1;
 
    return 0;
}

int HA_box_reset_ctrs(HABox_t *habox) {
    uint32_t reset_dword;
    if (!habox) {
        printk(KERN_ERR "Why you try to reset an empty habox???\n");
        return -1;
    }
 
    if (pcicfg_box_read_dword(habox->box,
                              habox->box->control_addr,
                              &reset_dword) != YEAH)
        return -1;
    reset_dword |= HA_PCI_PMON_BOX_CTL_rst_ctrs;
    if (pcicfg_box_write_dword(habox->box,
                               habox->box->control_addr,
                               reset_dword) != YEAH)
        return -1;
 
    return 0;
}

int HA_box_clear_overflow(HABox_t *habox) {
    uint32_t overflow;
    if (!habox) {
        printk(KERN_ERR "Why you try to clear overflow of an empty habox???\n");
        return -1;
    }

    if (pcicfg_box_read_dword(habox->box,
                              habox->box->status_addr,
                              &overflow) != YEAH)
        return -1;
    overflow |= HA_PCI_PMON_BOX_STATUS_ov;
    if (pcicfg_box_write_dword(habox->box,
                               habox->box->status_addr,
                               overflow) != YEAH)
        return -1;
 
    return 0;
}

typedef struct {
    uint32_t counter;
    uint32_t controller;
} pair_t;

const pair_t HA_pairs[4] = {
    {
        .counter = HA_PCI_PMON_CTR0,
        .controller = HA_PCI_PMON_CTL0,
    },

    {
        .counter = HA_PCI_PMON_CTR1,
        .controller = HA_PCI_PMON_CTL1,
    },
    
    {
        .counter = HA_PCI_PMON_CTR2,
        .controller = HA_PCI_PMON_CTL2,
    },

    {
        .counter = HA_PCI_PMON_CTR3,
        .controller = HA_PCI_PMON_CTL3,
    },
};

int HA_reset_ctr(HABox_t *habox, int pairnr) {
    uint32_t cl;
    if (!habox) {
        printk(KERN_ERR "Why you try to reset an empty box?\n");
        return -1;
    }

    if (pairnr < 0 || pairnr > 3) {
        printk(KERN_ERR "Why you try to reset a non-exist pari?\n");
        return -1;
    }


    if (pcicfg_box_read_dword(habox->box,
                              HA_pairs[pairnr].controller,
                              &cl) != YEAH)
        return -1;
    cl |= HA_PCI_PMON_CTRL_rst;
    if (pcicfg_box_write_dword(habox->box,
                               HA_pairs[pairnr].controller,
                               cl) != YEAH)
        return -1;
    return 0;
}

int HA_enable(HABox_t *habox, int pairnr) {
    uint32_t cl;
    if (!habox) {
        printk(KERN_ERR "Why you try to enable an empty box?\n");
        return -1;
    }

    if (pairnr < 0 || pairnr > 3) {
        printk(KERN_ERR "Why you try to enable a non-exist pari?\n");
        return -1;
    }


    if (pcicfg_box_read_dword(habox->box,
                              HA_pairs[pairnr].controller,
                              &cl) != YEAH)
        return -1;
    cl |= HA_PCI_PMON_CTRL_en;
    if (pcicfg_box_write_dword(habox->box,
                               HA_pairs[pairnr].controller,
                               cl) != YEAH)
        return -1;
    return 0;
}

int HA_disable(HABox_t *habox, int pairnr) {
    uint32_t cl;
    if (!habox) {
        printk(KERN_ERR "Why you try to disable an empty box?\n");
        return -1;
    }

    if (pairnr < 0 || pairnr > 3) {
        printk(KERN_ERR "Why you try to disable a non-exist pari?\n");
        return -1;
    }


    if (pcicfg_box_read_dword(habox->box,
                              HA_pairs[pairnr].controller,
                              &cl) != YEAH)
        return -1;
    cl &= (~HA_PCI_PMON_CTRL_en);
    if (pcicfg_box_write_dword(habox->box,
                               HA_pairs[pairnr].controller,
                               cl) != YEAH)
        return -1;
    return 0;
}

int HA_enable_overflow(HABox_t *habox, int pairnr) {
    uint32_t cl;
    if (!habox) {
        printk(KERN_ERR "Why you try to enable overflow an empty box?\n");
        return -1;
    }

    if (pairnr < 0 || pairnr > 3) {
        printk(KERN_ERR "Why you try to enable overfow a non-exist pari?\n");
        return -1;
    }


    if (pcicfg_box_read_dword(habox->box,
                              HA_pairs[pairnr].controller,
                              &cl) != YEAH)
        return -1;
    cl |= HA_PCI_PMON_CTRL_ov_en;
    if (pcicfg_box_write_dword(habox->box,
                               HA_pairs[pairnr].controller,
                               cl) != YEAH)
        return -1;
    return 0;
}

int HA_disable_overflow(HABox_t *habox, int pairnr) {
    uint32_t cl;
    if (!habox) {
        printk(KERN_ERR "Why you try to enable overflow an empty box?\n");
        return -1;
    }

    if (pairnr < 0 || pairnr > 3) {
        printk(KERN_ERR "Why you try to enable overfow a non-exist pari?\n");
        return -1;
    }


    if (pcicfg_box_read_dword(habox->box,
                              HA_pairs[pairnr].controller,
                              &cl) != YEAH)
        return -1;
    cl &= (~HA_PCI_PMON_CTRL_ov_en);
    if (pcicfg_box_write_dword(habox->box,
                               HA_pairs[pairnr].controller,
                               cl) != YEAH)
        return -1;
    return 0;
}

int HA_choose_event(HABox_t *habox, int pairnr, const event_t *event) {
    uint32_t cl;
    if (!habox) {
        printk(KERN_ERR "HA box empty?\n");
        return -1;
    }

    if (pairnr < 0 || pairnr > 3) {
        printk(KERN_ERR "Pair number invalid?\n");
        return -1;
    }

    habox->event->event_code = event->event_code;
    habox->event->umask = event->umask;
    strncpy(habox->event->name, event->name, 23);
    habox->event->name[23] = '\0';


    if (pcicfg_box_read_dword(habox->box,
                              HA_pairs[pairnr].controller,
                              &cl) != YEAH)
        return -1;
    cl &= 0xffff0000;
    cl |= ((uint32_t)event->event_code << 8) | event->umask;
    if (pcicfg_box_write_dword(habox->box,
                               HA_pairs[pairnr].controller,
                               cl) != YEAH)
        return -1;
    return 0;
}

int HA_read_counter(HABox_t *habox, int pairnr, uint64_t *val) {
    if (!habox) {
        printk(KERN_ERR "HA box empty?\n");
        return -1;
    }

    if (pairnr < 0 || pairnr > 3) {
        printk(KERN_ERR "Pair number invalid?\n");
        return -1;
    }
    
    return (pcicfg_box_read_qword(habox->box,
                                  HA_pairs[pairnr].counter,
                                  val) != YEAH);
}
#endif

