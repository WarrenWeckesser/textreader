
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "file_buffer.h"

#ifdef __APPLE__
#define fseeko64 fseek
#define ftello64 ftell
#define off64_t off_t
#endif

typedef struct _file_buffer {

    FILE *file;

    /* Size of the file, in bytes. */
    off64_t size;

    /* file position when the file_buffer was created. */
    off64_t initial_file_pos;

    int line_number;

    int fileno;
    off64_t current_pos;
    off64_t last_pos;
    char *memmap;

} file_buffer;


/*
 * XXX Currently the entire file is memory mapped.  It might be better
 *     to map blocks of the file sequentially.  Or, as least advise the OS
 *     occasionally that pages that we're finished with can be released.
 */


/*
 *  file_buffer *new_file_buffer(FILE *f)
 *
 *  Allocate a new file_buffer.
 *  Returns NULL if the memory allocation fails or if the call to mmap fails.
 */

file_buffer *new_file_buffer(FILE *f)
{
    struct stat buf;
    int fd;
    file_buffer *fb;
    off64_t current_pos;
    off64_t filesize;

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
    fb->size = (off64_t) filesize;
    fb->line_number = 0;  // XXX Maybe more natural to start at 1?

    fb->fileno = fd;
    fb->current_pos = ftello64(f);
    fb->last_pos = (off64_t) filesize;

    fb->memmap = mmap(NULL, filesize, PROT_READ, MAP_FILE|MAP_SHARED, fd, 0);
    if (cust->memmap == NULL) {
        /* XXX Eventually remove this print statement. */
        fprintf(stderr, "new_file_buffer: mmap() failed.\n");
        free(cust);
        free(fb);
        fb = NULL;
    }

    fb->custom = (void *) cust;

    return fb;
}


void del_file_buffer(file_buffer *fb, int restore)
{
    custom *cust = (custom *) fb->custom;


    munmap(cust->memmap, fb->size);

    if (restore == RESTORE_INITIAL) {
        /* XXX Is this necessary?  Has the file position moved? */
        /* No, it should not have moved... */
        //fseeko64(fb->file, fb->initial_file_pos, SEEK_SET);
    }
    else if (restore == RESTORE_FINAL) {
        printf("del_file_buffer: current_pos = %lld\n", cust->current_pos);
        fseeko64(fb->file, cust->current_pos, SEEK_SET);
    }
    free(cust);
    free(fb);
}

/*
 *  int fetch(file_buffer *fb)
 *
 *  Get a single character from the buffer, and advance the buffer pointer.
 *
 *  Returns FB_EOF when the end of the file is reached.
 *  The sequence '\r\n' is treated as a single '\n'.  That is, when the next
 *  two bytes in the buffer are '\r\n', the buffer pointer is advanced by 2
 *  and '\n' is returned.
 *  When '\n' is returned, fb->line_number is incremented.
 */

int fetch(file_buffer *fb)
{
    char c;
    custom *cust = (custom *) fb->custom;
    
    if (cust->current_pos == cust->last_pos) {
        return FB_EOF;
    }

    if (cust->current_pos + 1 < cust->last_pos && cust->memmap[cust->current_pos] == '\r'
          && cust->memmap[cust->current_pos + 1] == '\n') {
        c = '\n';
        cust->current_pos += 2;
    } else {
        c = cust->memmap[cust->current_pos];
        cust->current_pos += 1;
    }
    if (c == '\n') {
        fb->line_number++;
    }
    return c;
}


/*
 *  int next(file_buffer *fb)
 *
 *  Returns the next byte in the buffer, but does not advance the pointer.
 */

int next(file_buffer *fb)
{
    custom *cust = (custom *) fb->custom;

    if (cust->current_pos + 1 >= cust->last_pos)
        return FB_EOF;
    else
        return cust->memmap[cust->current_pos];
}


/*
 *  skipline(file_buffer *fb)
 *
 *  Read bytes from the buffer until a newline or the end of the file is reached.
 */

void skipline(file_buffer *fb)
{
    while (next(fb) != '\n' && next(fb) != FB_EOF) {
        fetch(fb);
    }
    if (next(fb) == '\n') {
        fetch(fb);
    }
}
