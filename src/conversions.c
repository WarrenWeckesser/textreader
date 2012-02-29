
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "sizes.h"
#include "constants.h"


int to_double(char *item, double *p_value, char sci, char decimal)
{
    char *p_end;
    char tmp[FIELD_BUFFER_SIZE];
    char *p, *q;

    if (sci == 'D' || decimal != '.') {
        /*
         *  Copy item to tmp, changing 'd' or 'D' to 'e',
         *  and decimal to '.'.
         */
        for (p = tmp, q = item; *q != '\0'; ++p, ++q) {
            if ((sci == 'D') && (*q == 'd' || *q == 'D')) {
                *p = 'e';
            }
            else if ((decimal != '.') && (*q == decimal)) {
                *p = '.';
            }
            else {
                *p = *q;
            }
        }
        *p = '\0';
        item = tmp;
    }
    // Try float conversion with strtod().
    // FIXME: strtod can set errno--should check!
    *p_value = strtod(item, &p_end);
    return (p_end == item + strlen(item));
}

int to_longlong(char *item, long long *p_value)
{
    char *p_end;

    // Try integer conversion.  We explicitly give the base to be 10. If
    // we used 0, strtoll() would convert '012' to 10, because the leading 0 in
    // '012' signals an octal number in C.  For a general purpose reader, that
    // would be a bug, not a feature.
    *p_value = strtoll(item, &p_end, 10);
    return (p_end == item + strlen(item));
}
