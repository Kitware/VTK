/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWrapPython.c

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPython.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "vtkParse.h"
#include "vtkParseMain.h"
#include "vtkParseHierarchy.h"
#include "vtkConfigure.h"

/* -------------------------------------------------------------------- */
/* the main entry method, called by vtkParse.y */
void vtkParseOutput(FILE *fp, FileInfo *data);


/* -------------------------------------------------------------------- */
/* prototypes for the methods used by the python wrappers */

/* generate the class docstring and write it to "fp" */
static void vtkWrapPython_ClassDoc(
  FILE *fp, FileInfo *file_info, ClassInfo *data, HierarchyInfo *hinfo,
  int is_vtkobject);

/* print out headers for any special types used by methods */
static void vtkWrapPython_GenerateSpecialHeaders(
  FILE *fp, ClassInfo *data, HierarchyInfo *hinfo);

/* print out any custom methods */
static void vtkWrapPython_CustomMethods(
  FILE *fp, ClassInfo *data, int do_constructors);

/* print out all methods and the method table */
static void vtkWrapPython_GenerateMethods(
  FILE *fp, ClassInfo *data, HierarchyInfo *hinfo,
  int is_vtkobject, int do_constructors);

/* make a temporary variable for an arg value or a return value */
static void vtkWrapPython_MakeTempVariable(
  FILE *fp, FunctionInfo *currentFunction, int i);

/* print the code to build python return value from a method */
static void vtkWrapPython_ReturnValue(
  FILE *fp, FunctionInfo *currentFunction);

/* print the code to return a hinted value from a method */
static void vtkWrapPython_ReturnHintedValue(
  FILE *fp, FunctionInfo *currentFunction);


/* -------------------------------------------------------------------- */
/* prototypes for utility methods */

/* check whether a method is wrappable */
static int vtkWrapPython_MethodCheck(
  ClassInfo *data, FunctionInfo *currentFunction, HierarchyInfo *hinfo);

/* is the method a constructor of the class */
static int vtkWrapPython_IsConstructor(
  ClassInfo *data, FunctionInfo *currentFunction);

/* is the method a destructor of the class */
static int vtkWrapPython_IsDestructor(
  ClassInfo *data, FunctionInfo *currentFunction);

/* create a format string for PyArg_ParseTuple */
static char *vtkWrapPython_FormatString(
  FunctionInfo *currentFunction);

/* weed out methods that will never be called */
static void vtkWrapPython_RemovePreceededMethods(
  FunctionInfo *wrappedFunctions[],
  int numberWrapped, int fnum);

/* create a string for checking arguments against available signatures */
static char *vtkWrapPython_ArgCheckString(
  int isvtkobjmethod, FunctionInfo *currentFunction);

/* quote a string for inclusion in a C string literal */
static const char *vtkWrapPython_QuoteString(
  const char *comment, size_t maxlen);

/* format a comment to a 70 char linewidth */
static const char *vtkWrapPython_FormatComment(
  const char *comment, size_t width);

/* format a method signature to a 70 char linewidth */
static const char *vtkWrapPython_FormatSignature(
  const char *signature, size_t width);

/* return a python-ized signature */
static const char *vtkWrapPython_PythonSignature(
  FunctionInfo *currentFunction);

/* -------------------------------------------------------------------- */
/* Use the hints in the hints file to get the tuple size to use when
 * returning for a pointer-type return value.  The Python return value
 * is created with Py_BuildValue() with the appropriate format string,
 * e.g. Py_BuildValue((char *)"fff", temp20[0], temp20[1], temp20[2]))
 * for a trio of floats.  The type cast is needed because python 2.0
 * Py_BuildValue does not use const.  The "20" is MAX_ARGS, which is
 * used to signify the return value.  If a null pointer is encountered,
 * then Py_BuildValue((char *)"") is used to create a None object. */

static void vtkWrapPython_ReturnHintedValue(
  FILE *fp, FunctionInfo *currentFunction)
{
  int i;
  const char *c = 0;

  /* Get the char code for the return type */
  switch (currentFunction->ReturnType & VTK_PARSE_UNQUALIFIED_TYPE)
    {
    /* Basic types */
    case VTK_PARSE_FLOAT_PTR: { c = "f"; break;}
    case VTK_PARSE_DOUBLE_PTR: { c = "d"; break;}
    case VTK_PARSE_INT_PTR: { c = "i"; break; }
    case VTK_PARSE_SHORT_PTR: { c = "i"; break; }
    case VTK_PARSE_UNSIGNED_SHORT_PTR: { c = "i"; break; }
    case VTK_PARSE_SIGNED_CHAR_PTR: { c = "i"; break; }
    case VTK_PARSE_UNSIGNED_CHAR_PTR: { c = "i"; break; }
    case VTK_PARSE_LONG_PTR: { c = "l"; break; }

    /* Bool was "int" until Python 2.3 */
    case VTK_PARSE_BOOL_PTR: { c = "i"; break; }

    /* The vtkIdType depends on configuration */
#ifdef VTK_USE_64BIT_IDS
#ifdef PY_LONG_LONG
    case VTK_PARSE_ID_TYPE_PTR: { c = "L"; break; }
#else
    case VTK_PARSE_ID_TYPE_PTR: { c = "l"; break; }
#endif
#else
    case VTK_PARSE_ID_TYPE_PTR: { c = "i"; break; }
#endif

    /* The 64-bit types require PY_LONG_LONG */
#ifdef PY_LONG_LONG
    case VTK_PARSE_LONG_LONG_PTR: { c = "L"; break; }
    case VTK_PARSE___INT64_PTR: { c = "L"; break; }
#else
    case VTK_PARSE_LONG_LONG_PTR: { c = "l"; break; }
    case VTK_PARSE___INT64_PTR: { c = "l"; break; }
#endif

    /* These should be added with appropriate compile-time checks */
    case VTK_PARSE_UNSIGNED_INT_PTR:
    case VTK_PARSE_UNSIGNED_LONG_PTR:
    case VTK_PARSE_UNSIGNED___INT64_PTR:
    case VTK_PARSE_UNSIGNED_LONG_LONG_PTR:
      break;
    }

  if (c)
    {
    /* Check to make sure the pointer is not NULL */
    fprintf(fp,
            "    if(temp%i)\n"
            "      {\n"
            "      result = Py_BuildValue((char*)\"",
            MAX_ARGS);

    for (i = 0; i < currentFunction->HintSize; i++)
      {
      fprintf(fp, "%s", c);
      }
    fprintf(fp, "\"");

    for (i = 0; i < currentFunction->HintSize; i++)
      {
      fprintf(fp, ",temp%i[%d]",MAX_ARGS,i);
      }
    fprintf(fp, ");\n"
            "      }\n"
            "    else\n");
    }

  /* If the pointer was NULL, then build a None and return it */
  fprintf(fp,
          "      {\n"
          "      result = Py_BuildValue((char*)\"\");\n"
          "      }\n");

  return;
}


/* -------------------------------------------------------------------- */
/* This method produces a temporary variable of the required type:
 * "i" is the argument id, to keep the various temps unique, and
 * if "i" == MAX_ARGS, then declare as return type instead of arg type */

static void vtkWrapPython_MakeTempVariable(
  FILE *fp, FunctionInfo *currentFunction, int i)
{
  unsigned int aType = currentFunction->ReturnType;
  char *Id = currentFunction->ReturnClass;
  int aCount = 0;

  if (i < MAX_ARGS)
    {
    aType = currentFunction->ArgTypes[i];
    Id = currentFunction->ArgClasses[i];
    aCount = currentFunction->ArgCounts[i];
    }

  /* handle the function pointer type */
  if (aType == VTK_PARSE_FUNCTION)
    {
    fprintf(fp,
            "  PyObject *temp%i;\n",
            i);
    return;
    }

  /* do nothing for void, unless it is "void *" */
  if (((aType & VTK_PARSE_BASE_TYPE) == VTK_PARSE_VOID) &&
      ((aType & VTK_PARSE_INDIRECT) == 0))
    {
    return;
    }

  /* for const * return types, prepend with const */
  if ((i == MAX_ARGS) && ((aType & VTK_PARSE_CONST) != 0) &&
      (((aType & VTK_PARSE_INDIRECT) == VTK_PARSE_POINTER) ||
       ((aType & VTK_PARSE_INDIRECT) == VTK_PARSE_REF)))
    {
    fprintf(fp,"  const ");
    }
  else
    {
    fprintf(fp,"  ");
    }

  /* for unsigned, prepend with "unsigned" */
  if ((aType & VTK_PARSE_UNSIGNED) != 0)
    {
    fprintf(fp,"unsigned ");
    }

  /* print the type itself */
  switch ((aType & VTK_PARSE_BASE_TYPE) & ~VTK_PARSE_UNSIGNED)
    {
    case VTK_PARSE_FLOAT:       fprintf(fp,"float "); break;
    case VTK_PARSE_DOUBLE:      fprintf(fp,"double "); break;
    case VTK_PARSE_INT:         fprintf(fp,"int "); break;
    case VTK_PARSE_SHORT:       fprintf(fp,"short "); break;
    case VTK_PARSE_LONG:        fprintf(fp,"long "); break;
    case VTK_PARSE_VOID:        fprintf(fp,"void "); break;
    case VTK_PARSE_CHAR:        fprintf(fp,"char "); break;
    case VTK_PARSE_OBJECT:      fprintf(fp,"%s ",Id); break;
    case VTK_PARSE_ID_TYPE:     fprintf(fp,"vtkIdType "); break;
    case VTK_PARSE_LONG_LONG:   fprintf(fp,"long long "); break;
    case VTK_PARSE___INT64:     fprintf(fp,"__int64 "); break;
    case VTK_PARSE_SIGNED_CHAR: fprintf(fp,"signed char "); break;
    case VTK_PARSE_BOOL:        fprintf(fp,"bool "); break;
    case VTK_PARSE_STRING:      fprintf(fp,"%s ",Id); break;
    case VTK_PARSE_UNICODE_STRING: fprintf(fp,"vtkUnicodeString "); break;
    case VTK_PARSE_UNKNOWN:     return;
    }

  /* then print the decorators for ref and pointer, but not for arrays */
  switch (aType & VTK_PARSE_INDIRECT)
    {
    case VTK_PARSE_REF:
      if (((aType & VTK_PARSE_BASE_TYPE) == VTK_PARSE_OBJECT) ||
           (i == MAX_ARGS))
        {
        fprintf(fp, "*"); /* refs are converted to pointers */
        }
      break;
    case VTK_PARSE_POINTER:
      if ((i == MAX_ARGS) ||
          ((aType & VTK_PARSE_BASE_TYPE) == VTK_PARSE_OBJECT) ||
          ((aType & VTK_PARSE_UNQUALIFIED_TYPE) == VTK_PARSE_CHAR_PTR) ||
          ((aType & VTK_PARSE_UNQUALIFIED_TYPE) == VTK_PARSE_VOID_PTR))
        {
        fprintf(fp, "*");
        }
      break;
    case VTK_PARSE_POINTER_REF:
      fprintf(fp, "*&");
      break;
    case VTK_PARSE_POINTER_POINTER:
      fprintf(fp, "**");
      break;
    default:
      break;
    }

  /* handle non-vtkObjectBase object arguments as pointers */
  if ((aType & VTK_PARSE_UNQUALIFIED_TYPE) == VTK_PARSE_OBJECT &&
      i != MAX_ARGS)
    {
    fprintf(fp, "*");
    }

  /* the variable name */
  fprintf(fp,"temp%i",i);

  /* print the array decorators */
  if (((aType & VTK_PARSE_INDIRECT) == VTK_PARSE_POINTER) &&
      (i != MAX_ARGS) &&
      ((aType & VTK_PARSE_BASE_TYPE) != VTK_PARSE_OBJECT) &&
      ((aType & VTK_PARSE_UNQUALIFIED_TYPE) != VTK_PARSE_CHAR_PTR) &&
      ((aType & VTK_PARSE_UNQUALIFIED_TYPE) != VTK_PARSE_VOID_PTR))
    {
    fprintf(fp,"[%i]",aCount);
    }

  /* finish off with a semicolon */
  if (i == MAX_ARGS)
    {
    fprintf(fp, "; // return value\n");
    }
  else
    {
    fprintf(fp, "; // arg %d\n", i);
    }

  /* for "void *", add another temp to hold the size of the argument */
  if (((aType & VTK_PARSE_UNQUALIFIED_TYPE) == VTK_PARSE_VOID_PTR) &&
      (i != MAX_ARGS))
    {
    fprintf(fp,
            "  int size%d;\n",
            i);
    }

  /* for VTK_OBJECT arguments, a PyObject temp is also needed */
  if ((i != MAX_ARGS) &&
      ((aType & VTK_PARSE_BASE_TYPE) == VTK_PARSE_OBJECT))
    {
    fprintf(fp,
            "  PyObject *tempH%d = 0;\n",
            i);
    }

  /* ditto for bool */
  if ((i != MAX_ARGS) &&
      (((aType & VTK_PARSE_UNQUALIFIED_TYPE) == VTK_PARSE_BOOL) ||
       ((aType & VTK_PARSE_UNQUALIFIED_TYPE) == VTK_PARSE_BOOL_REF)))
    {
    fprintf(fp,
            "  PyObject *tempB%d = 0;\n"
            "  int tempI%d;\n",
            i, i);
    }

  /* ditto for string */
  if ((i != MAX_ARGS) &&
       (((aType & VTK_PARSE_UNQUALIFIED_TYPE) == VTK_PARSE_STRING) ||
        ((aType & VTK_PARSE_UNQUALIFIED_TYPE) == VTK_PARSE_STRING_REF)))
    {
    fprintf(fp,
            "  const char *tempC%d = 0;\n",
            i);
    }

  /* ditto for unicode */
  if ((i != MAX_ARGS) &&
      (((aType & VTK_PARSE_UNQUALIFIED_TYPE) == VTK_PARSE_UNICODE_STRING) ||
       ((aType & VTK_PARSE_UNQUALIFIED_TYPE) == VTK_PARSE_UNICODE_STRING_REF)))
    {
    fprintf(fp,
            "  PyObject *tempU%d = 0;\n"
            "  PyObject *tempS%d = 0;\n",
            i, i);
    }

  /* A temporary mini-string for character return value conversion */
  if ((i == MAX_ARGS) &&
      (((aType & VTK_PARSE_UNQUALIFIED_TYPE) == VTK_PARSE_CHAR) ||
       ((aType & VTK_PARSE_UNQUALIFIED_TYPE) == VTK_PARSE_CHAR_REF)))
    {
    fprintf(fp,
            "  char tempA%d[2];\n",
            i);
    }
}


/* -------------------------------------------------------------------- */
/* Convert values into python object and return them within python,
 * using the static var "currentFunction" as the current function */

static void vtkWrapPython_ReturnValue(
  FILE *fp, FunctionInfo *currentFunction)
{
  const char *deref = "";
  const char *acces = ".";

  /* since refs are handled as pointers, they must be deref'd */
  if ((currentFunction->ReturnType & VTK_PARSE_INDIRECT) == VTK_PARSE_REF)
    {
    deref = "*";
    acces = "->";
    }

  /* for void, just return "None" */
  if (((currentFunction->ReturnType & VTK_PARSE_BASE_TYPE) == VTK_PARSE_VOID)
      && ((currentFunction->ReturnType & VTK_PARSE_INDIRECT) == 0))
    {
    fprintf(fp,
            "    Py_INCREF(Py_None);\n"
            "    result = Py_None;\n");
    return;
    }

  /* for other types, handle as required */
  switch (currentFunction->ReturnType & VTK_PARSE_UNQUALIFIED_TYPE)
    {

    /* convert "char *" to a python string, by copying */
    case VTK_PARSE_CHAR_PTR:
      {
      fprintf(fp,
              "    if (temp%i == NULL)\n"
              "      {\n"
              "      Py_INCREF(Py_None);\n"
              "      result = Py_None;\n"
              "      }\n"
              "    else\n"
              "      {\n"
              "      result = PyString_FromString(temp%i);\n"
              "      }\n",
              MAX_ARGS, MAX_ARGS);
      break;
      }

    /* convert VTK objects to Python objects */
    case VTK_PARSE_OBJECT_PTR:
      {
      fprintf(fp,
              "    result = vtkPythonUtil::GetObjectFromPointer((vtkObjectBase *)temp%i);\n",
              MAX_ARGS);
      break;
      }

    /* convert special objects to Python objects */
    case VTK_PARSE_OBJECT_REF:
      {
      fprintf(fp,
              "    result = vtkPythonUtil::GetSpecialObjectFromPointer(temp%i, \"%s\");\n",
              MAX_ARGS, currentFunction->ReturnClass);
      break;
      }

    /* convert special objects to Python objects */
    case VTK_PARSE_OBJECT:
      {
      fprintf(fp,
              "    result = vtkPythonUtil::GetSpecialObjectFromPointer(&temp%i, \"%s\");\n",
              MAX_ARGS, currentFunction->ReturnClass);
      break;
      }

    /* handle functions returning tuples via the hints file */
    case VTK_PARSE_FLOAT_PTR:
    case VTK_PARSE_DOUBLE_PTR:
    case VTK_PARSE_ID_TYPE_PTR:
    case VTK_PARSE_LONG_LONG_PTR:
    case VTK_PARSE___INT64_PTR:
    case VTK_PARSE_SIGNED_CHAR_PTR:
    case VTK_PARSE_INT_PTR:
    case VTK_PARSE_SHORT_PTR:
    case VTK_PARSE_LONG_PTR:
    case VTK_PARSE_BOOL_PTR:
      vtkWrapPython_ReturnHintedValue(fp, currentFunction);
      break;

    /* convert void pointers to None (if NULL) or to a python string,
     * where the string refers to rather than copies the contents */
    case VTK_PARSE_VOID_PTR:
      {
      fprintf(fp,
              "    if (temp%i == NULL)\n"
              "      {\n"
              "      Py_INCREF(Py_None);\n"
              "      result = Py_None;\n"
              "      }\n"
              "    else\n"
              "      {\n"
              "      result = PyString_FromString(vtkPythonUtil::ManglePointer(temp%i,\"void_p\"));\n"
              "      }\n",
              MAX_ARGS, MAX_ARGS);
      break;
      }

    /* handle all basic types by simple conversion */
    case VTK_PARSE_FLOAT:
    case VTK_PARSE_FLOAT_REF:
    case VTK_PARSE_DOUBLE:
    case VTK_PARSE_DOUBLE_REF:
      {
      fprintf(fp,
              "    result = PyFloat_FromDouble(%stemp%i);\n",
              deref, MAX_ARGS);
      break;
      }
#if (VTK_SIZEOF_INT < VTK_SIZEOF_LONG)
    case VTK_PARSE_UNSIGNED_INT:
    case VTK_PARSE_UNSIGNED_INT_REF:
#endif
    case VTK_PARSE_UNSIGNED_CHAR:
    case VTK_PARSE_UNSIGNED_CHAR_REF:
    case VTK_PARSE_UNSIGNED_SHORT:
    case VTK_PARSE_UNSIGNED_SHORT_REF:
    case VTK_PARSE_INT:
    case VTK_PARSE_INT_REF:
    case VTK_PARSE_SHORT:
    case VTK_PARSE_SHORT_REF:
    case VTK_PARSE_LONG:
    case VTK_PARSE_LONG_REF:
    case VTK_PARSE_SIGNED_CHAR:
    case VTK_PARSE_SIGNED_CHAR_REF:
      {
      fprintf(fp,
              "    result = PyInt_FromLong(%stemp%i);\n",
              deref, MAX_ARGS);
      break;
      }

    /* PyBool_FromLong was introduced in Python 2.3,
     * but PyInt_FromLong is a good substitute */
    case VTK_PARSE_BOOL:
    case VTK_PARSE_BOOL_REF:
      {
      fprintf(fp,
              "#if PY_VERSION_HEX >= 0x02030000\n"
              "    result = PyBool_FromLong((long)%stemp%i);\n"
              "#else\n"
              "    result = PyInt_FromLong((long)%stemp%i);\n"
              "#endif\n",
              deref, MAX_ARGS, deref, MAX_ARGS);
      break;
      }

    /* PyLong_FromUnsignedLong() is new to Python 2.2 */
#if (VTK_SIZEOF_INT == VTK_SIZEOF_LONG)
    case VTK_PARSE_UNSIGNED_INT:
    case VTK_PARSE_UNSIGNED_INT_REF:
#endif
    case VTK_PARSE_UNSIGNED_LONG:
    case VTK_PARSE_UNSIGNED_LONG_REF:
      {
      fprintf(fp,
              "#if (PY_VERSION_HEX >= 0x02020000)\n"
              "    if ((long)(%stemp%i) >= 0)\n"
              "      {\n"
              "      result = PyInt_FromLong((long)(%stemp%i));\n"
              "      }\n"
              "    else\n"
              "      {\n"
              "      result = PyLong_FromUnsignedLong(%stemp%i);\n"
              "      }\n"
              "#else\n"
              "    result = PyInt_FromLong((long)(%stemp%i));\n"
              "#endif\n",
              deref, MAX_ARGS, deref, MAX_ARGS, deref, MAX_ARGS,
              deref, MAX_ARGS);
      break;
      }

    /* Support for vtkIdType depends on config and capabilities */
#if defined(VTK_USE_64BIT_IDS) && defined(PY_LONG_LONG) && (VTK_SIZEOF_LONG != 8)
    case VTK_PARSE_ID_TYPE:
    case VTK_PARSE_ID_TYPE_REF:
      {
      fprintf(fp,
              "    result = PyLong_FromLongLong(%stemp%i);\n",
              deref, MAX_ARGS);
      break;
      }
    case VTK_PARSE_UNSIGNED_ID_TYPE:
    case VTK_PARSE_UNSIGNED_ID_TYPE_REF:
      {
      fprintf(fp,
              "    result = PyLong_FromUnsignedLongLong(%stemp%i);\n",
              deref, MAX_ARGS);
      break;
      }
#else
    case VTK_PARSE_ID_TYPE:
    case VTK_PARSE_ID_TYPE_REF:
      {
      fprintf(fp,
              "    result = PyInt_FromLong((long)%stemp%i);\n",
              deref, MAX_ARGS);
      break;
      }
    case VTK_PARSE_UNSIGNED_ID_TYPE:
    case VTK_PARSE_UNSIGNED_ID_TYPE_REF:
      {
      fprintf(fp,
              "#if (PY_VERSION_HEX >= 0x02020000)\n"
              "    if ((long)(%stemp%i) >= 0)\n"
              "      {\n"
              "      result = PyInt_FromLong((long)(%stemp%i));\n"
              "      }\n"
              "    else\n"
              "      {\n"
              "      result = PyLong_FromUnsignedLong(%stemp%i);\n"
              "      }\n"
              "#else\n"
              "    result = PyInt_FromLong((long)(%stemp%i));\n"
              "#endif\n",
              deref, MAX_ARGS, deref, MAX_ARGS, deref, MAX_ARGS,
              deref, MAX_ARGS);
      break;
      }
#endif

    /* support for "long long" depends on config and capabilities */
#if defined(VTK_TYPE_USE_LONG_LONG) || defined(VTK_TYPE_USE___INT64)
# if defined(PY_LONG_LONG) && (VTK_SIZEOF_LONG != 8)
    case VTK_PARSE_LONG_LONG:
    case VTK_PARSE_LONG_LONG_REF:
    case VTK_PARSE___INT64:
    case VTK_PARSE___INT64_REF:
      {
      fprintf(fp,
              "    result = PyLong_FromLongLong(%stemp%i);\n",
              deref, MAX_ARGS);
      break;
      }
    case VTK_PARSE_UNSIGNED_LONG_LONG:
    case VTK_PARSE_UNSIGNED_LONG_LONG_REF:
    case VTK_PARSE_UNSIGNED___INT64:
    case VTK_PARSE_UNSIGNED___INT64_REF:
      {
      fprintf(fp,
              "    result = PyLong_FromUnsignedLongLong(%stemp%i);\n",
              deref, MAX_ARGS);
      break;
      }
# else
    case VTK_PARSE_LONG_LONG:
    case VTK_PARSE_LONG_LONG_REF:
    case VTK_PARSE___INT64:
    case VTK_PARSE___INT64_REF:
      {
      fprintf(fp,
              "    result = PyLong_FromLong(%stemp%i);\n",
              deref, MAX_ARGS);
      break;
      }
    case VTK_PARSE_UNSIGNED_LONG_LONG:
    case VTK_PARSE_UNSIGNED_LONG_LONG_REF:
    case VTK_PARSE_UNSIGNED___INT64:
    case VTK_PARSE_UNSIGNED___INT64_REF:
      {
      fprintf(fp,
              "#if (PY_VERSION_HEX >= 0x02020000)\n"
              "    if ((long)(%stemp%i) >= 0)\n"
              "      {\n"
              "      result = PyInt_FromLong((long)(%stemp%i));\n"
              "      }\n"
              "    else\n"
              "      {\n"
              "      result = PyLong_FromUnsignedLong(%stemp%i);\n"
              "      }\n"
              "#else\n"
              "    result = PyInt_FromLong((long)(%stemp%i));\n"
              "#endif\n",
              deref, MAX_ARGS, deref, MAX_ARGS, deref, MAX_ARGS,
              deref, MAX_ARGS);
      break;
      }
# endif
#endif

    /* return a char as a string of unit length */
    case VTK_PARSE_CHAR:
    case VTK_PARSE_CHAR_REF:
      {
      fprintf(fp,
              "    tempA%i[0] = %stemp%i;\n"
              "    tempA%i[1] = \'\\0\';\n"
              "    result = PyString_FromStringAndSize(tempA%i,1);\n",
              MAX_ARGS, deref, MAX_ARGS, MAX_ARGS, MAX_ARGS);
      break;
      }

    /* return a string */
    case VTK_PARSE_STRING:
    case VTK_PARSE_STRING_REF:
      {
      fprintf(fp,
              "    result = PyString_FromString(temp%i%sc_str());\n",
              MAX_ARGS, acces);
      break;
      }

    /* return a vtkUnicodeString, using utf8 intermediate because python
     * can be configured for either 32-bit or 16-bit unicode and it's
     * tricky to test both, so utf8 is a safe alternative */
    case VTK_PARSE_UNICODE_STRING:
    case VTK_PARSE_UNICODE_STRING_REF:
      {
      fprintf(fp,
              "      {\n"
              "      const char *s = temp%i%sutf8_str();\n"
              "      result = PyUnicode_DecodeUTF8(s, strlen(s), \"strict\");\n"
              "      }\n",
              MAX_ARGS, acces);
      break;
      }
    }
}


/* -------------------------------------------------------------------- */
/* Create a format string for PyArg_ParseTuple(), see the python
 * documentation for PyArg_ParseTuple() for more information.
 * Briefly, "O" is for objects and "d", "f", "i" etc are basic types.
 *
 * If any new format characters are added here, they must also be
 * added to vtkPythonUtil::CheckArg() in vtkPythonUtil.cxx
 */

static char *vtkWrapPython_FormatString(FunctionInfo *currentFunction)
{
  static char result[1024];
  size_t currPos = 0;
  unsigned int argtype;
  int i, j;
  char typeChar;

  if (currentFunction->ArgTypes[0] == VTK_PARSE_FUNCTION)
    {
    result[currPos++] = 'O';
    result[currPos] = '\0';
    return result;
    }

  for (i = 0; i < currentFunction->NumberOfArguments; i++)
    {
    argtype = (currentFunction->ArgTypes[i] & VTK_PARSE_UNQUALIFIED_TYPE);

    switch ( (argtype & VTK_PARSE_BASE_TYPE) )
      {
      case VTK_PARSE_OBJECT:
        result[currPos++] = 'O';
        break;
      case VTK_PARSE_FLOAT:
        result[currPos++] = 'f';
        break;
      case VTK_PARSE_DOUBLE:
        result[currPos++] = 'd';
        break;
      case VTK_PARSE_UNSIGNED_INT:
      case VTK_PARSE_INT:
        result[currPos++] = 'i';
        break;
      case VTK_PARSE_UNSIGNED_SHORT:
      case VTK_PARSE_SHORT:
        result[currPos++] = 'h';
        break;
      case VTK_PARSE_UNSIGNED_LONG:
      case VTK_PARSE_LONG:
        result[currPos++] = 'l';
        break;
      case VTK_PARSE_UNSIGNED_ID_TYPE:
      case VTK_PARSE_ID_TYPE:
#ifdef VTK_USE_64BIT_IDS
#ifdef PY_LONG_LONG
        result[currPos++] = 'L';
#else
        result[currPos++] = 'l';
#endif
#else
        result[currPos++] = 'i';
#endif
        break;
#ifdef PY_LONG_LONG
      case VTK_PARSE_UNSIGNED_LONG_LONG:
      case VTK_PARSE_UNSIGNED___INT64:
      case VTK_PARSE_LONG_LONG:
      case VTK_PARSE___INT64:
        result[currPos++] = 'L';
        break;
#else
      case VTK_PARSE_UNSIGNED_LONG_LONG:
      case VTK_PARSE_UNSIGNED___INT64:
      case VTK_PARSE_LONG_LONG:
      case VTK_PARSE___INT64:
        result[currPos++] = 'l';
        break;
#endif
      case VTK_PARSE_SIGNED_CHAR:
        result[currPos++] = 'b';
        break;
      case VTK_PARSE_CHAR:
        result[currPos++] = 'c';
        break;
      case VTK_PARSE_UNSIGNED_CHAR:
        result[currPos++] = 'b';
        break;
      case VTK_PARSE_BOOL:
        result[currPos++] = 'O';
        break;
      case VTK_PARSE_STRING:
        result[currPos++] = 's';
        break;
      case VTK_PARSE_UNICODE_STRING:
        result[currPos++] = 'O';
        break;
      default:
        result[currPos++] = 'O';
        break;
      }

    if ((argtype & VTK_PARSE_INDIRECT) == VTK_PARSE_POINTER &&
        argtype != VTK_PARSE_OBJECT_PTR)
      {
      /* back up and replace the char */
      --currPos;

      if (argtype == VTK_PARSE_CHAR_PTR)
        {
        /* string with "None" equivalent to "NULL" */
        result[currPos++] = 'z';
        }
      else if (argtype == VTK_PARSE_VOID_PTR)
        {
        /* buffer type, None not allowed to avoid passing NULL pointer */
        result[currPos++] = 's';
        result[currPos++] = '#';
        }
      else
        {
        typeChar = result[currPos];

        if (argtype == VTK_PARSE_BOOL_PTR)
          {
          /* A tuple of ints stands in for a tuple of bools,
           * since python bool is a subclass of python int */
          typeChar = 'i';
          }

        result[currPos++] = '(';
        for (j = 0; j < currentFunction->ArgCounts[i]; j++)
          {
          result[currPos++] = typeChar;
          }
        result[currPos++] = ')';
        }
      }
    }

  result[currPos++] = '\0';
  return result;
}

/* -------------------------------------------------------------------- */
/* Create a string to describe the signature of a method.
 * If isvtkobject is set the string will start with an ampersand.
 * Following the optional space will be a ParseTuple format string,
 * followed by the names of any VTK classes required.  The optional
 * ampersand indicates that methods like vtkClass.Method(self, arg1,...)
 * are possible, and the ampersand is a placeholder for "self". */

static char *vtkWrapPython_ArgCheckString(
  int isvtkobjmethod, FunctionInfo *currentFunction)
{
  static char result[1024];
  size_t currPos = 0;
  unsigned int argtype;
  int i;

  if (isvtkobjmethod)
    {
    result[currPos++] = '@';
    }

  strcpy(&result[currPos], vtkWrapPython_FormatString(currentFunction));
  currPos = strlen(result);

  if (currentFunction->ArgTypes[0] == VTK_PARSE_FUNCTION)
    {
    strcpy(&result[currPos], " func");
    return result;
    }

  for (i = 0; i < currentFunction->NumberOfArguments; i++)
    {
    argtype = (currentFunction->ArgTypes[i] & VTK_PARSE_UNQUALIFIED_TYPE);

    if (argtype == VTK_PARSE_BOOL ||
        argtype == VTK_PARSE_BOOL_REF)
      {
      strcpy(&result[currPos], " bool");
      currPos += 5;
      }

    if (argtype == VTK_PARSE_UNICODE_STRING ||
        argtype == VTK_PARSE_UNICODE_STRING_REF)
      {
      strcpy(&result[currPos], " unicode");
      currPos += 8;
      }

    if (argtype == VTK_PARSE_OBJECT_REF ||
        argtype == VTK_PARSE_OBJECT_PTR ||
        argtype == VTK_PARSE_OBJECT)
      {
      result[currPos++] = ' ';
      if (argtype == VTK_PARSE_OBJECT_REF)
        {
        result[currPos++] = '&';
        }
      else if (argtype == VTK_PARSE_OBJECT_PTR)
        {
        result[currPos++] = '*';
        }
      strcpy(&result[currPos], currentFunction->ArgClasses[i]);
      currPos += strlen(currentFunction->ArgClasses[i]);
      }
    }

  return result;
}

/* -------------------------------------------------------------------- */
/* For the purpose of the python docstrings, convert special characters
 * in a string into their escape codes, so that the string can be quoted
 * in a source file (the specified maxlen must be at least 32 chars)*/

static const char *vtkWrapPython_QuoteString(
  const char *comment, size_t maxlen)
{
  static char *result = 0;
  static size_t oldmaxlen = 0;
  size_t i, j, n;

  if (maxlen > oldmaxlen)
    {
    if (result)
      {
      free(result);
      }
    result = (char *)malloc((size_t)(maxlen+1));
    oldmaxlen = maxlen;
    }

  if (comment == NULL)
    {
    return "";
    }

  j = 0;

  n = strlen(comment);
  for (i = 0; i < n; i++)
    {
    if (comment[i] == '\"')
      {
      strcpy(&result[j],"\\\"");
      j += 2;
      }
    else if (comment[i] == '\\')
      {
      strcpy(&result[j],"\\\\");
      j += 2;
      }
    else if (comment[i] == '\n')
      {
      strcpy(&result[j],"\\n");
      j += 2;
      }
    else if (isprint(comment[i]))
      {
      result[j] = comment[i];
      j++;
      }
    else
      {
      sprintf(&result[j],"\\%3.3o",comment[i]);
      j += 4;
      }
    if (j >= maxlen - 21)
      {
      sprintf(&result[j]," ...\\n [Truncated]\\n");
      j += (int)strlen(" ...\\n [Truncated]\\n");
      break;
      }
    }
  result[j] = '\0';

  return result;
}

/* -------------------------------------------------------------------- */
/* The method signatures are for the python docstrings. */

static void vtkWrapPython_AddToSignature(
  char *sig, const char *add, size_t *i)
{
  size_t j = 0;
  char *cp = &sig[*i];
  for (; add[j] != '\0'; j++)
    {
    // stop at the semicolon, there's often garbage after it
    if ((cp[j] = add[j]) == ';')
      {
      cp[++j] = '\0';
      break;
      }
    }
  *i += j;
}

/* -------------------------------------------------------------------- */
/* Create a signature for the python version of a method. */

static const char *vtkWrapPython_PythonSignature(
  FunctionInfo *currentFunction)
{
  static char result[1024];
  size_t currPos = 0;
  unsigned int argtype;
  int i, j;

  /* print out the name of the method */
  vtkWrapPython_AddToSignature(result,"V.",&currPos);
  vtkWrapPython_AddToSignature(result,currentFunction->Name,&currPos);

  /* print the arg list */
  vtkWrapPython_AddToSignature(result,"(",&currPos);

  for (i = 0; i < currentFunction->NumberOfArguments; i++)
    {
    if (currentFunction->ArgTypes[i] == VTK_PARSE_FUNCTION)
      {
      vtkWrapPython_AddToSignature(result,"function",&currPos);
      }

    argtype = (currentFunction->ArgTypes[i] & VTK_PARSE_UNQUALIFIED_TYPE);

    if (i != 0)
      {
      vtkWrapPython_AddToSignature(result,", ",&currPos);
      }

    switch (argtype)
      {
      case VTK_PARSE_FLOAT_PTR:
      case VTK_PARSE_DOUBLE_PTR:
        vtkWrapPython_AddToSignature(result,"(",&currPos);
        for (j = 0; j < currentFunction->ArgCounts[i]; j++)
          {
          if (j != 0)
            {
            vtkWrapPython_AddToSignature(result,", ",&currPos);
            }
          vtkWrapPython_AddToSignature(result,"float",&currPos);
          }
        vtkWrapPython_AddToSignature(result,")",&currPos);
        break;
      case VTK_PARSE_INT_PTR:
        vtkWrapPython_AddToSignature(result,"(",&currPos);
        for (j = 0; j < currentFunction->ArgCounts[i]; j++)
          {
          if (j != 0)
            {
            vtkWrapPython_AddToSignature(result,", ",&currPos);
            }
          vtkWrapPython_AddToSignature(result,"int",&currPos);
          }
        vtkWrapPython_AddToSignature(result,")",&currPos);
        break;
      case VTK_PARSE_ID_TYPE_PTR:
        vtkWrapPython_AddToSignature(result,"(",&currPos);
        for (j = 0; j < currentFunction->ArgCounts[i]; j++)
          {
          if (j != 0)
            {
            vtkWrapPython_AddToSignature(result,", ",&currPos);
            }
#if defined(VTK_USE_64BIT_IDS) && (VTK_SIZEOF_LONG != VTK_SIZEOF_ID_TYPE)
          vtkWrapPython_AddToSignature(result,"long",&currPos);
#else
          vtkWrapPython_AddToSignature(result,"int",&currPos);
#endif
          }
        vtkWrapPython_AddToSignature(result,")",&currPos);
        break;
      case VTK_PARSE_LONG_LONG_PTR:
      case VTK_PARSE___INT64_PTR:
        vtkWrapPython_AddToSignature(result,"(",&currPos);
        for (j = 0; j < currentFunction->ArgCounts[i]; j++)
          {
          if (j != 0)
            {
            vtkWrapPython_AddToSignature(result,", ",&currPos);
            }
          vtkWrapPython_AddToSignature(result,"long",&currPos);
          }
        vtkWrapPython_AddToSignature(result,")",&currPos);
        break;
      case VTK_PARSE_OBJECT_REF:
      case VTK_PARSE_OBJECT_PTR:
      case VTK_PARSE_OBJECT:
        vtkWrapPython_AddToSignature(result,currentFunction->ArgClasses[i],
                                    &currPos);
        break;
      case VTK_PARSE_VOID_PTR:
      case VTK_PARSE_CHAR_PTR:
        vtkWrapPython_AddToSignature(result,"string",&currPos);
        break;
      case VTK_PARSE_FLOAT:
      case VTK_PARSE_DOUBLE:
        vtkWrapPython_AddToSignature(result,"float",&currPos);
        break;
      case VTK_PARSE_SIGNED_CHAR:
      case VTK_PARSE_ID_TYPE:
      case VTK_PARSE_UNSIGNED_LONG_LONG:
      case VTK_PARSE_LONG_LONG:
      case VTK_PARSE_UNSIGNED___INT64:
      case VTK_PARSE___INT64:
      case VTK_PARSE_UNSIGNED_INT:
      case VTK_PARSE_INT:
      case VTK_PARSE_UNSIGNED_SHORT:
      case VTK_PARSE_SHORT:
      case VTK_PARSE_UNSIGNED_LONG:
      case VTK_PARSE_LONG:
        vtkWrapPython_AddToSignature(result,"int",&currPos);
        break;
      case VTK_PARSE_CHAR:
        vtkWrapPython_AddToSignature(result,"char",&currPos);
        break;
      case VTK_PARSE_UNSIGNED_CHAR:
        vtkWrapPython_AddToSignature(result,"int",&currPos);
        break;
      case VTK_PARSE_BOOL:
        vtkWrapPython_AddToSignature(result,"bool",&currPos);
        break;
      case VTK_PARSE_STRING:
        vtkWrapPython_AddToSignature(result,"string",&currPos);
        break;
      case VTK_PARSE_UNICODE_STRING:
        vtkWrapPython_AddToSignature(result,"unicode",&currPos);
        break;
      }
    }

  vtkWrapPython_AddToSignature(result,")",&currPos);

  /* if this is a void method, we are finished */
  /* otherwise, print "->" and the return type */
  if (!((currentFunction->ReturnType & VTK_PARSE_BASE_TYPE) == VTK_PARSE_VOID)
      || (currentFunction->ReturnType & VTK_PARSE_INDIRECT))
    {
    vtkWrapPython_AddToSignature(result," -> ",&currPos);

    switch (currentFunction->ReturnType & VTK_PARSE_UNQUALIFIED_TYPE)
      {
      case VTK_PARSE_VOID_PTR:
      case VTK_PARSE_CHAR_PTR:
        vtkWrapPython_AddToSignature(result,"string",&currPos);
        break;
      case VTK_PARSE_OBJECT_REF:
      case VTK_PARSE_OBJECT_PTR:
      case VTK_PARSE_OBJECT:
        vtkWrapPython_AddToSignature(result,currentFunction->ReturnClass,
                                    &currPos);
        break;
      case VTK_PARSE_FLOAT_PTR:
      case VTK_PARSE_DOUBLE_PTR:
        vtkWrapPython_AddToSignature(result,"(",&currPos);
        for (j = 0; j < currentFunction->HintSize; j++)
          {
          if (j != 0)
            {
            vtkWrapPython_AddToSignature(result,", ",&currPos);
            }
          vtkWrapPython_AddToSignature(result,"float",&currPos);
          }
        vtkWrapPython_AddToSignature(result,")",&currPos);
        break;
      case VTK_PARSE_INT_PTR:
        vtkWrapPython_AddToSignature(result,"(",&currPos);
        for (j = 0; j < currentFunction->HintSize; j++)
          {
          if (j != 0)
            {
            vtkWrapPython_AddToSignature(result,", ",&currPos);
            }
          vtkWrapPython_AddToSignature(result,"int",&currPos);
          }
        vtkWrapPython_AddToSignature(result,")",&currPos);
        break;
      case VTK_PARSE_ID_TYPE_PTR:
        vtkWrapPython_AddToSignature(result,"(",&currPos);
        for (j = 0; j < currentFunction->HintSize; j++)
          {
          if (j != 0)
            {
            vtkWrapPython_AddToSignature(result,", ",&currPos);
            }
#if defined(VTK_USE_64BIT_IDS) && (VTK_SIZEOF_LONG != VTK_SIZEOF_ID_TYPE)
          vtkWrapPython_AddToSignature(result,"long",&currPos);
#else
          vtkWrapPython_AddToSignature(result,"int",&currPos);
#endif
          }
        vtkWrapPython_AddToSignature(result,")",&currPos);
        break;
      case VTK_PARSE_LONG_LONG_PTR:
      case VTK_PARSE___INT64_PTR:
        vtkWrapPython_AddToSignature(result,"(",&currPos);
        for (j = 0; j < currentFunction->HintSize; j++)
          {
          if (j != 0)
            {
            vtkWrapPython_AddToSignature(result,", ",&currPos);
            }
          vtkWrapPython_AddToSignature(result,"long",&currPos);
          }
        vtkWrapPython_AddToSignature(result,")",&currPos);
        break;
      case VTK_PARSE_FLOAT:
      case VTK_PARSE_DOUBLE:
        vtkWrapPython_AddToSignature(result,"float",&currPos);
        break;
      case VTK_PARSE_ID_TYPE:
      case VTK_PARSE_LONG_LONG:
      case VTK_PARSE___INT64:
      case VTK_PARSE_SIGNED_CHAR:
      case VTK_PARSE_UNSIGNED_LONG_LONG:
      case VTK_PARSE_UNSIGNED___INT64:
      case VTK_PARSE_UNSIGNED_CHAR:
      case VTK_PARSE_UNSIGNED_INT:
      case VTK_PARSE_UNSIGNED_SHORT:
      case VTK_PARSE_UNSIGNED_LONG:
      case VTK_PARSE_INT:
      case VTK_PARSE_SHORT:
      case VTK_PARSE_LONG:
        vtkWrapPython_AddToSignature(result,"int",&currPos);
        break;
      case VTK_PARSE_CHAR:
        vtkWrapPython_AddToSignature(result,"char",&currPos);
        break;
      case VTK_PARSE_BOOL:
        vtkWrapPython_AddToSignature(result,"bool",&currPos);
        break;
      case VTK_PARSE_STRING:
        vtkWrapPython_AddToSignature(result,"string",&currPos);
        break;
      case VTK_PARSE_UNICODE_STRING:
        vtkWrapPython_AddToSignature(result,"unicode",&currPos);
        break;

      }
    }

  if (currentFunction->Signature)
    {
    vtkWrapPython_AddToSignature(result,"\nC++: ",&currPos);
    vtkWrapPython_AddToSignature(result,currentFunction->Signature,&currPos);
    }

  return result;
}

/* -------------------------------------------------------------------- */
/* Format a signature to a 70 char linewidth */
const char *vtkWrapPython_FormatSignature(
  const char *signature, size_t width)
{
  static char text[2048];
  size_t i, j, k, l, m;
  const char *cp = signature;
  char delim;

  if (signature == 0)
    {
    text[0] = '\0';
    return text;
    }

  i = 0;
  j = 0;
  k = 0;

  while (cp[i] != '\0')
    {
    while (k-j < width && cp[i] != '\n' && cp[i] != '\0')
      {
      /* escape quotes */
      if (cp[i] == '\"' || cp[i] == '\'')
        {
        delim = cp[i];
        text[k++] = '\\';
        text[k++] = cp[i++];
        while (cp[i] != delim && cp[i] != '\0')
          {
          if (cp[i] == '\\')
            {
            text[k++] = '\\';
            }
          text[k++] = cp[i++];
          }
        if (cp[i] == delim)
          {
          text[k++] = '\\';
          text[k++] = cp[i++];
          }
        }
      /* remove items that trail the closing parenthesis */
      else if (cp[i] == ')')
        {
        text[k++] = cp[i++];
        if (strncmp(&cp[i], " const", 6) == 0)
          {
          i += 6;
          }
        if (strncmp(&cp[i], " = 0", 4) == 0)
          {
          i += 4;
          }
        if (cp[i] == ';')
          {
          i++;
          }
        }
      /* anything else */
      else
        {
        text[k++] = cp[i++];
        }
      }

    /* break the line (try to break after a comma) */
    if (cp[i] != '\n' && cp[i] != '\0')
      {
      l = k;

      while (l > j && text[l-1] != '\n' && text[l-1] != ',' &&
        text[l-1] != '(' && text[l-1] != ')')
        {
        /* treat each string as a unit */
        if (l > 4 && (text[l-1] == '\'' || text[l-1] == '\"'))
          {
          delim = text[l-1];
          l -= 2;
          while (l > 3 && (text[l-1] != delim || text[l-3] == '\\'))
            {
            l--;
            if (text[l-1] == '\\')
              {
              l--;
              }
            }
          l -= 2;
          }
        else
          {
          l--;
          }
        }

      /* if none of these chars was found, split is impossible */
      if (text[l-1] != ',' && text[l-1] != '(' &&
          text[l-1] != ')' && text[l-1] != '\n')
        {
        j++;
        }

      else
        {
        if (k > l)
          {
          m = 0;
          while (m < 6 && text[l+m] == ' ')
            {
            m++;
            }
          memmove(&text[l+6-m], &text[l], k-l);
          k += 6-m;
          }
        else
          {
          k += 6;
          }
        text[l++] = '\\'; text[l++] = 'n';
        j = l;
        text[l++] = ' '; text[l++] = ' ';
        text[l++] = ' '; text[l++] = ' ';
        }
      }
    /* reached end of line: do next signature */
    else
      {
      while (k > 0 && (text[k-1] == ' ' || text[k-1] == '\r')) { k--; }
      if (cp[i] != '\0')
        {
        i++;
        text[k++] = '\\'; text[k++] = 'n';
        }
      j = k;
      }
    }

  while (k > 0 && (text[k-1] == ' ' || text[k-1] == '\r')) { k--; }
  text[k] = '\0';

  return text;
}

/* -------------------------------------------------------------------- */
/* Format a comment to a 70 char linewidth, in several steps:
 * 1) remove html tags, convert <p> and <br> into breaks
 * 2) remove doxygen tags like \em
 * 3) remove extra whitespace (except paragraph breaks)
 * 4) re-break the lines
 */

const char *vtkWrapPython_FormatComment(
  const char *comment, size_t width)
{
  static size_t size = 0;
  static char *text = 0;
  const char *cp;
  size_t i, j, k, l;
  size_t indent = 0;
  int nojoin = 0;
  int start;

  if (text == 0)
    {
    size = 2048;
    text = (char *)malloc(size);
    }

  if (comment == 0)
    {
    text[0] = '\0';
    return text;
    }

  i = 0; j = 0; k = 0; l = 0;
  start = 1;
  cp = comment;

  /* skip any leading whitespace */
  while (cp[i] == '\n' || cp[i] == '\r' ||
         cp[i] == '\t' || cp[i] == ' ') { i++; }

  while (cp[i] != '\0')
    {
    /* Add characters until the output line is complete */
    while (cp[i] != '\0' && k-j < width)
      {
      /* if the end of the line was found, see how next line begins */
      if (start)
        {
        /* eat the leading space */
        if (cp[i] == ' ') { i++; }

        /* skip ahead to find any interesting first characters */
        l = i;
        while (cp[l] == ' ' || cp[l] == '\t' || cp[l] == '\r') { l++; }

        /* check for new section */
        if (cp[l] == '.' && strncmp(&cp[l], ".SECTION", 8) == 0)
          {
          while (k > 0 && text[k-1] == '\n') { k--; }
          if (k > 0) { text[k++] = '\n'; text[k++] = '\n'; }
          i = l+8;
          while (cp[i] == '\r' || cp[i] == '\t' || cp[i] == ' ') { i++; }
          while (cp[i] != '\n' && cp[i] != '\0') { text[k++] = cp[i++]; }
          while (k > 0 && (text[k-1] == ' ' || text[k-1] == '\r' ||
                           text[k-1] == '\t')) { k--; }
          if (k > 0 && text[k-1] != ':') { text[k++] = ':'; }
          text[k++] = '\n'; text[k++] = '\n';
          j = k;
          indent = 0;
          if (cp[i] == '\n') { i++; }
          start = 1;
          continue;
          }

        /* handle doxygen tags that appear at start of line */
        if (cp[l] == '\\' || cp[l] == '@')
          {
          if (strncmp(&cp[l+1], "brief", 5) == 0 ||
              strncmp(&cp[l+1], "short", 5) == 0 ||
              strncmp(&cp[l+1], "pre", 3) == 0 ||
              strncmp(&cp[l+1], "post", 4) == 0 ||
              strncmp(&cp[l+1], "param", 5) == 0 ||
              strncmp(&cp[l+1], "tparam", 6) == 0 ||
              strncmp(&cp[l+1], "cmdparam", 8) == 0 ||
              strncmp(&cp[l+1], "exception", 9) == 0 ||
              strncmp(&cp[l+1], "return", 6) == 0 ||
              strncmp(&cp[l+1], "li", 2) == 0)
            {
            nojoin = 2;
            indent = 4;
            if (k > 0 && text[k-1] != '\n') { text[k++] = '\n'; }
            j = k;
            i = l;
            }
          }

        /* handle bullets and numbering */
        else if (cp[l] == '-' || cp[l] == '*' || cp[l] == '#' ||
                 (cp[l] >= '0' && cp[l] <= '9' &&
                  (cp[l+1] == ')' || cp[l+1] == '.') && cp[l+2] == ' '))
          {
          indent = 0;
          while (indent < 3 && cp[l+indent] != ' ') { indent++; }
          indent++;
          if (k > 0 && text[k-1] != '\n') { text[k++] = '\n'; }
          j = k;
          i = l;
          }

        /* keep paragraph breaks */
        else if (cp[l] == '\n')
          {
          i = l+1;
          while (k > 0 && text[k-1] == '\n') { k--; }
          if (k > 0) { text[k++] = '\n'; text[k++] = '\n'; }
          nojoin = 0;
          indent = 0;
          j = k;
          start = 1;
          continue;
          }

        /* add newline if nojoin is not set */
        else if (nojoin ||
                (cp[i] == ' ' && !indent))
          {
          if (nojoin == 2)
            {
            nojoin = 0;
            indent = 0;
            }
          text[k++] = '\n';
          j = k;
          }

        /* do line joining */
        else if (k > 0 && text[k-1] != '\n')
          {
          i = l;
          text[k++] = ' ';
          }
        }

      /* handle quotes */
      if (cp[i] == '\"')
        {
        size_t q = i;
        size_t r = k;

        text[k++] = cp[i++];
        while (cp[i] != '\"' && cp[i] != '\r' &&
               cp[i] != '\n' && cp[i] != '\0')
          {
          text[k++] = cp[i++];
          }
        if (cp[i] != '\"')
          {
          i = q;
          k = r;
          }
        }
      else if (cp[i] == '\'')
        {
        size_t q = i;
        size_t r = k;

        text[k++] = cp[i++];
        while (cp[i] != '\'' && cp[i] != '\r' &&
               cp[i] != '\n' && cp[i] != '\0')
          {
          text[k++] = cp[i++];
          }
        if (cp[i] != '\'')
          {
          i = q;
          k = r;
          }
        }

      /* handle simple html tags */
      else if (cp[i] == '<')
        {
        l = i+1;
        if (cp[l] == '/') { l++; }
        while ((cp[l] >= 'a' && cp[l] <= 'z') ||
               (cp[l] >= 'a' && cp[l] <= 'z')) { l++; }
        if (cp[l] == '>')
          {
          if (cp[i+1] == 'p' || cp[i+1] == 'P' ||
              (cp[i+1] == 'b' && cp[i+2] == 'r') ||
              (cp[i+1] == 'B' && cp[i+2] == 'R'))
            {
            while (k > 0 && (text[k-1] == ' ' || text[k-1] == '\n')) { k--; }
            text[k++] = '\n'; text[k++] = '\n';
            j = k;
            indent = 0;
            }
          i = l+1;
          while (cp[i] == '\r' || cp[i] == '\t' || cp[i] == ' ') { i++; }
          }
        }
      else if (cp[i] == '\\' || cp[i] == '@')
        {
        /* handle simple doxygen tags */
        if (strncmp(&cp[i+1], "em ", 3) == 0)
          {
          i += 4;
          }
        else if (strncmp(&cp[i+1], "a ", 2) == 0 ||
                 strncmp(&cp[i+1], "e ", 2) == 0 ||
                 strncmp(&cp[i+1], "c ", 2) == 0 ||
                 strncmp(&cp[i+1], "b ", 2) == 0 ||
                 strncmp(&cp[i+1], "p ", 2) == 0 ||
                 strncmp(&cp[i+1], "f$", 2) == 0 ||
                 strncmp(&cp[i+1], "f[", 2) == 0 ||
                 strncmp(&cp[i+1], "f]", 2) == 0)
          {
          if (i > 0 && cp[i-1] != ' ') { text[k++] = ' '; }
          if (cp[i+1] == 'f')
            {
            if (cp[i+2] == '$') { text[k++] = '$'; }
            else { text[k++] = '\\'; text[k++] = cp[i+2]; }
            }
          i += 3;
          }
        else if (cp[i+1] == '&' || cp[i+1] == '$' || cp[i+1] == '#' ||
                 cp[i+1] == '<' || cp[i+1] == '>' || cp[i+1] == '%' ||
                 cp[i+1] == '@' || cp[i+1] == '\\' || cp[i+1] == '\"')
          {
          i++;
          }
        else if (cp[i+1] == 'n')
          {
          while (k > 0 && (text[k-1] == ' ' || text[k-1] == '\n')) { k--; }
          text[k++] = '\n'; text[k++] = '\n';
          indent = 0;
          i += 2;
          j = k;
          }
        else if (strncmp(&cp[i+1], "code", 4) == 0)
          {
          nojoin = 1;
          i += 5;
          while (cp[i] == ' ' || cp[i] == '\r' ||
                 cp[i] == '\t' || cp[i] == '\n') { i++; }
          }
        else if (strncmp(&cp[i+1], "endcode", 7) == 0)
          {
          nojoin = 0;
          i += 8;
          l = i;
          while (cp[l] == ' ' || cp[l] == '\t' || cp[l] == '\r') { l++; }
          if (cp[l] == '\n')
            {
            i = l;
            text[k++] = '\n';
            j = k;
            }
          }
        else if (strncmp(&cp[i+1], "verbatim", 8) == 0)
          {
          i += 9;
          while (cp[i] != '\0' && ((cp[i] != '@' && cp[i] != '\\') ||
                 strncmp(&cp[i+1], "endverbatim", 11) != 0))
            {
            if (cp[i] != '\r') { text[k++] = cp[i]; };
            if (cp[i] == '\n') { j = k; }
            i++;
            if (size - k < 1024)
              {
              size *= 2;
              text = (char *)realloc(text, size);
              }
            }
          if (cp[i] != '\0') { i += 12; }
          }
        }

      /* search for newline */
      start = 0;
      l = i;
      while (cp[l] == ' ' || cp[l] == '\t' || cp[l] == '\r') { l++; }
      if (cp[l] == '\n')
        {
        i = l+1;
        start = 1;
        }

      /* append */
      else if (cp[i] != '\0')
        {
        text[k++] = cp[i++];
        }

      /* resize if there is not much space left */
      if (size - k < 1024)
        {
        size *= 2;
        text = (char *)realloc(text, size);
        }

      } /* while (cp[i] != '\0' && k-j < width) */

    if (cp[i] == '\0') { break; }

    /* try to break the line at a word */
    l = k;
    while (l > 0 && text[l-1] != ' ' && text[l-1] != '\n')
      {
      l--;
      }
    if (l > 0 && text[l-1] != '\n' && l-j > indent)
      {
      /* replace space with newline */
      text[l-1] = '\n';
      j = l;
      if (k > l && indent > 0)
        {
        memmove(&text[l+indent], &text[l], k-l);
        memset(&text[l], ' ', indent);
        k += indent;
        }
      }
    else
      {
      /* long word, just split the word */
      text[k++] = '\n';
      j = k;
      if (indent > 0)
        {
        memset(&text[k], ' ', indent);
        k += indent;
        }
      }
    }

  /* remove any trailing blank lines */
  while (k > 0 && text[k-1] == '\n') { k--; }
  text[k++] = '\n';
  text[k] = '\0';

  return text;
}

/* -------------------------------------------------------------------- */
/* Check for type precedence. Some method signatures will just never
 * be called because of the way python types map to C++ types.  If
 * we don't remove such methods, they can lead to ambiguities later.
 *
 * The precedence rule is the following:
 * The type closest to the native Python type wins.
 */

void vtkWrapPython_RemovePreceededMethods(
  FunctionInfo *wrappedFunctions[],
  int numberOfWrappedFunctions, int fnum)
{
  FunctionInfo *theFunc = wrappedFunctions[fnum];
  const char *name = theFunc->Name;
  FunctionInfo *sig1;
  FunctionInfo *sig2;
  int vote1 = 0;
  int vote2 = 0;
  int occ1, occ2;
  unsigned int baseType1, baseType2;
  unsigned int unsigned1, unsigned2;
  unsigned int indirect1, indirect2;
  int i, n;

  if (!name)
    {
    return;
    }

  for (occ1 = fnum; occ1 < numberOfWrappedFunctions; occ1++)
    {
    sig1 = wrappedFunctions[occ1];

    if (sig1->Name && strcmp(sig1->Name, name) == 0)
      {
      for (occ2 = occ1+1; occ2 < numberOfWrappedFunctions; occ2++)
        {
        sig2 = wrappedFunctions[occ2];
        vote1 = 0;
        vote2 = 0;

        if (sig2->NumberOfArguments == sig1->NumberOfArguments &&
            sig2->Name && strcmp(sig2->Name, name) == 0)
          {
          n = sig1->NumberOfArguments;
          for (i = 0; i < n; i++)
            {
            if (sig1->ArgCounts[i] != sig2->ArgCounts[i])
              {
              vote1 = 0;
              vote2 = 0;
              break;
              }
            else
              {
              baseType1 = (sig1->ArgTypes[i] & VTK_PARSE_BASE_TYPE);
              baseType2 = (sig2->ArgTypes[i] & VTK_PARSE_BASE_TYPE);

              unsigned1 = (baseType1 & VTK_PARSE_UNSIGNED);
              unsigned2 = (baseType2 & VTK_PARSE_UNSIGNED);

              baseType1 = (baseType1 & ~VTK_PARSE_UNSIGNED);
              baseType2 = (baseType2 & ~VTK_PARSE_UNSIGNED);

              indirect1 = (sig1->ArgTypes[i] & VTK_PARSE_INDIRECT);
              indirect2 = (sig2->ArgTypes[i] & VTK_PARSE_INDIRECT);

              /* double preceeds float */
              if ((indirect1 == indirect2) &&
                       (baseType1 == VTK_PARSE_DOUBLE) &&
                       (baseType2 == VTK_PARSE_FLOAT))
                {
                if (!vote2) { vote1 = 1; }
                }
              else if ((indirect1 == indirect2) &&
                  (baseType1 == VTK_PARSE_FLOAT) &&
                  (baseType2 == VTK_PARSE_DOUBLE))
                {
                if (!vote1) { vote2 = 1; }
                }
              /* unsigned char preceeds signed char */
              else if ((indirect1 == indirect2) &&
                       ((baseType1 == VTK_PARSE_CHAR) && unsigned1) &&
                       (baseType2 == VTK_PARSE_SIGNED_CHAR))
                {
                if (!vote2) { vote1 = 1; }
                }
              else if ((indirect1 == indirect2) &&
                       (baseType1 == VTK_PARSE_SIGNED_CHAR) &&
                       ((baseType2 == VTK_PARSE_CHAR) && unsigned2))
                {
                if (!vote1) { vote2 = 1; }
                }
              /* signed preceeds unsigned for everthing but char */
              else if ((indirect1 == indirect2) &&
                       (baseType1 != VTK_PARSE_CHAR) &&
                       (baseType2 != VTK_PARSE_CHAR) &&
                       (baseType1 == baseType2) &&
                       (unsigned1 != unsigned2))
                {
                if (unsigned2 && !vote2)
                  {
                  vote1 = 1;
                  }
                else if (unsigned1 && !vote1)
                  {
                  vote2 = 1;
                  }
                }
              /* integer promotion precedence */
              else if ((indirect1 == indirect2) &&
                       ((baseType1 == VTK_PARSE_INT) ||
                        (baseType1 == VTK_PARSE_ID_TYPE)) &&
                       ((baseType2 == VTK_PARSE_SHORT) ||
                        (baseType2 == VTK_PARSE_SIGNED_CHAR) ||
                        ((baseType2 == VTK_PARSE_CHAR) && unsigned2)))
                {
                if (!vote2) { vote1 = 1; }
                }
              else if ((indirect1 == indirect2) &&
                       ((baseType2 == VTK_PARSE_INT) ||
                        (baseType2 == VTK_PARSE_ID_TYPE)) &&
                       ((baseType1 == VTK_PARSE_SHORT) ||
                        (baseType1 == VTK_PARSE_SIGNED_CHAR) ||
                        ((baseType1 == VTK_PARSE_CHAR) && unsigned1)))
                {
                if (!vote1) { vote2 = 1; }
                }
              /* a "char *" method preceeds a string method */
              else if ((baseType1 == VTK_PARSE_CHAR) &&
                       (indirect1 == VTK_PARSE_POINTER) &&
                       (baseType2 == VTK_PARSE_STRING) &&
                       ((indirect2 == VTK_PARSE_REF) || (indirect2 == 0)))
                {
                if (!vote2) { vote1 = 1; }
                }
              else if ((baseType2 == VTK_PARSE_CHAR) &&
                       (indirect2 == VTK_PARSE_POINTER) &&
                       (baseType1 == VTK_PARSE_STRING) &&
                       ((indirect1 == VTK_PARSE_REF) || (indirect1 == 0)))
                {
                if (!vote1) { vote2 = 1; }
                }
              /* mismatch: both methods are allowed to live */
              else if ((baseType1 != baseType2) ||
                       (unsigned1 != unsigned2) ||
                       (indirect1 != indirect2))
                {
                vote1 = 0;
                vote2 = 0;
                break;
                }
              }
            }
          }

        if (vote1)
          {
          sig2->Name = 0;
          }
        else if (vote2)
          {
          sig1->Name = 0;
          break;
          }

        } /* for (occ2 ... */
      } /* if (sig1->Name ... */
    } /* for (occ1 ... */
}

/* -------------------------------------------------------------------- */
/* Print out all the python methods that call the C++ class methods.
 * After they're all printed, a Py_MethodDef array that has function
 * pointers and documentation for each method is printed.  In other
 * words, this poorly named function is "the big one". */

static void vtkWrapPython_GenerateMethods(
  FILE *fp, ClassInfo *data, HierarchyInfo *hinfo,
  int is_vtkobject, int do_constructors)
{
  int i, j, k;
  int is_static, is_pure_virtual;
  int fnum, occ;
  int numberOfSignatures, signatureCount;
  char signatureSuffix[8];
  int all_legacy, all_static;
  int numberOfWrappedFunctions = 0;
  FunctionInfo *wrappedFunctions[1000];
  FunctionInfo *theSignature;
  FunctionInfo *theFunc;
  const char *ccp;
  unsigned int returnType = 0;
  unsigned int argType = 0;
  int potential_error = 0;
  int needs_cleanup = 0;
  char on_error[32];

  /* output any custom methods */
  vtkWrapPython_CustomMethods(fp, data, do_constructors);

  /* go through all functions and see which are wrappable */
  for (i = 0; i < data->NumberOfFunctions; i++)
    {
    theFunc = data->Functions[i];

    /* check for wrappability */
    if (vtkWrapPython_MethodCheck(data, theFunc, hinfo) &&
        !vtkWrapPython_IsDestructor(data, theFunc) &&
        (!vtkWrapPython_IsConstructor(data, theFunc) == !do_constructors))
      {
      ccp = vtkWrapPython_PythonSignature(theFunc);
      theFunc->Signature = (char *)malloc(strlen(ccp)+1);
      strcpy(theFunc->Signature, ccp);
      wrappedFunctions[numberOfWrappedFunctions++] = theFunc;
      }
    }

  /* for each function in the array */
  for (fnum = 0; fnum < numberOfWrappedFunctions; fnum++)
    {
    /* make sure we haven't already done one of these */
    theFunc = wrappedFunctions[fnum];
    returnType = (theFunc->ReturnType & VTK_PARSE_UNQUALIFIED_TYPE);

    /* check for type precedence, don't need a "float" method if a
       "double" method exists */
    vtkWrapPython_RemovePreceededMethods(
      wrappedFunctions, numberOfWrappedFunctions, fnum);

    /* if theFunc wasn't removed, process all its signatures */
    if (theFunc->Name)
      {
      fprintf(fp,"\n");

      /* check whether all signatures are static methods or legacy */
      numberOfSignatures = 0;
      all_static = 1;
      all_legacy = 1;
      for (occ = fnum; occ < numberOfWrappedFunctions; occ++)
        {
        /* is it the same name */
        if (wrappedFunctions[occ]->Name &&
            !strcmp(theFunc->Name,wrappedFunctions[occ]->Name))
          {
          /* increment the signature count */
          numberOfSignatures++;

          /* check for static methods */
          if (!wrappedFunctions[occ]->IsStatic)
            {
            all_static = 0;
            }

          /* check for legacy */
          if (!wrappedFunctions[occ]->IsLegacy)
            {
            all_legacy = 0;
            }
          }
        }

      /* find all occurances of this method */
      signatureCount = 0;
      for (occ = fnum; occ < numberOfWrappedFunctions; occ++)
        {
        theSignature = wrappedFunctions[occ];
        potential_error = 0;
        needs_cleanup = 0;

        /* is it the same name */
        if (theSignature->Name &&
            !strcmp(theFunc->Name,theSignature->Name))
          {
          signatureCount++;

          if (theSignature->IsLegacy)
            {
            fprintf(fp,
                    "#if !defined(VTK_LEGACY_REMOVE)\n");
            }

          /* check for static methods */
          is_static = 0;
          if (theSignature->IsStatic)
            {
            is_static = 1;
            }

          /* check for pure virtual methods */
          is_pure_virtual = 0;
          if (theSignature->IsPureVirtual)
            {
            is_pure_virtual = 1;
            }

          /* method suffix to distinguish between signatures */
          signatureSuffix[0] = '\0';
          if (numberOfSignatures > 1)
            {
            sprintf(signatureSuffix, "_s%d", signatureCount);
            }

          /* declare the method */
          fprintf(fp,
                  "static PyObject *Py%s_%s%s(PyObject *%s, PyObject *args)\n"
                  "{\n",
                  data->Name, theSignature->Name, signatureSuffix,
                  ((is_static | do_constructors) ? "" : "self"));

          returnType = (theSignature->ReturnType &
                        VTK_PARSE_UNQUALIFIED_TYPE);

          /* declare the variables */
          if (!is_static)
            {
            if (is_vtkobject || do_constructors)
              {
              fprintf(fp,
                      "  %s *op;\n",
                      data->Name);
              }
            else
              {
              fprintf(fp,
                      "  %s *op = static_cast<%s *>(\n"
                      "    ((PyVTKSpecialObject *)self)->vtk_ptr);\n",
                      data->Name, data->Name);
              }
            }

          /* temp variables for arg values */
          for (i = 0; i < theSignature->NumberOfArguments; i++)
            {
            vtkWrapPython_MakeTempVariable(fp, theSignature, i);

            /* special object args need cleanup */
            if (((theSignature->ArgTypes[i] & VTK_PARSE_UNQUALIFIED_TYPE) ==
                 VTK_PARSE_OBJECT) ||
                ((theSignature->ArgTypes[i] & VTK_PARSE_UNQUALIFIED_TYPE) ==
                 VTK_PARSE_OBJECT_REF))
              {
              needs_cleanup = 1;
              }
            }

          /* temp variable for C++-type return value */
          vtkWrapPython_MakeTempVariable(fp, theSignature, MAX_ARGS);

          /* temp variable for the Python return value */
          fprintf(fp,
                  "  PyObject *result = NULL;\n"
                  "\n");

          /* is cleanup necessary, or can we ditch when an error occurs? */
          strcpy(on_error, "return NULL");
          if (needs_cleanup)
            {
            sprintf(on_error, "goto break%d", occ);
            }

          /* pure virtual class methods need "self" to be an object */
          if (is_vtkobject && is_pure_virtual)
            {
            fprintf(fp,
                    "  if (PyVTKClass_Check(self))\n"
                    "    {\n"
                    "    PyErr_SetString(PyExc_TypeError, \"pure virtual method call\");\n"
                    "    return NULL;\n"
                    "    }\n"
                    "\n");
            }

          /* Use ParseTuple to convert python args to C args */
          if (is_static || !is_vtkobject)
            {
            fprintf(fp,
                    "  if ((PyArg_ParseTuple(args, (char*)\"%s:%s\"",
                    vtkWrapPython_FormatString(theSignature),
                    theSignature->Name);
            }
          else
            {
            fprintf(fp,
                    "  op = static_cast<%s *>(vtkPythonUtil::VTKParseTuple(\n"
                    "    self, args, \"%s:%s\"",
                    data->Name,
                    vtkWrapPython_FormatString(theSignature),
                    theSignature->Name);
            }

          for (i = 0; i < theSignature->NumberOfArguments; i++)
            {
            argType = (theSignature->ArgTypes[i] &
                       VTK_PARSE_UNQUALIFIED_TYPE);

            if ((argType & VTK_PARSE_BASE_TYPE) == VTK_PARSE_OBJECT)
              {
              fprintf(fp,", &tempH%d",i);
              }
            else if (argType == VTK_PARSE_BOOL ||
                     argType == VTK_PARSE_BOOL_REF)
              {
              fprintf(fp,", &tempB%d",i);
              }
            else if (argType == VTK_PARSE_STRING ||
                     argType == VTK_PARSE_STRING_REF)
              {
              fprintf(fp,", &tempC%d",i);
              }
            else if (argType == VTK_PARSE_UNICODE_STRING ||
                     argType == VTK_PARSE_UNICODE_STRING_REF)
              {
              fprintf(fp,", &tempU%d",i);
              }
            else if (argType == VTK_PARSE_VOID_PTR)
              {
              fprintf(fp,", &temp%d, &size%d",i,i);
              }
            else
              {
              if (theSignature->ArgCounts[i])
                {
                for (j = 0; j < theSignature->ArgCounts[i]; j++)
                  {
                  fprintf(fp,", temp%d + %d",i,j);
                  }
                }
              else
                {
                fprintf(fp,", &temp%d",i);
                }
              }
            }
          if (is_static || !is_vtkobject)
            {
            fprintf(fp, ")))\n"
                    "    {\n");
            }
          else
            {
            fprintf(fp,"));\n"
                    "\n"
                    "  if (op)\n"
                    "    {\n");
            }

          /* lookup and required objects */
          for (i = 0; i < theSignature->NumberOfArguments; i++)
            {
            argType = (theSignature->ArgTypes[i] &
                       VTK_PARSE_UNQUALIFIED_TYPE);

            if (argType == VTK_PARSE_OBJECT_PTR)
              {
              fprintf(fp,
                      "    temp%d = (%s *)vtkPythonUtil::GetPointerFromObject(\n"
                      "      tempH%d,\"%s\");\n",
                      i, theSignature->ArgClasses[i], i,
                      theSignature->ArgClasses[i]);
              fprintf(fp,
                      "    if (!temp%d && tempH%d != Py_None)\n"
                      "      {\n"
                      "      %s;\n"
                      "      }\n",
                      i, i, on_error);

              potential_error = 1;
              }
            else if (argType == VTK_PARSE_OBJECT ||
                     argType == VTK_PARSE_OBJECT_REF)
              {
              fprintf(fp,
                      "    temp%d = static_cast<%s *>(\n"
                      "      vtkPythonUtil::GetPointerFromSpecialObject(\n"
                      "        tempH%d, \"%s\", &tempH%d));\n",
                      i, theSignature->ArgClasses[i], i,
                      theSignature->ArgClasses[i], i);

              fprintf(fp,
                      "    if (!temp%d)\n"
                      "      {\n"
                      "      %s;\n"
                      "      }\n",
                      i, on_error);

              potential_error = 1;
              }
            else if (argType == VTK_PARSE_BOOL ||
                     argType == VTK_PARSE_BOOL_REF)
              {
              fprintf(fp,
                      "    tempI%d = PyObject_IsTrue(tempB%d);\n"
                      "    if (tempI%d == -1)\n"
                      "      {\n"
                      "      %s;\n"
                      "      }\n"
                      "    temp%d = (tempI%d != 0);\n",
                      i, i, i, on_error, i, i);
              }
            else if (argType == VTK_PARSE_STRING ||
                     argType == VTK_PARSE_STRING_REF)
              {
              fprintf(fp,
                      "    temp%d = tempC%d;\n",
                      i, i);
              }
#ifdef Py_USING_UNICODE
            else if (argType == VTK_PARSE_UNICODE_STRING ||
                     argType == VTK_PARSE_UNICODE_STRING_REF)
              {
              fprintf(fp,
                      "    tempS%d = PyUnicode_AsUTF8String(tempU%d);\n"
                      "    if (tempS%d)\n"
                      "      {\n"
                      "      temp%d = vtkUnicodeString::from_utf8(PyString_AS_STRING(tempS%d));\n"
                      "      Py_DECREF(tempS%d);\n"
                      "      }\n"
                      "    else\n"
                      "      {\n"
                      "      %s;\n"
                      "      }\n",
                      i, i, i, i, i, i, on_error);
              }
#endif
            }
          /* make sure passed method is callable  for VAR functions */
          if (theSignature->NumberOfArguments == 1 &&
              theSignature->ArgTypes[0] == VTK_PARSE_FUNCTION)
            {
            fprintf(fp,
                    "    if (!PyCallable_Check(temp0) && temp0 != Py_None)\n"
                    "      {\n"
                    "      PyErr_SetString(PyExc_ValueError,\"vtk callback method passed to %s in %s was not callable.\");\n"
                    "      return NULL;\n"
                    "      }\n"
                    "    Py_INCREF(temp0);\n",
                    theSignature->Name, data->Name);
            }

          /* check for void pointers and pass appropriate info*/
          for (i = 0; i < theSignature->NumberOfArguments; i++)
            {
            argType = (theSignature->ArgTypes[i] &
                       VTK_PARSE_UNQUALIFIED_TYPE);

            if (argType == VTK_PARSE_VOID_PTR)
              {
              fprintf(fp,
                      "    temp%i = vtkPythonUtil::UnmanglePointer((char *)temp%i,&size%i,\"%s\");\n",
                      i, i, i, "void_p");

              fprintf(fp,
                      "    if (size%i == -1)\n"
                      "      {\n"
                      "      PyErr_SetString(PyExc_ValueError,\"mangled pointer to %s in %s was of incorrect type.\");\n"
                      "      %s;\n"
                      "      }\n",
                      i, theSignature->Name, data->Name, on_error);

              fprintf(fp,
                      "    else if (size%i == -2)\n"
                      "      {\n"
                      "      PyErr_SetString(PyExc_ValueError,\"mangled pointer to %s in %s was poorly formed.\");\n"
                      "      %s;\n"
                      "      }\n",
                      i, theSignature->Name, data->Name, on_error);

              potential_error = 1;
              }
            }

          for (k = 0;
               k < (2 - (is_static || !is_vtkobject || is_pure_virtual));
               k++)
            {
            char methodname[256];
            if (k == 0)
              {
              if (is_static)
                {
                sprintf(methodname,"%s::%s",
                        data->Name, theSignature->Name);
                }
              else if (do_constructors)
                {
                sprintf(methodname, "%s", theSignature->Name);
                }
              else if (!is_vtkobject || is_pure_virtual)
                {
                sprintf(methodname,"op->%s", theSignature->Name);
                }
              else
                {
                fprintf(fp,
                        "    if (PyVTKClass_Check(self))\n"
                        "      {\n");

                sprintf(methodname,"op->%s::%s",
                  data->Name, theSignature->Name);
                }
              }
            else
              {
              fprintf(fp,
                      "    else\n"
                      "      {\n");

              sprintf(methodname, "op->%s", theSignature->Name);
              }

            if (is_vtkobject && !is_static && !is_pure_virtual &&
                !do_constructors)
               {
               fprintf(fp, "  ");
               }

            switch (returnType)
              {
              case VTK_PARSE_VOID:
                if (do_constructors)
                  {
                  fprintf(fp,
                          "    op = new %s(",
                          methodname);
                  }
                else
                  {
                  fprintf(fp,
                          "    %s(",
                          methodname);
                  }
                break;
              default:
                if ((returnType & VTK_PARSE_INDIRECT) == VTK_PARSE_REF)
                  {
                  fprintf(fp,
                          "    temp%i = &%s(",
                          MAX_ARGS, methodname);
                  }
                else
                  {
                  fprintf(fp,
                          "    temp%i = %s(",
                          MAX_ARGS, methodname);
                  }
              }

            for (i = 0; i < theSignature->NumberOfArguments; i++)
              {
              argType = (theSignature->ArgTypes[i] &
                         VTK_PARSE_UNQUALIFIED_TYPE);

              if (i)
                {
                fprintf(fp,",");
                }
              if (argType == VTK_PARSE_OBJECT_REF ||
                  argType == VTK_PARSE_OBJECT)
                {
                fprintf(fp,"*(temp%i)",i);
                }
              else if (theSignature->NumberOfArguments == 1 &&
                       theSignature->ArgTypes[i] == VTK_PARSE_FUNCTION)
                {
                fprintf(fp,"((temp0 != Py_None) ? vtkPythonVoidFunc : NULL),(void *)temp%i",i);
                }
              else
                {
                fprintf(fp,"temp%i",i);
                }
              }
            fprintf(fp,");\n");

            if (theSignature->NumberOfArguments == 1 &&
                theSignature->ArgTypes[0] == VTK_PARSE_FUNCTION)
              {
              fprintf(fp,
                      "      %sArgDelete(vtkPythonVoidFuncArgDelete);\n",
                      methodname);
              }

            if (is_vtkobject && !is_static && !is_pure_virtual &&
                !do_constructors)
              {
              fprintf(fp,
                      "      }\n");
              }
            }

          /* If a mutable python sequence was used as a C array arg,
           * then if the VTK method changed any values in the array,
           * copy the changes from the C array into the python sequence */
          for (i = 0; i < theSignature->NumberOfArguments; i++)
            {
            argType = (theSignature->ArgTypes[i] &
                       VTK_PARSE_UNQUALIFIED_TYPE);

            if (theSignature->ArgCounts[i] &&  /* array */
                (argType & VTK_PARSE_BASE_TYPE) != 0 && /* not special type */
                (argType & VTK_PARSE_BASE_TYPE) != VTK_PARSE_OBJECT &&
                (argType & VTK_PARSE_BASE_TYPE) != VTK_PARSE_UNKNOWN &&
                (argType & VTK_PARSE_BASE_TYPE) != VTK_PARSE_VOID &&
                ((theSignature->ArgTypes[i] & VTK_PARSE_CONST) == 0))
              {
              fprintf(fp,
                      "    if (vtkPythonUtil::CheckArray(args,%d,temp%d,%d))\n"
                      "      {\n"
                      "      %s;\n"
                      "      }\n",
                      i, i, theSignature->ArgCounts[i], on_error);

              potential_error = 1;
              }
            }

          /* generate the code that builds the return value */
          if (do_constructors && !is_vtkobject)
            {
            fprintf(fp,
                    "    result = PyVTKSpecialObject_New(\"%s\", op);\n",
                    data->Name);
            }
          else
            {
            vtkWrapPython_ReturnValue(fp, theSignature);
            }

          /* Add a label if a goto was used */
          if (potential_error && needs_cleanup)
            {
            fprintf(fp,
                    "    break%d:\n",
                    occ);
            }

          /* Free any objects that were constructed by an earlier call
           * to vtkPythonUtil::GetPointerFromSpecialObject() */
          for (i = 0; i < theSignature->NumberOfArguments; i++)
            {
            argType = (theSignature->ArgTypes[i] &
                       VTK_PARSE_UNQUALIFIED_TYPE);

            if (argType == VTK_PARSE_OBJECT_REF ||
                argType == VTK_PARSE_OBJECT)
              {
              fprintf(fp,
                      "    if (tempH%d)\n"
                      "      {\n"
                      "      Py_DECREF(tempH%d);\n"
                      "      }\n",
                      i, i);
              }
            }

          /* It's all over... return the result */
          fprintf(fp,
                  "    }\n"
                  "  return result;\n"
                  "}\n");

          if (theSignature->IsLegacy)
            {
            fprintf(fp,
                    "#endif\n");
            }

          fprintf(fp,
                  "\n");
          }
        }

      if (numberOfSignatures > 1 || do_constructors)
        {
        /* output the method table for the signatures */

        if(all_legacy)
          {
          fprintf(fp,
                  "#if !defined(VTK_LEGACY_REMOVE)\n");
          }

        fprintf(fp,
               "static PyMethodDef Py%s_%sMethods[] = {\n",
                data->Name, wrappedFunctions[fnum]->Name);

        signatureCount = 0;
        for (occ = fnum; occ < numberOfWrappedFunctions; occ++)
          {
          theSignature = wrappedFunctions[occ];

          if (theSignature->Name == 0 ||
              strcmp(theSignature->Name, theFunc->Name) != 0)
            {
            continue;
            }

          signatureCount++;

          is_static = 0;
          if (theSignature->IsStatic)
            {
            is_static = 1;
            }

          if (theSignature->IsLegacy && !all_legacy)
            {
            fprintf(fp,
                   "#if !defined(VTK_LEGACY_REMOVE)\n");
            }

          /* method suffix to distinguish between signatures */
          signatureSuffix[0] = '\0';
          if (numberOfSignatures > 1)
            {
            sprintf(signatureSuffix, "_s%d", signatureCount);
            }

          fprintf(fp,
                  "  {NULL, (PyCFunction)Py%s_%s%s, 1,\n"
                  "   (char*)\"%s\"},\n",
                  data->Name, wrappedFunctions[occ]->Name,
                  signatureSuffix,
                  vtkWrapPython_ArgCheckString((is_vtkobject && !is_static),
                                               wrappedFunctions[occ]));

          if (theSignature->IsLegacy && !all_legacy)
            {
            fprintf(fp,
                    "#endif\n");
            }
          }

        fprintf(fp,
                "  {NULL,       NULL, 0, NULL}\n"
                "};\n"
                "\n");

        if (all_legacy)
          {
          fprintf(fp,
                  "#endif\n");
          }
        }

      if (numberOfSignatures > 1)
        {
        /* declare a "master method" to look through the signatures */
        if(all_legacy)
          {
          fprintf(fp,
                  "#if !defined(VTK_LEGACY_REMOVE)\n");
          }

        fprintf(fp,
                "static PyObject *Py%s_%s(PyObject *self, PyObject *args)\n"
                "{\n"
                "  PyMethodDef *methods = Py%s_%sMethods;\n"
                "\n"
                "  return vtkPythonUtil::CallOverloadedMethod(methods, self, args);\n"
                "}\n",
                 data->Name, wrappedFunctions[fnum]->Name,
                 data->Name, wrappedFunctions[fnum]->Name);

        if (all_legacy)
          {
          fprintf(fp,
                  "#endif\n");
          }
        }

      fprintf(fp,"\n");

      /* set the legacy flag */
      theFunc->IsLegacy = all_legacy;

      /* clear all occurances of this method from further consideration */
      for (occ = fnum + 1; occ < numberOfWrappedFunctions; occ++)
        {
        theSignature = wrappedFunctions[occ];

        /* is it the same name */
        if (theSignature->Name &&
            !strcmp(theFunc->Name,theSignature->Name))
          {
          size_t siglen = strlen(theFunc->Signature);

          /* memory leak here but ... */
          theSignature->Name = NULL;
          theFunc->Signature = (char *)
            realloc(theFunc->Signature,siglen+2+
                    strlen(theSignature->Signature));
          strcpy(&theFunc->Signature[siglen],"\n");
          strcpy(&theFunc->Signature[siglen+1],
                 theSignature->Signature);
          }
        }
      } /* is this method non NULL */
    } /* loop over all methods */

  /* the method table for constructors is produced elsewhere */
  if (do_constructors)
    {
    return;
    }

  /* output the method table, with pointers to each function defined above */
  fprintf(fp,
          "static PyMethodDef Py%sMethods[] = {\n",
          data->Name);

  for (fnum = 0; fnum < numberOfWrappedFunctions; fnum++)
    {
    if(wrappedFunctions[fnum]->IsLegacy)
      {
      fprintf(fp,
              "#if !defined(VTK_LEGACY_REMOVE)\n");
      }
    if (wrappedFunctions[fnum]->Name)
      {
      const char *comment;

      /* format the comment nicely */
      comment = vtkWrapPython_FormatComment(
        wrappedFunctions[fnum]->Comment, 66);

      fprintf(fp,
              "  {(char*)\"%s\",                (PyCFunction)Py%s_%s, 1,\n"
              "   (char*)\"%s\\n\\n%s\"},\n",
              wrappedFunctions[fnum]->Name, data->Name,
              wrappedFunctions[fnum]->Name,
              vtkWrapPython_FormatSignature(
                wrappedFunctions[fnum]->Signature, 66),
              vtkWrapPython_QuoteString(comment,1000));
      }
    if(wrappedFunctions[fnum]->IsLegacy)
      {
      fprintf(fp,
              "#endif\n");
      }
    }

  /* vtkObject needs a special entry for AddObserver */
  if (strcmp("vtkObject", data->Name) == 0)
    {
    fprintf(fp,
            "  {(char*)\"AddObserver\",  (PyCFunction)Py%s_AddObserver, 1,\n"
            "   (char*)\"V.AddObserver(int, function) -> int\\nC++: unsigned long AddObserver(const char *event,\\n    vtkCommand *command, float priority=0.0f)\\n\\nAdd an event callback function(vtkObject, int) for an event type.\\nReturns a handle that can be used with RemoveEvent(int).\"},\n",
            data->Name);
    }

  /* vtkObjectBase needs entries for GetAddressAsString and PrintRevisions */
  else if (strcmp("vtkObjectBase", data->Name) == 0)
    {
    fprintf(fp,
            "  {(char*)\"GetAddressAsString\",  (PyCFunction)Py%s_GetAddressAsString, 1,\n"
            "   (char*)\"V.GetAddressAsString(string) -> string\\nC++: const char *GetAddressAsString()\\n\\nGet address of C++ object in format 'Addr=%%p' after casting to\\nthe specified type.  You can get the same information from o.__this__.\"},\n"
            "  {(char*)\"PrintRevisions\",  (PyCFunction)Py%s_PrintRevisions, 1,\n"
            "   (char*)\"V.PrintRevisions() -> string\\nC++: const char *PrintRevisions()\\n\\nPrints the .cxx file CVS revisions of the classes in the\\nobject's inheritance chain.\"},\n",
            data->Name, data->Name);
    }

  /* python expects the method table to end with a "NULL" entry */
  fprintf(fp,
          "  {NULL,                       NULL, 0, NULL}\n"
          "};\n"
          "\n");
}

/* -------------------------------------------------------------------- */
static int vtkWrapPython_IsDestructor(
  ClassInfo *data, FunctionInfo *currentFunction)
{
  size_t i;
  char *cp;

  if (data->Name && currentFunction->Name)
    {
    cp = currentFunction->Signature;
    for (i = 0; cp[i] != '\0' && cp[i] != '('; i++)
      {
      if (cp[i] == '~')
        {
        return 1;
        }
      }
    }

  return 0;
}

/* -------------------------------------------------------------------- */
static int vtkWrapPython_IsConstructor(
  ClassInfo *data, FunctionInfo *currentFunction)
{
  if (data->Name && currentFunction->Name &&
      !vtkWrapPython_IsDestructor(data, currentFunction))
    {
    return (strcmp(data->Name, currentFunction->Name) == 0);
    }

  return 0;
}

/* -------------------------------------------------------------------- */
/* Check a method to see if it is wrappable in python */

static int vtkWrapPython_MethodCheck(ClassInfo *data,
  FunctionInfo *currentFunction, HierarchyInfo *hinfo)
{
  static unsigned int supported_types[] = {
    VTK_PARSE_VOID, VTK_PARSE_BOOL, VTK_PARSE_FLOAT, VTK_PARSE_DOUBLE,
    VTK_PARSE_CHAR, VTK_PARSE_UNSIGNED_CHAR, VTK_PARSE_SIGNED_CHAR,
    VTK_PARSE_INT, VTK_PARSE_UNSIGNED_INT,
    VTK_PARSE_SHORT, VTK_PARSE_UNSIGNED_SHORT,
    VTK_PARSE_LONG, VTK_PARSE_UNSIGNED_LONG,
    VTK_PARSE_ID_TYPE, VTK_PARSE_UNSIGNED_ID_TYPE,
#ifdef VTK_TYPE_USE_LONG_LONG
    VTK_PARSE_LONG_LONG, VTK_PARSE_UNSIGNED_LONG_LONG,
#endif
#ifdef VTK_TYPE_USE___INT64
    VTK_PARSE___INT64, VTK_PARSE_UNSIGNED___INT64,
#endif
    VTK_PARSE_OBJECT, VTK_PARSE_STRING,
#ifdef Py_USING_UNICODE
    VTK_PARSE_UNICODE_STRING,
#endif
    0
  };

  int i, j;
  int args_ok = 1;
  unsigned int argType = 0;
  unsigned int baseType = 0;
  unsigned int returnType = 0;

  /* some functions will not get wrapped no matter what else,
     and some really common functions will appear only in vtkObjectPython */
  if (currentFunction->IsOperator ||
      currentFunction->ArrayFailure ||
      !currentFunction->IsPublic ||
      !currentFunction->Name)
    {
    return 0;
    }

  /* The unwrappable methods in Filtering/vtkInformation.c */
  if (strcmp(data->Name, "vtkInformation") == 0 &&
      currentFunction->IsLegacy)
    {
    return 0;
    }

  /* check to see if we can handle all the args */
  for (i = 0; i < currentFunction->NumberOfArguments; i++)
    {
    argType = (currentFunction->ArgTypes[i] & VTK_PARSE_UNQUALIFIED_TYPE);
    baseType = (argType & VTK_PARSE_BASE_TYPE);

    if (currentFunction->ArgTypes[i] != VTK_PARSE_FUNCTION)
      {
      for (j = 0; supported_types[j] != 0; j++)
        {
        if (baseType == supported_types[j]) { break; }
        }
      if (supported_types[j] == 0)
        {
        args_ok = 0;
        }
      }

    if (((argType & VTK_PARSE_INDIRECT) != VTK_PARSE_POINTER) &&
        ((argType & VTK_PARSE_INDIRECT) != VTK_PARSE_REF) &&
        ((argType & VTK_PARSE_INDIRECT) != 0)) args_ok = 0;

    if (((argType & VTK_PARSE_INDIRECT) == VTK_PARSE_REF) &&
        (argType != VTK_PARSE_OBJECT_REF) &&
        ((currentFunction->ArgTypes[i] & VTK_PARSE_CONST) == 0)) args_ok = 0;

    if (argType == VTK_PARSE_CHAR_PTR &&
        currentFunction->ArgCounts[i] > 0) args_ok = 0;

    if ((argType & VTK_PARSE_BASE_TYPE) == VTK_PARSE_OBJECT && hinfo)
      {
      if ((argType == VTK_PARSE_OBJECT_REF || argType == VTK_PARSE_OBJECT) &&
          !vtkParseHierarchy_GetProperty(hinfo,
            currentFunction->ArgClasses[i], "WRAP_SPECIAL"))
        {
        args_ok = 0;
        }
      else if ((argType == VTK_PARSE_OBJECT_PTR) &&
          !vtkParseHierarchy_IsExtern(hinfo, currentFunction->ArgClasses[i])&&
          !vtkParseHierarchy_IsTypeOf(hinfo, currentFunction->ArgClasses[i],
                                      "vtkObjectBase"))
        {
        args_ok = 0;
        }
      }

    if (argType == VTK_PARSE_UNSIGNED_CHAR_PTR) args_ok = 0;
    if (argType == VTK_PARSE_UNSIGNED_INT_PTR) args_ok = 0;
    if (argType == VTK_PARSE_UNSIGNED_LONG_LONG_PTR) args_ok = 0;
    if (argType == VTK_PARSE_UNSIGNED___INT64_PTR) args_ok = 0;
    if (argType == VTK_PARSE_UNSIGNED_SHORT_PTR) args_ok = 0;
    if (argType == VTK_PARSE_UNSIGNED_LONG_PTR) args_ok = 0;
    if (argType == VTK_PARSE_STRING_PTR) args_ok = 0;
    if (argType == VTK_PARSE_UNICODE_STRING_PTR) args_ok = 0;
    }

  /* make sure we have all the info we need for array arguments */
  for (i = 0; i < currentFunction->NumberOfArguments; i++)
    {
    argType = (currentFunction->ArgTypes[i] & VTK_PARSE_UNQUALIFIED_TYPE);

    if (((argType & VTK_PARSE_INDIRECT) == VTK_PARSE_POINTER) &&
        (currentFunction->ArgCounts[i] <= 0) &&
        (argType != VTK_PARSE_OBJECT_PTR) &&
        (argType != VTK_PARSE_CHAR_PTR) &&
        (argType != VTK_PARSE_VOID_PTR)) args_ok = 0;
    }

  /* function pointer arguments for callbacks */
  if (currentFunction->NumberOfArguments &&
      (currentFunction->ArgTypes[0] == VTK_PARSE_FUNCTION) &&
      (currentFunction->NumberOfArguments != 1)) args_ok = 0;

  /* check the return type */
  returnType = (currentFunction->ReturnType & VTK_PARSE_UNQUALIFIED_TYPE);
  baseType = (returnType & VTK_PARSE_BASE_TYPE);

  for (j = 0; supported_types[j] != 0; j++)
    {
    if (baseType == supported_types[j]) { break; }
    }
  if (supported_types[j] == 0)
    {
    args_ok = 0;
    }

  if (((returnType & VTK_PARSE_INDIRECT) != VTK_PARSE_POINTER) &&
      ((returnType & VTK_PARSE_INDIRECT) != VTK_PARSE_REF) &&
      ((returnType & VTK_PARSE_INDIRECT) != 0)) args_ok = 0;

  if ((returnType & VTK_PARSE_BASE_TYPE) == VTK_PARSE_OBJECT && hinfo)
    {
    if ((returnType == VTK_PARSE_OBJECT_REF ||
         returnType == VTK_PARSE_OBJECT) &&
        !vtkParseHierarchy_GetProperty(hinfo,
          currentFunction->ReturnClass, "WRAP_SPECIAL"))
      {
      args_ok = 0;
      }
    else if ((returnType == VTK_PARSE_OBJECT_PTR) &&
        !vtkParseHierarchy_IsExtern(hinfo, currentFunction->ReturnClass) &&
        !vtkParseHierarchy_IsTypeOf(hinfo, currentFunction->ReturnClass,
                                   "vtkObjectBase"))
      {
      args_ok = 0;
      }
    }

  /* eliminate "unsigned char *" and "unsigned short *" */
  if (returnType == VTK_PARSE_UNSIGNED_CHAR_PTR) args_ok = 0;
  if (returnType == VTK_PARSE_UNSIGNED_INT_PTR) args_ok = 0;
  if (returnType == VTK_PARSE_UNSIGNED_LONG_LONG_PTR) args_ok = 0;
  if (returnType == VTK_PARSE_UNSIGNED___INT64_PTR) args_ok = 0;
  if (returnType == VTK_PARSE_UNSIGNED_SHORT_PTR) args_ok = 0;
  if (returnType == VTK_PARSE_UNSIGNED_LONG_PTR) args_ok = 0;

  if (returnType == VTK_PARSE_STRING_PTR) args_ok = 0;
  if (returnType == VTK_PARSE_UNICODE_STRING_PTR) args_ok = 0;

  /* if we need a return type hint make sure we have one */
  switch (returnType)
    {
    case VTK_PARSE_FLOAT_PTR:
    case VTK_PARSE_DOUBLE_PTR:
    case VTK_PARSE_ID_TYPE_PTR:
    case VTK_PARSE_LONG_LONG_PTR:
    case VTK_PARSE___INT64_PTR:
    case VTK_PARSE_SIGNED_CHAR_PTR:
    case VTK_PARSE_BOOL_PTR:
    case VTK_PARSE_INT_PTR:
    case VTK_PARSE_SHORT_PTR:
    case VTK_PARSE_LONG_PTR:
      args_ok = currentFunction->HaveHint;
      break;
    }

  /* char * is always treated as a string, not an array */
  if (returnType == VTK_PARSE_CHAR_PTR &&
      currentFunction->HaveHint &&
      currentFunction->HintSize > 0) args_ok = 0;

  /* make sure it isn't a Delete or New function */
  if (currentFunction->Name == 0 ||
      strcmp("Delete", currentFunction->Name) == 0 ||
      strcmp("New", currentFunction->Name) == 0)
    {
    args_ok = 0;
    }

  return args_ok;
}

/* -------------------------------------------------------------------- */
/* Create the docstring for a class, and print it to fp */

static void vtkWrapPython_ClassDoc(
  FILE *fp, FileInfo *file_info, ClassInfo *data, HierarchyInfo *hinfo,
  int is_vtkobject)
{
  char *cp;
  const char *ccp;
  size_t i, n;
  int j;
  char temp[500];
  char *comment;

  if (file_info->NameComment)
    {
    fprintf(fp,
            "    \"%s\\n\",\n",
            vtkWrapPython_QuoteString(
              vtkWrapPython_FormatComment(file_info->NameComment, 70), 500));
    }
  else
    {
    fprintf(fp,
            "    \"%s - no description provided.\\n\\n\",\n",
            vtkWrapPython_QuoteString(data->Name, 500));
    }


  if (data->NumberOfSuperClasses > 0)
    {
    fprintf(fp,
            "    \"Superclass: %s\\n\\n\",\n",
            vtkWrapPython_QuoteString(data->SuperClasses[0], 500));
    }

  n = 100;
  if (file_info->Description)
    {
    n += strlen(file_info->Description);
    }

  if (file_info->Caveats)
    {
    n += strlen(file_info->Caveats);
    }

  if (file_info->SeeAlso)
    {
    n += strlen(file_info->SeeAlso);
    }

  comment = (char *)malloc(n);
  cp = comment;
  *cp = '\0';

  if (file_info->Description)
    {
    strcpy(cp, file_info->Description);
    cp += strlen(cp);
    *cp++ = '\n'; *cp++ = '\n'; *cp = '\0';
    }

  if (file_info->Caveats)
    {
    sprintf(cp, ".SECTION Caveats\n\n");
    cp += strlen(cp);
    strcpy(cp, file_info->Caveats);
    cp += strlen(cp);
    *cp++ = '\n'; *cp++ = '\n'; *cp = '\0';
    }

  if (file_info->SeeAlso)
    {
    sprintf(cp, ".SECTION See Also\n\n");
    cp += strlen(cp);
    strcpy(cp, file_info->SeeAlso);
    cp += strlen(cp);
    *cp = '\0';
    }

  ccp = vtkWrapPython_FormatComment(comment, 70);
  free(comment);

  n = (strlen(ccp) + 400-1)/400;
  for (i = 0; i < n; i++)
    {
    strncpy(temp, &ccp[400*i], 400);
    temp[400] = '\0';
    if (i < n-1)
      {
      fprintf(fp,
              "    \"%s\",\n",
              vtkWrapPython_QuoteString(temp, 500));
      }
    else
      { /* just for the last time */
      fprintf(fp,
              "    \"%s\\n\",\n",
              vtkWrapPython_QuoteString(temp, 500));
      }
    }

  /* for special objects, add constructor signatures to the doc */
  if (!is_vtkobject)
    {
    for (j = 0; j < data->NumberOfFunctions; j++)
      {
      if (vtkWrapPython_MethodCheck(data, data->Functions[j], hinfo) &&
          vtkWrapPython_IsConstructor(data, data->Functions[j]))
        {
        fprintf(fp,"    \"%s\\n\",\n",
                vtkWrapPython_FormatSignature(
                  data->Functions[j]->Signature, 70));
        }
      }
    }
}

/* -------------------------------------------------------------------- */
/* generate includes for any special types that are used */
static void vtkWrapPython_GenerateSpecialHeaders(
  FILE *fp, ClassInfo *data, HierarchyInfo *hinfo)
{
#define MAX_PYTHON_TYPES 1024
  const char *types[MAX_PYTHON_TYPES];
  int numTypes = 0;
  FunctionInfo *currentFunction;
  int i, j, k, n, m;
  unsigned int aType;
  const char *classname;
  const char *ownincfile = "";

  n = data->NumberOfFunctions;
  for (i = 0; i < n; i++)
    {
    currentFunction = data->Functions[i];
    if (vtkWrapPython_MethodCheck(data, currentFunction, hinfo))
      {
      classname = currentFunction->ReturnClass;
      aType = currentFunction->ReturnType;
      m = currentFunction->NumberOfArguments;
      for (j = -1; j < m; j++)
        {
        if (j >= 0)
          {
          classname = currentFunction->ArgClasses[j];
          aType = currentFunction->ArgTypes[j];
          }
        /* we don't require the header file if it is just a pointer */
        if ((aType & VTK_PARSE_INDIRECT) != VTK_PARSE_POINTER)
          {
          if ((aType & VTK_PARSE_BASE_TYPE) == VTK_PARSE_STRING)
            {
            classname = "vtkStdString";
            }
          else if ((aType & VTK_PARSE_BASE_TYPE) == VTK_PARSE_UNICODE_STRING)
            {
            classname = "vtkUnicodeString";
            }
          else if ((aType & VTK_PARSE_BASE_TYPE) != VTK_PARSE_OBJECT)
            {
            classname = 0;
            }
          }
        else
          {
          classname = 0;
          }

        /* we already include our own header */
        if (classname && strcmp(classname, data->Name) != 0)
          {
          for (k = 0; k < numTypes; k++)
            {
            /* make a unique list of all classes found */
            if (strcmp(classname, types[k]) == 0)
              {
              break;
              }
            }

          if (k == numTypes)
            {
            if (k+1 >= MAX_PYTHON_TYPES)
              {
              fprintf(stderr, "vtkWrapPython -- too many types used in file"
                              ", increase MAX_PYTHON_TYPES\n");
              exit(1);
              }
            types[numTypes++] = classname;
            }
          }
        }
      }
    }

  /* try to get our own include file */
  if (hinfo)
    {
    if ((ownincfile = vtkParseHierarchy_ClassHeader(hinfo, data->Name)) == 0)
      {
      ownincfile = "";
      }
    }

  /* for each unique type found in the file */
  for (i = 0; i < numTypes; i++)
    {
    const char *incfile = 0;
    /* try to look up its header file */
    if (hinfo)
      {
      incfile = vtkParseHierarchy_ClassHeader(hinfo, types[i]);
      }
    else if (strcmp(types[i], "vtkArrayCoordinates") == 0)
      {
      incfile = "vtkArrayCoordinates.h";
      }
    else if (strcmp(types[i], "vtkArrayExtents") == 0)
      {
      incfile = "vtkArrayExtents.h";
      }
    else if (strcmp(types[i], "vtkArrayRange") == 0)
      {
      incfile = "vtkArrayRange.h";
      }
    else if (strcmp(types[i], "vtkTimeStamp") == 0)
      {
      incfile = "vtkTimeStamp.h";
      }
    else if (strcmp(types[i], "vtkVariant") == 0)
      {
      incfile = "vtkVariant.h";
      }
    else if (strcmp(types[i], "vtkStdString") == 0)
      {
      incfile = "vtkStdString.h";
      }
    else if (strcmp(types[i], "vtkUnicodeString") == 0)
      {
      incfile = "vtkUnicodeString.h";
      }
    if (incfile)
      {
      /* make sure it doesn't share our header file */
      if (strcmp(incfile, ownincfile) != 0)
        {
        fprintf(fp,
               "#include \"%s\"\n",
                incfile);
        }
      }
    }
}

/* -------------------------------------------------------------------- */
/* generate code for custom methods for some classes */
static void vtkWrapPython_CustomMethods(
  FILE *fp, ClassInfo *data, int do_constructors)
{
  int i;
  FunctionInfo *theFunc;

  /* the python vtkObject needs special hooks for observers */
  if (strcmp("vtkObject", data->Name) == 0 &&
      do_constructors == 0)
    {
    /* remove the original vtkCommand observer methods */
    for (i = 0; i < data->NumberOfFunctions; i++)
      {
      theFunc = data->Functions[i];

      if ((strcmp(theFunc->Name, "AddObserver") == 0) ||
          (strcmp(theFunc->Name, "GetCommand") == 0) ||
          ((strcmp(theFunc->Name, "RemoveObserver") == 0) &&
           (theFunc->ArgTypes[0] != VTK_PARSE_UNSIGNED_LONG)) ||
          (((strcmp(theFunc->Name, "RemoveObservers") == 0) ||
            (strcmp(theFunc->Name, "HasObserver") == 0)) &&
           (((theFunc->ArgTypes[0] != VTK_PARSE_UNSIGNED_LONG) &&
             (theFunc->ArgTypes[0] != (VTK_PARSE_CHAR_PTR|VTK_PARSE_CONST))) ||
            (theFunc->NumberOfArguments > 1))) ||
          ((strcmp(theFunc->Name, "RemoveAllObservers") == 0) &&
           (theFunc->NumberOfArguments > 0)))
        {
        data->Functions[i]->Name = NULL;
        }
      }

    /* Add the AddObserver method to vtkObject. */
    fprintf(fp,
            "static PyObject *PyvtkObject_AddObserver(PyObject *self, PyObject *args)\n"
            "{\n"
            "  vtkObject *op;\n"
            "  char *temp0; // arg 0\n"
            "  PyObject *temp1; // arg 1\n"
            "  float temp2 = 0.0f; // arg 2\n"
            "  unsigned long temp20 = 0; // return value\n"
            "\n");

    fprintf(fp,
            "  op = static_cast<vtkObject *>(vtkPythonUtil::VTKParseTuple(\n"
            "    self, args, \"zO|f:AddObserver\", &temp0, &temp1, &temp2));\n"
            "\n"
            "  if (op)\n"
            "    {\n"
            "    if (!PyCallable_Check(temp1) && temp1 != Py_None)\n"
            "      {\n"
            "      PyErr_SetString(PyExc_ValueError,\n"
            "        \"vtk callback method passed to AddObserver was not callable.\");\n"
            "      return NULL;\n"
            "      }\n"
            "    Py_INCREF(temp1);\n"
            "    vtkPythonCommand *cbc = vtkPythonCommand::New();\n"
            "    cbc->SetObject(temp1);\n"
            "    cbc->SetThreadState(PyThreadState_Get());\n"
            "    temp20 = op->AddObserver(temp0,cbc,temp2);\n"
            "    cbc->Delete();\n"
            "#if (PY_VERSION_HEX >= 0x02020000)\n"
            "    if ((long)(temp20) >= 0)\n"
            "      {\n"
            "      return PyInt_FromLong((long)(temp20));\n"
            "      }\n"
            "    else\n"
            "      {\n"
            "      return PyLong_FromUnsignedLong(temp20);\n"
            "      }\n"
            "#else\n"
            "    return PyInt_FromLong((long)(temp20));\n"
            "#endif\n"
            "    }\n");

    fprintf(fp,
            "  return NULL;\n"
            "}\n"
            "\n");
    }

  /* the python vtkObjectBase needs a couple extra functions */
  if (strcmp("vtkObjectBase", data->Name) == 0 &&
      do_constructors == 0)
   {
     /* remove the original methods, if they exist */
    for (i = 0; i < data->NumberOfFunctions; i++)
      {
      theFunc = data->Functions[i];

      if ((strcmp(theFunc->Name, "GetAddressAsString") == 0) ||
          (strcmp(theFunc->Name, "PrintRevisions") == 0))
        {
        theFunc->Name = NULL;
        }
      }

    /* add the GetAddressAsString method to vtkObjectBase */
    fprintf(fp,
            "PyObject *PyvtkObjectBase_GetAddressAsString(PyObject *self, PyObject *args)\n"
            "{\n"
            "  %s *op;\n"
            "  char *temp0; // arg 0\n"
            "  char temp20[256]; //return value\n"
            "\n"
            "  op = static_cast<%s *>(vtkPythonUtil::VTKParseTuple(\n"
            "   self, args, \"s:GetAddressAsString\", &temp0));\n"
            "\n"
            "  if (op)\n"
            "    {\n"
            "    sprintf(temp20,\"Addr=%%p\",op);\n"
            "    return PyString_FromString(temp20);\n"
            "    }\n"
            "  return NULL;\n"
            "}\n"
            "\n",
            data->Name, data->Name);

    /* add the PrintRevisions method to vtkObjectBase. */
    fprintf(fp,
            "PyObject *PyvtkObjectBase_PrintRevisions(PyObject *self, PyObject *args)\n"
            "{\n"
            "  %s *op;\n"
            "\n"
            "  op = static_cast<%s *>(vtkPythonUtil::VTKParseTuple(\n"
            "    self, args, \":PrintRevisions\"));\n"
            "\n"
            "  if (op)\n"
            "    {\n"
            "    vtksys_ios::ostringstream vtkmsg_with_warning_C4701;\n"
            "    op->PrintRevisions(vtkmsg_with_warning_C4701);\n"
            "    vtkmsg_with_warning_C4701.put('\\0');\n"
            "    PyObject *result = PyString_FromString(\n"
            "      vtkmsg_with_warning_C4701.str().c_str());\n"
            "    return result;\n"
            "    }\n"
            "  return NULL;\n"
            "}\n"
            "\n",
            data->Name, data->Name);
    }
}

/* -------------------------------------------------------------------- */
/* generate the New method for a vtkObjectBase object */
static void vtkWrapPython_GenerateObjectNew(
  FILE *fp, ClassInfo *data, int class_has_new)
{
  if (class_has_new)
    {
    fprintf(fp,
            "static vtkObjectBase *%sStaticNew()\n"
            "{\n"
            "  return %s::New();\n"
            "}\n"
            "\n",
            data->Name, data->Name);
    }

  fprintf(fp,
          "PyObject *PyVTKClass_%sNew(const char *modulename)\n"
          "{\n",
          data->Name);

  if (class_has_new)
    {
    fprintf(fp,
            "  return PyVTKClass_New(&%sStaticNew,\n",
            data->Name);
    }
  else
    {
    fprintf(fp,
            "  return PyVTKClass_New(NULL,\n");
    }

  fprintf(fp,
          "                        Py%sMethods,\n"
          "                        \"%s\",modulename,\n"
          "                        %sDoc(),",
          data->Name, data->Name, data->Name);

  if (strcmp(data->Name,"vtkObjectBase") == 0)
    {
    fprintf(fp, "0);\n");
    }
  else
    {
    fprintf(fp, "\n"
            "                        PyVTKClass_%sNew(modulename));\n",
            data->SuperClasses[0]);
    }

  fprintf(fp,
          "}\n"
          "\n");
}

/* -------------------------------------------------------------------- */
/* generate extra functions for a special object */
static void vtkWrapPython_GenerateSpecialObjectNew(
  FILE *fp, ClassInfo *data, HierarchyInfo *hinfo)
{
  static const char *compare_consts[6] = {
    "Py_LT", "Py_LE", "Py_EQ", "Py_NE", "Py_GT", "Py_GE" };
  static const char *compare_tokens[6] = {
    "<", "<=", "==", "!=", ">", ">=" };
  int compare_ops = 0;
  int has_hash = 0;
  int i;

  /* handle all constructors */
  vtkWrapPython_GenerateMethods(fp, data, hinfo, 0, 1);

  /* the method table for the New method */
  fprintf(fp,
          "static PyMethodDef Py%sNewMethod = \\\n"
          "{ (char*)\"%s\",  (PyCFunction)Py%s_%s, 1,\n"
          "  (char*)\"\" };\n"
          "\n",
          data->Name, data->Name, data->Name,
          data->Name);

  /* the copy constructor */
  fprintf(fp,
         "static void *vtkSpecial_%sCopy(const void *obj)\n"
          "{\n"
          "  if (obj)\n"
          "    {\n"
          "    return new %s(*static_cast<const %s*>(obj));\n"
          "    }\n"
          "  return 0;\n"
          "}\n"
          "\n",
          data->Name, data->Name, data->Name);

  /* the destructor */
  fprintf(fp,
          "static void vtkSpecial_%sDelete(void *obj)\n"
          "{\n"
          "  if (obj)\n"
          "    {\n"
          "    delete static_cast<%s *>(obj);\n"
          "    }\n"
          "}\n"
          "\n",
          data->Name, data->Name);

  /* the printer */
  fprintf(fp,
          "static void vtkSpecial_%sPrint(ostream &os, const void *obj)\n"
          "{\n"
          "  if (obj)\n"
          "    {\n"
          "    os << *static_cast<const %s *>(obj);\n"
          "    }\n"
          "}\n"
          "\n",
          data->Name, data->Name);

  /* hard-code comparison operators until vtkParse provides
   * operator information */
  if (strcmp(data->Name, "vtkVariant") == 0)
    {
    compare_ops =
     ( (1 << Py_LT) | (1 << Py_LE) | (1 << Py_EQ) |
       (1 << Py_NE) | (1 << Py_GT) | (1 << Py_GE));
    }
  else if (strcmp(data->Name, "vtkTimeStamp") == 0)
    {
    compare_ops = ( (1 << Py_LT) | (1 << Py_GT) );
    }

  /* the compare function */
  if (compare_ops != 0)
    {
    fprintf(fp,
            "static int vtkSpecial_%sCompare(const void *o1, const void *o2, int opid)\n"
            "{\n"
            "  const %s &so1 = *static_cast<const %s *>(o1);\n"
            "  const %s &so2 = *static_cast<const %s *>(o2);\n"
            "  switch (opid)\n"
            "    {\n",
            data->Name, data->Name, data->Name,
            data->Name, data->Name);

    for (i = Py_LT; i <= Py_GE; i++)
      {
      if ( ((compare_ops >> i) & 1) != 0 )
        {
        fprintf(fp,
          "    case %s:\n"
          "      return (so1 %s so2);\n",
          compare_consts[i-Py_LT], compare_tokens[i-Py_LT]);
        }
      }

    fprintf(fp,
          "  }"
          "  return -1;\n"
          "}\n"
          "\n");
    }

  /* the hash function for vtkTimeStamp */
  if (strcmp(data->Name, "vtkTimeStamp") == 0)
    {
    has_hash = 1;

    fprintf(fp,
            "static long vtkSpecial_%sHash(const void *self, int *immutable)\n"
            "{\n"
            "  unsigned long mtime = *(static_cast<const vtkTimeStamp *>(self));\n"
            "  long h = (long)mtime;\n"
            "  *immutable = 0;\n"
            "  if (h != -1) { return h; };\n"
            "  return -1;\n"
            "}\n"
            "\n",
            data->Name);
    }

  /* the hash function for vtkVariant */
  if (strcmp(data->Name, "vtkVariant") == 0)
    {
    has_hash = 1;

    fprintf(fp,
            "static long vtkSpecial_%sHash(const void *self, int *immutable)\n"
            "{\n"
            "  long h = vtkPythonUtil::VariantHash(static_cast<const vtkVariant *>(self));\n"
            "  *immutable = 1;\n"
            "  return h;\n"
            "}\n"
            "\n",
            data->Name);
    }

  /* the table to hold these special methods */
  fprintf(fp,
            "static PyVTKSpecialMethods vtkSpecial_%sSpecialMethods =\n"
            "{\n"
            "  &vtkSpecial_%sCopy,\n"
            "  &vtkSpecial_%sDelete,\n"
            "  &vtkSpecial_%sPrint,\n",
            data->Name, data->Name, data->Name,
            data->Name);

  if (compare_ops != 0)
    {
    fprintf(fp,
            "  &vtkSpecial_%sCompare,\n",
            data->Name);
    }
  else
    {
    fprintf(fp,
            "  0,\n");
    }

  if (has_hash)
    {
    fprintf(fp,
            "  &vtkSpecial_%sHash,\n",
            data->Name);
    }
  else
    {
    fprintf(fp,
            "  0,\n");
    }

  fprintf(fp,
          "};\n"
          "\n");

  /* the exported New method */
  fprintf(fp,
          "PyObject *PyVTKClass_%sNew(const char *)\n"
          "{\n"
          "  return PyVTKSpecialType_New(\n"
          "      &Py%sNewMethod, Py%sMethods, Py%s_%sMethods,\n"
          "      \"%s\", %sDoc(),\n"
          "      &vtkSpecial_%sSpecialMethods);\n"
          "}\n"
          "\n",
          data->Name, data->Name, data->Name,
          data->Name, data->Name, data->Name,
          data->Name, data->Name);
}

/* -------------------------------------------------------------------- */
/* This is the main entry point for the python wrappers.  When called,
 * it will print the vtkXXPython.c file contents to "fp".  */

void vtkParseOutput(FILE *fp, FileInfo *file_info)
{
  ClassInfo *data;
  OptionInfo *options;
  HierarchyInfo *hinfo = 0;
  int class_has_new = 0;
  int is_vtkobject = 1;
  int i;

  /* get the main class */
  if ((data = file_info->MainClass) == NULL)
    {
    return;
    }

  /* get the command-line options */
  options = vtkParse_GetCommandLineOptions();

  /* get the hierarchy info for accurate typing */
  if (options->HierarchyFileName)
    {
    hinfo = vtkParseHierarchy_ReadFile(options->HierarchyFileName);
    }

  /* the VTK_WRAPPING_CXX tells header files where they're included from */
  fprintf(fp,
          "// python wrapper for %s object\n//\n"
          "#define VTK_WRAPPING_CXX\n",
          data->Name);

  /* unless this is vtkObjectBase, define VTK_STREAMS_FWD_ONLY */
  if (strcmp("vtkObjectBase",data->Name) != 0)
    {
    /* Block inclusion of full streams.  */
    fprintf(fp,
            "#define VTK_STREAMS_FWD_ONLY\n");
    }

  /* include vtkPython.h on all platforms but apple */
  fprintf(fp,
          "#if !defined(__APPLE__)\n"
          "#include \"vtkPython.h\"\n"
          "#undef _XOPEN_SOURCE /* Conflicts with standards.h.  */\n"
          "#undef _THREAD_SAFE /* Conflicts with pthread.h.  */\n"
          "#endif\n");

  /* lots of important utility functions are defined in vtkPythonUtil.h */
  fprintf(fp,
          "#include \"vtkPythonUtil.h\"\n"
          "#include <vtksys/ios/sstream>\n");

  /* vtkPythonCommand is needed to wrap vtkObject.h */
  if (strcmp("vtkObject", data->Name) == 0)
    {
    fprintf(fp,
          "#include \"vtkPythonCommand.h\"\n");
    }

  /* generate includes for any special types that are used */
  vtkWrapPython_GenerateSpecialHeaders(fp, data, hinfo);

  /* the header file for the wrapped class */
  fprintf(fp,
          "#include \"%s.h\"\n",
          data->Name);

  /* is this isn't a vtkObjectBase-derived object, then it is special */
  is_vtkobject = options->IsVTKObject;

  /* add sstream header for special objects and vtkObjectBase*/
  if (strcmp(data->Name, "vtkObjectBase") == 0 ||
      (!is_vtkobject && !data->IsAbstract))
    {
    fprintf(fp,
            "\n"
            "#include <vtksys/ios/sstream>\n");
    }

  /* do the export of the main entry point */
  fprintf(fp,
          "\n"
          "#if defined(WIN32)\n"
          "extern \"C\" { __declspec( dllexport ) PyObject *PyVTKClass_%sNew(const char *); }\n"
          "#else\n"
          "extern \"C\" { PyObject *PyVTKClass_%sNew(const char *); }\n"
          "#endif\n"
          "\n",
          data->Name, data->Name);

  /* bring in all the superclasses */
  for (i = 0; i < data->NumberOfSuperClasses; i++)
    {
    fprintf(fp,
           "extern \"C\" { PyObject *PyVTKClass_%sNew(const char *); }\n",
            data->SuperClasses[i]);
    }

  /* prototype for the docstring function */
  fprintf(fp,
          "\n"
          "static const char **%sDoc();\n"
          "\n",
          data->Name);

  /* check for New() function */
  for (i = 0; i < data->NumberOfFunctions; i++)
    {
    if (data->Functions[i]->Name &&
        strcmp("New",data->Functions[i]->Name) == 0 &&
        data->Functions[i]->NumberOfArguments == 0)
      {
      class_has_new = 1;
      }
    }

  /* now output all the methods are wrappable */
  if (is_vtkobject || !data->IsAbstract)
    {
    vtkWrapPython_GenerateMethods(fp, data, hinfo, is_vtkobject, 0);
    }

  /* output the class initilization function for VTK objects */
  if (is_vtkobject)
    {
    vtkWrapPython_GenerateObjectNew(fp, data, class_has_new);
    }

  /* output the class initilization function for special objects */
  else if (!data->IsAbstract)
    {
    vtkWrapPython_GenerateSpecialObjectNew(fp, data, hinfo);
    }

  /* the New method for un-wrappable classes returns "NULL" */
  else
    {
    fprintf(fp,
            "PyObject *PyVTKClass_%sNew(const char *)\n"
            "{\n"
            "  return NULL;\n"
            "}\n"
            "\n",
            data->Name);
    }

  /* the docstring for the class, as a static var ending in "Doc" */
  if (is_vtkobject || !data->IsAbstract)
    {
    fprintf(fp,
            "const char **%sDoc()\n"
            "{\n"
            "  static const char *docstring[] = {\n",
            data->Name);

    vtkWrapPython_ClassDoc(fp, file_info, data, hinfo, is_vtkobject);

    fprintf(fp,
            "    NULL\n"
            "  };\n"
            "\n"
            "  return docstring;\n"
            "}\n"
            "\n");
    }
}

