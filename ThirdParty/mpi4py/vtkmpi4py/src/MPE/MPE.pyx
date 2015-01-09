__doc__ = """
Multi-Processing Environment
"""

# -----------------------------------------------------------------------------

cdef object MPI
from mpi4py import MPI
MPI = None

# -----------------------------------------------------------------------------

include "mpe-log.pxi"
include "helpers.pxi"

# -----------------------------------------------------------------------------

cdef class LogEvent:

    cdef int commID
    cdef int eventID[1]
    cdef int isActive
    cdef object name
    cdef object color

    def __cinit__(self, name, color=None):
        self.commID = 0
        self.eventID[0] = 0
        self.isActive = 0
        #
        cdef int commID = 2 # MPI_COMM_WORLD
        cdef char *cname = NULL
        cdef char *ccolor = b"blue"
        cdef char *cformat = NULL
        name  = toBytes(name,  &cname)
        color = toBytes(color, &ccolor)
        if not isReady(): return
        CHKERR( MPELog.NewEvent(commID,
                                cname, ccolor, cformat,
                                self.eventID) )
        #
        self.commID = commID
        self.isActive = 1
        self.name  = name
        self.color = color

    def __call__(self):
        return self

    def __enter__(self):
        self.log()
        return self

    def __exit__(self, *exc):
        return None

    def log(self):
        if not self.isActive: return
        if not self.commID: return
        if not isReady(): return
        CHKERR( MPELog.LogEvent(self.commID, self.eventID[0], NULL) )

    property name:
        def __get__(self):
            return self.name

    property active:
        def __get__(self):
            return <bint> self.isActive
        def __set__(self, bint active):
            self.isActive = active

    property eventID:
        def __get__(self):
            return self.eventID[0]


cdef class LogState:

    cdef int commID
    cdef int stateID[2]
    cdef int isActive
    cdef object name
    cdef object color

    def __cinit__(self, name, color=None):
        self.commID = 0
        self.stateID[0] = 0
        self.stateID[1] = 0
        self.isActive = 0
        #
        cdef int commID = 2 # MPI_COMM_WORLD
        cdef char *cname = NULL
        cdef char *ccolor = b"red"
        cdef char *cformat = NULL
        name  = toBytes(name,  &cname)
        color = toBytes(color, &ccolor)
        if not isReady(): return
        CHKERR( MPELog.NewState(commID,
                                cname, ccolor, cformat,
                                self.stateID) )
        #
        self.commID = commID
        self.isActive = 1
        self.name  = name
        self.color = color

    def __call__(self):
        return self

    def __enter__(self):
        self.enter()
        return self

    def __exit__(self, *exc):
        self.exit()
        return None

    def enter(self):
        if not self.isActive: return
        if not self.commID: return
        if not isReady(): return
        CHKERR( MPELog.LogEvent(self.commID, self.stateID[0], NULL) )

    def exit(self):
        if not self.isActive: return
        if not self.commID: return
        if not isReady(): return
        CHKERR( MPELog.LogEvent(self.commID, self.stateID[1], NULL) )

    property name:
        def __get__(self):
            return self.name

    property active:
        def __get__(self):
            return <bint> self.isActive
        def __set__(self, bint active):
            self.isActive = active

    property stateID:
        def __get__(self):
            return (self.stateID[0], self.stateID[1])


def initLog(logfile=None):
    initialize()
    setLogFileName(logfile)

def finishLog():
    CHKERR( finalize() )

def setLogFileName(filename):
    cdef char *cfilename = NULL
    filename = toBytes(filename, &cfilename)
    CHKERR( MPELog.SetFileName(cfilename) )

def syncClocks():
    if not isReady(): return
    CHKERR( MPELog.SyncClocks() )

def startLog():
    if not isReady(): return
    CHKERR( MPELog.Start() )

def stopLog():
    if not isReady(): return
    CHKERR( MPELog.Stop() )

def newLogEvent(name, color=None):
    cdef LogEvent event = LogEvent(name, color)
    return event

def newLogState(name, color=None):
    cdef LogState state = LogState(name, color)
    return state

# -----------------------------------------------------------------------------
