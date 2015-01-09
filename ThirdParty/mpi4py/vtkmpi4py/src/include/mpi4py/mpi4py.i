/* Author:  Lisandro Dalcin   */
/* Contact: dalcinl@gmail.com */

/* ---------------------------------------------------------------- */

#if SWIG_VERSION < 0x010328
%warn "SWIG version < 1.3.28 is not supported"
#endif

/* ---------------------------------------------------------------- */

%header %{
#include "mpi4py/mpi4py.h"
%}

%init %{
if (import_mpi4py() < 0)
#if PY_VERSION_HEX >= 0x03000000
  return NULL;
#else
  return;
#endif
%}

/* ---------------------------------------------------------------- */

%define %mpi4py_fragments(PyType, Type)
/* --- AsPtr --- */
%fragment(SWIG_AsPtr_frag(Type),"header") {
SWIGINTERN int
SWIG_AsPtr_dec(Type)(SWIG_Object input, Type **p) {
  if (input == Py_None) {
    if (p) *p = 0;
    return SWIG_OK;
  } else if (PyObject_TypeCheck(input,&PyMPI##PyType##_Type)) {
    if (p) *p = PyMPI##PyType##_Get(input);
    return SWIG_OK;
  } else {
    void *argp = 0;
    int res = SWIG_ConvertPtr(input,&argp,%descriptor(p_##Type), 0);
    if (!SWIG_IsOK(res)) return res;
    if (!argp) return SWIG_ValueError;
    if (p) *p = %static_cast(argp,Type*);
    return SWIG_OK;
  }
}
}
/* --- From --- */
%fragment(SWIG_From_frag(Type),"header")
{
SWIGINTERN SWIG_Object
SWIG_From_dec(Type)(Type v) {
  return PyMPI##PyType##_New(v);
}
}
%enddef /*mpi4py_fragments*/

/* ---------------------------------------------------------------- */

%define SWIG_TYPECHECK_MPI_Comm       400 %enddef
%define SWIG_TYPECHECK_MPI_Datatype   401 %enddef
%define SWIG_TYPECHECK_MPI_Request    402 %enddef
%define SWIG_TYPECHECK_MPI_Status     403 %enddef
%define SWIG_TYPECHECK_MPI_Op         404 %enddef
%define SWIG_TYPECHECK_MPI_Group      405 %enddef
%define SWIG_TYPECHECK_MPI_Info       406 %enddef
%define SWIG_TYPECHECK_MPI_File       407 %enddef
%define SWIG_TYPECHECK_MPI_Win        408 %enddef
%define SWIG_TYPECHECK_MPI_Errhandler 409 %enddef

%define %mpi4py_checkcode(Type)
%checkcode(Type)
%enddef /*mpi4py_checkcode*/

/* ---------------------------------------------------------------- */

%define %mpi4py_typemap(PyType, Type)
%types(Type*);
%mpi4py_fragments(PyType, Type);
%typemaps_asptrfromn(%mpi4py_checkcode(Type), Type);
%enddef /*mpi4py_typemap*/

/* ---------------------------------------------------------------- */


/*
 * Local Variables:
 * mode: C
 * End:
 */
