#include <iostream>
#include <cstdlib>
#include <cstdarg>

#include <dlfcn.h>
#include <sys/types.h>
#include <stdlib.h>
#include <signal.h>
#include <setjmp.h>

static void *libc = NULL;
static pid_t (*libc_fork)() = NULL;
static int (*libc_execv)(const char *pathname, char *const argv[]) = NULL;
static int (*libc_execvp)(const char *pathname, char *const argv[]) = NULL;
static int (*libc_execve)(const char *pathname, char *const argv[], char *const envp[]) = NULL;
static int (*libc_execvpe)(const char *pathname, char *const argv[], char *const envp[]) = NULL;
static uint tries = 20;

static const char *default_libc_path = "/usr/lib/aarch64-linux-gnu/libc.so.6";

// ref: https://stackoverflow.com/questions/9225415/linux3-gcc46-fnon-call-exceptions-which-signals-are-trapping-instructions
// ref: https://stackoverflow.com/questions/10202941/segmentation-fault-handling
//class SegmentationFault {};

static void general_segfault_handler(int){
	std::cerr << std::endl << "segfault outside of forking" << std::endl;
}

struct sigaction general_segfault_action = {
	general_segfault_handler,
	NULL,
	0,
	0,
	NULL
};

static sigjmp_buf point;
static int lj_val = 0;
static void forking_segfault_handler(int){
	//throw SegmentationFault();
	lj_val++;
	std::cerr << std::endl << "fork segfault received, tries: " << lj_val << std::endl;
	siglongjmp(point, lj_val);
}

struct sigaction forking_segfault_action = {
	forking_segfault_handler,
	NULL,
	0,
	0,
	NULL
};

static sigjmp_buf exec_point;
static int exec_lj_val = 0;
static void exec_segfault_handler(int){
	exec_lj_val++;
	std::cerr << std::endl << "exec segfault received, tries: " << exec_lj_val << std::endl;
	siglongjmp(exec_point, exec_lj_val);
}

struct sigaction exec_segfault_action = {
	exec_segfault_handler,
	NULL,
	0,
	0,
	NULL
};

// XXX std::cerr is not ready during constructor
__attribute__((constructor)) static void init(){
	// load libc
	const char* libc_path = getenv("FORK_WRAPPER_LIBC_PATH");
	if(libc_path != NULL){
		//std::cerr << "fork_wrapper: opening libc from " << libc_path << std::endl;
	}else{
		//std::cerr << "fork_wrapper: opening libc from " << default_libc_path << std::endl;
		libc_path = default_libc_path;
	}

	libc = dlopen(libc_path, RTLD_NOW);
	if(libc == NULL){
		//std::cerr << "fork_wrapper: failed opening libc" << std::endl;
		exit(1);
	}

	libc_fork = (pid_t(*)())dlsym(libc, "fork");
	if(libc_fork == NULL){
		//std::cerr << "fork_wrapper: failed dlsym for fork" << std::endl;
	}

	libc_execv = (int(*)(const char *pathname, char *const argv[]))dlsym(libc, "execv");
	libc_execvp = (int(*)(const char *pathname, char *const argv[]))dlsym(libc, "execvp");
	libc_execve = (int(*)(const char *pathname, char *const argv[], char *const envp[]))dlsym(libc, "execve");
	libc_execvpe = (int(*)(const char *pathname, char *const argv[], char *const envp[]))dlsym(libc, "execvpe");

	if(libc_execv == NULL || libc_execvp == NULL || libc_execve == NULL || libc_execvpe == NULL){
		//std::cerr << "fork_wrapper: failed dlsym for exec" << std::endl;
		exit(1);
	}

	auto tries_env = getenv("FORK_WRAPPER_TRIES");
	if(tries_env != NULL){
		tries = atoi(tries_env);
	}
	//std::cerr << "fork_wrapper: fork retries is set to " << tries << std::endl;

	sigaction(SIGSEGV, &general_segfault_action, NULL);
}

__attribute__((destructor)) static void fini(){
}

extern "C" pid_t fork(){
	std::cerr << "forking" << std::endl;
	pid_t ret = 0;
	int i = 0;
	sigaction(SIGSEGV, &forking_segfault_action, NULL);
	lj_val = 0;
	while(sigsetjmp(point, 1) < tries){
		ret = libc_fork();
		if(ret != 0){
			std::cerr << "child with pid " << ret << " created" << std::endl;
			/*
			int *seg = NULL;
			*seg = 1;
			*/
		}else{
			std::cerr << "child begins" << std::endl;
		}
		sigaction(SIGSEGV, &general_segfault_action, NULL);
		return ret;
	}
	std::cerr << "forking faulted too many times, terminating" << std::endl;
	exit(1);
	return 0;
}

extern "C" int execv(const char *pathname, char *const argv[]){
	int ret;
	std::cerr << "using execv" << std::endl;
	sigaction(SIGSEGV, &exec_segfault_action, NULL);
	exec_lj_val = 0;
	while(sigsetjmp(exec_point, 1) < tries){
		ret = libc_execv(pathname, argv);
		std::cerr << "execv failed with ret " << ret << std::endl;
		return ret;
	}
	std::cerr << "execv faulted too many times, terminating" << std::endl;
	exit(1);
	return 0;
}

extern "C" int execvp(const char *pathname, char *const argv[]){
	int ret;
	std::cerr << "using execvp" << std::endl;
	sigaction(SIGSEGV, &exec_segfault_action, NULL);
	exec_lj_val = 0;
	while(sigsetjmp(exec_point, 1) < tries){
		ret = libc_execvp(pathname, argv);
		std::cerr << "execvp failed with ret " << ret << std::endl;
		return ret;
	}
	std::cerr << "execvp faulted too many times, terminating" << std::endl;
	exit(1);
	return 0;
}

extern "C" int execve(const char *pathname, char *const argv[], char *const envp[]){
	int ret;
	std::cerr << "using execve" << std::endl;
	sigaction(SIGSEGV, &exec_segfault_action, NULL);
	exec_lj_val = 0;
	while(sigsetjmp(exec_point, 1) < tries){
		ret = libc_execve(pathname, argv, envp);
		std::cerr << "execve failed with ret " << ret << std::endl;
		return ret;
	}
	std::cerr << "execve faulted too many times, terminating" << std::endl;
	exit(1);
	return 0;
}

extern "C" int execvpe(const char *pathname, char *const argv[], char *const envp[]){
	int ret;
	std::cerr << "using execvpe" << std::endl;
	sigaction(SIGSEGV, &exec_segfault_action, NULL);
	exec_lj_val = 0;
	while(sigsetjmp(exec_point, 1) < tries){
		ret = libc_execvpe(pathname, argv, envp);
		std::cerr << "execvpe failed with ret " << ret << std::endl;
		return ret;
	}
	std::cerr << "execvpe faulted too many times, terminating" << std::endl;
	exit(1);
	return 0;
}

#define arg_buffer_len 5000
extern "C" int execl(const char *pathname, const char *c, ...){
	// too short?
	// don't want to deal with heap here however, should be good enough
	std::cerr << "using execl" << std::endl;
	char *arg_buffer[arg_buffer_len];
	va_list args;
	va_start(args, c);
	arg_buffer[0] = (char *)c;
	int i = 1;
	while(arg_buffer[i - 1] != NULL){
		arg_buffer[i] = va_arg(args, char *);
		i++;
	}
	return execv(pathname, arg_buffer);
}

extern "C" int execlp(const char *pathname, const char *c, ...){
	std::cerr << "using execlp" << std::endl;
	char *arg_buffer[arg_buffer_len];
	va_list args;
	va_start(args, c);
	arg_buffer[0] = (char *)c;
	int i = 1;
	while(arg_buffer[i - 1] != NULL){
		arg_buffer[i] = va_arg(args, char *);
		i++;
	}
	return execvp(pathname, arg_buffer);
}

extern "C" int execle(const char *pathname, const char *c, ...){
	std::cerr << "using execle" << std::endl;
	char *arg_buffer[arg_buffer_len];
	char **env;
	va_list args;
	va_start(args, c);
	arg_buffer[0] = (char *)c;
	int i = 1;
	while(arg_buffer[i - 1] != NULL){
		arg_buffer[i] = va_arg(args, char *);
		i++;
	}
	env = va_arg(args, char **);
	return execve(pathname, arg_buffer, env);
}
