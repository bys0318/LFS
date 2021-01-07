// gcc ../main.c ../metadata.h -o test_lfs
// Testing on basic file operations (linux style)
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>
#include <string.h>

int main(int argc, char *argv[]){
    FILE *stream1, *stream2;
    char buf1[1024], buf2[1024], buf3[1024], buf4[1024];
    printf("--------Test Clean: Testing on Garbage Collection--------\n");
    // test cp / cat
    memset(buf1, 0, 1024);
    memset(buf2, 0, 1024);
    memset(buf3, 0, 1024);
    memset(buf4, 0, 1024);
    printf("1. test cp / cat\n");
    popen("cp a.txt test/a.txt", "r");
    popen("cp a.txt mount/a.txt", "r");
    stream1 = popen("cat test/a.txt", "r");
    stream2 = popen("cat mount/a.txt", "r");
    fread(buf1, sizeof(char), sizeof(buf1), stream1);
    fread(buf2, sizeof(char), sizeof(buf2), stream2);
    //printf("buf1:%s\nbuf2:%s\n", buf1, buf2);
    if (strcmp(buf1, buf2)==0)
        printf("cp / cat SUCCESS\n");
    else
        printf("cp / cat FAILED\n");

    // test mv / mkdir / ln / touch / ls
    memset(buf1, 0, 1024);
    memset(buf2, 0, 1024);
    memset(buf3, 0, 1024);
    memset(buf4, 0, 1024);
    printf("2. test mkdir / ln / touch / ls\n");
    popen("mkdir test/a", "r");
    popen("mkdir mount/a", "r");
    popen("touch test/b.txt", "r");
    popen("touch mount/b.txt", "r");
    popen("touch test/b.txt", "r");
    popen("touch mount/b.txt", "r");
    popen("ln test/a.txt test/a/a.txt", "r");
    popen("ln mount/a.txt mount/a/a.txt", "r");
    popen("mv test/b.txt test/a/", "r");
    popen("mv mount/b.txt mount/a/", "r");
    stream1 = popen("ls test", "r");
    stream2 = popen("ls mount", "r");
    sleep(1);
    fread(buf1, sizeof(char), sizeof(buf1), stream1);
    fread(buf2, sizeof(char), sizeof(buf2), stream2);
    stream1 = popen("ls test/a", "r");
    stream2 = popen("ls mount/a", "r");
    sleep(1);
    fread(buf3, sizeof(char), sizeof(buf3), stream1);
    fread(buf4, sizeof(char), sizeof(buf4), stream2);
    //printf("buf1:%s\nbuf2:%s\n", buf1, buf2);
    //printf("buf3:%s\nbuf4:%s\n", buf3, buf4);
    if (strcmp(buf1, buf2)==0 && strcmp(buf3, buf4)==0)
        printf("mv / mkdir / ln / touch / ls SUCCESS\n");
    else
        printf("mv / mkdir / ln / touch / ls FAILED\n");

    // test rm
    memset(buf1, 0, 1024);
    memset(buf2, 0, 1024);
    memset(buf3, 0, 1024);
    memset(buf4, 0, 1024);
    printf("3. test rm\n");
    popen("rm test/a.txt", "r");
    popen("rm mount/a.txt", "r");
    sleep(1);
    stream1 = popen("ls test", "r");
    stream2 = popen("ls mount", "r");
    sleep(1);
    fread(buf1, sizeof(char), sizeof(buf1), stream1);
    fread(buf2, sizeof(char), sizeof(buf2), stream2);
    stream1 = popen("ls test/a", "r");
    stream2 = popen("ls mount/a", "r");
    sleep(1);
    fread(buf3, sizeof(char), sizeof(buf3), stream1);
    fread(buf4, sizeof(char), sizeof(buf4), stream2);
    if (strcmp(buf1, buf2)==0 && strcmp(buf3, buf4)==0)
        printf("rm SUCCESS\n");
    else
        printf("rm FAILED\n");
    
    // test chmod / chown / ls -l
    memset(buf1, 0, 1024);
    memset(buf2, 0, 1024);
    memset(buf3, 0, 1024);
    memset(buf4, 0, 1024);
    printf("4. test chmod / ls -l\n");
    popen("chmod 544 test/a/b.txt", "r");
    popen("chmod 544 mount/a/b.txt", "r");
    sleep(1);
    char buf[100] = "not written";
    int fd = open("mount/a/b.txt", O_RDWR);
    assert(fd < 0);
    stream1 = popen("ls -l test/a", "r");
    stream2 = popen("ls -l mount/a", "r");
    sleep(1);
    fread(buf1, sizeof(char), sizeof(buf1), stream1);
    fread(buf2, sizeof(char), sizeof(buf2), stream2);
    if (strcmp(buf1, buf2)==0)
        printf("chmod / ls -l SUCCESS\n");
    else
        printf("chmod / ls -l FAILED\n");
    
    // test modifying file (more than one block)
    printf("5. test modifying file\n");
    char bufwrite[10000];
    for (int i = 0; i < 8000; i++)
        bufwrite[i] = 'b';
    bufwrite[8000] = '\0';
    fd = open("mount/a/a.txt", O_RDWR);
    assert(fd > 0);
    int r_write = write(fd, bufwrite, strlen(bufwrite));
    printf("Number of bytes written to file : %d\n", r_write);
    close(fd);
    char *bufread = (char*)malloc(10000);
    fd = open("mount/a/a.txt", O_RDWR);
    if (fd < 0)
        fprintf(stderr, "%d\n", errno);
    assert(fd > 0);
    int r_read = pread(fd, bufread, 10000, 0);
    printf("Number of bytes read from file : %d\n", r_read);
    close(fd);
    if (strcmp(bufwrite, bufread)==0)
        printf("modifying file SUCCESS\n");
    else{
        printf("modifying file FAILED\n");
    }
    free(bufread);

    printf("6. test clean\n");
    char* buffer = malloc(1024 * 1024 * 10);
    for (int i = 0; i < 1024 * 1024; ++i) buffer[i] = 'b';
    fd = open("mount/a/a.txt", W_RDWR);
    assert(fd > 0);
    for (int i = 0; i < 100; ++i) {
        int r_write = write(fd, buffer, strlen(buffer));
        printf("Number of bytes written to file : %d\n", r_write);
    }
    close(fd);
    free(buffer);
    printf("--------Test Clean: Completed--------\n");
}
