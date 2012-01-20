#ifndef __PYX_HAVE_API__mpi4py__MPI
#define __PYX_HAVE_API__mpi4py__MPI
#include "Python.h"
#include "mpi4py.MPI.h"

static PyTypeObject *__pyx_ptype_6mpi4py_3MPI_Status = 0;
#define PyMPIStatus_Type (*__pyx_ptype_6mpi4py_3MPI_Status)
static PyTypeObject *__pyx_ptype_6mpi4py_3MPI_Datatype = 0;
#define PyMPIDatatype_Type (*__pyx_ptype_6mpi4py_3MPI_Datatype)
static PyTypeObject *__pyx_ptype_6mpi4py_3MPI_Request = 0;
#define PyMPIRequest_Type (*__pyx_ptype_6mpi4py_3MPI_Request)
static PyTypeObject *__pyx_ptype_6mpi4py_3MPI_Prequest = 0;
#define PyMPIPrequest_Type (*__pyx_ptype_6mpi4py_3MPI_Prequest)
static PyTypeObject *__pyx_ptype_6mpi4py_3MPI_Grequest = 0;
#define PyMPIGrequest_Type (*__pyx_ptype_6mpi4py_3MPI_Grequest)
static PyTypeObject *__pyx_ptype_6mpi4py_3MPI_Op = 0;
#define PyMPIOp_Type (*__pyx_ptype_6mpi4py_3MPI_Op)
static PyTypeObject *__pyx_ptype_6mpi4py_3MPI_Group = 0;
#define PyMPIGroup_Type (*__pyx_ptype_6mpi4py_3MPI_Group)
static PyTypeObject *__pyx_ptype_6mpi4py_3MPI_Info = 0;
#define PyMPIInfo_Type (*__pyx_ptype_6mpi4py_3MPI_Info)
static PyTypeObject *__pyx_ptype_6mpi4py_3MPI_Errhandler = 0;
#define PyMPIErrhandler_Type (*__pyx_ptype_6mpi4py_3MPI_Errhandler)
static PyTypeObject *__pyx_ptype_6mpi4py_3MPI_Comm = 0;
#define PyMPIComm_Type (*__pyx_ptype_6mpi4py_3MPI_Comm)
static PyTypeObject *__pyx_ptype_6mpi4py_3MPI_Intracomm = 0;
#define PyMPIIntracomm_Type (*__pyx_ptype_6mpi4py_3MPI_Intracomm)
static PyTypeObject *__pyx_ptype_6mpi4py_3MPI_Cartcomm = 0;
#define PyMPICartcomm_Type (*__pyx_ptype_6mpi4py_3MPI_Cartcomm)
static PyTypeObject *__pyx_ptype_6mpi4py_3MPI_Graphcomm = 0;
#define PyMPIGraphcomm_Type (*__pyx_ptype_6mpi4py_3MPI_Graphcomm)
static PyTypeObject *__pyx_ptype_6mpi4py_3MPI_Distgraphcomm = 0;
#define PyMPIDistgraphcomm_Type (*__pyx_ptype_6mpi4py_3MPI_Distgraphcomm)
static PyTypeObject *__pyx_ptype_6mpi4py_3MPI_Intercomm = 0;
#define PyMPIIntercomm_Type (*__pyx_ptype_6mpi4py_3MPI_Intercomm)
static PyTypeObject *__pyx_ptype_6mpi4py_3MPI_Win = 0;
#define PyMPIWin_Type (*__pyx_ptype_6mpi4py_3MPI_Win)
static PyTypeObject *__pyx_ptype_6mpi4py_3MPI_File = 0;
#define PyMPIFile_Type (*__pyx_ptype_6mpi4py_3MPI_File)

static PyObject *(*__pyx_f_6mpi4py_3MPI_PyMPIDatatype_New)(MPI_Datatype) = 0;
#define PyMPIDatatype_New __pyx_f_6mpi4py_3MPI_PyMPIDatatype_New
static MPI_Datatype *(*__pyx_f_6mpi4py_3MPI_PyMPIDatatype_Get)(PyObject *) = 0;
#define PyMPIDatatype_Get __pyx_f_6mpi4py_3MPI_PyMPIDatatype_Get
static PyObject *(*__pyx_f_6mpi4py_3MPI_PyMPIStatus_New)(MPI_Status *) = 0;
#define PyMPIStatus_New __pyx_f_6mpi4py_3MPI_PyMPIStatus_New
static MPI_Status *(*__pyx_f_6mpi4py_3MPI_PyMPIStatus_Get)(PyObject *) = 0;
#define PyMPIStatus_Get __pyx_f_6mpi4py_3MPI_PyMPIStatus_Get
static PyObject *(*__pyx_f_6mpi4py_3MPI_PyMPIRequest_New)(MPI_Request) = 0;
#define PyMPIRequest_New __pyx_f_6mpi4py_3MPI_PyMPIRequest_New
static MPI_Request *(*__pyx_f_6mpi4py_3MPI_PyMPIRequest_Get)(PyObject *) = 0;
#define PyMPIRequest_Get __pyx_f_6mpi4py_3MPI_PyMPIRequest_Get
static PyObject *(*__pyx_f_6mpi4py_3MPI_PyMPIOp_New)(MPI_Op) = 0;
#define PyMPIOp_New __pyx_f_6mpi4py_3MPI_PyMPIOp_New
static MPI_Op *(*__pyx_f_6mpi4py_3MPI_PyMPIOp_Get)(PyObject *) = 0;
#define PyMPIOp_Get __pyx_f_6mpi4py_3MPI_PyMPIOp_Get
static PyObject *(*__pyx_f_6mpi4py_3MPI_PyMPIInfo_New)(MPI_Info) = 0;
#define PyMPIInfo_New __pyx_f_6mpi4py_3MPI_PyMPIInfo_New
static MPI_Info *(*__pyx_f_6mpi4py_3MPI_PyMPIInfo_Get)(PyObject *) = 0;
#define PyMPIInfo_Get __pyx_f_6mpi4py_3MPI_PyMPIInfo_Get
static PyObject *(*__pyx_f_6mpi4py_3MPI_PyMPIGroup_New)(MPI_Group) = 0;
#define PyMPIGroup_New __pyx_f_6mpi4py_3MPI_PyMPIGroup_New
static MPI_Group *(*__pyx_f_6mpi4py_3MPI_PyMPIGroup_Get)(PyObject *) = 0;
#define PyMPIGroup_Get __pyx_f_6mpi4py_3MPI_PyMPIGroup_Get
static PyObject *(*__pyx_f_6mpi4py_3MPI_PyMPIComm_New)(MPI_Comm) = 0;
#define PyMPIComm_New __pyx_f_6mpi4py_3MPI_PyMPIComm_New
static MPI_Comm *(*__pyx_f_6mpi4py_3MPI_PyMPIComm_Get)(PyObject *) = 0;
#define PyMPIComm_Get __pyx_f_6mpi4py_3MPI_PyMPIComm_Get
static PyObject *(*__pyx_f_6mpi4py_3MPI_PyMPIWin_New)(MPI_Win) = 0;
#define PyMPIWin_New __pyx_f_6mpi4py_3MPI_PyMPIWin_New
static MPI_Win *(*__pyx_f_6mpi4py_3MPI_PyMPIWin_Get)(PyObject *) = 0;
#define PyMPIWin_Get __pyx_f_6mpi4py_3MPI_PyMPIWin_Get
static PyObject *(*__pyx_f_6mpi4py_3MPI_PyMPIFile_New)(MPI_File) = 0;
#define PyMPIFile_New __pyx_f_6mpi4py_3MPI_PyMPIFile_New
static MPI_File *(*__pyx_f_6mpi4py_3MPI_PyMPIFile_Get)(PyObject *) = 0;
#define PyMPIFile_Get __pyx_f_6mpi4py_3MPI_PyMPIFile_Get
static PyObject *(*__pyx_f_6mpi4py_3MPI_PyMPIErrhandler_New)(MPI_Errhandler) = 0;
#define PyMPIErrhandler_New __pyx_f_6mpi4py_3MPI_PyMPIErrhandler_New
static MPI_Errhandler *(*__pyx_f_6mpi4py_3MPI_PyMPIErrhandler_Get)(PyObject *) = 0;
#define PyMPIErrhandler_Get __pyx_f_6mpi4py_3MPI_PyMPIErrhandler_Get

#ifndef __PYX_HAVE_RT_ImportModule
#define __PYX_HAVE_RT_ImportModule
static PyObject *__Pyx_ImportModule(const char *name) {
    PyObject *py_name = 0;
    PyObject *py_module = 0;

    #if PY_MAJOR_VERSION < 3
    py_name = PyString_FromString(name);
    #else
    py_name = PyUnicode_FromString(name);
    #endif
    if (!py_name)
        goto bad;
    py_module = PyImport_Import(py_name);
    Py_DECREF(py_name);
    return py_module;
bad:
    Py_XDECREF(py_name);
    return 0;
}
#endif

#ifndef __PYX_HAVE_RT_ImportFunction
#define __PYX_HAVE_RT_ImportFunction
static int __Pyx_ImportFunction(PyObject *module, const char *funcname, void (**f)(void), const char *sig) {
    PyObject *d = 0;
    PyObject *cobj = 0;
    union {
        void (*fp)(void);
        void *p;
    } tmp;

    d = PyObject_GetAttrString(module, (char *)"__pyx_capi__");
    if (!d)
        goto bad;
    cobj = PyDict_GetItemString(d, funcname);
    if (!cobj) {
        PyErr_Format(PyExc_ImportError,
            "%s does not export expected C function %s",
                PyModule_GetName(module), funcname);
        goto bad;
    }
#if PY_VERSION_HEX >= 0x02070000 && !(PY_MAJOR_VERSION==3&&PY_MINOR_VERSION==0)
    if (!PyCapsule_IsValid(cobj, sig)) {
        PyErr_Format(PyExc_TypeError,
            "C function %s.%s has wrong signature (expected %s, got %s)",
             PyModule_GetName(module), funcname, sig, PyCapsule_GetName(cobj));
        goto bad;
    }
    tmp.p = PyCapsule_GetPointer(cobj, sig);
#else
    {const char *desc, *s1, *s2;
    desc = (const char *)PyCObject_GetDesc(cobj);
    if (!desc)
        goto bad;
    s1 = desc; s2 = sig;
    while (*s1 != '\0' && *s1 == *s2) { s1++; s2++; }
    if (*s1 != *s2) {
        PyErr_Format(PyExc_TypeError,
            "C function %s.%s has wrong signature (expected %s, got %s)",
             PyModule_GetName(module), funcname, sig, desc);
        goto bad;
    }
    tmp.p = PyCObject_AsVoidPtr(cobj);}
#endif
    *f = tmp.fp;
    if (!(*f))
        goto bad;
    Py_DECREF(d);
    return 0;
bad:
    Py_XDECREF(d);
    return -1;
}
#endif

#ifndef __PYX_HAVE_RT_ImportType
#define __PYX_HAVE_RT_ImportType
static PyTypeObject *__Pyx_ImportType(const char *module_name, const char *class_name,
    size_t size, int strict)
{
    PyObject *py_module = 0;
    PyObject *result = 0;
    PyObject *py_name = 0;
    char warning[200];

    py_module = __Pyx_ImportModule(module_name);
    if (!py_module)
        goto bad;
    #if PY_MAJOR_VERSION < 3
    py_name = PyString_FromString(class_name);
    #else
    py_name = PyUnicode_FromString(class_name);
    #endif
    if (!py_name)
        goto bad;
    result = PyObject_GetAttr(py_module, py_name);
    Py_DECREF(py_name);
    py_name = 0;
    Py_DECREF(py_module);
    py_module = 0;
    if (!result)
        goto bad;
    if (!PyType_Check(result)) {
        PyErr_Format(PyExc_TypeError,
            "%s.%s is not a type object",
            module_name, class_name);
        goto bad;
    }
    if (!strict && ((PyTypeObject *)result)->tp_basicsize > (Py_ssize_t)size) {
        PyOS_snprintf(warning, sizeof(warning),
            "%s.%s size changed, may indicate binary incompatibility",
            module_name, class_name);
        #if PY_VERSION_HEX < 0x02050000
        if (PyErr_Warn(NULL, warning) < 0) goto bad;
        #else
        if (PyErr_WarnEx(NULL, warning, 0) < 0) goto bad;
        #endif
    }
    else if (((PyTypeObject *)result)->tp_basicsize != (Py_ssize_t)size) {
        PyErr_Format(PyExc_ValueError,
            "%s.%s has the wrong size, try recompiling",
            module_name, class_name);
        goto bad;
    }
    return (PyTypeObject *)result;
bad:
    Py_XDECREF(py_module);
    Py_XDECREF(result);
    return NULL;
}
#endif

static int import_mpi4py__MPI(void) {
  PyObject *module = 0;
  module = __Pyx_ImportModule("mpi4py.MPI");
  if (!module) goto bad;
  if (__Pyx_ImportFunction(module, "PyMPIDatatype_New", (void (**)(void))&__pyx_f_6mpi4py_3MPI_PyMPIDatatype_New, "PyObject *(MPI_Datatype)") < 0) goto bad;
  if (__Pyx_ImportFunction(module, "PyMPIDatatype_Get", (void (**)(void))&__pyx_f_6mpi4py_3MPI_PyMPIDatatype_Get, "MPI_Datatype *(PyObject *)") < 0) goto bad;
  if (__Pyx_ImportFunction(module, "PyMPIStatus_New", (void (**)(void))&__pyx_f_6mpi4py_3MPI_PyMPIStatus_New, "PyObject *(MPI_Status *)") < 0) goto bad;
  if (__Pyx_ImportFunction(module, "PyMPIStatus_Get", (void (**)(void))&__pyx_f_6mpi4py_3MPI_PyMPIStatus_Get, "MPI_Status *(PyObject *)") < 0) goto bad;
  if (__Pyx_ImportFunction(module, "PyMPIRequest_New", (void (**)(void))&__pyx_f_6mpi4py_3MPI_PyMPIRequest_New, "PyObject *(MPI_Request)") < 0) goto bad;
  if (__Pyx_ImportFunction(module, "PyMPIRequest_Get", (void (**)(void))&__pyx_f_6mpi4py_3MPI_PyMPIRequest_Get, "MPI_Request *(PyObject *)") < 0) goto bad;
  if (__Pyx_ImportFunction(module, "PyMPIOp_New", (void (**)(void))&__pyx_f_6mpi4py_3MPI_PyMPIOp_New, "PyObject *(MPI_Op)") < 0) goto bad;
  if (__Pyx_ImportFunction(module, "PyMPIOp_Get", (void (**)(void))&__pyx_f_6mpi4py_3MPI_PyMPIOp_Get, "MPI_Op *(PyObject *)") < 0) goto bad;
  if (__Pyx_ImportFunction(module, "PyMPIInfo_New", (void (**)(void))&__pyx_f_6mpi4py_3MPI_PyMPIInfo_New, "PyObject *(MPI_Info)") < 0) goto bad;
  if (__Pyx_ImportFunction(module, "PyMPIInfo_Get", (void (**)(void))&__pyx_f_6mpi4py_3MPI_PyMPIInfo_Get, "MPI_Info *(PyObject *)") < 0) goto bad;
  if (__Pyx_ImportFunction(module, "PyMPIGroup_New", (void (**)(void))&__pyx_f_6mpi4py_3MPI_PyMPIGroup_New, "PyObject *(MPI_Group)") < 0) goto bad;
  if (__Pyx_ImportFunction(module, "PyMPIGroup_Get", (void (**)(void))&__pyx_f_6mpi4py_3MPI_PyMPIGroup_Get, "MPI_Group *(PyObject *)") < 0) goto bad;
  if (__Pyx_ImportFunction(module, "PyMPIComm_New", (void (**)(void))&__pyx_f_6mpi4py_3MPI_PyMPIComm_New, "PyObject *(MPI_Comm)") < 0) goto bad;
  if (__Pyx_ImportFunction(module, "PyMPIComm_Get", (void (**)(void))&__pyx_f_6mpi4py_3MPI_PyMPIComm_Get, "MPI_Comm *(PyObject *)") < 0) goto bad;
  if (__Pyx_ImportFunction(module, "PyMPIWin_New", (void (**)(void))&__pyx_f_6mpi4py_3MPI_PyMPIWin_New, "PyObject *(MPI_Win)") < 0) goto bad;
  if (__Pyx_ImportFunction(module, "PyMPIWin_Get", (void (**)(void))&__pyx_f_6mpi4py_3MPI_PyMPIWin_Get, "MPI_Win *(PyObject *)") < 0) goto bad;
  if (__Pyx_ImportFunction(module, "PyMPIFile_New", (void (**)(void))&__pyx_f_6mpi4py_3MPI_PyMPIFile_New, "PyObject *(MPI_File)") < 0) goto bad;
  if (__Pyx_ImportFunction(module, "PyMPIFile_Get", (void (**)(void))&__pyx_f_6mpi4py_3MPI_PyMPIFile_Get, "MPI_File *(PyObject *)") < 0) goto bad;
  if (__Pyx_ImportFunction(module, "PyMPIErrhandler_New", (void (**)(void))&__pyx_f_6mpi4py_3MPI_PyMPIErrhandler_New, "PyObject *(MPI_Errhandler)") < 0) goto bad;
  if (__Pyx_ImportFunction(module, "PyMPIErrhandler_Get", (void (**)(void))&__pyx_f_6mpi4py_3MPI_PyMPIErrhandler_Get, "MPI_Errhandler *(PyObject *)") < 0) goto bad;
  Py_DECREF(module); module = 0;
  __pyx_ptype_6mpi4py_3MPI_Status = __Pyx_ImportType("mpi4py.MPI", "Status", sizeof(struct PyMPIStatusObject), 1); if (!__pyx_ptype_6mpi4py_3MPI_Status) goto bad;
  __pyx_ptype_6mpi4py_3MPI_Datatype = __Pyx_ImportType("mpi4py.MPI", "Datatype", sizeof(struct PyMPIDatatypeObject), 1); if (!__pyx_ptype_6mpi4py_3MPI_Datatype) goto bad;
  __pyx_ptype_6mpi4py_3MPI_Request = __Pyx_ImportType("mpi4py.MPI", "Request", sizeof(struct PyMPIRequestObject), 1); if (!__pyx_ptype_6mpi4py_3MPI_Request) goto bad;
  __pyx_ptype_6mpi4py_3MPI_Prequest = __Pyx_ImportType("mpi4py.MPI", "Prequest", sizeof(struct PyMPIPrequestObject), 1); if (!__pyx_ptype_6mpi4py_3MPI_Prequest) goto bad;
  __pyx_ptype_6mpi4py_3MPI_Grequest = __Pyx_ImportType("mpi4py.MPI", "Grequest", sizeof(struct PyMPIGrequestObject), 1); if (!__pyx_ptype_6mpi4py_3MPI_Grequest) goto bad;
  __pyx_ptype_6mpi4py_3MPI_Op = __Pyx_ImportType("mpi4py.MPI", "Op", sizeof(struct PyMPIOpObject), 1); if (!__pyx_ptype_6mpi4py_3MPI_Op) goto bad;
  __pyx_ptype_6mpi4py_3MPI_Group = __Pyx_ImportType("mpi4py.MPI", "Group", sizeof(struct PyMPIGroupObject), 1); if (!__pyx_ptype_6mpi4py_3MPI_Group) goto bad;
  __pyx_ptype_6mpi4py_3MPI_Info = __Pyx_ImportType("mpi4py.MPI", "Info", sizeof(struct PyMPIInfoObject), 1); if (!__pyx_ptype_6mpi4py_3MPI_Info) goto bad;
  __pyx_ptype_6mpi4py_3MPI_Errhandler = __Pyx_ImportType("mpi4py.MPI", "Errhandler", sizeof(struct PyMPIErrhandlerObject), 1); if (!__pyx_ptype_6mpi4py_3MPI_Errhandler) goto bad;
  __pyx_ptype_6mpi4py_3MPI_Comm = __Pyx_ImportType("mpi4py.MPI", "Comm", sizeof(struct PyMPICommObject), 1); if (!__pyx_ptype_6mpi4py_3MPI_Comm) goto bad;
  __pyx_ptype_6mpi4py_3MPI_Intracomm = __Pyx_ImportType("mpi4py.MPI", "Intracomm", sizeof(struct PyMPIIntracommObject), 1); if (!__pyx_ptype_6mpi4py_3MPI_Intracomm) goto bad;
  __pyx_ptype_6mpi4py_3MPI_Cartcomm = __Pyx_ImportType("mpi4py.MPI", "Cartcomm", sizeof(struct PyMPICartcommObject), 1); if (!__pyx_ptype_6mpi4py_3MPI_Cartcomm) goto bad;
  __pyx_ptype_6mpi4py_3MPI_Graphcomm = __Pyx_ImportType("mpi4py.MPI", "Graphcomm", sizeof(struct PyMPIGraphcommObject), 1); if (!__pyx_ptype_6mpi4py_3MPI_Graphcomm) goto bad;
  __pyx_ptype_6mpi4py_3MPI_Distgraphcomm = __Pyx_ImportType("mpi4py.MPI", "Distgraphcomm", sizeof(struct PyMPIDistgraphcommObject), 1); if (!__pyx_ptype_6mpi4py_3MPI_Distgraphcomm) goto bad;
  __pyx_ptype_6mpi4py_3MPI_Intercomm = __Pyx_ImportType("mpi4py.MPI", "Intercomm", sizeof(struct PyMPIIntercommObject), 1); if (!__pyx_ptype_6mpi4py_3MPI_Intercomm) goto bad;
  __pyx_ptype_6mpi4py_3MPI_Win = __Pyx_ImportType("mpi4py.MPI", "Win", sizeof(struct PyMPIWinObject), 1); if (!__pyx_ptype_6mpi4py_3MPI_Win) goto bad;
  __pyx_ptype_6mpi4py_3MPI_File = __Pyx_ImportType("mpi4py.MPI", "File", sizeof(struct PyMPIFileObject), 1); if (!__pyx_ptype_6mpi4py_3MPI_File) goto bad;
  return 0;
  bad:
  Py_XDECREF(module);
  return -1;
}

#endif /* !__PYX_HAVE_API__mpi4py__MPI */
