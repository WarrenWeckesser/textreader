
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>

#include "sizes.h"
#include "constants.h"

double xstrtod(char *p, char **q, int decimal, int sci, int skip_trailing);


/*
 *  `item` must be the nul-terminated string that is to be
 *  converted to a double.
 *
 *  To be successful, to_double() must use *all* the characters
 *  in `item`.  E.g. "1.q25" will fail.  Leading and trailing 
 *  spaces are allowed.
 *
 *  `sci` is the scientific notation exponent character, usually
 *  either 'E' or 'D'.  Case is ignored.
 *
 *  `decimal` is the decimal point character, usually either
 *  '.' or ','.
 *
 */

int to_double(char *item, double *p_value, char sci, char decimal)
{
    char *p_end;

    *p_value = xstrtod(item, &p_end, decimal, sci, TRUE);

    return (errno == 0) && (!*p_end);
}


int to_complex(char *item, double *p_real, double *p_imag, char sci, char decimal)
{
    char *p_end;

    *p_real = xstrtod(item, &p_end, decimal, sci, FALSE);
    if (*p_end == '\0') {
        *p_imag = 0.0;
        return errno == 0;
    }
    if (*p_end == 'i' || *p_end == 'j') {
        *p_imag = *p_real;
        *p_real = 0.0;
        ++p_end;
    }
    else {
        if (*p_end == '+') {
            ++p_end;
        }
        *p_imag = xstrtod(p_end, &p_end, decimal, sci, FALSE);
        if (errno || ((*p_end != 'i') && (*p_end != 'j'))) {
            return FALSE;
        }
        ++p_end;
    }
    while(*p_end == ' ') {
        ++p_end;
    }
    return *p_end == '\0';
}


int to_longlong(char *item, long long *p_value)
{
    char *p_end;

    // Try integer conversion.  We explicitly give the base to be 10. If
    // we used 0, strtoll() would convert '012' to 10, because the leading 0 in
    // '012' signals an octal number in C.  For a general purpose reader, that
    // would be a bug, not a feature.
    *p_value = strtoll(item, &p_end, 10);

    // Allow trailing spaces.
    while (isspace(*p_end)) ++p_end;

    return (errno == 0) && (!*p_end);
}


#ifdef TEST

int main(int argc, char *argv[])
{
    double x, y;
    int status;
    char *s;

    //s = "0.10e-3-+5.5e2i";
    s = "1-0j";
    status = to_complex(s, &x, &y, 'e', '.');
    printf("s = '%s'\n", s);
    printf("status = %d\n", status);
    printf("x = %lg,  y = %lg\n", x, y);

    return 0;
}
#endif