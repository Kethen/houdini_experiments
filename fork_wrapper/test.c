#include <unistd.h>
#include <stdio.h>

int main(){
	pid_t pid =  fork();
	printf("pid is %d\n", pid);
}
