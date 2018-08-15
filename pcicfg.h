#ifndef __PCI_CFG__
#define __PCI_CFG__

#include "common.h"

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
#endif
