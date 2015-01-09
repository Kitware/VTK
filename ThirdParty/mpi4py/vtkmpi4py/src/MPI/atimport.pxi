# -----------------------------------------------------------------------------

cdef extern from "Python.h":
    int Py_IsInitialized() nogil
    void PySys_WriteStderr(char*,...)
    int Py_AtExit(void (*)())

    void Py_INCREF(object)
    void Py_DECREF(object)
    void Py_CLEAR(void*)

# -----------------------------------------------------------------------------

cdef extern from "atimport.h":
    pass

# -----------------------------------------------------------------------------

ctypedef struct RCParams:
    int initialize
    int threaded
    int thread_level
    int finalize

cdef int warnRC(object attr, object value) except -1:
    cdef object warn
    from warnings import warn
    warn("mpi4py.rc: '%s': unexpected value '%r'" % (attr, value))

cdef int getRCParams(RCParams* rc) except -1:
    #
    rc.initialize = 1
    rc.threaded = 1
    rc.thread_level = MPI_THREAD_MULTIPLE
    rc.finalize = 1
    #
    cdef object rcmod
    try: from mpi4py import rc as rcmod
    except: return 0
    #
    cdef object initialize = True
    cdef object threaded = True
    cdef object thread_level = 'multiple'
    cdef object finalize = True
    try: initialize = rcmod.initialize
    except: pass
    try: threaded = rcmod.threaded
    except: pass
    try: thread_level = rcmod.thread_level
    except: pass
    try: finalize = rcmod.finalize
    except: pass
    #
    if initialize in (True, 'yes'):
        rc.initialize = 1
    elif initialize in (False, 'no'):
        rc.initialize = 0
    else:
        warnRC("initialize", initialize)
    #
    if threaded in (True, 'yes'):
        rc.threaded = 1
    elif threaded in (False, 'no'):
        rc.threaded = 0
    else:
        warnRC("threaded", threaded)
    #
    if thread_level == 'single':
        rc.thread_level = MPI_THREAD_SINGLE
    elif thread_level == 'funneled':
        rc.thread_level = MPI_THREAD_FUNNELED
    elif thread_level == 'serialized':
        rc.thread_level = MPI_THREAD_SERIALIZED
    elif thread_level == 'multiple':
        rc.thread_level = MPI_THREAD_MULTIPLE
    else:
        warnRC("thread_level", thread_level)
    #
    if finalize in (True, 'yes'):
        rc.finalize = 1
    elif finalize in (False, 'no'):
        rc.finalize = 0
    else:
        warnRC("finalize", finalize)
    #
    return 0

# -----------------------------------------------------------------------------

cdef extern from *:
    #
    int PyMPI_STARTUP_DONE
    int PyMPI_StartUp() nogil
    #
    int PyMPI_CLEANUP_DONE
    int PyMPI_CleanUp() nogil

cdef int inited_atimport = 0
cdef int finalize_atexit = 0

PyMPI_STARTUP_DONE = 0
PyMPI_CLEANUP_DONE = 0

cdef int initialize() except -1:
    global inited_atimport
    global finalize_atexit
    cdef int ierr = MPI_SUCCESS
    # MPI initialized ?
    cdef int initialized = 1
    ierr = MPI_Initialized(&initialized)
    # MPI finalized ?
    cdef int finalized = 1
    ierr = MPI_Finalized(&finalized)
    # Do we have to initialize MPI?
    if initialized:
        if not finalized:
            # Cleanup at (the very end of) Python exit
            if Py_AtExit(atexit) < 0:
                PySys_WriteStderr(b"warning: could not register "
                                  b"cleanup with Py_AtExit()\n", 0)
        return 0
    # Use user parameters from 'mpi4py.rc' module
    cdef RCParams rc
    getRCParams(&rc)
    cdef int required = MPI_THREAD_SINGLE
    cdef int provided = MPI_THREAD_SINGLE
    if rc.initialize: # We have to initialize MPI
        if rc.threaded:
            required = rc.thread_level
            ierr = MPI_Init_thread(NULL, NULL, required, &provided)
            if ierr != MPI_SUCCESS: raise RuntimeError(
                "MPI_Init_thread() failed [error code: %d]" % ierr)
        else:
            ierr = MPI_Init(NULL, NULL)
            if ierr != MPI_SUCCESS: raise RuntimeError(
                "MPI_Init() failed [error code: %d]" % ierr)
        inited_atimport = 1 # We initialized MPI
        if rc.finalize:     # We have to finalize MPI
            finalize_atexit = 1
    # Cleanup at (the very end of) Python exit
    if Py_AtExit(atexit) < 0:
        PySys_WriteStderr(b"warning: could not register "
                          b"cleanup with Py_AtExit()\n", 0)
    return 0

cdef inline int mpi_active() nogil:
    cdef int ierr = MPI_SUCCESS
    # MPI initialized ?
    cdef int initialized = 0
    ierr = MPI_Initialized(&initialized)
    if not initialized or ierr: return 0
    # MPI finalized ?
    cdef int finalized = 1
    ierr = MPI_Finalized(&finalized)
    if finalized or ierr: return 0
    # MPI should be active ...
    return 1

cdef void startup() nogil:
    cdef int ierr = MPI_SUCCESS
    if not mpi_active(): return
    #DBG# fprintf(stderr, b"statup: BEGIN\n"); fflush(stderr)
    ierr = PyMPI_StartUp();
    if ierr: pass
    #DBG# fprintf(stderr, b"statup: END\n"); fflush(stderr)

cdef void cleanup() nogil:
    cdef int ierr = MPI_SUCCESS
    if not mpi_active(): return
    #DBG# fprintf(stderr, b"cleanup: BEGIN\n"); fflush(stderr)
    ierr = PyMPI_CleanUp()
    if ierr: pass
    #DBG# fprintf(stderr, b"cleanup: END\n"); fflush(stderr)

cdef void atexit() nogil:
    cdef int ierr = MPI_SUCCESS
    if not mpi_active(): return
    #DBG# fprintf(stderr, b"atexit: BEGIN\n"); fflush(stderr)
    cleanup()
    if not finalize_atexit: return
    ierr = MPI_Finalize()
    if ierr: pass
    #DBG# fprintf(stderr, b"atexit: END\n"); fflush(stderr)

# -----------------------------------------------------------------------------

# Vile hack for raising a exception and not contaminate the traceback

cdef extern from *:
    void PyErr_SetObject(object, object)
    void *PyExc_RuntimeError
    void *PyExc_NotImplementedError

cdef object MPIException = <object>PyExc_RuntimeError

cdef int PyMPI_Raise(int ierr) except -1 with gil:
    if ierr == -1:
        PyErr_SetObject(<object>PyExc_NotImplementedError, None)
        return 0
    if (<void*>MPIException) != NULL:
        PyErr_SetObject(MPIException, <long>ierr)
    else:
        PyErr_SetObject(<object>PyExc_RuntimeError, <long>ierr)
    return 0

cdef inline int CHKERR(int ierr) nogil except -1:
    if ierr == 0: return 0
    PyMPI_Raise(ierr)
    return -1

cdef inline void print_traceback():
    cdef object sys, traceback
    import sys, traceback
    traceback.print_exc()
    try: sys.stderr.flush()
    except: pass

# -----------------------------------------------------------------------------
