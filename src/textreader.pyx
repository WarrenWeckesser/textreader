
import numpy
cimport numpy

cdef extern from "Python.h":
    ctypedef struct FILE
    FILE* PyFile_AsFile(object)

# Enter the builtin file class into the namespace:
cdef extern from "fileobject.h":
    ctypedef class __builtin__.file [object PyFileObject]:
        pass

cdef extern from "rows.h":
    int count_rows(FILE *f, char delimiter, char quote, char comment,
                   int allow_embedded_newline)
    void *read_rows(FILE *f, int *nrows, char *fmt,
                    char delimiter, char quote, char comment,
                    char sci, char decimal,
                    int allow_embedded_newline,
                    char *datetime_fmt,
                    void *usecols, int num_usecols,
                    int skiprows,
                    void *data_array)


def countrows(file f, delimiter=None, quote='"', comment='#',
                    allow_embedded_newline=True):
    cdef int count
    if delimiter is None:
        delimiter = ' '

    count = count_rows(PyFile_AsFile(f), ord(delimiter[0]), ord(quote[0]), ord(comment[0]),
                       allow_embedded_newline)
    return count


def dtypestr2fmt(st):
    if st == "f8":
        fmt = "d"
    elif st == "f4":
        fmt = "f"
    elif st == "i4":
        fmt = "i"
    elif st == "i2":
        fmt = "h"
    elif st == "M8[us]":
        fmt = "U"  # Temporary experiment
    elif st.startswith('S'):
        fmt = st[1:] + 's'
    else:
        raise ValueError('dtypestr2fmt: unsupported dtype string: %s' % (st,))
    return fmt


def _prod(x, y):
    return x*y


def dtype2fmt(dtype):
    """ This is not a finished, but it is sufficient for simple cases."""
    fmt = ''
    for name in dtype.names:
        field_dt = dtype[name]
        if field_dt.subdtype is not None:
            subdt, shape = field_dt.subdtype
            n = reduce(_prod, shape)
            subfmt = dtypestr2fmt(subdt.str[1:])
            fmt += subfmt * n
        else:
            fmt += dtypestr2fmt(field_dt.str[1:])
    return fmt


def readrows(f, dtype, delimiter=None, quote='"', comment='#',
             sci='E', decimal='.',
             allow_embedded_newline=True, datetime_fmt=None,
             usecols=None, skiprows=None, numrows=None):
    """
    readrows(f, dtype, delimiter=None, quote='"', comment='#',
             sci='E', decimal='.',
             allow_embedded_newline=True, datetime_fmt=None,
             usecols=None, skiprows=None, numrows=None)

    Read a CSV (or similar) text file and return a numpy array.

    Parameters
    ----------
    f : file or str
        File or name of file to read.
    dtype : numpy dtype
        Numpy dtype of the data to read.  This must be a structured
        array.
        XXX Eventually this will be optional, and the code will figure
            it out automatically if the dtype is not given.
    delimiter : str with length 1 or None, optional
        The character that separates fields in the text file.  If
        None, fields are separates by white space.
        Default is None.
    quote : str with length of 1, optional
        The quote character used for quoted fields.
        Default is '"'.
    comment : str with length of 1, optional
        The character that marks the beginning of a comment.  All text
        from this character to the end of the line is ignored.
        Default is '#'
    sci : str with length 1, optional
        If this string is 'd' or 'D', the reader will expect numbers
        expressed in scientific notation to use the letter 'd' or 'D'
        before the exponent, instead of the usual 'e' or 'E'.
        Default is 'E' (meaning either 'e' or 'E' is used for
        scientific notation).
    decimal : str with length 1, optional
        The character that is used as the decimal point in
        floating point numbers.
        Default is '.'.
    allow_embedded_newline : bool, optional
        Default is True.
    datetime_fmt : str or None, optional
        If not None, this must be a string that can be used by
        strptime to parse a datetime string.
    usecols : sequence of ints, optional
        If given, this is the set of column indices (starting
        at 0) of the columns to keep.  The data type given in
        `dtype` must match the columns specified with `usecols`.
    skiprows : int or None, optional
        Numer of rows to skip before beginning to read rows of data.
        Default is None (don't skip any rows).  
    numrows : int or None, optional
        If given, at most this number of rows of data will be read
        (not including `skiprows`).
        In this case, the first pass throught the file that counts
        the number of rows is skipped.  Instead an array of length
        `numrows` is created, and is filled in with data from the
        file.

    Notes
    -----
    When a field contains no data, the following are assigned::

       float:    nan
       int:      0
       string:   '' (empty string)
       datetime: 0

    """
    cdef numpy.ndarray a
    cdef numpy.ndarray usecols_array
    cdef char *dt_fmt
    cdef int opened_here = False
    cdef int nrows

    if isinstance(f, basestring):
        opened_here = True
        filename = f
        f = open(f, 'r')

    fmt = dtype2fmt(dtype)

    print "readrows: fmt =", fmt

    if datetime_fmt is None:
        dt_fmt = ''
    else:
        dt_fmt = datetime_fmt

    if delimiter is None:
        delimiter = '\x00'

    sci = sci.upper()
    if sci != 'E' and sci != 'D':
        raise ValueError("sci must be 'D' or 'E'.")

    if len(decimal) != 1:
        raise ValueError("'%s' is not a valid value for decimal." % decimal)

    if skiprows is None:
        skiprows = 0

    if numrows is None:
        numrows = countrows(f, delimiter, quote, comment, allow_embedded_newline)
        if numrows == -1:
            raise RuntimeError("An error occurred while counting the number of rows in the file.")
        # XXX What if the following makes numrows negative?
        numrows -= skiprows

    # XXX Remove this print eventually.  For now, it helps to show
    # how long countrows() takes.
    print "readrows: numrows =", numrows

    # XXX Hack
    num_fields = sum(c not in "0123456789" for c in fmt)
    print "readrows: num_fields =", num_fields

    if usecols is None:
        usecols_array = numpy.arange(num_fields, dtype=int)
    else:
        usecols_array = numpy.asarray(usecols, dtype=int)
        if usecols_array.size > num_fields:
            raise ValueError("Length of the 'usecols' sequence exceeds the number of fields in the dtype.")

    a = numpy.empty((numrows,), dtype=dtype)

    nrows = numrows
    result = read_rows(PyFile_AsFile(f), &nrows, fmt, ord(delimiter[0]), ord(quote[0]),
                         ord(comment[0]), ord(sci[0]), ord(decimal[0]), allow_embedded_newline, dt_fmt,
                         <int *>usecols_array.data, usecols_array.size, skiprows, a.data)

    if opened_here:
        f.close()

    print "numrows =", numrows, "  nrows =", nrows
    if nrows < numrows:
        a = a[:nrows]

    return a
