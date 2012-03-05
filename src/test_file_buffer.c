

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "file_buffer.h"


int test1()
{
    FILE *f;
    int len;
    char *pattern = "0123456789";
    void *fb;
    int count;
    int c;
    int k;
    long filepos;
    int fail = 0;

    len = strlen(pattern);

    /* Create a test file. */
    f = fopen("tmp.dat", "wb");
    for (k = 0; k < 3; ++k) {
        fputs(pattern, f);
    }
    fclose(f);

    f = fopen("tmp.dat", "rb");
    fb = new_file_buffer(f, 7);
    count = 0;
    while ((c = fetch(fb)) != FB_EOF) {
        int pattern_index, pc;
        pattern_index = count % len;
        pc = pattern[pattern_index];
        if (pc != c) {
            printf("test1: error: count=%d, c='%c', pc='%c'\n", count, c, pc);
            fail = 1;
            break;
        }
        ++count;
    }
    del_file_buffer(fb, RESTORE_INITIAL);
    /*
     *  The file buffer was deleted with RESTORE_INITIAL, so the position
     *  of the underlying file f should be back where it was when new_file_buffer()
     *  was called, which is 0.
     */
    filepos = ftell(f);
    if (filepos != 0) {
        printf("test1: error: file position not restored to 0");
        fail = 1;
    }
    if (!fail) {
        printf("test1 passed.\n");
    }
    unlink("tmp.dat");
    return fail;
}

int test2()
{
    FILE *f;
    char *pattern = "0123456789";
    void *fb;
    int count;
    int len;
    int c;
    int k;
    long filepos;
    int fail = 0;

    /* Create a test file. */
    f = fopen("tmp.dat", "wb");
    for (k = 0; k < 5; ++k) {
        fputs(pattern, f);
    }
    fclose(f);

    len = strlen(pattern);

    f = fopen("tmp.dat", "rb");
    fb = new_file_buffer(f, 15);
    count = 20;
    for (k = 0; k < count; ++k) {
        int pattern_index, pc;
        c = fetch(fb);
        pattern_index = k % len;
        pc = pattern[pattern_index];
        if (pc != c) {
            printf("test2: error: k=%d, c='%c', pc='%c'\n", k, c, pc);
            fail = 1;
        }
    }
    del_file_buffer(fb, RESTORE_FINAL);

    /*
     *  The file buffer was deleted with RESTORE_FINAL, and `count` characters
     *  were read using fetch(), so the file position should now be `count`.
     */
    filepos = ftell(f);
    if (filepos != count) {
        printf("test2: error: file position (%ld) not restored to %d\n", filepos, count);
        fail |= 1;
    }
    if (!fail) {
        printf("test2 passed.\n");
    }
    unlink("tmp.dat");
    return fail;
}


int main(int argc, char *argvp[])
{
    int fail;

    fail = test1();
    fail |= test2();
    return fail;
}
