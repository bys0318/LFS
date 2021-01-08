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
    printf("6. test clean\n");
    char* buffer = malloc(1024 * 1024 * 9 + 1);
    for (int i = 0; i < 24; ++i) {
        for (int j = 0; j < 1024 * 1024 * 9 + 1; ++j) buffer[j] = (char) ('0' + (i % 10));
        FILE *file = fopen("mount/a.txt", "r+");
        fseek(file, 0, 0);
        fwrite(buffer, strlen(buffer), 1, file);
        fclose(file);
        printf("success write %d times\n", i + 1);
    }
    free(buffer);
    printf("--------Test Clean: Completed--------\n");
}
