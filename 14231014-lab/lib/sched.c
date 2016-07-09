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
    //printf("AlephDebug: ==========sched_yield==========\n");
    static int env_i = 0;
  	int ti, i;

  	while(1)
    {
      env_i = (env_i+1)%NENV;
  		if (envs[env_i].env_status == ENV_RUNNABLE)
      {
        //printf("AlephDebug: Call Env #%d\n",env_i);
        env_run(&envs[env_i]);
      }
      else
      {
        //printf("AlephDebug: Call Env #%d\n Fail",env_i);
      }
  	}
  }
