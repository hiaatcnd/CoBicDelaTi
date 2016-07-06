#include<stdio.h>
int main(){
  int a[2100000];
  int i;
  for(i=0;i<2100000;i++){
    a[i]=i;
    printf("--test-- a[%d] : %08x\n",i,&(a[i]));
  }
  printf("--test-- &i : %08x",&i);
  return 0;
}
