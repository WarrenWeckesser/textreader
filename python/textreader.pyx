
import time
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
    int count_fields(FILE *f, char delimiter, char quote, char comment,
                     int allow_embedded_newline)
    void *read_rows(FILE *f, int *nrows, char *fmt,
                    char delimiter, char quote, char comment,
                    char sci, char decimal,
                    int allow_embedded_newline,
                    char *datetime_fmt,
                    int tz_offset,
                    void *usecols, int num_usecols,
                    int skiprows,
                    void *data_array,
                    int *p_error_type, int *p_error_lineno)


def countrows(file f, delimiter=None, quote='"', comment='#',
                    allow_embedded_newline=True):
    cdef int count
    if delimiter is None:
        delimiter = ' '

    count = count_rows(PyFile_AsFile(f), ord(delimiter[0]), ord(quote[0]), ord(comment[0]),
                       allow_embedded_newline)
    return count


def countfields(file f, delimiter=None, quote='"', comment='#',
                    allow_embedded_newline=True):
    cdef int count
    if delimiter is None:
        delimiter = ' '

    count = count_fields(PyFile_AsFile(f), ord(delimiter[0]), ord(quote[0]), ord(comment[0]),
                       allow_embedded_newline)
    return count


_dtype_str_map = dict(i1='b', u1='B', i2='h', u2='H', i4='i', u4='I',
                    i8='q', u8='Q', f4='f', f8='d', c8='c', c16='z')

def dtypestr2fmt(st):
    fmt = _dtype_str_map.get(st)
    if fmt is None:
        if st == "M8[us]":
            fmt = 'U'  # Temporary experiment
        elif st.startswith('S'):
            fmt = st[1:] + 's'
        else:
            raise ValueError('dtypestr2fmt: unsupported dtype string: %s' % (st,))
    return fmt


def _prod(x, y):
    return x*y


def flatten_dtype(dt):

    if not isinstance(dt, numpy.dtype):
        dt = numpy.dtype(dt)
    if dt.names is None:
        if dt.subdtype is not None:
            subdt, shape = dt.subdtype
            if isinstance(shape, int):
                n = shape
            else:
                n = reduce(_prod, shape)
            fmt = flatten_dtype(subdt) * n
        else:
            fmt = dtypestr2fmt(dt.str[1:])
    else:
        fmt = ''.join([flatten_dtype(dt[name]) for name in dt.names])
    return fmt


def readrows(f, dtype, delimiter=None, quote='"', comment='#',
             sci='E', decimal='.',
             allow_embedded_newline=True, datetime_fmt=None,
             tzoffset=0,
             usecols=None, skiprows=None, numrows=None):
    """
    readrows(f, dtype, delimiter=None, quote='"', comment='#',
             sci='E', decimal='.',
             allow_embedded_newline=True, datetime_fmt=None,
             tzoffset=0,
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
    tzoffset : int or None, optional
        Offset in seconds from UTC of date/time values in the file.
        Default is time.timezone.
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
    cdef int error_type, error_lineno
    cdef int tz_offset
    cdef int num_filed_fields

    if datetime_fmt is None:
        dt_fmt = ''
    else:
        dt_fmt = datetime_fmt

    if tzoffset is None:
        tz_offset = time.timezone
    else:
        tz_offset = tzoffset

    if delimiter is None:
        delimiter = '\x00'

    sci = sci.upper()
    if sci != 'E' and sci != 'D':
        raise ValueError("sci must be 'D' or 'E'.")

    if len(decimal) != 1:
        raise ValueError("'%s' is not a valid value for decimal." % decimal)

    if skiprows is None:
        skiprows = 0

    if isinstance(f, basestring):
        opened_here = True
        filename = f
        f = open(f, 'r')

    if not isinstance(dtype, numpy.dtype):
        dtype = numpy.dtype(dtype)
    simple_dtype = False
    if dtype.names is None and dtype.subdtype is None:
        # Not a structured array or other complex dtype.
        simple_dtype = True
        num_file_fields = countfields(f, delimiter, quote, comment, allow_embedded_newline)
        fmt = dtypestr2fmt(dtype.str[1:])
    else:
        fmt = flatten_dtype(dtype)

    if numrows is None:
        numrows = countrows(f, delimiter, quote, comment, allow_embedded_newline)
        if numrows == -1:
            raise RuntimeError("An error occurred while counting the number of rows in the file.")
        # XXX What if the following makes numrows negative?
        numrows -= skiprows

    # XXX Remove this print eventually.  For now, it helps to show
    # how long countrows() takes.
    print "readrows: numrows =", numrows

    if simple_dtype:
        if usecols is None:
            num_fields = num_file_fields
            usecols_array = numpy.arange(num_fields, dtype=numpy.int32)
        else:
            usecols_array = numpy.asarray(usecols, dtype=numpy.int32)
            num_fields = usecols_array.size
        fmt = fmt * num_fields
        shape = (numrows, num_fields)
    else:
        if usecols is None:
            num_fields = sum(c not in "0123456789" for c in fmt)
            usecols_array = numpy.arange(num_fields, dtype=numpy.int32)
        else:
            usecols_array = numpy.asarray(usecols, dtype=numpy.int32)
            #if usecols_array.size > num_fields:
            #    raise ValueError("Length of the 'usecols' sequence exceeds the number of fields in the dtype.")
        shape = (numrows,)

    a = numpy.empty(shape, dtype=dtype)

    nrows = numrows
    result = read_rows(PyFile_AsFile(f), &nrows, fmt, ord(delimiter[0]), ord(quote[0]),
                         ord(comment[0]), ord(sci[0]), ord(decimal[0]), allow_embedded_newline,
                         dt_fmt, tz_offset,
                         <int *>usecols_array.data, usecols_array.size, skiprows, a.data,
                         &error_type, &error_lineno)

    if opened_here:
        f.close()

    if nrows < numrows:
        a = a[:nrows]

    return a
