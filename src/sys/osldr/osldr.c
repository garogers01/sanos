//
// osldr.c
//
// Operating system loader
//
// Copyright (C) 2002 Michael Ringgaard. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 
// 1. Redistributions of source code must retain the above copyright 
//    notice, this list of conditions and the following disclaimer.  
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.  
// 3. Neither the name of the project nor the names of its contributors
//    may be used to endorse or promote products derived from this software
//    without specific prior written permission. 
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
// OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF 
// SUCH DAMAGE.
// 

#include <os.h>

#include <os/pdir.h>
#include <os/tss.h>
#include <os/seg.h>
#include <os/syspage.h>
#include <os/timer.h>
#include <os/object.h>
#include <os/sched.h>

void kprintf(const char *fmt,...);
void clear_screen();
void init_video();

#define VIDEO_BASE           0xB8000
#define HEAP_START           (1 * M)

unsigned long mem_end;          // Size of memory
char *heap;                     // Heap pointer point to next free page in memory
pte_t *pdir;                    // Page directory
struct syspage *syspage;        // System page with tss and gdt
pte_t *syspagetable;            // System page table
struct tcb *inittcb;            // Initial thread control block for kernel
unsigned long krnlentry;        // Virtual address of kernel entry point
unsigned short tssval;          // TSS selector value
unsigned short ldtnull;         // LDT null selector
struct selector gdtsel;         // GDT selector
struct selector idtsel;         // IDT selector
int bootdrive;                  // Boot drive
int bootpart;                   // Boot partition

void load_kernel();

void init_bootfd(int bootdrv);
void uninit_bootfd();
int boothd_read(void *buffer, size_t count, blkno_t blkno);

void init_boothd(int bootdrv);
void uninit_boothd();
int bootfd_read(void *buffer, size_t count, blkno_t blkno);

int boot_read(void *buffer, size_t count, blkno_t blkno)
{
  if (bootdrive & 0x80)
    return boothd_read(buffer, count, blkno);
  else
    return bootfd_read(buffer, count, blkno);
}

void panic(char *msg)
{
  kprintf("Panic: %s\n", msg);
  while (1);
}

char *alloc_heap(int numpages)
{
  char *p;

  // Check for out of memory
  if ((unsigned long) heap + numpages * PAGESIZE >= mem_end) panic("out of memory");
  p = heap;

  // Zero allocated pages
  memset(p, 0, numpages * PAGESIZE);

  // Update heap pointer
  heap += numpages * PAGESIZE;

  return p;
}

unsigned long memsize()
{
  volatile unsigned long *mem;
  unsigned long addr;
  unsigned long value;
  unsigned long cr0save;
  unsigned long cr0new;

  // Start at 1MB
  addr = 1 * M;

  // Save a copy of CR0
  __asm { mov eax, cr0 };
  __asm { mov [cr0save], eax };

  // Invalidate the cache (write-back and invalidate the cache)
  __asm { wbinvd };

  // Plug cr0 with just PE/CD/NW (cache disable(486+), no-writeback(486+), 32bit mode(386+))
  cr0new = cr0save | 0x00000001 | 0x40000000 | 0x20000000;
  __asm { mov eax, [cr0new] };
  __asm { mov cr0, eax };

  // Probe for each megabyte
  while (addr < 0xFFF0000000000)
  {
    addr += 1 * M;
    mem= (unsigned long *) addr;

    value = *mem;
    *mem = 0x55AA55AA;

    if (*mem != 0x55AA55AA) break;

    *mem = 0xAA55AA55;
    if(*mem != 0xAA55AA55) break;

    *mem = value;
  }

  // Restore 
  __asm { mov eax, [cr0save] };
  __asm { mov cr0, eax };

  return addr;
}
	
void setup_descriptors()
{
  struct syspage *syspage;
  struct tss *tss;
  //struct segment *seg;

  // Get syspage virtual address
  syspage = (struct syspage *) SYSPAGE_ADDRESS;

  // Initialize tss
  tss = &syspage->tss;
  tss->cr3 = (unsigned long) pdir;
  tss->eip = krnlentry;
  tss->cs = SEL_KTEXT;
  tss->ss0 =  SEL_KDATA;
  tss->ds =  SEL_KDATA;
  tss->es =  SEL_KDATA;
  tss->ss = SEL_KDATA;
  tss->esp = INITTCB_ADDRESS + TCBESP;
  tss->esp0 = INITTCB_ADDRESS + TCBESP;

  // Setup kernel text segment (4GB read and execute, ring 0)
  seginit(&syspage->gdt[GDT_KTEXT], 0, 0x100000, D_CODE | D_DPL0 | D_READ | D_PRESENT, D_BIG | D_BIG_LIM);

  // Setup kernel data segment (4GB read and write, ring 0)
  seginit(&syspage->gdt[GDT_KDATA], 0, 0x100000, D_DATA | D_DPL0 | D_WRITE | D_PRESENT, D_BIG | D_BIG_LIM);

  // Setup user text segment (2GB read and execute, ring 3)
  seginit(&syspage->gdt[GDT_UTEXT], 0, BTOP(OSBASE), D_CODE | D_DPL3 | D_READ | D_PRESENT, D_BIG | D_BIG_LIM);

  // Setup user data segment (2GB read and write, ring 3)
  seginit(&syspage->gdt[GDT_UDATA], 0, BTOP(OSBASE), D_DATA | D_DPL3 | D_WRITE | D_PRESENT, D_BIG | D_BIG_LIM);

  // Setup TSS segment
  seginit(&syspage->gdt[GDT_TSS], (unsigned long) &syspage->tss, sizeof(struct tss), D_TSS | D_DPL0 | D_PRESENT, 0);

  // Setup TIB segment
  seginit(&syspage->gdt[GDT_TIB], 0, PAGESIZE, D_DATA | D_DPL3 | D_WRITE | D_PRESENT, 0);

  // Set GDT to the new segment descriptors
  gdtsel.limit = (sizeof(struct segment) * MAXGDT) - 1;
  gdtsel.dt = syspage->gdt;
  __asm { lgdt gdtsel }

  // Make all LDT selectors invalid
  ldtnull = 0;
  __asm { lldt [ldtnull] };

  // Set IDT to IDT in syspage
  idtsel.limit = (sizeof(struct gate) * MAXIDT) - 1;
  idtsel.dt = syspage->idt;
  __asm { lidt idtsel }

  // Load task register with new TSS segment
  tssval = SEL_TSS;
  __asm { ltr tssval };
}

void __stdcall start(void *hmod, int bootdrv, int reserved2)
{
  pte_t *pt;
  int i;

  // Initialize video 
  init_video();
  clear_screen();
  kprintf("OSLDR\n\n");

  // Determine size of RAM
  mem_end = memsize();
  //kprintf("Memory size %d MB\n", mem_end / M);

  // Page allocation starts at 1MB
  heap = (char *) HEAP_START;

  // Allocate page for page directory
  pdir = (pte_t *) alloc_heap(1);

  // Make recursive entry for access to page tables
  pdir[PDEIDX(PTBASE)] = (unsigned long) pdir | PT_PRESENT | PT_WRITABLE;

  // Allocate system page
  syspage = (struct syspage *) alloc_heap(1);

  // Allocate initial thread control block
  inittcb = (struct tcb *) alloc_heap(PAGES_PER_TCB);

  // Allocate system page directory page
  syspagetable = (pte_t *) alloc_heap(1);

  // Map system page, page directory and and video buffer
  pdir[PDEIDX(SYSBASE)] = (unsigned long) syspagetable | PT_PRESENT | PT_WRITABLE;
  syspagetable[PTEIDX(SYSPAGE_ADDRESS)] = (unsigned long) syspage | PT_PRESENT | PT_WRITABLE;
  syspagetable[PTEIDX(PAGEDIR_ADDRESS)] = (unsigned long) pdir | PT_PRESENT | PT_WRITABLE;
  syspagetable[PTEIDX(VIDBASE_ADDRESS)] = VIDEO_BASE | PT_PRESENT | PT_WRITABLE;

  // Map initial TCB
  for (i = 0; i < PAGES_PER_TCB; i++)
  {
    syspagetable[PTEIDX(INITTCB_ADDRESS) + i] = ((unsigned long) inittcb + i * PAGESIZE) | PT_PRESENT | PT_WRITABLE;
  }

  // Map first 4MB to physical memory
  pt = (pte_t *) alloc_heap(1);
  pdir[0] = (unsigned long) pt | PT_PRESENT | PT_WRITABLE | PT_USER;
  for (i = 0; i < PTES_PER_PAGE; i++) pt[i] = (i * PAGESIZE) | PT_PRESENT | PT_WRITABLE;

  // Copy kernel from boot disk
  bootdrive = bootdrv;
  if (bootdrv & 0x80)
  {
    init_boothd(bootdrv);
    load_kernel(bootdrv);
    uninit_boothd();
  }
  else
  {
    init_bootfd(bootdrv);
    load_kernel(bootdrv);
    uninit_bootfd();
  }

  // Set page directory (CR3) enable paging (PG bit in CR0)
  __asm
  {
    mov eax, pdir
    mov cr3, eax
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax
  }

  // Setup new descriptors in syspage
  setup_descriptors();

  // Setup boot parameters
  syspage->bootparams.heapstart = HEAP_START;
  syspage->bootparams.heapend = (unsigned long) heap;
  syspage->bootparams.memend = mem_end;
  syspage->bootparams.bootdrv = bootdrv;
  syspage->bootparams.bootpart = bootpart;
  memcpy(syspage->biosdata, (void *) 0x0400, 256);

  // Reload segment registers
  __asm
  {
    mov	ax, SEL_KDATA
    mov	ds, ax
    mov	es, ax
    mov	fs, ax
    mov	gs, ax
    mov	ss, ax
    push SEL_KTEXT
    push offset startpg
    retf
    startpg:
  }

  // Switch to inital kernel stack and jump to kernel
  __asm
  {
    mov esp, INITTCB_ADDRESS + TCBESP
    push 0
    push 0
    push OSBASE
    call [krnlentry]
    cli
    hlt
  }
}
