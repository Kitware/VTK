#---------------------------------------------------------------------

cdef extern from *:
    ctypedef char const_char "const char"
    object PyMPIString_AsStringAndSize(object,const_char**,Py_ssize_t*)
    object PyMPIString_FromString(const_char*)
    object PyMPIString_FromStringAndSize(const_char*,Py_ssize_t)

#---------------------------------------------------------------------

cdef inline object asmpistr(object ob, char **s, int *n):
    cdef const_char *sbuf = NULL
    cdef Py_ssize_t slen = 0, *slenp = NULL
    if n != NULL: slenp = &slen
    ob = PyMPIString_AsStringAndSize(ob, &sbuf, slenp)
    if s: s[0] = <char*> sbuf
    if n: n[0] = <int>   slen
    return ob

cdef inline object tompistr(const_char *s, int n):
    return PyMPIString_FromStringAndSize(s, n)

cdef inline object mpistr(const_char *s):
    return PyMPIString_FromString(s)

#---------------------------------------------------------------------
