#ifndef _KERN_MM_PMM_H_
#define _KERN_MM_PMM_H_

#include <list.h>
#include <stddef.h>

struct phy_frame {
  unsigned long pf_ref;
  struct linked_node pf_link;
};

unsigned long mm_get_freemem_base(void);
void *palloc(size_t size, size_t align);

void pmm_init(void);
long pa2frame(unsigned long addr, struct phy_frame **frame) __attribute__((warn_unused_result));
long frame2pa(struct phy_frame *frame, unsigned long *addr) __attribute__((warn_unused_result));
long phy_frame_alloc(struct phy_frame **frame) __attribute__((warn_unused_result));
long phy_frame_ref_dec(struct phy_frame *frame) __attribute__((warn_unused_result));
long phy_frame_ref_inc(struct phy_frame *frame) __attribute__((warn_unused_result));

#endif
