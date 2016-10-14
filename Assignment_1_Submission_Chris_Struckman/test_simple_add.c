#include <stdio.h>
#define _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h>

int main(){
int r;
int result;
r = syscall(327,5 ,4, &result);
printf("Your result after running the adding system call is: %d\n",result );
return 0;
}
