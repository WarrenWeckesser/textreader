
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
#include "rows.h"
#include "error_types.h"

int64_t str_to_int64(const char *p_item, int64_t int_min, int64_t int_max, int *error);
uint64_t str_to_uint64(const char *p_item, uint64_t uint_max, int *error);


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
    int tok_error_type;

    fb = new_file_buffer(f, -1);
    if (fb == NULL) {
        return -1;
    }
 
    row_count = 0;
    while ((result = tokenize(fb, word_buffer, WORD_BUFFER_SIZE,
                              delimiter, quote, comment, &num_fields, TRUE, &tok_error_type)) != NULL) {
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


int count_fields(FILE *f, char delimiter, char quote, char comment, int allow_embedded_newline)
{
    void *fb;
    int num_fields;
    char **result;
    char word_buffer[WORD_BUFFER_SIZE];
    int tok_error_type;

    fb = new_file_buffer(f, -1);
    if (fb == NULL) {
        return -1;
    }
 
    result = tokenize(fb, word_buffer, WORD_BUFFER_SIZE,
                      delimiter, quote, comment, &num_fields, TRUE, &tok_error_type);
    if (result == NULL) {
        num_fields = -1;
    }
    else {
        free(result);
    }

    del_file_buffer(fb, RESTORE_INITIAL);

    return num_fields;
}

/*
 *  XXX Work-in-progress: need a way to get the number of fields when the user gives
 *  a dtype that is not a structured array.  count_fields() is not done; it will have
 *  to duplicate the initial code in read_rows() (skip comments, skip skiprows, etc).
 *  Something like goto_first_row() could be written to eliminate repeated code.
 */
 /*
int count_fields(FILE *f, char delimiter, char quote, char comment, int allow_embedded_newline)
{
    void *fb;
    int row_count;
    int num_fields;
    char **result;
    char word_buffer[WORD_BUFFER_SIZE];
    int tok_error_type;

    fb = new_file_buffer(f, -1);
    if (fb == NULL) {
        return -1;
    }
 
    result = tokenize(fb, word_buffer, WORD_BUFFER_SIZE,
                      delimiter, quote, comment, &num_fields, TRUE, &tok_error_type)
    if (result == NULL) {
        num_fields = -1;
    }
    free(result);

    del_file_buffer(fb, RESTORE_INITIAL);

    return num_fields;
}
*/


/*
 *  XXX Handle errors in any of the functions called by read_rows().
 *
 *  XXX Currently *nrows must be at least 1.
 */

void *read_rows(FILE *f, int *nrows, char *fmt,
                char delimiter, char quote, char comment,
                char sci, char decimal,
                int allow_embedded_newline,
                char *datetime_fmt,
                int tz_offset,
                int32_t *usecols, int num_usecols,
                int skiprows,
                void *data_array,
                int *p_error_type, int *p_error_lineno)
{
    void *fb;
    char *data_ptr;
    int num_fields, current_num_fields;
    char **result;
    int fmt_nfields;
    field_type *ftypes;
    int size;
    int row_count;
    int j;
    int *valid_usecols;
    char word_buffer[WORD_BUFFER_SIZE];
    int tok_error_type;

    *p_error_type = 0;
    *p_error_lineno = 0;

    if (datetime_fmt == NULL || strlen(datetime_fmt) == 0) {
        datetime_fmt = "%Y-%m-%d %H:%M:%S";
    }

    size = (*nrows) * calc_size(fmt, &fmt_nfields);

    ftypes = enumerate_fields(fmt);  /* Must free this when finished. */
    if (ftypes == NULL) {
        /* Out of memory. */
        *p_error_type = READ_ERROR_OUT_OF_MEMORY;
        return NULL;
    }

    /*
    for (k = 0; k < fmt_nfields; ++k) {
        printf("k = %d  typechar = '%c'  size = %d\n", k, ftypes[k].typechar, ftypes[k].size);
    }
    printf("size = %d\n", size);
    printf("-----\n");
    */

    if (data_array == NULL) {
        /* XXX The case where data_ptr is allocated here is untested. */
        data_ptr = malloc(size);
    }
    else {
        data_ptr = data_array;
    }

    fb = new_file_buffer(f, -1);
    if (fb == NULL) {
        free(ftypes);
        *p_error_type = ERROR_OUT_OF_MEMORY;
        return NULL;
    }

    /* XXX Check interaction of skiprows with comments. */
    while ((skiprows > 0) && ((result = tokenize(fb, word_buffer, WORD_BUFFER_SIZE,
                              delimiter, quote, comment, &num_fields, TRUE, &tok_error_type)) != NULL)) {
        if (result == NULL) {
            break;
        }
        free(result);
        --skiprows;
    }

    if (skiprows > 0) {
        /* There were fewer rows in the file than skiprows. */
        /* This is not treated as an error. The result should be an empty array. */
        *nrows = 0;
        free(ftypes);
        del_file_buffer(fb, RESTORE_FINAL);
        return data_ptr;
    }

    /* XXX Assume *nrows > 0! */
    /*
     *  Read the first row to get the number of fields in the file.
     *  We'll then use this to pre-validate the values in usecols.
     *  (It might be easier to do this in the Python wrapper, but that
     *  would require refactoring the C interface a bit to expose more
     *  to Python.)
     */
    row_count = 0;
    result = tokenize(fb, word_buffer, WORD_BUFFER_SIZE,
                              delimiter, quote, comment, &num_fields, TRUE, &tok_error_type);
    if (result == NULL) {
        *p_error_type = tok_error_type;
        *p_error_lineno = 1;
        free(ftypes);
        del_file_buffer(fb, RESTORE_FINAL);
        return NULL;
    }

    valid_usecols = (int *) malloc(num_usecols * sizeof(int));
    if (valid_usecols == NULL) {
        /* Out of memory. */
        *p_error_type = ERROR_OUT_OF_MEMORY;
        free(result);
        free(ftypes);
        del_file_buffer(fb, RESTORE_FINAL);
        return NULL;
    }

    /*
     *  Validate the column indices in usecols, and put the validated
     *  column indices in valid_usecols.
     */
    for (j = 0; j < num_usecols; ++j) {

        int32_t k;
        k = usecols[j];
        if (k < -num_fields || k >= num_fields) {
            /* Invalid column index. */
            *p_error_type = ERROR_INVALID_COLUMN_INDEX;
            *p_error_lineno = j;  /* Abuse 'lineno' and put the bad column index there. */
            free(valid_usecols);
            free(result);
            free(ftypes);
            del_file_buffer(fb, RESTORE_FINAL);
            return NULL;
        }
        if (k < 0) {
            k += num_fields;
        }
        valid_usecols[j] = k;
    }

    current_num_fields = num_fields;
    row_count = 0;
    do {
        int j, k;
        int item_type;
        double x;
        long long m;

        if (current_num_fields != num_fields) {
            *p_error_type = ERROR_CHANGED_NUMBER_OF_FIELDS;
            *p_error_lineno = line_number(fb);
            break;
        }

        for (j = 0; j < num_usecols; ++j) {

            int error;
            char typ = ftypes[j].typechar;
            /* k is the column index of the field in the file. */
            k = valid_usecols[j];

            /* XXX Handle error != 0 in the following cases. */
            if (typ == 'b') {
                int8_t x = (int8_t) str_to_int64(result[k], INT8_MIN, INT8_MAX, &error);
                *(int8_t *) data_ptr = x;
                data_ptr += ftypes[j].size;
            }
            else if (typ == 'B') {
                uint8_t x = (uint8_t) str_to_uint64(result[k], UINT8_MAX, &error);
                *(uint8_t *) data_ptr = x;
                data_ptr += ftypes[j].size;   
            }
            else if (typ == 'h') {
                int16_t x = (int16_t) str_to_int64(result[k], INT16_MIN, INT16_MAX, &error);
                *(int16_t *) data_ptr = x;
                data_ptr += ftypes[j].size;
            }
            else if (typ == 'H') {
                uint16_t x = (uint16_t) str_to_uint64(result[k], UINT16_MAX, &error);
                *(uint16_t *) data_ptr = x;
                data_ptr += ftypes[j].size;    
            }
            else if (typ == 'i') {
                int32_t x = (int32_t) str_to_int64(result[k], INT32_MIN, INT32_MAX, &error);
                *(int32_t *) data_ptr = x;
                data_ptr += ftypes[j].size;   
            }
            else if (typ == 'I') {
                uint32_t x = (uint32_t) str_to_uint64(result[k], UINT32_MAX, &error);
                *(uint32_t *) data_ptr = x;
                data_ptr += ftypes[j].size;   
            }
            else if (typ == 'q') {
                int64_t x = (int64_t) str_to_int64(result[k], INT64_MIN, INT64_MAX, &error);
                *(int64_t *) data_ptr = x;
                data_ptr += ftypes[j].size; 
            }
            else if (typ == 'Q') {
                uint64_t x = (uint64_t) str_to_uint64(result[k], UINT64_MAX, &error);
                *(uint64_t *) data_ptr = x;
                data_ptr += ftypes[j].size;    
            }
            else if (typ == 'f' || typ == 'd') {
                // Convert to float.
                double x;
                if ((strlen(result[k]) == 0) || !to_double(result[k], &x, sci, decimal)) {
                    // XXX  Find the canonical platform-independent method to assign nan.
                    x = 0.0 / 0.0;
                }
                if (typ == 'f')
                    *(float *) data_ptr = (float) x;
                else
                    *(double *) data_ptr = x;
                data_ptr += ftypes[j].size;
            }
            else if (typ == 'c' || typ == 'z') {
                // Convert to complex.
                double x, y;
                if ((strlen(result[k]) == 0) || !to_complex(result[k], &x, &y, sci, decimal)) {
                    // XXX  Find the canonical platform-independent method to assign nan.
                    x = 0.0 / 0.0;
                    y = x;
                }
                if (typ == 'c') {
                    *(float *) data_ptr = (float) x;
                    data_ptr += ftypes[j].size / 2;
                    *(float *) data_ptr = (float) y;
                }
                else {
                    *(double *) data_ptr = x;
                    data_ptr += ftypes[j].size / 2; 
                    *(double *) data_ptr = y;
                }
                data_ptr += ftypes[j].size / 2;
            }
            else if (typ == 'U') {
                // Datetime64, microseconds.
                struct tm tm = {0,0,0,0,0,0,0,0,0};
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
                        *(uint64_t *) data_ptr = (long long) (t - tz_offset) * 1000000L;
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
    } while ((row_count < *nrows) && (result = tokenize(fb, word_buffer, WORD_BUFFER_SIZE,
                              delimiter, quote, comment, &current_num_fields, TRUE, &tok_error_type)) != NULL);

    del_file_buffer(fb, RESTORE_FINAL);

    *nrows = row_count;

    free(valid_usecols);

    return (void *) data_ptr;
}
