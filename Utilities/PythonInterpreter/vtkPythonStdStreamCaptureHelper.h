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

#if defined(_WIN32)
#include <cstdio> // for _fileno and the stdin/stdout/stderr streams
#include <io.h>
// MSVC has no STDIN_FILENO and friends. Query the CRT streams instead.
#define VTK_PYTHON_STDIN_FILENO _fileno(stdin)
#define VTK_PYTHON_STDOUT_FILENO _fileno(stdout)
#define VTK_PYTHON_STDERR_FILENO _fileno(stderr)
#else
#include <fcntl.h>
#include <unistd.h>
#define VTK_PYTHON_STDIN_FILENO STDIN_FILENO
#define VTK_PYTHON_STDOUT_FILENO STDOUT_FILENO
#define VTK_PYTHON_STDERR_FILENO STDERR_FILENO
#endif

VTK_ABI_NAMESPACE_BEGIN
struct vtkPythonStdStreamCaptureHelper
{
  PyObject_HEAD
  int softspace;        // Used by print to keep track of its state.
  const char* Encoding; // Encoding, set to "utf-8"
  bool DumpToError;
  int FileDescriptor; // Descriptor of the standard stream this helper stands in for, or -1.

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
    return isatty(VTK_PYTHON_STDIN_FILENO); // when not captured, uses cin
  }

  int Fileno()
  {
    if (this->FileDescriptor < 0)
    {
      return -1;
    }
    // when stdin is captured, input comes from observers, not from a descriptor.
    if (this->FileDescriptor == VTK_PYTHON_STDIN_FILENO && vtkPythonInterpreter::GetCaptureStdin())
    {
      return -1;
    }
#if defined(_WIN32)
    if (_get_osfhandle(this->FileDescriptor) == -1)
#else
    if (fcntl(this->FileDescriptor, F_GETFD) == -1)
#endif
    {
      // no such descriptor, happens for GUI applications started without standard streams.
      return -1;
    }
    return this->FileDescriptor;
  }
  void Close() { this->Flush(); }
};

static PyObject* vtkWrite(PyObject* self, PyObject* args);
static PyObject* vtkRead(PyObject* self, PyObject* args);
static PyObject* vtkFlush(PyObject* self, PyObject* args);
static PyObject* vtkIsatty(PyObject* self, PyObject* args);
static PyObject* vtkClose(PyObject* self, PyObject* args);
static PyObject* vtkFileno(PyObject* self, PyObject* unused);

static PyMethodDef vtkPythonStdStreamCaptureHelperMethods[] = { { "write", vtkWrite, METH_VARARGS,
                                                                  "Dump message" },
  { "readline", vtkRead, METH_VARARGS, "Read input line" },
  { "flush", vtkFlush, METH_VARARGS, "Flush" }, { "isatty", vtkIsatty, METH_VARARGS, "Is a TTY" },
  { "close", vtkClose, METH_VARARGS, "Close" },
  { "fileno", vtkFileno, METH_NOARGS, "File descriptor of the underlying standard stream" },
  { nullptr, nullptr, 0, nullptr } };

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
  { VTK_PYTHON_MEMBER_DEF_STR("encoding"), T_STRING,
    offsetof(vtkPythonStdStreamCaptureHelper, Encoding), READONLY,
    VTK_PYTHON_MEMBER_DEF_STR("Text encoding for file.") },
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

static PyObject* vtkFileno(PyObject* self, PyObject* unused)
{
  (void)unused;
  if (!self || !PyObject_TypeCheck(self, &vtkPythonStdStreamCaptureHelperType))
  {
    return nullptr;
  }

  vtkPythonStdStreamCaptureHelper* wrapper =
    reinterpret_cast<vtkPythonStdStreamCaptureHelper*>(self);
  const int fd = wrapper ? wrapper->Fileno() : -1;
  if (fd < 0)
  {
    // same error `io.UnsupportedOperation` derives from, raised by streams with no descriptor.
    PyErr_SetString(PyExc_OSError, "underlying stream has no file descriptor");
    return nullptr;
  }
  return PyLong_FromLong(fd);
}

static vtkPythonStdStreamCaptureHelper* NewPythonStdStreamCaptureHelper(
  bool for_stderr, int file_descriptor)
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
    wrapper->Encoding = "utf-8";
    wrapper->DumpToError = for_stderr;
    wrapper->FileDescriptor = file_descriptor;
  }

  return wrapper;
}

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkPythonStdStreamCaptureHelper.h
