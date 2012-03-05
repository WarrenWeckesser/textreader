
#include <stdio.h>
#include <stdlib.h>

#include "file_buffer.h"
#include "sizes.h"
#include "constants.h"
#include "tokenize.h"
#include "error_types.h"


/* Tokenization state machine states. */
#define TOKENIZE_UNQUOTED   1
#define TOKENIZE_QUOTED     2
#define TOKENIZE_WHITESPACE 3


/*
 *  tokenize a row of input, with an explicit field delimiter char (sep_char).
 *
 *  The fields are stored in word_buffer, and words is an array of
 *  pointers to the starts of the words parsed so far. 
 *
 *  Returns an array of char*.  Points to memory malloc'ed here so it
 *  must be freed by the caller.
 *
 *  Returns NULL for several different conditions:
 *  * Reached EOF before finding *any* data to parse.
 *  * The amount of text copied to word_buffer exceeded the buffer size.
 *  * Failed to parse a single field. This is the condition field_number == 0
 *    that is checked after the main loop.  To do: double check exactly what
 *    can lead to this condition.
 *  * Out of memory: could not allocate the memory to hold the array of
 *    char pointer that the function returns.
 *  * The row has more fields than MAX_NUM_COLUMNS.
 */

static char **tokenize_sep(void *fb, char *word_buffer, int word_buffer_size,
                           char sep_char, char quote_char, char comment_char,
                           int *p_num_fields, int allow_embedded_newline,
                           int *p_error_type)
{
    int n;
    char *stop;
    char c;
    int state;
    char *words[MAX_NUM_COLUMNS];
    char *p_word_start, *p_word_end;
    int field_number;
    char **result;

    *p_error_type = 0;

    while (next(fb) == comment_char) {
        skipline(fb);
    }

    if (next(fb) == FB_EOF) {
        *p_error_type = ERROR_NO_DATA;
        return NULL;
    }

    state = TOKENIZE_UNQUOTED;
    field_number = 0;
    p_word_start = word_buffer;
    p_word_end = p_word_start;

    while (TRUE) {
        if ((p_word_end - word_buffer) >= word_buffer_size) {
            *p_error_type = ERROR_TOO_MANY_CHARS;
            break;
        }
        if (field_number >= MAX_NUM_COLUMNS) {
            *p_error_type = ERROR_TOO_MANY_FIELDS;
            break;
        }
        c = fetch(fb);
        if (state == TOKENIZE_UNQUOTED) {
            if (c == quote_char) {
                // Opening quote. Switch state to TOKENIZE_QUOTED.
                state = TOKENIZE_QUOTED;
            } else if ((c == sep_char) || (c == comment_char) || (c == '\n') || (c == FB_EOF)) {
                // End of a field.  Save the field, and remain in this state.
                *p_word_end = '\0';
                words[field_number] = p_word_start;
                ++field_number;
                ++p_word_end;
                p_word_start = p_word_end;
                if (c == '\n' || c == FB_EOF) {
                    break;
                } else if (c == comment_char) {
                    skipline(fb);
                }
            } else {
                *p_word_end = c;
                ++p_word_end;
            } 
        } else if (state == TOKENIZE_QUOTED) {
            if ((c != quote_char && c != '\n' && c != FB_EOF) || (c == '\n' && allow_embedded_newline)) {
                *p_word_end = c;
                ++p_word_end;
            } else if (c == quote_char && next(fb)==quote_char) {
                // Repeated quote characters; treat the pair as a single quote char.
                *p_word_end = c;
                ++p_word_end;
                // Skip the second double-quote.
                fetch(fb);
            } else if (c == quote_char) {
                // Closing quote.  Switch state to TOKENIZE_UNQUOTED.
                state = TOKENIZE_UNQUOTED;
            } else {
                // c must be '\n' or FB_EOF.
                // If we are here, it means we've reached the end of the file
                // while inside quotes, or the end of the line while inside
                // quotes and 'allow_embedded_newline' is 0.
                // This could be treated as an error, but for now, we'll simply
                // end the field (and the row).
                *p_word_end = '\0';
                words[field_number] = p_word_start;
                ++field_number;
                ++p_word_end;
                p_word_start = p_word_end;
                break;
            }
        }
    }

    if (*p_error_type) {
        return NULL;
    }

    if (field_number == 0) {
        /* XXX Is this the appropriate error type? */
        *p_error_type = ERROR_NO_DATA;
        return NULL;
    }

    *p_num_fields = field_number;
    result = (char **) malloc(sizeof(char *) * field_number);
    if (result == NULL) {
        *p_error_type = ERROR_OUT_OF_MEMORY;
        return NULL;
    }

    for (n = 0; n < field_number; ++n) {
        result[n] = words[n];
    }

    return result;
}


/*
 *  XXX Currently, 'white space' is simply one or more space characters.
 *      This could be extended to sequence of spaces and tabs without too
 *      much effort.
 *
 *  XXX double check the use of 'strict_quoting'
 *
 *  XXX Returns NULL for several different error cases or edge cases.
 *      This needs to be refined.
 */

static char **tokenize_ws(void *fb, char *word_buffer, int word_buffer_size,
                          char quote_char, char comment_char,
                          int *p_num_fields,
                          int allow_embedded_newline,
                          int strict_quoting, int *p_error_type)
{
    int n;
    char *p;
    char *stop;
    char c;
    int state;
    char *words[MAX_NUM_COLUMNS];
    char *p_word_start, *p_word_end;
    int field_number;
    char **result;

    *p_error_type = 0;

    while (next(fb) == comment_char) {
        skipline(fb);
    }

    if (next(fb) == FB_EOF) {
        *p_error_type = ERROR_NO_DATA;
        return NULL;
    }

    state = TOKENIZE_WHITESPACE;
    field_number = 0;
    p_word_start = word_buffer;
    p_word_end = p_word_start;

    while (TRUE) {
        if ((p_word_end - word_buffer) >= word_buffer_size) {
            *p_error_type = ERROR_TOO_MANY_CHARS;
            break;
        }
        if (field_number == MAX_NUM_COLUMNS) {
            *p_error_type = ERROR_TOO_MANY_FIELDS;
            break;
        }
        c = fetch(fb);
        //printf("c=%c (%d) next=%c (%d) state=%d\n", c, c, next(fb), next(fb), state);

        if (state == TOKENIZE_WHITESPACE) {
            if (c == quote_char) {
                // Opening quote.  Switch state to TOKENIZE_QUOTED
                state = TOKENIZE_QUOTED;
            } else if (c == '\n' || c == FB_EOF) {
                break;
            } else if (c != ' ') {
                *p_word_end = c;
                ++p_word_end;
                state = TOKENIZE_UNQUOTED;
            }
        } else if (state == TOKENIZE_UNQUOTED) {
            if (c == quote_char && !strict_quoting) {
                // Opening quote.  Switch state to TOKENIZE_QUOTED
                state = TOKENIZE_QUOTED;
            } else if ((c == ' ') || (c == '\n') || (c == FB_EOF)) {
                *p_word_end = '\0';
                words[field_number] = p_word_start;
                ++field_number;
                ++p_word_end;
                p_word_start = p_word_end;
                if (c == '\n' || c == FB_EOF) {
                    break;
                }
                // Switch state to TOKENIZE_WHITESPACE.
                state = TOKENIZE_WHITESPACE;
            } else {
                *p_word_end = c;
                ++p_word_end;
            } 
        } else if (state == TOKENIZE_QUOTED) {
            if ((c != quote_char && c != '\n' && c != FB_EOF) || (c == '\n' && allow_embedded_newline)) {
                *p_word_end = c;
                ++p_word_end;
            } else if (c == quote_char && next(fb)==quote_char) {
                *p_word_end = c;
                ++p_word_end;
                // Skip the second quote char.
                fetch(fb);
            } else if (c == quote_char && next(fb) != ' ' && next(fb) != '\n' && next(fb) != FB_EOF) {
                *p_word_end = c;
                ++p_word_end; 
            } else if (c == quote_char) {
                // Closing quote.  Just switch to TOKENIZE_UNQUOTED.
                // Note that this does not terminate the field.  This means
                // an input such as
                // ABC"123"DEF
                // will be processed as a single field, containing
                // ABC123DEF
                // Not sure if this is desirable.
                state = TOKENIZE_UNQUOTED;
            } else {
                // c must be '\n' or FB_EOF.
                // If we are here, it means we've reached the end of the file
                // while inside quotes, or the end of the line while inside
                // quotes and 'allow_embedded_newline' is 0.
                // This could be treated as an error, but for now, we'll simply
                // end the field (and the row).
                *p_word_end = '\0';
                words[field_number] = p_word_start;
                ++field_number;
                ++p_word_end;
                p_word_start = p_word_end;
                break;
            }
        } 
    }

    if (*p_error_type) {
        return NULL;
    }

    if (field_number == 0) {
        /* XXX Is this the appropriate error type? */
        *p_error_type = ERROR_NO_DATA;
        return NULL;
    }

    *p_num_fields = field_number;

    result = (char **) malloc(sizeof(char *) * field_number);
    if (result == NULL) {
        *p_error_type = ERROR_OUT_OF_MEMORY;
        return NULL;
    }

    for (n = 0; n < field_number; ++n) {
        result[n] = words[n];
    }

    return result;
}


char **tokenize(void *fb, char *word_buffer, int word_buffer_size,
                char sep_char, char quote_char, char comment_char,
                int *p_num_fields, int allow_embedded_newline,
                int *p_error_type)
{
    char **result;

    if (sep_char == 0) {
        result = tokenize_ws(fb, word_buffer, word_buffer_size,
                             quote_char, comment_char, p_num_fields,
                             allow_embedded_newline, TRUE, p_error_type);
    } else {
        result = tokenize_sep(fb, word_buffer, word_buffer_size,
                              sep_char, quote_char, comment_char, p_num_fields,
                              allow_embedded_newline, p_error_type);
    }
    return result;
}
