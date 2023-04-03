#include <unistd.h>
#include <stdio.h>

int main(){
	pid_t pid =  fork();
	printf("pid is %d\n", pid);

	char *args[] = {"/usr/bin/ls", NULL};
	char *env[] = {NULL};
	/*
	int ret = execvp("/usr/bin/ls", args);
	if(ret != 0){
		printf("execvp failed with %d\n", ret);
	}
	ret = execv("/usr/bin/ls", args);
	if(ret != 0){
		printf("execv failed with %d\n", ret);
	}
	ret = execve("/usr/bin/ls", args, env);
	if(ret != 0){
		printf("execve failed with %d\n", ret);
	}
	*/
	int ret = execlp("/usr/bin/bash", "/usr/bin/bash", "-c", "echo haha", NULL);
	if(ret != 0){
		printf("execlp failed with %d\n", ret);
	}
}
