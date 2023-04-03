#include <iostream>
#include <cstdlib>

#include <dlfcn.h>
#include <sys/types.h>
#include <stdlib.h>

static void *libc = NULL;
static pid_t (*libc_fork)() = NULL;
static uint tries = 20;

static const char *default_libc_path = "/usr/lib/aarch64-linux-gnu/libc.so.6";

class SegmentationFault {};

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
		//std::cerr << "fork_wrapper: failed dlsym" << std::endl;
	}

	auto tries_env = getenv("FORK_WRAPPER_TRIES");
	if(tries_env != NULL){
		tries = atoi(tries_env);
	}
	//std::cerr << "fork_wrapper: fork retries is set to " << tries << std::endl;
}

__attribute__((destructor)) static void fini(){
}

extern "C" pid_t fork(){
	std::cerr << "forking" << std::endl;
	for(int i = 0;i < tries; i++){
		try{
			pid_t ret = libc_fork();
			if(ret != 0){
				std::cerr << "fork_wrapper: created new process " << ret << std::endl;
			}else{
				std::cerr  << "fork_wrapper: child process fork invoked" << std::endl;
			}
			return ret;
		}catch (const SegmentationFault&){
			std::cerr << "failed forked, tries: " << i << std::endl;
		}
	}
	std::cerr << "forking faulted too many times, terminating" << std::endl;
	exit(1);
	return 0;
}
