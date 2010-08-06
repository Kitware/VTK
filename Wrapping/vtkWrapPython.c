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

/* write out a python type object */
static void vtkWrapPython_GenerateSpecialType(
  FILE *fp, ClassInfo *data, FileInfo *finfo, HierarchyInfo *hinfo);

/* -------------------------------------------------------------------- */
/* prototypes for utility methods */

/* Make a guess about whether a class is wrapped */
int vtkWrapPython_IsClassWrapped(
  const char *classname, HierarchyInfo *hinfo);

/* check whether a method is wrappable */
static int vtkWrapPython_MethodCheck(
  ClassInfo *data, FunctionInfo *currentFunction, HierarchyInfo *hinfo);

/* is the method a constructor of the class */
static int vtkWrapPython_IsConstructor(
  ClassInfo *data, FunctionInfo *currentFunction);

/* is the method a destructor of the class */
static int vtkWrapPython_IsDestructor(
  ClassInfo *data, FunctionInfo *currentFunction);

/* Get the python format char for the give type */
static char vtkWrapPython_FormatChar(
  unsigned int argtype);

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

/* format a method signature to a 70 char linewidth and char limit */
static const char *vtkWrapPython_FormatSignature(
  const char *signature, size_t width, size_t maxlen);

/* return a python-ized signature */
static const char *vtkWrapPython_PythonSignature(
  FunctionInfo *currentFunction);

/* -------------------------------------------------------------------- */
/* A struct for special types to store info about the type, it is fairly
 * small because not many operators or special features are wrapped */
typedef struct _SpecialTypeInfo
{
  int has_print;    /* there is "<<" stream operator */
  int has_compare;  /* there are comparison operators e.g. "<" */
} SpecialTypeInfo;

/* -------------------------------------------------------------------- */
/* Make a guess about whether a class is wrapped */
int vtkWrapPython_IsClassWrapped(
  const char *classname, HierarchyInfo *hinfo)
{
  if (hinfo)
    {
    if (!vtkParseHierarchy_IsExtern(hinfo, classname) &&
        (!vtkParseHierarchy_GetProperty(
           hinfo, classname, "WRAP_EXCLUDE") ||
         vtkParseHierarchy_GetProperty(
           hinfo, classname, "WRAP_SPECIAL")))
      {
      return 1;
      }
    }
  else if (strncmp("vtk", classname, 3) == 0)
    {
    return 1;
    }

  return 0;
}

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
  const char *sizeMethod = "GetNumberOfComponents";

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
            "      {\n",
            MAX_ARGS);

    if (currentFunction->HintSize == -1)
      {
      fprintf(fp,
              "      vtkIdType nc = op->%s();\n"
              "      result = PyTuple_New(nc);\n"
              "      for (vtkIdType ic = 0; ic < nc; ic++)\n"
              "        {\n"
              "        PyTuple_SET_ITEM(result, ic,\n"
              "                         Py_BuildValue((char *)\"%s\",temp%i[ic]));\n"
              "        }\n",
              sizeMethod, c, MAX_ARGS);
      }
    else
      {
      /* Handle return values of constant size */
      fprintf(fp,
              "      result = Py_BuildValue((char*)\"");

      for (i = 0; i < currentFunction->HintSize; i++)
        {
        fprintf(fp, "%s", c);
        }
      fprintf(fp, "\"");

      for (i = 0; i < currentFunction->HintSize; i++)
        {
        fprintf(fp, ",temp%i[%d]", MAX_ARGS, i);
        }
      fprintf(fp, ");\n");
      }

    fprintf(fp,
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
  const char *Id = currentFunction->ReturnClass;
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
    case VTK_PARSE_QOBJECT:      fprintf(fp,"%s ",Id); break;
    case VTK_PARSE_UNKNOWN:     return;
    }

  /* then print the decorators for ref and pointer, but not for arrays */
  switch (aType & VTK_PARSE_INDIRECT)
    {
    case VTK_PARSE_REF:
      if (((aType & VTK_PARSE_BASE_TYPE) == VTK_PARSE_OBJECT ||
           (aType & VTK_PARSE_BASE_TYPE) == VTK_PARSE_QOBJECT) ||
           (i == MAX_ARGS))
        {
        fprintf(fp, "*"); /* refs are converted to pointers */
        }
      break;
    case VTK_PARSE_POINTER:
      if ((i == MAX_ARGS) ||
          ((aType & VTK_PARSE_BASE_TYPE) == VTK_PARSE_OBJECT) ||
          ((aType & VTK_PARSE_BASE_TYPE) == VTK_PARSE_QOBJECT) ||
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
  if (((aType & VTK_PARSE_UNQUALIFIED_TYPE) == VTK_PARSE_OBJECT ||
      (aType & VTK_PARSE_UNQUALIFIED_TYPE) == VTK_PARSE_QOBJECT )
      && i != MAX_ARGS)
    {
    fprintf(fp, "*");
    }

  /* the variable name */
  fprintf(fp,"temp%i",i);

  /* print the array decorators */
  if (((aType & VTK_PARSE_INDIRECT) == VTK_PARSE_POINTER) &&
      (i != MAX_ARGS) &&
      ((aType & VTK_PARSE_BASE_TYPE) != VTK_PARSE_OBJECT) &&
      ((aType & VTK_PARSE_BASE_TYPE) != VTK_PARSE_QOBJECT) &&
      ((aType & VTK_PARSE_UNQUALIFIED_TYPE) != VTK_PARSE_CHAR_PTR) &&
      ((aType & VTK_PARSE_UNQUALIFIED_TYPE) != VTK_PARSE_VOID_PTR))
    {
    if (aCount == -1)
      {
      fprintf(fp, "[%i]", 20);
      }
    else
      {
      fprintf(fp, "[%i]", aCount);
      }
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

  /* for arrays of dynamic size (Count == -1), need a tuple */
  if (aCount == -1)
    {
    fprintf(fp,
            "  PyObject *tempT%d = 0;\n",
            i);
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
      (((aType & VTK_PARSE_BASE_TYPE) == VTK_PARSE_OBJECT) ||
      ((aType & VTK_PARSE_BASE_TYPE) == VTK_PARSE_QOBJECT)))
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

    case VTK_PARSE_QOBJECT_PTR:
    case VTK_PARSE_QOBJECT_REF:
      {
      fprintf(fp,
              "    result = vtkPythonUtil::SIPGetObjectFromPointer(temp%i, \"%s\", false);\n",
              MAX_ARGS, currentFunction->ReturnClass);
      break;
      }
    case VTK_PARSE_QOBJECT:
      {
      fprintf(fp,
              "    result = vtkPythonUtil::SIPGetObjectFromPointer(new %s(temp%i), \"%s\", true);\n",
              currentFunction->ReturnClass, MAX_ARGS,
              currentFunction->ReturnClass);
      break;
      }

    /* convert special objects to Python objects */
    case VTK_PARSE_OBJECT_REF:
      {
      fprintf(fp,
              "    result = PyVTKSpecialObject_CopyNew(\"%s\", temp%i);\n",
              currentFunction->ReturnClass, MAX_ARGS);
      break;
      }

    /* convert special objects to Python objects */
    case VTK_PARSE_OBJECT:
      {
      fprintf(fp,
              "    result = PyVTKSpecialObject_CopyNew(\"%s\", &temp%i);\n",
              currentFunction->ReturnClass, MAX_ARGS);
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
/* Get the python format char for the give type, after retrieving the
 * base type from the type */
static char vtkWrapPython_FormatChar(unsigned int argtype)
{
  char typeChar = 'O';

  switch ( (argtype & VTK_PARSE_BASE_TYPE) )
    {
    case VTK_PARSE_OBJECT:
    case VTK_PARSE_QOBJECT:
      typeChar = 'O';
      break;
    case VTK_PARSE_FLOAT:
      typeChar = 'f';
      break;
    case VTK_PARSE_DOUBLE:
      typeChar = 'd';
      break;
    case VTK_PARSE_UNSIGNED_INT:
    case VTK_PARSE_INT:
      typeChar = 'i';
      break;
    case VTK_PARSE_UNSIGNED_SHORT:
    case VTK_PARSE_SHORT:
      typeChar = 'h';
      break;
    case VTK_PARSE_UNSIGNED_LONG:
    case VTK_PARSE_LONG:
      typeChar = 'l';
      break;
    case VTK_PARSE_UNSIGNED_ID_TYPE:
    case VTK_PARSE_ID_TYPE:
#ifdef VTK_USE_64BIT_IDS
#ifdef PY_LONG_LONG
      typeChar = 'L';
#else
      typeChar = 'l';
#endif
#else
      typeChar = 'i';
#endif
      break;
#ifdef PY_LONG_LONG
    case VTK_PARSE_UNSIGNED_LONG_LONG:
    case VTK_PARSE_UNSIGNED___INT64:
    case VTK_PARSE_LONG_LONG:
    case VTK_PARSE___INT64:
      typeChar = 'L';
      break;
#else
    case VTK_PARSE_UNSIGNED_LONG_LONG:
    case VTK_PARSE_UNSIGNED___INT64:
    case VTK_PARSE_LONG_LONG:
    case VTK_PARSE___INT64:
      typeChar = 'l';
      break;
#endif
    case VTK_PARSE_SIGNED_CHAR:
      typeChar = 'b';
      break;
    case VTK_PARSE_CHAR:
      typeChar = 'c';
      break;
    case VTK_PARSE_UNSIGNED_CHAR:
      typeChar = 'b';
      break;
    case VTK_PARSE_BOOL:
      typeChar = 'O';
      break;
    case VTK_PARSE_STRING:
      typeChar = 's';
      break;
    case VTK_PARSE_UNICODE_STRING:
      typeChar = 'O';
      break;
    default:
      typeChar = 'O';
      break;
    }

  return typeChar;
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
    result[currPos++] = vtkWrapPython_FormatChar(argtype);

    if ((argtype & VTK_PARSE_INDIRECT) == VTK_PARSE_POINTER &&
        argtype != VTK_PARSE_OBJECT_PTR && argtype != VTK_PARSE_QOBJECT_PTR)
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

        if (currentFunction->ArgCounts[i] == -1)
          {
          result[currPos++] = 'O';
          }
        else
          {
          result[currPos++] = '(';
          for (j = 0; j < currentFunction->ArgCounts[i]; j++)
            {
            result[currPos++] = typeChar;
            }
          result[currPos++] = ')';
          }
        }
      }
    }

  result[currPos++] = '\0';
  return result;
}

/* -------------------------------------------------------------------- */
/* Create a string to describe the signature of a method.
 * If isvtkobject is set the string will start with an "at" symbol.
 * Following the optional space will be a ParseTuple format string,
 * followed by the names of any VTK classes required.  The optional
 * "at" symbol indicates that methods like vtkClass.Method(self, arg1,...)
 * are possible, so the "at" is a placeholder for "self". */

static char *vtkWrapPython_ArgCheckString(
  int isvtkobjmethod, FunctionInfo *currentFunction)
{
  static char result[1024];
  size_t currPos = 0;
  unsigned int argtype;
  int i;

  if (currentFunction->IsExplicit)
    {
    result[currPos++] = '-';
    }

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

    else if (argtype == VTK_PARSE_UNICODE_STRING ||
        argtype == VTK_PARSE_UNICODE_STRING_REF)
      {
      strcpy(&result[currPos], " unicode");
      currPos += 8;
      }

    else if (argtype == VTK_PARSE_OBJECT_REF ||
        argtype == VTK_PARSE_OBJECT_PTR ||
        argtype == VTK_PARSE_OBJECT ||
        argtype == VTK_PARSE_QOBJECT ||
        argtype == VTK_PARSE_QOBJECT_REF ||
        argtype == VTK_PARSE_QOBJECT_PTR)
      {
      result[currPos++] = ' ';
      if (argtype == VTK_PARSE_OBJECT_REF ||
          argtype == VTK_PARSE_QOBJECT_REF)
        {
        result[currPos++] = '&';
        }
      else if (argtype == VTK_PARSE_OBJECT_PTR ||
               argtype == VTK_PARSE_QOBJECT_PTR)
        {
        result[currPos++] = '*';
        }
      strcpy(&result[currPos], currentFunction->ArgClasses[i]);
      currPos += strlen(currentFunction->ArgClasses[i]);
      }

    else if (currentFunction->ArgCounts[i] == -1)
      {
      result[currPos++] = ' ';
      result[currPos++] = '*';
      result[currPos++] = vtkWrapPython_FormatChar(argtype);
      result[currPos] = '\0';
      }
    }

  return result;
}

/* -------------------------------------------------------------------- */
/* For the purpose of the python docstrings, convert special characters
 * in a string into their escape codes, so that the string can be quoted
 * in a source file (the specified maxlen must be at least 32 chars, and
 * should not be over 2047 since that is the maximum length of a string
 * literal on some systems)*/

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
/* A simple string that grows as necessary. */

struct vtkWPString
{
  char *str;
  size_t len;
  size_t maxlen;
};

/* -- append ---------- */
static void vtkWPString_Append(
  struct vtkWPString *str, const char *text)
{
  size_t n = strlen(text);

  if (str->len + n + 1 > str->maxlen)
    {
    str->maxlen = (str->len + n + 1 + 2*str->maxlen);
    str->str = (char *)realloc(str->str, str->maxlen);
    }

  strncpy(&str->str[str->len], text, n);
  str->len += n;
  str->str[str->len] = '\0';
}

/* -- add a char ---------- */
static void vtkWPString_PushChar(
  struct vtkWPString *str, char c)
{
  if (str->len + 2 > str->maxlen)
    {
    str->maxlen = (str->len + 2 + 2*str->maxlen);
    str->str = (char *)realloc(str->str, str->maxlen);
    }

  str->str[str->len++] = c;
  str->str[str->len] = '\0';
}

/* -- strip any of the given chars from the end ---------- */
static void vtkWPString_Strip(
  struct vtkWPString *str, const char *trailers)
{
  size_t k = str->len;
  char *cp = str->str;
  size_t j = 0;
  size_t n;

  if (cp)
    {
    n = strlen(trailers);

    while (k > 0 && j < n)
      {
      for (j = 0; j < n; j++)
        {
        if (cp[k-1] == trailers[j])
          {
          k--;
          break;
          }
        }
      }

    str->len = k;
    str->str[k] = '\0';
    }
}

/* -- Return the last char ---------- */
static char vtkWPString_LastChar(
  struct vtkWPString *str)
{
  if (str->str && str->len > 0)
    {
    return str->str[str->len-1];
    }
  return '\0';
}

/* -- do a linebreak on a method declaration ---------- */
static void vtkWPString_BreakSignatureLine(
  struct vtkWPString *str, size_t *linestart, size_t indentation)
{
  size_t i = 0;
  size_t m = 0;
  size_t j = *linestart;
  size_t l = str->len;
  size_t k = str->len;
  char *text = str->str;
  char delim;

  if (!text)
    {
    return;
    }

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
    /* Append some chars to guarantee size */
    vtkWPString_PushChar(str, '\n');
    vtkWPString_PushChar(str, '\n');
    for (i = 0; i < indentation; i++)
      {
      vtkWPString_PushChar(str, ' ');
      }
    /* re-get the char pointer, it may have been reallocated */
    text = str->str;

    if (k > l)
      {
      m = 0;
      while (m < indentation+2 && text[l+m] == ' ')
        {
        m++;
        }
      memmove(&text[l+indentation+2-m], &text[l], k-l);
      k += indentation+2-m;
      }
    else
      {
      k += indentation+2;
      }
    text[l++] = '\\'; text[l++] = 'n';
    j = l;
    for (i = 0; i < indentation; i++)
      {
      text[l++] = ' ';
      }
    }

  str->len = k;

  /* return the new line start position */
  *linestart = j;
}

/* -- do a linebreak on regular text ---------- */
static void vtkWPString_BreakCommentLine(
  struct vtkWPString *str, size_t *linestart, size_t indent)
{
  size_t i = 0;
  size_t j = *linestart;
  size_t l = str->len;
  char *text = str->str;

  if (!text)
    {
    return;
    }

  /* try to break the line at a word */
  while (l > 0 && text[l-1] != ' ' && text[l-1] != '\n')
    {
    l--;
    }
  if (l > 0 && text[l-1] != '\n' && l-j > indent)
    {
    /* replace space with newline */
    text[l-1] = '\n';
    j = l;

    /* Append some chars to guarantee size */
    vtkWPString_PushChar(str, '\n');
    vtkWPString_PushChar(str, '\n');
    for (i = 0; i < indent; i++)
      {
      vtkWPString_PushChar(str, ' ');
      }
    /* re-get the char pointer, it may have been reallocated */
    text = str->str;
    str->len -= indent+2;

    if (str->len > l && indent > 0)
      {
      memmove(&text[l+indent], &text[l], str->len-l);
      memset(&text[l], ' ', indent);
      str->len += indent;
      }
    }
  else
    {
    /* long word, just split the word */
    vtkWPString_PushChar(str, '\n');
    j = str->len;
    for (i = 0; i < indent; i++)
      {
      vtkWPString_PushChar(str, ' ');
      }
    }

  /* return the new line start position */
  *linestart = j;
}

/* -------------------------------------------------------------------- */
/* Create a signature for the python version of a method. */

static const char *vtkWrapPython_PythonSignature(
  FunctionInfo *currentFunction)
{
  /* string is intentionally not freed until the program exits */
  static struct vtkWPString staticString = { NULL, 0, 0 };
  struct vtkWPString *result;
  unsigned int argtype;
  int i, j;

  result = &staticString;
  result->len = 0;

  /* print out the name of the method */
  vtkWPString_Append(result, "V.");
  vtkWPString_Append(result, currentFunction->Name);

  /* print the arg list */
  vtkWPString_Append(result, "(");

  for (i = 0; i < currentFunction->NumberOfArguments; i++)
    {
    if (currentFunction->ArgTypes[i] == VTK_PARSE_FUNCTION)
      {
      vtkWPString_Append(result, "function");
      }

    argtype = (currentFunction->ArgTypes[i] & VTK_PARSE_UNQUALIFIED_TYPE);

    if (i != 0)
      {
      vtkWPString_Append(result, ", ");
      }

    switch (argtype)
      {
      case VTK_PARSE_FLOAT_PTR:
      case VTK_PARSE_DOUBLE_PTR:
        vtkWPString_Append(result, "(");
        for (j = 0; j < currentFunction->ArgCounts[i]; j++)
          {
          if (j != 0)
            {
            vtkWPString_Append(result, ", ");
            }
          vtkWPString_Append(result, "float");
          }
        vtkWPString_Append(result, ")");
        break;
      case VTK_PARSE_INT_PTR:
        vtkWPString_Append(result, "(");
        for (j = 0; j < currentFunction->ArgCounts[i]; j++)
          {
          if (j != 0)
            {
            vtkWPString_Append(result, ", ");
            }
          vtkWPString_Append(result, "int");
          }
        vtkWPString_Append(result, ")");
        break;
      case VTK_PARSE_ID_TYPE_PTR:
        vtkWPString_Append(result, "(");
        for (j = 0; j < currentFunction->ArgCounts[i]; j++)
          {
          if (j != 0)
            {
            vtkWPString_Append(result, ", ");
            }
#if defined(VTK_USE_64BIT_IDS) && (VTK_SIZEOF_LONG != VTK_SIZEOF_ID_TYPE)
          vtkWPString_Append(result, "long");
#else
          vtkWPString_Append(result, "int");
#endif
          }
        vtkWPString_Append(result, ")");
        break;
      case VTK_PARSE_LONG_LONG_PTR:
      case VTK_PARSE___INT64_PTR:
        vtkWPString_Append(result, "(");
        for (j = 0; j < currentFunction->ArgCounts[i]; j++)
          {
          if (j != 0)
            {
            vtkWPString_Append(result, ", ");
            }
          vtkWPString_Append(result, "long");
          }
        vtkWPString_Append(result, ")");
        break;
      case VTK_PARSE_OBJECT_REF:
      case VTK_PARSE_OBJECT_PTR:
      case VTK_PARSE_OBJECT:
        vtkWPString_Append(result, currentFunction->ArgClasses[i]);
        break;
      case VTK_PARSE_VOID_PTR:
      case VTK_PARSE_CHAR_PTR:
        vtkWPString_Append(result, "string");
        break;
      case VTK_PARSE_FLOAT:
      case VTK_PARSE_DOUBLE:
        vtkWPString_Append(result, "float");
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
        vtkWPString_Append(result, "int");
        break;
      case VTK_PARSE_CHAR:
        vtkWPString_Append(result, "char");
        break;
      case VTK_PARSE_UNSIGNED_CHAR:
        vtkWPString_Append(result, "int");
        break;
      case VTK_PARSE_BOOL:
        vtkWPString_Append(result, "bool");
        break;
      case VTK_PARSE_STRING:
      case VTK_PARSE_STRING_REF:
        vtkWPString_Append(result, "string");
        break;
      case VTK_PARSE_UNICODE_STRING:
      case VTK_PARSE_UNICODE_STRING_REF:
        vtkWPString_Append(result, "unicode");
        break;
      }
    }

  vtkWPString_Append(result, ")");

  /* if this is a void method, we are finished */
  /* otherwise, print "->" and the return type */
  if (!((currentFunction->ReturnType & VTK_PARSE_BASE_TYPE) == VTK_PARSE_VOID)
      || (currentFunction->ReturnType & VTK_PARSE_INDIRECT))
    {
    vtkWPString_Append(result, " -> ");

    switch (currentFunction->ReturnType & VTK_PARSE_UNQUALIFIED_TYPE)
      {
      case VTK_PARSE_VOID_PTR:
      case VTK_PARSE_CHAR_PTR:
        vtkWPString_Append(result, "string");
        break;
      case VTK_PARSE_OBJECT_REF:
      case VTK_PARSE_OBJECT_PTR:
      case VTK_PARSE_OBJECT:
        vtkWPString_Append(result, currentFunction->ReturnClass);
        break;
      case VTK_PARSE_FLOAT_PTR:
      case VTK_PARSE_DOUBLE_PTR:
        vtkWPString_Append(result, "(");
        for (j = 0; j < currentFunction->HintSize; j++)
          {
          if (j != 0)
            {
            vtkWPString_Append(result, ", ");
            }
          vtkWPString_Append(result, "float");
          }
        vtkWPString_Append(result, ")");
        break;
      case VTK_PARSE_INT_PTR:
        vtkWPString_Append(result, "(");
        for (j = 0; j < currentFunction->HintSize; j++)
          {
          if (j != 0)
            {
            vtkWPString_Append(result, ", ");
            }
          vtkWPString_Append(result, "int");
          }
        vtkWPString_Append(result, ")");
        break;
      case VTK_PARSE_ID_TYPE_PTR:
        vtkWPString_Append(result, "(");
        for (j = 0; j < currentFunction->HintSize; j++)
          {
          if (j != 0)
            {
            vtkWPString_Append(result, ", ");
            }
#if defined(VTK_USE_64BIT_IDS) && (VTK_SIZEOF_LONG != VTK_SIZEOF_ID_TYPE)
          vtkWPString_Append(result, "long");
#else
          vtkWPString_Append(result, "int");
#endif
          }
        vtkWPString_Append(result, ")");
        break;
      case VTK_PARSE_LONG_LONG_PTR:
      case VTK_PARSE___INT64_PTR:
        vtkWPString_Append(result, "(");
        for (j = 0; j < currentFunction->HintSize; j++)
          {
          if (j != 0)
            {
            vtkWPString_Append(result, ", ");
            }
          vtkWPString_Append(result, "long");
          }
        vtkWPString_Append(result, ")");
        break;
      case VTK_PARSE_FLOAT:
      case VTK_PARSE_DOUBLE:
        vtkWPString_Append(result, "float");
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
        vtkWPString_Append(result, "int");
        break;
      case VTK_PARSE_CHAR:
        vtkWPString_Append(result, "char");
        break;
      case VTK_PARSE_BOOL:
        vtkWPString_Append(result, "bool");
        break;
      case VTK_PARSE_STRING:
        vtkWPString_Append(result, "string");
        break;
      case VTK_PARSE_UNICODE_STRING:
        vtkWPString_Append(result, "unicode");
        break;

      }
    }

  if (currentFunction->Signature)
    {
    vtkWPString_Append(result, "\nC++: ");
    vtkWPString_Append(result, currentFunction->Signature);
    }

  return result->str;
}

/* -------------------------------------------------------------------- */
/* Format a signature to a 70 char linewidth and char limit */
const char *vtkWrapPython_FormatSignature(
  const char *signature, size_t width, size_t maxlen)
{
  static struct vtkWPString staticString = { NULL, 0, 0 };
  struct vtkWPString *text;
  size_t i, j, n;
  const char *cp = signature;
  char delim;
  size_t lastSigStart = 0;
  size_t sigCount = 0;

  text = &staticString;
  text->len = 0;

  if (signature == 0)
    {
    return "";
    }

  i = 0;
  j = 0;

  while (cp[i] != '\0')
    {
    while (text->len - j < width && cp[i] != '\n' && cp[i] != '\0')
      {
      /* escape quotes */
      if (cp[i] == '\"' || cp[i] == '\'')
        {
        delim = cp[i];
        vtkWPString_PushChar(text, '\\');
        vtkWPString_PushChar(text, cp[i++]);
        while (cp[i] != delim && cp[i] != '\0')
          {
          if (cp[i] == '\\')
            {
            vtkWPString_PushChar(text, '\\');
            }
          vtkWPString_PushChar(text, cp[i++]);
          }
        if (cp[i] == delim)
          {
          vtkWPString_PushChar(text, '\\');
          vtkWPString_PushChar(text, cp[i++]);
          }
        }
      /* remove items that trail the closing parenthesis */
      else if (cp[i] == ')')
        {
        vtkWPString_PushChar(text, cp[i++]);
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
        vtkWPString_PushChar(text, cp[i++]);
        }
      }

    /* break the line (try to break after a comma) */
    if (cp[i] != '\n' && cp[i] != '\0')
      {
      vtkWPString_BreakSignatureLine(text, &j, 4);
      }
    /* reached end of line: do next signature */
    else
      {
      vtkWPString_Strip(text, " \r\t");
      if (cp[i] != '\0')
        {
        sigCount++;
        /* if sig count is even, check length against maxlen */
        if ((sigCount & 1) == 0)
          {
          n = strlen(text->str);
          if (n >= maxlen)
            {
            break;
            }
          lastSigStart = n;
          }

        i++;
        vtkWPString_PushChar(text, '\\');
        vtkWPString_PushChar(text, 'n');
        }
      /* mark the position of the start of the line */
      j = text->len;
      }
    }

  vtkWPString_Strip(text, " \r\t");

  if (strlen(text->str) >= maxlen)
    {
    /* terminate before the current signature */
    text->str[lastSigStart] = '\0';
    }

  return text->str;
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
  static struct vtkWPString staticString = { NULL, 0, 0 };
  struct vtkWPString *text;
  const char *cp;
  size_t i, j, l;
  size_t indent = 0;
  int nojoin = 0;
  int start;

  text = &staticString;
  text->len = 0;

  if (comment == 0)
    {
    return "";
    }

  i = 0; j = 0; l = 0;
  start = 1;
  cp = comment;

  /* skip any leading whitespace */
  while (cp[i] == '\n' || cp[i] == '\r' ||
         cp[i] == '\t' || cp[i] == ' ')
    {
    i++;
    }

  while (cp[i] != '\0')
    {
    /* Add characters until the output line is complete */
    while (cp[i] != '\0' && text->len-j < width)
      {
      /* if the end of the line was found, see how next line begins */
      if (start)
        {
        /* eat the leading space */
        if (cp[i] == ' ')
          {
          i++;
          }

        /* skip ahead to find any interesting first characters */
        l = i;
        while (cp[l] == ' ' || cp[l] == '\t' || cp[l] == '\r')
          {
          l++;
          }

        /* check for new section */
        if (cp[l] == '.' && strncmp(&cp[l], ".SECTION", 8) == 0)
          {
          vtkWPString_Strip(text, "\n");
          if (text->len > 0)
            {
            vtkWPString_PushChar(text, '\n');
            vtkWPString_PushChar(text, '\n');
            }
          i = l+8;
          while (cp[i] == '\r' || cp[i] == '\t' || cp[i] == ' ')
            {
            i++;
            }
          while (cp[i] != '\n' && cp[i] != '\0')
            {
            vtkWPString_PushChar(text, cp[i++]);
            }
          vtkWPString_Strip(text, " \t\r");

          if (vtkWPString_LastChar(text) != ':')
            {
            vtkWPString_PushChar(text, ':');
            }
          vtkWPString_PushChar(text, '\n');
          vtkWPString_PushChar(text, '\n');
          j = text->len;
          indent = 0;
          if (cp[i] == '\n')
            {
            i++;
            }
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
            if (text->len > 0 && vtkWPString_LastChar(text) != '\n')
              {
              vtkWPString_PushChar(text, '\n');
              }
            j = text->len;
            i = l;
            }
          }

        /* handle bullets and numbering */
        else if (cp[l] == '-' || cp[l] == '*' || cp[l] == '#' ||
                 (cp[l] >= '0' && cp[l] <= '9' &&
                  (cp[l+1] == ')' || cp[l+1] == '.') && cp[l+2] == ' '))
          {
          indent = 0;
          while (indent < 3 && cp[l+indent] != ' ')
            {
            indent++;
            }
          indent++;
          if (text->len > 0 && vtkWPString_LastChar(text) != '\n')
            {
            vtkWPString_PushChar(text, '\n');
            }
          j = text->len;
          i = l;
          }

        /* keep paragraph breaks */
        else if (cp[l] == '\n')
          {
          i = l+1;
          vtkWPString_Strip(text, "\n");
          if (text->len > 0)
            {
            vtkWPString_PushChar(text, '\n');
            vtkWPString_PushChar(text, '\n');
            }
          nojoin = 0;
          indent = 0;
          j = text->len;
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
          vtkWPString_PushChar(text, '\n');
          j = text->len;
          }

        /* do line joining */
        else if (text->len > 0 && vtkWPString_LastChar(text) != '\n')
          {
          i = l;
          vtkWPString_PushChar(text, ' ');
          }
        }

      /* handle quotes */
      if (cp[i] == '\"')
        {
        size_t q = i;
        size_t r = text->len;

        /* try to keep the quote whole */
        vtkWPString_PushChar(text, cp[i++]);
        while (cp[i] != '\"' && cp[i] != '\r' &&
               cp[i] != '\n' && cp[i] != '\0')
          {
          vtkWPString_PushChar(text, cp[i++]);
          }
        /* if line ended before quote did, then reset */
        if (cp[i] != '\"')
          {
          i = q;
          text->len = r;
          }
        }
      else if (cp[i] == '\'')
        {
        size_t q = i;
        size_t r = text->len;

        /* try to keep the quote whole */
        vtkWPString_PushChar(text, cp[i++]);
        while (cp[i] != '\'' && cp[i] != '\r' &&
               cp[i] != '\n' && cp[i] != '\0')
          {
          vtkWPString_PushChar(text, cp[i++]);
          }
        /* if line ended before quote did, then reset */
        if (cp[i] != '\'')
          {
          i = q;
          text->len = r;
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
            vtkWPString_Strip(text, " \n");
            vtkWPString_PushChar(text, '\n');
            vtkWPString_PushChar(text, '\n');
            j = text->len;
            indent = 0;
            }
          i = l+1;
          while (cp[i] == '\r' || cp[i] == '\t' || cp[i] == ' ')
            {
            i++;
            }
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
          if (i > 0 && cp[i-1] != ' ')
            {
            vtkWPString_PushChar(text, ' ');
            }
          if (cp[i+1] == 'f')
            {
            if (cp[i+2] == '$')
              {
              vtkWPString_PushChar(text, '$');
              }
            else
              {
              vtkWPString_PushChar(text, '\\');
              vtkWPString_PushChar(text, cp[i+2]);
              }
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
          vtkWPString_Strip(text, " \n");
          vtkWPString_PushChar(text, '\n');
          vtkWPString_PushChar(text, '\n');
          indent = 0;
          i += 2;
          j = text->len;
          }
        else if (strncmp(&cp[i+1], "code", 4) == 0)
          {
          nojoin = 1;
          i += 5;
          while (cp[i] == ' ' || cp[i] == '\r' ||
                 cp[i] == '\t' || cp[i] == '\n')
            {
            i++;
            }
          }
        else if (strncmp(&cp[i+1], "endcode", 7) == 0)
          {
          nojoin = 0;
          i += 8;
          l = i;
          while (cp[l] == ' ' || cp[l] == '\t' || cp[l] == '\r')
            {
            l++;
            }
          if (cp[l] == '\n')
            {
            i = l;
            vtkWPString_PushChar(text, '\n');
            j = text->len;
            }
          }
        else if (strncmp(&cp[i+1], "verbatim", 8) == 0)
          {
          i += 9;
          while (cp[i] != '\0' && ((cp[i] != '@' && cp[i] != '\\') ||
                 strncmp(&cp[i+1], "endverbatim", 11) != 0))
            {
            if (cp[i] != '\r')
              {
              vtkWPString_PushChar(text, cp[i]);
              }
            if (cp[i] == '\n')
              {
              j = text->len;
              }
            i++;
            }
          if (cp[i] != '\0')
            {
            i += 12;
            }
          }
        }

      /* search for newline */
      start = 0;
      l = i;
      while (cp[l] == ' ' || cp[l] == '\t' || cp[l] == '\r')
        {
        l++;
        }
      if (cp[l] == '\n')
        {
        i = l+1;
        start = 1;
        }

      /* append */
      else if (cp[i] != '\0')
        {
        vtkWPString_PushChar(text, cp[i++]);
        }

      } /* while (cp[i] != '\0' && text->len-j < width) */

    if (cp[i] == '\0')
      {
      break;
      }

    vtkWPString_BreakCommentLine(text, &j, indent);
    }

  /* remove any trailing blank lines */
  vtkWPString_Strip(text, "\n");
  vtkWPString_PushChar(text, '\n');

  return text->str;
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
  FunctionInfo **wrappedFunctions;
  FunctionInfo *theSignature;
  FunctionInfo *theFunc;
  const char *ccp;
  char *cp;
  unsigned int returnType = 0;
  unsigned int argType = 0;
  char typeChar = 0;
  const char *sizeMethod = "GetNumberOfComponents";
  int potential_error = 0;
  int needs_cleanup = 0;
  char on_error[32];

  wrappedFunctions = (FunctionInfo **)malloc(
    data->NumberOfFunctions*sizeof(FunctionInfo *));

  /* output any custom methods */
  vtkWrapPython_CustomMethods(fp, data, do_constructors);

  /* go through all functions and see which are wrappable */
  for (i = 0; i < data->NumberOfFunctions; i++)
    {
    theFunc = data->Functions[i];

    /* add hints for array GetTuple methods */
    if (strcmp(data->Name, "vtkDataArray") == 0 || (hinfo &&
        vtkParseHierarchy_IsTypeOf(hinfo, data->Name, "vtkDataArray")))
      {
      if ((strcmp(theFunc->Name, "GetTuple") == 0 ||
           strcmp(theFunc->Name, "GetTupleValue") == 0) &&
          theFunc->HaveHint == 0 &&
          theFunc->NumberOfArguments == 1 &&
          theFunc->ArgTypes[0] == VTK_PARSE_ID_TYPE &&
          (theFunc->ReturnType & VTK_PARSE_BASE_TYPE) != VTK_PARSE_CHAR)
        {
        theFunc->HaveHint = 1;
        theFunc->HintSize = -1;
        }
      else if ((strcmp(theFunc->Name, "SetTuple") == 0 ||
                strcmp(theFunc->Name, "SetTupleValue") == 0 ||
                strcmp(theFunc->Name, "GetTuple") == 0 ||
                strcmp(theFunc->Name, "GetTupleValue") == 0 ||
                strcmp(theFunc->Name, "InsertTuple") == 0 ||
                strcmp(theFunc->Name, "InsertTupleValue") == 0) &&
               theFunc->NumberOfArguments == 2 &&
               theFunc->ArgTypes[0] == VTK_PARSE_ID_TYPE &&
               theFunc->ArgCounts[1] == 0 &&
               (theFunc->ArgTypes[1] & VTK_PARSE_BASE_TYPE) != VTK_PARSE_CHAR)
        {
        theFunc->ArgCounts[1] = -1;
        }
      else if ((strcmp(theFunc->Name, "InsertNextTuple") == 0 ||
                strcmp(theFunc->Name, "InsertNextTupleValue") == 0) &&
               theFunc->NumberOfArguments == 1 &&
               theFunc->ArgCounts[0] == 0 &&
               (theFunc->ArgTypes[0] & VTK_PARSE_BASE_TYPE) != VTK_PARSE_CHAR)
        {
        theFunc->ArgCounts[0] = -1;
        }
      }

    /* check for wrappability */
    if (vtkWrapPython_MethodCheck(data, theFunc, hinfo) &&
        !vtkWrapPython_IsDestructor(data, theFunc) &&
        (!vtkWrapPython_IsConstructor(data, theFunc) == !do_constructors))
      {
      ccp = vtkWrapPython_PythonSignature(theFunc);
      cp = (char *)malloc(strlen(ccp)+1);
      strcpy(cp, ccp);
      theFunc->Signature = cp;
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

            if ((argType & VTK_PARSE_BASE_TYPE) == VTK_PARSE_OBJECT ||
                (argType & VTK_PARSE_BASE_TYPE) == VTK_PARSE_QOBJECT
                )
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
              if (theSignature->ArgCounts[i] == -1)
                {
                fprintf(fp,", &tempT%d",i);
                }
              else if (theSignature->ArgCounts[i])
                {
                for (j = 0; j < theSignature->ArgCounts[i]; j++)
                  {
                  fprintf(fp,", &temp%d[%d]",i,j);
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
            else if (argType == VTK_PARSE_QOBJECT_PTR ||
                     argType == VTK_PARSE_QOBJECT ||
                     argType == VTK_PARSE_QOBJECT_REF)
              {
              fprintf(fp,
                      "    temp%d = (%s *)vtkPythonUtil::SIPGetPointerFromObject(tempH%d,\"%s\");\n",
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
            else if (theSignature->ArgCounts[i] == -1)
              {
              /* go through the tuple and set the array */
              typeChar = vtkWrapPython_FormatChar(theSignature->ArgTypes[i]);
              if ((theSignature->ArgTypes[i] & VTK_PARSE_BASE_TYPE)
                  == VTK_PARSE_BOOL)
                {
                typeChar = 'i';
                }

              fprintf(fp,
                      "    if (PySequence_Check(tempT%d))\n"
                      "      {\n"
                      "      int typesAreOk = 1;\n"
                      "      Py_ssize_t n = op->%s();\n"
                      "#if PY_MAJOR_VERSION >= 2\n"
                      "      Py_ssize_t m = PySequence_Size(tempT%d);\n"
                      "#else\n"
                      "      Py_ssize_t m = PySequence_Length(tempT%d);\n"
                      "#endif\n"
                      "      if (m > 20)\n"
                      "        {\n"
                      "        PyErr_SetString(PyExc_TypeError,\n"
                      "          \"VTK-python cannot unpack sequences of more than 20 items\");\n"
                      "        }\n",
                      i, sizeMethod, i, i);

              fprintf(fp,
                      "      else if (n == m)\n"
                      "        {\n"
                      "        for (Py_ssize_t i = 0; i < n && typesAreOk; i++)\n"
                      "          {\n"
                      "          PyObject *item = PySequence_GetItem(tempT%d, i);\n"
                      "          if (item == NULL)\n"
                      "            {\n"
                      "            PyErr_Clear();\n"
                      "            PyErr_SetString(PyExc_TypeError, \"cannot get items from sequence\");\n"
                      "            break;\n"
                      "            }\n"
                      "          typesAreOk = PyArg_Parse(item, \"%c\", &temp%d[i]);\n"
                      "          Py_DECREF(item);\n"
                      "          }\n"
                      "        }\n",
                      i, typeChar, i);

              fprintf(fp,
                      "      else\n"
                      "        {\n"
                      "        char text[128];\n"
                      "        sprintf(text, \"must be a sequence of length %%li, not %%li\", (long)n, (long)m);\n"
                      "        PyErr_SetString(PyExc_TypeError, text);\n"
                      "        }\n"
                      "      }\n"
                      "    else\n"
                      "      {\n"
                      "      PyErr_SetString(PyExc_TypeError, \"a sequence is required\");\n"
                      "      }\n"
                      "    if (PyErr_Occurred())\n"
                      "      {\n"
                      "      %s;\n"
                      "      }\n"
                      "  \n",
                      on_error);
              }
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
                  argType == VTK_PARSE_OBJECT ||
                  argType == VTK_PARSE_QOBJECT_REF ||
                  argType == VTK_PARSE_QOBJECT)
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
                      "    if (vtkPythonUtil::CheckArray(args,%d,temp%d,",
                      i, i);

              if (theSignature->ArgCounts[i] == -1)
                {
                fprintf(fp, "\n"
                        "          op->%s()", sizeMethod);
                }
              else
                {
                fprintf(fp, "%d", theSignature->ArgCounts[i]);
                }

              fprintf(fp, "))\n"
                      "      {\n"
                      "      %s;\n"
                      "      }\n",
                      on_error);

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
                  "  {NULL, Py%s_%s%s, 1,\n"
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
          cp = (char *)malloc(siglen+2+ strlen(theSignature->Signature));
          strcpy(cp, theFunc->Signature);
          strcpy(&cp[siglen],"\n");
          strcpy(&cp[siglen+1], theSignature->Signature);
          theFunc->Signature = cp;
          }
        }
      } /* is this method non NULL */
    } /* loop over all methods */

  /* the method table for constructors is produced elsewhere */
  if (do_constructors)
    {
    free(wrappedFunctions);
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
      /* string literals must be under 2048 chars */
      size_t maxlen = 2040;
      const char *comment;
      const char *signatures;

      /* format the comment nicely to a 66 char width */
      signatures = vtkWrapPython_FormatSignature(
        wrappedFunctions[fnum]->Signature, 66, maxlen - 32);
      comment = vtkWrapPython_FormatComment(
        wrappedFunctions[fnum]->Comment, 66);
      comment = vtkWrapPython_QuoteString(
        comment, maxlen - strlen(signatures));

      fprintf(fp,
              "  {(char*)\"%s\", Py%s_%s, 1,\n",
              wrappedFunctions[fnum]->Name, data->Name,
              wrappedFunctions[fnum]->Name);

      fprintf(fp,
              "   (char*)\"%s\\n\\n%s\"},\n",
              signatures, comment);
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
            "  {(char*)\"AddObserver\",  Py%s_AddObserver, 1,\n"
            "   (char*)\"V.AddObserver(int, function) -> int\\nC++: unsigned long AddObserver(const char *event,\\n    vtkCommand *command, float priority=0.0f)\\n\\nAdd an event callback function(vtkObject, int) for an event type.\\nReturns a handle that can be used with RemoveEvent(int).\"},\n",
            data->Name);
    }

  /* vtkObjectBase needs entries for GetAddressAsString and PrintRevisions */
  else if (strcmp("vtkObjectBase", data->Name) == 0)
    {
    fprintf(fp,
            "  {(char*)\"GetAddressAsString\",  Py%s_GetAddressAsString, 1,\n"
            "   (char*)\"V.GetAddressAsString(string) -> string\\nC++: const char *GetAddressAsString()\\n\\nGet address of C++ object in format 'Addr=%%p' after casting to\\nthe specified type.  You can get the same information from o.__this__.\"},\n"
            "  {(char*)\"PrintRevisions\",  Py%s_PrintRevisions, 1,\n"
            "   (char*)\"V.PrintRevisions() -> string\\nC++: const char *PrintRevisions()\\n\\nPrints the .cxx file CVS revisions of the classes in the\\nobject's inheritance chain.\"},\n",
            data->Name, data->Name);
    }

  /* python expects the method table to end with a "NULL" entry */
  fprintf(fp,
          "  {NULL,                       NULL, 0, NULL}\n"
          "};\n"
          "\n");

  free(wrappedFunctions);
}

/* -------------------------------------------------------------------- */
static int vtkWrapPython_IsDestructor(
  ClassInfo *data, FunctionInfo *currentFunction)
{
  size_t i;
  const char *cp;

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
    VTK_PARSE_QOBJECT,
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
        (currentFunction->ArgCounts[i] == 0) &&
        (argType != VTK_PARSE_OBJECT_PTR) &&
        (argType != VTK_PARSE_QOBJECT_PTR) &&
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
  if (args_ok)
    {
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

  /* only consider superclasses that are wrapped */
  for (j = 0; j < data->NumberOfSuperClasses; j++)
    {
    if (vtkWrapPython_IsClassWrapped(data->SuperClasses[j], hinfo))
      {
      break;
      }
    }

  if (j < data->NumberOfSuperClasses)
    {
    fprintf(fp,
            "    \"Superclass: %s\\n\\n\",\n",
            vtkWrapPython_QuoteString(data->SuperClasses[j], 500));
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
                  data->Functions[j]->Signature, 70, 2000));
        }
      }
    }
}

/* -------------------------------------------------------------------- */
/* generate includes for any special types that are used */
static void vtkWrapPython_GenerateSpecialHeaders(
  FILE *fp, ClassInfo *data, HierarchyInfo *hinfo)
{
  const char **types;
  int numTypes = 0;
  FunctionInfo *currentFunction;
  int i, j, k, n, m;
  unsigned int aType;
  const char *classname;
  const char *ownincfile = "";

  types = (const char **)malloc(1000*sizeof(const char *));

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
            if (numTypes > 0 && (numTypes % 1000) == 0)
              {
              types = (const char **)realloc((char **)types,
                (numTypes + 1000)*sizeof(const char *));
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
    /* look up the header file, automatic if "hinfo" is present */
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
    else if (strcmp(types[i], "vtkArrayExtentsList") == 0)
      {
      incfile = "vtkArrayExtentsList.h";
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

  free((char **)types);
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
  FILE *fp, ClassInfo *data, HierarchyInfo *hinfo, int class_has_new)
{
  int i;

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

  /* find the first superclass that is a VTK class */
  for (i = 0; i < data->NumberOfSuperClasses; i++)
    {
    if (vtkWrapPython_IsClassWrapped(data->SuperClasses[i], hinfo))
      {
      if (hinfo == 0 || vtkParseHierarchy_IsTypeOf(
                          hinfo, data->SuperClasses[i], "vtkObjectBase"))
        {
        break;
        }
      }
    }

  if (i < data->NumberOfSuperClasses)
    {
    fprintf(fp, "\n"
            "                        PyVTKClass_%sNew(modulename));\n",
            data->SuperClasses[i]);
    }
  else
    {
    fprintf(fp, "0);\n");
    }

  fprintf(fp,
          "}\n"
          "\n");
}

/* -------------------------------------------------------------------- */
/* generate extra functions for a special object */
static void vtkWrapPython_SpecialObjectProtocols(
  FILE *fp, ClassInfo *data, FileInfo *finfo, SpecialTypeInfo *info)
{
  static const char *compare_consts[6] = {
    "Py_LT", "Py_LE", "Py_EQ", "Py_NE", "Py_GT", "Py_GE" };
  static const char *compare_tokens[6] = {
    "<", "<=", "==", "!=", ">", ">=" };
  int compare_ops = 0;
  int i;
  FunctionInfo *func;

  /* clear all info about the type */
  info->has_print = 0;
  info->has_compare = 0;

  /* the new method for python versions >= 2.2 */
  fprintf(fp,
    "#if PY_VERSION_HEX >= 0x02020000\n"
    "static PyObject *Py%s_New(PyTypeObject *,"
    "  PyObject *args, PyObject *)\n"
    "{\n"
    "  PyMethodDef *methods = Py%s_%sMethods;\n"
    "  return vtkPythonUtil::CallOverloadedMethod(methods, NULL, args);\n"
    "}\n"
    "#endif\n"
    "\n",
    data->Name, data->Name, data->Name);

  /* the delete method */
  fprintf(fp,
    "static void Py%s_Delete(PyObject *self)\n"
    "{\n"
    "  PyVTKSpecialObject *obj = (PyVTKSpecialObject *)self;\n"
    "  if (obj->vtk_ptr)\n"
    "    {\n"
    "    delete static_cast<%s *>(obj->vtk_ptr);\n"
    "    }\n"
    "#if PY_MAJOR_VERSION >= 2\n"
    "  PyObject_Del(self);\n"
    "#else\n"
    "  PyMem_DEL(self);\n"
    "#endif\n"
    "}\n"
    "\n",
    data->Name, data->Name);

  /* look in the file for "operator<<" for printing */
  for (i = 0; i < finfo->Contents->NumberOfFunctions; i++)
    {
    func = finfo->Contents->Functions[i];
    if (func->Name && func->IsOperator &&
        strcmp(func->Name, "operator<<") == 0)
      {
      if (func->NumberOfArguments == 2 &&
          (func->ArgTypes[0] & VTK_PARSE_UNQUALIFIED_TYPE) ==
              VTK_PARSE_OSTREAM_REF &&
          (func->ArgTypes[1] & VTK_PARSE_BASE_TYPE) ==
              VTK_PARSE_OBJECT &&
          (func->ArgTypes[1] & VTK_PARSE_POINTER_MASK) == 0 &&
          strcmp(func->ArgClasses[1], data->Name) == 0)
        {
        info->has_print = 1;
        }
      }
    }

  /* the repr function */
  fprintf(fp,
    "static PyObject *Py%s_Repr(PyObject *self)\n"
    "{\n"
    "  PyVTKSpecialObject *obj = (PyVTKSpecialObject *)self;\n"
    "  vtksys_ios::ostringstream os;\n"
    "  const char *name = self->ob_type->tp_name;\n"
    "  os << \"(\" << name << \")\";\n"
    "  if (obj->vtk_ptr)\n"
    "    {\n"
    "    os << %sstatic_cast<const %s *>(obj->vtk_ptr);\n"
    "    }\n"
    "  const vtksys_stl::string &s = os.str();\n"
    "  return PyString_FromStringAndSize(s.data(), s.size());\n"
    "}\n"
    "\n",
    data->Name, (info->has_print ? "*" : ""), data->Name);

  /* the str function */
  if (info->has_print)
    {
    fprintf(fp,
      "static PyObject *Py%s_String(PyObject *self)\n"
      "{\n"
      "  PyVTKSpecialObject *obj = (PyVTKSpecialObject *)self;\n"
      "  vtksys_ios::ostringstream os;\n"
      "  if (obj->vtk_ptr)\n"
      "    {\n"
      "    os << *static_cast<const %s *>(obj->vtk_ptr);\n"
      "    }\n"
      "  const vtksys_stl::string &s = os.str();\n"
      "  return PyString_FromStringAndSize(s.data(), s.size());\n"
      "}\n"
      "\n",
      data->Name, data->Name);
    }

  /* look for comparison operator methods */
  compare_ops = 0;
  for (i = 0; i < data->NumberOfFunctions; i++)
    {
    func = data->Functions[i];
    if (func->IsOperator && func->Name != NULL)
      {
      if (strcmp(func->Name, "operator<") == 0)
        {
        compare_ops = (compare_ops | (1 << 0));
        }
      else if (strcmp(func->Name, "operator<=") == 0)
        {
        compare_ops = (compare_ops | (1 << 1));
        }
      else if (strcmp(func->Name, "operator==") == 0)
        {
        compare_ops = (compare_ops | (1 << 2));
        }
      else if (strcmp(func->Name, "operator!=") == 0)
        {
        compare_ops = (compare_ops | (1 << 3));
        }
      else if (strcmp(func->Name, "operator>") == 0)
        {
        compare_ops = (compare_ops | (1 << 4));
        }
      else if (strcmp(func->Name, "operator>=") == 0)
        {
        compare_ops = (compare_ops | (1 << 5));
        }
      }
    }

  /* the compare function */
  if (compare_ops != 0)
    {
    info->has_compare = 1;

    fprintf(fp,
      "#if PY_VERSION_HEX >= 0x02010000\n"
      "static PyObject *Py%s_RichCompare(\n"
      "  PyObject *o1, PyObject *o2, int opid)\n"
      "{\n"
      "  PyObject *n1 = NULL;\n"
      "  PyObject *n2 = NULL;\n"
      "  const %s *so1 = NULL;\n"
      "  const %s *so2 = NULL;\n"
      "  int result = -1;\n"
      "\n",
      data->Name, data->Name, data->Name);

    for (i = 1; i <= 2; i++)
      {
      /* use GetPointerFromSpecialObject to do type conversion, but
       * at least one of the args will already be the correct type */
      fprintf(fp,
        "  if (o%d->ob_type == &Py%s_Type)\n"
        "    {\n"
        "    PyVTKSpecialObject *s%d = (PyVTKSpecialObject *)o%d;\n"
        "    so%d = static_cast<const %s *>(s%d->vtk_ptr);\n"
        "    }\n"
        "  else\n"
        "    {\n"
        "    so%d = static_cast<const %s *>(\n"
        "      vtkPythonUtil::GetPointerFromSpecialObject(\n"
        "        o%d, \"%s\", &n%d));\n"
        "    if (so%d == NULL)\n"
        "      {\n"
        "      PyErr_Clear();\n"
        "      Py_INCREF(Py_NotImplemented);\n"
        "      return Py_NotImplemented;\n"
        "      }\n"
        "    }\n"
        "\n",
        i, data->Name, i, i, i, data->Name, i, i, data->Name,
        i, data->Name, i, i);
      }

    /* the switch statement for all possible compare ops */
    fprintf(fp,
      "  switch (opid)\n"
      "    {\n");

    for (i = 0; i < 6; i++)
      {
      if ( ((compare_ops >> i) & 1) != 0 )
        {
        fprintf(fp,
          "    case %s:\n"
          "      result = ((*so1) %s (*so2));\n"
          "      break;\n",
          compare_consts[i], compare_tokens[i]);
        }
      else
        {
        fprintf(fp,
          "    case %s:\n"
          "      break;\n",
          compare_consts[i]);
        }
      }

    fprintf(fp,
      "    }\n"
      "\n");

    /* delete temporary objects, there will be at most one */
    fprintf(fp,
      "  if (n1)\n"
      "    {\n"
      "    Py_DECREF(n1);\n"
      "    }\n"
      "  else if (n2)\n"
      "    {\n"
      "    Py_DECREF(n2);\n"
      "    }\n"
      "\n");

    /* return the result */
    fprintf(fp,
      "  if (result == 0)\n"
      "    {\n"
      "    Py_INCREF(Py_False);\n"
      "    return Py_False;\n"
      "    }\n"
      "  else if (result != -1)\n"
      "    {\n"
      "    Py_INCREF(Py_True);\n"
      "    return Py_True;\n"
      "    }\n"
      "\n"
      "  PyErr_SetString(PyExc_TypeError, (char *)\"operation not available\");\n"
      "  return NULL;\n"
      "}\n"
      "#endif\n"
      "\n");
    }

  /* the hash function, defined only for specific types */
  fprintf(fp,
    "static long Py%s_Hash(PyObject *self)\n",
    data->Name);

  if (strcmp(data->Name, "vtkTimeStamp") == 0)
    {
    /* hash for vtkTimeStamp is just the timestamp itself */
    fprintf(fp,
      "{\n"
      "  PyVTKSpecialObject *obj = (PyVTKSpecialObject *)self;\n"
      "  const vtkTimeStamp *op = static_cast<const vtkTimeStamp *>(obj->vtk_ptr);\n"
      "  unsigned long mtime = *op;\n"
      "  long h = (long)mtime;\n"
      "  if (h != -1) { return h; }\n"
      "  return -2;\n"
      "}\n"
      "\n");
    }
  else if (strcmp(data->Name, "vtkVariant") == 0)
    {
    /* hash for vtkVariant is cached to avoid recomputation, this is
     * safe because vtkVariant is an immutable object, and is necessary
     * because computing the hash for vtkVariant is very expensive */
    fprintf(fp,
      "{\n"
      "  PyVTKSpecialObject *obj = (PyVTKSpecialObject *)self;\n"
      "  const vtkVariant *op = static_cast<const vtkVariant *>(obj->vtk_ptr);\n"
      "  long h = obj->vtk_hash;\n"
      "  if (h != -1)\n"
      "    {\n"
      "    return h;\n"
      "    }\n"
      "  h = vtkPythonUtil::VariantHash(op);\n"
      "  obj->vtk_hash = h;\n"
      "  return h;\n"
      "}\n"
      "\n");
    }
  else
    {
    /* if hash is not implemented, raise an exception */
    fprintf(fp,
      "{\n"
      "#if PY_VERSION_HEX >= 0x020600B2\n"
      "  return PyObject_HashNotImplemented(self);\n"
      "#else\n"
      "  char text[256];\n"
      "  sprintf(text, \"unhashable type: \'%%s\'\", self->ob_type->tp_name);\n"
      "  PyErr_SetString(PyExc_TypeError, text);\n"
      "  return -1;\n"
      "#endif\n"
      "}\n"
      "\n");
    }
}

/* -------------------------------------------------------------------- */
/* write out a special type object */
static void vtkWrapPython_GenerateSpecialType(
  FILE *fp, ClassInfo *data, FileInfo *finfo, HierarchyInfo *hinfo)
{
  SpecialTypeInfo info;

  /* forward declaration of the type object */
  fprintf(fp,
    "extern PyTypeObject Py%s_Type;\n"
    "\n",
    data->Name);

  /* generate all constructor methods */
  vtkWrapPython_GenerateMethods(fp, data, hinfo, 0, 1);

  /* generate the method table for the New method */
  fprintf(fp,
    "static PyMethodDef Py%sNewMethod = \\\n"
    "{ (char*)\"%s\", Py%s_%s, 1,\n"
    "  (char*)\"\" };\n"
    "\n",
    data->Name, data->Name, data->Name,
    data->Name);

  /* generate all functions and protocols needed for the type */
  vtkWrapPython_SpecialObjectProtocols(fp, data, finfo, &info);

  /* Generate the TypeObject */
  fprintf(fp,
    "PyTypeObject Py%s_Type = {\n"
    "  PyObject_HEAD_INIT(&PyType_Type)\n"
    "  0,\n"
    "  (char*)\"%s\", // tp_name\n"
    "  sizeof(PyVTKSpecialObject), // tp_basicsize\n"
    "  0, // tp_itemsize\n"
    "  Py%s_Delete, // tp_dealloc\n"
    "  0, // tp_print\n"
    "  0, // tp_getattr\n"
    "  0, // tp_setattr\n"
    "  0, // tp_compare\n"
    "  Py%s_Repr, // tp_repr\n",
    data->Name, data->Name, data->Name, data->Name);

  fprintf(fp,
    "  0, // tp_as_number\n"
    "  0, // tp_as_sequence\n"
    "  0, // tp_as_mapping\n"
    "  Py%s_Hash, // tp_hash\n"
    "  0, // tp_call\n",
    data->Name);

  if (info.has_print)
    {
    fprintf(fp,
      "  Py%s_String, // tp_str\n",
      data->Name);
    }
  else
    {
    fprintf(fp,
      "  0, // tp_str\n");
    }

  fprintf(fp,
    "#if PY_VERSION_HEX >= 0x02020000\n"
    "  PyObject_GenericGetAttr, // tp_getattro\n"
    "#else\n"
    "  PyVTKSpecialObject_GetAttr, // tp_getattro\n"
    "#endif\n"
    "  0, // tp_setattro\n"
    "  0, // tp_as_buffer\n"
    "  Py_TPFLAGS_DEFAULT, // tp_flags\n"
    "  0, // tp_doc\n"
    "  0, // tp_traverse\n"
    "  0, // tp_clear\n");

  if (info.has_compare)
    {
    fprintf(fp,
      "#if PY_VERSION_HEX >= 0x02010000\n"
      "  Py%s_RichCompare, // tp_richcompare\n"
      "#else\n"
      "  0, // tp_richcompare\n"
      "#endif\n",
      data->Name);
    }
  else
    {
    fprintf(fp,
      "  0, // tp_richcompare\n");
    }

  fprintf(fp,
    "  0, // tp_weaklistoffset\n"
    "#if PY_VERSION_HEX >= 0x02020000\n"
    "  0, // tp_iter\n"
    "  0, // tp_iternext\n");

  /* class methods introduced in python 2.2 */
  fprintf(fp,
    "  Py%sMethods, // tp_methods\n"
    "  0, // tp_members\n"
    "  0, // tp_getset\n"
    "  0, // tp_base\n"
    "  0, // tp_dict\n"
    "  0, // tp_descr_get\n"
    "  0, // tp_descr_set\n"
    "  0, // tp_dictoffset\n"
    "  0, // tp_init\n"
    "  0, // tp_alloc\n"
    "  Py%s_New, // tp_new\n"
    "  _PyObject_Del, // tp_free\n"
    "  0, // tp_is_gc\n",
    data->Name, data->Name);

  /* fields set by python itself */
  fprintf(fp,
    "  0, // tp_bases\n"
    "  0, // tp_mro\n"
    "  0, // tp_cache\n"
    "  0, // tp_subclasses\n"
    "  0, // tp_weaklist\n"
    "#endif\n");

  /* internal struct members */
  fprintf(fp,
    "  VTK_WRAP_PYTHON_SUPRESS_UNINITIALIZED\n"
    "};\n"
    "\n");

  /* generate the copy constructor helper function */
  fprintf(fp,
    "static void *%s_Copy(const void *obj)\n"
    "{\n"
    "  if (obj)\n"
    "    {\n"
    "    return new %s(*static_cast<const %s*>(obj));\n"
    "    }\n"
    "  return 0;\n"
    "}\n"
    "\n",
    data->Name, data->Name, data->Name);

  /* the method for adding the VTK extras to the type,
   * the unused "const char *" arg is the module name */
  fprintf(fp,
    "static PyObject *Py%s_TypeNew(const char *)\n"
    "{\n"
    "  return PyVTKSpecialType_New(\n"
    "    &Py%s_Type,\n"
    "    Py%sMethods,\n"
    "    Py%s_%sMethods,\n"
    "    &Py%sNewMethod,\n"
    "    %sDoc(), &%s_Copy);\n"
    "}\n"
    "\n",
    data->Name, data->Name, data->Name,
    data->Name, data->Name, data->Name,
    data->Name, data->Name);
}


/* -------------------------------------------------------------------- */
/* This method adds constants defined in the file to the module */

void vtkWrapPython_AddConstant(
  FILE *fp, const char *indent, const char *dictvar, const char *objvar,
  ValueInfo *val)
{
  unsigned int valtype;
  const char *valstring;
  int objcreated = 0;

  valtype = (val->Type & VTK_PARSE_UNQUALIFIED_TYPE);
  valstring = val->Value;

  if (valtype == 0 && (valstring == NULL || valstring[0] == '\0'))
    {
    valtype = VTK_PARSE_VOID;
    }
  else if (strcmp(valstring, "NULL") == 0)
    {
    valtype = VTK_PARSE_VOID;
    }

  if (valtype == 0 || val->Name == NULL)
    {
    return;
    }

  switch (valtype)
    {
    case VTK_PARSE_VOID:
      fprintf(fp,
              "%sPy_INCREF(Py_None);\n"
              "%s%s = Py_None;\n",
              indent, indent, objvar);
      objcreated = 1;
      break;

    case VTK_PARSE_CHAR_PTR:
      fprintf(fp,
              "%s%s = PyString_FromString((char *)(%s));\n",
              indent, objvar, valstring);
      objcreated = 1;
      break;

    case VTK_PARSE_FLOAT:
    case VTK_PARSE_DOUBLE:
      fprintf(fp,
              "%s%s = PyFloat_FromDouble(%s);\n",
              indent, objvar, valstring);
      objcreated = 1;
      break;

    case VTK_PARSE_LONG:
    case VTK_PARSE_INT:
#if (VTK_SIZEOF_INT < VTK_SIZEOF_LONG)
    case VTK_PARSE_UNSIGNED_INT:
#endif
    case VTK_PARSE_SHORT:
    case VTK_PARSE_UNSIGNED_SHORT:
    case VTK_PARSE_CHAR:
    case VTK_PARSE_SIGNED_CHAR:
    case VTK_PARSE_UNSIGNED_CHAR:
      fprintf(fp,
              "%s%s = PyInt_FromLong(%s);\n",
              indent, objvar, valstring);
      objcreated = 1;
      break;

    case VTK_PARSE_UNSIGNED_LONG:
#if (VTK_SIZEOF_INT == VTK_SIZEOF_LONG)
    case VTK_PARSE_UNSIGNED_INT:
#endif
      fprintf(fp,
              "#if (PY_VERSION_HEX >= 0x02020000)\n"
              "%s%s = PyLong_FromUnsignedLong(%s);\n"
              "#else\n"
              "%s%s = PyInt_FromLong((long)(%s));\n"
              "#endif\n",
              indent, objvar, valstring, indent, objvar, valstring);
      objcreated = 1;
      break;

#ifdef PY_LONG_LONG
#ifdef VTK_TYPE_USE___INT64
    case VTK_PARSE___INT64:
      fprintf(fp,
              "%s%s = PyLong_FromLongLong(%s);\n",
              indent, objvar, valstring);
      objcreated = 1;
      break;

    case VTK_PARSE_UNSIGNED___INT64:
      fprintf(fp,
              "%s%s = PyLong_FromUnsignedLongLong(%s);\n",
              indent, objvar, valstring);
      objcreated = 1;
      break;
#endif

#ifdef VTK_TYPE_USE_LONG_LONG
    case VTK_PARSE_LONG_LONG:
      fprintf(fp,
              "%s%s = PyLong_FromLongLong(%s);\n",
              indent, objvar, valstring);
      objcreated = 1;
      break;

    case VTK_PARSE_UNSIGNED_LONG_LONG:
      fprintf(fp,
              "%s%s = PyLong_FromUnsignedLongLong(%s);\n",
              indent, objvar, valstring);
      objcreated = 1;
      break;
#endif
#endif

    case VTK_PARSE_BOOL:
      fprintf(fp,
              "#if PY_VERSION_HEX >= 0x02030000\n"
              "%s%s = PyBool_FromLong((long)(%s));\n"
              "#else\n"
              "%s%s = PyInt_FromLong((long)(%s));\n"
              "#endif\n",
              indent, objvar, valstring, indent, objvar, valstring);
      objcreated = 1;
      break;
    }

  if (objcreated)
    {
    fprintf(fp,
            "%sif (%s && PyDict_SetItemString(%s, (char *)\"%s\", %s) != 0)\n"
            "%s  {\n"
            "%s  Py_DECREF(%s);\n"
            "%s  }\n",
            indent, objvar, dictvar, val->Name, objvar,
            indent, indent, objvar, indent);
    }
}

/* -------------------------------------------------------------------- */
/* This is the main entry point for the python wrappers.  When called,
 * it will print the vtkXXPython.c file contents to "fp".  */

void vtkParseOutput(FILE *fp, FileInfo *file_info)
{
  ClassInfo *data;
  NamespaceInfo *contents;
  OptionInfo *options;
  HierarchyInfo *hinfo = 0;
  const char *name;
  char *name_from_file = 0;
  int class_has_new = 0;
  int has_constants = 0;
  int is_vtkobject = 1;
  int i;
  size_t j, m;

  /* get the main class */
  data = file_info->MainClass;
  if (data)
    {
    name = data->Name;
    }
  /* if no class, use the file name */
  else
    {
    name = file_info->FileName;
    m = strlen(name);
    for (j = m; j > 0; j--)
      {
      if (name[j] == '.') { break; }
      }
    if (j > 0) { m = j; }
    for (j = m; j > 0; j--)
      {
      if (!((name[j-1] >= 'a' && name[j-1] <= 'z') ||
            (name[j-1] >= 'A' && name[j-1] <= 'Z') ||
            name[j-1] == '_')) { break; }
      }
    name_from_file = (char *)malloc(m - j + 1);
    strncpy(name_from_file, &name[j], m - j);
    name_from_file[m-j] = '\0';
    name = name_from_file;
    }

  /* get the global namespace */
  contents = file_info->Contents;

  /* get the command-line options */
  options = vtkParse_GetCommandLineOptions();

  /* get the hierarchy info for accurate typing */
  if (options->HierarchyFileName)
    {
    hinfo = vtkParseHierarchy_ReadFile(options->HierarchyFileName);
    }

  /* the VTK_WRAPPING_CXX tells header files where they're included from */
  fprintf(fp,
          "// python wrapper for %s\n//\n"
          "#define VTK_WRAPPING_CXX\n",
          name);

  /* unless this is vtkObjectBase, define VTK_STREAMS_FWD_ONLY */
  if (strcmp("vtkObjectBase", name) != 0)
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
  if (strcmp("vtkObject", name) == 0)
    {
    fprintf(fp,
          "#include \"vtkPythonCommand.h\"\n");
    }

  if (data)
    {
    /* generate includes for any special types that are used */
    vtkWrapPython_GenerateSpecialHeaders(fp, data, hinfo);
    }

  /* the header file for the wrapped class */
  fprintf(fp,
          "#include \"%s.h\"\n",
          name);

  /* is this isn't a vtkObjectBase-derived object, then it is special */
  is_vtkobject = options->IsVTKObject;

  /* add sstream header for special objects and vtkObjectBase*/
  if (data && (strcmp(data->Name, "vtkObjectBase") == 0 ||
               (!is_vtkobject && !data->IsAbstract)))
    {
    fprintf(fp,
            "\n"
            "#include <vtksys/ios/sstream>\n");
    }

  /* do the export of the main entry point */
  fprintf(fp,
          "#if defined(WIN32)\n"
          "extern \"C\" { __declspec( dllexport ) void PyVTKAddFile_%s(PyObject *, const char *); }\n"
          "#else\n"
          "extern \"C\" { void PyVTKAddFile_%s(PyObject *, const char *); }\n"
          "#endif\n",
          name, name);

  if (data && is_vtkobject)
    {
    fprintf(fp,
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
      if (vtkWrapPython_IsClassWrapped(data->SuperClasses[i], hinfo))
        {
        if (hinfo == 0 || vtkParseHierarchy_IsTypeOf(
                            hinfo, data->SuperClasses[i], "vtkObjectBase"))
          {
          fprintf(fp,
            "extern \"C\" { PyObject *PyVTKClass_%sNew(const char *); }\n",
            data->SuperClasses[i]);
          }
        }
      }
    }

  /* prototype for the docstring function */
  if (data)
    {
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
      vtkWrapPython_GenerateObjectNew(fp, data, hinfo, class_has_new);
      }

    /* output the class initilization function for special objects */
    else if (!data->IsAbstract)
      {
      vtkWrapPython_GenerateSpecialType(fp, data, file_info, hinfo);
      }
    }

  /* The function for adding everything to the module dict */
  fprintf(fp,
          "void PyVTKAddFile_%s(\n"
          "  PyObject *dict, const char *modulename)\n"
          "{\n"
          "  PyObject *o;\n",
          name);

  if (data && is_vtkobject)
    {
    fprintf(fp,
            "  o = PyVTKClass_%sNew(modulename);\n"
            "\n"
            "  if (o && PyDict_SetItemString(dict, (char *)\"%s\", o) != 0)\n"
            "    {\n"
            "    Py_DECREF(o);\n"
            "    }\n",
            data->Name, data->Name);

    for (i = 0; i < data->NumberOfConstants; i++)
      {
      if (data->Constants[i]->Access == VTK_ACCESS_PUBLIC)
        {
        has_constants = 1;
        break;
        }
      }

    if (has_constants)
      {
      fprintf(fp,
              "  else\n"
              "    {\n"
              "    PyObject *d = PyVTKClass_GetDict(o);\n"
              "\n");

      /* add any constants defined in the class */
      for (i = 0; i < data->NumberOfConstants; i++)
        {
        if (data->Constants[i]->Access == VTK_ACCESS_PUBLIC)
          {
          vtkWrapPython_AddConstant(fp, "    ", "d", "o", data->Constants[i]);
          fprintf(fp, "\n");
          }
        }

      fprintf(fp,
              "    }\n"
              "\n");
      }
    }
  else if (data) /* not is_vtkobject */
    {
    fprintf(fp,
            "  o = Py%s_TypeNew(modulename);\n"
            "\n"
            "  if (o && PyDict_SetItemString(dict, (char *)\"%s\", o) != 0)\n"
            "    {\n"
            "    Py_DECREF(o);\n"
            "    }\n",
            data->Name, data->Name);
    }

  /* add any constants defined in the file */
  for (i = 0; i < contents->NumberOfConstants; i++)
    {
    vtkWrapPython_AddConstant(fp, "  ", "dict", "o", contents->Constants[i]);
    fprintf(fp, "\n");
    }

  /* close the AddFile function */
  fprintf(fp,
          "}\n\n");

  /* the docstring for the class, as a static var ending in "Doc" */
  if (data && (is_vtkobject || !data->IsAbstract))
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

  if (name_from_file)
    {
    free(name_from_file);
    }
}
