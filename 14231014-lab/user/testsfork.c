#include "lib.h"
int g;
umain(void)
{
  g = 1;
  int e = sfork();
  if(e!=0){
    writef("i am father. g = %d\n",g);
    g = 2;
  }
  else{
    writef("i am child. g = %d\n",g);
  }
}
