gcc -Wall ../lfs.c `pkg-config fuse3 --cflags --libs` -o lfs
gcc test_lfs.c -o test_lfs
mkdir mount
./lfs mount
mkdir test
touch a.txt
./test_lfs
rm -r test
rm -r mount
rm a.txt
umount ./lfs
rm lfs