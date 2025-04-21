// #ifdef MM_PAGING
/*
 * PAGING based Memory Management
 * Virtual memory module mm/mm-vm.c
 */

#include "string.h"
#include "mm.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

/*get_vma_by_num - get vm area by numID
 *@mm: memory region
 *@vmaid: ID vm area to alloc memory region
 *
 */
struct vm_area_struct *get_vma_by_num(struct mm_struct *mm, int vmaid)
{
  struct vm_area_struct *pvma = mm->mmap;

  if (mm->mmap == NULL)
    return NULL;

  int vmait = pvma->vm_id;

  while (vmait < vmaid)
  {
    if (pvma == NULL)
      return NULL;

    pvma = pvma->vm_next;
    vmait = pvma->vm_id;
  }

  return pvma;
}

int __mm_swap_page(struct pcb_t *caller, int vicfpn , int swpfpn)
{
    __swap_cp_page(caller->mram, vicfpn, caller->active_mswp, swpfpn);
    return 0;
}

/*get_vm_area_node - get vm area for a number of pages
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@incpgnum: number of page
 *@vmastart: vma end
 *@vmaend: vma end
 *
 */
struct vm_rg_struct* get_vm_area_node_at_brk(struct pcb_t *caller, int vmaid, int size, int alignedsz)
{
  struct vm_rg_struct * newrg;
  struct vm_area_struct *current_vma = get_vma_by_num(caller->mm, vmaid);

  newrg = malloc(sizeof(struct vm_rg_struct));

  newrg->rg_start = current_vma->sbrk;
  newrg->rg_end = newrg->rg_start + size;

  return newrg;
}

/*validate_overlap_vm_area
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@vmastart: vma end
 *@vmaend: vma end
 *
 */
int validate_overlap_vm_area(struct pcb_t *caller, int vmaid, int vmastart, int vmaend)
{
  //struct vm_area_struct *vma = caller->mm->mmap;

  /* TODO validate the planned memory area is not overlapped */
  struct vm_area_struct* current_vma = get_vma_by_num(caller->mm, vmaid);


  struct vm_area_struct* vma;
  for (vma = caller->mm->mmap; vma != NULL; vma = vma->vm_next)
  {
    if (vma != current_vma && vma->vm_start != vma->vm_end && OVERLAP(vma->vm_start, vma->vm_end, current_vma->vm_start, (unsigned long) vmaend)) {
      printf("Invalid memory detected!: ");
      printf("[%ld - %ld] and [%ld - %d]\n\n", vma->vm_start, vma->vm_end, current_vma->vm_start, vmaend);
      return -1;
    }
  }
  return 0;
}

/*inc_vma_limit - increase vm area limits to reserve space for new variable
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@inc_sz: increment size
 *
 */
int inc_vma_limit(struct pcb_t *caller, int vmaid, int inc_sz)
{
  //TODO
  int increase_amt = PAGING_PAGE_ALIGNSZ(inc_sz);
  struct vm_area_struct *current_vma = get_vma_by_num(caller->mm, vmaid);
  struct vm_rg_struct * newrg = malloc(sizeof(struct vm_rg_struct));
  struct vm_rg_struct *area = get_vm_area_node_at_brk(caller, vmaid, inc_sz, increase_amt);
  

  int incnumpage =  increase_amt / PAGING_PAGESZ;
  

  int old = current_vma->vm_end;

  if (validate_overlap_vm_area(caller, vmaid, area->rg_start, area->rg_end) < 0) return -1; 

  current_vma->vm_end += inc_sz;
  if (vm_map_ram(caller, area->rg_start, area->rg_end, old, incnumpage , newrg) < 0) return -1; 

  return 0;

}



// #endif
