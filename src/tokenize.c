
#include <stdio.h>
#include <stdlib.h>

#include "file_buffer.h"
#include "sizes.h"
#include "constants.h"

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
 */

static char **tokenize_sep(void *fb, char *word_buffer, int word_buffer_size,
                           char sep_char, char quote_char, char comment_char,
                           int *p_num_fields, int allow_embedded_newline)
{
    int n;
    char *stop;
    char c;
    int state;
    char *words[MAX_NUM_COLUMNS];
    char *p_word_start, *p_word_end;
    int field_number;
    char **result;
    int overflow = FALSE;

    while (next(fb) == comment_char) {
        skipline(fb);
    }

    if (next(fb) == FB_EOF) {
        return NULL;
    }

    state = TOKENIZE_UNQUOTED;
    field_number = 0;
    p_word_start = word_buffer;
    p_word_end = p_word_start;

    while (TRUE) {
        if ((p_word_end - word_buffer) >= word_buffer_size) {
            overflow = TRUE;
            break;
        }
        c = fetch(fb);
        // printf("c=%c (%d) next=%c (%d) state=%d\n", c, c, next(fb), next(fb), state);
        if (state == TOKENIZE_UNQUOTED) {
            if (c == quote_char) {
                // Opening quote. Switch state to TOKENIZE_QUOTED.
                state = TOKENIZE_QUOTED;
            } else if ((c == sep_char) || (c == comment_char) || (c == '\n') || (c == FB_EOF)) {
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

    if (overflow) {
        printf("tokenize_sep: line too long");
        return NULL;
    }

    if (field_number == 0) {
        return NULL;
    }

    *p_num_fields = field_number;
    result = (char **) malloc(sizeof(char *) * field_number);
    if (result == NULL) {
        printf("tokenize_sep: out of memory for result\n");
        return NULL;
    }

    for (n = 0; n < field_number; ++n) {
        result[n] = words[n];
    }

    return(result);
}

// XXX Currently, 'white space' is simply one or more space characters.
//     This could be extended to sequence of spaces and tabs without too
//     much effort.
//
// XXX double check the use of 'strict_quoting'
//

static char **tokenize_ws(void *fb, char *word_buffer, int word_buffer_size,
                          char quote_char, char comment_char,
                          int *p_num_fields,
                          int allow_embedded_newline,
                          int strict_quoting)
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
    int overflow = FALSE;

    while (next(fb) == comment_char) {
        skipline(fb);
    }

    if (next(fb) == FB_EOF) {
        return NULL;
    }

    state = TOKENIZE_WHITESPACE;
    field_number = 0;
    p_word_start = word_buffer;
    p_word_end = p_word_start;
    while (TRUE) {
        if ((p_word_end - word_buffer) >= word_buffer_size) {
            overflow = TRUE;
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

    if (overflow) {
        printf("tokenize_ws: line too long\n");
        return NULL;
    }

    *p_num_fields = field_number;

    result = (char **) malloc(sizeof(char *) * field_number);
    if (result == NULL) {
        printf("tokenize_ws: out of memory for result\n");
        return NULL;
    }

    for (n = 0; n < field_number; ++n) {
        result[n] = words[n];
    }

    return(result);
}


char **tokenize(void *fb, char *word_buffer, int word_buffer_size,
                char sep_char, char quote_char, char comment_char,
                int *p_num_fields, int allow_embedded_newline)
{
    char **result;

    if (sep_char == 0) {
        result = tokenize_ws(fb, word_buffer, word_buffer_size,
                             quote_char, comment_char, p_num_fields,
                             allow_embedded_newline, TRUE);
    } else {
        result = tokenize_sep(fb, word_buffer, word_buffer_size,
                              sep_char, quote_char, comment_char, p_num_fields,
                              allow_embedded_newline);
    }
    return result;
}
