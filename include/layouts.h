#ifndef _LAYOUTS_H_
#define _LAYOUTS_H_

#define PGSHIFT 12
#define PGSIZE (1 << PGSHIFT)

#define SBIBASE 0x80000000

#define KERNOFF 0x200000
#define KERNBASE (SBIBASE + KERNOFF)
#define KERNENTRY KERNBASE

#define KSTKSHIFT 4
#define KSTKSIZE (PGSIZE << KSTKSHIFT)

#define DEVOFFSET 0xffffffff40000000UL

#endif
