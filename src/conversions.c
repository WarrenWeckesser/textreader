
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "sizes.h"
#include "constants.h"


int to_double(char *item, double *p_value)
{
    char *p_end;
    char tmp[FIELD_BUFFER_SIZE];
    char *p, *q;

    // Try float conversion with strtod().
    // FIXME: strtod can set errno--should check!
    //double x;
    *p_value = strtod(item, &p_end);
    if (p_end == item + strlen(item)) {
        return TRUE;
    }
    // Try float again, but first copy the string to tmp and convert 'd' or
    // 'D' to 'e' in the copy.  Some Fortran programs use a 'd' to indicate
    // the exponent.
    // XXX Perhaps this second test should only be performed if requested.
    //     What is the performance cost of this test?
    for (p = tmp, q = item; *q != '\0'; ++p, ++q) {
        if (*q == 'd' || *q == 'D') {
            *p = 'e';
        } else {
            *p = *q;
        }
    }
    *p = '\0';
    *p_value = strtod(tmp, &p_end);
    if (p_end == tmp + strlen(tmp)) {
        return TRUE;
    }
    return FALSE;
}

int to_longlong(char *item, long long *p_value)
{
    char *p_end;

    // Try integer conversion.  We explicity give the base to be 10. If
    // we used 0, strtoll() would convert '012' to 10, because the leading 0 in
    // '012' signals an octal number in C.  For a general purpose reader, that
    // would be a bug, not a feature.
    *p_value = strtoll(item, &p_end, 10);
    if (p_end == item + strlen(item)) {
        return TRUE;
    }
    return FALSE;
}
