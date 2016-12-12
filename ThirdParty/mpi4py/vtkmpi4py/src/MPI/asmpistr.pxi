#------------------------------------------------------------------------------

cdef extern from *:
    object PyMPIString_AsStringAndSize(object,const char*[],Py_ssize_t*)
    object PyMPIString_FromString(const char[])
    object PyMPIString_FromStringAndSize(const char[],Py_ssize_t)

#------------------------------------------------------------------------------

cdef inline object asmpistr(object ob, char *s[]):
    cdef const char *buf = NULL
    ob = PyMPIString_AsStringAndSize(ob, &buf, NULL)
    if s != NULL: s[0] = <char*> buf
    return ob

cdef inline object tompistr(const char s[], int n):
    return PyMPIString_FromStringAndSize(s, n)

cdef inline object mpistr(const char s[]):
    return PyMPIString_FromString(s)

#------------------------------------------------------------------------------
