#include <kern/lib/logger.h>
#include <kern/lib/sbi.h>
#include <layouts.h>
#include <types.h>

void kernel_init(unsigned long hartid, unsigned long opaque) {
  static int is_master = 1;
  static unsigned long hart_table = 0;
  if (is_master) {
    printk("[ hart %ld ] Hello Jrinx, I am master hart!\n", hartid);
    is_master = 0;
    for (int i = 0; i < CONFIG_NR_CORES; i++) {
      if (i != hartid) {
        hart_table |= 1 << i;
        sbi_hart_start(i, KERNBASE, 0);
      }
    }
    sbi_send_ipi(&hart_table);
    while (hart_table) {
    }
    haltk("[ hart %ld ] all cores running!", hartid);
  } else {
    printk("[ hart %ld ] Hello Jrinx, I am slave hart!\n", hartid);
    hart_table &= ~(1 << hartid);
    while (1) {
    }
  }
}
