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
#include "vtkConfigure.h"

/* -------------------------------------------------------------------- */
/* the main entry method, called by vtkParse.y */
void vtkParseOutput(FILE *fp, FileInfo *data);


/* -------------------------------------------------------------------- */
/* prototypes for the methods used by the python wrappers */

/* generate the class docstring and write it to "fp" */
static void vtkWrapPython_ClassDoc(
  FILE *fp, FileInfo *data);

/* print out all methods and the method table */
static void vtkWrapPython_GenerateMethods(
  FILE *fp, FileInfo *data, int class_has_new, int do_constructors);

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
  FileInfo *data, FunctionInfo *currentFunction, int contructor);

/* create a format string for PyArg_ParseTuple */
static char *vtkWrapPython_FormatString(
  FunctionInfo *currentFunction);

/* create a string for checking arguments against available signatures */
static char *vtkWrapPython_ArgCheckString(
  int isvtkobjmethod, FunctionInfo *currentFunction);

/* replace the original method signature with a python-ized signature */
static void vtkWrapPython_Signature(
  FunctionInfo *currentFunction);

/* quote a string for inclusion in a C string literal */
static const char *vtkWrapPython_QuoteString(
  const char *comment, int maxlen);


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
  int  i;
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
  int aType = currentFunction->ReturnType;
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
  if ((i == MAX_ARGS) && ((aType & VTK_PARSE_CONST) != 0))
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
    case VTK_PARSE_VTK_OBJECT:  fprintf(fp,"%s ",Id); break;
    case VTK_PARSE_ID_TYPE:     fprintf(fp,"vtkIdType "); break;
    case VTK_PARSE_LONG_LONG:   fprintf(fp,"long long "); break;
    case VTK_PARSE___INT64:     fprintf(fp,"__int64 "); break;
    case VTK_PARSE_SIGNED_CHAR: fprintf(fp,"signed char "); break;
    case VTK_PARSE_BOOL:        fprintf(fp,"bool "); break;
    case VTK_PARSE_UNKNOWN:     return;
    }

  /* then print the decorators for ref and pointer, but not for arrays */
  switch (aType & VTK_PARSE_INDIRECT)
    {
    case VTK_PARSE_REF:
      fprintf(fp, "*"); /* refs are converted to pointers */
      break;
    case VTK_PARSE_POINTER:
      if ((i == MAX_ARGS) ||
          ((aType & VTK_PARSE_BASE_TYPE) == VTK_PARSE_VTK_OBJECT) ||
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
  if ((aType & VTK_PARSE_UNQUALIFIED_TYPE) == VTK_PARSE_VTK_OBJECT &&
      i != MAX_ARGS)
    {
    fprintf(fp, "*");
    }

  /* the variable name */
  fprintf(fp,"temp%i",i);

  /* print the array decorators */
  if (((aType & VTK_PARSE_INDIRECT) == VTK_PARSE_POINTER) &&
      (i != MAX_ARGS) &&
      ((aType & VTK_PARSE_BASE_TYPE) != VTK_PARSE_VTK_OBJECT) &&
      ((aType & VTK_PARSE_UNQUALIFIED_TYPE) != VTK_PARSE_CHAR_PTR) &&
      ((aType & VTK_PARSE_UNQUALIFIED_TYPE) != VTK_PARSE_VOID_PTR))
    {
    fprintf(fp,"[%i]",aCount);
    }

  /* finish off with a semicolon */
  fprintf(fp,";\n");

  /* for "void *", add another temp to hold the size of the argument */
  if (((aType & VTK_PARSE_UNQUALIFIED_TYPE) == VTK_PARSE_VOID_PTR) &&
      (i != MAX_ARGS))
    {
    fprintf(fp,
            "  int      size%d;\n",
            i);
    }

  /* for VTK_OBJECT arguments, a PyObject temp is also needed */
  if ((i != MAX_ARGS) &&
      ((aType & VTK_PARSE_BASE_TYPE) == VTK_PARSE_VTK_OBJECT))
    {
    fprintf(fp,
            "  PyObject *tempH%d = 0;\n",
            i);
    }

  /* ditto for bool */
  if ((i != MAX_ARGS) && (aType == VTK_PARSE_BOOL))
    {
    fprintf(fp,
            "  PyObject *tempB%d = 0;\n",
            i);
    }
}


/* -------------------------------------------------------------------- */
/* Convert values into python object and return them within python,
 * using the static var "currentFunction" as the current function */

static void vtkWrapPython_ReturnValue(
  FILE *fp, FunctionInfo *currentFunction)
{
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
    case VTK_PARSE_VTK_OBJECT_PTR:
      {
      fprintf(fp,
              "    result = vtkPythonGetObjectFromPointer((vtkObjectBase *)temp%i);\n",
              MAX_ARGS);
      break;
      }

    /* convert special objects to Python objects */
    case VTK_PARSE_VTK_OBJECT_REF:
      {
      fprintf(fp,
              "    result = vtkPythonGetSpecialObjectFromPointer(temp%i, \"%s\");\n",
              MAX_ARGS, currentFunction->ReturnClass);
      break;
      }

    /* convert special objects to Python objects */
    case VTK_PARSE_VTK_OBJECT:
      {
      fprintf(fp,
              "    result = vtkPythonGetSpecialObjectFromPointer(&temp%i, \"%s\");\n",
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
              "      result = PyString_FromString(vtkPythonManglePointer(temp%i,\"void_p\"));\n"
              "      }\n",
              MAX_ARGS, MAX_ARGS);
      break;
      }

    /* handle all basic types by simple conversion */
    case VTK_PARSE_FLOAT:
    case VTK_PARSE_DOUBLE:
      {
      fprintf(fp,
              "    result = PyFloat_FromDouble(temp%i);\n",
              MAX_ARGS);
      break;
      }
    case VTK_PARSE_UNSIGNED_CHAR:
    case VTK_PARSE_UNSIGNED_INT:
    case VTK_PARSE_UNSIGNED_SHORT:
    case VTK_PARSE_INT:
    case VTK_PARSE_SHORT:
    case VTK_PARSE_LONG:
    case VTK_PARSE_SIGNED_CHAR:
      {
      fprintf(fp,
              "    result = PyInt_FromLong(temp%i);\n",
              MAX_ARGS);
      break;
      }

    /* PyBool_FromLong was introduced in Python 2.3,
     * but PyInt_FromLong is a good substitute */
    case VTK_PARSE_BOOL:
      {
      fprintf(fp,
              "#if PY_VERSION_HEX >= 0x02030000\n"
              "    result = PyBool_FromLong(temp%i);\n"
              "#else\n"
              "    result = PyInt_FromLong((long)temp%i);\n"
              "#endif\n",
              MAX_ARGS, MAX_ARGS);
      break;
      }

    /* PyLong_FromUnsignedLong() is new to Python 2.2 */
    case VTK_PARSE_UNSIGNED_LONG:
      {
      fprintf(fp,
              "#if (PY_VERSION_HEX >= 0x02020000)\n"
              "    result = PyLong_FromUnsignedLong(temp%i);\n"
              "#else\n"
              "    result = PyInt_FromLong((long)temp%i);\n"
              "#endif\n",
              MAX_ARGS, MAX_ARGS);
      break;
      }

    /* Support for vtkIdType depends on config and capabilities */
#if defined(VTK_USE_64BIT_IDS) && defined(PY_LONG_LONG) && (VTK_SIZEOF_LONG != VTK_SIZEOF_ID_TYPE)
    case VTK_PARSE_ID_TYPE:
      {
      fprintf(fp,
              "    result = PyLong_FromLongLong(temp%i);\n",
              MAX_ARGS);
      break;
      }
    case VTK_PARSE_UNSIGNED_ID_TYPE:
      {
      fprintf(fp,
              "    result = PyLong_FromUnsignedLongLong(temp%i);\n",
              MAX_ARGS);
      break;
      }
#else
    case VTK_PARSE_ID_TYPE:
      {
      fprintf(fp,
              "    result = PyInt_FromLong((long)temp%i);\n",
              MAX_ARGS);
      break;
      }
    case VTK_PARSE_UNSIGNED_ID_TYPE:
      {
      fprintf(fp,
              "#if (PY_VERSION_HEX >= 0x02020000)\n"
              "    result = PyLong_FromUnsignedLong((unsigned long)temp%i);\n"
              "#else\n"
              "    result = PyInt_FromLong((long)temp%i);\n"
              "#endif\n",
              MAX_ARGS, MAX_ARGS);
      break;
      }
#endif

    /* support for "long long" depends on config and capabilities */
#if defined(VTK_SIZEOF_LONG_LONG)
# if defined(PY_LONG_LONG) && (VTK_SIZEOF_LONG != VTK_SIZEOF_LONG_LONG)
    case VTK_PARSE_LONG_LONG:
      {
      fprintf(fp,
              "    result = PyLong_FromLongLong(temp%i);\n",
              MAX_ARGS);
      break;
      }
    case VTK_PARSE_UNSIGNED_LONG_LONG:
      {
      fprintf(fp,
              "    result = PyLong_FromUnsignedLongLong(temp%i);\n",
              MAX_ARGS);
      break;
      }
# else
    case VTK_PARSE_LONG_LONG:
      {
      fprintf(fp,
              "    result = PyLong_FromLong(temp%i);\n",
              MAX_ARGS);
      break;
      }
    case VTK_PARSE_UNSIGNED_LONG_LONG:
      {
      fprintf(fp,
              "    result = PyLong_FromUnsignedLong(temp%i);\n",
              MAX_ARGS);
      break;
      }
# endif
#endif

    /* support for "__int64" depends on config and capabilities */
#if defined(VTK_SIZEOF___INT64)
# if defined(PY_LONG_LONG) && (VTK_SIZEOF_LONG != VTK_SIZEOF___INT64)
    case VTK_PARSE___INT64:
      {
      fprintf(fp,
              "    result = PyLong_FromLongLong(temp%i);\n",
              MAX_ARGS);
      break;
      }
    case VTK_PARSE_UNSIGNED___INT64:
      {
      fprintf(fp,
              "    result = PyLong_FromUnsignedLongLong(temp%i);\n",
              MAX_ARGS);
      break;
      }
# else
    case VTK_PARSE___INT64:
      {
      fprintf(fp,
              "    result = PyLong_FromLong(temp%i);\n",
              MAX_ARGS);
      break;
      }
    case VTK_PARSE_UNSIGNED___INT64:
      {
      fprintf(fp,
              "#if (PY_VERSION_HEX >= 0x02020000)\n"
              "    result = PyLong_FromUnsignedLong((unsigned long)temp%i);\n"
              "#else\n"
              "    result = PyInt_FromLong((long)temp%i);\n"
              "#endif\n",
              MAX_ARGS, MAX_ARGS);
      break;
      }
# endif
#endif

    /* return a char as a string of unit length */
    case VTK_PARSE_CHAR:
      {
      fprintf(fp,
              "    char temp%iString[2];\n"
              "    temp%iString[0] = temp%i;\n"
              "    temp%iString[1] = \'\\0\';\n"
              "    result = PyString_FromStringAndSize(temp%iString,1);\n",
              MAX_ARGS, MAX_ARGS, MAX_ARGS, MAX_ARGS, MAX_ARGS);
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
 * added to PyVTKCheckArg() in vtkPythonUtil.cxx
 */

static char *vtkWrapPython_FormatString(FunctionInfo *currentFunction)
{
  static char result[1024];
  int currPos = 0;
  int argtype;
  int i, j;

  if (currentFunction->ArgTypes[0] == VTK_PARSE_FUNCTION)
    {
    result[currPos++] = 'O';
    result[currPos] = '\0';
    return result;
    }

  for (i = 0; i < currentFunction->NumberOfArguments; i++)
    {
    argtype = (currentFunction->ArgTypes[i] & VTK_PARSE_UNQUALIFIED_TYPE);

    switch (argtype)
      {
      case VTK_PARSE_FLOAT_PTR:
        result[currPos++] = '(';
        for (j = 0; j < currentFunction->ArgCounts[i]; j++)
          {
          result[currPos++] = 'f';
          }
        result[currPos++] = ')';
        break;
      case VTK_PARSE_DOUBLE_PTR:
        result[currPos++] = '(';
        for (j = 0; j < currentFunction->ArgCounts[i]; j++)
          {
          result[currPos++] = 'd';
          }
        result[currPos++] = ')';
        break;
      case VTK_PARSE_INT_PTR:
        result[currPos++] = '(';
        for (j = 0; j < currentFunction->ArgCounts[i]; j++)
          {
          result[currPos++] = 'i';
          }
        result[currPos++] = ')';
        break;
      case VTK_PARSE_ID_TYPE_PTR:
        result[currPos++] = '(';
        for (j = 0; j < currentFunction->ArgCounts[i]; j++)
          {
#ifdef VTK_USE_64BIT_IDS
#ifdef PY_LONG_LONG
          result[currPos++] = 'L';
#else
          result[currPos++] = 'l';
#endif
#else
          result[currPos++] = 'i';
#endif
          }
        result[currPos++] = ')';
        break;
      case VTK_PARSE_LONG_LONG_PTR:
      case VTK_PARSE___INT64_PTR:
        result[currPos++] = '(';
        for (j = 0; j < currentFunction->ArgCounts[i]; j++)
          {
#ifdef PY_LONG_LONG
          result[currPos++] = 'L';
#else
          result[currPos++] = 'l';
#endif
          }
        result[currPos++] = ')';
        break;
      case VTK_PARSE_VTK_OBJECT_REF:
      case VTK_PARSE_VTK_OBJECT_PTR:
      case VTK_PARSE_VTK_OBJECT:
        result[currPos++] = 'O';
        break;
      case VTK_PARSE_CHAR_PTR:
        result[currPos++] = 'z';
        break;
      case VTK_PARSE_VOID_PTR:
        result[currPos++] = 's';
        result[currPos++] = '#';
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
      }
    }

  result[currPos] = '\0';
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
  int currPos = 0;
  int argtype;
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

    if (argtype == VTK_PARSE_BOOL)
      {
      strcpy(&result[currPos], " bool");
      currPos += 5;
      }

    if (argtype == VTK_PARSE_VTK_OBJECT_REF ||
        argtype == VTK_PARSE_VTK_OBJECT_PTR ||
        argtype == VTK_PARSE_VTK_OBJECT)
      {
      result[currPos++] = ' ';
      if (argtype == VTK_PARSE_VTK_OBJECT_REF)
        {
        result[currPos++] = '&';
        }
      else if (argtype == VTK_PARSE_VTK_OBJECT_PTR)
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
/* The method signatures are for the python docstrings. */

static void vtkWrapPython_AddToSignature(char *sig, const char *add, int *i)
{
  int j = 0;
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
/* Create a signature for the python version of a method, and
 * write it directly to currentFunction->Signature */

static void vtkWrapPython_Signature(FunctionInfo *currentFunction)
{
  static char result[1024];
  int currPos = 0;
  int argtype;
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
      case VTK_PARSE_VTK_OBJECT_REF:
      case VTK_PARSE_VTK_OBJECT_PTR:
      case VTK_PARSE_VTK_OBJECT:
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
      case VTK_PARSE_VTK_OBJECT_REF:
      case VTK_PARSE_VTK_OBJECT_PTR:
      case VTK_PARSE_VTK_OBJECT:
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
      }
    }

  if (currentFunction->Signature)
    {
    vtkWrapPython_AddToSignature(result,"\\nC++: ",&currPos);
    vtkWrapPython_AddToSignature(result,currentFunction->Signature,&currPos);
    }

  currentFunction->Signature = realloc(currentFunction->Signature,
                                       (size_t)(currPos+1));
  strcpy(currentFunction->Signature,result);
  /* fprintf(stderr,"%s\n",currentFunction->Signature); */
}

/* -------------------------------------------------------------------- */
/* For the purpose of the python docstrings, convert special characters
 * in a string into their escape codes, so that the string can be quoted
 * in a source file (the specified maxlen must be at least 32 chars)*/

static const char *vtkWrapPython_QuoteString(const char *comment, int maxlen)
{
  static char *result = 0;
  static int oldmaxlen = 0;
  int i, j, n;

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

  n = (int)strlen(comment);
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
/* Print out all the python methods that call the C++ class methods.
 * After they're all printed, a Py_MethodDef array that has function
 * pointers and documentation for each method is printed.  In other
 * words, this poorly named function is "the big one". */

static void vtkWrapPython_GenerateMethods(
  FILE *fp, FileInfo *data, int class_has_new, int do_constructors)
{
  int i, j, k, is_static, is_pure_virtual, is_vtkobject, fnum, occ;
  int numberOfSignatures, signatureCount;
  char signatureSuffix[8];
  int all_legacy, all_static;
  int numberOfWrappedFunctions = 0;
  FunctionInfo *wrappedFunctions[1000];
  FunctionInfo *theSignature;
  FunctionInfo *theFunc;
  int returnType = 0;
  int argType = 0;
  int voteCurrent = 0;
  int voteMaster = 0;
  int potential_error = 0;
  int needs_cleanup = 0;
  char on_error[32];

  /* go through all functions and see which are wrappable,
   * note that "theSignature" is a global variable  */
  for (i = 0; i < data->NumberOfFunctions; i++)
    {
    theFunc = &data->Functions[i];

    /* check for wrappability */
    if (vtkWrapPython_MethodCheck(data, theFunc, do_constructors))
      {
      wrappedFunctions[numberOfWrappedFunctions++] = theFunc;
      }
    }

  /* check for derivation from vtkObjectBase */
  is_vtkobject = ((strcmp(data->ClassName,"vtkObjectBase") == 0) ||
                  (data->NumberOfSuperClasses != 0));

  /* create a python-type signature for each method (for use in docstring) */
  for (fnum = 0; fnum < numberOfWrappedFunctions; fnum++)
    {
    vtkWrapPython_Signature(wrappedFunctions[fnum]);
    }

  /* for each function in the array */
  for (fnum = 0; fnum < numberOfWrappedFunctions; fnum++)
    {
    /* make sure we haven't already done one of these */
    theFunc = wrappedFunctions[fnum];
    returnType = (theFunc->ReturnType & VTK_PARSE_UNQUALIFIED_TYPE);

    /* check for type precedence, don't need a "float" method if a
       "double" method exists */

    for (occ = fnum; occ < numberOfWrappedFunctions; occ++)
      {
      theSignature = wrappedFunctions[occ];

      if (theFunc->Name && theSignature->Name &&
          strcmp(theFunc->Name, theSignature->Name) == 0)
        {
        voteCurrent = 0;
        voteMaster = 0;
        if (theFunc->NumberOfArguments == theSignature->NumberOfArguments)
          {
          for (i = 0; i < theSignature->NumberOfArguments; i++)
            {
            if ((theSignature->ArgCounts[i] == theFunc->ArgCounts[i]) &&
                (theSignature->ArgTypes[i] & ~VTK_PARSE_BASE_TYPE) ==
                (theFunc->ArgTypes[i] & ~VTK_PARSE_BASE_TYPE))
              {
              if ((theFunc->ArgTypes[i] & VTK_PARSE_BASE_TYPE)
                  == VTK_PARSE_FLOAT &&
                  (theSignature->ArgTypes[i] & VTK_PARSE_BASE_TYPE)
                  == VTK_PARSE_DOUBLE &&
                  !voteMaster)
                {
                voteCurrent = 1;
                }
              else if ((theFunc->ArgTypes[i] & VTK_PARSE_BASE_TYPE)
                       == VTK_PARSE_FLOAT &&
                       (theSignature->ArgTypes[i] & VTK_PARSE_BASE_TYPE)
                       == VTK_PARSE_DOUBLE &&
                       !voteCurrent)
                {
                voteMaster = 1;
                }
              else if ((theFunc->ArgTypes[i] & VTK_PARSE_BASE_TYPE) !=
                       (theSignature->ArgTypes[i] & VTK_PARSE_BASE_TYPE))
                {
                voteCurrent = 0;
                voteMaster = 0;
                break;
                }
              }
            }
          }
        if (voteMaster)
          {
          /* prefer theFunc's signature over theSignature */
          theSignature->Name = 0;
          }
        else if (voteCurrent)
          {
          theFunc->Name = 0;
          }
        }
      }

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
          if ((wrappedFunctions[occ]->ReturnType & VTK_PARSE_STATIC) == 0)
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
          if ((theSignature->ReturnType & VTK_PARSE_STATIC) != 0)
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
                  data->ClassName, theSignature->Name, signatureSuffix,
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
                      data->ClassName);
              }
            else
              {
              fprintf(fp,
                      "  %s *op = (%s *)((PyVTKSpecialObject *)self)->vtk_ptr;\n",
                      data->ClassName, data->ClassName);
              }
            }

          /* temp variables for arg values */
          for (i = 0; i < theSignature->NumberOfArguments; i++)
            {
            vtkWrapPython_MakeTempVariable(fp, theSignature, i);

            /* special object args need cleanup */
            if (((theSignature->ArgTypes[i] & VTK_PARSE_UNQUALIFIED_TYPE) ==
                 VTK_PARSE_VTK_OBJECT) ||
                ((theSignature->ArgTypes[i] & VTK_PARSE_UNQUALIFIED_TYPE) ==
                 VTK_PARSE_VTK_OBJECT_REF))
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
                    "  if ((PyArg_ParseTuple(args, (char*)\"%s\"",
                    vtkWrapPython_FormatString(theSignature));
            }
          else
            {
            fprintf(fp,
                    "  op = (%s *)PyArg_VTKParseTuple(self, args, (char*)\"%s\"",
                    data->ClassName,
                    vtkWrapPython_FormatString(theSignature));
            }

          for (i = 0; i < theSignature->NumberOfArguments; i++)
            {
            argType = (theSignature->ArgTypes[i] &
                       VTK_PARSE_UNQUALIFIED_TYPE);

            if ((argType & VTK_PARSE_BASE_TYPE) == VTK_PARSE_VTK_OBJECT)
              {
              fprintf(fp,", &tempH%d",i);
              }
            else if (argType == VTK_PARSE_BOOL)
              {
              fprintf(fp,", &tempB%d",i);
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
            fprintf(fp,");\n"
                    "  if (op)\n"
                    "    {\n");
            }

          /* lookup and required objects */
          for (i = 0; i < theSignature->NumberOfArguments; i++)
            {
            argType = (theSignature->ArgTypes[i] &
                       VTK_PARSE_UNQUALIFIED_TYPE);

            if (argType == VTK_PARSE_VTK_OBJECT_PTR)
              {
              fprintf(fp,
                      "    temp%d = (%s *)vtkPythonGetPointerFromObject(tempH%d,(char*)\"%s\");\n",
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
            else if (argType == VTK_PARSE_VTK_OBJECT_REF ||
                     argType == VTK_PARSE_VTK_OBJECT)
              {
              fprintf(fp,
                      "    temp%d = (%s *)vtkPythonGetPointerFromSpecialObject(tempH%d, (char*)\"%s\", &tempH%d);\n",
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
            else if (argType == VTK_PARSE_BOOL)
              {
              fprintf(fp,
                      "    temp%d = PyObject_IsTrue(tempB%d);\n"
                      "    if (PyErr_Occurred())\n"
                      "      {\n"
                      "      %s;\n"
                      "      }\n",
                      i, i, on_error);
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
                    theSignature->Name, data->ClassName);
            }

          /* check for void pointers and pass appropriate info*/
          for (i = 0; i < theSignature->NumberOfArguments; i++)
            {
            argType = (theSignature->ArgTypes[i] &
                       VTK_PARSE_UNQUALIFIED_TYPE);

            if (argType == VTK_PARSE_VOID_PTR)
              {
              fprintf(fp,
                      "    temp%i = vtkPythonUnmanglePointer((char *)temp%i,&size%i,(char*)\"%s\");\n",
                      i, i, i, "void_p");

              fprintf(fp,
                      "    if (size%i == -1)\n"
                      "      {\n"
                      "      PyErr_SetString(PyExc_ValueError,\"mangled pointer to %s in %s was of incorrect type.\");\n"
                      "      %s;\n"
                      "      }\n",
                      i, theSignature->Name, data->ClassName, on_error);

              fprintf(fp,
                      "    else if (size%i == -2)\n"
                      "      {\n"
                      "      PyErr_SetString(PyExc_ValueError,\"mangled pointer to %s in %s was poorly formed.\");\n"
                      "      %s;\n"
                      "      }\n",
                      i, theSignature->Name, data->ClassName, on_error);

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
                        data->ClassName, theSignature->Name);
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
                  data->ClassName, theSignature->Name);
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
              case VTK_PARSE_VTK_OBJECT_REF:
                fprintf(fp,
                        "    temp%i = &%s(",
                        MAX_ARGS, methodname);
                break;
              default:
                fprintf(fp,
                        "    temp%i = %s(",
                        MAX_ARGS, methodname);
              }

            for (i = 0; i < theSignature->NumberOfArguments; i++)
              {
              argType = (theSignature->ArgTypes[i] &
                         VTK_PARSE_UNQUALIFIED_TYPE);

              if (i)
                {
                fprintf(fp,",");
                }
              if (argType == VTK_PARSE_VTK_OBJECT_REF ||
                  argType == VTK_PARSE_VTK_OBJECT)
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
                (argType & VTK_PARSE_BASE_TYPE) != VTK_PARSE_VTK_OBJECT &&
                (argType & VTK_PARSE_BASE_TYPE) != VTK_PARSE_UNKNOWN &&
                (argType & VTK_PARSE_BASE_TYPE) != VTK_PARSE_VOID &&
                ((theSignature->ArgTypes[i] & VTK_PARSE_CONST) == 0))
              {
              fprintf(fp,
                      "    if (vtkPythonCheckArray(args,%d,temp%d,%d))\n"
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
                    "    result = PyVTKSpecialObject_New((char*)\"%s\", op, 0);\n",
                    data->ClassName);
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
           * to vtkPythonGetPointerFromSpecialObject() */
          for (i = 0; i < theSignature->NumberOfArguments; i++)
            {
            argType = (theSignature->ArgTypes[i] &
                       VTK_PARSE_UNQUALIFIED_TYPE);

            if (argType == VTK_PARSE_VTK_OBJECT_REF ||
                argType == VTK_PARSE_VTK_OBJECT)
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
                data->ClassName, wrappedFunctions[fnum]->Name);

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
          if ((theSignature->ReturnType & VTK_PARSE_STATIC) != 0)
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
                  data->ClassName, wrappedFunctions[occ]->Name,
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
                "  return PyVTKCallOverloadedMethod(methods, self, args);\n"
                "}\n",
                 data->ClassName, wrappedFunctions[fnum]->Name,
                 data->ClassName, wrappedFunctions[fnum]->Name);

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
            realloc(theFunc->Signature,siglen+3+
                    strlen(theSignature->Signature));
          strcpy(&theFunc->Signature[siglen],"\\n");
          strcpy(&theFunc->Signature[siglen+2],
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
          data->ClassName);

  for (fnum = 0; fnum < numberOfWrappedFunctions; fnum++)
    {
    if(wrappedFunctions[fnum]->IsLegacy)
      {
      fprintf(fp,
              "#if !defined(VTK_LEGACY_REMOVE)\n");
      }
    if (wrappedFunctions[fnum]->Name)
      {
      fprintf(fp,
              "  {(char*)\"%s\",                (PyCFunction)Py%s_%s, 1,\n"
              "   (char*)\"%s\\n\\n%s\"},\n",
              wrappedFunctions[fnum]->Name, data->ClassName,
              wrappedFunctions[fnum]->Name, wrappedFunctions[fnum]->Signature,
              vtkWrapPython_QuoteString(wrappedFunctions[fnum]->Comment,1000));
      }
    if(wrappedFunctions[fnum]->IsLegacy)
      {
      fprintf(fp,
              "#endif\n");
      }
    }

  /* vtkObject needs a special entry for AddObserver */
  if (!strcmp("vtkObject",data->ClassName))
    {
    fprintf(fp,
            "  {(char*)\"AddObserver\",  (PyCFunction)Py%s_AddObserver, 1,\n"
            "   (char*)\"V.AddObserver(int, function) -> int\\n\\n Add an event callback function(vtkObject, int) for an event type.\\n Returns a handle that can be used with RemoveEvent(int).\"},\n",
            data->ClassName);
    }

  /* vtkObjectBase needs entries for GetAddressAsString and PrintRevisions */
  else if (!strcmp("vtkObjectBase",data->ClassName))
    {
    fprintf(fp,
            "  {(char*)\"GetAddressAsString\",  (PyCFunction)Py%s_GetAddressAsString, 1,\n"
            "   (char*)\"V.GetAddressAsString(string) -> string\\n\\n Get address of C++ object in format 'Addr=%%p' after casting to\\n the specified type.  You can get the same information from V.__this__.\"},\n"
            "  {(char*)\"PrintRevisions\",  (PyCFunction)Py%s_PrintRevisions, 1,\n"
            "   (char*)\"V.PrintRevisions() -> string\\n\\n Prints the .cxx file CVS revisions of the classes in the\\n object's inheritance chain.\"},\n",
            data->ClassName, data->ClassName);
    }

  /* python expects the method table to end with a "NULL" entry */
  fprintf(fp,
          "  {NULL,                       NULL, 0, NULL}\n"
          "};\n"
          "\n");
}


/* -------------------------------------------------------------------- */
/* Check a method to see if it is wrappable in python */

static int vtkWrapPython_MethodCheck(
  FileInfo *data, FunctionInfo *currentFunction, int constructor)
{
  int i;
  int args_ok = 1;
  int argType = 0;
  int returnType = 0;

  /* some functions will not get wrapped no matter what else,
     and some really common functions will appear only in vtkObjectPython */
  if (currentFunction->IsOperator ||
      currentFunction->ArrayFailure ||
      !currentFunction->IsPublic ||
      !currentFunction->Name)
    {
    return 0;
    }

  returnType = (currentFunction->ReturnType & VTK_PARSE_UNQUALIFIED_TYPE);

  /* check to see if we can handle all the args */
  for (i = 0; i < currentFunction->NumberOfArguments; i++)
    {
    argType = (currentFunction->ArgTypes[i] & VTK_PARSE_UNQUALIFIED_TYPE);

    if ((argType & VTK_PARSE_BASE_TYPE) == VTK_PARSE_UNKNOWN) args_ok = 0;
    if (((argType & VTK_PARSE_INDIRECT) != VTK_PARSE_POINTER) &&
        (argType != VTK_PARSE_VTK_OBJECT_REF) &&
        ((argType & VTK_PARSE_INDIRECT) != 0)) args_ok = 0;
    if (argType == VTK_PARSE_UNSIGNED_CHAR_PTR) args_ok = 0;
    if (argType == VTK_PARSE_UNSIGNED_INT_PTR) args_ok = 0;
    if (argType == VTK_PARSE_UNSIGNED___INT64_PTR) args_ok = 0;
    if (argType == VTK_PARSE_UNSIGNED_SHORT_PTR) args_ok = 0;
    if (argType == VTK_PARSE_UNSIGNED_LONG_PTR) args_ok = 0;
#ifndef VTK_SIZEOF___INT64
    if (argType == VTK_PARSE___INT64) args_ok = 0;
    if (argType == VTK_PARSE_UNSIGNED___INT64) args_ok = 0;
#endif
#ifndef VTK_SIZEOF_LONG_LONG
    if (argType == VTK_PARSE_LONG_LONG) args_ok = 0;
    if (argType == VTK_PARSE_UNSIGNED_LONG_LONG) args_ok = 0;
#endif
    }

  /* make sure we have all the info we need for array arguments */
  for (i = 0; i < currentFunction->NumberOfArguments; i++)
    {
    argType = (currentFunction->ArgTypes[i] & VTK_PARSE_UNQUALIFIED_TYPE);

    if (((argType & VTK_PARSE_INDIRECT) == VTK_PARSE_POINTER) &&
        (currentFunction->ArgCounts[i] <= 0) &&
        (argType != VTK_PARSE_VTK_OBJECT_PTR) &&
        (argType != VTK_PARSE_CHAR_PTR) &&
        (argType != VTK_PARSE_VOID_PTR)) args_ok = 0;
    }

  /* function pointer arguments for callbacks */
  if (currentFunction->NumberOfArguments &&
      (currentFunction->ArgTypes[0] == VTK_PARSE_FUNCTION) &&
      (currentFunction->NumberOfArguments != 1)) args_ok = 0;

  /* check the return type */
  if ((returnType & VTK_PARSE_BASE_TYPE) == VTK_PARSE_UNKNOWN) args_ok = 0;
  if (returnType == VTK_PARSE_VTK_OBJECT) args_ok = 0;
  if (((returnType & VTK_PARSE_INDIRECT) != VTK_PARSE_POINTER) &&
      (returnType != VTK_PARSE_VTK_OBJECT_REF) &&
      ((returnType & VTK_PARSE_INDIRECT) != 0)) args_ok = 0;

  /* eliminate "unsigned char *" and "unsigned short *" */
  if (returnType == VTK_PARSE_UNSIGNED_CHAR_PTR) args_ok = 0;
  if (returnType == VTK_PARSE_UNSIGNED_INT_PTR) args_ok = 0;
  if (returnType == VTK_PARSE_UNSIGNED___INT64_PTR) args_ok = 0;
  if (returnType == VTK_PARSE_UNSIGNED_SHORT_PTR) args_ok = 0;
  if (returnType == VTK_PARSE_UNSIGNED_LONG_PTR) args_ok = 0;

  /* eliminate types that aren't supported by the compiler */
#ifndef VTK_SIZEOF___INT64
    if (returnType == VTK_PARSE___INT64) args_ok = 0;
    if (returnType == VTK_PARSE_UNSIGNED___INT64) args_ok = 0;
#endif
#ifndef VTK_SIZEOF_LONG_LONG
    if (returnType == VTK_PARSE_LONG_LONG) args_ok = 0;
    if (returnType == VTK_PARSE_UNSIGNED_LONG_LONG) args_ok = 0;
#endif

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

  /* make sure it isn't a Delete or New function */
  if (currentFunction->Name == 0 ||
      strcmp("Delete", currentFunction->Name) == 0 ||
      strcmp("New", currentFunction->Name) == 0)
    {
    args_ok = 0;
    }

  /* make sure that the class and method have names */
  if (data->ClassName == 0 || currentFunction->Name == 0)
    {
    args_ok = 0;
    }
  else
    {
    /* check for destructor */
    if (currentFunction->Signature[0] == '~')
      {
      args_ok = 0;
      }

    /* check for constructors */
    if (strcmp(data->ClassName, currentFunction->Name) == 0)
      {
      /* if "constructor" is not set, disallow constructors */
      if (!constructor)
        {
        args_ok = 0;
        }
      }
    else if (constructor)
      {
      /* if "constructor" is set, only allow constructors */
      args_ok = 0;
      }
    }

  if (currentFunction->IsPublic)
    {
    return args_ok;
    }

  return 0;
}


/* -------------------------------------------------------------------- */
/* Create the docstring for a class, and print it to fp */

static void vtkWrapPython_ClassDoc(FILE *fp, FileInfo *data)
{
  const char *text;
  size_t i, n;
  char temp[500];

  if (data->NameComment)
    {
    text = data->NameComment;
    while (*text == ' ')
      {
      text++;
      }
    fprintf(fp,
            "    \"%s\\n\\n\",\n",
            vtkWrapPython_QuoteString(text,500));
    }
  else
    {
    fprintf(fp,
            "    \"%s - no description provided.\\n\\n\",\n",
            vtkWrapPython_QuoteString(data->ClassName,500));
    }

  if (data->NumberOfSuperClasses > 0)
    {
    fprintf(fp,
            "    \"Super Class:\\n\\n %s\\n\\n\",\n",
            vtkWrapPython_QuoteString(data->SuperClasses[0],500));
    }

  if (data->Description)
    {
    n = (strlen(data->Description) + 400-1)/400;
    for (i = 0; i < n; i++)
      {
      strncpy(temp, &data->Description[400*i], 400);
      temp[400] = '\0';
      if (i < n-1)
        {
        fprintf(fp,
                "    \"%s\",\n",
                vtkWrapPython_QuoteString(temp,500));
        }
      else
        { /* just for the last time */
        fprintf(fp,
                "    \"%s\\n\",\n",
                vtkWrapPython_QuoteString(temp,500));
        }
      }
    }
  else
    {
    fprintf(fp,
            "    \"%s\\n\",\n",
            "None provided.\\n");
    }

  if (data->Caveats)
    {
    fprintf(fp,
            "    \"Caveats:\\n\\n%s\\n\",\n",
            vtkWrapPython_QuoteString(data->Caveats,500));
    }

  if (data->SeeAlso)
    {
    char *sdup, *tok;

    fprintf(fp,
            "    \"See Also:\\n\\n");

    sdup = strdup(data->SeeAlso);
    tok = strtok(sdup," ");
    while (tok)
      {
      fprintf(fp,"   %s",vtkWrapPython_QuoteString(tok,120));
      tok = strtok(NULL," ");
      }
    free(sdup);
    fprintf(fp,"\\n\",\n");
    }

  /* for special objects, add constructor signatures to the doc */
  if ((data->NumberOfSuperClasses == 0) &&
      (strcmp(data->ClassName,"vtkObjectBase") != 0))
    {
    for (i = 0; i < data->NumberOfFunctions; i++)
      {
      if (vtkWrapPython_MethodCheck(data, &data->Functions[i], 1))
        {
        fprintf(fp,"    \"%s\\n\",\n", data->Functions[i].Signature);
        }
      }
    }
}

/* -------------------------------------------------------------------- */
/* This is the main entry point for the python wrappers.  When called,
 * it will print the vtkXXPython.c file contents to "fp".  */

void vtkParseOutput(FILE *fp, FileInfo *data)
{
  int i;
  int class_has_new = 0;

  /* the VTK_WRAPPING_CXX tells header files where they're included from */
  fprintf(fp,
          "// python wrapper for %s object\n//\n"
          "#define VTK_WRAPPING_CXX\n",
          data->ClassName);

  /* unless this is vtkObjectBase, define VTK_STREAMS_FWD_ONLY */
  if (strcmp("vtkObjectBase",data->ClassName) != 0)
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
          "#include <vtksys/ios/sstream>\n"
          "#include \"%s.h\"\n",
          data->ClassName);

  /* do the export of the main entry point */
  fprintf(fp,
          "#if defined(WIN32)\n"
          "extern \"C\" { __declspec( dllexport ) PyObject *PyVTKClass_%sNew(char *); }\n"
          "#else\n"
          "extern \"C\" { PyObject *PyVTKClass_%sNew(char *); }\n"
          "#endif\n"
          "\n",
          data->ClassName, data->ClassName);

  /* bring in all the superclasses */
  for (i = 0; i < data->NumberOfSuperClasses; i++)
    {
    fprintf(fp,
           "extern \"C\" { PyObject *PyVTKClass_%sNew(char *); }\n",
            data->SuperClasses[i]);
    }

  /* prototype for the docstring function */
  if (data->NumberOfSuperClasses || !data->IsAbstract)
    {
    fprintf(fp,
            "\n"
            "static const char **%sDoc();\n"
            "\n",
            data->ClassName);
    }

  /* the python vtkObject needs special hooks for observers */
  if (!strcmp("vtkObject",data->ClassName))
    {
    /* Add the AddObserver method to vtkObject. */
    fprintf(fp,
            "static PyObject *PyvtkObject_AddObserver(PyObject *self, PyObject *args)\n"
            "{\n"
            "  vtkObject *op;\n"
            "  char *temp0;\n"
            "  PyObject *temp1;\n"
            "  float temp2;\n"
            "  unsigned long     temp20 = 0;\n");

    fprintf(fp,
            "  op = (vtkObject *)PyArg_VTKParseTuple(self, args, (char*)\"zO\", &temp0, &temp1);\n"
            "  if (op)\n"
            "    {\n"
            "    if (!PyCallable_Check(temp1) && temp1 != Py_None)\n"
            "      {\n"
            "      PyErr_SetString(PyExc_ValueError,\"vtk callback method passed to AddObserver was not callable.\");\n"
            "      return NULL;\n"
            "      }\n"
            "    Py_INCREF(temp1);\n"
            "    vtkPythonCommand *cbc = vtkPythonCommand::New();\n"
            "    cbc->SetObject(temp1);\n"
            "    cbc->SetThreadState(PyThreadState_Get());\n"
            "    temp20 = op->AddObserver(temp0,cbc);\n"
            "    cbc->Delete();\n"
            "    return PyInt_FromLong((long)temp20);\n"
            "    }\n"
            "  PyErr_Clear();\n");

    fprintf(fp,
            "  op = (vtkObject *)PyArg_VTKParseTuple(self, args, (char*)\"zOf\", &temp0, &temp1, &temp2);\n"
            "  if (op)\n"
            "    {\n"
            "    if (!PyCallable_Check(temp1) && temp1 != Py_None)\n"
            "      {\n"
            "      PyErr_SetString(PyExc_ValueError,\"vtk callback method passed to AddObserver was not callable.\");\n"
            "      return NULL;\n"
            "      }\n"
            "    Py_INCREF(temp1);\n"
            "    vtkPythonCommand *cbc = vtkPythonCommand::New();\n"
            "    cbc->SetObject(temp1);\n"
            "    cbc->SetThreadState(PyThreadState_Get());\n"
            "    temp20 = op->AddObserver(temp0,cbc,temp2);\n"
            "    cbc->Delete();\n"
            "    return PyInt_FromLong((long)temp20);\n"
            "    }\n");

    fprintf(fp,
            "  return NULL;\n"
            "}\n"
            "\n");
    }

  /* the python vtkObjectBase needs a couple extra functions */
  if (!strcmp("vtkObjectBase",data->ClassName))
    {
    /* add the GetAddressAsString method to vtkObjectBase */
    fprintf(fp,
            "PyObject *PyvtkObjectBase_GetAddressAsString(PyObject *self, PyObject *args)\n"
            "{\n"
            "  %s *op;\n"
            "  char *typecast;\n"
            "\n"
            "  op = (%s *)PyArg_VTKParseTuple(self, args, (char*)\"s\", &typecast);\n"
            "  if (op)\n"
            "    {\n    char temp20[256];\n"
            "    sprintf(temp20,\"Addr=%%p\",op);\n"
            "    return PyString_FromString(temp20);\n"
            "    }\n"
            "  return NULL;\n"
            "}\n"
            "\n",
            data->ClassName, data->ClassName);

    /* add the PrintRevisions method to vtkObjectBase. */
    fprintf(fp,
            "PyObject *PyvtkObjectBase_PrintRevisions(PyObject *self, PyObject *args)\n"
            "{\n"
            "  %s *op;\n"
            "  op = (%s *)PyArg_VTKParseTuple(self, args, (char*)\"\");\n"
            "  if (op)\n"
            "    {\n"
            "    vtksys_ios::ostringstream vtkmsg_with_warning_C4701;\n"
            "    op->PrintRevisions(vtkmsg_with_warning_C4701);\n"
            "    vtkmsg_with_warning_C4701.put('\\0');\n"
            "    PyObject *result = PyString_FromString(vtkmsg_with_warning_C4701.str().c_str());\n"
            "    return result;\n"
            "    }\n"
            "  return NULL;\n"
            "}\n"
            "\n",
            data->ClassName, data->ClassName);
    }

  /* check for New() function */
  for (i = 0; i < data->NumberOfFunctions; i++)
    {
    if (data->Functions[i].Name &&
        strcmp("New",data->Functions[i].Name) == 0 &&
        data->Functions[i].NumberOfArguments == 0)
      {
      class_has_new = 1;
      }
    }

  /* now output all the methods are wrappable */
  if (data->NumberOfSuperClasses || !data->IsAbstract)
    {
    vtkWrapPython_GenerateMethods(fp, data, class_has_new, 0);
    }

  /* output the class initilization function */

  /* the New method for vtkObjectBase */
  if (strcmp(data->ClassName,"vtkObjectBase") == 0)
    {
    if (class_has_new)
      {
      fprintf(fp,
              "static vtkObjectBase *%sStaticNew()\n"
              "{\n"
              "  return %s::New();\n"
              "}\n"
              "\n",
              data->ClassName, data->ClassName);
      }

    fprintf(fp,
            "PyObject *PyVTKClass_%sNew(char *modulename)\n"
            "{\n",
            data->ClassName);

    if (class_has_new)
      {
      fprintf(fp,
              "  return PyVTKClass_New(&%sStaticNew,\n",
              data->ClassName);
      }
    else
      {
      fprintf(fp,
              "  return PyVTKClass_New(NULL,\n");
      }

    fprintf(fp,
            "                        Py%sMethods,\n"
            "                        (char*)\"%s\",modulename,\n"
            "                        (char**)%sDoc(),0);\n"
            "}\n"
            "\n",
            data->ClassName, data->ClassName, data->ClassName);
    }

  /* the New method for descendants of vtkObjectBase */
  else if (data->NumberOfSuperClasses)
    {
    if (class_has_new)
      {
      fprintf(fp,
              "static vtkObjectBase *%sStaticNew()\n"
              "{\n"
              "  return %s::New();\n"
              "}\n"
              "\n",
              data->ClassName, data->ClassName);
      }

    fprintf(fp,
            "PyObject *PyVTKClass_%sNew(char *modulename)\n"
            "{\n",
            data->ClassName);

    if (class_has_new)
      {
      fprintf(fp,
              "  return PyVTKClass_New(&%sStaticNew,\n",
              data->ClassName);
      }
    else
      {
      fprintf(fp,
              "  return PyVTKClass_New(NULL,\n");
      }

    fprintf(fp,
            "                        Py%sMethods,\n"
            "                        (char*)\"%s\",modulename,\n"
            "                        (char**)%sDoc(),\n"
            "                        PyVTKClass_%sNew(modulename));\n"
            "}\n"
            "\n",
            data->ClassName, data->ClassName, data->ClassName,
            data->SuperClasses[0]);
    }

  /* the New method of 'special' non-vtkObject classes */
  else if (!data->IsAbstract)
    {
    /* handle all constructors */
    vtkWrapPython_GenerateMethods(fp, data, class_has_new, 1);

    /* the method table for the New method */
    fprintf(fp,
            "static PyMethodDef Py%sNewMethod = \\\n"
            "{ (char*)\"%s\",  (PyCFunction)Py%s_%s, 1,\n"
            "  (char*)\"\" };\n"
            "\n",
            data->ClassName, data->ClassName, data->ClassName,
            data->ClassName);

    /* the copy constructor */
    fprintf(fp,
           "static void *vtkSpecial_%sCopy(void *obj)\n"
            "{\n"
            "  if (obj)\n"
            "    {\n"
            "    return new %s(*static_cast<%s*>(obj));\n"
            "    }\n"
            "  return 0;\n"
            "}\n"
            "\n",
            data->ClassName, data->ClassName, data->ClassName);

    /* the destructor */
    fprintf(fp,
            "static void vtkSpecial_%sDelete(void *obj)\n"
            "{\n"
            "  if (obj)\n"
            "    {\n"
            "    delete (static_cast<%s*>(obj));\n"
            "    }\n"
            "}\n"
            "\n",
            data->ClassName, data->ClassName);

    /* the printer */
    fprintf(fp,
            "static void vtkSpecial_%sPrint(ostream &os, void *obj)\n"
            "{\n"
            "  if (obj)\n"
            "    {\n"
            "    os << *(static_cast<%s*>(obj));\n"
            "    }\n"
            "}\n"
            "\n",
            data->ClassName, data->ClassName);

    /* the exported New method */
    fprintf(fp,
            "PyObject *PyVTKClass_%sNew(char *)\n"
            "{\n"
            "  PyVTKSpecialTypeInfo *info =\n"
            "    vtkPythonAddSpecialTypeToHash(\n"
            "      (char *)\"%s\", (char**)%sDoc(), Py%sMethods,\n"
            "      Py%s_%sMethods, &vtkSpecial_%sCopy,\n"
            "      &vtkSpecial_%sDelete, &vtkSpecial_%sPrint);\n"
            "\n"
            "  Py%sNewMethod.ml_doc = PyString_AsString(info->docstring);\n"
            "\n"
            "  return PyCFunction_New(&Py%sNewMethod,Py_None);\n"
            "}\n"
            "\n",
            data->ClassName, data->ClassName, data->ClassName,
            data->ClassName, data->ClassName, data->ClassName,
            data->ClassName, data->ClassName, data->ClassName,
            data->ClassName, data->ClassName);
    }

  /* the New method for un-wrappable classes returns "NULL" */
  else
    {
    fprintf(fp,
            "PyObject *PyVTKClass_%sNew(char *)\n"
            "{\n"
            "  return NULL;\n"
            "}\n"
            "\n",
            data->ClassName);
    }

  /* the docstring for the class, as a static var ending in "Doc" */
  if (data->NumberOfSuperClasses || !data->IsAbstract)
    {
    fprintf(fp,
            "const char **%sDoc()\n"
            "{\n"
            "  static const char *docstring[] = {\n",
            data->ClassName);

    vtkWrapPython_ClassDoc(fp,data);

    fprintf(fp,
            "    NULL\n"
            "  };\n"
            "\n"
            "  return docstring;\n"
            "}\n"
            "\n");
    }
}

