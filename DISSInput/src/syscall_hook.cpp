#include "syscall_hook.h"
#include "branch_pred.h"
#include "debug.h"
#include "libdft_api.h"
#include "pin.H"
#include "syscall_desc.h"
#include "tagmap.h"
#include "ins_common_op.h"

#include <iostream>
#include <unordered_map>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdlib.h>

/* threads context */
extern thread_ctx_t *threads_ctx;

void get_file_name(char* path, char* fn){
    char *p;
    strcpy(fn,(p=strrchr(path,'/')) ? p+1 : path);
}

extern syscall_desc_t syscall_desc[SYSCALL_MAX];
std::unordered_map<int, size_t> fuzzing_fd_map;
static unsigned int stdin_read_off = 0;
static const char* fuzzing_input_file;
static size_t network_read_off = 0;

static inline bool is_fuzzing_fd(int fd) {
  return fd == STDIN_FILENO || fuzzing_fd_map.count(fd) > 0;
}

static inline void add_fuzzing_fd(int fd) {
  if (fd > 0)
    fuzzing_fd_map.insert(std::make_pair(fd, 0));
}

static inline void remove_fuzzing_fd(int fd) { fuzzing_fd_map.erase(fd); }


/* __NR_open post syscall hook */
static void post_open_hook(THREADID tid, syscall_ctx_t *ctx) {
  const int fd = ctx->ret;
  if (unlikely(fd < 0))
    return;
  char *file_path = (char *)ctx->arg[SYSCALL_ARG0];
  char file_name[100];
  get_file_name(file_path, file_name);
  LOGD("[open] fd: %d : %s (file_name=%s # %s)\n", fd, file_path, file_name, fuzzing_input_file);
  if (strcmp(file_name, fuzzing_input_file) == 0) {
    add_fuzzing_fd(fd);
  }
}

/* __NR_openat post syscall hook */
// int openat(int dirfd, const char *pathname, int flags, mode_t mode);
static void post_openat_hook(THREADID tid, syscall_ctx_t *ctx) {
  const int fd = ctx->ret;
  char *file_path = (char *)ctx->arg[SYSCALL_ARG1];
  char file_name[100];
  get_file_name(file_path, file_name); 
  LOGD("[openat] fd: %d : %s (file_name=%s # %s)\n", fd, file_path, file_name, fuzzing_input_file);
  if (strcmp(file_name, fuzzing_input_file) == 0) {
    add_fuzzing_fd(fd);
  }
}

static void post_dup_hook(THREADID tid, syscall_ctx_t *ctx) {
  const int ret = ctx->ret;
  if (ret < 0)
    return;
  const int old_fd = ctx->arg[SYSCALL_ARG0];
  if (is_fuzzing_fd(old_fd)) {
    LOGD("[dup] fd: %d -> %d\n", old_fd, ret);
    add_fuzzing_fd(ret);
  }
}

static void post_dup2_hook(THREADID tid, syscall_ctx_t *ctx) {
  const int ret = ctx->ret;
  if (ret < 0)
    return;
  const int old_fd = ctx->arg[SYSCALL_ARG0];
  const int new_fd = ctx->arg[SYSCALL_ARG1];
  if (is_fuzzing_fd(old_fd)) {
    add_fuzzing_fd(new_fd);
    LOGD("[dup2] fd: %d -> %d\n", old_fd, new_fd);
  }
}

/* __NR_close post syscall hook */
static void post_close_hook(THREADID tid, syscall_ctx_t *ctx) {
  if (unlikely((long)ctx->ret < 0))
    return;
  const int fd = ctx->arg[SYSCALL_ARG0];
  if (is_fuzzing_fd(fd)) {
    remove_fuzzing_fd(fd);
    LOGD("[close] fd: %d \n", fd);
  }
}

static void post_read_hook(THREADID tid, syscall_ctx_t *ctx) {
  /* read() was not successful; optimized branch */
  const size_t nr = ctx->ret;
  if (unlikely(nr <= 0))
    return;

  const int fd = ctx->arg[SYSCALL_ARG0];
  const ADDRINT buf = ctx->arg[SYSCALL_ARG1];
  //size_t count = ctx->arg[SYSCALL_ARG2];

  /* taint-source */
  if (is_fuzzing_fd(fd)) {
    //std::unordered_map<int, size_t>::iterator it = fuzzing_fd_map.find(fd);

    unsigned int read_off = 0;
    if (fd == STDIN_FILENO) {
      // maintain it by ourself
      read_off = stdin_read_off;
      stdin_read_off += nr;
    } else {
      // low-level POSIX file descriptor I/O.
      read_off = lseek(fd, 0, SEEK_CUR);
      read_off -= nr; // post
    }
    /*
    if (fd != STDIN_FILENO && (*it).second >= read_off + nr){
      LOGD("[read] duplicate read %ld > %ld [%p, %p).\n", (*it).second, (read_off + nr), (char *)buf, (char *)(buf + nr));
      tagmap_clrn(buf, nr);
      return;
    }
    if (fd != STDIN_FILENO){
      (*it).second = read_off + nr;
    }
    */
    /* set the tag markings 
    // Attn: use count replace nr
    // But count may be very very large!
    if (count > nr) {
      count = nr;
    }
    */
    tag_entity* tag = tag_alloc(read_off, read_off + nr, 0, false, threads_ctx[tid].callstack);
    taint2m_op(buf, tag, false);
    LOGD("[read] fd: %d, addr: %p, offset: %d, readSize: %lu, tag:%s\n", fd,
         (char *)buf, read_off, nr, tag_sprint(tag).c_str());
    tagmap_setb_reg(tid, DFT_REG_RAX, 0, tag_traits::cleared_val);
  } else {
    /* clear the tag markings */
    tagmap_clrn(buf, nr);
  }
}

/* __NR_pread64 post syscall hook */
static void post_pread64_hook(THREADID tid, syscall_ctx_t *ctx) {
  const size_t nr = ctx->ret;
  if (unlikely(nr <= 0))
    return;
  const int fd = ctx->arg[SYSCALL_ARG0];
  const ADDRINT buf = ctx->arg[SYSCALL_ARG1];
  size_t count = ctx->arg[SYSCALL_ARG2];
  const unsigned int read_off = ctx->arg[SYSCALL_ARG3];
  if (is_fuzzing_fd(fd)) {

    /*
    if (count > nr + 32) {
      count = nr + 32;
    }
    */

    tag_entity* tag = tag_alloc(read_off, read_off + nr, 0, false, threads_ctx[tid].callstack);
    taint2m_op(buf, tag, false);
    LOGD("[pread64] fd: %d, addr: %p, offset: %d, size: %lu / %lu, tag:%s\n", fd,
         (char *)buf, read_off, nr, count, tag_sprint(tag).c_str());
  } else {
    /* clear the tag markings */
    tagmap_clrn(buf, count);
  }
}

// void *mmap(void *start, size_t length, int prot, int flags, int fd, off_t
// offset);
/* __NR_mmap post syscall hook */
static void post_mmap_hook(THREADID tid, syscall_ctx_t *ctx) {
  const ADDRINT ret = ctx->ret;
  const int fd = ctx->arg[SYSCALL_ARG4];
  const int prot = ctx->arg[SYSCALL_ARG2];
  // PROT_READ 0x1
  if ((void *)ret == (void *)-1 || !(prot & 0x1))
    return;
  const ADDRINT buf = ret;
  const size_t nr = ctx->arg[SYSCALL_ARG1];
  const off_t read_off = ctx->arg[SYSCALL_ARG5];
  if (is_fuzzing_fd(fd)) {
    tag_entity* tag = tag_alloc(read_off, read_off + nr, 0, false, threads_ctx[tid].callstack);
    LOGD("[mmap] fd: %d, offset: %ld, buf:%p, size: %lu, tag: %s\n", fd, read_off, (void*)buf, nr, tag_sprint(tag).c_str());
    ADDRINT map_addr = buf;
    if (map_addr == 0){
      map_addr = ctx->ret;
      if (map_addr == 0){
        return;
      }
    }
    for (unsigned int i = 0; i < nr; i++) {
      tagmap_setb(buf + i, tag->id);
    }
  }
}

static void post_recvfrom_hook(THREADID tid, syscall_ctx_t *ctx) {
  /* not successful; optimized branch */
  if (unlikely((long)ctx->ret <= 0))
    return;

  const ADDRINT buf = ctx->arg[SYSCALL_ARG1];
  const size_t nr = (size_t)ctx->ret;
  tag_entity* tag = tag_alloc(network_read_off, network_read_off + nr, 0, false, threads_ctx[tid].callstack);
  taint2m_op(buf, tag, false);
  LOGD("[recvfrom] addr: %p, offset: %ld, readSize: %lu, tag:%s\n", (char *)buf, network_read_off, nr, tag_sprint(tag).c_str());
  network_read_off += nr;
  tagmap_setb_reg(tid, DFT_REG_RAX, 0, tag_traits::cleared_val);
}

static void post_munmap_hook(THREADID tid, syscall_ctx_t *ctx) {
  const ADDRINT ret = ctx->ret;
  if ((void *)ret == (void *)-1)
    return;
  const ADDRINT buf = ctx->arg[SYSCALL_ARG0];
  const size_t nr = ctx->arg[SYSCALL_ARG1];

  // std::cerr <<"[munmap] addr: " << buf << ", nr: "<< nr << std::endl;
  tagmap_clrn(buf, nr);
}

void hook_file_syscall(const char* inputFileName) {
  (void)syscall_set_post(&syscall_desc[__NR_open], post_open_hook);
  (void)syscall_set_post(&syscall_desc[__NR_openat], post_openat_hook);
  (void)syscall_set_post(&syscall_desc[__NR_dup], post_dup_hook);
  (void)syscall_set_post(&syscall_desc[__NR_dup2], post_dup2_hook);
  (void)syscall_set_post(&syscall_desc[__NR_dup3], post_dup2_hook);
  (void)syscall_set_post(&syscall_desc[__NR_close], post_close_hook);

  (void)syscall_set_post(&syscall_desc[__NR_read], post_read_hook);
  (void)syscall_set_post(&syscall_desc[__NR_pread64], post_pread64_hook);
  (void)syscall_set_post(&syscall_desc[__NR_mmap], post_mmap_hook);
  (void)syscall_set_post(&syscall_desc[__NR_munmap], post_munmap_hook);
  fuzzing_input_file = inputFileName;
}

void hook_network_syscall() {
  (void)syscall_set_post(&syscall_desc[__NR_open], post_open_hook);
  (void)syscall_set_post(&syscall_desc[__NR_openat], post_openat_hook);
  (void)syscall_set_post(&syscall_desc[__NR_read], post_read_hook);
  (void)syscall_set_post(&syscall_desc[__NR_pread64], post_pread64_hook);
  (void)syscall_set_post(&syscall_desc[__NR_dup], post_dup_hook);
  (void)syscall_set_post(&syscall_desc[__NR_dup2], post_dup2_hook);
  (void)syscall_set_post(&syscall_desc[__NR_dup3], post_dup2_hook);
  (void)syscall_set_post(&syscall_desc[__NR_close], post_close_hook);
  (void)syscall_set_post(&syscall_desc[__NR_mmap], post_mmap_hook);
  (void)syscall_set_post(&syscall_desc[__NR_munmap], post_munmap_hook);
  (void)syscall_set_post(&syscall_desc[__NR_recvfrom], post_recvfrom_hook);
  fuzzing_input_file = "#";

}