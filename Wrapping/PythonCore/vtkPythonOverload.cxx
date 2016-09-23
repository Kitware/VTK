/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPythonOverload.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * Created in June 2010 by David Gobbi, originally in vtkPythonUtil.
 *
 * This file provides methods for calling overloaded functions
 * that are stored in a PyMethodDef table.  The arguments are
 * checked against the format strings that are stored in the
 * documentation fields of the table.  For more information,
 * see vtkWrapPython_ArgCheckString() in vtkWrapPython.c.
 */


#include "vtkPythonOverload.h"
#include "vtkPythonUtil.h"

#include "vtkObject.h"

#ifdef VTK_WRAP_PYTHON_SIP
#include "sip.h"
#endif


//--------------------------------------------------------------------
// Enums for vtkPythonOverload::CheckArg().
// Values between VTK_PYTHON_GOOD_MATCH and VTK_PYTHON_NEEDS_CONVERSION
// are reserved for checking how many generations a vtkObject arg is from
// the requested arg type.

enum vtkPythonArgPenalties
{
  VTK_PYTHON_EXACT_MATCH = 0,
  VTK_PYTHON_GOOD_MATCH = 1,
  VTK_PYTHON_NEEDS_CONVERSION = 65534,
  VTK_PYTHON_INCOMPATIBLE = 65535
};

//--------------------------------------------------------------------
// A helper struct for CallMethod
class vtkPythonOverloadHelper
{
public:
  vtkPythonOverloadHelper() : m_format(0), m_classname(0), m_penalty(0) {}
  void initialize(bool selfIsClass, const char *format);
  bool next(const char **format, const char **classname);
  bool optional() { return m_optional; }
  int penalty() { return m_penalty; }
  int penalty(int p) {
    if (p > m_penalty) { m_penalty = p; }
    return m_penalty; }

private:
  const char *m_format;
  const char *m_classname;
  int m_penalty;
  bool m_optional;
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
  this->m_optional = false;
}

// Get the next format char and, if char is 'O', the classname.
// The classname is terminated with space, not with null.
// If there is no classname for an arg, classname will be set to NULL.
bool vtkPythonOverloadHelper::next(
  const char **format, const char **classname)
{
  if (*m_format == '|')
  {
    m_optional = true;
    m_format++;
  }

  if (*m_format == '\0' || *m_format == ' ')
  {
    return false;
  }

  // return the pointer to the current format character
  *format = m_format;

  // check if the parameter has extended type information
  char c = *m_format;
  if (c == '0' || c == 'V' || c == 'W' || c == 'Q' || c == 'E' ||
      c == 'A' || c == 'P')
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
  else
  {
    *classname = 0;
  }

  // increment to the next format character
  m_format++;

  return true;
}

//--------------------------------------------------------------------
// If tmpi > VTK_INT_MAX, then penalize types of int size or smaller

static int vtkPythonIntPenalty(PY_LONG_LONG tmpi, int penalty, char format)
{
  if (tmpi > VTK_INT_MAX || tmpi < VTK_INT_MIN)
  {
    if (format != 'k')
    {
      if (penalty < VTK_PYTHON_GOOD_MATCH)
      {
        penalty = VTK_PYTHON_GOOD_MATCH;
#if VTK_SIZEOF_LONG == VTK_SIZEOF_INT
        if (format != 'i')
        {
          penalty++;
        }
#else
        if (format != 'l')
        {
          penalty++;
          if (format != 'i')
          {
            penalty++;
          }
        }
#endif
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

#ifdef VTK_PY3K
//--------------------------------------------------------------------
// Check if a unicode string is ascii, which makes it more suitable
// as a match for "char *" or "std::string".

static int vtkPythonStringPenalty(PyObject *u, char format, int penalty)
{
#if PY_VERSION_HEX > 0x03030000
  int ascii = 0;
  if (PyUnicode_READY(u) != -1)
  {
    if (PyUnicode_KIND(u) == PyUnicode_1BYTE_KIND)
    {
      Py_UCS1 *cp = PyUnicode_1BYTE_DATA(u);
      Py_ssize_t l = PyUnicode_GET_LENGTH(u);
      Py_UCS1 c = 0;
      for (int i = 0; i < l; i++)
      {
        c |= cp[i];
      }
      ascii = ((c & 0x80) == 0);
    }
  }
  else
  {
    PyErr_Clear();
  }
#else
  PyObject *ascii = PyUnicode_AsASCIIString(u);
  if (ascii == 0)
  {
    PyErr_Clear();
  }
  else
  {
    Py_DECREF(ascii);
  }
#endif

  if ((format == 'u') ^ (ascii == 0))
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

  return penalty;
}
#endif

//--------------------------------------------------------------------
// This must check the same format chars that are used by
// vtkWrapPython_ArgCheckString() in vtkWrapPythonOverload.c.
//
// The "level" parameter limits possible recursion of this method,
// it is incremented every time recursion occurs.

int vtkPythonOverload::CheckArg(
  PyObject *arg, const char *format, const char *name, int level)
{
  int penalty = VTK_PYTHON_EXACT_MATCH;
  bool badref = false;

  // terminate the name string at the space delimiter
  char classtext[256];
  classtext[0] = '\0';
  if (name)
  {
    int k = 0;
    for (; k < 255 && name[k] != ' ' && name[k] != '\0'; k++)
    {
      classtext[k] = name[k];
    }
    classtext[k] = '\0';
  }
  const char *classname = classtext;

  // If mutable object, check the type of the value inside
  if (PyVTKMutableObject_Check(arg))
  {
    arg = PyVTKMutableObject_GetValue(arg);
  }

  switch (*format)
  {
    case '@':
      // "self" for methods (always matches)
      break;

    case 'q':
      // boolean
      if (!PyBool_Check(arg))
      {
        penalty = VTK_PYTHON_NEEDS_CONVERSION;
        int tmpi = PyObject_IsTrue(arg);
        if (tmpi == -1 || PyErr_Occurred())
        {
          PyErr_Clear();
          penalty = VTK_PYTHON_INCOMPATIBLE;
        }
      }
      break;

    case 'b':
    case 'B':
    case 'h':
    case 'H':
    case 'l':
    case 'L':
    case 'i':
    case 'I':
#ifdef VTK_PY3K
    case 'k':
    case 'K':
#endif
      // integer types
      if (PyBool_Check(arg))
      {
        penalty = VTK_PYTHON_GOOD_MATCH;
        if (*format != 'i')
        {
          penalty++;
        }
      }
#ifndef VTK_PY3K
      else if (PyInt_Check(arg))
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
#endif /* VTK_PY3K */
      else if (PyLong_Check(arg))
      {
#ifndef VTK_PY3K
        penalty = VTK_PYTHON_GOOD_MATCH;
#endif
        PY_LONG_LONG tmpi = PyLong_AsLongLong(arg);
        if (PyErr_Occurred())
        {
          PyErr_Clear();
          tmpi = VTK_LONG_MAX;
        }

        penalty = vtkPythonIntPenalty(tmpi, penalty, *format);
      }
      else // not PyInt or PyLong
      {
        if (level == 0)
        {
          penalty = VTK_PYTHON_NEEDS_CONVERSION;
#ifdef VTK_PY3K
          PY_LONG_LONG tmpi = PyLong_AsLongLong(arg);
#else
          long tmpi = PyInt_AsLong(arg);
#endif
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

#ifndef VTK_PY3K
    case 'k':
    case 'K':
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
    case 'd':
      // double and float
      if (PyFloat_Check(arg))
      {
        if (*format != 'd')
        {
          penalty = VTK_PYTHON_GOOD_MATCH;
        }
      }
      else
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
      if (PyUnicode_Check(arg) && PyUnicode_GetSize(arg) == 1)
      {
        penalty = VTK_PYTHON_NEEDS_CONVERSION;
      }
      else if (PyBytes_Check(arg) && PyBytes_Size(arg) == 1)
      {
        penalty = VTK_PYTHON_NEEDS_CONVERSION;
      }
      else
      {
        penalty = VTK_PYTHON_INCOMPATIBLE;
      }
      break;

    case 's':
    case 'z':
      // string and "char *"
      if (arg == Py_None)
      {
        penalty = VTK_PYTHON_NEEDS_CONVERSION;
        if (*format == 's')
        {
          penalty = VTK_PYTHON_INCOMPATIBLE;
        }
      }
#ifdef Py_USING_UNICODE
      else if (PyUnicode_Check(arg))
      {
#ifdef VTK_PY3K
        penalty = vtkPythonStringPenalty(arg, *format, penalty);
#else
        penalty = VTK_PYTHON_NEEDS_CONVERSION;
#endif
      }
#endif
      else if (!PyBytes_Check(arg))
      {
        penalty = VTK_PYTHON_INCOMPATIBLE;
      }
      break;

#ifdef Py_USING_UNICODE
    case 'u':
      // unicode string
      if (!PyUnicode_Check(arg))
      {
        penalty = VTK_PYTHON_INCOMPATIBLE;
      }
#ifdef VTK_PY3K
      else
      {
        penalty = vtkPythonStringPenalty(arg, *format, penalty);
      }
#endif
      break;
#endif

    case 'v':
      // memory buffer (void pointer)
      penalty = VTK_PYTHON_GOOD_MATCH;
      if (arg == Py_None)
      {
        penalty = VTK_PYTHON_NEEDS_CONVERSION;
      }
      // make sure that arg can act as a buffer
      else if (Py_TYPE(arg)->tp_as_buffer == 0)
      {
        penalty = VTK_PYTHON_INCOMPATIBLE;
      }
      break;

    case 'F':
      // callback function or None
      if (arg == Py_None)
      {
        penalty = VTK_PYTHON_GOOD_MATCH;
      }
      else if (!PyCallable_Check(arg))
      {
        penalty = VTK_PYTHON_INCOMPATIBLE;
      }
      break;

    case 'V':
      // VTK object pointer (instance of vtkObjectBase or a subclass)
      if (classname[0] == '*')
      {
        classname++;

        if (arg == Py_None)
        {
          penalty = VTK_PYTHON_GOOD_MATCH;
        }
        else if (PyVTKObject_Check(arg))
        {
          PyVTKClass *info = vtkPythonUtil::FindClass(classname);
          PyTypeObject *pytype = (info ? info->py_type : NULL);
          if (Py_TYPE(arg) != pytype)
          {
            // Check superclasses
            PyTypeObject *basetype = Py_TYPE(arg)->tp_base;
            penalty = VTK_PYTHON_GOOD_MATCH;
            while (basetype && basetype != pytype)
            {
              penalty++;
              basetype = basetype->tp_base;
            }
            if (!basetype)
            {
              penalty = VTK_PYTHON_INCOMPATIBLE;
            }
          }
        }
        else
        {
          penalty = VTK_PYTHON_INCOMPATIBLE;
        }
      }
      else
      {
        badref = true;
      }
      break;

    case 'W':
      // VTK special type (non reference counted)
      if (classname[0] != '*' && classname[0] != '&')
      {
        // Look up the required type in the map
        PyVTKSpecialType *info = vtkPythonUtil::FindSpecialType(classname);
        PyTypeObject *pytype = (info ? info->py_type : NULL);

        // Check for an exact match
        if (Py_TYPE(arg) != pytype)
        {
          // Check superclasses
          PyTypeObject *basetype = Py_TYPE(arg)->tp_base;
          penalty = VTK_PYTHON_GOOD_MATCH;
          while (basetype && basetype != pytype)
          {
            penalty++;
            basetype = basetype->tp_base;
          }
          if (!basetype)
          {
            // If it didn't match, then maybe conversion is possible
            penalty = VTK_PYTHON_NEEDS_CONVERSION;

            // The "level != 0" ensures that we don't chain conversions
            if (level != 0 || info == 0)
            {
              penalty = VTK_PYTHON_INCOMPATIBLE;
            }
            else
            {
              // Try out all the constructor methods
              if (!vtkPythonOverload::FindConversionMethod(
                     info->vtk_constructors, arg))
              {
                penalty = VTK_PYTHON_INCOMPATIBLE;
              }
            }
          }
        }
      }
      else if (classname[0] == '&')
      {
        // Skip over the "&" that indicates a non-const reference
        classname++;

        // Look up the required type in the map
        PyVTKSpecialType *info = vtkPythonUtil::FindSpecialType(classname);
        PyTypeObject *pytype = (info ? info->py_type : NULL);

        // Check for an exact match
        if (Py_TYPE(arg) != pytype)
        {
          // Check superclasses
          PyTypeObject *basetype = Py_TYPE(arg)->tp_base;
          penalty = VTK_PYTHON_GOOD_MATCH;
          while (basetype && basetype != pytype)
          {
            penalty++;
            basetype = basetype->tp_base;
          }
          if (!basetype)
          {
            penalty = VTK_PYTHON_INCOMPATIBLE;
          }
        }
      }
      else
      {
        badref = true;
      }
      break;

    case 'O':
      // Generic python objects
      if (classname[0] == '*')
      {
        // Skip over the "*"
        classname++;

        // Mark this match as low priority compared to other matches
        penalty = VTK_PYTHON_NEEDS_CONVERSION;

        // Code can be added here to do inheritance-based checks, but
        // this has to be done on a case-by-case basis because the "C"
        // name of a python type is different from its "Python" name.
      }
      else
      {
        badref = true;
      }
      break;

    case 'Q':
      // Qt objects and Qt enums
      if (classname[0] == '*' && arg == Py_None)
      {
        penalty = VTK_PYTHON_GOOD_MATCH;
      }
      else
      {
        if (classname[0] == '&' || classname[0] == '*')
        {
          classname++;
        }
        if (vtkPythonUtil::SIPGetPointerFromObject(arg, classname))
        {
          // Qt enums keep exact match, but Qt objects just get
          // a good match because they might have been converted
          if (classname[0] == 'Q' && isupper(classname[1]))
          {
            penalty = VTK_PYTHON_GOOD_MATCH;
          }
        }
        else
        {
          penalty = VTK_PYTHON_INCOMPATIBLE;
          PyErr_Clear();
        }
      }
      break;

    case 'E':
      // enum type
      if (classname[0] != '*')
      {
        if (classname[0] == '&')
        {
          classname++;
        }
        if (PyInt_Check(arg))
        {
          PyTypeObject *pytype = vtkPythonUtil::FindEnum(classname);
          if (pytype && PyObject_TypeCheck(arg, pytype))
          {
            penalty = VTK_PYTHON_EXACT_MATCH;
          }
          else
          {
            penalty = VTK_PYTHON_NEEDS_CONVERSION;
          }
        }
        else
        {
          penalty = VTK_PYTHON_INCOMPATIBLE;
        }
      }
      else
      {
        badref = true;
      }
      break;

    case 'A':
    case 'P':
      // An array
      if (classname[0] == '*')
      {
        // incompatible unless the type checks out
        penalty = VTK_PYTHON_INCOMPATIBLE;
        char *cptr = const_cast<char *>(&classname[2]);
        Py_ssize_t sizeneeded = 0;
        PyObject *sarg = arg;
        while (PySequence_Check(sarg))
        {
          Py_ssize_t m = PySequence_Size(sarg);
          if (m <= 0 || (sizeneeded != 0 && m != sizeneeded))
          {
            break;
          }

          PyObject *sargsave = sarg;
          sarg = PySequence_GetItem(sarg, 0);
          if (*cptr != '[')
          {
            penalty = vtkPythonOverload::CheckArg(sarg, &classname[1], "");
            Py_DECREF(sarg);
            break;
          }

          cptr++;
          sizeneeded = (Py_ssize_t)strtol(cptr, &cptr, 0);
          if (*cptr == ']')
          {
            cptr++;
          }
          if (sargsave != arg)
          {
            Py_DECREF(sargsave);
          }
        }
      }
      else
      {
        badref = true;
      }
      break;

    default:
      vtkGenericWarningMacro("Unrecognized arg format character "
                             << format[0]);
      penalty = VTK_PYTHON_INCOMPATIBLE;
  }

  if (badref)
  {
    vtkGenericWarningMacro("Illegal class ref for arg format character "
                           << format[0] << " " << classname);
    penalty = VTK_PYTHON_INCOMPATIBLE;
  }

  return penalty;
}

//--------------------------------------------------------------------
// Call the overloaded method that is the best match for the arguments.
// The first arg is name of the class that the methods belong to, it
// is there for potential diagnostic usage but is currently unused.

PyObject *vtkPythonOverload::CallMethod(
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

    const char *format = 0;
    const char *classname = 0;
    bool selfIsClass = 0;
    int sig;

    // Is self a type object, rather than an instance?  If so, then the
    // first arg is an object, and other args should follow format.
    if (self && PyType_Check(self))
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

      // Initialize the helper for this signature
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
    Py_ssize_t n = PyTuple_GET_SIZE(args);
    for (Py_ssize_t i = 0; i < n; i++)
    {
      PyObject *arg = PyTuple_GET_ITEM(args, i);

      for (sig = 0; sig < nsig; sig++)
      {
        helper = &helperArray[sig];

        if (helper->penalty() != VTK_PYTHON_INCOMPATIBLE &&
            helper->next(&format, &classname))
        {
          helper->penalty(vtkPythonOverload::CheckArg(arg, format, classname));
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
      helper = &helperArray[sig];
      int penalty = helper->penalty();
      // check whether too few args were passed for signature
      if (helper->next(&format, &classname) && !helper->optional())
      {
        penalty = VTK_PYTHON_INCOMPATIBLE;
      }
      // check if this signature has the minimum penalty
      if (penalty <= minPenalty && penalty < VTK_PYTHON_INCOMPATIBLE)
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

PyMethodDef *vtkPythonOverload::FindConversionMethod(
  PyMethodDef *methods, PyObject *arg)
{
  vtkPythonOverloadHelper helper;
  const char *dummy1, *dummy2;
  const char *format = 0;
  const char *classname = 0;
  PyMethodDef *method = 0;
  int minPenalty = VTK_PYTHON_NEEDS_CONVERSION;
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
        int penalty = vtkPythonOverload::CheckArg(arg, format, classname, 1);

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
