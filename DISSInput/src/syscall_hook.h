
#ifndef __SYSCALL_HOOK_H__
#define __SYSCALL_HOOK_H__

void hook_file_syscall(const char* inputFileName);
void hook_network_syscall();

#endif