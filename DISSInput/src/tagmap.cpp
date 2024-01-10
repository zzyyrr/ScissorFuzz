/*-
 * Copyright (c) 2010, Columbia University
 * All rights reserved.
 *
 * This software was developed by Vasileios P. Kemerlis <vpk@cs.columbia.edu>
 * at Columbia University, New York, NY, USA, in June 2010.
 *
 * Georgios Portokalidis <porto@cs.columbia.edu> contributed to the
 * optimized implementation of tagmap_setn() and tagmap_clrn()
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Columbia University nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include "tagmap.h"
#include "branch_pred.h"
#include "debug.h"
#include "libdft_api.h"
#include "pin.H"
#include <err.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

tag_dir_t tag_dir;
extern thread_ctx_t *threads_ctx;

inline void tag_dir_setb(tag_dir_t &dir, ADDRINT addr, tag_t const &tag) {
  if (addr > 0x7fffffffffff) {
    return;
  }
  // LOG("Setting tag "+hexstr(addr)+"\n");
  if (dir.table[VIRT2PAGETABLE(addr)] == NULL) {
    //  LOG("No tag table for "+hexstr(addr)+" allocating new table\n");
    tag_table_t *new_table = new (std::nothrow) tag_table_t();
    if (new_table == NULL) {
      LOG("Failed to allocate tag table!\n");
      libdft_die();
    }
    dir.table[VIRT2PAGETABLE(addr)] = new_table;
  }

  tag_table_t *table = dir.table[VIRT2PAGETABLE(addr)];
  if ((*table).page[VIRT2PAGE(addr)] == NULL) {
    //    LOG("No tag page for "+hexstr(addr)+" allocating new page\n");
    tag_page_t *new_page = new (std::nothrow) tag_page_t();
    if (new_page == NULL) {
      LOG("Failed to allocate tag page!\n");
      libdft_die();
    }
    std::fill(new_page->tag, new_page->tag + PAGE_SIZE,
              tag_traits::cleared_val);
    (*table).page[VIRT2PAGE(addr)] = new_page;
  }

  tag_page_t *page = (*table).page[VIRT2PAGE(addr)];
  (*page).tag[VIRT2OFFSET(addr)] = tag;
  /*
  if (!tag_is_empty(tag)) {
    LOGD("[!]Writing tag for %p \n", (void *)addr);
  }
  */
}

inline tag_t const *tag_dir_getb_as_ptr(tag_dir_t const &dir, ADDRINT addr) {
  if (addr > 0x7fffffffffff) {
    return NULL;
  }
  if (dir.table[VIRT2PAGETABLE(addr)]) {
    tag_table_t *table = dir.table[VIRT2PAGETABLE(addr)];
    if ((*table).page[VIRT2PAGE(addr)]) {
      tag_page_t *page = (*table).page[VIRT2PAGE(addr)];
      if (page != NULL)
        return &(*page).tag[VIRT2OFFSET(addr)];
    }
  }
  return &tag_traits::cleared_val;
}

// PIN_FAST_ANALYSIS_CALL
void tagmap_setb(ADDRINT addr, tag_t const &tag) {
  if (tag){
    tag_dir_setb(tag_dir, addr, tag);
  } else {
    tagmap_clrb(addr);
  }
  
}


void tagmap_setb_reg(THREADID tid, unsigned int reg_idx, unsigned int off,
                     tag_t const &tag) {
  threads_ctx[tid].vcpu.gpr[reg_idx][off] = tag;
}

tag_t tagmap_getb(ADDRINT addr) { return *tag_dir_getb_as_ptr(tag_dir, addr); }
/*
tag_t tagmap_getb_reg(THREADID tid, unsigned int reg_idx, unsigned int off) {
  return threads_ctx[tid].vcpu.gpr[reg_idx][off];
}
*/

/*
void PIN_FAST_ANALYSIS_CALL tagmap_clrb(ADDRINT addr) {
  tag_entity* existsTag = tag_get(tagmap_getb(addr));
  if (existsTag != NULL){
    if (existsTag->getLen() == 1){
      tag_dir_setb(tag_dir, addr, tag_traits::cleared_val);
      return;
    }
    ADDRINT firstAddr = getFirstAddr(addr, existsTag->id);
    ADDRINT finalAddr = getFinalAddr(addr, existsTag->id);
    if (addr != firstAddr)
    {
      size_t leftLen = addr - firstAddr;
      tag_entity* newTag = tag_alloc(existsTag->begin, (existsTag->begin + leftLen), existsTag->parent, false);

      LOGD("[cutoff tag left] addr=%p, existsTag=%s, newTag=%s, existsTagAddress[%p,%p), newTagAddress[%p,%p)\n", (void*)addr, tag_sprint(existsTag).c_str() ,tag_sprint(newTag).c_str(), (void*)firstAddr, (void*)finalAddr, (void*)firstAddr, (void*)(firstAddr + leftLen));
      for (ADDRINT i = firstAddr; i < (firstAddr + leftLen); i++){
        tag_dir_setb(tag_dir, i, newTag->id);
      }
    }
    if (addr != (finalAddr - 1))
    {
      size_t rightLen = finalAddr - 1 - addr;
      tag_entity* newTag = tag_alloc((existsTag->end - rightLen), existsTag->end, existsTag->parent, false);
      LOGD("[cutoff tag right] addr=%p, existsTag=%s, newTag=%s, existsTagAddress[%p,%p), newTagAddress[%p,%p)\n", (void*)addr, tag_sprint(existsTag).c_str() ,tag_sprint(newTag).c_str(), (void*)firstAddr, (void*)finalAddr, (void*)(addr + 1), (void*)(finalAddr));
      for (ADDRINT i = (addr + 1); i < finalAddr; i++){
        tag_dir_setb(tag_dir, i, newTag->id);
      }
    }
  }
  tag_dir_setb(tag_dir, addr, tag_traits::cleared_val);
}
*/

void PIN_FAST_ANALYSIS_CALL tagmap_clrb(ADDRINT addr) {
  tag_entity* existsTag = tag_get(tagmap_getb(addr));
  if (existsTag == NULL || existsTag->getLen() == 1){
    tag_dir_setb(tag_dir, addr, tag_traits::cleared_val);
    return;
  }
  ADDRINT firstAddr = getFirstAddr(addr, existsTag->id);
  ADDRINT finalAddr = getFinalAddr(addr, existsTag->id);
  LOGD("[clear mem taint tag] dst=%p, clear_address[%p,%p)\n", (void*)addr, (void*)firstAddr, (void*)finalAddr);
  for (ADDRINT i = firstAddr; i < finalAddr; i++){
    tag_dir_setb(tag_dir, i, tag_traits::cleared_val);
  }
  }

void PIN_FAST_ANALYSIS_CALL tagmap_clrn(ADDRINT addr, UINT32 n) {
  ADDRINT i;
  for (i = addr; i < addr + n; i++) {
    tagmap_clrb(i);
  }
}
/*
tag_t tagmap_getn(ADDRINT addr, unsigned int n) {
  tag_t ts = tag_traits::cleared_val;
  for (size_t i = 0; i < n; i++) {
    const tag_t t = tagmap_getb(addr + i);
    if (tag_is_empty(t))
      continue;
    // LOGD("[tagmap_getn] %lu, ts: %d, %s\n", i, ts, tag_sprint(t).c_str());
    ts = tag_combine(ts, t);
    // LOGD("t: %d, ts:%d\n", t, ts);
  }
  return ts;
}

tag_t tagmap_getn_reg(THREADID tid, unsigned int reg_idx, unsigned int n) {
  tag_t ts = tag_traits::cleared_val;
  for (size_t i = 0; i < n; i++) {
    const tag_t t = tagmap_getb_reg(tid, reg_idx, i);
    if (tag_is_empty(t))
      continue;
    // LOGD("[tagmap_getn] %lu, ts: %d, %s\n", i, ts, tag_sprint(t).c_str());
    ts = tag_combine(ts, t);
    // LOGD("t: %d, ts:%d\n", t, ts);
  }
  return ts;
}
*/

ADDRINT getFirstAddr(ADDRINT addr, tag_t tag){
  while (tagmap_getb(addr-1) == tag){
    addr--;
  }
  return addr;
}

ADDRINT getFinalAddr(ADDRINT addr, tag_t tag){
  while (tagmap_getb(addr) == tag){
    addr++;
  }
  return addr;
}