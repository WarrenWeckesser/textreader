
#define FILE_BUFFER_SIZE 16777216

#define FB_EOF   -1
#define FB_ERROR -2

typedef struct _file_buffer {
    FILE *file;
    int line_number;
    int current_pos;
    int last_pos;
    int reached_eof;
    char buffer[FILE_BUFFER_SIZE];
} file_buffer;

file_buffer *new_file_buffer(FILE *f);
void fb_dump(file_buffer *fb);
int fetch(file_buffer *fb);
int _next(file_buffer *fb);
