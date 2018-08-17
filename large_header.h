#include "common.h"
#include <linux/pci.h>

#define YEAH              (0x0)
#define INVALID_DOMAIN    (0x1)
#define INVALID_BUS       (0x1)
#define INVALID_DEVICE    (0x3)
#define INVALID_FUNCTION  (0x4)
#define ENOPCICFG_FOUND   (0x5)

#define PCI_READ_FAILED   (0x6)
#define PCI_WRITE_FAILED   (0x6)


#define INITED            (0x3124)

typedef struct {
    uint16_t domain;   // 0 to 0xffff
    struct pci_bus *bus;
    uint8_t  device;   // 0 to 31
    uint8_t  function; // 0 to 7
    int inited;        // set to INITED if init_pcicfg is called on this struct
} pcicfg_t;

pcicfg_t *get_pcicfg(int domain, int busnr, int device, int fn);
int pcicfg_read_byte(pcicfg_t *pcicfg, int where, uint8_t *val);
int pcicfg_read_word(pcicfg_t *pcicfg, int where, uint16_t *val);
int pcicfg_read_dword(pcicfg_t *pcicfg, int where, uint32_t *val);
int pcicfg_read_qword(pcicfg_t *pcicfg, int where, uint64_t *val);
int pcicfg_write_byte(pcicfg_t *pcicfg, int where, uint8_t val);
int pcicfg_write_word(pcicfg_t *pcicfg, int where, uint16_t val);
int pcicfg_write_dword(pcicfg_t *pcicfg, int where, uint32_t val);
int pcicfg_write_qword(pcicfg_t *pcicfg, int where, uint64_t val);
void pcicfg_free(pcicfg_t *pcicfg);

#define ENOBOX        (0x1)
#define EREAD         (0x2)
#define EWRITE        (0x3)

typedef struct {
    pcicfg_t *pcicfg_space;
    uint32_t control;         // box-level control register
    uint32_t status;          // box-level status register
    uint32_t control_addr;
    uint32_t status_addr;
    int inited;
} pcicfg_box_t;

// domain number, bus number, device number and funtion number may represent a
// unique CFG.
pcicfg_box_t *get_pcicfg_box(int domain,
                             int busnr,
                             int device,
                             int fn,
                             uint32_t control,
                             uint32_t status);

int pcicfg_box_read_byte(pcicfg_box_t *pcicfg_box, int where, uint8_t *val);
int pcicfg_box_read_word(pcicfg_box_t *pcicfg_box, int where, uint16_t *val);
int pcicfg_box_read_dword(pcicfg_box_t *pcicfg_box, int where, uint32_t *val);
int pcicfg_box_read_qword(pcicfg_box_t *pcicfg_box, int where, uint64_t *val);
int pcicfg_box_write_byte(pcicfg_box_t *pcicfg_box, int where, uint8_t val);
int pcicfg_box_write_word(pcicfg_box_t *pcicfg_box, int where, uint16_t val);
int pcicfg_box_write_dword(pcicfg_box_t *pcicfg_box, int where, uint32_t val);
int pcicfg_box_write_qword(pcicfg_box_t *pcicfg_box, int where, uint64_t val);
void pcicfg_box_free(pcicfg_box_t *box);

pcicfg_t *get_pcicfg(int domain, int busnr, int device, int fn) {

    pcicfg_t *pcicfg = (pcicfg_t*)kmalloc(sizeof(pcicfg_t), GFP_KERNEL);
    struct pci_bus *bus = NULL;
    if (!pcicfg) {
        printk(KERN_ERR "No memory for a pcicfg!!!\n");
        return NULL;
    }
    
    pcicfg->inited = 0;
    
    if (domain < 0 || domain > 0xffff) {
        printk(KERN_ERR "domain is not valid\n");
        kfree(pcicfg);
        return NULL;
    }

    if (busnr < 0) {
        printk(KERN_ERR "busnr is not valid\n");
        kfree(pcicfg);
        return NULL;
    }

    if (device < 0 || device > 31) {
        printk(KERN_ERR "device number is not valid\n");
        kfree(pcicfg);
        return NULL;
    }

    if (fn < 0 || fn > 7) {
        printk(KERN_ERR "function number is not valid\n");
        kfree(pcicfg);
        return NULL;
    }
    
    bus = pci_find_bus(domain, busnr);
    if (!bus) {
        printk(KERN_ERR "bus not found\n");
        kfree(pcicfg);
        return NULL;
    } else {
        pcicfg->domain = domain;
        pcicfg->bus = bus;
        pcicfg->device = device;
        pcicfg->function = fn;
        pcicfg->inited = INITED;
    }
    return pcicfg;
}

static int inited(pcicfg_t *pcicfg) {
    if (!pcicfg || pcicfg->inited != INITED) {
        printk(KERN_WARNING "This pcicfg is not initialized by init_pcicfg!\n");
        return -1;
    }
    return 1;
}



/*
  Generally, device number is 5-bit wide and function 3-bit wide
  Linux combined them into one byte.
*/
int pcicfg_read_byte(pcicfg_t *pcicfg, int where, uint8_t *val) {
    unsigned int devfn = 0;
    if (!inited(pcicfg) || !val) {
        return -PCI_READ_FAILED;
    }
    devfn = (pcicfg->device << 3) | (pcicfg->function);
    return pci_bus_read_config_byte(pcicfg->bus, devfn, where, val);
}

int pcicfg_read_word(pcicfg_t *pcicfg, int where, uint16_t *val) {
    unsigned int devfn = 0;
    if (!inited(pcicfg) || !val) {
        return -PCI_READ_FAILED;
    }
    devfn = (pcicfg->device << 3) | (pcicfg->function);
    return pci_bus_read_config_word(pcicfg->bus, devfn, where, val);
}

int pcicfg_read_dword(pcicfg_t *pcicfg, int where, uint32_t *val) {
    unsigned int devfn = 0;
    if (!inited(pcicfg) || !val) {
        return -PCI_READ_FAILED;
    }
    devfn = (pcicfg->device << 3) | (pcicfg->function);
    return pci_bus_read_config_dword(pcicfg->bus, devfn, where, val);
}

/*
  Some counters may consist of two 32-bits registers, if so, 
  @where should be address of the low 32-bit register
*/
int pcicfg_read_qword(pcicfg_t *pcicfg, int where, uint64_t *val) {
    unsigned int devfn = 0;
    uint32_t temp = 0;
    if (!inited(pcicfg) || !val) {
        return -PCI_READ_FAILED;
    }
    *val = 0;
    devfn = (pcicfg->device << 3) | (pcicfg->function);
    temp = 0;
    if (pci_bus_read_config_dword(pcicfg->bus,
                              devfn,
                              where + 4,
                              &temp) != PCIBIOS_SUCCESSFUL) {
        printk(KERN_WARNING "read lower 32 bits failed\n");
        return -PCI_READ_FAILED;
    }
    *val |= temp;
    *val = (*val) << 32;
    if (pci_bus_read_config_dword(pcicfg->bus,
                              devfn,
                              where,
                              &temp) != PCIBIOS_SUCCESSFUL) {
        printk(KERN_WARNING "read higher 32 bits failed\n");
        return -PCI_READ_FAILED;
    }
    *val |= temp;
    return YEAH;
}


int pcicfg_write_byte(pcicfg_t *pcicfg, int where, uint8_t val) {
    unsigned int devfn = 0;
    if (!inited(pcicfg)) {
        return -PCI_WRITE_FAILED;
    }
    devfn = (pcicfg->device << 3) | (pcicfg->function);
    return pci_bus_write_config_byte(pcicfg->bus, devfn, where, val);
}

int pcicfg_write_word(pcicfg_t *pcicfg, int where, uint16_t val) {
    unsigned int devfn = 0;
    if (!inited(pcicfg)) {
        return -PCI_WRITE_FAILED;
    }
    devfn = (pcicfg->device << 3) | (pcicfg->function);
    return pci_bus_write_config_word(pcicfg->bus, devfn, where, val);
}

int pcicfg_write_dword(pcicfg_t *pcicfg, int where, uint32_t val) {
    unsigned int devfn = 0;
    if (!inited(pcicfg)) {
        return -PCI_WRITE_FAILED;
    }
    devfn = (pcicfg->device << 3) | (pcicfg->function);
    return pci_bus_write_config_dword(pcicfg->bus, devfn, where, val);
}

int pcicfg_write_qword(pcicfg_t *pcicfg, int where, uint64_t val) {
    unsigned int devfn = 0;
    uint32_t temp = 0;
    if (!inited(pcicfg)) {
        return -PCI_WRITE_FAILED;
    }
    devfn = (pcicfg->device << 3) | (pcicfg->function);
    temp = (uint32_t)val;
    if (pci_bus_write_config_dword(pcicfg->bus,
                               devfn,
                               where,
                               temp) != PCIBIOS_SUCCESSFUL) {
        printk(KERN_WARNING "write lower 32 bits failed\n");
        return -PCI_WRITE_FAILED;
    }
    temp = val >> 32;
    if (pci_bus_write_config_dword(pcicfg->bus,
                               devfn,
                               where + 4,
                               temp) != PCIBIOS_SUCCESSFUL) {
        printk(KERN_WARNING "write higher 32 bits failed\n");
        return -PCI_WRITE_FAILED;
    }
    return YEAH;
}

void pcicfg_free(pcicfg_t *pcicfg) {
    if (!pcicfg) {
        return;
    }
    kfree(pcicfg);
    pcicfg = NULL;
}

pcicfg_box_t *get_pcicfg_box(int domain,
                              int busnr,
                              int device,
                              int fn,
                              uint32_t ctrl_addr,
                              uint32_t status_addr) {
    pcicfg_t *pcicfg = NULL;
    pcicfg_box_t *pcicfg_box =
        (pcicfg_box_t *)kmalloc(sizeof(pcicfg_box_t), GFP_KERNEL);
    if (!pcicfg_box) {
        printk(KERN_ERR "No memory for a box!!!!\n");
        return NULL;
    }
    
    pcicfg_box->inited = 0;
    
    if (!(pcicfg = get_pcicfg(domain, busnr, device, fn))) {
        printk(KERN_ERR "can n1ot initialize this pcicfg box\n");
        kfree(pcicfg_box);
        return NULL;
    } else {
        pcicfg_box->pcicfg_space = pcicfg;
        pcicfg_box->control_addr = ctrl_addr;
        pcicfg_box->status_addr = status_addr;
        if (pcicfg_read_dword(pcicfg, ctrl_addr, &pcicfg_box->control) != YEAH) {
            printk(KERN_WARNING "read control register failed\n");
            kfree(pcicfg_box);
            return NULL;
        }
        if (pcicfg_read_dword(pcicfg, status_addr, &pcicfg_box->status) != YEAH) {
            printk(KERN_WARNING "read status register failed\n");
            kfree(pcicfg_box);
            return NULL;
        }
        pcicfg_box->inited = INITED;
    }
    return pcicfg_box;
}


static int box_check(pcicfg_box_t *box) {
    if (!box || box->inited != INITED) {
        printk(KERN_ERR "pcicfg_box can not be used!!!!\n");
        return -ENOBOX;
    }
    return 1;
}


int pcicfg_box_read_byte(pcicfg_box_t *pcicfg_box, int where, uint8_t *val) {
    if (!box_check(pcicfg_box) || !val) {
        return -EREAD;
    }
    return pcicfg_read_byte(pcicfg_box->pcicfg_space, where, val);
}

int pcicfg_box_read_word(pcicfg_box_t *pcicfg_box, int where, uint16_t *val) {
    if (!box_check(pcicfg_box) || !val) {
        return -EREAD;
    }
    return pcicfg_read_word(pcicfg_box->pcicfg_space, where, val);
}

int pcicfg_box_read_dword(pcicfg_box_t *pcicfg_box, int where, uint32_t *val) {
    if (!box_check(pcicfg_box) || !val) {
        return -EREAD;
    }
    return pcicfg_read_dword(pcicfg_box->pcicfg_space, where, val);
}

int pcicfg_box_read_qword(pcicfg_box_t *pcicfg_box, int where, uint64_t *val) {
    if (!box_check(pcicfg_box) || !val) {
        return -EREAD;
    }
    return pcicfg_read_qword(pcicfg_box->pcicfg_space, where, val);
}

int pcicfg_box_write_byte(pcicfg_box_t *pcicfg_box, int where, uint8_t val) {
    if (!box_check(pcicfg_box)) {
        return -EREAD;
    }
    return pcicfg_write_byte(pcicfg_box->pcicfg_space, where, val);
}

int pcicfg_box_write_word(pcicfg_box_t *pcicfg_box, int where, uint16_t val) {
    if (!box_check(pcicfg_box)) {
        return -EWRITE;
    }
    return pcicfg_write_word(pcicfg_box->pcicfg_space, where, val);
}

int pcicfg_box_write_dword(pcicfg_box_t *pcicfg_box, int where, uint32_t val) {
    if (!box_check(pcicfg_box)) {
        return -EWRITE;
    }
    return pcicfg_write_dword(pcicfg_box->pcicfg_space, where, val);
}

int pcicfg_box_write_qword(pcicfg_box_t *pcicfg_box, int where, uint64_t val) {
    if (!box_check(pcicfg_box)) {
        return -EWRITE;
    }
    return pcicfg_write_qword(pcicfg_box->pcicfg_space, where, val);
}

void pcicfg_box_free(pcicfg_box_t *box) {
    if (!box_check(box)) {
        printk(KERN_WARNING "You can not free an uninitialized box\n");
        return;
    }
    pcicfg_free(box->pcicfg_space);
    kfree(box);
    box = NULL;
}

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

// Reads
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
    .umask = 0x03,
    .name = "reads",
};

// Writes
const event_t HA_event_remote_writes = {
    .event_code = 0x01,
    .umask = 0x20,
    .name = "remote writes",
};

const event_t HA_event_local_writes = {
    .event_code = 0x01,
    .umask = 0x10,
    .name = "local writes",
};

const event_t HA_event_writes = {
    .event_code = 0x01,
    .umask = 0x30,
    .name = "writes",
};

const event_t HA_event_remote_access = {
    .event_code = 0x01,
    .umask = 0x22,
    .name = "remote aceess",
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

    printk(KERN_INFO "%x : %x on %s\n", event->event_code, event->umask, event->name);
    habox->event = event;

    if (pcicfg_box_read_dword(habox->box,
                              HA_pairs[pairnr].controller,
                              &cl) != YEAH)
        return -1;
    cl &= 0xffff0000;
    cl |= ((uint32_t)event->umask << 8) | event->event_code;
    printk(KERN_INFO "event is %x\n", cl);
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
