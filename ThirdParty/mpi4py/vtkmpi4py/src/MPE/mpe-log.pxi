cdef extern from "MPE/mpe-log.h" nogil:
    ctypedef struct PyMPELogAPI:
        int (*Init)() nogil
        int (*Finish)() nogil
        int (*Initialized)() nogil
        int (*SetFileName)(char[]) nogil
        int (*SyncClocks)() nogil
        int (*Start)() nogil
        int (*Stop)() nogil
        int (*NewState)(int, char[], char[], char[], int[2]) nogil
        int (*NewEvent)(int, char[], char[], char[], int[1]) nogil
        int (*LogEvent)(int, int, char[]) nogil
        int (*PackBytes)(char[], int *, char, int, void *) nogil

cdef extern from "MPE/mpe-log.c" nogil:
    PyMPELogAPI *MPELog"(PyMPELog)"
