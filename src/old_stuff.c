
#define ITEM_STRING   0
#define ITEM_FLOAT    1
#define ITEM_INT      2
#define ITEM_MISSING  3


int process_item(char *item, double *p_double, long long *p_long_int) {
    char tmp[FIELD_BUFFER_SIZE];
    char *p_end;
    char *p, *q;

    if (*item == '\0') {
        return ITEM_STRING;
    }
    // Try integer conversion first.  We explicity give the base to be 10. If
    // we used 0, strtoll() would convert '012' to 10, because the leading 0 in
    // '012' signals an octal number in C.  For a general purpose reader, that
    // would be a bug, not a feature.
    *p_long_int = strtoll(item, &p_end, 10);
    if (p_end == item + strlen(item)) {
        return ITEM_INT;
    }
    // Try hex; if we don't do it here, strtod will do it, which is not what
    // we want.
    if (strchr(item, 'x') != NULL || strchr(item, 'X') != NULL) {
        *p_long_int = strtoll(item, &p_end, 16);
        if (p_end == item + strlen(item)) {
            return ITEM_INT;
        }
    }
    // Try float conversion with strtod().
    // FIXME: strtod can set errno--should check!
    *p_double = strtod(item, &p_end);
    if (p_end == item +strlen(item)) {
        return ITEM_FLOAT;
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
    *p_double = strtod(tmp, &p_end);
    if (p_end == tmp + strlen(tmp)) {
        return(ITEM_FLOAT);
    }
    // Attempt to convert to a numeric type failed, so return ITEM_STRING.
    return(ITEM_STRING);
}
