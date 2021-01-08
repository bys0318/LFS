#include<unistd.h>
#include<fcntl.h>
#define SIZE (5*1024*1024)
char str[SIZE];
int main()
{
	int file;
	int f1;
	file=open("mount/file",O_RDWR);
	for(f1=0;f1<SIZE;f1++)
		str[f1]='a';
	write(file,str,SIZE);
	fsync(file);
	close(file);
	return 0;
}
