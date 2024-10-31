# Author:  Lisandro Dalcin
# Contact: dalcinl@gmail.com

# --

from mpi4py.libmpi cimport MPI_Aint
from mpi4py.libmpi cimport MPI_Offset
from mpi4py.libmpi cimport MPI_Count
from mpi4py.libmpi cimport MPI_Status
from mpi4py.libmpi cimport MPI_Datatype
from mpi4py.libmpi cimport MPI_Request
from mpi4py.libmpi cimport MPI_Message
from mpi4py.libmpi cimport MPI_Op
from mpi4py.libmpi cimport MPI_Group
from mpi4py.libmpi cimport MPI_Info
from mpi4py.libmpi cimport MPI_Errhandler
from mpi4py.libmpi cimport MPI_Session
from mpi4py.libmpi cimport MPI_Comm
from mpi4py.libmpi cimport MPI_Win
from mpi4py.libmpi cimport MPI_File

# --

cdef extern from *:
    ctypedef MPI_Aint   Aint   "MPI_Aint"
    ctypedef MPI_Offset Offset "MPI_Offset"
    ctypedef MPI_Count  Count  "MPI_Count"

ctypedef api class Datatype [
    type   PyMPIDatatype_Type,
    object PyMPIDatatypeObject,
]:
    cdef MPI_Datatype ob_mpi
    cdef unsigned     flags
    cdef object     __weakref__

ctypedef api class Status [
    type   PyMPIStatus_Type,
    object PyMPIStatusObject,
]:
    cdef MPI_Status ob_mpi
    cdef unsigned   flags
    cdef object   __weakref__

ctypedef api class Request [
    type   PyMPIRequest_Type,
    object PyMPIRequestObject,
]:
    cdef MPI_Request ob_mpi
    cdef unsigned    flags
    cdef object    __weakref__
    cdef object      ob_buf

ctypedef api class Prequest(Request) [
    type   PyMPIPrequest_Type,
    object PyMPIPrequestObject,
]:
    pass

ctypedef api class Grequest(Request) [
    type   PyMPIGrequest_Type,
    object PyMPIGrequestObject,
]:
    cdef MPI_Request ob_grequest

ctypedef api class Message [
    type   PyMPIMessage_Type,
    object PyMPIMessageObject,
]:
    cdef MPI_Message ob_mpi
    cdef unsigned    flags
    cdef object    __weakref__
    cdef object      ob_buf

ctypedef api class Op [
    type   PyMPIOp_Type,
    object PyMPIOpObject,
]:
    cdef MPI_Op   ob_mpi
    cdef unsigned flags
    cdef object __weakref__

ctypedef api class Group [
    type   PyMPIGroup_Type,
    object PyMPIGroupObject,
]:
    cdef MPI_Group ob_mpi
    cdef unsigned  flags
    cdef object  __weakref__

ctypedef api class Info [
    type   PyMPIInfo_Type,
    object PyMPIInfoObject,
]:
    cdef MPI_Info ob_mpi
    cdef unsigned flags
    cdef object __weakref__

ctypedef api class Errhandler [
    type   PyMPIErrhandler_Type,
    object PyMPIErrhandlerObject,
]:
    cdef MPI_Errhandler ob_mpi
    cdef unsigned       flags
    cdef object       __weakref__

ctypedef api class Session [
    type   PyMPISession_Type,
    object PyMPISessionObject,
]:
    cdef MPI_Session ob_mpi
    cdef unsigned    flags
    cdef object     __weakref__

ctypedef api class Comm [
    type   PyMPIComm_Type,
    object PyMPICommObject,
]:
    cdef MPI_Comm ob_mpi
    cdef unsigned flags
    cdef object __weakref__

ctypedef api class Intracomm(Comm) [
    type   PyMPIIntracomm_Type,
    object PyMPIIntracommObject,
]:
    pass

ctypedef api class Topocomm(Intracomm) [
    type   PyMPITopocomm_Type,
    object PyMPITopocommObject,
]:
    pass

ctypedef api class Cartcomm(Topocomm) [
    type   PyMPICartcomm_Type,
    object PyMPICartcommObject,
]:
    pass

ctypedef api class Graphcomm(Topocomm) [
    type   PyMPIGraphcomm_Type,
    object PyMPIGraphcommObject,
]:
    pass

ctypedef api class Distgraphcomm(Topocomm) [
    type   PyMPIDistgraphcomm_Type,
    object PyMPIDistgraphcommObject,
]:
    pass

ctypedef api class Intercomm(Comm) [
    type   PyMPIIntercomm_Type,
    object PyMPIIntercommObject,
]:
    pass

ctypedef api class Win [
    type   PyMPIWin_Type,
    object PyMPIWinObject,
]:
    cdef MPI_Win  ob_mpi
    cdef unsigned flags
    cdef object __weakref__
    cdef object   ob_mem

ctypedef api class File [
    type   PyMPIFile_Type,
    object PyMPIFileObject,
]:
    cdef MPI_File ob_mpi
    cdef unsigned flags
    cdef object __weakref__

# --
