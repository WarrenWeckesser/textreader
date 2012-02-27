

#define FB_EOF   -1
#define FB_ERROR -2

typedef struct _file_buffer {

    FILE *file;
    int fileno;
    long long int size;  /* Size of the file, in bytes. */
    void *bookmark;

    int line_number;
    long long int current_pos;
    long long int last_pos;
    int reached_eof;
    long long int buffer_size;
    char *buffer;

} file_buffer;

file_buffer *new_file_buffer(FILE *f);
void del_file_buffer(file_buffer *fb);
void fb_dump(file_buffer *fb);
void set_bookmark(file_buffer *fb);
void goto_bookmark(file_buffer *fb);
int fetch(file_buffer *fb);
int next(file_buffer *fb);
