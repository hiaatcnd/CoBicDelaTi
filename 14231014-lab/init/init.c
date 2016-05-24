#include <asm/asm.h>
#include <pmap.h>
#include <env.h>
#include <printf.h>
#include <kclock.h>
#include <trap.h>

void mips_init()
{
	printf("init.c:\tmips_init() is called\n");
	mips_detect_memory();

	mips_vm_init();
	page_init();

	env_init();
	//aleph();//Aleph Debug

	/*you can create some processes(env) here. in terms of binary code, please refer current directory/code_a.c
	 * code_b.c*/
	/*you may want to create process by MACRO, please read env.h file, in which you will find it. this MACRO is very
	 * interesting, have fun please*/
	//ENV_CREATE(user_A);
	//ENV_CREATE(user_B);
	ENV_CREATE(user_pingpong);

	//extern u_char binary_user_A_start[];
	//extern u_int  binary_user_A_size;
	//extern u_char binary_user_B_start[];
	//extern u_int  binary_user_B_size;
	//printf("AlephDebug: ===========mips_init===========\n");
	//printf("AlephDebug: OTHER_LOG:--------------------------\n");
	//printf("AlephDebug: binary_user_A_start = %08lx\n", binary_user_A_start);
	//printf("AlephDebug: &binary_user_A_size = %08lx\n", &binary_user_A_size);
	//printf("AlephDebug: binary_user_B_start = %08lx\n", binary_user_B_start);
	//printf("AlephDebug: &binary_user_B_size = %08lx\n", &binary_user_B_size);

	//printf("AlephDebug: CALL TRAP_INIT()\n");
	trap_init();
	//printf("AlephDebug: END TRAP_INIT()\n");
	//printf("AlephDebug: CALL KCLOCK_INIT()\n");
	kclock_init();
	//printf("AlephDebug: END KCLOCK_INIT()\n");

	//env_run(&envs[0]);
	//env_run(&envs[1]);

	panic("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^");
	while(1);
	panic("init.c:\tend of mips_init() reached!");
}

void bcopy(const void *src, void *dst, size_t len)
{
	void *max;

	max = dst + len;
	// copy machine words while possible
	while (dst + 3 < max)
	{
		*(int *)dst = *(int *)src;
		dst+=4;
		src+=4;
	}
	// finish remaining 0-3 bytes
	while (dst < max)
	{
		*(char *)dst = *(char *)src;
		dst+=1;
		src+=1;
	}
}

void bzero(void *b, size_t len)
{
	void *max;

	max = b + len;

	//printf("init.c:\tzero from %x to %x\n",(int)b,(int)max);

	// zero machine words while possible

	while (b + 3 < max)
	{
		*(int *)b = 0;
		b+=4;
	}

	// finish remaining 0-3 bytes
	while (b < max)
	{
		*(char *)b++ = 0;
	}

}
