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

/**
 * @class   vtkPythonArgs
*/

#ifndef vtkPythonArgs_h
#define vtkPythonArgs_h

#include "vtkWrappingPythonCoreModule.h" // For export macro
#include "vtkPythonUtil.h"
#include "PyVTKObject.h"
#include "PyVTKTemplate.h"

#include "vtkConfigure.h"
#include "vtkUnicodeString.h"

#include <string>
#include <cstring>

class VTKWRAPPINGPYTHONCORE_EXPORT vtkPythonArgs
{
public:

  //@{
  /**
   * Constructor for parsing args of a vtkObjectBase object.
   */
  vtkPythonArgs(PyObject *self, PyObject *args, const char *methodname) :
      Args(args), MethodName(methodname) {
      this->N = PyTuple_GET_SIZE(args);
      this->M = PyType_Check(self);
      this->I = this->M;
  }
  //@}

  //@{
  /**
   * Constructor for parsing method args.
   */
  vtkPythonArgs(PyObject *args, const char *methodname) :
    Args(args), MethodName(methodname) {
      this->N = PyTuple_GET_SIZE(args);
      this->M = 0;
      this->I = 0;
  }
  //@}

  /**
   * Reset in order to re-parse the args.
   */
  void Reset() { this->I = this->M; }

  /**
   * Get a pointer to the self object, converted to its C++ type.
   * Returns nullptr and sets a TypeError if the type is wrong.
   * If "self" is a class type, pull the object from the first arg.
   */
  static vtkObjectBase *GetSelfPointer(PyObject *self, PyObject *args);

  /**
   * Get a pointer to the self object, converted to its C++ type.
   * Returns nullptr and sets a TypeError if the type is wrong.
   * If "self" is a type, pull the object from the first arg.
   */
  static void *GetSelfSpecialPointer(PyObject *self, PyObject *args);

  /**
   * Get a pointer to the self object, converted to its C++ type.
   * Always succeeds.
   */
  static void *GetSelfSpecialPointer(PyObject *self);

  /**
   * Verify the arg count for a method with optional arguments.
   */
  bool CheckArgCount(Py_ssize_t nmin, Py_ssize_t nmax);

  /**
   * Verify the arg count.  Sets a python exception on failure.
   */
  bool CheckArgCount(Py_ssize_t n);

  /**
   * Verify preconditions.  Sets a python exception on failure.
   */
  bool CheckPrecond(bool c, const char *text);

  /**
   * Returns true if self is an object, false if self is a class.
   */
  bool IsBound() { return (this->M == 0); }

  /**
   * Raise an exception if method call is not bound.
   */
  bool IsPureVirtual();

  /**
   * Check if an error has occurred.
   */
  static bool ErrorOccurred();

  /**
   * Check if there are any args left.
   */
  bool NoArgsLeft() { return (this->I >= this->N); }

  /**
   * Get the size of an arg, if it is a string.
   * The returned size does not include the null byte.
   * If the arg is out of range, or is not a string,
   * then it returns 0 but doesn't set error.
   */
  size_t GetStringSize(int i);

  /**
   * Get the size of an arg, if it is a sequence.
   * If no size is available, or if the arg is out of range,
   * then it returns 0 but doesn't set error.
   */
  size_t GetArgSize(int i);

  /**
   * If arg i exists, and if m is not equal to the expected value n,
   * then set an error for arg i and return false.  In all other
   * cases, return true.
   */
  bool CheckSizeHint(int i, size_t m, size_t n);

  //@{
  /**
   * Get the next argument as a naked Python object.
   */
  bool GetPythonObject(PyObject *&v) {
    bool b;
    v = this->GetArgAsPythonObject(b);
    return b; }
  bool GetPythonObject(PyObject *o, PyObject *&v) {
    bool b;
    v = vtkPythonArgs::GetArgAsPythonObject(o, b);
    return b; }
  //@}

  //@{
  /**
   * Get the next argument as a vtkObjectBase derived type.
   * It uses a C-style cast instead of a static_cast, which
   * means that it works on incomplete types, and also means
   * that it will give undefined results if "T" uses multiple
   * inheritance.
   */
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
  //@}

  //@{
  /**
   * Get the next argument as a special object.  If a constructor
   * was needed to convert the arg, the constructed object will be
   * returned in "o" and must be freed after "v" is used.
   */
  template<class T>
  bool GetSpecialObject(T *&v, PyObject *&o, const char *classname) {
    v = static_cast<T *>(this->GetArgAsSpecialObject(classname, &o));
    return (v != nullptr); }
  template<class T>
  static bool GetSpecialObject(
    PyObject *arg, T *&v, PyObject *&o, const char *classname) {
    v = static_cast<T *>(
      vtkPythonArgs::GetArgAsSpecialObject(arg, classname, &o));
    return (v != nullptr); }
  //@}

  //@{
  /**
   * Get the next argument as a special object.  Use this if the
   * arg is a non-const ref, as it will disallow conversion.
   */
  template<class T>
  bool GetSpecialObject(T *&v, const char *classname) {
    v = static_cast<T *>(this->GetArgAsSpecialObject(classname, nullptr));
    return (v != nullptr); }
  template<class T>
  static bool GetSpecialObject(PyObject *o, T *&v, const char *classname) {
    v = static_cast<T *>(
      vtkPythonArgs::GetArgAsSpecialObject(o, classname, nullptr));
    return (v != nullptr); }
  //@}

  //@{
  /**
   * Get the next argument as an enum value.
   */
  template<class T>
  bool GetEnumValue(T &v, const char *enumname) {
    bool r;
    v = static_cast<T>(this->GetArgAsEnum(enumname, r));
    return r; }
  template<class T>
  static bool GetEnumValue(PyObject *o, T &v, const char *enumname) {
    bool r;
    v = static_cast<T>(vtkPythonArgs::GetArgAsEnum(o, enumname, r));
    return r; }
  //@}

  //@{
  /**
   * Get the arguments needed for a SetExecuteMethod or a similar
   * method that requires a function-pointer argument.
   */
  bool GetFunction(PyObject *&o);
  static bool GetFunction(PyObject *arg, PyObject *&o);
  //@}

  // Get the next arg as a pointer to a buffer.
  bool GetBuffer(void *&v, Py_buffer *buf);
  static bool GetBuffer(PyObject *o, void *&v, Py_buffer *buf);
  bool GetBuffer(const void *&v, Py_buffer *buf);
  static bool GetBuffer(PyObject *o, const void *&v, Py_buffer *buf);
  bool GetBuffer(float *&v, Py_buffer *buf);
  static bool GetBuffer(PyObject *o, float *&v, Py_buffer *buf);
  bool GetBuffer(const float *&v, Py_buffer *buf);
  static bool GetBuffer(PyObject *o, const float *&v, Py_buffer *buf);
  bool GetBuffer(double *&v, Py_buffer *buf);
  static bool GetBuffer(PyObject *o, double *&v, Py_buffer *buf);
  bool GetBuffer(const double *&v, Py_buffer *buf);
  static bool GetBuffer(PyObject *o, const double *&v, Py_buffer *buf);
  bool GetBuffer(bool *&v, Py_buffer *buf);
  static bool GetBuffer(PyObject *o, bool *&v, Py_buffer *buf);
  bool GetBuffer(const bool *&v, Py_buffer *buf);
  static bool GetBuffer(PyObject *o, const bool *&v, Py_buffer *buf);
  bool GetBuffer(char *&v, Py_buffer *buf);
  static bool GetBuffer(PyObject *o, char *&v, Py_buffer *buf);
  bool GetBuffer(const char *&v, Py_buffer *buf);
  static bool GetBuffer(PyObject *o, const char *&v, Py_buffer *buf);
  bool GetBuffer(signed char *&v, Py_buffer *buf);
  static bool GetBuffer(PyObject *o, signed char *&v, Py_buffer *buf);
  bool GetBuffer(const signed char *&v, Py_buffer *buf);
  static bool GetBuffer(PyObject *o, const signed char *&v, Py_buffer *buf);
  bool GetBuffer(unsigned char *&v, Py_buffer *buf);
  static bool GetBuffer(PyObject *o, unsigned char *&v, Py_buffer *buf);
  bool GetBuffer(const unsigned char *&v, Py_buffer *buf);
  static bool GetBuffer(PyObject *o, const unsigned char *&v, Py_buffer *buf);
  bool GetBuffer(short *&v, Py_buffer *buf);
  static bool GetBuffer(PyObject *o, short *&v, Py_buffer *buf);
  bool GetBuffer(const short *&v, Py_buffer *buf);
  static bool GetBuffer(PyObject *o, const short *&v, Py_buffer *buf);
  bool GetBuffer(unsigned short *&v, Py_buffer *buf);
  static bool GetBuffer(PyObject *o, unsigned short *&v, Py_buffer *buf);
  bool GetBuffer(const unsigned short *&v, Py_buffer *buf);
  static bool GetBuffer(PyObject *o, const unsigned short *&v, Py_buffer *buf);
  bool GetBuffer(int *&v, Py_buffer *buf);
  static bool GetBuffer(PyObject *o, int *&v, Py_buffer *buf);
  bool GetBuffer(const int *&v, Py_buffer *buf);
  static bool GetBuffer(PyObject *o, const int *&v, Py_buffer *buf);
  bool GetBuffer(unsigned int *&v, Py_buffer *buf);
  static bool GetBuffer(PyObject *o, unsigned int *&v, Py_buffer *buf);
  bool GetBuffer(const unsigned int *&v, Py_buffer *buf);
  static bool GetBuffer(PyObject *o, const unsigned int *&v, Py_buffer *buf);
  bool GetBuffer(long *&v, Py_buffer *buf);
  static bool GetBuffer(PyObject *o, long *&v, Py_buffer *buf);
  bool GetBuffer(const long *&v, Py_buffer *buf);
  static bool GetBuffer(PyObject *o, const long *&v, Py_buffer *buf);
  bool GetBuffer(unsigned long *&v, Py_buffer *buf);
  static bool GetBuffer(PyObject *o, unsigned long *&v, Py_buffer *buf);
  bool GetBuffer(const unsigned long *&v, Py_buffer *buf);
  static bool GetBuffer(PyObject *o, const unsigned long *&v, Py_buffer *buf);
  bool GetBuffer(long long *&v, Py_buffer *buf);
  static bool GetBuffer(PyObject *o, long long *&v, Py_buffer *buf);
  bool GetBuffer(const long long *&v, Py_buffer *buf);
  static bool GetBuffer(PyObject *o, const long long *&v, Py_buffer *buf);
  bool GetBuffer(unsigned long long *&v, Py_buffer *buf);
  static bool GetBuffer(PyObject *o, unsigned long long *&v, Py_buffer *buf);
  bool GetBuffer(const unsigned long long *&v, Py_buffer *buf);
  static bool GetBuffer(PyObject *o, const unsigned long long *&v,
                        Py_buffer *buf);


  //@{
  /**
   * Get the next argument as a string.
   */
  bool GetValue(const char *&v);
  static bool GetValue(PyObject *o, const char *&v);
  bool GetValue(std::string &v);
  static bool GetValue(PyObject *o, std::string &v);
  bool GetValue(vtkUnicodeString &v);
  static bool GetValue(PyObject *o, vtkUnicodeString &v);
  //@}

  //@{
  /**
   * Get the next string arg as a character.
   */
  bool GetValue(char &v);
  static bool GetValue(PyObject *o, char &v);
  //@}

  //@{
  /**
   * Get the next argument.  Sets a TypeError on failure.
   */
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
  bool GetValue(long long &v);
  static bool GetValue(PyObject *o, long long &v);
  bool GetValue(unsigned long long &v);
  static bool GetValue(PyObject *o, unsigned long long &v);
  //@}

  //@{
  /**
   * Get the next argument as an array.
   */
  bool GetArray(float *v, size_t n);
  bool GetArray(double *v, size_t n);
  bool GetArray(bool *v, size_t n);
  bool GetArray(char *v, size_t n);
  bool GetArray(signed char *v, size_t n);
  bool GetArray(unsigned char *v, size_t n);
  bool GetArray(short *v, size_t n);
  bool GetArray(unsigned short *v, size_t n);
  bool GetArray(int *v, size_t n);
  bool GetArray(unsigned int *v, size_t n);
  bool GetArray(long *v, size_t n);
  bool GetArray(unsigned long *v, size_t n);
  bool GetArray(long long *v, size_t n);
  bool GetArray(unsigned long long *v, size_t n);
  bool GetArray(std::string *v, size_t n);
  bool GetArray(vtkUnicodeString *v, size_t n);
  //@}

  //@{
  /**
   * Get the next argument as a multi-dimensional array.
   */
  bool GetNArray(float *v, int ndims, const size_t *dims);
  bool GetNArray(double *v, int ndims, const size_t *dims);
  bool GetNArray(bool *v, int ndims, const size_t *dims);
  bool GetNArray(char *v, int ndims, const size_t *dims);
  bool GetNArray(signed char *v, int ndims, const size_t *dims);
  bool GetNArray(unsigned char *v, int ndims, const size_t *dims);
  bool GetNArray(short *v, int ndims, const size_t *dims);
  bool GetNArray(unsigned short *v, int ndims, const size_t *dims);
  bool GetNArray(int *v, int ndims, const size_t *dims);
  bool GetNArray(unsigned int *v, int ndims, const size_t *dims);
  bool GetNArray(long *v, int ndims, const size_t *dims);
  bool GetNArray(unsigned long *v, int ndims, const size_t *dims);
  bool GetNArray(long long *v, int ndims, const size_t *dims);
  bool GetNArray(unsigned long long *v, int ndims, const size_t *dims);
  //@}

  //@{
  /**
   * Set the value of an argument that was passed by reference.
   */
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
  bool SetArgValue(int i, long long v);
  bool SetArgValue(int i, unsigned long long v);
  bool SetArgValue(int i, const float *v, size_t n);
  bool SetArgValue(int i, const double *v, size_t n);
  bool SetArgValue(int i, const bool *v, size_t n);
  bool SetArgValue(int i, const signed char *v, size_t n);
  bool SetArgValue(int i, const unsigned char *v, size_t n);
  bool SetArgValue(int i, const short *v, size_t n);
  bool SetArgValue(int i, const unsigned short *v, size_t n);
  bool SetArgValue(int i, const int *v, size_t n);
  bool SetArgValue(int i, const unsigned int *v, size_t n);
  bool SetArgValue(int i, const long *v, size_t n);
  bool SetArgValue(int i, const unsigned long *v, size_t n);
  bool SetArgValue(int i, const long long *v, size_t n);
  bool SetArgValue(int i, const unsigned long long *v, size_t n);
  //@}

  //@{
  /**
   * Set the values in an array argument.
   */
  bool SetArray(int i, const float *v, size_t n);
  bool SetArray(int i, const double *v, size_t n);
  bool SetArray(int i, const bool *v, size_t n);
  bool SetArray(int i, const char *v, size_t n);
  bool SetArray(int i, const signed char *v, size_t n);
  bool SetArray(int i, const unsigned char *v, size_t n);
  bool SetArray(int i, const short *v, size_t n);
  bool SetArray(int i, const unsigned short *v, size_t n);
  bool SetArray(int i, const int *v, size_t n);
  bool SetArray(int i, const unsigned int *v, size_t n);
  bool SetArray(int i, const long *v, size_t n);
  bool SetArray(int i, const unsigned long *v, size_t n);
  bool SetArray(int i, const long long *v, size_t n);
  bool SetArray(int i, const unsigned long long *v, size_t n);
  //@}

  //@{
  /**
   * Set the values in a multi-dimensional array argument.
   */
  bool SetNArray(int i, const float *v, int n, const size_t *d);
  bool SetNArray(int i, const double *v, int n, const size_t *d);
  bool SetNArray(int i, const bool *v, int n, const size_t *d);
  bool SetNArray(int i, const char *v, int n, const size_t *d);
  bool SetNArray(int i, const signed char *v, int n, const size_t *d);
  bool SetNArray(int i, const unsigned char *v, int n, const size_t *d);
  bool SetNArray(int i, const short *v, int n, const size_t *d);
  bool SetNArray(int i, const unsigned short *v, int n, const size_t *d);
  bool SetNArray(int i, const int *v, int n, const size_t *d);
  bool SetNArray(int i, const unsigned int *v, int n, const size_t *d);
  bool SetNArray(int i, const long *v, int n, const size_t *d);
  bool SetNArray(int i, const unsigned long *v, int n, const size_t *d);
  bool SetNArray(int i, const long long *v, int n, const size_t *d);
  bool SetNArray(int i, const unsigned long long *v, int n, const size_t *d);
  //@}

  /**
   * Set the contents of the specified argument from a sequence,
   * the same as doing "arg[:] = seq" in Python.
   */
  bool SetContents(int i, PyObject *seq);

  /**
   * Build a value of None.
   */
  static PyObject *BuildNone();

  /**
   * Build a vtkObjectBase object, use GetClassName() to get its type.
   * If a null pointer is given, then None will be returned.
   */
  static PyObject *BuildVTKObject(const void *v);

  /**
   * Build a non-vtkObjectBase object of the specified type.
   */
  static PyObject *BuildSpecialObject(const void *v, const char *classname);

  /**
   * Build an enum value object of the specified type.
   */
  static PyObject *BuildEnumValue(int v, const char *enumname);

  /**
   * Create a mangled string containing a memory address.
   */
  static PyObject *BuildValue(const void *v);

  //@{
  /**
   * Build a string return value.
   */
  static PyObject *BuildValue(const char *v, size_t l);
  static PyObject *BuildValue(const char *v);
  static PyObject *BuildValue(const std::string &v);
  static PyObject *BuildValue(const vtkUnicodeString &v);
  //@}

  /**
   * Build a char return value.
   */
  static PyObject *BuildValue(char v);

  //@{
  /**
   * Build a numeric return value.
   */
  static PyObject *BuildValue(double v);
  static PyObject *BuildValue(bool v);
  static PyObject *BuildValue(int v);
  static PyObject *BuildValue(unsigned int v);
  static PyObject *BuildValue(long v);
  static PyObject *BuildValue(unsigned long v);
  static PyObject *BuildValue(long long v);
  static PyObject *BuildValue(unsigned long long v);
  //@}

  /**
   * Build a bytes object (or string).
   */
  static PyObject *BuildBytes(const char *v, size_t n);

  //@{
  /**
   * Build a tuple for a return value.
   */
  static PyObject *BuildTuple(const float *v, size_t n);
  static PyObject *BuildTuple(const double *v, size_t n);
  static PyObject *BuildTuple(const bool *v, size_t n);
  static PyObject *BuildTuple(const signed char *v, size_t n);
  static PyObject *BuildTuple(const unsigned char *v, size_t n);
  static PyObject *BuildTuple(const short *v, size_t n);
  static PyObject *BuildTuple(const unsigned short *v, size_t n);
  static PyObject *BuildTuple(const int *v, size_t n);
  static PyObject *BuildTuple(const unsigned int *v, size_t n);
  static PyObject *BuildTuple(const long *v, size_t n);
  static PyObject *BuildTuple(const unsigned long *v, size_t n);
  static PyObject *BuildTuple(const long long *v, size_t n);
  static PyObject *BuildTuple(const unsigned long long *v, size_t n);
  static PyObject *BuildTuple(const std::string *v, size_t n);
  static PyObject *BuildTuple(const vtkUnicodeString *v, size_t n);
  //@}

  /**
   * Copy an array.
   */
  template<class T>
  static void Save(const T *a, T *b, size_t n) {
    memcpy(b, a, n*sizeof(T)); }

  /**
   * Check if an array has changed.
   */
  template<class T>
  static bool HasChanged(const T *a, const T *b, size_t n) {
    return (memcmp(a, b, n*sizeof(T)) != 0); }

  /**
   * Get the argument count.
   */
  static int GetArgCount(PyObject *args) {
    return static_cast<int>(PyTuple_GET_SIZE(args)); }

  /**
   * Get the argument count for a method that might be unbound.
   */
  static int GetArgCount(PyObject *self, PyObject *args) {
    return (static_cast<int>(PyTuple_GET_SIZE(args)) -
            PyType_Check(self)); }

  /**
   * Raise a type error just saying that the arg count is wrong.
   */
  static bool ArgCountError(Py_ssize_t n, const char *name);

  /**
   * Raise an error that says that a precondition failed.
   */
  static bool PrecondError(const char *name);

  /**
   * A simple RAII array class that stores small arrays on the stack.
   */
  template<class T>
  class Array
  {
  public:
    Array(size_t n);

    ~Array() { if (Pointer != Storage) { delete [] Pointer; } }

    T *Data() { return Pointer; }

  private:
    static const size_t basicsize = 6;
    T *Pointer;
    T Storage[basicsize];
  };

protected:

  /**
   * Get the "self" object from the first argument.
   */
  static PyObject *GetSelfFromFirstArg(PyObject *self, PyObject *args);

  //@{
  /**
   * Get the next argument as an object of the given type.
   */
  PyObject *GetArgAsPythonObject(bool &valid);
  static PyObject *GetArgAsPythonObject(
    PyObject *o, bool &valid);
  //@}

  //@{
  /**
   * Get the next argument as an object of the given type.
   */
  vtkObjectBase *GetArgAsVTKObject(const char *classname, bool &valid);
  static vtkObjectBase *GetArgAsVTKObject(
    PyObject *o, const char *classname, bool &valid);
  //@}

  //@{
  /**
   * Get the next argument as an object of the given type.
   */
  void *GetArgAsSpecialObject(const char *classname, PyObject **newobj);
  static void *GetArgAsSpecialObject(
    PyObject *o, const char *classname, PyObject **newobj);
  //@}

  //@{
  /**
   * Get the next argument as an object of the given type.
   */
  int GetArgAsEnum(const char *enumname, bool &valid);
  static int GetArgAsEnum(
    PyObject *o, const char *enumname, bool &valid);
  //@}

  /**
   * Raise a TypeError if a virtual method call was called.
   */
  bool PureVirtualError();

  /**
   * Raise an TypeError stating that the arg count is incorrect.
   */
  bool ArgCountError(Py_ssize_t m, Py_ssize_t n);

  /**
   * Prefix a TypeError that has occurred with the arg number.
   */
  bool RefineArgTypeError(Py_ssize_t i);

private:

  PyObject *Args;
  const char *MethodName;

  Py_ssize_t N; // size of args tuple
  int M; // 1 if Self is a PyVTKClass and first arg is the PyVTKObject
  Py_ssize_t I; // the arg counter, starts at M
};

//--------------------------------------------------------------------
// Inline methods for getting "self" as its original type

// Get "self" from a PyVTKObject, which contains a vtkObjectBase object.
inline
vtkObjectBase *vtkPythonArgs::GetSelfPointer(PyObject *self, PyObject *args)
{
  if (PyType_Check(self))
  {
    self = vtkPythonArgs::GetSelfFromFirstArg(self, args);
  }
  return (self ? ((PyVTKObject *)self)->vtk_ptr : nullptr);
}

// Get "self" from a PyVTKSpecialObject.
inline
void *vtkPythonArgs::GetSelfSpecialPointer(PyObject *self, PyObject *args)
{
  if (PyType_Check(self))
  {
    self = vtkPythonArgs::GetSelfFromFirstArg(self, args);
  }
  return (self ? ((PyVTKSpecialObject *)self)->vtk_ptr : nullptr);
}

// Get "self" from a PyVTKSpecialObject (for methods with no args).
inline
void *vtkPythonArgs::GetSelfSpecialPointer(PyObject *self)
{
  return ((PyVTKSpecialObject *)self)->vtk_ptr;
}

//--------------------------------------------------------------------
// Inline methods for checking the arg count

// Verify the arg count for a method with optional arguments.
inline
bool vtkPythonArgs::CheckArgCount(Py_ssize_t nmin, Py_ssize_t nmax)
{
  Py_ssize_t nargs = this->N - this->M;
  if (nargs >= nmin && nargs <= nmax)
  {
    return true;
  }
  this->ArgCountError(nmin, nmax);
  return false;
}

// Verify the arg count for a method with optional arguments.
inline
bool vtkPythonArgs::CheckArgCount(Py_ssize_t n)
{
  Py_ssize_t nargs = this->N - this->M;
  if (nargs == n)
  {
    return true;
  }
  this->ArgCountError(n, n);
  return false;
}

//--------------------------------------------------------------------
// Inline method for checking generic preconditions.

inline
bool vtkPythonArgs::CheckPrecond(bool c, const char *text)
{
  if (!c)
  {
    this->PrecondError(text);
  }
  return c;
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
  return (PyErr_Occurred() != nullptr);
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
  return nullptr;
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
PyObject *vtkPythonArgs::BuildValue(const char *a, size_t l)
{
#if PY_VERSION_HEX < 0x03000000
  return PyString_FromStringAndSize(a, static_cast<Py_ssize_t>(l));
#else
#if PY_VERSION_HEX >= 0x03030000
  PyObject *o = PyUnicode_FromStringAndSize(a, static_cast<Py_ssize_t>(l));
#else
  PyObject *o = PyUnicode_Decode(a, static_cast<Py_ssize_t>(l),
                                 nullptr, nullptr);
#endif
  if (o == nullptr)
  {
    PyErr_Clear();
    o = PyBytes_FromStringAndSize(a, static_cast<Py_ssize_t>(l));
  }
  return o;
#endif
}

inline
PyObject *vtkPythonArgs::BuildValue(const char *a)
{
  if (a)
  {
    return vtkPythonArgs::BuildValue(a, strlen(a));
  }
  Py_INCREF(Py_None);
  return Py_None;
}

inline
PyObject *vtkPythonArgs::BuildValue(const std::string &a)
{
  return vtkPythonArgs::BuildValue(a.data(), a.size());
}

inline
PyObject *vtkPythonArgs::BuildValue(const vtkUnicodeString &a)
{
  std::string s;
  a.utf8_str(s);
#ifdef Py_USING_UNICODE
  return PyUnicode_DecodeUTF8(s.c_str(), static_cast<Py_ssize_t>(s.size()), nullptr);
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
  return PyBool_FromLong((long)a);
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
  if (static_cast<long>(a) >= 0)
  {
    return PyInt_FromLong(static_cast<long>(a));
  }
  return PyLong_FromUnsignedLong(a);
}

inline
PyObject *vtkPythonArgs::BuildValue(long long a)
{
  return PyLong_FromLongLong(a);
}

inline
PyObject *vtkPythonArgs::BuildValue(unsigned long long a)
{
  return PyLong_FromUnsignedLongLong(a);
}

inline
PyObject *vtkPythonArgs::BuildBytes(const char *a, size_t n)
{
  return PyBytes_FromStringAndSize(a, n);
}

// List of all types for the Array class template:

#define vtkPythonArgsTemplateMacro(decl) \
  decl<bool>; \
  decl<float>; \
  decl<double>; \
  decl<char>; \
  decl<signed char>; \
  decl<unsigned char>; \
  decl<short>; \
  decl<unsigned short>; \
  decl<int>; \
  decl<unsigned int>; \
  decl<long>; \
  decl<unsigned long>; \
  decl<long long>; \
  decl<unsigned long long>;

// Forward declare the Array class template over all types:
#if defined(VTK_USE_EXTERN_TEMPLATE) && !defined(vtkPythonArgs_cxx)
vtkPythonArgsTemplateMacro(
  extern template class VTKWRAPPINGPYTHONCORE_EXPORT vtkPythonArgs::Array
)
#endif

#endif
