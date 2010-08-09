/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPythonUtil.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPythonUtil.h"

#include "vtkSystemIncludes.h"

#include "vtkObject.h"
#include "vtkSmartPointerBase.h"
#include "vtkVariant.h"
#include "vtkWindows.h"
#include "vtkToolkits.h"

#include <vtksys/ios/sstream>
#include <vtkstd/map>
#include <vtkstd/string>
#include <vtkstd/utility>

#ifdef VTK_WRAP_PYTHON_SIP
#include "sip.h"
#endif

//--------------------------------------------------------------------
// There are three maps associated with the Python wrappers

class vtkPythonObjectMap
  : public vtkstd::map<vtkSmartPointerBase, PyObject*>
{
};

class vtkPythonClassMap
  : public vtkstd::map<vtkstd::string, PyObject*>
{
};

class vtkPythonSpecialTypeMap
  : public vtkstd::map<vtkstd::string, PyVTKSpecialType>
{
};

//--------------------------------------------------------------------
// The singleton for vtkPythonUtil

vtkPythonUtil *vtkPythonMap = NULL;

// destructs the singleton when python exits
void vtkPythonUtilDelete()
{
  delete vtkPythonMap;
  vtkPythonMap = NULL;
}

//--------------------------------------------------------------------
vtkPythonUtil::vtkPythonUtil()
{
  this->ObjectMap = new vtkPythonObjectMap;
  this->ClassMap = new vtkPythonClassMap;
  this->SpecialTypeMap = new vtkPythonSpecialTypeMap;
}

//--------------------------------------------------------------------
vtkPythonUtil::~vtkPythonUtil()
{
  delete this->ObjectMap;
  delete this->ClassMap;
  delete this->SpecialTypeMap;
}

//--------------------------------------------------------------------
// Concatenate an array of strings into a single string.  The resulting
// string is allocated via new.  The array of strings must be null-terminated,
// e.g. static char *strings[] = {"string1", "string2", NULL};
PyObject *vtkPythonUtil::BuildDocString(const char *docstring[])
{
  PyObject *result;
  char *data;
  size_t i, j, n;
  size_t *m;
  size_t total = 0;

  for (n = 0; docstring[n] != NULL; n++)
    {
    ;
    }

  m = new size_t[n];

  for (i = 0; i < n; i++)
    {
    m[i] = strlen(docstring[i]);
    total += m[i];
    }

  result = PyString_FromStringAndSize((char *)docstring[0], (Py_ssize_t)m[0]);

  if (n > 1)
    {
    _PyString_Resize(&result, (Py_ssize_t)total);
    }

  data = PyString_AsString(result);

  j = m[0];
  for (i = 1; i < n; i++)
    {
    strcpy(&data[j], docstring[i]);
    j += m[i];
    }

  delete [] m;

  return result;
}

//--------------------------------------------------------------------
// Enums for vtkPythonUtil::CheckArg, the values between VTK_PYTHON_GOOD_MATCH
// and VTK_PYTHON_NEEDS_CONVERSION are reserved for checking how
// many generations a vtkObject arg is from the requested arg type.

enum vtkPythonArgPenalties
{
  VTK_PYTHON_EXACT_MATCH = 0,
  VTK_PYTHON_GOOD_MATCH = 1,
  VTK_PYTHON_NEEDS_CONVERSION = 65534,
  VTK_PYTHON_INCOMPATIBLE = 65535
};

//--------------------------------------------------------------------
// A helper struct for CallOverloadedMethod
class vtkPythonOverloadHelper
{
public:
  vtkPythonOverloadHelper() : m_format(0), m_classname(0), m_penalty(0) {};
  void initialize(bool selfIsClass, const char *format);
  bool next(const char **format, const char **classname);
  int penalty() { return m_penalty; };
  int penalty(int p) {
    if (p > m_penalty) { m_penalty = p; };
    return m_penalty; };

private:
  const char *m_format;
  const char *m_classname;
  int m_penalty;
  PyCFunction m_meth;
};

// Construct the object with a penalty of VTK_PYTHON_EXACT_MATCH
void vtkPythonOverloadHelper::initialize(bool selfIsClass, const char *format)
{
  // remove the "explicit" marker for constructors
  if (*format == '-')
    {
    format++;
    }

  // remove the first arg check if "self" is not a PyVTKClass
  if (*format == '@' && !selfIsClass)
    {
    format++;
    }

  m_format = format;
  m_classname = format;
  while (*m_classname != '\0' && *m_classname != ' ')
    {
    m_classname++;
    }
  if (*m_classname == ' ')
    {
    m_classname++;
    }

  this->m_penalty = VTK_PYTHON_EXACT_MATCH;
}

// Get the next format char and, if char is 'O', the classname.
// The classname is terminated with space, not with null
bool vtkPythonOverloadHelper::next(const char **format, const char **classname)
{
  if (*m_format == '\0' || *m_format == ' ')
    {
    return false;
    }

  *format = m_format;

  if (*m_format == 'O')
    {
    *classname = m_classname;

    while (*m_classname != '\0' && *m_classname != ' ')
      {
      m_classname++;
      }
    if (*m_classname == ' ')
      {
      m_classname++;
      }
    }

  m_format++;
  if (!isalpha(*m_format) && *m_format != '(' && *m_format != ')' &&
      *m_format != '\0' && *m_format != ' ')
    {
    m_format++;
    }

  return true;
}

//--------------------------------------------------------------------
// If tmpi > VTK_INT_MAX, then penalize unless format == 'l'

#if VTK_SIZEOF_LONG != VTK_SIZEOF_INT
#ifdef PY_LONG_LONG
int vtkPythonIntPenalty(PY_LONG_LONG tmpi, int penalty, char format)
#else
int vtkPythonIntPenalty(long tmpi, int penalty, char format)
#endif
{
  if (tmpi > VTK_INT_MAX || tmpi < VTK_INT_MIN)
    {
    if (format != 'l')
      {
      if (penalty < VTK_PYTHON_GOOD_MATCH)
        {
        penalty = VTK_PYTHON_GOOD_MATCH;
        if (format != 'i')
          {
          penalty++;
          }
        }
      else
        {
        penalty++;
        }
      }
    }
  else
    {
    if (format != 'i')
      {
      if (penalty < VTK_PYTHON_GOOD_MATCH)
        {
        penalty = VTK_PYTHON_GOOD_MATCH;
        }
      else
        {
        penalty++;
        }
      }
    }
  return penalty;
}

#else
#ifdef PY_LONG_LONG
int vtkPythonIntPenalty(PY_LONG_LONG, int penalty, char)
#else
int vtkPythonIntPenalty(long, int penalty, char)
#endif
{
  return penalty;
}
#endif

//--------------------------------------------------------------------
// This must check the same format chars that are used by
// vtkWrapPython_FormatString() in vtkWrapPython.c
//
// The "level" parameter limits possible recursion of this method,
// it is incremented every time recursion occurs.

int vtkPythonUtil::CheckArg(
  PyObject *arg, const char *format, const char *classname, int level)
{
  int penalty = VTK_PYTHON_EXACT_MATCH;

  switch (*format)
    {
    case 'b':
    case 'h':
    case 'l':
    case 'i':
#if PY_VERSION_HEX >= 0x02030000
      if (PyBool_Check(arg))
        {
        penalty = VTK_PYTHON_GOOD_MATCH;
        if (*format != 'i')
          {
          penalty++;
          }
        }
      else
#endif
      if (PyInt_Check(arg))
        {
#if VTK_SIZEOF_LONG == VTK_SIZEOF_INT
        if (*format != 'i')
          {
          penalty = VTK_PYTHON_GOOD_MATCH;
          }
#else
        penalty = vtkPythonIntPenalty(PyInt_AsLong(arg), penalty, *format);
#endif
        }
      else if (PyLong_Check(arg))
        {
        penalty = VTK_PYTHON_GOOD_MATCH;
#if VTK_SIZEOF_LONG == VTK_SIZEOF_INT
        if (*format != 'i')
          {
          penalty++;
          }
#else
# ifdef PY_LONG_LONG
        PY_LONG_LONG tmpi = PyLong_AsLongLong(arg);
# else
        long tmpi = PyLong_AsLong(arg);
# endif
        if (PyErr_Occurred())
          {
          PyErr_Clear();
          tmpi = VTK_LONG_MAX;
          }

        penalty = vtkPythonIntPenalty(tmpi, penalty, *format);
#endif
        }
      else // not PyInt or PyLong
        {
        if (level == 0)
          {
          penalty = VTK_PYTHON_NEEDS_CONVERSION;
          long tmpi = PyInt_AsLong(arg);
          if (tmpi == -1 || PyErr_Occurred())
            {
            PyErr_Clear();
            penalty = VTK_PYTHON_INCOMPATIBLE;
            }
          }
        else
          {
          penalty = VTK_PYTHON_INCOMPATIBLE;
          }
        }
      break;

#ifdef PY_LONG_LONG
    case 'L':
      if (!PyLong_Check(arg))
        {
        penalty = VTK_PYTHON_GOOD_MATCH;
        if (!PyInt_Check(arg))
          {
          if (level == 0)
            {
            penalty = VTK_PYTHON_NEEDS_CONVERSION;
            PyLong_AsLongLong(arg);
            if (PyErr_Occurred())
              {
              PyErr_Clear();
              penalty = VTK_PYTHON_INCOMPATIBLE;
              }
            }
          else
            {
            penalty = VTK_PYTHON_INCOMPATIBLE;
            }
          }
        }
      break;
#endif

    case 'f':
      penalty = VTK_PYTHON_GOOD_MATCH;
    case 'd':
      if (!PyFloat_Check(arg))
        {
        penalty = VTK_PYTHON_NEEDS_CONVERSION;
        if (level == 0)
          {
          PyFloat_AsDouble(arg);
          if (PyErr_Occurred())
            {
            PyErr_Clear();
            penalty = VTK_PYTHON_INCOMPATIBLE;
            }
          }
        else
          {
          penalty = VTK_PYTHON_INCOMPATIBLE;
          }
        }
      break;

    case 'c':
      // penalize chars, they must be converted from strings
      penalty = VTK_PYTHON_NEEDS_CONVERSION;
      if (!PyString_Check(arg) || PyString_Size(arg) != 1)
        {
        penalty = VTK_PYTHON_INCOMPATIBLE;
        }
      break;

    case 's':
    case 'z':
      if (format[1] == '#') // memory buffer
        {
        penalty = VTK_PYTHON_GOOD_MATCH;
        if (arg == Py_None)
          {
          penalty = VTK_PYTHON_NEEDS_CONVERSION;
          if (format[0] == 's')
            {
            penalty = VTK_PYTHON_INCOMPATIBLE;
            }
          }
        // make sure that arg can act as a buffer
        else if (arg->ob_type->tp_as_buffer == 0)
          {
          penalty = VTK_PYTHON_INCOMPATIBLE;
          }
        }
      else if (arg == Py_None)
        {
        penalty = VTK_PYTHON_NEEDS_CONVERSION;
        if (format[0] == 's')
          {
          penalty = VTK_PYTHON_INCOMPATIBLE;
          }
        }
#ifdef Py_USING_UNICODE
      else if (PyUnicode_Check(arg))
        {
        penalty = VTK_PYTHON_NEEDS_CONVERSION;
        }
#endif
      else if (!PyString_Check(arg))
        {
        penalty = VTK_PYTHON_INCOMPATIBLE;
        }
      break;

    case '@':
      // '@' is a placeholder that always succeeds
      break;

    case 'O':
      {
      // classname is terminated by a space, not a null
      const char *cp = classname;
      char name[128];
      int i = 0;
      for (; i < 127 && cp[i] != ' ' && cp[i] != '\0'; i++)
        {
        name[i] = cp[i];
        }
      name[i] = '\0';
      classname = name;

      // booleans
      if (name[0] == 'b' && strcmp(classname, "bool") == 0)
        {
#if PY_VERSION_HEX >= 0x02030000
        if (!PyBool_Check(arg))
#endif
          {
          penalty = VTK_PYTHON_NEEDS_CONVERSION;
          int tmpi = PyObject_IsTrue(arg);
          if (tmpi == -1 || PyErr_Occurred())
            {
            PyErr_Clear();
            penalty = VTK_PYTHON_INCOMPATIBLE;
            }
          }
        }

      // unicode string
#ifdef Py_USING_UNICODE
      else if (name[0] == 'u' && strcmp(classname, "unicode") == 0)
        {
        if (!PyUnicode_Check(arg))
          {
          penalty = VTK_PYTHON_INCOMPATIBLE;
          }
        }
#endif

      // callback functions
      else if (name[0] == 'f' && strcmp(classname, "func") == 0)
        {
        if (!PyCallable_Check(arg))
          {
          penalty = VTK_PYTHON_GOOD_MATCH;
          if (arg != Py_None)
            {
            penalty = VTK_PYTHON_INCOMPATIBLE;
            }
          }
        }

      // Assume any pointers are vtkObjectBase-derived types
      else if (classname[0] == '*' && strncmp(&classname[1], "vtk", 3) == 0)
        {
        classname++;

        if (arg == Py_None)
          {
          penalty = VTK_PYTHON_GOOD_MATCH;
          }
        else if (PyVTKObject_Check(arg))
          {
          PyVTKObject *vobj = (PyVTKObject *)arg;
          if (strncmp(vobj->vtk_ptr->GetClassName(), classname, 127) != 0)
            {
            // Trace back through superclasses to look for a match
            PyVTKClass *cls = vobj->vtk_class;
            if (PyTuple_Size(cls->vtk_bases) == 0)
              {
              penalty = VTK_PYTHON_INCOMPATIBLE;
              }
            else
              {
              penalty = VTK_PYTHON_GOOD_MATCH;
              cls = (PyVTKClass *)PyTuple_GetItem(cls->vtk_bases,0);
              while (strncmp(PyString_AS_STRING(cls->vtk_name),
                     classname, 127) != 0)
                {
                if (PyTuple_Size(cls->vtk_bases) > 0)
                  {
                  cls = (PyVTKClass *)PyTuple_GetItem(cls->vtk_bases,0);
                  }
                else
                  {
                  penalty = VTK_PYTHON_INCOMPATIBLE;
                  break;
                  }
                if (penalty+1 < VTK_PYTHON_NEEDS_CONVERSION)
                  {
                  penalty++;
                  }
                }
              }
            }
          }
        else
          {
          penalty = VTK_PYTHON_INCOMPATIBLE;
          }
        }

      // Any other object starting with "vtk" is a special object
      else if ((classname[0] == '&' && strncmp(&classname[1], "vtk", 3) == 0)
               || strncmp(classname, "vtk", 3) == 0)
        {
        // Skip over the "&" that indicates a reference
        if (classname[0] == '&')
          {
          classname++;
          }

        // Check for an exact match
        if (strncmp(arg->ob_type->tp_name, classname, 127) != 0)
          {
          // If it didn't match, then maybe conversion is possible
          penalty = VTK_PYTHON_NEEDS_CONVERSION;

          // Look up the required type in the map
          vtkPythonSpecialTypeMap::iterator iter;

          if (level != 0 ||
              (iter = vtkPythonMap->SpecialTypeMap->find(classname)) ==
              vtkPythonMap->SpecialTypeMap->end())
            {
            penalty = VTK_PYTHON_INCOMPATIBLE;
            }
          else
            {
            // Get info about the required type
            PyVTKSpecialType *info = &iter->second;

            // Try out all the constructor methods
            if (!vtkPythonUtil::FindConversionMethod(info->constructors, arg))
              {
              penalty = VTK_PYTHON_INCOMPATIBLE;
              }
            }
          }
        }

      // Check for Qt types
      else if (classname[0] == '*' && classname[1] == 'Q' &&
        (classname[2] == 't' || classname[2] == toupper(classname[2])))
        {
        classname++;

        if (arg == Py_None)
          {
          penalty = VTK_PYTHON_GOOD_MATCH;
          }
        void* qobj = SIPGetPointerFromObject(arg, classname);
        if(!qobj)
          {
          penalty = VTK_PYTHON_INCOMPATIBLE;
          }
        else
          {
          penalty = VTK_PYTHON_GOOD_MATCH;
          }
        }

      // An array
      else if (classname[0] == '*')
        {
        // incompatible unless the type checks out
        penalty = VTK_PYTHON_INCOMPATIBLE;
        if (PySequence_Check(arg))
          {
#if PY_MAJOR_VERSION >= 2
          Py_ssize_t m = PySequence_Size(arg);
#else
          Py_ssize_t m = PySequence_Length(arg);
#endif
          if (m > 0)
            {
            // the "bool" is really just a dummy
            PyObject *sarg = PySequence_GetItem(arg, 0);
            penalty = vtkPythonUtil::CheckArg(sarg, &classname[1], "bool");
            Py_DECREF(sarg);
            }
          }
        }

      // An object of unrecognized type
      else
        {
        penalty = VTK_PYTHON_INCOMPATIBLE;
        }
      }
      break;

    default:
      vtkGenericWarningMacro("Unrecognized python format character "
                             << format[0]);
      penalty = VTK_PYTHON_INCOMPATIBLE;
    }

  return penalty;
}

//--------------------------------------------------------------------
// Call the overloaded method that is the best match for the arguments.
// The first arg is name of the class that the methods belong to, it
// is there for potential diagnostic usage but is currently unused.

PyObject *vtkPythonUtil::CallOverloadedMethod(
  PyMethodDef *methods, PyObject *self, PyObject *args)
{
  PyMethodDef *meth = &methods[0];
  int matchCount = 1;

  // Make sure there is more than one method
  if (methods[1].ml_meth != 0)
    {
    vtkPythonOverloadHelper helperStorage[16];
    vtkPythonOverloadHelper *helperArray = helperStorage;
    vtkPythonOverloadHelper *helper;

    const char *format;
    const char *classname;
    bool selfIsClass = 0;
    int sig;

    // Is self a PyVTKClass object, rather than a PyVTKObject?  If so,
    // then first arg is an object, and other args should follow format.
    if (self && PyVTKClass_Check(self))
      {
      selfIsClass = true;
      }

    for (sig = 0; methods[sig].ml_meth != 0; sig++)
      {
      // Have we overgrown the stack storage?
      if ((sig & 15) == 0 && sig != 0)
        {
        // Grab more space from the heap
        vtkPythonOverloadHelper *tmp = helperArray;
        helperArray = new vtkPythonOverloadHelper[sig+16];
        for (int k = 0; k < sig; k++)
          {
          helperArray[k] = tmp[k];
          }
        if (tmp != helperStorage)
          {
          delete [] tmp;
          }
        }

      // Initialize the helper for ths signature
      helperArray[sig].initialize(selfIsClass, methods[sig].ml_doc);
      }

    // Get the number of signatures
    int nsig = sig;

    // Go through the tuple and check each arg against each format, knocking
    // out mismatched functions as we go along.  For matches, prioritize:
    // 0) exact type matches first
    // 1) trivial conversions second, e.g. double to float
    // 2) other conversions third, e.g. double to int

    // Loop through args
    Py_ssize_t n = PyTuple_Size(args);
    for (Py_ssize_t i = 0; i < n; i++)
      {
      PyObject *arg = PyTuple_GetItem(args, i);

      for (sig = 0; sig < nsig; sig++)
        {
        helper = &helperArray[sig];

        if (helper->penalty() != VTK_PYTHON_INCOMPATIBLE &&
            helper->next(&format, &classname))
          {
          if (*format != '(')
            {
            helper->penalty(vtkPythonUtil::CheckArg(arg, format, classname));
            }
          else
            {
            if (!PySequence_Check(arg))
              {
              helper->penalty(VTK_PYTHON_INCOMPATIBLE);
              }
            else
              {
              // Note: we don't reject the method if the sequence count
              // doesn't match.  If that circumstance occurs, we want the
              // method to be called with an incorrect count so that a
              // useful error will be reported to the user.  Also, we want
              // to mimic C++ semantics, and C++ doesn't care about the
              // size of arrays when it resolves overloads.
#if PY_MAJOR_VERSION >= 2
              Py_ssize_t m = PySequence_Size(arg);
#else
              Py_ssize_t m = PySequence_Length(arg);
#endif
              for (Py_ssize_t j = 0;; j++)
                {
                if (!helper->next(&format, &classname))
                  {
                  helper->penalty(VTK_PYTHON_INCOMPATIBLE);
                  break;
                  }
                if (*format == ')')
                  {
                  break;
                  }

                if (j < m)
                  {
                  PyObject *sarg = PySequence_GetItem(arg, j);
                  helper->penalty(vtkPythonUtil::CheckArg(sarg, format, classname));
                  Py_DECREF(sarg);
                  }
                }
              }
            }
          }
        else
          {
          helper->penalty(VTK_PYTHON_INCOMPATIBLE);
          }
        }
      }

    // Loop through methods and identify the best match
    int minPenalty = VTK_PYTHON_INCOMPATIBLE;
    meth = 0;
    matchCount = 0;
    for (sig = 0; sig < nsig; sig++)
      {
      // the "helper->next" check ensures that there are no leftover args
      helper = &helperArray[sig];
      int penalty = helper->penalty();
      if (penalty <= minPenalty && penalty < VTK_PYTHON_INCOMPATIBLE &&
          !helper->next(&format, &classname))
        {
        if (penalty < minPenalty)
          {
          matchCount = 0;
          minPenalty = penalty;
          meth = &methods[sig];
          }
        matchCount++;
        }
      }

    // Free any heap space that we have used
    if (helperArray != helperStorage)
      {
      delete [] helperArray;
      }
    }

  if (meth && matchCount > 1)
    {
    PyErr_SetString(PyExc_TypeError,
      "ambiguous call, multiple overloaded methods match the arguments");

    return NULL;
    }

  if (meth)
    {
    return meth->ml_meth(self, args);
    }

  PyErr_SetString(PyExc_TypeError,
    "arguments do not match any overloaded methods");

  return NULL;
}

//--------------------------------------------------------------------
// Look through the a batch of constructor methods to see if any of
// them take the provided argument.

PyMethodDef *vtkPythonUtil::FindConversionMethod(
  PyMethodDef *methods, PyObject *arg)
{
  vtkPythonOverloadHelper helper;
  const char *format, *classname, *dummy1, *dummy2;
  int minPenalty = VTK_PYTHON_NEEDS_CONVERSION;
  PyMethodDef *method = 0;
  int matchCount = 0;

  for (PyMethodDef *meth = methods; meth->ml_meth != NULL; meth++)
    {
    // If method has "explicit" marker, don't use for conversions
    if (meth->ml_doc[0] != '-')
      {
      // If meth only takes one arg
      helper.initialize(0, meth->ml_doc);
      if (helper.next(&format, &classname) &&
          !helper.next(&dummy1, &dummy2))
        {
        // If the constructor accepts the arg without
        // additional conversion, then we found a match
        int penalty = vtkPythonUtil::CheckArg(arg, format, classname, 1);

        if (penalty < minPenalty)
          {
          matchCount = 1;
          minPenalty = penalty;
          method = meth;
          }
        else if (meth && penalty == minPenalty)
          {
          matchCount++;
          }
        }
      }
    }

  // if matchCount > 1, there was ambiguity, but we silently use
  // the first match that was found instead of raising an error

  return method;
}

//--------------------------------------------------------------------
vtkObjectBase *vtkPythonUtil::VTKParseTuple(
  PyObject *pself, PyObject *args, const char *format, ...)
{
  PyVTKObject *self = (PyVTKObject *)pself;
  vtkObjectBase *obj = NULL;
  va_list va;
  va_start(va, format);

  /* check if this was called as an unbound method */
  if (PyVTKClass_Check(pself))
    {
    int n = PyTuple_Size(args);
    PyVTKClass *vtkclass = (PyVTKClass *)self;

    if (n == 0 || (self = (PyVTKObject *)PyTuple_GetItem(args, 0)) == NULL ||
        !PyVTKObject_Check(pself) ||
        !self->vtk_ptr->IsA(PyString_AS_STRING(vtkclass->vtk_name)))
      {
      char buf[256];
      sprintf(buf,"unbound method requires a %s as the first argument",
              PyString_AS_STRING(vtkclass->vtk_name));
      PyErr_SetString(PyExc_ValueError,buf);
      return NULL;
      }
    // re-slice the args to remove 'self'
    args = PyTuple_GetSlice(args,1,n);
    if (PyArg_VaParse(args, (char *)format, va))
      {
      obj = self->vtk_ptr;
      }
    Py_DECREF(args);
    }
  /* it was called as a bound method */
  else
    {
    if (PyArg_VaParse(args, (char *)format, va))
      {
      obj = self->vtk_ptr;
      }
    }
  return obj;
}

//--------------------------------------------------------------------
PyVTKSpecialType *vtkPythonUtil::AddSpecialTypeToMap(
  PyTypeObject *pytype, PyMethodDef *methods, PyMethodDef *constructors,
  const char *docstring[], PyVTKSpecialCopyFunc copyfunc)
{
  const char *classname = pytype->tp_name;

  if (vtkPythonMap == NULL)
    {
    vtkPythonMap = new vtkPythonUtil();
    Py_AtExit(vtkPythonUtilDelete);
    }

#ifdef VTKPYTHONDEBUG
  //  vtkGenericWarningMacro("Adding an type " << type << " to map ptr");
#endif

  // lets make sure it isn't already there
  vtkPythonSpecialTypeMap::iterator i =
    vtkPythonMap->SpecialTypeMap->find(classname);
  if(i != vtkPythonMap->SpecialTypeMap->end())
    {
#ifdef VTKPYTHONDEBUG
    vtkGenericWarningMacro("Attempt to add type to the map when already there!!!");
#endif
    return 0;
    }

  i = vtkPythonMap->SpecialTypeMap->insert(i,
    vtkPythonSpecialTypeMap::value_type(
      classname,
      PyVTKSpecialType(pytype, methods, constructors,
                       docstring, copyfunc)));

#ifdef VTKPYTHONDEBUG
  //  vtkGenericWarningMacro("Added type to map type = " << typeObject);
#endif

  return &i->second;
}

//--------------------------------------------------------------------
PyVTKSpecialType *vtkPythonUtil::FindSpecialType(const char *classname)
{
  if (vtkPythonMap)
    {
    vtkPythonSpecialTypeMap::iterator it =
      vtkPythonMap->SpecialTypeMap->find(classname);

    if (it != vtkPythonMap->SpecialTypeMap->end())
      {
      return &it->second;
      }
    }

  return NULL;
}

//--------------------------------------------------------------------
void vtkPythonUtil::AddObjectToMap(PyObject *obj, vtkObjectBase *ptr)
{
  if (vtkPythonMap == NULL)
    {
    vtkPythonMap = new vtkPythonUtil();
    Py_AtExit(vtkPythonUtilDelete);
    }

#ifdef VTKPYTHONDEBUG
  vtkGenericWarningMacro("Adding an object to map ptr = " << ptr);
#endif

  ((PyVTKObject *)obj)->vtk_ptr = ptr;
  (*vtkPythonMap->ObjectMap)[ptr] = obj;

#ifdef VTKPYTHONDEBUG
  vtkGenericWarningMacro("Added object to map obj= " << obj << " "
                         << ptr);
#endif
}

//--------------------------------------------------------------------
void vtkPythonUtil::RemoveObjectFromMap(PyObject *obj)
{
  vtkObjectBase *ptr = ((PyVTKObject *)obj)->vtk_ptr;

#ifdef VTKPYTHONDEBUG
  vtkGenericWarningMacro("Deleting an object from map obj = " << obj << " "
                         << obj->vtk_ptr);
#endif

  if (vtkPythonMap)
    {
    vtkPythonMap->ObjectMap->erase(ptr);
    }
}

//--------------------------------------------------------------------
PyObject *vtkPythonUtil::GetObjectFromPointer(vtkObjectBase *ptr)
{
  PyObject *obj = NULL;

#ifdef VTKPYTHONDEBUG
  vtkGenericWarningMacro("Checking into pointer " << ptr);
#endif

  if (ptr)
    {
    vtkstd::map<vtkSmartPointerBase, PyObject*>::iterator i =
      vtkPythonMap->ObjectMap->find(ptr);
    if(i != vtkPythonMap->ObjectMap->end())
      {
      obj = i->second;
      }
    if (obj)
      {
      Py_INCREF(obj);
      }
    }
  else
    {
    Py_INCREF(Py_None);
    obj = Py_None;
    }
#ifdef VTKPYTHONDEBUG
  vtkGenericWarningMacro("Checking into pointer " << ptr << " obj = " << obj);
#endif

  if (obj == NULL)
    {
    PyObject *vtkclass = NULL;
    vtkstd::map<vtkstd::string, PyObject*>::iterator i =
      vtkPythonMap->ClassMap->find(ptr->GetClassName());
    if(i != vtkPythonMap->ClassMap->end())
      {
      vtkclass = i->second;
      }

      // if the class was not in the map, then find the nearest base class
      // that is and associate ptr->GetClassName() with that base class
    if (vtkclass == NULL)
      {
      vtkclass = vtkPythonUtil::FindNearestBaseClass(ptr);
      vtkPythonUtil::AddClassToMap(vtkclass, ptr->GetClassName());
      }

    obj = PyVTKObject_New(vtkclass, ptr);
    }

  return obj;
}

//--------------------------------------------------------------------
void vtkPythonUtil::AddClassToMap(PyObject *vtkclass, const char *classname)
{
  if (vtkPythonMap == NULL)
    {
    vtkPythonMap = new vtkPythonUtil();
    Py_AtExit(vtkPythonUtilDelete);
    }

#ifdef VTKPYTHONDEBUG
  //  vtkGenericWarningMacro("Adding an type " << type << " to map ptr");
#endif

  // lets make sure it isn't already there
  vtkPythonClassMap::iterator i =
    vtkPythonMap->ClassMap->find(classname);
  if(i != vtkPythonMap->ClassMap->end())
    {
#ifdef VTKPYTHONDEBUG
    vtkGenericWarningMacro("Attempt to add type to the map when already there!!!");
#endif
    return;
    }

  (*vtkPythonMap->ClassMap)[classname] = vtkclass;

#ifdef VTKPYTHONDEBUG
  //  vtkGenericWarningMacro("Added type to map type = " << typeObject);
#endif
}

//--------------------------------------------------------------------
PyObject *vtkPythonUtil::FindClass(const char *classname)
{
  if (vtkPythonMap)
    {
    vtkPythonClassMap::iterator it =
      vtkPythonMap->ClassMap->find(classname);
    if (it != vtkPythonMap->ClassMap->end())
      {
      return it->second;
      }
    }

  return NULL;
}

//--------------------------------------------------------------------
// this is a helper function to find the nearest base class for an
// object whose class is not in the ClassDict
PyObject *vtkPythonUtil::FindNearestBaseClass(vtkObjectBase *ptr)
{
  PyObject *nearestbase = NULL;
  int maxdepth = 0;
  int depth;

  for(vtkPythonClassMap::iterator classes =
        vtkPythonMap->ClassMap->begin();
      classes != vtkPythonMap->ClassMap->end(); ++classes)
    {
    PyObject *pyclass = classes->second;

    if (ptr->IsA(PyString_AS_STRING(((PyVTKClass *)pyclass)->vtk_name)))
      {
      PyObject *cls = pyclass;
      PyObject *bases = ((PyVTKClass *)pyclass)->vtk_bases;
      // count the heirarchy depth for this class
      for (depth = 0; PyTuple_Size(bases) != 0; depth++)
        {
        cls = PyTuple_GetItem(bases,0);
        bases = ((PyVTKClass *)cls)->vtk_bases;
        }
      // we want the class that is furthest from vtkObjectBase
      if (depth > maxdepth)
        {
        maxdepth = depth;
        nearestbase = pyclass;
        }
      }
    }

  return nearestbase;
}

//--------------------------------------------------------------------
vtkObjectBase *vtkPythonUtil::GetPointerFromObject(
  PyObject *obj, const char *result_type)
{
  vtkObjectBase *ptr;

  // convert Py_None to NULL every time
  if (obj == Py_None)
    {
      return NULL;
    }

  // check to ensure it is a vtk object
  if (!PyVTKObject_Check(obj))
    {
    obj = PyObject_GetAttrString(obj,(char*)"__vtk__");
    if (obj)
      {
      PyObject *arglist = Py_BuildValue((char*)"()");
      PyObject *result = PyEval_CallObject(obj, arglist);
      Py_DECREF(arglist);
      Py_DECREF(obj);
      if (result == NULL)
        {
        return NULL;
        }
      if (PyVTKObject_Check(result))
        {
        PyErr_SetString(PyExc_ValueError,"__vtk__() doesn't return a VTK object");
        Py_DECREF(result);
        return NULL;
        }
      else
        {
        ptr = ((PyVTKObject *)result)->vtk_ptr;
        Py_DECREF(result);
        }
      }
    else
      {
#ifdef VTKPYTHONDEBUG
      vtkGenericWarningMacro("Object " << obj << " is not a VTK object!!");
#endif
      PyErr_SetString(PyExc_ValueError,"method requires a VTK object");
      return NULL;
      }
    }
  else
    {
    ptr = ((PyVTKObject *)obj)->vtk_ptr;
    }

#ifdef VTKPYTHONDEBUG
  vtkGenericWarningMacro("Checking into obj " << obj << " ptr = " << ptr);
#endif

  if (ptr->IsA(result_type))
    {
#ifdef VTKPYTHONDEBUG
      vtkGenericWarningMacro("Got obj= " << obj << " ptr= " << ptr << " " << result_type);
#endif
      return ptr;
    }
  else
    {
    char error_string[256];
#ifdef VTKPYTHONDEBUG
    vtkGenericWarningMacro("vtk bad argument, type conversion failed.");
#endif
    sprintf(error_string,"method requires a %s, a %s was provided.",
            result_type,((vtkObjectBase *)ptr)->GetClassName());
    PyErr_SetString(PyExc_ValueError,error_string);
    return NULL;
    }
}

//----------------
// union of long int and pointer
union vtkPythonUtilPointerUnion
{
  void *p;
#if VTK_SIZEOF_VOID_P == VTK_SIZEOF_LONG
  unsigned long l;
#elif defined(VTK_TYPE_USE_LONG_LONG)
  unsigned long long l;
#elif defined(VTK_TYPE_USE___INT64)
  unsigned __int64 l;
#endif
};

//----------------
// union of long int and pointer
union vtkPythonUtilConstPointerUnion
{
  const void *p;
#if VTK_SIZEOF_VOID_P == VTK_SIZEOF_LONG
  unsigned long l;
#elif defined(VTK_TYPE_USE_LONG_LONG)
  unsigned long long l;
#elif defined(VTK_TYPE_USE___INT64)
  unsigned __int64 l;
#endif
};

//--------------------------------------------------------------------
PyObject *vtkPythonUtil::GetObjectFromObject(
  PyObject *arg, const char *type)
{
  union vtkPythonUtilPointerUnion u;

  if (PyString_Check(arg))
    {
    vtkObjectBase *ptr;
    char *ptrText = PyString_AsString(arg);

    char typeCheck[256];  // typeCheck is currently not used
#if VTK_SIZEOF_VOID_P == VTK_SIZEOF_LONG
    int i = sscanf(ptrText,"_%lx_%s", &u.l, typeCheck);
#elif defined(VTK_TYPE_USE_LONG_LONG)
    int i = sscanf(ptrText,"_%llx_%s", &u.l, typeCheck);
#elif defined(VTK_TYPE_USE___INT64)
    int i = sscanf(ptrText,"_%I64x_%s", &u.l, typeCheck);
#endif

    if (i <= 0)
      {
#if VTK_SIZEOF_VOID_P == VTK_SIZEOF_LONG
      i = sscanf(ptrText,"Addr=0x%lx", &u.l);
#elif defined(VTK_TYPE_USE_LONG_LONG)
      i = sscanf(ptrText,"Addr=0x%llx", &u.l);
#elif defined(VTK_TYPE_USE___INT64)
      i = sscanf(ptrText,"Addr=0x%I64x", &u.l);
#endif
      }
    if (i <= 0)
      {
      i = sscanf(ptrText, "%p", &u.p);
      }
    if (i <= 0)
      {
      PyErr_SetString(PyExc_ValueError,"could not extract hexidecimal address from argument string");
      return NULL;
      }

    ptr = static_cast<vtkObjectBase *>(u.p);

    if (!ptr->IsA(type))
      {
      char error_string[256];
      sprintf(error_string,"method requires a %s address, a %s address was provided.",
              type, ptr->GetClassName());
      PyErr_SetString(PyExc_TypeError,error_string);
      return NULL;
      }

    return vtkPythonUtil::GetObjectFromPointer(ptr);
    }

  PyErr_SetString(PyExc_TypeError,"method requires a string argument");
  return NULL;
}

//--------------------------------------------------------------------
void *vtkPythonUtil::GetPointerFromSpecialObject(
  PyObject *obj, const char *result_type, PyObject **newobj)
{
  // Clear newobj, it will only be set if a new obj is created
  *newobj = 0;

  // The type name
  const char *object_type = obj->ob_type->tp_name;

  // check to make sure that it is the right type
  if (strcmp(object_type, result_type) == 0)
    {
    return ((PyVTKSpecialObject *)obj)->vtk_ptr;
    }
  else if (PyVTKObject_Check(obj))
    {
    // use the VTK type name, instead of "vtkobject"
    object_type =
      PyString_AS_STRING(((PyVTKObject *)obj)->vtk_class->vtk_name);
    }

  // try to construct the special object from the supplied object
  vtkstd::map<vtkstd::string, PyVTKSpecialType>::iterator it =
    vtkPythonMap->SpecialTypeMap->find(result_type);
  if(it != vtkPythonMap->SpecialTypeMap->end())
    {
    PyObject *sobj = 0;

    PyVTKSpecialType *info = &it->second;
    PyMethodDef *meth =
      vtkPythonUtil::FindConversionMethod(info->constructors, obj);

    // If a constructor signature exists for "obj", call it
    if (meth && meth->ml_meth)
      {
      PyObject *args = PyTuple_New(1);
      PyTuple_SET_ITEM(args, 0, obj);
      Py_INCREF(obj);

      sobj = meth->ml_meth(0, args);

      Py_DECREF(args);
      }

    if (sobj)
      {
      *newobj = sobj;
      return ((PyVTKSpecialObject *)sobj)->vtk_ptr;
      }
    else if (sobj)
      {
      Py_DECREF(sobj);
      }

    // If a TypeError occurred, clear it and set our own error
    PyObject *ex = PyErr_Occurred();
    if (ex == NULL || !PyErr_GivenExceptionMatches(ex, PyExc_TypeError))
      {
      return NULL;
      }

    PyErr_Clear();
    }

#ifdef VTKPYTHONDEBUG
  vtkGenericWarningMacro("vtk bad argument, type conversion failed.");
#endif

  char error_string[256];
  sprintf(error_string,"method requires a %s, a %s was provided.",
          result_type, object_type);
  PyErr_SetString(PyExc_TypeError, error_string);

  return NULL;
}

//--------------------------------------------------------------------
// mangle a void pointer into a SWIG-style string
char *vtkPythonUtil::ManglePointer(const void *ptr, const char *type)
{
  static char ptrText[128];
  int ndigits = 2*(int)sizeof(void *);
  union vtkPythonUtilConstPointerUnion u;
  u.p = ptr;
#if VTK_SIZEOF_VOID_P == VTK_SIZEOF_LONG
  sprintf(ptrText, "_%*.*lx_%s", ndigits, ndigits, u.l, type);
#elif defined(VTK_TYPE_USE_LONG_LONG)
  sprintf(ptrText, "_%*.*llx_%s", ndigits, ndigits, u.l, type);
#elif defined(VTK_TYPE_USE___INT64)
  sprintf(ptrText, "_%*.*I64x_%s", ndigits, ndigits, u.l, type);
#endif

  return ptrText;
}

//--------------------------------------------------------------------
// unmangle a void pointer from a SWIG-style string
void *vtkPythonUtil::UnmanglePointer(char *ptrText, int *len, const char *type)
{
  int i;
  union vtkPythonUtilPointerUnion u;
  char text[256];
  char typeCheck[256];
  typeCheck[0] = '\0';

  // Do some minimal checks that it might be a swig pointer.
  if (*len < 256 && *len > 4 && ptrText[0] == '_')
    {
    strncpy(text, ptrText, *len);
    text[*len] = '\0';
    i = *len;
    // Allow one null byte, in case trailing null is part of *len
    if (i > 0 && text[i-1] == '\0')
      {
      i--;
      }
    // Verify that there are no other null bytes
    while (i > 0 && text[i-1] != '\0')
      {
      i--;
      }

    // If no null bytes, then do a full check for a swig pointer
    if (i == 0)
      {
#if VTK_SIZEOF_VOID_P == VTK_SIZEOF_LONG
      i = sscanf(text, "_%lx_%s", &u.l ,typeCheck);
#elif defined(VTK_TYPE_USE_LONG_LONG)
      i = sscanf(text, "_%llx_%s", &u.l ,typeCheck);
#elif defined(VTK_TYPE_USE___INT64)
      i = sscanf(text, "_%I64x_%s", &u.l ,typeCheck);
#endif
      if (strcmp(type,typeCheck) == 0)
        { // sucessfully unmangle
        *len = 0;
        return u.p;
        }
      else if (i == 2)
        { // mangled pointer of wrong type
        *len = -1;
        return NULL;
        }
      }
    }

  // couldn't unmangle: return string as void pointer if it didn't look
  // like a SWIG mangled pointer
  return (void *)ptrText;
}

//--------------------------------------------------------------------
// These functions check an array that was sent to a method to see if
// any of the values were changed by the method.
// If a value was changed, then the corresponding value in the python
// list is modified.

template<class T>
inline
int vtkPythonCheckFloatArray(PyObject *args, int i, T *a, int n)
{
  int changed = 0;

  PyObject *seq = PyTuple_GET_ITEM(args, i);
  for (i = 0; i < n; i++)
    {
    PyObject *oldobj = PySequence_GetItem(seq, i);
    T oldval = (T)PyFloat_AsDouble(oldobj);
    Py_DECREF(oldobj);
    changed |= (a[i] != oldval);
    }

  if (changed)
    {
    for (i = 0; i < n; i++)
      {
      PyObject *newobj = PyFloat_FromDouble(a[i]);
      int rval = PySequence_SetItem(seq, i, newobj);
      Py_DECREF(newobj);
      if (rval == -1)
        {
        return -1;
        }
      }
    }

  return 0;
}

template<class T>
inline
int vtkPythonCheckIntArray(PyObject *args, int i, T *a, int n)
{
  int changed = 0;

  PyObject *seq = PyTuple_GET_ITEM(args, i);
  for (i = 0; i < n; i++)
    {
    PyObject *oldobj = PySequence_GetItem(seq, i);
    T oldval = (T)PyInt_AsLong(oldobj);
    Py_DECREF(oldobj);
    changed |= (a[i] != oldval);
    }

  if (changed)
    {
    for (i = 0; i < n; i++)
      {
      PyObject *newobj = PyInt_FromLong(a[i]);
      int rval = PySequence_SetItem(seq, i, newobj);
      Py_DECREF(newobj);
      if (rval == -1)
        {
        return -1;
        }
      }
    }

  return 0;
}

#if defined(VTK_TYPE_USE_LONG_LONG) || defined(VTK_TYPE_USE___INT64)
template<class T>
inline
int vtkPythonCheckLongArray(PyObject *args, int i, T *a, int n)
{
  int changed = 0;

  PyObject *seq = PyTuple_GET_ITEM(args, i);
  for (i = 0; i < n; i++)
    {
    PyObject *oldobj = PySequence_GetItem(seq, i);
    T oldval;
    if (PyLong_Check(oldobj))
      {
#ifdef PY_LONG_LONG
      oldval = PyLong_AsLongLong(oldobj);
#else
      oldval = PyLong_AsLong(oldobj);
#endif
      }
    else
      {
      oldval = PyInt_AsLong(oldobj);
      }
    Py_DECREF(oldobj);
    changed |= (a[i] != oldval);
    }

  if (changed)
    {
    for (i = 0; i < n; i++)
      {
#if defined(VTK_TYPE_USE_LONG_LONG)
# if defined(PY_LONG_LONG) && (VTK_SIZEOF_LONG != VTK_SIZEOF_LONG_LONG)
      PyObject *newobj = PyLong_FromLongLong(a[i]);
# else
      PyObject *newobj = PyInt_FromLong((long)a[i]);
# endif
#else
# if defined(PY_LONG_LONG) && (VTK_SIZEOF_LONG != VTK_SIZEOF___INT64)
      PyObject *newobj = PyLong_FromLongLong(a[i]);
# else
      PyObject *newobj = PyInt_FromLong((long)a[i]);
# endif
#endif
      int rval = PySequence_SetItem(seq, i, newobj);
      Py_DECREF(newobj);
      if (rval == -1)
        {
        return -1;
        }
      }
    }

  return 0;
}
#endif

int vtkPythonUtil::CheckArray(PyObject *args, int i, bool *a, int n)
{
  int changed = 0;

  PyObject *seq = PyTuple_GET_ITEM(args, i);
  for (i = 0; i < n; i++)
    {
    PyObject *oldobj = PySequence_GetItem(seq, i);
    bool oldval = (PyObject_IsTrue(oldobj) != 0);
    Py_DECREF(oldobj);
    changed |= (a[i] != oldval);
    }

  if (changed)
    {
    for (i = 0; i < n; i++)
      {
#if PY_VERSION_HEX >= 0x02030000
      PyObject *newobj = PyBool_FromLong(a[i]);
#else
      PyObject *newobj = PyInt_FromLong(a[i]);
#endif
      int rval = PySequence_SetItem(seq, i, newobj);
      Py_DECREF(newobj);
      if (rval == -1)
        {
        return -1;
        }
      }
    }

  return 0;
}

int vtkPythonUtil::CheckArray(PyObject *args, int i, char *a, int n)
{
  return vtkPythonCheckIntArray(args, i, a, n);
}

int vtkPythonUtil::CheckArray(PyObject *args, int i, signed char *a, int n)
{
  return vtkPythonCheckIntArray(args, i, a, n);
}

int vtkPythonUtil::CheckArray(PyObject *args, int i, unsigned char *a, int n)
{
  return vtkPythonCheckIntArray(args, i, a, n);
}

int vtkPythonUtil::CheckArray(PyObject *args, int i, short *a, int n)
{
  return vtkPythonCheckIntArray(args, i, a, n);
}

int vtkPythonUtil::CheckArray(PyObject *args, int i, unsigned short *a, int n)
{
  return vtkPythonCheckIntArray(args, i, a, n);
}

int vtkPythonUtil::CheckArray(PyObject *args, int i, int *a, int n)
{
  return vtkPythonCheckIntArray(args, i, a, n);
}

int vtkPythonUtil::CheckArray(PyObject *args, int i, unsigned int *a, int n)
{
  return vtkPythonCheckIntArray(args, i, a, n);
}

int vtkPythonUtil::CheckArray(PyObject *args, int i, long *a, int n)
{
  return vtkPythonCheckIntArray(args, i, a, n);
}

int vtkPythonUtil::CheckArray(PyObject *args, int i, unsigned long *a, int n)
{
  return vtkPythonCheckIntArray(args, i, a, n);
}

int vtkPythonUtil::CheckArray(PyObject *args, int i, float *a, int n)
{
  return vtkPythonCheckFloatArray(args, i, a, n);
}

int vtkPythonUtil::CheckArray(PyObject *args, int i, double *a, int n)
{
  return vtkPythonCheckFloatArray(args, i, a, n);
}

#if defined(VTK_TYPE_USE_LONG_LONG)
int vtkPythonUtil::CheckArray(PyObject *args, int i, long long *a, int n)
{
  return vtkPythonCheckLongArray(args, i, a, n);
}
int vtkPythonUtil::CheckArray(PyObject *args, int i, unsigned long long *a, int n)
{
  return vtkPythonCheckLongArray(args, i, a, n);
}
#endif

#if defined(VTK_TYPE_USE___INT64)
int vtkPythonUtil::CheckArray(PyObject *args, int i, __int64 *a, int n)
{
  return vtkPythonCheckLongArray(args, i, a, n);
}
int vtkPythonUtil::CheckArray(PyObject *args, int i, unsigned __int64 *a, int n)
{
  return vtkPythonCheckLongArray(args, i, a, n);
}
#endif

//--------------------------------------------------------------------
long vtkPythonUtil::VariantHash(const vtkVariant *v)
{
  long h = -1;

  // This uses the same rules as the vtkVariant "==" operator.
  // All types except for vtkObject are converted to strings.
  // Quite inefficient, but it gets the job done.  Fortunately,
  // the python vtkVariant is immutable, so its hash can be cached.

  switch (v->GetType())
    {
    case VTK_OBJECT:
      {
#if PY_MAJOR_VERSION >= 2
      h = _Py_HashPointer(v->ToVTKObject());
#else
      h = (long)(v->ToVTKObject());
#endif
      break;
      }

#ifdef Py_USING_UNICODE
    case VTK_UNICODE_STRING:
      {
      vtkUnicodeString u = v->ToUnicodeString();
      const char *s = u.utf8_str();
      PyObject *tmp = PyUnicode_DecodeUTF8(s, strlen(s), "strict");
      if (tmp == 0)
        {
        PyErr_Clear();
        return 0;
        }
      h = PyObject_Hash(tmp);
      Py_DECREF(tmp);
      break;
      }
#endif

    default:
      {
      vtkStdString s = v->ToString();
      PyObject *tmp = PyString_FromString(s.c_str());
      h = PyObject_Hash(tmp);
      Py_DECREF(tmp);
      break;
      }
    }

  return h;
}

//--------------------------------------------------------------------
void vtkPythonVoidFunc(void *arg)
{
  PyObject *arglist, *result;
  PyObject *func = (PyObject *)arg;

  // Sometimes it is possible for the function to be invoked after
  // Py_Finalize is called, this will cause nasty errors so we return if
  // the interpreter is not initialized.
  if (Py_IsInitialized() == 0)
    {
    return;
    }

#ifndef VTK_NO_PYTHON_THREADS
#if (PY_MAJOR_VERSION > 2) || \
((PY_MAJOR_VERSION == 2) && (PY_MINOR_VERSION >= 3))
  PyGILState_STATE state = PyGILState_Ensure();
#endif
#endif

  arglist = Py_BuildValue((char*)"()");

  result = PyEval_CallObject(func, arglist);
  Py_DECREF(arglist);

  if (result)
    {
    Py_XDECREF(result);
    }
  else
    {
    if (PyErr_ExceptionMatches(PyExc_KeyboardInterrupt))
      {
      cerr << "Caught a Ctrl-C within python, exiting program.\n";
      Py_Exit(1);
      }
    PyErr_Print();
    }

#ifndef VTK_NO_PYTHON_THREADS
#if (PY_MAJOR_VERSION > 2) || \
((PY_MAJOR_VERSION == 2) && (PY_MINOR_VERSION >= 3))
  PyGILState_Release(state);
#endif
#endif
}

//--------------------------------------------------------------------
void vtkPythonVoidFuncArgDelete(void *arg)
{
  PyObject *func = (PyObject *)arg;

  // Sometimes it is possible for the function to be invoked after
  // Py_Finalize is called, this will cause nasty errors so we return if
  // the interpreter is not initialized.
  if (Py_IsInitialized() == 0)
    {
    return;
    }

#ifndef VTK_NO_PYTHON_THREADS
#if (PY_MAJOR_VERSION > 2) || \
((PY_MAJOR_VERSION == 2) && (PY_MINOR_VERSION >= 3))
  PyGILState_STATE state = PyGILState_Ensure();
#endif
#endif

  if (func)
    {
    Py_DECREF(func);
    }

#ifndef VTK_NO_PYTHON_THREADS
#if (PY_MAJOR_VERSION > 2) || \
((PY_MAJOR_VERSION == 2) && (PY_MINOR_VERSION >= 3))
  PyGILState_Release(state);
#endif
#endif
}




#ifdef VTK_WRAP_PYTHON_SIP
// utilities to provide access to Python objects wrapped with SIP
static const sipAPIDef *get_sip_api()
{
  static sipAPIDef *sip_api = NULL;

  if(!sip_api)
    {
    PyObject *c_api = NULL;
    PyObject *sip_module;
    PyObject *sip_module_dict;

    /* Import the SIP module. */
    sip_module = PyImport_ImportModule("sip");

    if (sip_module == NULL)
      return NULL;

    /* Get the module's dictionary. */
    sip_module_dict = PyModule_GetDict(sip_module);

    /* Get the "_C_API" attribute. */
    c_api = PyDict_GetItemString(sip_module_dict, "_C_API");

    if (c_api == NULL)
      return NULL;

    /* Sanity check that it is the right type. */
    if (PyCObject_Check(c_api))
      sip_api = (sipAPIDef *)PyCObject_AsVoidPtr(c_api);

    /* some versions of SIP use PyCapsule instead of PyCObject */
#if PY_VERSION_HEX >= 0x02070000
    if (PyCapsule_CheckExact(c_api))
      sip_api = (sipAPIDef *)PyCapsule_GetPointer(c_api, "sip._C_API");
#endif
    }

  /* Get the actual pointer from the object. */
  return sip_api;
}
#endif

void* vtkPythonUtil::SIPGetPointerFromObject(PyObject *obj, const char *classname)
{
#ifdef VTK_WRAP_PYTHON_SIP
  const sipAPIDef * api = get_sip_api();
  if(!api)
    {
    PyErr_SetString(PyExc_TypeError, "Unable to convert to SIP type without api");
    return NULL;
    }

  const sipTypeDef * td = api->api_find_type(classname);
  if(!td)
    {
    PyErr_SetString(PyExc_TypeError, "Unable to convert to SIP type without typedef");
    return NULL;
    }

  if(sipTypeIsEnum(td))
    {
    ssize_t v = PyInt_AsLong(obj);
    if(v == -1)
      {
      PyErr_SetString(PyExc_TypeError, "Unable to convert to SIP enum type");
      return NULL;
      }
    return reinterpret_cast<void*>(v);
    }

  if(!api->api_can_convert_to_type(obj, td, 0))
    {
    PyErr_SetString(PyExc_TypeError, "Unable to convert to SIP type");
    return NULL;
    }

  int iserr = 0;
  void* ptr = api->api_convert_to_type(obj, td, NULL, 0, NULL, &iserr);
  if(iserr)
    {
    PyErr_SetString(PyExc_TypeError, "Error doing SIP conversion");
    return NULL;
    }
  return ptr;
#else
  obj = obj;
  classname = classname;
  PyErr_SetString(PyExc_TypeError, "method requires VTK built with SIP support");
  return NULL;
#endif
}


PyObject* vtkPythonUtil::SIPGetObjectFromPointer(const void *ptr, const char* classname, bool is_new)
{
#ifdef VTK_WRAP_PYTHON_SIP
  const sipAPIDef * api = get_sip_api();
  if(!api)
    {
    PyErr_SetString(PyExc_TypeError, "Unable to convert to SIP type without api");
    return NULL;
    }

  const sipTypeDef * td = api->api_find_type(classname);
  if(!td)
    {
    PyErr_SetString(PyExc_TypeError, "Unable to convert to SIP type without typedef");
    return NULL;
    }

  if(sipTypeIsEnum(td))
    {
    size_t v = reinterpret_cast<size_t>(ptr);
    return api->api_convert_from_enum(v, td);
    }

  if(is_new)
    {
    return api->api_convert_from_new_type(const_cast<void*>(ptr), td, NULL);
    }

  return api->api_convert_from_type(const_cast<void*>(ptr), td, NULL);

#else
  ptr = ptr;
  classname = classname;
  is_new = is_new;
  PyErr_SetString(PyExc_TypeError, "method requires VTK built with SIP support");
  return NULL;
#endif
}
