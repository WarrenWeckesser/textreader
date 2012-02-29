

int count_rows(FILE *f, char delimiter, char quote, char comment, int allow_embedded_newline);

void *read_rows(FILE *f, int *nrows, char *fmt,
                char delimiter, char quote, char comment,
                char sci, char decimal,
                int allow_embedded_newline,
                char *datetime_fmt,
                int *usecols, int num_usecols,
                void *data_array);