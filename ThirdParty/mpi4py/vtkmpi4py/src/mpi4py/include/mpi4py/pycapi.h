/* Author:  Lisandro Dalcin   */
/* Contact: dalcinl@gmail.com */

#ifndef MPI4PY_PYCAPI_H
#define MPI4PY_PYCAPI_H

#include <mpi.h>
#include <Python.h>

#define _mpi4py_declare_pycapi(Type, star) \
static PyTypeObject *_mpi4py_PyMPI##Type = NULL; \
static PyObject *(*_mpi4py_PyMPI##Type##_New)(MPI_##Type star) = NULL; \
static MPI_##Type *(*_mpi4py_PyMPI##Type##_Get)(PyObject *) = NULL;

#ifndef MPI4PY_LIMITED_API_SKIP_DATATYPE
_mpi4py_declare_pycapi(Datatype,)
#define PyMPIDatatype_Type (*_mpi4py_PyMPIDatatype)
#define PyMPIDatatype_New _mpi4py_PyMPIDatatype_New
#define PyMPIDatatype_Get _mpi4py_PyMPIDatatype_Get
#endif

#ifndef MPI4PY_LIMITED_API_SKIP_STATUS
_mpi4py_declare_pycapi(Status,*)
#define PyMPIStatus_Type (*_mpi4py_PyMPIStatus)
#define PyMPIStatus_New _mpi4py_PyMPIStatus_New
#define PyMPIStatus_Get _mpi4py_PyMPIStatus_Get
#endif

#ifndef MPI4PY_LIMITED_API_SKIP_REQUEST
_mpi4py_declare_pycapi(Request,)
#define PyMPIRequest_Type (*_mpi4py_PyMPIRequest)
#define PyMPIRequest_New _mpi4py_PyMPIRequest_New
#define PyMPIRequest_Get _mpi4py_PyMPIRequest_Get
#endif

#ifndef MPI4PY_LIMITED_API_SKIP_MESSAGE
_mpi4py_declare_pycapi(Message,)
#define PyMPIMessage_Type (*_mpi4py_PyMPIMessage)
#define PyMPIMessage_New _mpi4py_PyMPIMessage_New
#define PyMPIMessage_Get _mpi4py_PyMPIMessage_Get
#endif

#ifndef MPI4PY_LIMITED_API_SKIP_OP
_mpi4py_declare_pycapi(Op,)
#define PyMPIOp_Type (*_mpi4py_PyMPIOp)
#define PyMPIOp_New _mpi4py_PyMPIOp_New
#define PyMPIOp_Get _mpi4py_PyMPIOp_Get
#endif

#ifndef MPI4PY_LIMITED_API_SKIP_GROUP
_mpi4py_declare_pycapi(Group,)
#define PyMPIGroup_Type (*_mpi4py_PyMPIGroup)
#define PyMPIGroup_New _mpi4py_PyMPIGroup_New
#define PyMPIGroup_Get _mpi4py_PyMPIGroup_Get
#endif

#ifndef MPI4PY_LIMITED_API_SKIP_INFO
_mpi4py_declare_pycapi(Info,)
#define PyMPIInfo_Type (*_mpi4py_PyMPIInfo)
#define PyMPIInfo_New _mpi4py_PyMPIInfo_New
#define PyMPIInfo_Get _mpi4py_PyMPIInfo_Get
#endif

#ifndef MPI4PY_LIMITED_API_SKIP_ERRHANDLER
_mpi4py_declare_pycapi(Errhandler,)
#define PyMPIErrhandler_Type (*_mpi4py_PyMPIErrhandler)
#define PyMPIErrhandler_New _mpi4py_PyMPIErrhandler_New
#define PyMPIErrhandler_Get _mpi4py_PyMPIErrhandler_Get
#endif

#ifndef MPI4PY_LIMITED_API_SKIP_SESSION
_mpi4py_declare_pycapi(Session,)
#define PyMPISession_Type (*_mpi4py_PyMPISession)
#define PyMPISession_New _mpi4py_PyMPISession_New
#define PyMPISession_Get _mpi4py_PyMPISession_Get
#endif

#ifndef MPI4PY_LIMITED_API_SKIP_COMM
_mpi4py_declare_pycapi(Comm,)
#define PyMPIComm_Type (*_mpi4py_PyMPIComm)
#define PyMPIComm_New _mpi4py_PyMPIComm_New
#define PyMPIComm_Get _mpi4py_PyMPIComm_Get
#endif

#ifndef MPI4PY_LIMITED_API_SKIP_WIN
_mpi4py_declare_pycapi(Win,)
#define PyMPIWin_Type (*_mpi4py_PyMPIWin)
#define PyMPIWin_New _mpi4py_PyMPIWin_New
#define PyMPIWin_Get _mpi4py_PyMPIWin_Get
#endif

#ifndef MPI4PY_LIMITED_API_SKIP_FILE
_mpi4py_declare_pycapi(File,)
#define PyMPIFile_Type (*_mpi4py_PyMPIFile)
#define PyMPIFile_New _mpi4py_PyMPIFile_New
#define PyMPIFile_Get _mpi4py_PyMPIFile_Get
#endif

#undef _mpi4py_define_pycapi

static int _mpi4py_ImportType(PyObject *module,
                              const char *type_name,
                              PyTypeObject **type)
{
  PyObject *attr = NULL;
  attr = PyObject_GetAttrString(module, type_name);
  if (!attr)
    goto fn_fail;
  if (!PyType_Check(attr)) {
    PyErr_Format(PyExc_TypeError,
                 "%.200s.%.200s is not a type object",
                 PyModule_GetName(module), type_name);
    goto fn_fail;
  }
  *type = (PyTypeObject *)attr;
  return 0;
 fn_fail:
  Py_DecRef(attr);
  return -1;
}

static int _mpi4py_ImportFunc(PyObject *module,
                              const char *func_name,
                              const char *signature,
                              void (**func)(void))
{
    PyObject *pyxcapi = NULL;
    PyObject *capsule = NULL;
    union { void *obj; void (*fcn)(void); } ptr;
    pyxcapi = PyObject_GetAttrString(module, (char *)"__pyx_capi__");
    if (!pyxcapi)
      goto fn_fail;

    capsule = PyDict_GetItemString(pyxcapi, func_name);
    if (!capsule) {
      PyErr_Format(PyExc_ImportError,
                   "%.200s does not export expected C function %.200s",
                   PyModule_GetName(module), func_name);
      goto fn_fail;
    }
    if (!PyCapsule_CheckExact(capsule)) {
      PyErr_Format(PyExc_TypeError,
                   "%.200s.%.200s is not a capsule",
                   PyModule_GetName(module), func_name);
    }
    if (!signature) {
      signature = PyCapsule_GetName(capsule);
    }
    if (!PyCapsule_IsValid(capsule, signature)) {
      PyErr_Format(PyExc_TypeError,
                   "C function %.200s.%.200s has wrong signature "
                   "(expected %.500s, got %.500s)",
                   PyModule_GetName(module), func_name,
                   signature, PyCapsule_GetName(capsule));
      goto fn_fail;
    }
    ptr.obj = PyCapsule_GetPointer(capsule, signature);
    if (!ptr.obj)
      goto fn_fail;
    *func = ptr.fcn;
    Py_DecRef(pyxcapi);
    return 0;
 fn_fail:
    Py_DecRef(pyxcapi);
    return -1;
}

static int import_mpi4py_MPI(void)
{
  PyObject *module = PyImport_ImportModule("mpi4py.MPI");
  if (!module)
    goto fn_fail;

#define _mpi4py_import_pycapi(Type) do {                                     \
    if (_mpi4py_ImportType(module, #Type, &_mpi4py_PyMPI##Type) < 0)         \
      goto fn_fail;                                                          \
    if (_mpi4py_ImportFunc(module, "PyMPI" #Type "_New", NULL,               \
                           (void (**)(void))&_mpi4py_PyMPI##Type##_New) < 0) \
      goto fn_fail;                                                          \
    if (_mpi4py_ImportFunc(module, "PyMPI" #Type "_Get", NULL,               \
                           (void (**)(void))&_mpi4py_PyMPI##Type##_Get) < 0) \
      goto fn_fail;                                                          \
  } while (0)

#ifndef MPI4PY_LIMITED_API_SKIP_DATATYPE
  _mpi4py_import_pycapi(Datatype);
#endif

#ifndef MPI4PY_LIMITED_API_SKIP_STATUS
  _mpi4py_import_pycapi(Status);
#endif

#ifndef MPI4PY_LIMITED_API_SKIP_REQUEST
  _mpi4py_import_pycapi(Request);
#endif

#ifndef MPI4PY_LIMITED_API_SKIP_MESSAGE
  _mpi4py_import_pycapi(Message);
#endif

#ifndef MPI4PY_LIMITED_API_SKIP_OP
  _mpi4py_import_pycapi(Op);
#endif

#ifndef MPI4PY_LIMITED_API_SKIP_GROUP
  _mpi4py_import_pycapi(Group);
#endif

#ifndef MPI4PY_LIMITED_API_SKIP_INFO
  _mpi4py_import_pycapi(Info);
#endif

#ifndef MPI4PY_LIMITED_API_SKIP_ERRHANDLER
  _mpi4py_import_pycapi(Errhandler);
#endif

#ifndef MPI4PY_LIMITED_API_SKIP_SESSION
  _mpi4py_import_pycapi(Session);
#endif

#ifndef MPI4PY_LIMITED_API_SKIP_COMM
  _mpi4py_import_pycapi(Comm);
#endif

#ifndef MPI4PY_LIMITED_API_SKIP_WIN
  _mpi4py_import_pycapi(Win);
#endif

#ifndef MPI4PY_LIMITED_API_SKIP_FILE
  _mpi4py_import_pycapi(File);
#endif

#undef _mpi4py_import_pycapi

  Py_DecRef(module);
  return 0;
 fn_fail:
  Py_DecRef(module);
  return -1;
}

#define __PYX_HAVE_API__mpi4py__MPI
#define import_mpi4py__MPI import_mpi4py_MPI

#endif /* MPI4PY_PYCAPI_H */
