
char **tokenize(void *fb, char *word_buffer, int word_buffer_size,
                char sep_char, char quote_char, char comment_char,
                int *p_num_fields, int allow_embedded_newline,
                int *p_error_type);
