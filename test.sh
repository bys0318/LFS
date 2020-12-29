gcc -Wall test.c metadata.c `pkg-config fuse3 --cflags --libs` -o test
./test