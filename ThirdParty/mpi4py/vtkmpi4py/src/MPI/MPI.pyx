__doc__ = """
Message Passing Interface
"""

include "mpi4py/mpi.pxi"

include "stdlib.pxi"
include "atimport.pxi"

initialize()
startup()

include "asmpistr.pxi"
include "asbuffer.pxi"
include "asmemory.pxi"
include "asarray.pxi"
include "helpers.pxi"
include "message.pxi"
include "pickled.pxi"

include "CAPI.pxi"

include "Exception.pyx"
include "Errhandler.pyx"
include "Datatype.pyx"
include "Status.pyx"
include "Request.pyx"
include "Info.pyx"
include "Op.pyx"
include "Group.pyx"
include "Comm.pyx"
include "Win.pyx"
include "File.pyx"

# Assorted constants
# ------------------

UNDEFINED = MPI_UNDEFINED
#"""Undefined integer value"""

ANY_SOURCE = MPI_ANY_SOURCE
#"""Wildcard source value for receives"""

ANY_TAG = MPI_ANY_TAG
#"""Wildcard tag value for receives"""

PROC_NULL = MPI_PROC_NULL
#"""Special process rank for send/receive"""

ROOT = MPI_ROOT
#"""Root process for collective inter-communications"""

BOTTOM = __BOTTOM__
#"""Special address for buffers"""

IN_PLACE = __IN_PLACE__
#"""*In-place* option for collective communications"""



# Predefined Attribute Keyvals
# ----------------------------

KEYVAL_INVALID  = MPI_KEYVAL_INVALID

TAG_UB          = MPI_TAG_UB
HOST            = MPI_HOST
IO              = MPI_IO
WTIME_IS_GLOBAL = MPI_WTIME_IS_GLOBAL

UNIVERSE_SIZE   = MPI_UNIVERSE_SIZE
APPNUM          = MPI_APPNUM

LASTUSEDCODE    = MPI_LASTUSEDCODE

WIN_BASE        = MPI_WIN_BASE
WIN_SIZE        = MPI_WIN_SIZE
WIN_DISP_UNIT   = MPI_WIN_DISP_UNIT


# Memory Allocation
# -----------------

def Alloc_mem(Aint size, Info info=INFO_NULL):
    """
    Allocate memory for message passing and RMA
    """
    cdef void *base = NULL
    cdef MPI_Info cinfo = arg_Info(info)
    CHKERR( MPI_Alloc_mem(size, cinfo, &base) )
    return tomemory(base, size)

def Free_mem(memory):
    """
    Free memory allocated with `Alloc_mem()`
    """
    cdef void *base = NULL
    asmemory(memory, &base, NULL)
    CHKERR( MPI_Free_mem(base) )


# Initialization and Exit
# -----------------------

def Init():
    """
    Initialize the MPI execution environment
    """
    CHKERR( MPI_Init(NULL, NULL) )
    startup()

def Finalize():
    """
    Terminate the MPI execution environment
    """
    cleanup()
    CHKERR( MPI_Finalize() )

# Levels of MPI threading support
# -------------------------------

THREAD_SINGLE     = MPI_THREAD_SINGLE
# """Only one thread will execute"""

THREAD_FUNNELED   = MPI_THREAD_FUNNELED
# """MPI calls are *funneled* to the main thread"""

THREAD_SERIALIZED = MPI_THREAD_SERIALIZED
# """MPI calls are *serialized*"""

THREAD_MULTIPLE   = MPI_THREAD_MULTIPLE
# """Multiple threads may call MPI"""

def Init_thread(int required=THREAD_MULTIPLE):
    """
    Initialize the MPI execution environment
    """
    cdef int provided = MPI_THREAD_SINGLE
    CHKERR( MPI_Init_thread(NULL, NULL, required, &provided) )
    startup()
    return provided

def Query_thread():
    """
    Return the level of thread support
    provided by the MPI library
    """
    cdef int provided = MPI_THREAD_SINGLE
    CHKERR( MPI_Query_thread(&provided) )
    return provided

def Is_thread_main():
    """
    Indicate whether this thread called
    ``Init`` or ``Init_thread``
    """
    cdef int flag = 1
    CHKERR( MPI_Is_thread_main(&flag) )
    return <bint>flag

def Is_initialized():
    """
    Indicates whether ``Init`` has been called
    """
    cdef int flag = 0
    CHKERR( MPI_Initialized(&flag) )
    return <bint>flag

def Is_finalized():
    """
    Indicates whether ``Finalize`` has completed
    """
    cdef int flag = 0
    CHKERR( MPI_Finalized(&flag) )
    return <bint>flag

# Implementation Information
# --------------------------

# MPI Version Number
# -----------------

VERSION    = MPI_VERSION
SUBVERSION = MPI_SUBVERSION

def Get_version():
    """
    Obtain the version number of the MPI standard supported
    by the implementation as a tuple ``(version, subversion)``
    """
    cdef int version = 1
    cdef int subversion = 0
    CHKERR( MPI_Get_version(&version, &subversion) )
    return (version, subversion)

# Environmental Inquires
# ----------------------

def Get_processor_name():
    """
    Obtain the name of the calling processor
    """
    cdef char name[MPI_MAX_PROCESSOR_NAME+1]
    cdef int nlen = 0
    CHKERR( MPI_Get_processor_name(name, &nlen) )
    return tompistr(name, nlen)

# Timers and Synchronization
# --------------------------

def Wtime():
    """
    Return an elapsed time on the calling processor
    """
    return MPI_Wtime()

def Wtick():
    """
    Return the resolution of ``Wtime``
    """
    return MPI_Wtick()

# Control of Profiling
# --------------------

def Pcontrol(int level):
    """
    Control profiling
    """
    if level < 0 or level > 2: CHKERR(MPI_ERR_ARG)
    CHKERR( MPI_Pcontrol(level) )


# Maximum string sizes
# --------------------

# MPI-1
MAX_PROCESSOR_NAME = MPI_MAX_PROCESSOR_NAME
MAX_ERROR_STRING   = MPI_MAX_ERROR_STRING
# MPI-2
MAX_PORT_NAME      = MPI_MAX_PORT_NAME
MAX_INFO_KEY       = MPI_MAX_INFO_KEY
MAX_INFO_VAL       = MPI_MAX_INFO_VAL
MAX_OBJECT_NAME    = MPI_MAX_OBJECT_NAME
MAX_DATAREP_STRING = MPI_MAX_DATAREP_STRING



# --------------------------------------------------------------------

cdef extern from "vendor.h":
    int MPI_Get_vendor(const_char**,int*,int*,int*)

def get_vendor():
    """
    Infomation about the underlying MPI implementation

    :Returns:
      - a string with the name of the MPI implementation
      - an integer 3-tuple version ``(major, minor, micro)``
    """
    cdef const_char *name=NULL
    cdef int major=0, minor=0, micro=0
    CHKERR( MPI_Get_vendor(&name, &major, &minor, &micro) )
    return (mpistr(name), (major, minor, micro))

# --------------------------------------------------------------------
