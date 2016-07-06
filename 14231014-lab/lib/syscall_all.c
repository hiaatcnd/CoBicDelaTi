#include "../drivers/gxconsole/dev_cons.h"
#include <mmu.h>
#include <env.h>
#include <printf.h>
#include <pmap.h>
#include <sched.h>

extern char *KERNEL_SP;
extern struct Env *curenv;
#define UVPD (UVPT+(UVPT>>12)*4)

/* Overview:
 * 	This function is used to print a character on screen.
 *
 * Pre-Condition:
 * 	`c` is the character you want to print.
 */
void sys_putchar(int sysno, int c, int a2, int a3, int a4, int a5)
{
	printcharc((char) c);
	return ;
}

/* Overview:
 * 	This function enables you to copy content of `srcaddr` to `destaddr`.
 *
 * Pre-Condition:
 * 	`destaddr` and `srcaddr` can't be NULL. Also, the `srcaddr` area
 * 	shouldn't overlap the `destaddr`, otherwise the behavior of this
 * 	function is undefined.
 *
 * Post-Condition:
 * 	the content of `destaddr` area(from `destaddr` to `destaddr`+`len`) will
 * be same as that of `srcaddr` area.
 */
void *memcpy(void *destaddr, void const *srcaddr, u_int len)
{
	char *dest = destaddr;
	char const *src = srcaddr;

	while (len-- > 0) {
		*dest++ = *src++;
	}

	return destaddr;
}

/* Overview:
 *	This function provides the environment id of current process.
 *
 * Post-Condition:
 * 	return the current environment id
 */
u_int sys_getenvid(void)
{
	return curenv->env_id;
}

/* Overview:
 *	This function enables the current process to give up CPU.
 *
 * Post-Condition:
 * 	Deschedule current environment. This function will never return.
 */
void sys_yield(void)
{
	bcopy((void *)KERNEL_SP - sizeof(struct Trapframe),
		  (void *)TIMESTACK - sizeof(struct Trapframe),
		  sizeof(struct Trapframe));
	sched_yield();
}

/* Overview:
 * 	This function is used to destroy the current environment.
 *
 * Pre-Condition:
 * 	The parameter `envid` must be the environment id of a
 * process, which is either a child of the caller of this function
 * or the caller itself.
 *
 * Post-Condition:
 * 	Return 0 on success, < 0 when error occurs.
 */
int sys_env_destroy(int sysno, u_int envid)
{
	/*
		printf("[%08x] exiting gracefully\n", curenv->env_id);
		env_destroy(curenv);
	*/
	int r;
	struct Env *e;

	if ((r = envid2env(envid, &e, 1)) < 0) {
		return r;
	}

	printf("[%08x] destroying %08x\n", curenv->env_id, e->env_id);
	env_destroy(e);
	return 0;
}

/* Overview:
 * 	Set envid's pagefault handler entry point and exception stack.
 *
 * Pre-Condition:
 * 	xstacktop points one byte past exception stack.
 *
 * Post-Condition:
 * 	The envid's pagefault handler will be set to `func` and its
 * 	exception stack will be set to `xstacktop`.
 * 	Returns 0 on success, < 0 on error.
 */
int sys_set_pgfault_handler(int sysno, u_int envid, u_int func, u_int xstacktop)
{
	// Your code here.
	struct Env *env;
	int ret;


	if ((ret = envid2env(envid, &env, 0)) < 0) {
		return ret;
	}

	//	printf("sys_set_pgfault_handler():envid:%x\tfunc:%x",env->env_id,func);
	env->env_pgfault_handler = func;
	xstacktop = TRUP(xstacktop);
	env->env_xstacktop = xstacktop;
	return 0;
	//	panic("sys_set_pgfault_handler not implemented");
}

/* Overview:
 * 	Allocate a page of memory and map it at 'va' with permission
 * 'perm' in the address space of 'envid'.
 *
 * 	If a page is already mapped at 'va', that page is unmapped as a
 * side-effect.
 *
 * Pre-Condition:
 * perm -- PTE_V is required,
 *         PTE_COW is not allowed(return -E_INVAL),
 *         other bits are optional.
 *
 * Post-Condition:
 * Return 0 on success, < 0 on error
 *	- va must be < UTOP
 *	- env may modify its own address space or the address space of its children
 */
int sys_mem_alloc(int sysno, u_int envid, u_int va, u_int perm)
{
	// Your code here.
	struct Env *env;
	struct Page *ppage;
	int ret;
	ret = 0;

	//printf("sys_mem_alloc(): envid:%x  va%x  perm:%x\n",envid,va,perm);
	if ((perm & PTE_V) == 0) {
		return -E_INVAL;
	}

	//if((perm & PTE_R)==0)
	//	return -E_INVAL;
	if ((perm & PTE_COW) != 0) {
		return -E_INVAL;
	}

	if (va >= UTOP) {
		return -E_INVAL;
	}

	//printf("sys_mem_alloc(): envid2env begin!\n");
	if ((ret = envid2env(envid, &env, 0)) < 0) {
		return ret;
	}

	//printf("sys_mem_alloc(): page_alloc begin!\n");
	if ((ret = page_alloc(&ppage)) < 0) {
		return ret;
	}

	bzero((void *)page2kva(ppage), BY2PG);

	//printf("sys_mem_alloc(): page_insert begin!\n");
	if ((ret = page_insert(env->env_pgdir, ppage, va, perm)) < 0) {
		return ret;
	}

	return ret;
	//	panic("sys_mem_alloc not implemented");
}

/* Overview:
 * 	Map the page of memory at 'srcva' in srcid's address space
 * at 'dstva' in dstid's address space with permission 'perm'.
 * Perm has the same restrictions as in sys_mem_alloc.
 * (Probably we should add a restriction that you can't go from
 * non-writable to writable?)
 *
 * Post-Condition:
 * 	Return 0 on success, < 0 on error.
 *
 * Note:
 * 	Cannot access pages above UTOP.
 */
int sys_mem_map(int sysno, u_int srcid, u_int srcva, u_int dstid, u_int dstva,
				u_int perm)
{
	// Your code here.
	int ret;
	u_int round_srcva, round_dstva;
	struct Env *srcenv;
	struct Env *dstenv;
	struct Page *ppage;
	Pte *ppte;

	ppage = NULL;
	ret = 0;
	round_srcva = ROUNDDOWN(srcva, BY2PG);
	round_dstva = ROUNDDOWN(dstva, BY2PG);

	//printf("sys_mem_map comes 1\n");
	if (srcva > UTOP || dstva > UTOP) {
		return -E_INVAL;
	}

	//printf("sys_mem_map comes 2\n");
	if ((perm & PTE_V) == 0) {
		return -E_INVAL;
	}

	//printf("sys_mem_map comes 3\n");
	if ((ret = envid2env(srcid, &srcenv, 0)) < 0) {
		return ret;
	}

	//printf("sys_mem_map comes 4\n");
	if ((ret = envid2env(dstid, &dstenv, 0)) < 0) {
		return ret;
	}

	//printf("sys_mem_map comes 5 pgdir %x, va %x\n", srcenv->env_pgdir,round_srcva);
	ppage = page_lookup(srcenv->env_pgdir, round_srcva, &ppte);

	if (ppage == 0) {
		return -E_INVAL;
	}

	//printf("sys_mem_map comes 6\n");
	ret = page_insert(dstenv->env_pgdir, ppage, round_dstva, perm);
	//printf("sys_mem_map comes 7\n");
	return ret;
	//	panic("sys_mem_map not implemented");
}

/* Overview:
 * 	Unmap the page of memory at 'va' in the address space of 'envid'
 * (if no page is mapped, the function silently succeeds)
 *
 * Post-Condition:
 * 	Return 0 on success, < 0 on error.
 *
 * Cannot unmap pages above UTOP.
 */
int sys_mem_unmap(int sysno, u_int envid, u_int va)
{
	// Your code here.
	int ret;
	struct Env *env;

	ret = 0;

	if ((ret = envid2env(envid, &env, 0)) < 0) {
		return ret;
	}

	page_remove(env->env_pgdir, va);
	return ret;
	//	panic("sys_mem_unmap not implemented");
}

/* Overview:
 * 	Allocate a new environment.
 *
 * Pre-Condition:
 * The new child is left as env_alloc created it, except that
 * status is set to ENV_NOT_RUNNABLE and the register set is copied
 * from the current environment.
 *
 * Post-Condition:
 * 	In the child, the register set is tweaked so sys_env_alloc returns 0.
 * 	Returns envid of new environment, or < 0 on error.
 */
int sys_env_alloc(void)
{
	// Your code here.
	int r;
	struct Env *e;


	if ((r = env_alloc(&e, curenv->env_id)) < 0) {
		return r;
	}

	bcopy(KERNEL_SP - sizeof(struct Trapframe), &e->env_tf,
		  sizeof(struct Trapframe));
	printf("sys_env_alloc():the child pc :%x", e->env_tf.cp0_epc);
	e->env_tf.pc = e->env_tf.cp0_epc;
	//e->env_tf.cp0_status = 0x1000000C;
	e->env_status = ENV_NOT_RUNNABLE;
	e->env_tf.regs[2] = 0;	//important
	e->env_pgfault_handler = 0;//curenv->env_pgfault_handler;
	return e->env_id;
	//	panic("sys_env_alloc not implemented");
}

/* Overview:
 * 	Set envid's env_status to status.
 *
 * Pre-Condition:
 * 	status should be one of `ENV_RUNNABLE`, `ENV_NOT_RUNNABLE` and
 * `ENV_FREE`. Otherwise return -E_INVAL.
 *
 * Post-Condition:
 * 	Returns 0 on success, < 0 on error.
 * 	Return -E_INVAL if status is not a valid status for an environment.
 * 	The status of environment will be set to `status` on success.
 */
int sys_set_env_status(int sysno, u_int envid, u_int status)
{
	// Your code here.
	struct Env *env;
	int ret;

	if ((status != ENV_RUNNABLE) && (status != ENV_NOT_RUNNABLE) &&
		(status != ENV_FREE)) {
		return -E_INVAL;
	}

	if ((ret = envid2env(envid, &env, 0)) < 0) {
		return ret;
	}

	env->env_status = status;
	return 0;
	//	panic("sys_env_set_status not implemented");
}

/* Overview:
 * 	Set envid's trap frame to tf.
 *
 * Pre-Condition:
 * 	`tf` should be valid.
 *
 * Post-Condition:
 * 	Returns 0 on success, < 0 on error.
 * 	Return -E_INVAL if the environment cannot be manipulated.
 *
 * Note: This hasn't be used now?
 */
int sys_set_trapframe(int sysno, u_int envid, struct Trapframe *tf)
{

	return 0;
}

/* Overview:
 * 	Kernel panic with message `msg`.
 *
 * Pre-Condition:
 * 	msg can't be NULL
 *
 * Post-Condition:
 * 	This function will make the whole system stop.
 */
void sys_panic(int sysno, char *msg)
{
	// no page_fault_mode -- we are trying to panic!
	panic("%s", TRUP(msg));
}

/* Overview:
 * 	This function enables caller to receive message from
 * other process. To be more specific, it will flag
 * the current process so that other process could send
 * message to it.
 *
 * Pre-Condition:
 * 	`dstva` is valid (Note: NULL is also a valid value for `dstva`).
 *
 * Post-Condition:
 * 	This syscall will set the current process's status to
 * ENV_NOT_RUNNABLE, giving up cpu.
 */
void sys_ipc_recv(int sysno, u_int dstva)
{
	/*
		// Your code here
		curenv->env_status = ENV_NOT_RUNNABLE;
		curenv->env_ipc_recving = 1;
		sched_yield();
	//	panic("sys_ipc_recv not implemented");
	*/
	if (curenv->env_ipc_recving) {
		panic("already recving!");
	}

	if (dstva >= UTOP) {
		panic("invalid dstva:%x", dstva);
	}

	//printf("Kernel:sys_ipc_recv come 0\n");
	//panic("_________________");
	curenv->env_ipc_recving = 1;
	curenv->env_ipc_dstva = dstva;
	curenv->env_status = ENV_NOT_RUNNABLE;
	//printf("Kernel:sys_ipc_recv come 1\n");
	bcopy((void *)KERNEL_SP - sizeof(struct Trapframe),
		  (void *)TIMESTACK - sizeof(struct Trapframe), sizeof(struct Trapframe));
	sched_yield();
}

/* Overview:
 * 	Try to send 'value' to the target env 'envid'.
 *
 * 	The send fails with a return value of -E_IPC_NOT_RECV if the
 * target has not requested IPC with sys_ipc_recv.
 * 	Otherwise, the send succeeds, and the target's ipc fields are
 * updated as follows:
 *    env_ipc_recving is set to 0 to block future sends
 *    env_ipc_from is set to the sending envid
 *    env_ipc_value is set to the 'value' parameter
 * 	The target environment is marked runnable again.
 *
 * Post-Condition:
 * 	Return 0 on success, < 0 on error.
 *
 * Hint: the only function you need to call is envid2env.
 */
int sys_ipc_can_send(int sysno, u_int envid, u_int value, u_int srcva,
					 u_int perm)
{
	/*
		// Your code here
		struct Env *env;
		int ret;
		ret = envid2env(envid, &env, 0);
		if(ret==0)
		{
			if(env->env_ipc_recving == 0) return -E_IPC_NOT_RECV;
			env->env_status =ENV_RUNNABLE;
			env->env_ipc_recving = 0;
			env->env_ipc_from = curenv->env_id;
			env->env_ipc_value = value;
			return ret;
		}else
		{
			printf("No such env\n");
			return ret;
		}*/
	//panic("sys_ipc_can_send not implemented");

	int r;
	struct Env *e;
	struct Page *p;

	if ((r = envid2env(envid, &e, 0)) < 0) {
		return r;
	}

	if (!(e)->env_ipc_recving) {
		//writef("there is no env to recevie!!");
		return -E_IPC_NOT_RECV;
	}

	if (srcva != 0 && e->env_ipc_dstva != 0) {

		if (srcva >= UTOP) {
			return -E_INVAL;
		}

		//we have no fault //if (((~perm)&(PTE_U|PTE_P)) || (perm&~(PTE_U|PTE_P|PTE_AVAIL|PTE_W))) return -E_INVAL;
		//		printf("sys_ipc_can_send:come 1\n");
		//		printf("srcva = %x,	dstva = %x\n",srcva,e->env_ipc_dstva);

		p = page_lookup(curenv->env_pgdir, srcva, 0);

		if (p == 0) {
			printf("[%08x] page_lookup %08x failed in sys_ipc_can_send\n",
				   curenv->env_id, srcva);
			return -E_INVAL;
		}

		//		printf("sys_ipc_can_sen:come 2\n");
		r = page_insert(e->env_pgdir, p, e->env_ipc_dstva, perm);

		if (r < 0) {
			return r;
		}

		e->env_ipc_perm = perm;
	} else {
		e->env_ipc_perm = 0;
	}

	e->env_ipc_recving = 0;
	e->env_ipc_from = curenv->env_id;
	e->env_ipc_value = value;
	e->env_status = ENV_RUNNABLE;
	//	printf("sys_ipc_can_send:out\n");
	return 0;
}

static int s_mem_map(int sysno, u_int srcid, u_int srcva, u_int dstid, u_int dstva,	u_int perm)
{
	// Your code here.
	int ret;
	u_int round_srcva, round_dstva;
	struct Env *srcenv;
	struct Env *dstenv;
	struct Page *ppage;
	Pte *ppte;

	ppage = NULL;
	ret = 0;
	round_srcva = ROUNDDOWN(srcva, BY2PG);
	round_dstva = ROUNDDOWN(dstva, BY2PG);

	//printf("sys_mem_map comes 2\n");
	if ((perm & PTE_V) == 0) {
		return -E_INVAL;
	}

	//printf("sys_mem_map comes 3\n");
	if ((ret = envid2env(srcid, &srcenv, 0)) < 0) {
		return ret;
	}

	//printf("sys_mem_map comes 4\n");
	if ((ret = envid2env(dstid, &dstenv, 0)) < 0) {
		return ret;
	}

	//printf("sys_mem_map comes 5 pgdir %x, va %x\n", srcenv->env_pgdir,round_srcva);
	ppage = page_lookup(srcenv->env_pgdir, round_srcva, &ppte);

	if (ppage == 0) {
		return -E_INVAL;
	}

	//printf("sys_mem_map comes 6\n");
	ppage->pp_ref++;
	printf("round_srcva = %08x; round_dstva = %08x; ppage->pp_ref = %d\n",round_srcva,round_dstva,ppage->pp_ref);
	//ret = page_insert(dstenv->env_pgdir, ppage, round_dstva, perm);
	//printf("sys_mem_map comes 7\n");
	return ret;
	//	panic("sys_mem_map not implemented");
}

int sys_s_env_alloc(u_int envid)
{
	// Your code here.
	int r;
	struct Env *e;
	struct Env *parent;
	int i;
	struct Page *p;
	Pte *ppte;

	if( (r = envid2env(curenv->env_id, &parent, 0)) < 0){
		panic("env_setup_vm - envid2env error\n");
	}


	if ((r = env_alloc(&e, curenv->env_id)) < 0) {
		return r;
	}

	bcopy(KERNEL_SP - sizeof(struct Trapframe), &e->env_tf,
		  sizeof(struct Trapframe));
	printf("sys_env_alloc():the child pc :%x\n", e->env_tf.cp0_epc);
	e->env_tf.pc = e->env_tf.cp0_epc;
	//e->env_tf.cp0_status = 0x1000000C;
	e->env_status = ENV_NOT_RUNNABLE;
	e->env_tf.regs[2] = 0;	//important
	e->env_pgfault_handler = 0;//curenv->env_pgfault_handler;

	for (i = 0; i < PDX((UTOP - 2*PDMAP)); i++) {
		if((parent->env_pgdir)[i] != 0){
			(e->env_pgdir)[i] = (parent->env_pgdir)[i];
			r = s_mem_map(0, curenv->env_id, UVPT + i * BY2PG, e->env_id, UVPT + i * BY2PG, ((parent->env_pgdir)[i])&0xfff);
			printf("s_setup 1: UVPT + i * BY2PG = %08x; pgdir[i] = %08x; (parent->env_pgdir)[i] = %08x; r = %d\n",UVPT + i * BY2PG,e->env_pgdir[i] ,(parent->env_pgdir)[i], r );
		}
	}
	//printf("!!!!!! PTX((UTOP - 2*PDMAP)) = %08x\n",PTX((UTOP - 2*PDMAP)) );
	for (i = 0; i < (UTOP - 2*PDMAP)/BY2PG; i++) {
		if (((u_int *)UVPD)[i / PTE2PT] != 0 && ((u_int *)UVPT)[i] != 0) {
			s_mem_map(0, curenv->env_id, i * BY2PG, e->env_id, i * BY2PG, ((u_int *)UVPT)[i]&0xfff);
			printf("s_setup 2: i * BY2PG = %08x; perm = %08x \n",i * BY2PG ,((u_int *)UVPT)[i]&0xfff);
		}
	}
	/*
	for (i = 0; i < (UTOP - 2*PDMAP)/BY2PG; i++) {
		if (((u_int *)UVPD)[i / PTE2PT] != 0 && ((u_int *)UVPT)[i] != 0) {
			printf("s_setup check: i = %d\n",i);
			p = page_lookup(e->env_pgdir, i*BY2PG, &ppte);
			printf("s_setup check: p->pp_ref = %d\n",p->pp_ref );
		}
	}
	for (i = UVPT/BY2PG; i < (UVPT + PDMAP)/BY2PG; i++) {
		if (((u_int *)UVPD)[i / PTE2PT] != 0 && ((u_int *)UVPT)[i] != 0) {
			printf("s_setup check: i = %d\n",i);
			p = page_lookup(e->env_pgdir, i*BY2PG, &ppte);
			printf("s_setup check: p->pp_ref = %d\n",p->pp_ref );
		}
	}
*/
	return e->env_id;
	//	panic("sys_env_alloc not implemented");
}
