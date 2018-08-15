#include "common.h"
#include "pcicfg.h"

/* 
   Construct a pcicfg struct given domain, bus number, device number and funciton
   number
*/
pcicfg_t *get_pcicfg(int domain, int busnr, int device, int fn) {

    pcicfg_t *pcicfg = (pcicfg_t*)kmalloc(sizeof(pcicfg_t), GFP_KERNEL);
    struct pci_bus *bus = NULL;
    enter();
    if (!pcicfg) {
        printk(KERN_ERR "No memory for a pcicfg!!!\n");
        leave();
        return NULL;
    }
    
    pcicfg->inited = 0;
    
    if (domain < 0 || domain > 0xffff) {
        printk(KERN_ERR "domain is not valid\n");
        kfree(pcicfg);
        leave();
        return NULL;
    }

    if (busnr < 0) {
        printk(KERN_ERR "busnr is not valid\n");
        kfree(pcicfg);
        leave();
        return NULL;
    }

    if (device < 0 || device > 31) {
        printk(KERN_ERR "device number is not valid\n");
        kfree(pcicfg);
        leave();
        return NULL;
    }

    if (fn < 0 || fn > 7) {
        printk(KERN_ERR "function number is not valid\n");
        kfree(pcicfg);
        leave();
        return NULL;
    }
    
    bus = pci_find_bus(domain, busnr);
    if (!bus) {
        printk(KERN_ERR "bus not found\n");
        kfree(pcicfg);
        leave();
        return NULL;
    } else {
        pcicfg->domain = domain;
        pcicfg->bus = bus;
        pcicfg->device = device;
        pcicfg->function = fn;
        pcicfg->inited = INITED;
    }
    leave();
    return pcicfg;
}

static int inited(pcicfg_t *pcicfg) {
    enter();
    if (!pcicfg || pcicfg->inited != INITED) {
        printk(KERN_WARNING "This pcicfg is not initialized by init_pcicfg!\n");
        leave();
        return -1;
    }
    leave();
    return 1;
}



/*
  Generally, device number is 5-bit wide and function 3-bit wide
  Linux combined them into one byte.
*/
int pcicfg_read_byte(pcicfg_t *pcicfg, int where, uint8_t *val) {
    unsigned int devfn = 0;
    enter();
    if (!inited(pcicfg) || !val) {
        leave();
        return -PCI_READ_FAILED;
    }
    devfn = (pcicfg->device << 3) | (pcicfg->function);
    leave();
    return pci_bus_read_config_byte(pcicfg->bus, devfn, where, val);
}

int pcicfg_read_word(pcicfg_t *pcicfg, int where, uint16_t *val) {
    unsigned int devfn = 0;
    enter();
    if (!inited(pcicfg) || !val) {
        leave();
        return -PCI_READ_FAILED;
    }
    devfn = (pcicfg->device << 3) | (pcicfg->function);
    leave();
    return pci_bus_read_config_word(pcicfg->bus, devfn, where, val);
}

int pcicfg_read_dword(pcicfg_t *pcicfg, int where, uint32_t *val) {
    unsigned int devfn = 0;
    enter();
    if (!inited(pcicfg) || !val) {
        leave();
        return -PCI_READ_FAILED;
    }
    devfn = (pcicfg->device << 3) | (pcicfg->function);
    leave();
    return pci_bus_read_config_dword(pcicfg->bus, devfn, where, val);
}

/*
  Some counters may consist of two 32-bits registers, if so, 
  @where should be address of the low 32-bit register
*/
int pcicfg_read_qword(pcicfg_t *pcicfg, int where, uint64_t *val) {
    unsigned int devfn = 0;
    uint32_t temp = 0;
    enter();
    if (!inited(pcicfg) || !val) {
        leave();
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
        leave();
        return -PCI_READ_FAILED;
    }
    *val |= temp;
    *val = (*val) << 32;
    if (pci_bus_read_config_dword(pcicfg->bus,
                              devfn,
                              where,
                              &temp) != PCIBIOS_SUCCESSFUL) {
        printk(KERN_WARNING "read higher 32 bits failed\n");
        leave();
        return -PCI_READ_FAILED;
    }
    *val |= temp;
    leave();
    return YEAH;
}


int pcicfg_write_byte(pcicfg_t *pcicfg, int where, uint8_t val) {
    unsigned int devfn = 0;
    enter();
    if (!inited(pcicfg)) {
        leave();
        return -PCI_WRITE_FAILED;
    }
    devfn = (pcicfg->device << 3) | (pcicfg->function);
    leave();
    return pci_bus_write_config_byte(pcicfg->bus, devfn, where, val);
}

int pcicfg_write_word(pcicfg_t *pcicfg, int where, uint16_t val) {
    unsigned int devfn = 0;
    enter();
    if (!inited(pcicfg)) {
        return -PCI_WRITE_FAILED;
    }
    devfn = (pcicfg->device << 3) | (pcicfg->function);
    leave();
    return pci_bus_write_config_word(pcicfg->bus, devfn, where, val);
}

int pcicfg_write_dword(pcicfg_t *pcicfg, int where, uint32_t val) {
    unsigned int devfn = 0;
    enter();
    if (!inited(pcicfg)) {
        leave();
        return -PCI_WRITE_FAILED;
    }
    devfn = (pcicfg->device << 3) | (pcicfg->function);
    leave();
    return pci_bus_write_config_dword(pcicfg->bus, devfn, where, val);
}

int pcicfg_write_qword(pcicfg_t *pcicfg, int where, uint64_t val) {
    unsigned int devfn = 0;
    uint32_t temp = 0;
    enter();
    if (!inited(pcicfg)) {
        leave();
        return -PCI_WRITE_FAILED;
    }
    devfn = (pcicfg->device << 3) | (pcicfg->function);
    temp = (uint32_t)val;
    if (pci_bus_write_config_dword(pcicfg->bus,
                               devfn,
                               where,
                               temp) != PCIBIOS_SUCCESSFUL) {
        printk(KERN_WARNING "write lower 32 bits failed\n");
        leave();
        return -PCI_WRITE_FAILED;
    }
    temp = val >> 32;
    if (pci_bus_write_config_dword(pcicfg->bus,
                               devfn,
                               where + 4,
                               temp) != PCIBIOS_SUCCESSFUL) {
        printk(KERN_WARNING "write higher 32 bits failed\n");
        leave();
        return -PCI_WRITE_FAILED;
    }
    return YEAH;
}

void pcicfg_free(pcicfg_t *pcicfg) {
    enter();
    if (!pcicfg) {
        return;
        leave();
    }
    kfree(pcicfg);
    pcicfg = NULL;
    leave();
}
