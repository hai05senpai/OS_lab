/*
 * Copyright (C) 2025 pdnguyen of HCMC University of Technology VNU-HCM
 */

/* Sierra release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 */

// #ifdef MM_PAGING
/*
 * System Library
 * Memory Module Library libmem.c 
 */

#include "string.h"
#include "mm.h"
#include "syscall.h"
#include "libmem.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>


static pthread_mutex_t mmvm_lock = PTHREAD_MUTEX_INITIALIZER;

/*enlist_vm_freerg_list - add new rg to freerg_list
 *@mm: memory region
 *@rg_elmt: new region
 *
 */
int enlist_vm_freerg_list(struct mm_struct *mm, struct vm_rg_struct *rg_elmt)
{
  struct vm_rg_struct *rg_node = mm->mmap->vm_freerg_list;

  if (rg_elmt->rg_start >= rg_elmt->rg_end)
    return -1;

  if (rg_node != NULL)
    rg_elmt->rg_next = rg_node;

  /* Enlist the new region */
  mm->mmap->vm_freerg_list = rg_elmt;

  return 0;
}

/*get_symrg_byid - get mem region by region ID
 *@mm: memory region
 *@rgid: region ID act as symbol index of variable
 *
 */
struct vm_rg_struct *get_symrg_byid(struct mm_struct *mm, int rgid)
{
  if (rgid < 0 || rgid > PAGING_MAX_SYMTBL_SZ)
    return NULL;

  return &mm->symrgtbl[rgid];
}

/*__alloc - allocate a region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size
 *@alloc_addr: address of allocated memory region
 *
 */
int __alloc(struct pcb_t *caller, int vmaid, int rgid, int size, int *alloc_addr)
{
    //TODO
    struct vm_rg_struct rgnode;

    // Tìm vùng nhớ trống trong danh sách freerg_list
    if (get_free_vmrg_area(caller, vmaid, size, &rgnode) == 0) {
        // Cập nhật bảng ký hiệu với vùng nhớ được cấp phát
        caller->mm->symrgtbl[rgid].rg_start = rgnode.rg_start;
        caller->mm->symrgtbl[rgid].rg_end = rgnode.rg_end;

        *alloc_addr = rgnode.rg_start;

        return 0; // Thành công
    }

    // Nếu không tìm thấy vùng nhớ trống, xử lý mở rộng vùng nhớ
    struct vm_area_struct *current_vma = get_vma_by_num(caller->mm, vmaid);
    if (current_vma == NULL) {
        printf("Error: Invalid VM area ID %d.\n", vmaid);
        return -1; // Lỗi: Không tìm thấy vùng bộ nhớ ảo
    }

    int increase_size = PAGING_PAGE_ALIGNSZ(size);
    int past_sbrk = current_vma->sbrk;

    // Mở rộng vùng nhớ
    if (inc_vma_limit(caller, vmaid, increase_size) == 0) {
        // Cập nhật danh sách vùng nhớ trống
        if (increase_size > size) {
            struct vm_rg_struct *new_rgnode = malloc(sizeof(struct vm_rg_struct));
            if (new_rgnode == NULL) {
                printf("Error: Memory allocation failed for free region node.\n");
                return -1; // Lỗi: Không thể cấp phát bộ nhớ
            }

            new_rgnode->rg_start = past_sbrk + size + 1;
            new_rgnode->rg_end = past_sbrk + increase_size;

            enlist_vm_freerg_list(caller->mm, new_rgnode);
        }

        // Cập nhật sbrk và bảng ký hiệu
        current_vma->sbrk += increase_size;
        caller->mm->symrgtbl[rgid].rg_start = past_sbrk;
        caller->mm->symrgtbl[rgid].rg_end = past_sbrk + size;
        *alloc_addr = past_sbrk;

        printf("===== PHYSICAL MEMORY AFTER ALLOCATION =====\n");
        printf("PID=%d - Region=%d - Address=%08x - Size=%d byte\n", 
              caller->pid, rgid, *alloc_addr, size);
        print_pgtbl(caller, 0, -1); // In bảng trang

        // Duyệt qua bảng trang và in thông tin ánh xạ Page Number -> Frame Number
        struct mm_struct *mm = caller->mm;
        for (int pgn = 0; pgn < PAGING_MAX_PGN; pgn++) {
            uint32_t pte = mm->pgd[pgn];
            if (PAGING_PAGE_PRESENT(pte)) {
                int fpn = GETVAL(pte, PAGING_PTE_FPN_MASK, PAGING_PTE_FPN_LOBIT);
                printf("Page Number: %d -> Frame Number: %d\n", pgn, fpn);
            }
        }

        MEMPHY_dump(caller->mram);  // In trạng thái bộ nhớ vật lý
        printf("================================================================\n");
        return 0; // Thành công
    }

    return 0; 
}

/*__free - remove a region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size
 *
 */
int __free(struct pcb_t *caller, int vmaid, int rgid)
{
  //struct vm_rg_struct rgnode;

  if(rgid < 0 || rgid > PAGING_MAX_SYMTBL_SZ)
    return -1;
  
 

  /* TODO: Manage the collect freed region to freerg_list */
  
  // rgnode = ;
  /*enlist the obsoleted memory region */

  enlist_vm_freerg_list(caller->mm, &caller->mm->symrgtbl[rgid]);
  printf("===== PHYSICAL MEMORY AFTER DEALLOCATION =====\n");
  printf("PID=%d - Region=%d\n", caller->pid, rgid);
  print_pgtbl(caller, 0, -1); // In bảng trang

  // Duyệt qua bảng trang và in thông tin ánh xạ Page Number -> Frame Number
  struct mm_struct *mm = caller->mm;
  for (int pgn = 0; pgn < PAGING_MAX_PGN; pgn++) {
      uint32_t pte = mm->pgd[pgn];
      if (PAGING_PAGE_PRESENT(pte)) {
          int fpn = GETVAL(pte, PAGING_PTE_FPN_MASK, PAGING_PTE_FPN_LOBIT);
          printf("Page Number: %d -> Frame Number: %d\n", pgn, fpn);
      }
  }

  MEMPHY_dump(caller->mram);  // In trạng thái bộ nhớ vật lý
  printf("================================================================\n");
  return 0;
}

/*liballoc - PAGING-based allocate a region memory
 *@proc:  Process executing the instruction
 *@size: allocated size
 *@reg_index: memory region ID (used to identify variable in symbole table)
 */
int liballoc(struct pcb_t *proc, uint32_t size, uint32_t reg_index)
{
  /* TODO Implement allocation on vm area 0 */
  int addr;

  /* By default using vmaid = 0 */
  return __alloc(proc, 0, reg_index, size, &addr);
}

/*libfree - PAGING-based free a region memory
 *@proc: Process executing the instruction
 *@size: allocated size
 *@reg_index: memory region ID (used to identify variable in symbole table)
 */

 int libfree(struct pcb_t *proc, uint32_t reg_index)
 {
   /* TODO Implement free region */
 
   /* By default using vmaid = 0 */
   return __free(proc, 0, reg_index);
 }

/*pg_getpage - get the page in ram
 *@mm: memory region
 *@pagenum: PGN
 *@framenum: return FPN
 *@caller: caller
 *
 */
int pg_getpage(struct mm_struct *mm, int pgn, int *fpn, struct pcb_t *caller)
{
  uint32_t pte = mm->pgd[pgn];
  // printf("pte: %d\n", pte);
  if (pte < 0) {
    printf("Error: Invalid page table entry for page %d.\n", pgn);
    return -1;
  }
  
  if (!PAGING_PAGE_PRESENT(pte))
  { 
    int vicpgn, swpfpn; 
    int vicfpn;
    uint32_t vicpte;

    int target_fpn = GETVAL(pte, PAGING_PTE_SWPOFF_MASK,PAGING_PTE_SWPOFF_LOBIT);
    /* TODO: Play with your paging theory here */
    find_victim_page(caller->mm, &vicpgn);

    vicpte = mm->pgd[vicpgn];
    vicfpn = GETVAL(vicpte, PAGING_PTE_FPN_MASK,PAGING_PTE_FPN_LOBIT);

    MEMPHY_get_freefp(caller->active_mswp, &swpfpn);
    // Hoán đổi trang
    __swap_cp_page(caller->mram, vicfpn, caller->active_mswp, swpfpn);
    __swap_cp_page(caller->active_mswp, target_fpn, caller->mram, vicfpn);
    MEMPHY_put_freefp(caller->active_mswp, target_fpn); 

    pte_set_swap(&mm->pgd[vicpgn], 0, swpfpn);
    pte_set_fpn(&mm->pgd[pgn], vicfpn);

    enlist_pgn_node(&caller->mm->fifo_pgn,pgn);

    *fpn = target_fpn;
    return 0;
  }
  *fpn = GETVAL(pte, PAGING_PTE_FPN_MASK,PAGING_PTE_FPN_LOBIT);
  return 0;
}

/*pg_getval - read value at given offset
 *@mm: memory region
 *@addr: virtual address to acess
 *@value: value
 *
 */
int pg_getval(struct mm_struct *mm, int addr, BYTE *data, struct pcb_t *caller)
{
  int pgn = PAGING_PGN(addr);
  int off = PAGING_OFFST(addr);
  int fpn;

  
  if (pg_getpage(mm, pgn, &fpn, caller) != 0) {
      return -1; 
  }



  int phyaddr = (fpn << PAGING_ADDR_FPN_LOBIT) + off;

  MEMPHY_read(caller->mram,phyaddr, data);

  return 0;
}

/*pg_setval - write value to given offset
 *@mm: memory region
 *@addr: virtual address to acess
 *@value: value
 *
 */
int pg_setval(struct mm_struct *mm, int addr, BYTE value, struct pcb_t *caller)
{
  int pgn = PAGING_PGN(addr);
  int off = PAGING_OFFST(addr);
  int fpn;

  // Đảm bảo trang có mặt trong RAM, hoán đổi từ swap nếu cần
  if (pg_getpage(mm, pgn, &fpn, caller) != 0) {
      return -1; // Lỗi: Không thể truy cập trang
  }

  int phyaddr = (fpn << PAGING_ADDR_FPN_LOBIT) + off;

  if (MEMPHY_write(caller->mram, phyaddr, value) != 0) {
      return -1; // Lỗi: Không thể ghi dữ liệu
  }

   return 0;
}

/*__read - read value in region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@offset: offset to acess in memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size
 *
 */
int __read(struct pcb_t *caller, int vmaid, int rgid, int offset, BYTE *data)
{
  struct vm_rg_struct *currg = get_symrg_byid(caller->mm, rgid);
  struct vm_area_struct *current_vma = get_vma_by_num(caller->mm, vmaid);

  if (currg == NULL || current_vma == NULL) /* Invalid memory identify */
    return -1;

  pg_getval(caller->mm, currg->rg_start + offset, data, caller);

  return 0;
}

/*libread - PAGING-based read a region memory */
int libread(
    struct pcb_t *proc, // Process executing the instruction
    uint32_t source,    // Index of source register
    uint32_t offset,    // Source address = [source] + [offset]
    uint32_t* destination)
{
  BYTE data;
  int val = __read(proc, 0, source, offset, &data);

  /* TODO update result of reading action*/
  if (val != 0) {
      printf("libread: Failed to read memory at region %u with offset %u.\n", source, offset);
      return -1; // Lỗi: Không thể đọc bộ nhớ
  }

  // Cập nhật giá trị đọc được vào biến đích
  *destination = (uint32_t)data;

  #ifdef IODUMP
  printf("libread: Read region=%d offset=%d value=%d\n", source, offset, data);
  #ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1); // In bảng trang
  #endif
  MEMPHY_dump(proc->mram); // In trạng thái bộ nhớ vật lý
  #endif

  /* In thông tin sau khi đọc */
  printf("===== PHYSICAL MEMORY AFTER READING =====\n");
  printf("read region=%d offset=%d value=%d\n", source, offset, data);
  print_pgtbl(proc, 0, -1); // In bảng trang

  // Duyệt qua bảng trang và in thông tin ánh xạ Page Number -> Frame Number
  struct mm_struct *mm = proc->mm;
  for (int pgn = 0; pgn < PAGING_MAX_PGN; pgn++) {
      uint32_t pte = mm->pgd[pgn];
      if (PAGING_PAGE_PRESENT(pte)) {
          int fpn = GETVAL(pte, PAGING_PTE_FPN_MASK, PAGING_PTE_FPN_LOBIT);
          printf("Page Number: %d -> Frame Number: %d\n", pgn, fpn);
      }
  }

  MEMPHY_dump(proc->mram);  // In trạng thái bộ nhớ vật lý
  printf("================================================================\n");

  return 0; // Thành công
}

/*__write - write a region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@offset: offset to acess in memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size
 *
 */
int __write(struct pcb_t *caller, int vmaid, int rgid, int offset, BYTE value)
{
  struct vm_rg_struct *currg = get_symrg_byid(caller->mm, rgid);
  struct vm_area_struct *current_vma = get_vma_by_num(caller->mm, vmaid);

  if (currg == NULL || current_vma == NULL) /* Invalid memory identify */
    return -1;

  pg_setval(caller->mm, currg->rg_start + offset, value, caller);

  printf("===== PHYSICAL MEMORY AFTER WRITING =====\n");
  printf("write region=%d offset=%d value=%d\n", rgid, offset, value);
  print_pgtbl(caller, 0, -1); // In bảng trang

  // Duyệt qua bảng trang và in thông tin ánh xạ Page Number -> Frame Number
  struct mm_struct *mm = caller->mm;
  for (int pgn = 0; pgn < PAGING_MAX_PGN; pgn++) {
      uint32_t pte = mm->pgd[pgn];
      if (PAGING_PAGE_PRESENT(pte)) {
          int fpn = GETVAL(pte, PAGING_PTE_FPN_MASK, PAGING_PTE_FPN_LOBIT);
          printf("Page Number: %d -> Frame Number: %d\n", pgn, fpn);
      }
  }

  MEMPHY_dump(caller->mram);  // In trạng thái bộ nhớ vật lý
  printf("================================================================\n");

  return 0;
}

/*libwrite - PAGING-based write a region memory */
int libwrite(
    struct pcb_t *proc,   // Process executing the instruction
    BYTE data,            // Data to be wrttien into memory
    uint32_t destination, // Index of destination register
    uint32_t offset)
{
#ifdef IODUMP
  printf("write region=%d offset=%d value=%d\n", destination, offset, data);
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1); //print max TBL
#endif
  MEMPHY_dump(proc->mram);
#endif

  return __write(proc, 0, destination, offset, data);
}

/*free_pcb_memphy - collect all memphy of pcb
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@incpgnum: number of page
 */
int free_pcb_memph(struct pcb_t *caller)
{
  int pagenum, fpn;
  uint32_t pte;


  for(pagenum = 0; pagenum < PAGING_MAX_PGN; pagenum++)
  {
    pte= caller->mm->pgd[pagenum];

    if (!PAGING_PAGE_PRESENT(pte))
    {
      fpn = PAGING_PTE_FPN(pte);
      MEMPHY_put_freefp(caller->mram, fpn);
    } else {
      fpn = PAGING_PTE_SWP(pte);
      MEMPHY_put_freefp(caller->active_mswp, fpn);    
    }
  }

  return 0;
}


/*find_victim_page - find victim page
 *@caller: caller
 *@pgn: return page number
 *
 */
int find_victim_page(struct mm_struct *mm, int *retpgn)
{
  ///TODO
  struct pgn_t *page = mm->fifo_pgn;
  if(page == NULL) return -1;
  if(page->pg_next == NULL){  
    *retpgn = page->pgn;
    free(page);
    return 0;
  }
  
  struct pgn_t *previous_page = mm->fifo_pgn; 

  /* TODO: Implement the theorical mechanism to find the victim page */

  while(page->pg_next != NULL) {
    previous_page = page;
    page = page->pg_next;
  }
  *retpgn = page->pgn;
  previous_page->pg_next = NULL; 
  free(page);
  
  return 0; 
}

/*get_free_vmrg_area - get a free vm region
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@size: allocated size
 *
 */
int get_free_vmrg_area(struct pcb_t *caller, int vmaid, int size, struct vm_rg_struct *newrg)
{
    //TODO
    struct vm_area_struct *current_vma = get_vma_by_num(caller->mm, vmaid);

    struct vm_rg_struct *rgit = current_vma->vm_freerg_list;
    if (rgit == NULL) {
        return -1; // Không có vùng nhớ trống
    }

    /* Khởi tạo newrg */
    newrg->rg_start = newrg->rg_end = -1;

    /* Duyệt danh sách freerg_list để tìm vùng nhớ trống */
    while (rgit != NULL) {
        if (rgit->rg_start + size <= rgit->rg_end) {
            newrg->rg_start = rgit->rg_start;
            newrg->rg_end = rgit->rg_start + size;

            /* Cập nhật vùng nhớ còn dư */
            if (rgit->rg_start + size < rgit->rg_end) {
                rgit->rg_start += size;
            } else {
                /* Sử dụng hết vùng nhớ, xóa node hiện tại */
                struct vm_rg_struct *nextrg = rgit->rg_next;
                if (nextrg != NULL) {
                    rgit->rg_start = nextrg->rg_start;
                    rgit->rg_end = nextrg->rg_end;
                    rgit->rg_next = nextrg->rg_next;
                    free(nextrg);
                } else {
                    rgit->rg_start = rgit->rg_end; // dummy, size 0 region
                    rgit->rg_next = NULL;
                }
            }
            return 0; // Thành công
        }
        rgit = rgit->rg_next; // Duyệt tiếp
    }

    if(newrg->rg_start == -1) // new region not found
      return -1;

    return 0;
}



//#endif
