//
// krnl.h
//
// Main kernel include file
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

#ifndef KRNL_H
#define KRNL_H

#include <os.h>

#include <sys/types.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include <bitops.h>
#include <rmap.h>
#include <inifile.h>
#include <moddb.h>

#include <os/tss.h>
#include <os/seg.h>
#include <os/fpu.h>
#include <os/cpu.h>

#include <os/pdir.h>
#include <os/pframe.h>
#include <os/kmem.h>
#include <os/kmalloc.h>
#include <os/vmm.h>

#include <os/syspage.h>

#include <os/pe.h>

#include <os/buf.h>

#include <os/timer.h>
#include <os/object.h>
#include <os/queue.h>
#include <os/sched.h>
#include <os/trap.h>
#include <os/dbg.h>

#include <os/pic.h>
#include <os/pit.h>

#include <os/dev.h>
#include <os/pci.h>
#include <os/pnpbios.h>

#include <os/video.h>
#include <os/kbd.h>

#include <os/iovec.h>
#include <os/vfs.h>
#include <os/dfs.h>
#include <os/devfs.h>
#include <os/procfs.h>

#include <os/mbr.h>

#include <os/pe.h>
#include <os/ldr.h>

#include <os/syscall.h>

#include <net/net.h>

#if _MSC_VER < 1300
#pragma warning(disable : 4761)
#endif

// start.c

krnlapi extern devno_t bootdev;
krnlapi extern struct section *krnlcfg;
krnlapi extern struct peb *peb;

krnlapi void panic(char *msg);

void exit();
void stop(int restart);

// syscall.c

void init_syscall();

// smbfs.c

void init_smbfs();

// pipefs.c

void init_pipefs();
int pipe(struct file **readpipe, struct file **writepipe);

// cons.c

extern devno_t consdev;

krnlapi void kprintf(const char *fmt, ...);

// hd.c

void init_hd();

// fd.c

void init_fd();

// iop.c

void insw(int port, void *buf, int count);
void outsw(int port, void *buf, int count);
void insd(int port, void *buf, int count);
void outsd(int port, void *buf, int count);

// Intrinsic i/o functions

int __cdecl _inp(port_t);
unsigned short __cdecl _inpw(port_t);
unsigned long __cdecl _inpd(port_t);

int __cdecl _outp(port_t, int);
unsigned short __cdecl _outpw(port_t, unsigned short);
unsigned long __cdecl _outpd(port_t, unsigned long);

#ifdef DEBUG

#define inp _inp
#define inpw _inpw
#define inpd _inpd
#define outp _outp
#define outpw _outpw
#define outpd _outpd

#endif

#endif
