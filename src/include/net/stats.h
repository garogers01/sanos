//
// stats.h
//
// Network statistics
//
// Copyright (C) 2002 Michael Ringgaard. All rights reserved.
// Portions Copyright (C) 2001, Swedish Institute of Computer Science.
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

#ifndef STATS_H
#define STATS_H

struct stats_proto 
{
  unsigned long xmit;    // Transmitted packets
  unsigned long rexmit;  // Retransmitted packets
  unsigned long recv;    // Received packets
  unsigned long fw;      // Forwarded packets
  unsigned long drop;    // Dropped packets
  unsigned long chkerr;  // Checksum error
  unsigned long lenerr;  // Invalid length error
  unsigned long memerr;  // Out of memory error
  unsigned long rterr;   // Routing error
  unsigned long proterr; // Protocol error
  unsigned long opterr;  // Error in options
  unsigned long err;     // Misc error
};

struct stats_pbuf 
{
  unsigned long avail;
  unsigned long used;
  unsigned long max;  
  unsigned long err;
  unsigned long reclaimed;

  unsigned long alloc_locked;
  unsigned long refresh_locked;

  unsigned long rwbufs;
};

struct stats_all
{
  struct stats_proto link;
  struct stats_proto ip;
  struct stats_proto icmp;
  struct stats_proto udp;
  struct stats_proto tcp;
  struct stats_pbuf pbuf;
};

krnlapi extern struct stats_all stats;

void stats_init();

#endif
