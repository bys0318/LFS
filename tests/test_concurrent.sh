gcc -Wall ../main.c ../metadata.c `pkg-config fuse3 --cflags --libs` -o lfs
gcc test_concurrent.c -o test_concurrent
mkdir mount
./lfs mount
./test_concurrent
umount ./lfs
rm -r mount
rm lfs
rm test_concurrent