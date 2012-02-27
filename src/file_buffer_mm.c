
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>

#include "file_buffer.h"


/*
 *  file_buffer *new_file_buffer(FILE *f)
 *
 *  Allocate a new file_buffer.
 *  Returns NULL if the memory allocation fails.
 */

file_buffer *new_file_buffer(FILE *f)
{
    struct stat buf;
    int fd;
    file_buffer *fb;
    long current_pos;
    off_t filesize;

    fd = fileno(f);
    fstat(fd, &buf);
    filesize = buf.st_size;  /* XXX This might be 32 bits. */
    current_pos = ftell(f);

    fb = (file_buffer *) malloc(sizeof(file_buffer));
    if (fb != NULL) {
        fb->file = f;
        fb->fileno = fd;
        fb->size = (long long int) filesize;
        fb->line_number = 0;  // XXX Maybe more natural to start at 1?
        fb->current_pos = current_pos;
        fb->last_pos = (long long int) filesize;
        fb->reached_eof = 0;
        fb->buffer_size = (long long int) filesize; // ?
        fb->buffer = mmap(NULL, filesize, PROT_READ, MAP_FILE|MAP_SHARED, fd, 0);
        if (fb->buffer == NULL) {
            fprintf(stderr, "uh oh...\n");
        }
        else {
            fprintf(stderr, "Created mmap\n");
        }
        fb->bookmark = NULL;
    }
    else {
        /*  XXX Temporary print statement. */
        printf("new_file_buffer: out of memory\n");
    }
    return fb;
}

void del_file_buffer(file_buffer *fb)
{
    free(fb->bookmark);
    munmap(fb->buffer, fb->buffer_size);
    free(fb);
}

/*
 *  Print the file buffer fields.
 */

void fb_dump(file_buffer *fb)
{
    printf("fb->file        = %p\n", (void *) fb->file);
    printf("fb->line_number = %d\n", fb->line_number);
    printf("fb->current_pos = %lld\n", fb->current_pos);
    printf("fb->last_pos    = %lld\n", fb->last_pos);
    printf("fb->reached_eof = %d\n", fb->reached_eof);
}


void set_bookmark(file_buffer *fb)
{
    if (fb->bookmark) {
        free(fb->bookmark);
    }
    fb->bookmark = (void *) malloc(sizeof(long long int));
    *(long long int *)(fb->bookmark) = fb->current_pos;
}


void goto_bookmark(file_buffer *fb)
{
    if (fb->bookmark == NULL) {
        fprintf(stderr, "error: bookmark has not been set.\n");
    }
    else {
        fb->current_pos = *(long long int *)(fb->bookmark);
    }
}


/*
 *  int _fb_load(file_buffer *fb)
 *
 *  Get data from the file into the buffer.
 *
 *  XXX This function needs some help--it can fall through without hitting a
 *      return statement.
 */

int _fb_load(file_buffer *fb)
{
    return 0;
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
    
    //printf("fetch: current_pos = %lld\n", fb->current_pos);

    if (fb->current_pos == fb->last_pos) {
        fb->reached_eof = 1;
        return FB_EOF;
    }

    if (fb->current_pos + 1 < fb->last_pos && fb->buffer[fb->current_pos] == '\r'
          && fb->buffer[fb->current_pos + 1] == '\n') {
        c = '\n';
        fb->current_pos += 2;
    } else {
        c = fb->buffer[fb->current_pos];
        fb->current_pos += 1;
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
    if (fb->current_pos+1 >= fb->last_pos)
        return FB_EOF;
    else
        return fb->buffer[fb->current_pos];
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
