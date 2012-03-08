
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "file_buffer.h"


typedef struct _file_buffer {

    FILE *file;

    /* Size of the file, in bytes. */
    off_t size;

    /* file position when the file_buffer was created. */
    off_t initial_file_pos;

    int line_number;

    int fileno;
    off_t current_pos;
    off_t last_pos;
    char *memmap;

} file_buffer;

#define FB(fb)  ((file_buffer *)fb)


/*
 * XXX Currently the entire file is memory mapped.  It might be better
 *     to map blocks of the file sequentially.  Or, as least advise the OS
 *     occasionally that pages that we're finished with can be released.
 */


/*
 *  void *new_file_buffer(FILE *f, int buffer_size)
 *
 *  Allocate a new file_buffer.
 *  Returns NULL if the memory allocation fails or if the call to mmap fails.
 *
 *  buffer_size is ignored.
 */

void *new_file_buffer(FILE *f, int buffer_size)
{
    struct stat buf;
    int fd;
    file_buffer *fb;
    off_t current_pos;
    off_t filesize;

    fd = fileno(f);
    if (fstat(fd, &buf) == -1) {
        fprintf(stderr, "new_file_buffer: fstat() failed. errno =%d\n", errno);
        return NULL;
    }
    filesize = buf.st_size;  /* XXX This might be 32 bits. */

    fb = (file_buffer *) malloc(sizeof(file_buffer));
    if (fb == NULL) {
        /* XXX Eventually remove this print statement. */
        fprintf(stderr, "new_file_buffer: malloc() failed.\n");
        return NULL;
    }
    fb->file = f;
    fb->size = (off_t) filesize;
    fb->line_number = 0;  // XXX Maybe more natural to start at 1?

    fb->fileno = fd;
    fb->current_pos = ftell(f);
    fb->last_pos = (off_t) filesize;

    fb->memmap = mmap(NULL, filesize, PROT_READ, MAP_SHARED, fd, 0);
    if (fb->memmap == NULL) {
        /* XXX Eventually remove this print statement. */
        fprintf(stderr, "new_file_buffer: mmap() failed.\n");
        free(fb);
        fb = NULL;
    }

    return fb;
}


void del_file_buffer(void *fb, int restore)
{
    munmap(FB(fb)->memmap, FB(fb)->size);

    /*
     *  With a memory mapped file, there is no need to do
     *  anything if restore == RESTORE_INITIAL.
     */
    if (restore == RESTORE_FINAL) {
        fseek(FB(fb)->file, FB(fb)->current_pos, SEEK_SET);
    }
    free(fb);
}


inline int line_number(void *fb)
{
    return FB(fb)->line_number;
}

/*
 *  int fetch(void *fb)
 *
 *  Get a single character from the buffer, and advance the buffer pointer.
 *
 *  Returns FB_EOF when the end of the file is reached.
 *  The sequence '\r\n' is treated as a single '\n'.  That is, when the next
 *  two bytes in the buffer are '\r\n', the buffer pointer is advanced by 2
 *  and '\n' is returned.
 *  When '\n' is returned, fb->line_number is incremented.
 */

int fetch(void *fb)
{
    char c;
    
    if (FB(fb)->current_pos == FB(fb)->last_pos) {
        return FB_EOF;
    }

    if (FB(fb)->current_pos + 1 < FB(fb)->last_pos && FB(fb)->memmap[FB(fb)->current_pos] == '\r'
          && FB(fb)->memmap[FB(fb)->current_pos + 1] == '\n') {
        c = '\n';
        FB(fb)->current_pos += 2;
    } else {
        c = FB(fb)->memmap[FB(fb)->current_pos];
        FB(fb)->current_pos += 1;
    }
    if (c == '\n') {
        FB(fb)->line_number++;
    }
    return c;
}


/*
 *  int next(void *fb)
 *
 *  Returns the next byte in the buffer, but does not advance the pointer.
 */

int next(void *fb)
{
    if (FB(fb)->current_pos + 1 >= FB(fb)->last_pos)
        return FB_EOF;
    else
        return FB(fb)->memmap[FB(fb)->current_pos];
}


/*
 *  void skipline(void *fb)
 *
 *  Read bytes from the buffer until a newline or the end of the file is reached.
 */

void skipline(void *fb)
{
    while (next(fb) != '\n' && next(fb) != FB_EOF) {
        fetch(fb);
    }
    if (next(fb) == '\n') {
        fetch(fb);
    }
}
