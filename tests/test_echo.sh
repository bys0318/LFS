gcc -Wall ../echo.c `pkg-config fuse3 --cflags --libs` -o echo1
mkdir mount1
./echo1 mount1
python3 over.py
sleep 1
# test linux commands: ls, cat, chmod, mv, cp, ln, rm
echo '--------Test 1: Testing on echo file system--------'
echo -e '\nls mount1'
ls mount1
python3 readlog.py ls
sleep 0.1
echo -e '\ncat mount1/a.txt'
cat mount1/a.txt
python3 readlog.py cat
sleep 0.1
echo -e '\nchmod 777 mount1/a.txt'
chmod 777 mount1/a.txt
python3 readlog.py chmod
sleep 0.1
echo -e '\nmv mount1/a.txt mount1/b.txt'
mv mount1/a.txt mount1/b.txt
python3 readlog.py mv
sleep 0.1
echo -e '\ncp mount1/a.txt mount1/b.txt'
cp mount1/a.txt mount1/b.txt
python3 readlog.py cp
sleep 0.1
echo -e '\nln mount1/a.txt mount1/a'
ln mount1/a.txt mount1/a
python3 readlog.py ln
sleep 0.1
echo -e '\nrm mount1/a.txt'
rm mount1/a.txt
python3 readlog.py rm
sleep 0.1
echo -e '\n--------Test 1: Completed--------'
rm last.log
umount ./echo1
rm -r mount1
rm echo1