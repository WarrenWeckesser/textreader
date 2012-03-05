

#define FB_EOF   -1
#define FB_ERROR -2

#define RESTORE_NOT     0
#define RESTORE_INITIAL 1
#define RESTORE_FINAL   2

/*
 *  This is the API used to access a file.
 *  All the code in rows.c and tokenize.c accesses the
 *  file using these four functions.
 *
 *  The pointer returned by new_file_buffer() is intentionally
 *  opaque.  An implementation of this interface may define it
 *  however it finds necessary.
 *
 */

void *new_file_buffer(FILE *f, int buffer_size);

/*
 * restore:
 *  RESTORE_NOT     (0):
 *      Free memory, but leave the file position wherever it
 *      happend to be.
 *  RESTORE_INITIAL (1):
 *      Restore the file position to the location at which
 *      the file_buffer was created.
 *  RESTORE_FINAL   (2):
 *      Put the file position at the next byte after the
 *      data read from the file_buffer.
 */
void del_file_buffer(void *fb, int restore);

int line_number(void *fb);

int fetch(void *fb);
int next(void *fb);
