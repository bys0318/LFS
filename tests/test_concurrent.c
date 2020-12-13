// gcc test_concurrent.c -o test
// Testing on concurrent processes accessing the file system
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
    printf("--------Test 3: Testing on concurrent processes accessing the file system--------\n");
    printf("test 1: two concurrent processes\n");
    pid_t pid = fork();
    if (pid > 0){ // parent writes
        int fd = open("mount/test_concurrent.txt", O_CREAT | O_RDWR, 0777);
        assert(fd > 0);
        char* buf = "I am your father";
        printf("Parent process ready to write\n");
        int w = write(fd, buf, strlen(buf));
        printf("Parent process write: I am your father\n");
        close(fd);
    }
    else{ // child reads
        int fd = open("mount/test_concurrent.txt", O_CREAT | O_RDWR, 0777);
        assert(fd > 0);
        char* buf = (char *)malloc(100);
        printf("Child process ready to read\n");
        int r = read(fd, buf, 100);
        printf("Child process read: %s...\n", buf);
        close(fd);
        exit(2);
    }
    sleep(1);
    printf("test 1 passed\n");
    
    printf("test 2: multiple concurrent processes\n");
    int i;
    for (i = 0; i < 10; i++){
        pid = fork();
        if (pid == 0 || pid == -1) break;
    }
    if (pid == 0){ // children write
        int fd = open("mount/test_concurrent1.txt", O_CREAT | O_RDWR | O_APPEND, 0777);
        assert(fd > 0);
        char* buf = "child process writing...\n";
        int w = write(fd, buf, strlen(buf));
        printf("child process writing...\n");
        close(fd);
        exit(2);
    }
    else{ // parent reads
        sleep(1);
        printf("parent process reading!\n");
        int fd = open("mount/test_concurrent1.txt", O_CREAT | O_RDWR, 0777);
        assert(fd > 0);
        char* childmsg = "child process writing...\n";
        char* buf = (char *)malloc(500);
        int r = read(fd, buf, 500);
        assert(strlen(buf) == 10 * strlen(childmsg));
        close(fd);
    }
    printf("test 2 passed\n");
    printf("--------Test 3: Completed--------\n");
}