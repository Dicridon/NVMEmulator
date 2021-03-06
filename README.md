## Homemade NVM emulator
This is a simple NVM emulator working on Intel Xeon E5 v4 and E7 v4

The key idea for latency emulation is to count memory access number and
send a function to interrupt the process that is performing memory access
to emulate extra latency.

Accuracy of emulation result is not guaranteed.

## Code
The emulator project is organized as below
- pcicfg.h: describe a PCI configuration space. A PCI cfg space consists of
          domain number, bus nmber, device number and function number and 
          certain byte/word/dword can be accessed with certain offset 
          within the pcicfg space. Usually, we combine device number and
          function number into one byte since device number is between
          0 and 31 while function number is between 0 to 7 (so that is 8 
          bits in total.)

- pcicfg.c: Functions to read and write PCICFG regsiters

- pcibox.h: Encapsulation of pcicfg structure and operations. A pcibox is
          a uncore PMON box within certain PCICFG space.

- pcibox.c: Implementation of pcibox and relating operations.

- instance.h: Definition of HA box and relating operations.

- emulator.c: Implementation of an emulator using functions offered by pcicfg.h

- large_hearder.h: This is combination of pcibox.h, pcibox.c, pcicfg.h, pcicfg.c
                 I really DO NOT want this file, but my Makefile is just not
                 correct and I DO NOT know why. So I aggregate all the header
                 files and object files into one to make the Makefile work.


## P.S.
NOTICE:
    This emulator is not completely developed by myself, my supervisor's 
    student Yizhou Shan developed E5 v3 version, but we failed to port it
    to E5 v4, so I was asked to develope an E5 v4 version. 
    Please refer to https://github.com/lastweek/NVM-Emulator
