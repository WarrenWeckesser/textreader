
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "file_buffer.h"

#ifdef __APPLE__
#define fseeko64 fseek
#define ftello64 ftell
#define off64_t off_t
#endif

#define DEFAULT_BUFFER_SIZE 16777216

typedef struct _file_buffer {

    FILE *file;

    /* Size of the file, in bytes. */
    off64_t size;

    /* file position when the file_buffer was created. */
    off64_t initial_file_pos;

    int line_number;

    /* Boolean: has the end of the file been reached? */
    int reached_eof;

    /* Offset in the file of the data currently in the buffer. */
    off64_t buffer_file_pos;

    /* Position in the buffer of the next character to read. */
    off64_t current_buffer_pos;

    /* Actual number of bytes in the current buffer. (Can be less then buffer_size.) */
    off64_t last_pos;

    /* Size (in bytes) of the buffer. */
    off64_t buffer_size;

    /* Pointer to the buffer. */
    char *buffer;

} file_buffer;

#define FB(fb)  ((file_buffer *)fb)


/*
 *  void *new_file_buffer(FILE *f, int buffer_size)
 *
 *  Allocate a new file_buffer.
 *  Returns NULL if the memory allocation fails.
 */

void *new_file_buffer(FILE *f, int buffer_size)
{
    file_buffer *fb;


    fb = (file_buffer *) malloc(sizeof(file_buffer));
    if (fb == NULL) {
        fprintf(stderr, "new_file_buffer: malloc() failed.\n");
        return NULL;
    }

    fb->file = f;
    fb->initial_file_pos = ftello64(f);

    fb->line_number = 0;  // XXX Maybe more natural to start at 1?

    fb->buffer_file_pos = fb->initial_file_pos;

    fb->current_buffer_pos = 0;
    fb->last_pos = 0;

    fb->reached_eof = 0;

    if (buffer_size < 1) {
        buffer_size = DEFAULT_BUFFER_SIZE;
    }

    fb->buffer_size = buffer_size;
    fb->buffer = malloc(fb->buffer_size);
    if (fb->buffer == NULL) {
        fprintf(stderr, "new_file_buffer: malloc() failed.\n");
        free(fb);
        fb = NULL;
    }

    return (void *) fb;
}


void del_file_buffer(void *fb, int restore)
{
    if (restore == RESTORE_INITIAL) {
        fseeko64(FB(fb)->file, FB(fb)->initial_file_pos, SEEK_SET);
    }
    else if (restore == RESTORE_FINAL) {
        fseeko64(FB(fb)->file, FB(fb)->buffer_file_pos + FB(fb)->current_buffer_pos, SEEK_SET);
    }
    free(FB(fb)->buffer);
    free(fb);
}


/*
 *  int _fb_load(void *fb)
 *
 *  Get data from the file into the buffer.
 *
 */

int _fb_load(void *fb)
{
    char *buffer = FB(fb)->buffer;

    if (!FB(fb)->reached_eof && (FB(fb)->current_buffer_pos == FB(fb)->last_pos || FB(fb)->current_buffer_pos+1 == FB(fb)->last_pos)) {
        size_t num_read;
        /* k will be either 0 or 1. */
        int k = FB(fb)->last_pos - FB(fb)->current_buffer_pos;
        if (k) {
            buffer[0] = buffer[FB(fb)->current_buffer_pos];
        }

        FB(fb)->buffer_file_pos  = ftello64(FB(fb)->file) - k;
        
        num_read = fread(&(buffer[k]), 1, FB(fb)->buffer_size - k, FB(fb)->file);

        FB(fb)->current_buffer_pos = 0;
        FB(fb)->last_pos = num_read + k;
        if (num_read < FB(fb)->buffer_size - k)
            if (feof(FB(fb)->file))
                FB(fb)->reached_eof = 1;
            else
                return FB_ERROR;
    }
    return 0;
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
    char *buffer = FB(fb)->buffer;
  
    _fb_load(fb);

    if (FB(fb)->current_buffer_pos == FB(fb)->last_pos)
        return FB_EOF;

    if ((FB(fb)->current_buffer_pos + 1 < FB(fb)->last_pos) && (buffer[FB(fb)->current_buffer_pos] == '\r')
          && (buffer[FB(fb)->current_buffer_pos + 1] == '\n')) {
        c = '\n';
        FB(fb)->current_buffer_pos += 2;
    } else {
        c = buffer[FB(fb)->current_buffer_pos];
        FB(fb)->current_buffer_pos += 1;
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

    _fb_load(fb);
    if (FB(fb)->current_buffer_pos + 1 >= FB(fb)->last_pos) {
        return FB_EOF;
    }
    else {
        int c;
        c = FB(fb)->buffer[FB(fb)->current_buffer_pos];
        return c;
    }
}


/*
 *  skipline(void *fb)
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
