// User-level IPC library routines

#include "lib.h"
#include <mmu.h>
#include <env.h>

extern struct Env *env;

// Send val to whom.  This function keeps trying until
// it succeeds.  It should panic() on any error other than
// -E_IPC_NOT_RECV.
//
// Hint: use syscall_yield() to be CPU-friendly.
void
ipc_send(u_int whom, u_int val, u_int srcva, u_int perm)
{
	int r;

	while ((r = syscall_ipc_can_send(whom, val, srcva, perm)) == -E_IPC_NOT_RECV) {
		syscall_yield();
		//writef("QQ");
	}

	if (r == 0) {
		return;
	}

	user_panic("error in ipc_send: %d", r);
}

// Receive a value.  Return the value and store the caller's envid
// in *whom.
//
// Hint: use env to discover the value and who sent it.
u_int
ipc_recv(u_int *whom, u_int dstva, u_int *perm)
{
	//printf("ipc_recv:come 0\n");
	syscall_ipc_recv(dstva);

	if (whom) {
		*whom = env->env_ipc_from;
	}

	if (perm) {
		*perm = env->env_ipc_perm;
	}

	return env->env_ipc_value;
}

void new_ipc_send(u_int whom, u_int val, u_int srcva, u_int perm) {

	int r;
	int ack;
	int sender = 0;
	u_int temp = 0;

	ipc_send(whom, val, srcva, perm);

	/*
	if ((r =	syscall_mem_alloc(0, temp, PTE_V | PTE_R) {
		user_panic("syscall_mem_alloc error : %d\n", r );
	}
	*/

	ack = ipc_recv(&sender, temp, 0);

	if(ack == val + 1) {
		ipc_send(whom, val, srcva, perm);
	}
	else {
		writef("ALEPH_DEBUG: ack error!\n");
	}
}

u_int new_ipc_recv(u_int *whom, u_int dstva, u_int *perm) {

	int r;
	int rec = 0;
	int check = -1;
	u_int temp = 0;

	rec = ipc_recv(whom, dstva, perm);

	ipc_send(*whom, rec + 1, temp, 0);

	if((check = ipc_recv(whom, dstva, perm)) == rec) {
		return env->env_ipc_value;
	}
	else {
		user_panic("ALEPH_DEBUG: recv error");
	}
	return -10086;
}
