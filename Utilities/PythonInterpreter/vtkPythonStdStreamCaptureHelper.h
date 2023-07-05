// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPythonStdStreamCaptureHelper
 *
 */

#ifndef vtkPythonStdStreamCaptureHelper_h
#define vtkPythonStdStreamCaptureHelper_h

#include "structmember.h"
#include "vtkPythonCompatibility.h"
#include "vtkPythonInterpreter.h"

VTK_ABI_NAMESPACE_BEGIN
struct vtkPythonStdStreamCaptureHelper
{
  PyObject_HEAD
  int softspace; // Used by print to keep track of its state.
  bool DumpToError;

  void Write(const char* string)
  {
    if (this->DumpToError)
    {
      vtkPythonInterpreter::WriteStdErr(string);
    }
    else
    {
      vtkPythonInterpreter::WriteStdOut(string);
    }
  }

  void Flush()
  {
    if (this->DumpToError)
    {
      vtkPythonInterpreter::FlushStdErr();
    }
    else
    {
      vtkPythonInterpreter::FlushStdOut();
    }
  }

  vtkStdString Read() { return vtkPythonInterpreter::ReadStdin(); }

  bool IsATTY()
  {
    if (vtkPythonInterpreter::GetCaptureStdin())
    {
      return false;
    }
    return isatty(fileno(stdin)); // when not captured, uses cin
  }
  void Close() { this->Flush(); }
};

static PyObject* vtkWrite(PyObject* self, PyObject* args);
static PyObject* vtkRead(PyObject* self, PyObject* args);
static PyObject* vtkFlush(PyObject* self, PyObject* args);
static PyObject* vtkIsatty(PyObject* self, PyObject* args);
static PyObject* vtkClose(PyObject* self, PyObject* args);

static PyMethodDef vtkPythonStdStreamCaptureHelperMethods[] = { { "write", vtkWrite, METH_VARARGS,
                                                                  "Dump message" },
  { "readline", vtkRead, METH_VARARGS, "Read input line" },
  { "flush", vtkFlush, METH_VARARGS, "Flush" }, { "isatty", vtkIsatty, METH_VARARGS, "Is a TTY" },
  { "close", vtkClose, METH_VARARGS, "Close" }, { nullptr, nullptr, 0, nullptr } };

static PyObject* vtkPythonStdStreamCaptureHelperNew(
  PyTypeObject* type, PyObject* /*args*/, PyObject* /*kwds*/)
{
  return type->tp_alloc(type, 0);
}

#if PY_VERSION_HEX >= 0x03070000
#define VTK_PYTHON_MEMBER_DEF_STR(x) x
#else
#define VTK_PYTHON_MEMBER_DEF_STR(x) const_cast<char*>(x)
#endif

static PyMemberDef vtkPythonStdStreamCaptureHelperMembers[] = {
  { VTK_PYTHON_MEMBER_DEF_STR("softspace"), T_INT,
    offsetof(vtkPythonStdStreamCaptureHelper, softspace), 0,
    VTK_PYTHON_MEMBER_DEF_STR("Placeholder so print can keep state.") },
  { nullptr, 0, 0, 0, nullptr }
};

#undef VTK_PYTHON_MEMBER_DEF_STR

#ifdef VTK_PYTHON_NEEDS_DEPRECATION_WARNING_SUPPRESSION
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

// clang-format off
static PyTypeObject vtkPythonStdStreamCaptureHelperType = {
  PyVarObject_HEAD_INIT(&PyType_Type, 0)
  "vtkPythonStdStreamCaptureHelper",       // tp_name
  sizeof(vtkPythonStdStreamCaptureHelper), // tp_basicsize
  0,                                       // tp_itemsize
  nullptr,                                 // tp_dealloc
#if PY_VERSION_HEX >= 0x03080000
  0, // tp_vectorcall_offset
#else
  nullptr, // tp_print
#endif
  nullptr,                                  // tp_getattr
  nullptr,                                  // tp_setattr
  nullptr,                                  // tp_compare
  nullptr,                                  // tp_repr
  nullptr,                                  // tp_as_number
  nullptr,                                  // tp_as_sequence
  nullptr,                                  // tp_as_mapping
  nullptr,                                  // tp_hash
  nullptr,                                  // tp_call
  nullptr,                                  // tp_str
  PyObject_GenericGetAttr,                  // tp_getattro
  PyObject_GenericSetAttr,                  // tp_setattro
  nullptr,                                  // tp_as_buffer
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, // tp_flags
  "vtkPythonStdStreamCaptureHelper",        //  tp_doc
  nullptr,                                  //  tp_traverse
  nullptr,                                  //  tp_clear
  nullptr,                                  //  tp_richcompare
  0,                                        //  tp_weaklistoffset
  nullptr,                                  //  tp_iter
  nullptr,                                  //  tp_iternext
  vtkPythonStdStreamCaptureHelperMethods,   //  tp_methods
  vtkPythonStdStreamCaptureHelperMembers,   //  tp_members
  nullptr,                                  //  tp_getset
  nullptr,                                  //  tp_base
  nullptr,                                  //  tp_dict
  nullptr,                                  //  tp_descr_get
  nullptr,                                  //  tp_descr_set
  0,                                        //  tp_dictoffset
  nullptr,                                  //  tp_init
  nullptr,                                  //  tp_alloc
  vtkPythonStdStreamCaptureHelperNew,       //  tp_new
  nullptr,                                  // freefunc tp_free; /* Low-level free-memory routine */
  nullptr,                                  // inquiry tp_is_gc; /* For PyObject_IS_GC */
  nullptr,                                  // PyObject *tp_bases;
  nullptr,                                  // PyObject *tp_mro; /* method resolution order */
  nullptr,                                  // PyObject *tp_cache;
  nullptr,                                  // PyObject *tp_subclasses;
  nullptr,                                  // PyObject *tp_weaklist;
  VTK_WRAP_PYTHON_SUPPRESS_UNINITIALIZED
};
// clang-format on

static PyObject* vtkWrite(PyObject* self, PyObject* args)
{
  if (!self || !PyObject_TypeCheck(self, &vtkPythonStdStreamCaptureHelperType))
  {
    return nullptr;
  }

  vtkPythonStdStreamCaptureHelper* wrapper =
    reinterpret_cast<vtkPythonStdStreamCaptureHelper*>(self);

  char* string;
  // const_cast since older versions of python are not const correct.
  if (wrapper && PyArg_ParseTuple(args, "s", &string))
  {
    wrapper->Write(string);
  }
  return Py_BuildValue("");
}

static PyObject* vtkRead(PyObject* self, PyObject* args)
{
  (void)args;
  if (!self || !PyObject_TypeCheck(self, &vtkPythonStdStreamCaptureHelperType))
  {
    return nullptr;
  }

  vtkPythonStdStreamCaptureHelper* wrapper =
    reinterpret_cast<vtkPythonStdStreamCaptureHelper*>(self);

  std::string ret;
  if (wrapper)
  {
    ret = wrapper->Read();
  }
  return Py_BuildValue("s", ret.c_str());
}

static PyObject* vtkFlush(PyObject* self, PyObject* args)
{
  (void)args;
  if (!self || !PyObject_TypeCheck(self, &vtkPythonStdStreamCaptureHelperType))
  {
    return nullptr;
  }

  vtkPythonStdStreamCaptureHelper* wrapper =
    reinterpret_cast<vtkPythonStdStreamCaptureHelper*>(self);
  if (wrapper)
  {
    wrapper->Flush();
  }
  return Py_BuildValue("");
}

static PyObject* vtkIsatty(PyObject* self, PyObject* args)
{
  (void)args;
  if (!self || !PyObject_TypeCheck(self, &vtkPythonStdStreamCaptureHelperType))
  {
    return nullptr;
  }
  vtkPythonStdStreamCaptureHelper* wrapper =
    reinterpret_cast<vtkPythonStdStreamCaptureHelper*>(self);
  if (wrapper->IsATTY())
  {
    Py_INCREF(Py_True);
    return Py_True;
  }
  Py_INCREF(Py_False);
  return Py_False;
}

static PyObject* vtkClose(PyObject* self, PyObject* args)
{
  (void)args;
  if (!self || !PyObject_TypeCheck(self, &vtkPythonStdStreamCaptureHelperType))
  {
    return nullptr;
  }

  vtkPythonStdStreamCaptureHelper* wrapper =
    reinterpret_cast<vtkPythonStdStreamCaptureHelper*>(self);
  if (wrapper)
  {
    wrapper->Close();
  }
  return Py_BuildValue("");
}

static vtkPythonStdStreamCaptureHelper* NewPythonStdStreamCaptureHelper(bool for_stderr = false)
{
  vtkPythonScopeGilEnsurer gilEnsurer;
  if (PyType_Ready(&vtkPythonStdStreamCaptureHelperType) < 0)
  {
    return nullptr;
  }

  vtkPythonStdStreamCaptureHelper* wrapper =
    PyObject_New(vtkPythonStdStreamCaptureHelper, &vtkPythonStdStreamCaptureHelperType);
  if (wrapper)
  {
    wrapper->DumpToError = for_stderr;
  }

  return wrapper;
}

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkPythonStdStreamCaptureHelper.h
