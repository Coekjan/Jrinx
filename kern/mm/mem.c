#include <brpred.h>
#include <endian.h>
#include <kern/drivers/device.h>
#include <kern/lib/debug.h>
#include <kern/lib/errors.h>
#include <kern/lock/lock.h>
#include <kern/lock/spinlock.h>
#include <kern/mm/mem.h>
#include <layouts.h>
#include <lib/string.h>
#include <stddef.h>
#include <stdint.h>

static size_t mem_num;
static uint64_t *mem_addr;
static uint64_t *mem_size;

static unsigned long freemem_base = 0;

static void *bare_alloc(size_t size) {
  if (unlikely(freemem_base == 0)) {
    extern uint8_t kern_end[];
    freemem_base = (unsigned long)kern_end;
  }

  freemem_base = freemem_base / size * size + size;

  void *res = (void *)freemem_base;

  freemem_base += size;

  return res;
}

static void bare_free(void *ptr) {
  fatal("unexpected free %016lx", (unsigned long)ptr);
}

static void mm_print_bytes(unsigned long size, size_t shift) {
  static const char *unit[] = {"B", "KiB", "MiB", "GiB", "TiB"};

  if (size == 0 && shift == 0) {
    printk("0");
    return;
  }

  unsigned long rem = size & ((1UL << 10) - 1);
  unsigned long quo = size >> 10;
  if (quo != 0) {
    mm_print_bytes(quo, shift + 1);
  }
  if (rem != 0) {
    if (quo != 0) {
      printk(" + ");
    }
    printk("%lu %s", rem, unit[shift]);
  }
}

static void mm_print_range(unsigned long addr, unsigned long size, const char *suffix) {
  printk("[%016lx, %016lx) <size: ", addr, addr + size);
  mm_print_bytes(size, 0);
  printk(">");
  if (suffix) {
    printk("%s", suffix);
  } else {
    printk("\n");
  }
}

static long mem_probe(const struct dev_node *node) {
  int reg_found = 0;
  struct dev_node_prop *prop;
  TAILQ_FOREACH (prop, &node->nd_prop_tailq, pr_link) {
    if (strcmp(prop->pr_name, "reg") == 0) {
      reg_found = 1;
      if (prop->pr_len % (sizeof(uint64_t) * 2) != 0) {
        return -KER_DTB_ER;
      }
      mem_num = prop->pr_len / (sizeof(uint64_t) * 2);
      mem_addr = alloc(sizeof(uint64_t) * mem_num);
      mem_size = alloc(sizeof(uint64_t) * mem_num);
      uint64_t *reg_table = (uint64_t *)prop->pr_values;
      for (size_t i = 0; i < mem_num; i++) {
        mem_addr[i] = from_be(reg_table[i * 2]);
        mem_size[i] = from_be(reg_table[i * 2 + 1]);
      }
      break;
    }
  }

  if (!reg_found) {
    return -KER_DTB_ER;
  }

  return KER_SUCCESS;
}

struct device memory_device = {
    .d_name = "memory",
    .d_probe = mem_probe,
    .d_probe_pri = HIGHEST,
};

void *(*alloc)(size_t size) = bare_alloc;
void (*free)(void *ptr) = bare_free;

static struct phy_frame_list *pf_free_list;
static with_spinlock(pf_free_list);
static struct phy_frame **pf_array;
static size_t *pf_array_len;

void memory_init(void) {
  info("%lu memory probed\n", mem_num);

  for (size_t i = 0; i < mem_num; i++) {
    info("memory[%lu] locates at ", i);
    mm_print_range(mem_addr[i], mem_size[i], NULL);
  }

  pf_free_list = alloc(sizeof(struct phy_frame_list) * mem_num);
  pf_array = alloc(sizeof(struct phy_frame *) * mem_num);
  pf_array_len = alloc(sizeof(size_t) * mem_num);
  for (size_t i = 0; i < mem_num; i++) {
    LIST_INIT(&pf_free_list[i]);
    pf_array_len[i] = mem_size[i] / PGSIZE;
    pf_array[i] = alloc(sizeof(struct phy_frame) * pf_array_len[i]);
    for (size_t j = 0; j < pf_array_len[i]; j++) {
      LIST_INSERT_HEAD(&pf_free_list[i], &pf_array[i][j], pf_link);
    }
  }

  freemem_base = freemem_base / PGSIZE * PGSIZE + PGSIZE;

  info("opensbi & kernel reserve memory at ");
  mm_print_range(SBIBASE, freemem_base - SBIBASE, NULL);

  for (unsigned long rsvaddr = SBIBASE; rsvaddr < freemem_base; rsvaddr += PGSIZE) {
    struct phy_frame *frame;
    panic_e(pa2frame(rsvaddr, &frame));
    LIST_REMOVE(frame, pf_link);
  }
}

long pa2sel(unsigned long addr, unsigned long *sel) {
  for (size_t i = 0; i < mem_num; i++) {
    if (addr >= mem_addr[i] && addr < mem_addr[i] + mem_size[i]) {
      *sel = i;
      return KER_SUCCESS;
    }
  }
  return -KER_MEM_ER;
}

long frame2sel(struct phy_frame *frame, unsigned long *sel) {
  for (size_t i = 0; i < mem_num; i++) {
    if (frame >= pf_array[i] && frame < pf_array[i] + pf_array_len[i]) {
      *sel = i;
      return KER_SUCCESS;
    }
  }
  return -KER_MEM_ER;
}

long pa2frame(unsigned long addr, struct phy_frame **frame) {
  unsigned long sel;
  catch_e(pa2sel(addr, &sel));
  unsigned long off = addr - mem_addr[sel];
  *frame = &pf_array[sel][off / PGSIZE];
  return KER_SUCCESS;
}

long frame2pa(struct phy_frame *frame, unsigned long *addr) {
  unsigned long sel;
  catch_e(frame2sel(frame, &sel));
  unsigned long off = (frame - pf_array[sel]) * PGSIZE;
  *addr = mem_addr[sel] + off;
  return KER_SUCCESS;
}

long phy_frame_alloc(struct phy_frame **frame) {
  catch_e(lk_acquire(&spinlock_of(pf_free_list)));
  for (size_t i = 0; i < mem_num; i++) {
    if (!LIST_EMPTY(&pf_free_list[i])) {
      *frame = LIST_FIRST(&pf_free_list[i]);
      LIST_REMOVE(*frame, pf_link);
      catch_e(lk_release(&spinlock_of(pf_free_list)));
      return KER_SUCCESS;
    }
  }
  catch_e(lk_release(&spinlock_of(pf_free_list)));
  return -KER_MEM_ER;
}

static long phy_frame_free(struct phy_frame *frame) {
  unsigned long sel;
  catch_e(frame2sel(frame, &sel));
  catch_e(lk_acquire(&spinlock_of(pf_free_list)));
  LIST_INSERT_HEAD(&pf_free_list[sel], frame, pf_link);
  catch_e(lk_release(&spinlock_of(pf_free_list)));
  return KER_SUCCESS;
}

static with_spinlock(frame_ref_mod);

long phy_frame_ref_desc(struct phy_frame *frame) {
  catch_e(lk_acquire(&spinlock_of(frame_ref_mod)));
  if (frame->pf_ref == 0) {
    catch_e(lk_release(&spinlock_of(frame_ref_mod)));
    return -KER_MEM_ER;
  }
  frame->pf_ref--;
  if (frame->pf_ref == 0) {
    catch_e(phy_frame_free(frame), {
      catch_e(lk_release(&spinlock_of(frame_ref_mod)));
      return err;
    });
  }
  catch_e(lk_release(&spinlock_of(frame_ref_mod)));
  return KER_SUCCESS;
}

long phy_frame_ref_asc(struct phy_frame *frame) {
  catch_e(lk_acquire(&spinlock_of(frame_ref_mod)));
  frame->pf_ref++;
  catch_e(lk_release(&spinlock_of(frame_ref_mod)));
  return KER_SUCCESS;
}
