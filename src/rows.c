
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <errno.h>

#include "file_buffer.h"
#include "tokenize.h"
#include "sizes.h"
#include "conversions.h"
#include "constants.h"
#include "fields.h"

/*
 *  XXX Might want to couple count_rows() with read_rows() to avoid duplication
 *      of some file I/O.
 */


/*
 *  WORD_BUFFER_SIZE determines the maximum amount of non-delimiter
 *  text in a row.
 */
#define WORD_BUFFER_SIZE 4000

/*
 *
 *  int count_rows(FILE *f, char delimiter, char quote, char comment, int allow_embedded_newline)
 *
 *  Negative return values indicate an error.
 *
 *  XXX Need a mechanism to pass more error information back to the caller.
 */

int count_rows(FILE *f, char delimiter, char quote, char comment, int allow_embedded_newline)
{
    void *fb;
    int row_count;
    int num_fields;
    char **result;
    char word_buffer[WORD_BUFFER_SIZE];

    fb = new_file_buffer(f, -1);
    if (fb == NULL) {
        return -1;
    }
 
    row_count = 0;
    while ((result = tokenize(fb, word_buffer, WORD_BUFFER_SIZE,
                              delimiter, quote, comment, &num_fields, TRUE)) != NULL) {
        if (result == NULL) {
            row_count = -1;
            break;
        }
        free(result);
        ++row_count;
    }

    del_file_buffer(fb, RESTORE_INITIAL);

    return row_count;
}


/*
 *  XXX Handle errors in any of the functions called by read_rows().
 */

void *read_rows(FILE *f, int *nrows, char *fmt,
                char delimiter, char quote, char comment,
                int allow_embedded_newline,
                char *datetime_fmt,
                int *usecols, int num_usecols,
                void *data_array)
{
    void *fb;
    char *data_ptr;
    int num_fields;
    char **result;
    int fmt_nfields;
    field_type *ftypes;
    int size;
    int row_count;
    int k;
    char word_buffer[WORD_BUFFER_SIZE];

    if (datetime_fmt == NULL || strlen(datetime_fmt) == 0) {
        datetime_fmt = "%Y-%m-%d %H:%M:%S";
    }

    size = (*nrows) * calc_size(fmt, &fmt_nfields);

    ftypes = enumerate_fields(fmt);

    /*
    for (k = 0; k < fmt_nfields; ++k) {
        printf("k = %d  typechar = '%c'  size = %d\n", k, ftypes[k].typechar, ftypes[k].size);
    }
    printf("size = %d\n", size);
    printf("-----\n");
    */

    if (data_array == NULL) {
        /* XXX The case where data_array is allocated here is untested. */
        data_array = malloc(size);
    }
    data_ptr = data_array;

    fb = new_file_buffer(f, -1);
    row_count = 0;
    while ((row_count < *nrows) && (result = tokenize(fb, word_buffer, WORD_BUFFER_SIZE,
                              delimiter, quote, comment, &num_fields, TRUE)) != NULL) {
        int j, k;
        int item_type;
        double x;
        long long m;
        for (j = 0; j < num_usecols; ++j) {
            k = usecols[j];
            if (ftypes[j].typechar == 'q' || ftypes[j].typechar == 'i' || ftypes[j].typechar == 'h') {
                // Convert to int.
                long long x;
                if (to_longlong(result[k], &x)) {
                    if (ftypes[j].typechar == 'i')
                        *(int32_t *) data_ptr = (int32_t) x;
                    else if (ftypes[j].typechar == 'h')
                        *(int16_t *) data_ptr = (int16_t) x;
                    else
                        *(int64_t *) data_ptr = (int64_t) x;
                }
                else {
                    // Conversion failed.  Fill with 0.
                    memset(data_ptr, 0, ftypes[j].size);
                }
                data_ptr += ftypes[j].size;
            }
            else if (ftypes[j].typechar == 'f' || ftypes[j].typechar == 'd') {
                // Convert to float.
                double x;
                if ((strlen(result[k]) == 0) || !to_double(result[k], &x)) {
                    // XXX  Find the canonical platform-independent method to assign nan.
                    x = 0.0 / 0.0;
                }
                if (ftypes[j].typechar == 'f')
                    *(float *) data_ptr = (float) x;
                else
                    *(double *) data_ptr = x;
                data_ptr += ftypes[j].size;
            }
            else if (ftypes[j].typechar == 'U') {
                // Datetime64, microseconds.
                // Hard-coded format, just for testing.
                struct tm tm;
                time_t t;
                if (strptime(result[k], datetime_fmt, &tm) == NULL) {
                    memset(data_ptr, 0, 8);
                }
                else {
                    tm.tm_isdst = -1;
                    t = mktime(&tm);
                    if (t == -1) {
                        memset(data_ptr, 0, 8);
                    }
                    else {
                        *(uint64_t *) data_ptr = (long long) t * 1000000L;
                    }
                }
                data_ptr += 8;
            }
            else {
                // String
                strncpy(data_ptr, result[k], ftypes[j].size);
                data_ptr += ftypes[j].size;
            }
        }
        free(result);
        ++row_count;
    }

    del_file_buffer(fb, RESTORE_FINAL);
    *nrows = row_count;

    return (void *) data_array;
}
