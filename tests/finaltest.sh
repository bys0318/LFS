PKG_CONFIG_PATH=$PKG_CONFIG_PATH:/usr/local/lib64/pkgconfig/
export PKG_CONFIG_PATH
gcc -Wall ../main.c ../metadata.c `pkg-config fuse3 --cflags --libs` -o lfs
mkdir mount
./lfs mount
for ((i=1;i<3;i++));
do
python3 lab2.py
done
umount ./lfs
rm -rf mount
rm lfs
