gcc -Wall -D_FILE_OFFSET_BITS=64 ../main.c ../metadata.c `pkg-config fuse3 --cflags --libs` -o lfs
gcc test_lfs.c -o test_lfs
mkdir mount
./lfs mount
mkdir test
touch a.txt
sleep 0.1
./test_lsf
rm -rf test
rm a.txt
umount ./lfs
rm -rf mount
rm lfs
rm test_lfs
