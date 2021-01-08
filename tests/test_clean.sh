PKG_CONFIG_PATH=$PKG_CONFIG_PATH:/usr/local/lib64/pkgconfig/
export PKG_CONFIG_PATH
gcc -Wall ../main.c ../metadata.c `pkg-config fuse3 --cflags --libs` -o lfs
gcc test_clean.c -o test_clean
mkdir mount
./lfs mount
cd mount
touch a.txt
cd ..
sleep 0.1
./test_clean
for ((i=1;i<10;i++));
do
cp LFS mount/LFS
rm mount/LFS
done
cp LFS mount/LFS
