#ifndef _LAYOUTS_H_
#define _LAYOUTS_H_

#define SYSCORE 1

#define PGSHIFT 12
#define PGSIZE (1 << PGSHIFT)

#define SBIBASE 0x80000000

#define KERNOFF 0x200000
#define KERNBASE (SBIBASE + KERNOFF)
#define KERNENTRY KERNBASE

#define KSTKSHIFT 4
#define KSTKSIZE (PGSIZE << KSTKSHIFT)

#define KALLOCSIZE (PGSIZE << 10)

#define VA_TOP 0x8000000000
#define MMIOBASE (VA_TOP - 0x5000000000)

#define USTKSIZE_DEFAULT PGSIZE
#define USTKLIMIT (MMIOBASE - 0x100000000)

#define USERBASE 0x400000

#endif
