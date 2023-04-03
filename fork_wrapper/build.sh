set -xe
g++ -std=c++11 -g -fPIC -c fork_wrapper.cpp -o fork_wrapper.o
g++ -g -shared -o libfork_wrapper.so fork_wrapper.o -ldl
gcc test.c -o test
export FORK_WRAPPER_LIBC_PATH=/usr/lib64/libc.so.6
LD_PRELOAD="$(pwd)/libfork_wrapper.so" ./test
