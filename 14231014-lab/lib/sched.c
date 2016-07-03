#include <env.h>
#include <pmap.h>
#include <printf.h>

/* Overview:
 *  Implement simple round-robin scheduling.
 *  Search through 'envs' for a runnable environment ,
 *  in circular fashion statrting after the previously running env,
 *  and switch to the first such environment found.
 *
 * Hints:
 *  The variable which is for counting should be defined as 'static'.
 */
void sched_yield(void)
{
	static int Next_EnvNum = 0;
	int iTemp, i;

	for (i = 0; i < NENV; i++) {	//envs[0] must be runnable
		if (envs[Next_EnvNum].env_status == ENV_RUNNABLE) {
			iTemp = Next_EnvNum;
			Next_EnvNum++;
			Next_EnvNum = Next_EnvNum % NENV;

			if (iTemp != 0) {
				//panic("BBBBBBBBBBBBBBBBBBBB id:%x",iTemp);
				env_run(&envs[iTemp]);
			}
		} else {
			Next_EnvNum++;
			Next_EnvNum = Next_EnvNum % NENV;
		}
	}

	//env_run(&envs[0]);
	panic("There is no process to run ! haha !\n");
}

