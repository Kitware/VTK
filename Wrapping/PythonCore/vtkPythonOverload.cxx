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
// Enums for vtkPythonOverload::CheckArg, the values between VTK_PYTHON_GOOD_MATCH
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
// A helper struct for CallMethod
class vtkPythonOverloadHelper
{
public:
  vtkPythonOverloadHelper() : m_format(0), m_classname(0), m_penalty(0) {};
  void initialize(bool selfIsClass, const char *format);
  bool next(const char **format, const char **classname);
  bool optional() { return m_optional; };
  int penalty() { return m_penalty; };
  int penalty(int p) {
    if (p > m_penalty) { m_penalty = p; };
    return m_penalty; };

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
// The classname is terminated with space, not with null
bool vtkPythonOverloadHelper::next(const char **format, const char **classname)
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
      *m_format != '|' && *m_format != '\0' && *m_format != ' ')
    {
    m_format++;
    }

  return true;
}

//--------------------------------------------------------------------
// If tmpi > VTK_INT_MAX, then penalize unless format == 'l'

#if VTK_SIZEOF_LONG != VTK_SIZEOF_INT
#ifdef PY_LONG_LONG
static int vtkPythonIntPenalty(PY_LONG_LONG tmpi, int penalty, char format)
#else
static int vtkPythonIntPenalty(long tmpi, int penalty, char format)
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

int vtkPythonOverload::CheckArg(
  PyObject *arg, const char *format, const char *classname, int level)
{
  int penalty = VTK_PYTHON_EXACT_MATCH;

  // If mutable object, check the type of the value inside
  if (PyVTKMutableObject_Check(arg))
    {
    arg = PyVTKMutableObject_GetValue(arg);
    }

  switch (*format)
    {
    case 'b':
    case 'B':
    case 'h':
    case 'H':
    case 'l':
    case 'k':
    case 'i':
    case 'I':
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
      else if (classname[0] == '*' && classname[1] == 'v' &&
               classname[2] == 't' && classname[3] == 'k')
        {
        classname++;

        if (arg == Py_None)
          {
          penalty = VTK_PYTHON_GOOD_MATCH;
          }
        else if (PyVTKObject_Check(arg))
          {
          PyVTKObject *vobj = (PyVTKObject *)arg;
          if (strncmp(vtkPythonUtil::PythonicClassName(
                vobj->vtk_ptr->GetClassName()), classname, 127) != 0)
            {
            // Trace back through superclasses to look for a match
            PyVTKClass *cls = vobj->vtk_class;
            if (PyTuple_GET_SIZE(cls->vtk_bases) == 0)
              {
              penalty = VTK_PYTHON_INCOMPATIBLE;
              }
            else
              {
              penalty = VTK_PYTHON_GOOD_MATCH;
              cls = (PyVTKClass *)PyTuple_GET_ITEM(cls->vtk_bases,0);
              while (strncmp(PyString_AS_STRING(cls->vtk_name),
                     classname, 127) != 0)
                {
                if (PyTuple_Size(cls->vtk_bases) > 0)
                  {
                  cls = (PyVTKClass *)PyTuple_GET_ITEM(cls->vtk_bases,0);
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
      else if (classname[0] == 'v' && classname[1] == 't' &&
               classname[2] == 'k')
        {
        // Check for an exact match
        if (strncmp(arg->ob_type->tp_name, classname, 127) != 0)
          {
#if PY_VERSION_HEX >= 0x02020000
          // Check superclasses
          PyTypeObject *basetype = arg->ob_type->tp_base;
          penalty = VTK_PYTHON_GOOD_MATCH;
          while (basetype &&
                 strncmp(basetype->tp_name, classname, 127) != 0)
            {
            penalty++;
            basetype = basetype->tp_base;
            }
          if (!basetype)
#endif
            {
            // If it didn't match, then maybe conversion is possible
            penalty = VTK_PYTHON_NEEDS_CONVERSION;

            // Look up the required type in the map
            PyVTKSpecialType *info = NULL;

            // The "level != 0" ensures that we don't chain conversions
            if (level != 0 ||
                (info = vtkPythonUtil::FindSpecialType(classname)) == NULL)
              {
              penalty = VTK_PYTHON_INCOMPATIBLE;
              }
            else
              {
              // Try out all the constructor methods
              if (!vtkPythonOverload::FindConversionMethod(
                     info->constructors, arg))
                {
                penalty = VTK_PYTHON_INCOMPATIBLE;
                }
              }
            }
          }
        }
      else if (classname[0] == '&' && classname[1] == 'v' &&
               classname[2] == 't' && classname[3] == 'k')
        {
        // Skip over the "&" that indicates a non-const reference
        classname++;

        // Check for an exact match
        if (strncmp(arg->ob_type->tp_name, classname, 127) != 0)
          {
#if PY_VERSION_HEX >= 0x02020000
          // Check superclasses
          PyTypeObject *basetype = arg->ob_type->tp_base;
          penalty = VTK_PYTHON_GOOD_MATCH;
          while (basetype &&
                 strncmp(basetype->tp_name, classname, 127) != 0)
            {
            penalty++;
            basetype = basetype->tp_base;
            }
          if (!basetype)
#endif
            {
            penalty = VTK_PYTHON_INCOMPATIBLE;
            }
          }
        }

      // Generic python objects
      else if (classname[0] == '*' && classname[1] == 'P' &&
               classname[2] == 'y')
        {
        // Skip over the "*"
        classname++;

        // Mark this match as low priority compared to other matches
        penalty = VTK_PYTHON_NEEDS_CONVERSION;

        // Code can be added here to do inheritance-based checks, but
        // this has to be done on a case-by-case basis because the "C"
        // name of a python type is different from its "Python" name.
        }

      // Qt objects and enums
      else if (((classname[0] == '*' || classname[0] == '&') &&
                (classname[1] == 'Q' && isalpha(classname[2]))) ||
               (classname[0] == 'Q' && isalpha(classname[1])))
        {
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
        }

      // Enum type
      else if (isalpha(classname[0]) ||
               (classname[0] == '&' && isalpha(classname[1])))
        {
        if (classname[0] == '&')
          {
          classname++;
          }
        if (PyInt_Check(arg))
          {
          if (strcmp(arg->ob_type->tp_name, classname) == 0)
            {
            penalty = VTK_PYTHON_EXACT_MATCH;
            }
          else
            {
            /* tp_name doesn't include namespace, so we also allow
               matches between "name" and "namespace.name" */
            size_t l, m;
            l = strlen(arg->ob_type->tp_name);
            m = strlen(classname);
            if (l < m && !isalnum(classname[m-l-1]) &&
                strcmp(arg->ob_type->tp_name, &classname[m-l]) == 0)
              {
              penalty = VTK_PYTHON_GOOD_MATCH;
              }
            else
              {
              penalty = VTK_PYTHON_NEEDS_CONVERSION;
              }
            }
          }
        else
          {
          penalty = VTK_PYTHON_INCOMPATIBLE;
          }
        }

      // An array
      else if (classname[0] == '*')
        {
        // incompatible unless the type checks out
        penalty = VTK_PYTHON_INCOMPATIBLE;
        char *cptr = const_cast<char *>(&classname[2]);
        Py_ssize_t sizeneeded = 0;
        PyObject *sarg = arg;
        while (PySequence_Check(sarg))
          {
#if PY_MAJOR_VERSION >= 2
          Py_ssize_t m = PySequence_Size(sarg);
#else
          Py_ssize_t m = PySequence_Length(sarg);
#endif
          if (m <= 0 || (sizeneeded != 0 && m != sizeneeded))
            {
            break;
            }

          PyObject *sargsave = sarg;
          sarg = PySequence_GetItem(sarg, 0);
          if (*cptr != '[')
            {
            // the "bool" is really just a dummy
            penalty = vtkPythonOverload::CheckArg(sarg, &classname[1], "bool");
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
