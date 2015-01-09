# -----------------------------------------------------------------------------

cdef inline object toBytes(object ob, char *p[]):
    if ob is None:
        return None
    if not isinstance(ob, bytes):
        ob = ob.encode()
    if p:
        p[0] = ob
    return ob

# -----------------------------------------------------------------------------

cdef extern from *:
    void PyErr_SetObject(object, object)

cdef int PyMPE_Raise(int ierr) except -1 with gil:
    PyErr_SetObject(RuntimeError, <long>ierr)
    return 0

cdef inline int CHKERR(int ierr) nogil except -1:
    if ierr == 0: return 0
    PyMPE_Raise(ierr)
    return -1

# -----------------------------------------------------------------------------

cdef extern from "stdio.h" nogil:
    ctypedef struct FILE
    FILE *stdin, *stdout, *stderr
    int fprintf(FILE *, char *, ...)
    int fflush(FILE *)

cdef extern from "Python.h":
    ctypedef struct PyObject
    int Py_IsInitialized() nogil
    void PySys_WriteStderr(char*,...)
    void PySys_WriteStderr(char*,...)
    int Py_AtExit(void (*)())

cdef int logInitedHere = 0 # initialized from this module
cdef int logFinishAtExit = 0 # finalize at Python process exit

cdef inline int initialize() except -1:
    # Is logging active?
    if MPELog.Initialized() == 1:
        return 0
    # Initialize logging library
    global logInitedHere
    cdef int ierr = 0
    ierr = MPELog.Init()
    if ierr != 0: raise RuntimeError(
        "MPE logging initialization failed "
        "[error code: %d]" % ierr)
    logInitedHere = 1
    # Register cleanup at Python exit
    global logFinishAtExit
    if not logFinishAtExit:
        if Py_AtExit(atexit) < 0:
            PySys_WriteStderr(
                b"warning: could not register "
                b"cleanup with Py_AtExit()\n", 0)
        logFinishAtExit = 1
    return 1

cdef int finalize() nogil:
    # Is logging active?
    if MPELog.Initialized() != 1:
        return 0
    # Do we initialized logging?
    global logInitedHere
    if not logInitedHere:
        return 0
    # Finalize logging library
    cdef int ierr = 0
    ierr = MPELog.Finish()
    return ierr

cdef void atexit() nogil:
    cdef int ierr = 0
    ierr = finalize()
    if ierr != 0: fprintf(stderr,
        b"error: in MPE finalization "
        b"[code: %d]", ierr); fflush(stderr)

cdef inline int isReady() nogil:
    return (MPELog.Initialized() == 1)

# -----------------------------------------------------------------------------

cdef inline int packArgs(object arglist, char bytebuf[]) except -1:
    cdef int  pos    = 0
    cdef char token  = 0
    cdef int  count  = 0
    cdef char *data  = NULL
    #
    cdef long   idata = 0
    cdef double fdata = 0
    cdef char*  sdata = NULL
    #
    cdef arg = None
    bytebuf[0] = 0
    for arg in arglist:
        #
        if isinstance(arg, int):
            if sizeof(long) == 4:
                token = c'd'
            else:
                token = c'l'
            idata = arg
            count = 1
            data = <char*>&idata
        elif isinstance(arg, float):
            token = c'E'
            fdata = arg
            count = 1
            data = <char*>&fdata
        elif isinstance(arg, bytes):
            token = c's'
            sdata = arg
            count = <int>len(arg)
            data = <char*>sdata
        else:
            token = 0
            count = 0
            data  = NULL
            continue
        #
        CHKERR( MPELog.PackBytes(bytebuf, &pos, token, count, data) )
    return 0

# -----------------------------------------------------------------------------

#cdef colornames = ["white", "black", "red", "yellow", "green",
#                   "cyan", "blue", "magenta", "aquamarine",
#                   "forestgreen", "orange", "maroon", "brown",
#                   "pink", "coral", "gray" ]
#
# -----------------------------------------------------------------------------
