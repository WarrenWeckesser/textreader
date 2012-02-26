
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "field_type.h"

/*
 *  Format characters for data types:
 *    h : 16 bit signed integer
 *    H : 16 bit unsigned integer
 *    i : 32 bit signed integer
 *    I : 32 bit unsigned integer
 *    q : 64 bit signed integer
 *    Q : 64 bit unsigned integer
 *    f : 32 bit floating point
 *    d : 64 bit floating point
 *    s : character
 *    U : 64 bit datetime (very experimental)
 */


/*
 *  int calc_size(char *fmt, int *p_nfields)
 *
 *  Calculate the sum of the sizes of the data types
 *  specified in the given data type format string.
 *  Returns -1 when fmt is invalid.
 *
 *  Examples:
 *     calc_size("id")     -> 12 (32 bit int, 64 bit double)
 *     calc_size("4f10s")  -> 26 (4 32 bit floats, string with length 10)
 *     calc_size("2d2s")   -> 18 (2 64 bit doubles, string with length 2)
 *     calc_size("")       ->  0
 *     calc_size("4f10") -  > -1 (invalid)
 *     calc_size("p")      -> -1 (invalid)
 */

int calc_size(char *fmt, int *p_nfields)
{
    int size;
    unsigned long repcount;
    unsigned long slen;
    char *p, *p_end;
 
    size = 0;
    if (p_nfields != NULL)
        *p_nfields = 0;
    p = fmt;
    while (*p) {
        errno = 0;
        repcount = strtol(p, &p_end, 10);
        if (p_end == p) {
            repcount = 1;
        }
        errno = 0;
        p = p_end;
        if (*p == 'h' || *p == 'H') {
            size += repcount * 2;
            ++p;
        }
        else if (*p == 'i' || *p == 'I' || *p == 'f') {
            size += repcount * 4;
            ++p;
        }
        else if (*p == 'q' || *p == 'Q' || *p == 'd' || *p == 'U') {
            size += repcount * 8;
            ++p;
        }
        else if (*p == 's') {
            ++p;
            size += repcount;
            repcount = 1;
        }
        else {
            size = -1;
            break;
        }
        if (p_nfields != NULL)
            *p_nfields += repcount;
    }
    return size;
}


field_type *enumerate_fields(char *fmt)
{
    int item_size, fmt_size;
    unsigned long repcount;
    unsigned long slen;
    char *p, *p_end;
    int nfields;
    field_type *result;
    int field;
    char c;
    int k;

    fmt_size = calc_size(fmt, &nfields);
    result = (field_type *) malloc(nfields * sizeof(field_type));
    if (result == NULL) {
        printf("enumerate_fields: out of memory\n");
        return NULL;
    }

    field = 0;
    p = fmt;
    while (*p) {
        errno = 0;
        repcount = strtol(p, &p_end, 10);
        if (p_end == p) {
            errno = 0;
            repcount = 1;
        }
        errno = 0;
        p = p_end;
        c = *p;
        if (c == 'h' || c == 'H') {
            item_size = 2;
            ++p;
        }
        else if (c == 'i' || c == 'I' || c == 'f') {
            item_size = 4;
            ++p;
        }
        else if (c == 'q' || c == 'Q' || c == 'd' || c == 'U') {
            item_size = 8;
            ++p;
        }
        else if (c == 's') {
            item_size = repcount;
            repcount = 1;
            ++p;
        }
        else {
            // XXX handle this better!
            item_size = -1;
            break;
        }
        for (k = field; k < field + repcount; ++k) {
            result[k].typechar = c;
            result[k].size = item_size;
        }
        field += repcount;
    }
    return result;
}
