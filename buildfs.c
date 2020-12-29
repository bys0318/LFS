#include<stdio.h>
int main()
{
	FILE*fp;
	int f1;
	fp=fopen("LFS","w");
	for(f1=0;f1<100000000;f1++)
		fputc('0',fp);
	fclose(fp);
	return 0;
}
