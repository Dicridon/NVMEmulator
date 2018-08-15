#ifndef __PCIBOX__
#define __PCIBOX__

#include "common.h"
#include "pcicfg.h"

/*
  Uncore PMON PCICFG boxes consist of several counters and controllers.
  These counters and controllers are mapped to a PCICFG space, which is described
  in pcicfg.h.
  But notice that though all boxes have box-level control register, box-level
  status register and counter-controller pairs, different boxes may contain some
  specialized registers.
  Thus this header only describe basic register access operations. 
*/

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
#endif
