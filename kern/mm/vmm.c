#include <aligns.h>
#include <kern/drivers/mems.h>
#include <kern/lib/debug.h>
#include <kern/lib/errors.h>
#include <kern/lib/regs.h>
#include <kern/lib/sync.h>
#include <kern/mm/pmm.h>
#include <kern/mm/vmm.h>
#include <layouts.h>
#include <lib/string.h>
#include <stddef.h>

pte_t kern_pgdir[PGSIZE / sizeof(pte_t)] __attribute__((aligned(PGSIZE)));

static void *kalloc(size_t size, size_t align) {
  UNIMPLEMENTED;
}

static void kfree(const void *ptr) {
  UNIMPLEMENTED;
}

static long pt_boot_frame_alloc(paddr_t *pa) {
  void *addr = alloc(PGSIZE, PGSIZE);
  memset(addr, 0, PGSIZE);
  pa->val = (unsigned long)addr;
  return KER_SUCCESS;
}

static long pt_boot_frame_ref_inc(paddr_t pa) {
  return KER_SUCCESS;
}

static long pt_boot_frame_ref_dec(paddr_t pa) {
  return KER_SUCCESS;
}

static long pt_phy_frame_alloc(paddr_t *pa) {
  struct phy_frame *pf;
  catch_e(phy_frame_alloc(&pf));
  panic_e(phy_frame_ref_inc(pf));
  panic_e(frame2pa(pf, &pa->val));
  return KER_SUCCESS;
}

static long pt_phy_frame_ref_inc(paddr_t pa) {
  struct phy_frame *pf;
  catch_e(pa2frame(pa.val, &pf), {
    // pa locates at mmio region
    return KER_SUCCESS;
  });
  panic_e(phy_frame_ref_inc(pf));
  return KER_SUCCESS;
}

static long pt_phy_frame_ref_dec(paddr_t pa) {
  struct phy_frame *pf;
  catch_e(pa2frame(pa.val, &pf), {
    // pa locates at mmio region
    return KER_SUCCESS;
  });
  panic_e(phy_frame_ref_dec(pf));
  return KER_SUCCESS;
}

static long (*pt_frame_alloc)(paddr_t *pa) = pt_boot_frame_alloc;
static long (*pt_frame_ref_inc)(paddr_t pa) = pt_boot_frame_ref_inc;
static long (*pt_frame_ref_dec)(paddr_t pa) = pt_boot_frame_ref_dec;

static long pt_walk(pte_t *pgdir, vaddr_t va, int create, pte_t **pte) {
  paddr_t pa1 = {.val = 0};
  paddr_t pa2 = {.val = 0};
  pte_t *ptr = pgdir + va.bits.vpn2;
  if (!ptr->bits.v) {
    if (!create) {
      *pte = NULL;
      return KER_SUCCESS;
    }
    catch_e(pt_frame_alloc(&pa1));
    ptr->val = 0;
    ptr->bits.v = 1;
    ptr->pp.ppn = pa1.pp.ppn;
  }

  ptr = (pte_t *)pte2pa(*ptr).val + va.bits.vpn1;
  if (!ptr->bits.v) {
    if (!create) {
      *pte = NULL;
      return KER_SUCCESS;
    }
    catch_e(pt_frame_alloc(&pa2), {
      if (pa1.val != 0) {
        panic_e(pt_frame_ref_dec(pa1));
      }
      return err;
    });
    ptr->val = 0;
    ptr->bits.v = 1;
    ptr->pp.ppn = pa2.pp.ppn;
  }

  *pte = (pte_t *)pte2pa(*ptr).val + va.bits.vpn0;
  return KER_SUCCESS;
}

long pt_lookup(pte_t *pgdir, vaddr_t va, pte_t **res) {
  pte_t *pte;
  catch_e(pt_walk(pgdir, va, 0, &pte));
  if (pte == NULL || !pte->bits.v) {
    *res = NULL;
    return KER_SUCCESS;
  }
  *res = pte;
  return KER_SUCCESS;
}

long pt_unmap(pte_t *pgdir, vaddr_t va) {
  pte_t *pte;
  catch_e(pt_lookup(pgdir, va, &pte));

  if (pte == NULL) {
    return KER_SUCCESS;
  }

  paddr_t pa = pte2pa(*pte);
  catch_e(pt_frame_ref_dec(pa));
  pte->val = 0;
  return KER_SUCCESS;
}

long pt_map(pte_t *pgdir, vaddr_t va, paddr_t pa, perm_t perm) {
  pte_t *pte;
  va.pp.off = 0;
  pa.pp.off = 0;
  catch_e(pt_walk(pgdir, va, 0, &pte));
  if (pte != NULL && pte->bits.v) {
    catch_e(pt_unmap(pgdir, va));
  }
  catch_e(pt_walk(pgdir, va, 1, &pte));
  pte->pp.ppn = pa.pp.ppn;
  perm.bits.a = 1;
  perm.bits.d = 1;
  perm.bits.v = 1;
  pte->pp.perm = perm.val;
  panic_e(pt_frame_ref_inc(pa));
  return KER_SUCCESS;
}

struct mmio_setup {
  char *mm_name;
  unsigned long *mm_addr;
  unsigned long mm_size;
  LIST_ENTRY(mmio_setup) mm_link;
};

static LIST_HEAD(__magic, mmio_setup) vmm_mmio_setup_queue;

void vmm_register_mmio(char *name, unsigned long *addr, unsigned long size) {
  struct mmio_setup *mmio = alloc(sizeof(struct mmio_setup), sizeof(struct mmio_setup));
  mmio->mm_name = name;
  mmio->mm_addr = addr;
  mmio->mm_size = size;
  LIST_INSERT_HEAD(&vmm_mmio_setup_queue, mmio, mm_link);
}

void vmm_setup_mmio(void) {
  struct mmio_setup *mmio;
  LIST_FOREACH (mmio, &vmm_mmio_setup_queue, mm_link) {
    info("set up %s mmio at ", mmio->mm_name);
    mem_print_range(*mmio->mm_addr + DEVOFFSET, mmio->mm_size, NULL);
    vaddr_t va = {.val = *mmio->mm_addr + DEVOFFSET};
    paddr_t pa = {.val = *mmio->mm_addr};
    perm_t perm = {.bits = {.a = 1, .d = 1, .r = 1, .w = 1, .g = 1}};
    for (; pa.val < *mmio->mm_addr + mmio->mm_size; pa.val += PGSIZE, va.val += PGSIZE) {
      panic_e(pt_map(kern_pgdir, va, pa, perm));
    }
    *mmio->mm_addr += DEVOFFSET;
  }
}

void vmm_setup_kern(void) {
  extern uint8_t kern_text_end[];
  vaddr_t va = {.val = KERNBASE};
  paddr_t pa = {.val = KERNBASE};
  size_t text_end = align_up((size_t)kern_text_end, PGSIZE);

  for (; va.val < text_end; va.val += PGSIZE, pa.val += PGSIZE) {
    perm_t perm = {.bits = {.a = 1, .d = 1, .r = 1, .x = 1, .w = 1, .g = 1}};
    panic_e(pt_map(kern_pgdir, va, pa, perm));
  }

  size_t mem_num = mem_get_num();
  for (size_t i = 0; i < mem_num; i++) {
    uint64_t mem_addr;
    uint64_t mem_size;
    panic_e(mem_get_addr(i, &mem_addr));
    panic_e(mem_get_size(i, &mem_size));
    vaddr_t va;
    perm_t perm = {.bits = {.a = 1, .d = 1, .r = 1, .w = 1, .g = 1}};
    if (text_end >= mem_addr && text_end < mem_addr + mem_size) {
      va.val = text_end;
    } else {
      va.val = mem_addr;
    }
    paddr_t pa = {.val = va.val};
    for (; va.val < mem_addr + mem_size; va.val += PGSIZE, pa.val += PGSIZE) {
      panic_e(pt_map(kern_pgdir, va, pa, perm));
    }
  }

  pmm_init();

  alloc = kalloc;
  free = kfree;
  pt_frame_alloc = pt_phy_frame_alloc;
  pt_frame_ref_inc = pt_phy_frame_ref_inc;
  pt_frame_ref_dec = pt_phy_frame_ref_dec;
}

void vmm_start(void) {
  rv64_satp satp_reg = {
      .bits = {.mode = SV39, .asid = 0, .ppn = ((unsigned long)&kern_pgdir) / PGSIZE}};
  csrw_satp(satp_reg.val);
  sfence_vma;
  info("enable virtual memory (satp: %016lx)\n", satp_reg.val);
}

void vmm_summary(void) {
  unsigned long freemem_base = mm_get_freemem_base();
  size_t free_end = align_up((size_t)freemem_base, PGSIZE);
  info("os kernel reserves memory ");
  mem_print_range(KERNBASE, free_end - KERNBASE, NULL);
}
