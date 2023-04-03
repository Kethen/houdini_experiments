set -xe
gcc -g -fPIC -c notatty.c -o notatty.o
gcc -g -shared -o libnotatty.so notatty.o
