#ifndef __PYX_HAVE__mpi4py__MPI
#define __PYX_HAVE__mpi4py__MPI

struct PyMPIStatusObject;
struct PyMPIDatatypeObject;
struct PyMPIRequestObject;
struct PyMPIPrequestObject;
struct PyMPIGrequestObject;
struct PyMPIOpObject;
struct PyMPIGroupObject;
struct PyMPIInfoObject;
struct PyMPIErrhandlerObject;
struct PyMPICommObject;
struct PyMPIIntracommObject;
struct PyMPICartcommObject;
struct PyMPIGraphcommObject;
struct PyMPIDistgraphcommObject;
struct PyMPIIntercommObject;
struct PyMPIWinObject;
struct PyMPIFileObject;

/* "mpi4py/MPI.pxd":49
 *     ctypedef MPI_Offset Offset "MPI_Offset"
 * 
 * ctypedef public api class Status [             # <<<<<<<<<<<<<<
 *     type   PyMPIStatus_Type,
 *     object PyMPIStatusObject,
 */
struct PyMPIStatusObject {
  PyObject_HEAD
  MPI_Status ob_mpi;
  int flags;
};
typedef struct PyMPIStatusObject PyMPIStatusObject;

/* "mpi4py/MPI.pxd":56
 *     cdef int        flags
 * 
 * ctypedef public api class Datatype [             # <<<<<<<<<<<<<<
 *     type   PyMPIDatatype_Type,
 *     object PyMPIDatatypeObject,
 */
struct PyMPIDatatypeObject {
  PyObject_HEAD
  MPI_Datatype ob_mpi;
  int flags;
};
typedef struct PyMPIDatatypeObject PyMPIDatatypeObject;

/* "mpi4py/MPI.pxd":63
 *     cdef int          flags
 * 
 * ctypedef public api class Request [             # <<<<<<<<<<<<<<
 *     type   PyMPIRequest_Type,
 *     object PyMPIRequestObject,
 */
struct PyMPIRequestObject {
  PyObject_HEAD
  MPI_Request ob_mpi;
  int flags;
  PyObject *ob_buf;
};
typedef struct PyMPIRequestObject PyMPIRequestObject;

/* "mpi4py/MPI.pxd":71
 *     cdef object      ob_buf
 * 
 * ctypedef public api class Prequest(Request) [             # <<<<<<<<<<<<<<
 *     type   PyMPIPrequest_Type,
 *     object PyMPIPrequestObject,
 */
struct PyMPIPrequestObject {
  struct PyMPIRequestObject __pyx_base;
};
typedef struct PyMPIPrequestObject PyMPIPrequestObject;

/* "mpi4py/MPI.pxd":77
 *     pass
 * 
 * ctypedef public api class Grequest(Request) [             # <<<<<<<<<<<<<<
 *     type   PyMPIGrequest_Type,
 *     object PyMPIGrequestObject,
 */
struct PyMPIGrequestObject {
  struct PyMPIRequestObject __pyx_base;
  MPI_Request ob_grequest;
};
typedef struct PyMPIGrequestObject PyMPIGrequestObject;

/* "mpi4py/MPI.pxd":83
 *     cdef MPI_Request ob_grequest
 * 
 * ctypedef public api class Op [             # <<<<<<<<<<<<<<
 *     type   PyMPIOp_Type,
 *     object PyMPIOpObject,
 */
struct PyMPIOpObject {
  PyObject_HEAD
  MPI_Op ob_mpi;
  int flags;
  PyObject *(*ob_func)(PyObject *, PyObject *);
  int ob_usrid;
};
typedef struct PyMPIOpObject PyMPIOpObject;

/* "mpi4py/MPI.pxd":92
 *     cdef int    ob_usrid
 * 
 * ctypedef public api class Group [             # <<<<<<<<<<<<<<
 *     type   PyMPIGroup_Type,
 *     object PyMPIGroupObject,
 */
struct PyMPIGroupObject {
  PyObject_HEAD
  MPI_Group ob_mpi;
  int flags;
};
typedef struct PyMPIGroupObject PyMPIGroupObject;

/* "mpi4py/MPI.pxd":99
 *     cdef int       flags
 * 
 * ctypedef public api class Info [             # <<<<<<<<<<<<<<
 *     type   PyMPIInfo_Type,
 *     object PyMPIInfoObject,
 */
struct PyMPIInfoObject {
  PyObject_HEAD
  MPI_Info ob_mpi;
  int flags;
};
typedef struct PyMPIInfoObject PyMPIInfoObject;

/* "mpi4py/MPI.pxd":106
 *     cdef int      flags
 * 
 * ctypedef public api class Errhandler [             # <<<<<<<<<<<<<<
 *     type   PyMPIErrhandler_Type,
 *     object PyMPIErrhandlerObject,
 */
struct PyMPIErrhandlerObject {
  PyObject_HEAD
  MPI_Errhandler ob_mpi;
  int flags;
};
typedef struct PyMPIErrhandlerObject PyMPIErrhandlerObject;

/* "mpi4py/MPI.pxd":113
 *     cdef int            flags
 * 
 * ctypedef public api class Comm [             # <<<<<<<<<<<<<<
 *     type   PyMPIComm_Type,
 *     object PyMPICommObject,
 */
struct PyMPICommObject {
  PyObject_HEAD
  MPI_Comm ob_mpi;
  int flags;
};
typedef struct PyMPICommObject PyMPICommObject;

/* "mpi4py/MPI.pxd":120
 *     cdef int      flags
 * 
 * ctypedef public api class Intracomm(Comm) [             # <<<<<<<<<<<<<<
 *     type   PyMPIIntracomm_Type,
 *     object PyMPIIntracommObject,
 */
struct PyMPIIntracommObject {
  struct PyMPICommObject __pyx_base;
};
typedef struct PyMPIIntracommObject PyMPIIntracommObject;

/* "mpi4py/MPI.pxd":126
 *     pass
 * 
 * ctypedef public api class Cartcomm(Intracomm) [             # <<<<<<<<<<<<<<
 *     type   PyMPICartcomm_Type,
 *     object PyMPICartcommObject,
 */
struct PyMPICartcommObject {
  struct PyMPIIntracommObject __pyx_base;
};
typedef struct PyMPICartcommObject PyMPICartcommObject;

/* "mpi4py/MPI.pxd":132
 *     pass
 * 
 * ctypedef public api class Graphcomm(Intracomm) [             # <<<<<<<<<<<<<<
 *     type   PyMPIGraphcomm_Type,
 *     object PyMPIGraphcommObject,
 */
struct PyMPIGraphcommObject {
  struct PyMPIIntracommObject __pyx_base;
};
typedef struct PyMPIGraphcommObject PyMPIGraphcommObject;

/* "mpi4py/MPI.pxd":138
 *     pass
 * 
 * ctypedef public api class Distgraphcomm(Intracomm) [             # <<<<<<<<<<<<<<
 *     type   PyMPIDistgraphcomm_Type,
 *     object PyMPIDistgraphcommObject,
 */
struct PyMPIDistgraphcommObject {
  struct PyMPIIntracommObject __pyx_base;
};
typedef struct PyMPIDistgraphcommObject PyMPIDistgraphcommObject;

/* "mpi4py/MPI.pxd":144
 *     pass
 * 
 * ctypedef public api class Intercomm(Comm) [             # <<<<<<<<<<<<<<
 *     type   PyMPIIntercomm_Type,
 *     object PyMPIIntercommObject,
 */
struct PyMPIIntercommObject {
  struct PyMPICommObject __pyx_base;
};
typedef struct PyMPIIntercommObject PyMPIIntercommObject;

/* "mpi4py/MPI.pxd":150
 *     pass
 * 
 * ctypedef public api class Win [             # <<<<<<<<<<<<<<
 *     type   PyMPIWin_Type,
 *     object PyMPIWinObject,
 */
struct PyMPIWinObject {
  PyObject_HEAD
  MPI_Win ob_mpi;
  int flags;
};
typedef struct PyMPIWinObject PyMPIWinObject;

/* "mpi4py/MPI.pxd":157
 *     cdef int     flags
 * 
 * ctypedef public api class File [             # <<<<<<<<<<<<<<
 *     type   PyMPIFile_Type,
 *     object PyMPIFileObject,
 */
struct PyMPIFileObject {
  PyObject_HEAD
  MPI_File ob_mpi;
  int flags;
};
typedef struct PyMPIFileObject PyMPIFileObject;

#ifndef __PYX_HAVE_API__mpi4py__MPI

#ifndef __PYX_EXTERN_C
  #ifdef __cplusplus
    #define __PYX_EXTERN_C extern "C"
  #else
    #define __PYX_EXTERN_C extern
  #endif
#endif

__PYX_EXTERN_C DL_IMPORT(PyTypeObject) PyMPIStatus_Type;
__PYX_EXTERN_C DL_IMPORT(PyTypeObject) PyMPIDatatype_Type;
__PYX_EXTERN_C DL_IMPORT(PyTypeObject) PyMPIRequest_Type;
__PYX_EXTERN_C DL_IMPORT(PyTypeObject) PyMPIPrequest_Type;
__PYX_EXTERN_C DL_IMPORT(PyTypeObject) PyMPIGrequest_Type;
__PYX_EXTERN_C DL_IMPORT(PyTypeObject) PyMPIOp_Type;
__PYX_EXTERN_C DL_IMPORT(PyTypeObject) PyMPIGroup_Type;
__PYX_EXTERN_C DL_IMPORT(PyTypeObject) PyMPIInfo_Type;
__PYX_EXTERN_C DL_IMPORT(PyTypeObject) PyMPIErrhandler_Type;
__PYX_EXTERN_C DL_IMPORT(PyTypeObject) PyMPIComm_Type;
__PYX_EXTERN_C DL_IMPORT(PyTypeObject) PyMPIIntracomm_Type;
__PYX_EXTERN_C DL_IMPORT(PyTypeObject) PyMPICartcomm_Type;
__PYX_EXTERN_C DL_IMPORT(PyTypeObject) PyMPIGraphcomm_Type;
__PYX_EXTERN_C DL_IMPORT(PyTypeObject) PyMPIDistgraphcomm_Type;
__PYX_EXTERN_C DL_IMPORT(PyTypeObject) PyMPIIntercomm_Type;
__PYX_EXTERN_C DL_IMPORT(PyTypeObject) PyMPIWin_Type;
__PYX_EXTERN_C DL_IMPORT(PyTypeObject) PyMPIFile_Type;

#endif /* !__PYX_HAVE_API__mpi4py__MPI */

#if PY_MAJOR_VERSION < 3
PyMODINIT_FUNC initMPI(void);
#else
PyMODINIT_FUNC PyInit_MPI(void);
#endif

#endif /* !__PYX_HAVE__mpi4py__MPI */
