/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPythonArgs.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-----------------------------------------------------------------------
The vtkPythonArgs class was created in Oct 2010 by David Gobbi for.

This class provides methods for reading an argument tuple from Python
and converting it to types that can be used by VTK.  It is meant to be
more efficient and flexible that the original PyArg_ParseTuple() code,
resulting in wrapper code that is faster and more compact.
-----------------------------------------------------------------------*/

// .NAME vtkPythonArgs

#ifndef vtkPythonArgs_h
#define vtkPythonArgs_h

#include "vtkWrappingPythonCoreModule.h" // For export macro
#include "vtkPythonUtil.h"
#include "PyVTKClass.h"
#include "PyVTKTemplate.h"

#include "vtkConfigure.h"
#include "vtkUnicodeString.h"

#include <string>

class VTKWRAPPINGPYTHONCORE_EXPORT vtkPythonArgs
{
public:

  // Description:
  // Constructor for parsing args of a vtkObjectBase object.
  vtkPythonArgs(PyObject *self, PyObject *args, const char *methodname) :
      Args(args), MethodName(methodname) {
      this->N = PyTuple_GET_SIZE(args);
      this->M = PyVTKClass_Check(self);
      this->I = this->M;
    }

  // Description:
  // Constructor for parsing method args.
  vtkPythonArgs(PyObject *args, const char *methodname) :
    Args(args), MethodName(methodname) {
      this->N = PyTuple_GET_SIZE(args);
      this->M = 0;
      this->I = 0;
    }

  // Description:
  // Reset in order to re-parse the args.
  void Reset() { this->I = this->M; }

  // Description:
  // Get a pointer to the self object, converted to its C++ type.
  // Returns NULL and sets a TypeError if the type is wrong.
  // If "self" is a PyVTKClass, pull the object from the first arg.
  static vtkObjectBase *GetSelfPointer(PyObject *self, PyObject *args);

  // Description:
  // Get a pointer to the self object, converted to its C++ type.
  // Always succeeds.
  static void *GetSelfPointer(PyObject *self);

  // Description:
  // Verify the arg count for a method with optional arguments.
  bool CheckArgCount(int nmin, int nmax);

  // Description:
  // Verify the arg count.  Sets a python exception on failure.
  bool CheckArgCount(int n);

  // Description:
  // Returns true if self is an object, false if self is a class.
  bool IsBound() { return (this->M == 0); }

  // Description:
  // Raise an exception if method call is not bound.
  bool IsPureVirtual();

  // Description:
  // Check if an error has occurred.
  static bool ErrorOccurred();

  // Description:
  // Check if there are any args left.
  bool NoArgsLeft() { return (this->I >= this->N); }

  // Description:
  // Get the size of an arg, if it is a sequence.
  // If no size is available, or if the arg is out of range,
  // then it returns 0 but doesn't set error.
  int GetArgSize(int i);

  // Description:
  // Get the next argument as a naked Python object.
  bool GetPythonObject(PyObject *&v) {
    bool b;
    v = this->GetArgAsPythonObject(b);
    return b; }
  bool GetPythonObject(PyObject *o, PyObject *&v) {
    bool b;
    v = vtkPythonArgs::GetArgAsPythonObject(o, b);
    return b; }

  // Description:
  // Get the next argument as a vtkObjectBase derived type.
  // It uses a C-style cast instead of a static_cast, which
  // means that it works on incomplete types, and also means
  // that it will give undefined results if "T" uses multiple
  // inheritance.
  template<class T>
  bool GetVTKObject(T *&v, const char *classname) {
    bool b;
    v = (T *)this->GetArgAsVTKObject(classname, b);
    return b; }
  template<class T>
  bool GetVTKObject(PyObject *o, T *&v, const char *classname) {
    bool b;
    v = (T *)vtkPythonArgs::GetArgAsVTKObject(o, classname, b);
    return b; }

  // Description:
  // Get the next argument as a special object.  If a constructor
  // was needed to convert the arg, the constructed object will be
  // returned in "o" and must be freed after "v" is used.
  template<class T>
  bool GetSpecialObject(T *&v, PyObject *&o, const char *classname) {
    v = static_cast<T *>(this->GetArgAsSpecialObject(classname, &o));
    return (v != NULL); }
  template<class T>
  static bool GetSpecialObject(
    PyObject *arg, T *&v, PyObject *&o, const char *classname) {
    v = static_cast<T *>(
      vtkPythonArgs::GetArgAsSpecialObject(arg, classname, &o));
    return (v != NULL); }

  // Description:
  // Get the next argument as a special object.  Use this if the
  // arg is a non-const ref, as it will disallow conversion.
  template<class T>
  bool GetSpecialObject(T *&v, const char *classname) {
    v = static_cast<T *>(this->GetArgAsSpecialObject(classname, NULL));
    return (v != NULL); }
  template<class T>
  static bool GetSpecialObject(PyObject *o, T *&v, const char *classname) {
    v = static_cast<T *>(
      vtkPythonArgs::GetArgAsSpecialObject(o, classname, NULL));
    return (v != NULL); }

  // Description:
  // Get the next argument as an enum value.
  template<class T>
  bool GetEnumValue(T &v, PyTypeObject *enumtype) {
    bool r;
    v = static_cast<T>(this->GetArgAsEnum(enumtype, r));
    return r; }
  template<class T>
  static bool GetEnumValue(PyObject *o, T &v, PyTypeObject *enumtype) {
    bool r;
    v = static_cast<T>(vtkPythonArgs::GetArgAsEnum(o, enumtype, r));
    return r; }

  // Description:
  // Get the next argument as a SIP object.
  template<class T>
  bool GetSIPObject(T *&v, const char *classname) {
    bool r;
    v = (T *)this->GetArgAsSIPObject(classname, r);
    return r; }
  template<class T>
  static bool GetSIPObject(PyObject *o, T *&v, const char *classname) {
    bool r;
    v = (T *)vtkPythonArgs::GetArgAsSIPObject(o, classname, r);
    return r; }

  // Description:
  // Get the next argument as a SIP enum value.
  template<class T>
  bool GetSIPEnumValue(T &v, const char *enumname) {
    bool r;
    v = static_cast<T>(this->GetArgAsSIPEnum(enumname, r));
    return r; }
  template<class T>
  static bool GetSIPEnumValue(PyObject *o, T &v, const char *enumname) {
    bool r;
    v = static_cast<T>(vtkPythonArgs::GetArgAsSIPEnum(o, enumname, r));
    return r; }

  // Description:
  // Get the arguments needed for a SetExecuteMethod or a similar
  // method that requires a function-pointer argument.
  bool GetFunction(PyObject *&o);
  static bool GetFunction(PyObject *arg, PyObject *&o);

  // Get the next arg as a void pointer (to a buffer object).
  bool GetValue(void *&v);
  static bool GetValue(PyObject *o, void *&v);
  bool GetValue(const void *&v);
  static bool GetValue(PyObject *o, const void *&v);

  // Description:
  // Get the next argument as a string.
  bool GetValue(const char *&v);
  static bool GetValue(PyObject *o, const char *&v);
  bool GetValue(char *&v);
  static bool GetValue(PyObject *o, char *&v);
  bool GetValue(std::string &v);
  static bool GetValue(PyObject *o, std::string &v);
  bool GetValue(vtkUnicodeString &v);
  static bool GetValue(PyObject *o, vtkUnicodeString &v);

  // Description:
  // Get the next string arg as a character.
  bool GetValue(char &v);
  static bool GetValue(PyObject *o, char &v);

  // Description:
  // Get the next argument.  Sets a TypeError on failure.
  bool GetValue(float &v);
  static bool GetValue(PyObject *o, float &v);
  bool GetValue(double &v);
  static bool GetValue(PyObject *o, double &v);
  bool GetValue(bool &v);
  static bool GetValue(PyObject *o, bool &v);
  bool GetValue(signed char &v);
  static bool GetValue(PyObject *o, signed char &v);
  bool GetValue(unsigned char &v);
  static bool GetValue(PyObject *o, unsigned char &v);
  bool GetValue(short &v);
  static bool GetValue(PyObject *o, short &v);
  bool GetValue(unsigned short &v);
  static bool GetValue(PyObject *o, unsigned short &v);
  bool GetValue(int &v);
  static bool GetValue(PyObject *o, int &v);
  bool GetValue(unsigned int &v);
  static bool GetValue(PyObject *o, unsigned int &v);
  bool GetValue(long &v);
  static bool GetValue(PyObject *o, long &v);
  bool GetValue(unsigned long &v);
  static bool GetValue(PyObject *o, unsigned long &v);
#ifdef VTK_TYPE_USE_LONG_LONG
  bool GetValue(long long &v);
  static bool GetValue(PyObject *o, long long &v);
  bool GetValue(unsigned long long &v);
  static bool GetValue(PyObject *o, unsigned long long &v);
#endif
#ifdef VTK_TYPE_USE___INT64
  bool GetValue(__int64 &v);
  static bool GetValue(PyObject *o, __int64 &v);
  bool GetValue(unsigned __int64 &v);
  static bool GetValue(PyObject *o, unsigned __int64 &v);
#endif

  // Description:
  // Get the next argument as an array.
  bool GetArray(float *v, int n);
  bool GetArray(double *v, int n);
  bool GetArray(bool *v, int n);
  bool GetArray(char *v, int n);
  bool GetArray(signed char *v, int n);
  bool GetArray(unsigned char *v, int n);
  bool GetArray(short *v, int n);
  bool GetArray(unsigned short *v, int n);
  bool GetArray(int *v, int n);
  bool GetArray(unsigned int *v, int n);
  bool GetArray(long *v, int n);
  bool GetArray(unsigned long *v, int n);
#ifdef VTK_TYPE_USE_LONG_LONG
  bool GetArray(long long *v, int n);
  bool GetArray(unsigned long long *v, int n);
#endif
#ifdef VTK_TYPE_USE___INT64
  bool GetArray(__int64 *v, int n);
  bool GetArray(unsigned __int64 *v, int n);
#endif

  // Description:
  // Get the next argument as a multi-dimensional array.
  bool GetNArray(float *v, int ndims, const int *dims);
  bool GetNArray(double *v, int ndims, const int *dims);
  bool GetNArray(bool *v, int ndims, const int *dims);
  bool GetNArray(char *v, int ndims, const int *dims);
  bool GetNArray(signed char *v, int ndims, const int *dims);
  bool GetNArray(unsigned char *v, int ndims, const int *dims);
  bool GetNArray(short *v, int ndims, const int *dims);
  bool GetNArray(unsigned short *v, int ndims, const int *dims);
  bool GetNArray(int *v, int ndims, const int *dims);
  bool GetNArray(unsigned int *v, int ndims, const int *dims);
  bool GetNArray(long *v, int ndims, const int *dims);
  bool GetNArray(unsigned long *v, int ndims, const int *dims);
#ifdef VTK_TYPE_USE_LONG_LONG
  bool GetNArray(long long *v, int ndims, const int *dims);
  bool GetNArray(unsigned long long *v, int ndims, const int *dims);
#endif
#ifdef VTK_TYPE_USE___INT64
  bool GetNArray(__int64 *v, int ndims, const int *dims);
  bool GetNArray(unsigned __int64 *v, int ndims, const int *dims);
#endif

  // Description:
  // Set the value of an argument if it is an assignable type.
  bool SetArgValue(int i, const std::string &v);
  bool SetArgValue(int i, const vtkUnicodeString &v);
  bool SetArgValue(int i, char v);
  bool SetArgValue(int i, float v);
  bool SetArgValue(int i, double v);
  bool SetArgValue(int i, bool v);
  bool SetArgValue(int i, signed char v);
  bool SetArgValue(int i, unsigned char v);
  bool SetArgValue(int i, short v);
  bool SetArgValue(int i, unsigned short v);
  bool SetArgValue(int i, int v);
  bool SetArgValue(int i, unsigned int v);
  bool SetArgValue(int i, long v);
  bool SetArgValue(int i, unsigned long v);
#ifdef VTK_TYPE_USE_LONG_LONG
  bool SetArgValue(int i, long long v);
  bool SetArgValue(int i, unsigned long long v);
#endif
#ifdef VTK_TYPE_USE___INT64
  bool SetArgValue(int i, __int64 v);
  bool SetArgValue(int i, unsigned __int64 v);
#endif

  // Description:
  // Set the values in an array argument.
  bool SetArray(int i, const float *v, int n);
  bool SetArray(int i, const double *v, int n);
  bool SetArray(int i, const bool *v, int n);
  bool SetArray(int i, const char *v, int n);
  bool SetArray(int i, const signed char *v, int n);
  bool SetArray(int i, const unsigned char *v, int n);
  bool SetArray(int i, const short *v, int n);
  bool SetArray(int i, const unsigned short *v, int n);
  bool SetArray(int i, const int *v, int n);
  bool SetArray(int i, const unsigned int *v, int n);
  bool SetArray(int i, const long *v, int n);
  bool SetArray(int i, const unsigned long *v, int n);
#ifdef VTK_TYPE_USE_LONG_LONG
  bool SetArray(int i, const long long *v, int n);
  bool SetArray(int i, const unsigned long long *v, int n);
#endif
#ifdef VTK_TYPE_USE___INT64
  bool SetArray(int i, const __int64 *v, int n);
  bool SetArray(int i, const unsigned __int64 *v, int n);
#endif

  // Description:
  // Set the values in a multi-dimensional array argument.
  bool SetNArray(int i, const float *v, int n, const int *d);
  bool SetNArray(int i, const double *v, int n, const int *d);
  bool SetNArray(int i, const bool *v, int n, const int *d);
  bool SetNArray(int i, const char *v, int n, const int *d);
  bool SetNArray(int i, const signed char *v, int n, const int *d);
  bool SetNArray(int i, const unsigned char *v, int n, const int *d);
  bool SetNArray(int i, const short *v, int n, const int *d);
  bool SetNArray(int i, const unsigned short *v, int n, const int *d);
  bool SetNArray(int i, const int *v, int n, const int *d);
  bool SetNArray(int i, const unsigned int *v, int n, const int *d);
  bool SetNArray(int i, const long *v, int n, const int *d);
  bool SetNArray(int i, const unsigned long *v, int n, const int *d);
#ifdef VTK_TYPE_USE_LONG_LONG
  bool SetNArray(int i, const long long *v, int n, const int *d);
  bool SetNArray(int i, const unsigned long long *v, int n, const int *d);
#endif
#ifdef VTK_TYPE_USE___INT64
  bool SetNArray(int i, const __int64 *v, int n, const int *d);
  bool SetNArray(int i, const unsigned __int64 *v, int n, const int *d);
#endif

  // Description:
  // Build a value of None.
  static PyObject *BuildNone();

  // Description:
  // Build a vtkObjectBase object, use GetClassName() to get its type.
  // If a null pointer is given, then None will be returned.
  static PyObject *BuildVTKObject(const void *v);

  // Description:
  // Build a non-vtkObjectBase object of the specified type.
  static PyObject *BuildSpecialObject(const void *v, const char *classname);

  // Description:
  // Build an enum value object of the specified type.
  static PyObject *BuildEnumValue(int v, const char *enumname);

  // Description:
  // Build a SIP object of the specified type.  Set "created" to true
  // if the object was just created with new.
  static PyObject *BuildSIPObject(
    const void *v, const char *classname, bool created);

  // Description:
  // Build a SIP enum object.
  static PyObject *BuildSIPEnumValue(int v, const char *classname);

  // Description:
  // Create a mangled string containing a memory address.
  static PyObject *BuildValue(const void *v);

  // Description:
  // Build a string return value.
  static PyObject *BuildValue(const char *v);
  static PyObject *BuildValue(const std::string &v);
  static PyObject *BuildValue(const vtkUnicodeString &v);

  // Description:
  // Build a char return value.
  static PyObject *BuildValue(char v);

  // Description:
  // Build a numeric return value.
  static PyObject *BuildValue(double v);
  static PyObject *BuildValue(bool v);
  static PyObject *BuildValue(int v);
  static PyObject *BuildValue(unsigned int v);
  static PyObject *BuildValue(long v);
  static PyObject *BuildValue(unsigned long v);
#ifdef VTK_TYPE_USE_LONG_LONG
  static PyObject *BuildValue(long long v);
  static PyObject *BuildValue(unsigned long long v);
#endif
#ifdef VTK_TYPE_USE___INT64
  static PyObject *BuildValue(__int64 v);
  static PyObject *BuildValue(unsigned __int64 v);
#endif

  // Description:
  // Build a bytes object (or string).
  static PyObject *BuildBytes(const char *v, int n);

  // Description:
  // Build a tuple for a return value.
  static PyObject *BuildTuple(const float *v, int n);
  static PyObject *BuildTuple(const double *v, int n);
  static PyObject *BuildTuple(const bool *v, int n);
  static PyObject *BuildTuple(const signed char *v, int n);
  static PyObject *BuildTuple(const unsigned char *v, int n);
  static PyObject *BuildTuple(const short *v, int n);
  static PyObject *BuildTuple(const unsigned short *v, int n);
  static PyObject *BuildTuple(const int *v, int n);
  static PyObject *BuildTuple(const unsigned int *v, int n);
  static PyObject *BuildTuple(const long *v, int n);
  static PyObject *BuildTuple(const unsigned long *v, int n);
#ifdef VTK_TYPE_USE_LONG_LONG
  static PyObject *BuildTuple(const long long *v, int n);
  static PyObject *BuildTuple(const unsigned long long *v, int n);
#endif
#ifdef VTK_TYPE_USE___INT64
  static PyObject *BuildTuple(const __int64 *v, int n);
  static PyObject *BuildTuple(const unsigned __int64 *v, int n);
#endif

  // Description:
  // Copy an array.
  template<class T>
  static void SaveArray(const T *a, T *b, int n) {
    int i = 0;
    do { b[i] = a[i]; } while (++i < n); }

  // Description:
  // Check if an array has changed.
  template<class T>
  static bool ArrayHasChanged(const T *a, const T *b, int n) {
    int i = 0;
    do { if (a[i] != b[i]) break; } while (++i < n);
    return (i < n); }

  // Description:
  // Get the argument count.
  static int GetArgCount(PyObject *args) {
    return static_cast<int>(PyTuple_GET_SIZE(args)); }

  // Description:
  // Get the argument count for a method that might be unbound.
  static int GetArgCount(PyObject *self, PyObject *args) {
    return (static_cast<int>(PyTuple_GET_SIZE(args)) -
            PyVTKClass_Check(self)); }

  // Description:
  // Raise a type error just saying that the arg count is wrong.
  static bool ArgCountError(int n, const char *name);


protected:

  // Description:
  // Get the "self" object from the first argument.
  static vtkObjectBase *GetSelfFromFirstArg(PyObject *self, PyObject *args);

  // Description:
  // Get the next argument as an object of the given type.
  PyObject *GetArgAsPythonObject(bool &valid);
  static PyObject *GetArgAsPythonObject(
    PyObject *o, bool &valid);

  // Description:
  // Get the next argument as an object of the given type.
  vtkObjectBase *GetArgAsVTKObject(const char *classname, bool &valid);
  static vtkObjectBase *GetArgAsVTKObject(
    PyObject *o, const char *classname, bool &valid);

  // Description:
  // Get the next argument as an object of the given type.
  void *GetArgAsSpecialObject(const char *classname, PyObject **newobj);
  static void *GetArgAsSpecialObject(
    PyObject *o, const char *classname, PyObject **newobj);

  // Description:
  // Get the next argument as an object of the given type.
  int GetArgAsEnum(PyTypeObject *enumtype, bool &valid);
  static int GetArgAsEnum(
    PyObject *o, PyTypeObject *enumtype, bool &valid);

  // Description:
  // Get the next argument as an object of the given type.
  void *GetArgAsSIPObject(const char *classname, bool &valid);
  static void *GetArgAsSIPObject(
    PyObject *o, const char *classname, bool &valid);

  // Description:
  // Get the next argument as an object of the given type.
  int GetArgAsSIPEnum(const char *classname, bool &valid);
  static int GetArgAsSIPEnum(
    PyObject *o, const char *classname, bool &valid);

  // Description:
  // Raise a TypeError if a virtual method call was called.
  bool PureVirtualError();

  // Description:
  // Raise an TypeError stating that the arg count is incorrect.
  bool ArgCountError(int m, int n);

  // Description:
  // Prefix a TypeError that has occurred with the arg number.
  bool RefineArgTypeError(int i);

private:

  PyObject *Args;
  const char *MethodName;

  int N; // size of args tuple
  int M; // 1 if Self is a PyVTKClass and first arg is the PyVTKObject
  int I; // the arg counter, starts at M
};

//--------------------------------------------------------------------
// Inline methods for getting "self" as its original type

// Get "self" from a PyVTKObject, which contains a vtkObjectBase object.
inline
vtkObjectBase *vtkPythonArgs::GetSelfPointer(PyObject *self, PyObject *args)
{
  if (PyVTKClass_Check(self))
    {
    return vtkPythonArgs::GetSelfFromFirstArg(self, args);
    }
  return ((PyVTKObject *)self)->vtk_ptr;
}

// Get "self" from a PyVTKSpecialObject.
inline
void *vtkPythonArgs::GetSelfPointer(PyObject *self)
{
  return ((PyVTKSpecialObject *)self)->vtk_ptr;
}

//--------------------------------------------------------------------
// Inline methods for checking the arg count

// Verify the arg count for a method with optional arguments.
inline
bool vtkPythonArgs::CheckArgCount(int nmin, int nmax)
{
  int nargs = this->N - this->M;
  if (nargs >= nmin && nargs <= nmax)
    {
    return true;
    }
  this->ArgCountError(nmin, nmax);
  return false;
}

// Verify the arg count for a method with optional arguments.
inline
bool vtkPythonArgs::CheckArgCount(int n)
{
  int nargs = this->N - this->M;
  if (nargs == n)
    {
    return true;
    }
  this->ArgCountError(n, n);
  return false;
}

//--------------------------------------------------------------------
// Inline method for guarding against pure virtual method calls

inline
bool vtkPythonArgs::IsPureVirtual()
{
  if (IsBound())
    {
    return false;
    }
  this->PureVirtualError();
  return true;
}

//--------------------------------------------------------------------
// Inline method for checking if an error has occurred.

inline
bool vtkPythonArgs::ErrorOccurred()
{
  return (PyErr_Occurred() != NULL);
}

//--------------------------------------------------------------------
// Inline methods for building python objects of various types.

inline
PyObject *vtkPythonArgs::BuildNone()
{
  Py_INCREF(Py_None);
  return Py_None;
}

inline
PyObject *vtkPythonArgs::BuildVTKObject(const void *v)
{
  return vtkPythonUtil::GetObjectFromPointer(
    static_cast<vtkObjectBase *>(const_cast<void *>(v)));
}

inline
PyObject *vtkPythonArgs::BuildSpecialObject(const void *v,
                                            const char *classname)
{
  return PyVTKSpecialObject_CopyNew(classname, v);
}

inline
PyObject *vtkPythonArgs::BuildEnumValue(int, const char *)
{
  /* not implemented */
  return NULL;
}

inline
PyObject *vtkPythonArgs::BuildSIPObject(
  const void *v, const char *classname, bool created)
{
  return vtkPythonUtil::SIPGetObjectFromPointer(v, classname, created);
}

inline
PyObject *vtkPythonArgs::BuildSIPEnumValue(int, const char *)
{
  /* not implemented */
  return NULL;
}

inline
PyObject *vtkPythonArgs::BuildValue(const void *a)
{
  if (a)
    {
    const char *s = vtkPythonUtil::ManglePointer(a, "p_void");
    return PyString_FromString(s);
    }
  Py_INCREF(Py_None);
  return Py_None;
}

inline
PyObject *vtkPythonArgs::BuildValue(const char *a)
{
  if (a)
    {
    return PyString_FromString(a);
    }
  Py_INCREF(Py_None);
  return Py_None;
}

inline
PyObject *vtkPythonArgs::BuildValue(const std::string &a)
{
  return PyString_FromStringAndSize(a.c_str(), static_cast<Py_ssize_t>(a.size()));
}

inline
PyObject *vtkPythonArgs::BuildValue(const vtkUnicodeString &a)
{
  std::string s;
  a.utf8_str(s);
#ifdef Py_USING_UNICODE
  return PyUnicode_DecodeUTF8(s.c_str(), static_cast<Py_ssize_t>(s.size()), NULL);
#else
  return PyString_FromStringAndSize(s.c_str(), static_cast<Py_ssize_t>(s.size()));
#endif
}

inline
PyObject *vtkPythonArgs::BuildValue(char a)
{
  char b[2];
  b[0] = a;
  b[1] = '\0';
  return PyString_FromString(b);
}

inline
PyObject *vtkPythonArgs::BuildValue(double a)
{
  return PyFloat_FromDouble(a);
}

inline
PyObject *vtkPythonArgs::BuildValue(bool a)
{
#if PY_VERSION_HEX >= 0x02030000
  return PyBool_FromLong((long)a);
#else
  return PyInt_FromLong((long)a);
#endif
}

inline
PyObject *vtkPythonArgs::BuildValue(int a)
{
  return PyInt_FromLong(a);
}

inline
PyObject *vtkPythonArgs::BuildValue(unsigned int a)
{
#if VTK_SIZEOF_INT < VTK_SIZEOF_LONG
  return PyInt_FromLong(a);
#else
  if ((long)(a) >= 0)
    {
    return PyInt_FromLong((long)(a));
    }
  return PyLong_FromUnsignedLong(a);
#endif
}

inline
PyObject *vtkPythonArgs::BuildValue(long a)
{
  return PyInt_FromLong(a);
}

inline
PyObject *vtkPythonArgs::BuildValue(unsigned long a)
{
  if ((long)(a) >= 0)
    {
    return PyInt_FromLong((long)(a));
    }
  return PyLong_FromUnsignedLong(a);
}

#if defined(VTK_TYPE_USE_LONG_LONG)
inline
PyObject *vtkPythonArgs::BuildValue(long long a)
{
#if defined(PY_LONG_LONG)
  return PyLong_FromLongLong(a);
#else
  return PyLong_FromLong((long)(a));
#endif
}

inline
PyObject *vtkPythonArgs::BuildValue(unsigned long long a)
{
#if defined(PY_LONG_LONG)
  return PyLong_FromUnsignedLongLong(a);
#else
  return PyLong_FromUnsignedLong((unsigned long)(a));
#endif
}
#endif

#if defined(VTK_TYPE_USE___INT64)
inline
PyObject *vtkPythonArgs::BuildValue(__int64 a)
{
#if defined(PY_LONG_LONG)
  return PyLong_FromLongLong(a);
#else
  return PyLong_FromLong((long)(a));
#endif
}

inline
PyObject *vtkPythonArgs::BuildValue(unsigned __int64 a)
{
#if defined(PY_LONG_LONG)
  return PyLong_FromUnsignedLongLong(a);
#else
  return PyLong_FromUnsignedLong((unsigned long)(a));
#endif
}
#endif

inline
PyObject *vtkPythonArgs::BuildBytes(const char *a, int n)
{
  return PyString_FromStringAndSize(a, n);
}

#endif
