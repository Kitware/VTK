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
#include "vtkWrap.h"
#include "vtkWrapText.h"
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

/* declare all variables needed by the wrapper method */
static void vtkWrapPython_DeclareVariables(
  FILE *fp, FunctionInfo *theFunc);

/* get the tuple size for vtkDataArray and subclasses */
static void vtkWrapPython_GetSizesForArrays(
  FILE *fp, FunctionInfo *theFunc, int is_vtkobject);

/* Write the code to convert the arguments with vtkPythonArgs */
static void vtkWrapPython_GetAllArguments(
  FILE *fp, FunctionInfo *currentFunction);

/* save the contents of all arrays prior to calling the function */
static void vtkWrapPython_SaveArrayArgs(
  FILE *fp, FunctionInfo *currentFunction);

/* generate the code that calls the C++ method */
static void vtkWrapPython_GenerateMethodCall(
  FILE *fp, FunctionInfo *currentFunction, ClassInfo *data,
  int is_vtkobject);

/* Write back to all the reference arguments and array arguments */
static void vtkWrapPython_WriteBackToArgs(
  FILE *fp, FunctionInfo *currentFunction);

/* print the code to build python return value from a method */
static void vtkWrapPython_ReturnValue(
  FILE *fp, ValueInfo *val);

/* free any arrays that were allocated */
static void vtkWrapPython_FreeAllocatedArrays(
  FILE *fp, FunctionInfo *currentFunction);

/* Delete object created by conversion constructors */
static void vtkWrapPython_FreeConstructedObjects(
  FILE *fp, FunctionInfo *currentFunction);

/* output the method table for all overloads of a particular method */
static void vtkWrapPython_OverloadMethodDef(
  FILE *fp, ClassInfo *data, int *overloadMap,
  FunctionInfo **wrappedFunctions, int numberOfWrappedFunctions, int fnum,
  int numberOfOccurrences, int is_vtkobject, int all_legacy);

/* a master method to choose which overload to call */
static void vtkWrapPython_OverloadMasterMethod(
  FILE *fp, ClassInfo *data, int *overloadMap, int maxArgs,
  FunctionInfo **wrappedFunctions, int numberOfWrappedFunctions, int fnum,
  int numberOfOccurrences, int is_vtkobject, int all_legacy);

/* output the MethodDef table for this class */
static void vtkWrapPython_ClassMethodDef(
  FILE *fp, ClassInfo *data,
  FunctionInfo **wrappedFunctions, int numberOfWrappedFunctions, int fnum);

/* write out a python type object */
static void vtkWrapPython_GenerateSpecialType(
  FILE *fp, ClassInfo *data, FileInfo *finfo, HierarchyInfo *hinfo);

/* -------------------------------------------------------------------- */
/* prototypes for utility methods */

/* check for wrappability, flags may be VTK_WRAP_ARG or VTK_WRAP_RETURN */
static int vtkWrapPython_IsValueWrappable(
  ValueInfo *val, HierarchyInfo *hinfo, int flags);

/* check whether a method is wrappable */
static int vtkWrapPython_MethodCheck(
  FunctionInfo *currentFunction, HierarchyInfo *hinfo);

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

/* look for all signatures of the specified method */
static int vtkWrapPython_CountAllOccurrences(
  FunctionInfo **wrappedFunctions, int n, int fnum,
  int *all_static, int *all_legacy);

/* generate an int array that maps arg counts to overloads */
static int *vtkWrapPython_ArgCountToOverloadMap(
  FunctionInfo **wrappedFunctions, int numberOfWrappedFunctions,
  int fnum, int is_vtkobject, int *nmax, int *overlap);

/* create a string for checking arguments against available signatures */
static char *vtkWrapPython_ArgCheckString(
  int isvtkobjmethod, FunctionInfo *currentFunction);


/* -------------------------------------------------------------------- */
/* A struct for special types to store info about the type, it is fairly
 * small because not many operators or special features are wrapped */
typedef struct _SpecialTypeInfo
{
  int has_print;    /* there is "<<" stream operator */
  int has_compare;  /* there are comparison operators e.g. "<" */
} SpecialTypeInfo;


/* -------------------------------------------------------------------- */
/* Get the header file for the specified class */
static const char *vtkWrapPython_ClassHeader(
  HierarchyInfo *hinfo, const char *classname)
{
  HierarchyEntry *entry;

  /* if "hinfo" is present, use it to find the file */
  if (hinfo)
    {
    entry = vtkParseHierarchy_FindEntry(hinfo, classname);
    if (entry)
      {
      return entry->HeaderFile;
      }
    }

  /* otherwise, use these hard-coded entries */
  if (strcmp(classname, "vtkArrayCoordinates") == 0)
    {
    return "vtkArrayCoordinates.h";
    }
  else if (strcmp(classname, "vtkArrayExtents") == 0)
    {
    return "vtkArrayExtents.h";
    }
  else if (strcmp(classname, "vtkArrayExtentsList") == 0)
    {
    return "vtkArrayExtentsList.h";
    }
  else if (strcmp(classname, "vtkArrayRange") == 0)
    {
    return "vtkArrayRange.h";
    }
  else if (strcmp(classname, "vtkTimeStamp") == 0)
    {
    return "vtkTimeStamp.h";
    }
  else if (strcmp(classname, "vtkVariant") == 0)
    {
    return "vtkVariant.h";
    }
  else if (strcmp(classname, "vtkStdString") == 0)
    {
    return "vtkStdString.h";
    }
  else if (strcmp(classname, "vtkUnicodeString") == 0)
    {
    return "vtkUnicodeString.h";
    }

  return 0;
}

/* -------------------------------------------------------------------- */
/* Declare all local variables used by the wrapper method */
static void vtkWrapPython_DeclareVariables(
  FILE *fp, FunctionInfo *theFunc)
{
  ValueInfo *arg;
  int i, n;
  int storageSize;

  n = vtkWrap_CountWrappedArgs(theFunc);

  /* temp variables for arg values */
  for (i = 0; i < n; i++)
    {
    arg = theFunc->Arguments[i];

    /* a callable python object for function args */
    if (vtkWrap_IsFunction(arg))
      {
      fprintf(fp,
              "  PyObject *temp%d = NULL;\n",
              i);
      break;
      }

    /* make a "temp" variable for the argument */
    vtkWrap_DeclareVariable(fp, arg, "temp", i, VTK_WRAP_ARG);

    /* temps for conversion constructed objects, which only occur
     * for special objects */
    if (vtkWrap_IsSpecialObject(arg) &&
        !vtkWrap_IsNonConstRef(arg))
      {
      fprintf(fp,
              "  PyObject *pobj%d = NULL;\n",
              i);
      }

    /* temps for arrays */
    if (vtkWrap_IsArray(arg) || vtkWrap_IsNArray(arg))
      {
      storageSize = 4;
      if (!vtkWrap_IsConst(arg) &&
          !vtkWrap_IsSetVectorMethod(theFunc))
        {
        /* for saving a copy of the array */
        vtkWrap_DeclareVariable(fp, arg, "save", i, VTK_WRAP_ARG);
        storageSize *= 2;
        }
      if (arg->CountHint)
        {
        fprintf(fp,
                "  %s small%d[%d];\n",
                vtkWrap_GetTypeName(arg), i, storageSize);
        }
      /* write an int array containing the dimensions */
      vtkWrap_DeclareVariableSize(fp, arg, "size", i);
      }
    }

  if (theFunc->ReturnValue)
    {
    /* temp variable for C++-type return value */
    vtkWrap_DeclareVariable(fp, theFunc->ReturnValue,
      "tempr", -1, VTK_WRAP_RETURN);

    /* the size for a one-dimensional array */
    if (vtkWrap_IsArray(theFunc->ReturnValue))
      {
      fprintf(fp,
              "  int sizer = %d;\n",
              (theFunc->ReturnValue->CountHint ?
               0 : theFunc->ReturnValue->Count));
      }
    }

  /* temp variable for the Python return value */
  fprintf(fp,
          "  PyObject *result = NULL;\n"
          "\n");
}

/* -------------------------------------------------------------------- */
/* Get the size for vtkDataArray Tuple arguments */
static void vtkWrapPython_GetSizesForArrays(
  FILE *fp, FunctionInfo *theFunc, int is_vtkobject)
{
  int i, j, n;
  const char *ndnt;
  const char *mtwo;

  /* the indentation amount */
  ndnt = (is_vtkobject ? "  " : "");

  n = vtkWrap_CountWrappedArgs(theFunc);

  j = (is_vtkobject ? 1 : 0);
  for (i = 0; i < n; i++)
    {
    if (theFunc->Arguments[i]->CountHint)
      {
      if (j == 1)
        {
        fprintf(fp,
                "  if (op)\n"
                "    {\n");
        }
      j += 2;
      fprintf(fp,
              "  %ssize%d = op->%s;\n",
              ((j & 1) != 0 ? "  " : ""), i,
              theFunc->Arguments[i]->CountHint);

      /* for non-const arrays, alloc twice as much space */
      mtwo = "";
      if (!vtkWrap_IsConst(theFunc->Arguments[i]) &&
          !vtkWrap_IsSetVectorMethod(theFunc))
        {
        mtwo = "2*";
        }

      fprintf(fp,
              "  %stemp%d = small%d;\n"
              "  %sif (size%d > 4)\n"
              "    %s{\n"
              "    %stemp%d = new %s[%ssize%d];\n"
              "    %s}\n",
              ndnt, i, i, ndnt, i, ndnt, ndnt,
              i, vtkWrap_GetTypeName(theFunc->Arguments[i]), mtwo, i,
              ndnt);

      if (*mtwo)
        {
        fprintf(fp,
              "  %ssave%d = &temp%d[size%d];\n",
              ndnt, i, i, i);
        }
      }
    }
  if (theFunc->ReturnValue && theFunc->ReturnValue->CountHint)
    {
    if (j == 1)
      {
      fprintf(fp,
              "  if (op)\n"
              "    {\n");
      }
    j += 2;
    fprintf(fp,
            "  %ssizer = op->%s;\n",
            ((j & 1) != 0 ? "  " : ""),
            theFunc->ReturnValue->CountHint);
    }
  if (j > 1)
    {
    if ((j & 1) != 0)
      {
      fprintf(fp,
                  "    }\n");
      }
    fprintf(fp,
            "\n");
    }
}

/* -------------------------------------------------------------------- */
/* Write the code to convert the arguments with vtkPythonArgs */
static void vtkWrapPython_GetAllArguments(
  FILE *fp, FunctionInfo *currentFunction)
{
  ValueInfo *arg;
  int requiredArgs, totalArgs;
  int i;

  totalArgs = vtkWrap_CountWrappedArgs(currentFunction);
  requiredArgs = vtkWrap_CountRequiredArgs(currentFunction);

  if (requiredArgs == totalArgs)
    {
    fprintf(fp,
            "ap.CheckArgCount(%d)",
            totalArgs);
    }
  else
    {
    fprintf(fp,
            "ap.CheckArgCount(%d, %d)",
            requiredArgs, totalArgs);
    }

  for (i = 0; i < totalArgs; i++)
    {
    arg = currentFunction->Arguments[i];

    fprintf(fp, " &&\n"
            "      ");

    if (i >= requiredArgs)
      {
      fprintf(fp, "(ap.NoArgsLeft() || ");
      }

    if (vtkWrap_IsVTKObject(arg))
      {
      fprintf(fp, "ap.GetVTKObject(temp%d, \"%s\")",
              i, arg->Class);
      }
    else if (vtkWrap_IsSpecialObject(arg) &&
             !vtkWrap_IsNonConstRef(arg))
      {
      fprintf(fp, "ap.GetSpecialObject(temp%d, pobj%d, \"%s\")",
              i, i, arg->Class);
      }
    else if (vtkWrap_IsSpecialObject(arg) &&
             vtkWrap_IsNonConstRef(arg))
      {
      fprintf(fp, "ap.GetSpecialObject(temp%d, \"%s\")",
              i, arg->Class);
      }
    else if (vtkWrap_IsQtEnum(arg))
      {
      fprintf(fp, "ap.GetSIPEnumValue(temp%d, \"%s\")",
              i, arg->Class);
      }
    else if (vtkWrap_IsQtObject(arg))
      {
      fprintf(fp, "ap.GetSIPObject(temp%d, \"%s\")",
              i, arg->Class);
      }
    else if (vtkWrap_IsFunction(arg))
      {
      fprintf(fp, "ap.GetFunction(temp%d)",
              i);
      break;
      }
    else if (vtkWrap_IsVoidPointer(arg))
      {
      fprintf(fp, "ap.GetValue(temp%d)",
              i);
      }
    else if (vtkWrap_IsString(arg) ||
             vtkWrap_IsCharPointer(arg))
      {
      fprintf(fp, "ap.GetValue(temp%d)",
              i);
      }
    else if (vtkWrap_IsNumeric(arg) &&
             vtkWrap_IsScalar(arg))
      {
      fprintf(fp, "ap.GetValue(temp%d)",
              i);
      }
    else if (vtkWrap_IsNArray(arg))
      {
      fprintf(fp, "ap.GetNArray(%.*stemp%d, %d, size%d)",
              (int)(arg->NumberOfDimensions-1), "**********",
              i, arg->NumberOfDimensions, i);
      }
    else if (vtkWrap_IsArray(arg))
      {
      fprintf(fp, "ap.GetArray(temp%d, size%d)",
              i, i);
      }

    if (i >= requiredArgs)
      {
      fprintf(fp, ")");
      }
    }
}


/* -------------------------------------------------------------------- */
/* Convert values into python object and return them within python */

static void vtkWrapPython_ReturnValue(FILE *fp, ValueInfo *val)
{
  const char *deref = "";

  fprintf(fp,
          "    if (!ap.ErrorOccurred())\n"
          "      {\n");

  if (val && vtkWrap_IsRef(val))
    {
    deref = "*";
    }

  if (vtkWrap_IsVoid(val))
    {
    fprintf(fp,
            "      result = ap.BuildNone();\n");
    }
  else if (vtkWrap_IsVTKObject(val))
    {
    fprintf(fp,
            "      result = ap.BuildVTKObject(tempr);\n");
    }
  else if (vtkWrap_IsSpecialObject(val) &&
           vtkWrap_IsRef(val))
    {
    fprintf(fp,
            "      result = ap.BuildSpecialObject(tempr, \"%s\");\n",
            val->Class);
    }
  else if (vtkWrap_IsSpecialObject(val) &&
           !vtkWrap_IsRef(val))
    {
    fprintf(fp,
            "      result = ap.BuildSpecialObject(&tempr, \"%s\");\n",
            val->Class);
    }
  else if (vtkWrap_IsQtObject(val) &&
           (vtkWrap_IsRef(val) || vtkWrap_IsPointer(val)))
    {
    fprintf(fp,
            "      result = ap.BuildSIPObject(tempr, \"%s\", false);\n",
            val->Class);
    }
  else if (vtkWrap_IsQtObject(val) &&
           !vtkWrap_IsRef(val) && !vtkWrap_IsPointer(val))
    {
    fprintf(fp,
            "      result = ap.BuildSIPObject(new %s(tempr), \"%s\", false);\n",
            val->Class, val->Class);
    }
  else if (vtkWrap_IsQtEnum(val))
    {
    fprintf(fp,
            "      result = ap.BuildSIPEnumValue(tempr, \"%s\");\n",
            val->Class);
    }
  else if (vtkWrap_IsCharPointer(val))
    {
    fprintf(fp,
            "      result = ap.BuildValue(tempr);\n");
    }
  else if (vtkWrap_IsVoidPointer(val))
    {
    fprintf(fp,
            "      result = ap.BuildValue(tempr);\n");
    }
  else if (vtkWrap_IsChar(val) && vtkWrap_IsArray(val))
    {
    fprintf(fp,
            "      result = ap.BuildBytes(tempr, sizer);\n");
    }
  else if (vtkWrap_IsArray(val))
    {
    fprintf(fp,
            "      result = ap.BuildTuple(tempr, sizer);\n");
    }
  else
    {
    fprintf(fp,
            "      result = ap.BuildValue(%stempr);\n",
            deref);
    }

  fprintf(fp,
          "      }\n");
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
#if PY_VERSION_HEX >= 0x02030000
      typeChar = 'I';
      break;
#endif
    case VTK_PARSE_INT:
      typeChar = 'i';
      break;
    case VTK_PARSE_UNSIGNED_SHORT:
#if PY_VERSION_HEX >= 0x02030000
      typeChar = 'H';
      break;
#endif
    case VTK_PARSE_SHORT:
      typeChar = 'h';
      break;
    case VTK_PARSE_UNSIGNED_LONG:
#if PY_VERSION_HEX >= 0x02030000
      typeChar = 'k';
      break;
#endif
    case VTK_PARSE_LONG:
      typeChar = 'l';
      break;
    case VTK_PARSE_UNSIGNED_ID_TYPE:
#if PY_VERSION_HEX >= 0x02030000
#ifdef VTK_USE_64BIT_IDS
#ifdef PY_LONG_LONG
      typeChar = 'K';
#else
      typeChar = 'k';
#endif
#else
      typeChar = 'I';
#endif
#endif
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
    case VTK_PARSE_SIZE_T:
    case VTK_PARSE_UNSIGNED_LONG_LONG:
    case VTK_PARSE_UNSIGNED___INT64:
#if PY_VERSION_HEX >= 0x02030000
      typeChar = 'K';
      break;
#endif
    case VTK_PARSE_SSIZE_T:
    case VTK_PARSE_LONG_LONG:
    case VTK_PARSE___INT64:
      typeChar = 'L';
      break;
#else
    case VTK_PARSE_SIZE_T:
    case VTK_PARSE_UNSIGNED_LONG_LONG:
    case VTK_PARSE_UNSIGNED___INT64:
#if PY_VERSION_HEX >= 0x02030000
      typeChar = 'k';
      break;
#endif
    case VTK_PARSE_SSIZE_T:
    case VTK_PARSE_LONG_LONG:
    case VTK_PARSE___INT64:
      typeChar = 'l';
      break;
#endif
    case VTK_PARSE_SIGNED_CHAR:
#if PY_VERSION_HEX >= 0x02030000
      typeChar = 'B';
#else
      typeChar = 'b';
#endif
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
 * An optional arg separator '|' is added before the first arg that
 * has a default value.
 *
 * If any new format characters are added here, they must also be
 * added to vtkPythonUtil::CheckArg() in vtkPythonUtil.cxx
 */

static char *vtkWrapPython_FormatString(FunctionInfo *currentFunction)
{
  static char result[2048]; /* max literal string length */
  size_t currPos = 0;
  ValueInfo *arg;
  unsigned int argtype;
  int i;
  int totalArgs, requiredArgs;

  totalArgs = vtkWrap_CountWrappedArgs(currentFunction);
  requiredArgs = vtkWrap_CountRequiredArgs(currentFunction);

  for (i = 0; i < totalArgs; i++)
    {
    arg = currentFunction->Arguments[i];
    argtype = (arg->Type & VTK_PARSE_UNQUALIFIED_TYPE);

    if (i == requiredArgs)
      {
      /* make all following arguments optional */
      result[currPos++] = '|';
      }

    /* add the format char to the string */
    result[currPos++] = vtkWrapPython_FormatChar(argtype);

    if (((argtype & VTK_PARSE_INDIRECT) == VTK_PARSE_POINTER ||
         (argtype & VTK_PARSE_INDIRECT) == VTK_PARSE_ARRAY) &&
        argtype != VTK_PARSE_OBJECT_PTR &&
        argtype != VTK_PARSE_QOBJECT_PTR)
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
        result[currPos++] = 'O';
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
  static char result[2048]; /* max literal string length */
  size_t currPos = 0;
  ValueInfo *arg;
  unsigned int argtype;
  int i, j, k;
  int totalArgs;

  totalArgs = vtkWrap_CountWrappedArgs(currentFunction);

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

  for (i = 0; i < totalArgs; i++)
    {
    arg = currentFunction->Arguments[i];
    argtype = (arg->Type & VTK_PARSE_UNQUALIFIED_TYPE);

    if ((argtype & VTK_PARSE_BASE_TYPE) == VTK_PARSE_FUNCTION)
      {
      strcpy(&result[currPos], " func");
      currPos += 5;
      }

    else if (argtype == VTK_PARSE_BOOL ||
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
      strcpy(&result[currPos], arg->Class);
      currPos += strlen(arg->Class);
      }

    else if (vtkWrap_IsArray(arg) || vtkWrap_IsNArray(arg))
      {
      result[currPos++] = ' ';
      result[currPos++] = '*';
      result[currPos++] = vtkWrapPython_FormatChar(argtype);
      if (vtkWrap_IsNArray(arg))
        {
        for (j = 1; j < arg->NumberOfDimensions; j++)
          {
          result[currPos++] = '[';
          for (k = 0; arg->Dimensions[j][k]; k++)
            {
            result[currPos++] = arg->Dimensions[j][k];
            }
          result[currPos++] = ']';
          }
        }
      result[currPos] = '\0';
      }
    }

  return result;
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
  ValueInfo *val1;
  ValueInfo *val2;
  int vote1 = 0;
  int vote2 = 0;
  int occ1, occ2;
  unsigned int baseType1, baseType2;
  unsigned int unsigned1, unsigned2;
  unsigned int indirect1, indirect2;
  int i, nargs1, nargs2;
  int argmatch, allmatch;

  if (!name)
    {
    return;
    }

  for (occ1 = fnum; occ1 < numberOfWrappedFunctions; occ1++)
    {
    sig1 = wrappedFunctions[occ1];
    nargs1 = vtkWrap_CountWrappedArgs(sig1);

    if (sig1->Name && strcmp(sig1->Name, name) == 0)
      {
      for (occ2 = occ1+1; occ2 < numberOfWrappedFunctions; occ2++)
        {
        sig2 = wrappedFunctions[occ2];
        nargs2 = vtkWrap_CountWrappedArgs(sig2);
        vote1 = 0;
        vote2 = 0;

        if (nargs2 == nargs1 &&
            sig2->Name && strcmp(sig2->Name, name) == 0)
          {
          allmatch = 1;
          for (i = 0; i < nargs1; i++)
            {
            argmatch = 0;
            val1 = sig1->Arguments[i];
            val2 = sig2->Arguments[i];
            if (val1->NumberOfDimensions != val2->NumberOfDimensions)
              {
              vote1 = 0;
              vote2 = 0;
              allmatch = 0;
              break;
              }
            else
              {
              baseType1 = (val1->Type & VTK_PARSE_BASE_TYPE);
              baseType2 = (val2->Type & VTK_PARSE_BASE_TYPE);

              unsigned1 = (baseType1 & VTK_PARSE_UNSIGNED);
              unsigned2 = (baseType2 & VTK_PARSE_UNSIGNED);

              baseType1 = (baseType1 & ~VTK_PARSE_UNSIGNED);
              baseType2 = (baseType2 & ~VTK_PARSE_UNSIGNED);

              indirect1 = (val1->Type & VTK_PARSE_INDIRECT);
              indirect2 = (val2->Type & VTK_PARSE_INDIRECT);

              if ((indirect1 == indirect2) &&
                  (unsigned1 == unsigned2) &&
                  (baseType1 == baseType2) &&
                  ((val1->Type & VTK_PARSE_CONST) ==
                   (val2->Type & VTK_PARSE_CONST)))
                {
                argmatch = 1;
                }
              /* double preceeds float */
              else if ((indirect1 == indirect2) &&
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
              /* a string method preceeds a "char *" method */
              else if ((baseType2 == VTK_PARSE_CHAR) &&
                       (indirect2 == VTK_PARSE_POINTER) &&
                       (baseType1 == VTK_PARSE_STRING) &&
                       ((indirect1 == VTK_PARSE_REF) || (indirect1 == 0)))
                {
                if (!vote2) { vote1 = 1; }
                }
              else if ((baseType1 == VTK_PARSE_CHAR) &&
                       (indirect1 == VTK_PARSE_POINTER) &&
                       (baseType2 == VTK_PARSE_STRING) &&
                       ((indirect2 == VTK_PARSE_REF) || (indirect2 == 0)))
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
                allmatch = 0;
                break;
                }
              }

            if (argmatch == 0)
              {
              allmatch = 0;
              }
            }

          /* if all args match, prefer the non-const method */
          if (allmatch)
            {
            if (sig1->IsConst)
              {
              vote2 = 1;
              }
            else if (sig2->IsConst)
              {
              vote1 = 1;
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
/* Look for all signatures of the specified method.  Return the number
 * found, as well as whether all signatures were static or legacy */
static int vtkWrapPython_CountAllOccurrences(
  FunctionInfo **wrappedFunctions, int n,
  int fnum, int *all_static, int *all_legacy)
{
  const char *name;
  int occ;
  int numberOfOccurrences = 0;

  *all_static = 1;
  *all_legacy = 1;

  name = wrappedFunctions[fnum]->Name;

  for (occ = fnum; occ < n; occ++)
    {
    /* is it the same name */
    if (wrappedFunctions[occ]->Name &&
        !strcmp(name, wrappedFunctions[occ]->Name))
      {
      /* increment the signature count */
      numberOfOccurrences++;

      /* check for static methods */
      if (!wrappedFunctions[occ]->IsStatic)
        {
        *all_static = 0;
        }

      /* check for legacy */
      if (!wrappedFunctions[occ]->IsLegacy)
        {
        *all_legacy = 0;
        }
      }
    }

  return numberOfOccurrences;
}

/* -------------------------------------------------------------------- */
/* Generate an int array that maps arg counts to overloads.
 * Each element in the array will either contain the index of the
 * overload that it maps to, or -1 if it maps to multiple overloads,
 * or zero if it does not map to any.  The length of the array is
 * returned in "nmax". The value of "overlap" is set to 1 if there
 * are some arg counts that map to more than one method. */

static int *vtkWrapPython_ArgCountToOverloadMap(
  FunctionInfo **wrappedFunctions, int numberOfWrappedFunctions,
  int fnum, int is_vtkobject, int *nmax, int *overlap)
{
  static int overloadMap[512];
  int totalArgs, requiredArgs;
  int occ, occCounter;
  FunctionInfo *theOccurrence;
  FunctionInfo *theFunc;
  int mixed_static, any_static;
  int i;

  *nmax = 0;
  *overlap = 0;

  theFunc = wrappedFunctions[fnum];

  any_static = 0;
  mixed_static = 0;
  for (i = fnum; i < numberOfWrappedFunctions; i++)
    {
    if (wrappedFunctions[i]->Name &&
        strcmp(wrappedFunctions[i]->Name, theFunc->Name) == 0)
      {
      if (wrappedFunctions[i]->IsStatic)
        {
        any_static = 1;
        }
      else if (any_static)
        {
        mixed_static = 1;
        }
      }
    }

  for (i = 0; i < 100; i++)
    {
    overloadMap[i] = 0;
    }

  occCounter = 0;
  for (occ = fnum; occ < numberOfWrappedFunctions; occ++)
    {
    theOccurrence = wrappedFunctions[occ];

    if (theOccurrence->Name == 0 ||
        strcmp(theOccurrence->Name, theFunc->Name) != 0)
      {
      continue;
      }

    occCounter++;

    totalArgs = vtkWrap_CountWrappedArgs(theOccurrence);
    requiredArgs = vtkWrap_CountRequiredArgs(theOccurrence);

    /* vtkobject calls might have an extra "self" arg in front */
    if (mixed_static && is_vtkobject &&
        !theOccurrence->IsStatic)
      {
      totalArgs++;
      }

    if (totalArgs > *nmax)
      {
      *nmax = totalArgs;
      }

    for (i = requiredArgs; i <= totalArgs && i < 100; i++)
      {
      if (overloadMap[i] == 0)
        {
        overloadMap[i] = occCounter;
        }
      else
        {
        overloadMap[i] = -1;
        *overlap = 1;
        }
      }
    }

  return overloadMap;
}



/* -------------------------------------------------------------------- */
/* Save a copy of each non-const array arg, so that we can check
 * if they were changed by the method call */
void vtkWrapPython_SaveArrayArgs(FILE *fp, FunctionInfo *currentFunction)
{
  const char *asterisks = "**********";
  ValueInfo *arg;
  int i, j, n, m;
  int noneDone = 1;

  /* do nothing for SetVector macros */
  if (vtkWrap_IsSetVectorMethod(currentFunction))
    {
    return;
    }

  m = vtkWrap_CountWrappedArgs(currentFunction);

  /* save arrays for args that are non-const */
  for (i = 0; i < m; i++)
    {
    arg = currentFunction->Arguments[i];
    n = arg->NumberOfDimensions;
    if (n < 1 && vtkWrap_IsArray(arg))
      {
      n = 1;
      }

    if ((vtkWrap_IsArray(arg) || vtkWrap_IsNArray(arg)) &&
        (arg->Type & VTK_PARSE_CONST) == 0)
      {
      noneDone = 0;

      fprintf(fp,
              "    ap.SaveArray(%.*stemp%d, %.*ssave%d, ",
              (n-1), asterisks, i, (n-1), asterisks, i);

      if (vtkWrap_IsNArray(arg))
        {
        for (j = 0; j < arg->NumberOfDimensions; j++)
          {
          fprintf(fp, "%ssize%d[%d]", (j == 0 ? "" : "*"), i, j);
          }
        }
      else
        {
        fprintf(fp, "size%d", i);
        }

      fprintf(fp, ");\n");
      }
    }

  if (!noneDone)
    {
    fprintf(fp,
            "\n");
    }
}

/* -------------------------------------------------------------------- */
/* generate the code that calls the C++ method */
static void vtkWrapPython_GenerateMethodCall(
  FILE *fp, FunctionInfo *currentFunction, ClassInfo *data,
  int is_vtkobject)
{
  char methodname[256];
  ValueInfo *arg;
  int totalArgs;
  int is_constructor;
  int i, k, n;

  totalArgs = vtkWrap_CountWrappedArgs(currentFunction);

  is_constructor = vtkWrap_IsConstructor(data, currentFunction);

  /* for vtkobjects, do a bound call and an unbound call */
  n = 1;
  if (is_vtkobject &&
      !currentFunction->IsStatic &&
      !currentFunction->IsPureVirtual &&
      !is_constructor)
    {
    n = 2;
    }

  /* print the code that calls the method */
  for (k = 0; k < n; k++)
    {
    if (k == 1)
      {
      /* unbound method call */
      sprintf(methodname, "op->%s::%s",
              data->Name, currentFunction->Name);
      }
    else if (currentFunction->IsStatic)
      {
      /* static method call */
      sprintf(methodname, "%s::%s",
              data->Name, currentFunction->Name);
      }
    else if (is_constructor)
      {
      /* constructor call */
      sprintf(methodname, "new %s", currentFunction->Name);
      }
    else
      {
      /* standard bound method call */
      sprintf(methodname, "op->%s", currentFunction->Name);
      }

    if (n == 2)
      {
      /* need to check if it is a bound call or unbound */
      if (k == 0)
        {
        fprintf(fp,
                "    if (ap.IsBound())\n"
                "      {\n"
                "  ");
        }
      else
        {
        fprintf(fp,
                "    else\n"
                "      {\n"
                "  ");
        }
      }

    if (is_constructor)
      {
      fprintf(fp,
              "    %s *op = %s(",
              data->Name, methodname);
      }
    else if (vtkWrap_IsVoid(currentFunction->ReturnValue))
      {
      fprintf(fp,
              "    %s(",
              methodname);
      }
    else if (vtkWrap_IsRef(currentFunction->ReturnValue))
      {
      fprintf(fp,
              "    tempr = &%s(",
              methodname);
      }
    else
      {
      fprintf(fp,
              "    tempr = %s(",
              methodname);
      }

    /* print all the arguments in the call */
    for (i = 0; i < totalArgs; i++)
      {
      arg = currentFunction->Arguments[i];

      if (vtkWrap_IsFunction(arg))
        {
        fprintf(fp,"\n"
                "        (temp%d == Py_None ? NULL : vtkPythonVoidFunc),\n"
                "        (temp%d == Py_None ? NULL : temp%d));\n",
                i, i, i);
        fprintf(fp,
                "      if (temp%d != Py_None)\n"
                "        {\n"
                "        Py_INCREF(temp%d);\n"
                "        }\n"
                "      %sArgDelete(\n"
                "        (temp%d == Py_None ? NULL : vtkPythonVoidFuncArgDelete)",
                i, i, methodname, i);
        break;
        }

      if (i)
        {
        fprintf(fp,", ");
        }

      if ((vtkWrap_IsSpecialObject(arg) ||
           vtkWrap_IsQtObject(arg)) &&
          !vtkWrap_IsPointer(arg))
        {
        fprintf(fp,"*temp%i",i);
        }
      else
        {
        fprintf(fp,"temp%i",i);
        }
      }
    fprintf(fp,");\n");

    /* end the "if (ap.IsBound())" clause */
    if (n == 2)
      {
      fprintf(fp,
              "      }\n");
      }
    }

  fprintf(fp,
          "\n");
}

/* -------------------------------------------------------------------- */
/* Write back to all the reference arguments and array arguments that
 * were passed, but only write to arrays if the array has changed and
 * the array arg was non-const */
static void vtkWrapPython_WriteBackToArgs(
  FILE *fp, FunctionInfo *currentFunction)
{
  const char *asterisks = "**********";
  ValueInfo *arg;
  int i, j, n, m;

  /* do nothing for SetVector macros */
  if (vtkWrap_IsSetVectorMethod(currentFunction))
    {
    return;
    }

  m = vtkWrap_CountWrappedArgs(currentFunction);

  /* check array value change for args that are non-const */
  for (i = 0; i < m; i++)
    {
    arg = currentFunction->Arguments[i];
    n = arg->NumberOfDimensions;
    if (n < 1 && vtkWrap_IsArray(arg))
      {
      n = 1;
      }

    if (vtkWrap_IsNonConstRef(arg) &&
        !vtkWrap_IsObject(arg))
      {
      fprintf(fp,
              "    if (!ap.ErrorOccurred())\n"
              "      {\n"
              "      ap.SetArgValue(%d, temp%d);\n"
              "      }\n",
              i, i);
      }

    else if ((vtkWrap_IsArray(arg) || vtkWrap_IsNArray(arg)) &&
             !vtkWrap_IsConst(arg) &&
             !vtkWrap_IsSetVectorMethod(currentFunction))
      {
      fprintf(fp,
              "    if (ap.ArrayHasChanged(%.*stemp%d, %.*ssave%d, ",
              (n-1), asterisks, i, (n-1), asterisks, i);

      if (vtkWrap_IsNArray(arg))
        {
        for (j = 0; j < arg->NumberOfDimensions; j++)
          {
          fprintf(fp, "%ssize%d[%d]", (j == 0 ? "" : "*"), i, j);
          }
        }
      else
        {
        fprintf(fp, "size%d", i);
        }

      fprintf(fp, ") &&\n"
              "        !ap.ErrorOccurred())\n"
              "      {\n");

      if (vtkWrap_IsNArray(arg))
        {
        fprintf(fp,
                "      ap.SetNArray(%d, %.*stemp%d, %d, size%d);\n",
                i, (n-1), asterisks, i, n, i);
        }
      else
        {
        fprintf(fp,
                "      ap.SetArray(%d, temp%d, size%d);\n",
                i, i, i);
        }

      fprintf(fp,
              "      }\n"
              "\n");
      }
    }
}

/* -------------------------------------------------------------------- */
/* Free any arrays that were allocated */
static void vtkWrapPython_FreeAllocatedArrays(
  FILE *fp, FunctionInfo *currentFunction)
{
  ValueInfo *arg;
  int i, j, n;

  n = vtkWrap_CountWrappedArgs(currentFunction);

  /* check array value change for args that are non-const */
  j = 0;
  for (i = 0; i < n; i++)
    {
    arg = currentFunction->Arguments[i];

    if (arg->CountHint)
      {
      fprintf(fp,
              "  if (temp%d && temp%d != small%d)\n"
              "    {\n"
              "    delete [] temp%d;\n"
              "    }\n",
              i, i, i, i);
      j = 1;
      }
    }

  if (j)
    {
    fprintf(fp,
            "\n");
    }
}

/* -------------------------------------------------------------------- */
/* If any conversion constructors might have been used, then delete
 * the objects that were created */
static void vtkWrapPython_FreeConstructedObjects(
  FILE *fp, FunctionInfo *currentFunction)
{
  ValueInfo *arg;
  int i, j, n;

  n = vtkWrap_CountWrappedArgs(currentFunction);

  /* check array value change for args that are non-const */
  j = 0;
  for (i = 0; i < n; i++)
    {
    arg = currentFunction->Arguments[i];

    if (vtkWrap_IsSpecialObject(arg) &&
        !vtkWrap_IsNonConstRef(arg))
      {
      fprintf(fp,
              "  Py_XDECREF(pobj%d);\n",
              i);
      j = 1;
      }
    }

  if (j)
    {
    fprintf(fp,
            "\n");
    }
}

/* -------------------------------------------------------------------- */
/* output the method table for all overloads of a particular method,
 * this is also used to write out all constructors for the class */

static void vtkWrapPython_OverloadMethodDef(
  FILE *fp, ClassInfo *data, int *overloadMap,
  FunctionInfo **wrappedFunctions, int numberOfWrappedFunctions,
  int fnum, int numberOfOccurrences, int is_vtkobject, int all_legacy)
{
  char occSuffix[8];
  int occ, occCounter;
  FunctionInfo *theOccurrence;
  FunctionInfo *theFunc;
  int totalArgs, requiredArgs;
  int i;
  int putInTable;

  theFunc = wrappedFunctions[fnum];

  if (all_legacy)
    {
    fprintf(fp,
            "#if !defined(VTK_LEGACY_REMOVE)\n");
    }

  fprintf(fp,
         "static PyMethodDef Py%s_%s_Methods[] = {\n",
          data->Name, theFunc->Name);

  occCounter = 0;
  for (occ = fnum; occ < numberOfWrappedFunctions; occ++)
    {
    theOccurrence = wrappedFunctions[occ];

    if (theOccurrence->Name == 0 ||
        strcmp(theOccurrence->Name, theFunc->Name) != 0)
      {
      continue;
      }

    occCounter++;

    totalArgs = vtkWrap_CountWrappedArgs(theOccurrence);
    requiredArgs = vtkWrap_CountRequiredArgs(theOccurrence);

    putInTable = 0;

    /* all conversion constructors must go into the table */
    if (vtkWrap_IsConstructor(data, theOccurrence) &&
        requiredArgs <= 1 && totalArgs >= 1 &&
        !theOccurrence->IsExplicit)
      {
      putInTable = 1;
      }

    /* all methods that overlap with others must go in the table */
    for (i = requiredArgs; i <= totalArgs; i++)
      {
      if (overloadMap[i] == -1)
        {
        putInTable = 1;
        }
      }

    if (!putInTable)
      {
      continue;
      }

    if (theOccurrence->IsLegacy && !all_legacy)
      {
      fprintf(fp,
             "#if !defined(VTK_LEGACY_REMOVE)\n");
      }

    /* method suffix to distinguish between signatures */
    occSuffix[0] = '\0';
    if (numberOfOccurrences > 1)
      {
      sprintf(occSuffix, "_s%d", occCounter);
      }

    fprintf(fp,
            "  {NULL, Py%s_%s%s, 1,\n"
            "   (char*)\"%s\"},\n",
            data->Name, wrappedFunctions[occ]->Name,
            occSuffix,
            vtkWrapPython_ArgCheckString(
              (is_vtkobject && !theOccurrence->IsStatic),
              wrappedFunctions[occ]));

    if (theOccurrence->IsLegacy && !all_legacy)
      {
      fprintf(fp,
              "#endif\n");
      }
    }

  fprintf(fp,
          "  {NULL, NULL, 0, NULL}\n"
          "};\n");

  if (all_legacy)
    {
    fprintf(fp,
            "#endif\n");
    }

  fprintf(fp,
          "\n");
}

/* -------------------------------------------------------------------- */
/* make a method that will choose which overload to call */

static void vtkWrapPython_OverloadMasterMethod(
  FILE *fp, ClassInfo *data, int *overloadMap, int maxArgs,
  FunctionInfo **wrappedFunctions, int numberOfWrappedFunctions, int fnum,
  int numberOfOccurrences, int is_vtkobject, int all_legacy)
{
  FunctionInfo *currentFunction;
  int overlap = 0;
  int occ;
  int i;
  int foundOne;
  int any_static = 0;

  currentFunction = wrappedFunctions[fnum];

  for (i = fnum; i < numberOfWrappedFunctions; i++)
    {
    if (wrappedFunctions[i]->Name &&
        strcmp(wrappedFunctions[i]->Name, currentFunction->Name) == 0)
      {
      if (wrappedFunctions[i]->IsStatic)
        {
        any_static = 1;
        }
      }
    }

  for (i = 0; i <= maxArgs; i++)
    {
    if (overloadMap[i] == -1)
      {
      overlap = 1;
      }
    }

  if (all_legacy)
    {
    fprintf(fp,
            "#if !defined(VTK_LEGACY_REMOVE)\n");
    }

  fprintf(fp,
          "static PyObject *\n"
          "Py%s_%s(PyObject *self, PyObject *args)\n"
          "{\n",
           data->Name, currentFunction->Name);

  if (overlap)
    {
    fprintf(fp,
          "  PyMethodDef *methods = Py%s_%s_Methods;\n",
           data->Name, currentFunction->Name);
    }

  fprintf(fp,
          "  int nargs = vtkPythonArgs::GetArgCount(%sargs);\n"
          "\n",
          ((is_vtkobject && !any_static) ? "self, " : ""));

  fprintf(fp,
          "  switch(nargs)\n"
          "    {\n");

  for (occ = 1; occ <= numberOfOccurrences; occ++)
    {
    foundOne = 0;
    for (i = 0; i <= maxArgs; i++)
      {
      if (overloadMap[i] == occ)
        {
        fprintf(fp,
                "    case %d:\n",
                i);
        foundOne = 1;
        }
      }
    if (foundOne)
      {
      fprintf(fp,
              "      return Py%s_%s_s%d(self, args);\n",
              data->Name, currentFunction->Name, occ);
      }
    }

  if (overlap)
    {
    for (i = 0; i <= maxArgs; i++)
      {
      if (overloadMap[i] == -1)
        {
        fprintf(fp,
                "    case %d:\n",
                i);
        }
      }
    fprintf(fp,
            "      return vtkPythonOverload::CallMethod(methods, self, args);\n");
    }

  fprintf(fp,
          "    }\n"
          "\n");

  fprintf(fp,
          "  vtkPythonArgs::ArgCountError(nargs, \"%.200s\");\n",
          currentFunction->Name);

  fprintf(fp,
          "  return NULL;\n"
          "}\n"
          "\n");

  if (all_legacy)
    {
    fprintf(fp,
            "#endif\n");
    }

  fprintf(fp,"\n");
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
  char occSuffix[8];
  int i;
  int fnum, occ, occCounter;
  int numberOfOccurrences;
  int all_legacy, all_static;
  int numberOfWrappedFunctions = 0;
  FunctionInfo **wrappedFunctions;
  FunctionInfo *theOccurrence;
  FunctionInfo *theFunc;
  const char *ccp;
  char *cp;
  int *overloadMap = NULL;
  int maxArgs = 0;
  int overlap = 0;

  wrappedFunctions = (FunctionInfo **)malloc(
    data->NumberOfFunctions*sizeof(FunctionInfo *));

  /* output any custom methods */
  vtkWrapPython_CustomMethods(fp, data, do_constructors);

  /* modify the arg count for vtkDataArray methods */
  vtkWrap_FindCountHints(data, hinfo);

  /* go through all functions and see which are wrappable */
  for (i = 0; i < data->NumberOfFunctions; i++)
    {
    theFunc = data->Functions[i];

    /* check for wrappability */
    if (vtkWrapPython_MethodCheck(theFunc, hinfo) &&
        !vtkWrap_IsDestructor(data, theFunc) &&
        (!vtkWrap_IsConstructor(data, theFunc) == !do_constructors))
      {
      ccp = vtkWrapText_PythonSignature(theFunc);
      cp = (char *)malloc(strlen(ccp)+1);
      strcpy(cp, ccp);
      theFunc->Signature = cp;
      wrappedFunctions[numberOfWrappedFunctions++] = theFunc;
      }
    }

  /* write out the wrapper for each function in the array */
  for (fnum = 0; fnum < numberOfWrappedFunctions; fnum++)
    {
    theFunc = wrappedFunctions[fnum];

    /* check for type precedence, don't need a "float" method if a
       "double" method exists */
    vtkWrapPython_RemovePreceededMethods(
      wrappedFunctions, numberOfWrappedFunctions, fnum);

    /* if theFunc wasn't removed, process all its signatures */
    if (theFunc->Name)
      {
      fprintf(fp,"\n");

      /* count all signatures, see if they are static methods or legacy */
      numberOfOccurrences = vtkWrapPython_CountAllOccurrences(
        wrappedFunctions, numberOfWrappedFunctions, fnum,
        &all_static, &all_legacy);

      /* find all occurances of this method */
      occCounter = 0;
      for (occ = fnum; occ < numberOfWrappedFunctions; occ++)
        {
        theOccurrence = wrappedFunctions[occ];

        /* is it the same name */
        if (theOccurrence->Name &&
            strcmp(theFunc->Name, theOccurrence->Name) == 0)
          {
          occCounter++;

          if (theOccurrence->IsLegacy)
            {
            fprintf(fp,
                    "#if !defined(VTK_LEGACY_REMOVE)\n");
            }

          /* method suffix to distinguish between signatures */
          occSuffix[0] = '\0';
          if (numberOfOccurrences > 1)
            {
            sprintf(occSuffix, "_s%d", occCounter);
            }

          /* declare the method */
          fprintf(fp,
                  "static PyObject *\n"
                  "Py%s_%s%s(PyObject *%s, PyObject *args)\n"
                  "{\n",
                  data->Name, theOccurrence->Name, occSuffix,
                  ((theOccurrence->IsStatic | do_constructors) ? "" : "self"));

          /* Use vtkPythonArgs to convert python args to C args */
          if (is_vtkobject && !theOccurrence->IsStatic)
            {
            fprintf(fp,
                    "  vtkPythonArgs ap(self, args, \"%s\");\n"
                    "  vtkObjectBase *vp = ap.GetSelfPointer(self, args);\n"
                    "  %s *op = static_cast<%s *>(vp);\n"
                    "\n",
                    theOccurrence->Name, data->Name, data->Name);
            }
          else if (!theOccurrence->IsStatic && !do_constructors)
            {
            fprintf(fp,
                    "  vtkPythonArgs ap(args, \"%s\");\n"
                    "  void *vp = ap.GetSelfPointer(self);\n"
                    "  %s *op = static_cast<%s *>(vp);\n"
                    "\n",
                    theOccurrence->Name, data->Name, data->Name);
            }
          else
            {
            fprintf(fp,
                    "  vtkPythonArgs ap(args, \"%s\");\n"
                    "\n",
                    theOccurrence->Name);
            }

          /* declare all argument variables */
          vtkWrapPython_DeclareVariables(fp, theOccurrence);

          /* get size for variable-size arrays */
          vtkWrapPython_GetSizesForArrays(fp, theOccurrence, is_vtkobject);

          /* open the "if" for getting all the args */
          fprintf(fp,
                  "  if (");

          /* special things for vtkObject methods */
          if (is_vtkobject && !theOccurrence->IsStatic)
            {
            fprintf(fp, "op && ");
            if (theOccurrence->IsPureVirtual)
              {
              fprintf(fp, "!ap.IsPureVirtual() && ");
              }
            }

          /* get all the arguments */
          vtkWrapPython_GetAllArguments(fp, theOccurrence);

          /* finished getting all the arguments */
          fprintf(fp, ")\n"
                  "    {\n");

          /* save a copy of all non-const array arguments */
          vtkWrapPython_SaveArrayArgs(fp, theOccurrence);

          /* generate the code that calls the C++ method */
          vtkWrapPython_GenerateMethodCall(fp, theOccurrence, data,
                                           is_vtkobject);

          /* write back to all array args */
          vtkWrapPython_WriteBackToArgs(fp, theOccurrence);

          /* generate the code that builds the return value */
          if (do_constructors && !is_vtkobject)
            {
            fprintf(fp,
                    "    result = PyVTKSpecialObject_New(\"%s\", op);\n",
                    data->Name);
            }
          else
            {
            vtkWrapPython_ReturnValue(fp, theOccurrence->ReturnValue);
            }

          /* close off the big "if" */
          fprintf(fp,
                  "    }\n"
                  "\n");

          /* arrays might have been allocated */
          vtkWrapPython_FreeAllocatedArrays(fp, theOccurrence);

          /* conversion constructors might have been used */
          vtkWrapPython_FreeConstructedObjects(fp, theOccurrence);

          /* it's all over... return the result */
          fprintf(fp,
                  "  return result;\n"
                  "}\n");

          if (theOccurrence->IsLegacy)
            {
            fprintf(fp,
                    "#endif\n");
            }

          fprintf(fp,
                  "\n");
          }
        }

      /* check for overloads */
      overloadMap = vtkWrapPython_ArgCountToOverloadMap(
        wrappedFunctions, numberOfWrappedFunctions,
        fnum, is_vtkobject, &maxArgs, &overlap);

      if (overlap || do_constructors)
        {
        /* output the method table for the signatures */
        vtkWrapPython_OverloadMethodDef(
          fp, data, overloadMap, wrappedFunctions, numberOfWrappedFunctions,
          fnum, numberOfOccurrences, is_vtkobject, all_legacy);
        }

      if (numberOfOccurrences > 1)
        {
        /* declare a "master method" to choose among the overloads */
        vtkWrapPython_OverloadMasterMethod(
          fp, data, overloadMap, maxArgs,
          wrappedFunctions, numberOfWrappedFunctions,
          fnum, numberOfOccurrences, is_vtkobject, all_legacy);
        }

      /* set the legacy flag */
      theFunc->IsLegacy = all_legacy;

      /* clear all occurances of this method from further consideration */
      for (occ = fnum + 1; occ < numberOfWrappedFunctions; occ++)
        {
        theOccurrence = wrappedFunctions[occ];

        /* is it the same name */
        if (theOccurrence->Name &&
            !strcmp(theFunc->Name,theOccurrence->Name))
          {
          size_t siglen = strlen(theFunc->Signature);

          /* memory leak here but ... */
          theOccurrence->Name = NULL;
          cp = (char *)malloc(siglen+2+ strlen(theOccurrence->Signature));
          strcpy(cp, theFunc->Signature);
          strcpy(&cp[siglen],"\n");
          strcpy(&cp[siglen+1], theOccurrence->Signature);
          theFunc->Signature = cp;
          }
        }
      } /* is this method non NULL */
    } /* loop over all methods */

  /* the method table for constructors is produced elsewhere */
  if (!do_constructors)
    {
    vtkWrapPython_ClassMethodDef(fp, data, wrappedFunctions,
                                 numberOfWrappedFunctions, fnum);
    }

  free(wrappedFunctions);
}

/* -------------------------------------------------------------------- */
/* output the MethodDef table for this class */
static void vtkWrapPython_ClassMethodDef(
  FILE *fp, ClassInfo *data,
  FunctionInfo **wrappedFunctions, int numberOfWrappedFunctions, int fnum)
{
  /* output the method table, with pointers to each function defined above */
  fprintf(fp,
          "static PyMethodDef Py%s_Methods[] = {\n",
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
      signatures = vtkWrapText_FormatSignature(
        wrappedFunctions[fnum]->Signature, 66, maxlen - 32);
      comment = vtkWrapText_FormatComment(
        wrappedFunctions[fnum]->Comment, 66);
      comment = vtkWrapText_QuoteString(
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
          "  {NULL, NULL, 0, NULL}\n"
          "};\n"
          "\n");
}

/* -------------------------------------------------------------------- */
/* Check an arg to see if it is wrappable */

static int vtkWrapPython_IsValueWrappable(
  ValueInfo *val, HierarchyInfo *hinfo, int flags)
{
  static unsigned int wrappableTypes[] = {
    VTK_PARSE_VOID, VTK_PARSE_BOOL, VTK_PARSE_FLOAT, VTK_PARSE_DOUBLE,
    VTK_PARSE_CHAR, VTK_PARSE_UNSIGNED_CHAR, VTK_PARSE_SIGNED_CHAR,
    VTK_PARSE_INT, VTK_PARSE_UNSIGNED_INT,
    VTK_PARSE_SHORT, VTK_PARSE_UNSIGNED_SHORT,
    VTK_PARSE_LONG, VTK_PARSE_UNSIGNED_LONG,
    VTK_PARSE_ID_TYPE, VTK_PARSE_UNSIGNED_ID_TYPE,
    VTK_PARSE_SSIZE_T, VTK_PARSE_SIZE_T,
#ifdef VTK_TYPE_USE_LONG_LONG
    VTK_PARSE_LONG_LONG, VTK_PARSE_UNSIGNED_LONG_LONG,
#endif
#ifdef VTK_TYPE_USE___INT64
    VTK_PARSE___INT64, VTK_PARSE_UNSIGNED___INT64,
#endif
    VTK_PARSE_OBJECT, VTK_PARSE_QOBJECT, VTK_PARSE_STRING,
#ifdef Py_USING_UNICODE
    VTK_PARSE_UNICODE_STRING,
#endif
    0
  };

  const char *aClass;
  unsigned int baseType;
  int j;

  if ((flags & VTK_WRAP_RETURN) != 0)
    {
    if (vtkWrap_IsVoid(val))
      {
      return 1;
      }

    if (vtkWrap_IsNArray(val))
      {
      return 0;
      }
    }

  aClass = val->Class;
  baseType = (val->Type & VTK_PARSE_BASE_TYPE);

  /* go through all types that are indicated as wrappable */
  for (j = 0; wrappableTypes[j] != 0; j++)
    {
    if (baseType == wrappableTypes[j]) { break; }
    }
  if (wrappableTypes[j] == 0)
    {
    return 0;
    }

  if (vtkWrap_IsRef(val) && !vtkWrap_IsScalar(val))
    {
    return 0;
    }

  if (vtkWrap_IsScalar(val))
    {
    if (vtkWrap_IsNumeric(val) ||
        vtkWrap_IsString(val))
      {
      return 1;
      }
    if (vtkWrap_IsObject(val))
      {
      if (vtkWrap_IsSpecialType(hinfo, aClass) ||
          vtkWrap_IsQtObject(val) ||
          vtkWrap_IsQtEnum(val))
        {
        return 1;
        }
      }
    }
  else if (vtkWrap_IsArray(val) || vtkWrap_IsNArray(val))
    {
    if (vtkWrap_IsNumeric(val))
      {
      return 1;
      }
    }
  else if (vtkWrap_IsPointer(val))
    {
    if (vtkWrap_IsCharPointer(val) ||
        vtkWrap_IsVoidPointer(val))
      {
      return 1;
      }
    if (vtkWrap_IsObject(val))
      {
      if (vtkWrap_IsVTKObjectBaseType(hinfo, aClass) ||
          vtkWrap_IsQtObject(val))
        {
        return 1;
        }
      }
    }

  return 0;
}

/* -------------------------------------------------------------------- */
/* Check a method to see if it is wrappable in python */

static int vtkWrapPython_MethodCheck(
  FunctionInfo *currentFunction, HierarchyInfo *hinfo)
{
  int i, n;

  /* some functions will not get wrapped no matter what */
  if (currentFunction->IsOperator ||
      currentFunction->Access != VTK_ACCESS_PUBLIC)
    {
    return 0;
    }

  /* new and delete are meaningless in wrapped languages */
  if (currentFunction->Name == 0 ||
      strcmp("Delete", currentFunction->Name) == 0 ||
      strcmp("New", currentFunction->Name) == 0)
    {
    return 0;
    }

  /* function pointer arguments for callbacks */
  if (currentFunction->NumberOfArguments == 2 &&
      vtkWrap_IsVoidFunction(currentFunction->Arguments[0]) &&
      vtkWrap_IsVoidPointer(currentFunction->Arguments[1]) &&
      !vtkWrap_IsConst(currentFunction->Arguments[1]) &&
      vtkWrap_IsVoid(currentFunction->ReturnValue))
    {
    return 1;
    }

  n = vtkWrap_CountWrappedArgs(currentFunction);

  /* check to see if we can handle all the args */
  for (i = 0; i < n; i++)
    {
    if (!vtkWrapPython_IsValueWrappable(
          currentFunction->Arguments[i], hinfo, VTK_WRAP_ARG))
      {
      return 0;
      }
    }

  /* check the return value */
  if (!vtkWrapPython_IsValueWrappable(
        currentFunction->ReturnValue, hinfo, VTK_WRAP_RETURN))
    {
    return 0;
    }

  return 1;
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
            vtkWrapText_QuoteString(
              vtkWrapText_FormatComment(file_info->NameComment, 70), 500));
    }
  else
    {
    fprintf(fp,
            "    \"%s - no description provided.\\n\\n\",\n",
            vtkWrapText_QuoteString(data->Name, 500));
    }

  /* only consider superclasses that are wrapped */
  for (j = 0; j < data->NumberOfSuperClasses; j++)
    {
    if (vtkWrap_IsClassWrapped(hinfo, data->SuperClasses[j]))
      {
      break;
      }
    }

  if (j < data->NumberOfSuperClasses)
    {
    fprintf(fp,
            "    \"Superclass: %s\\n\\n\",\n",
            vtkWrapText_QuoteString(data->SuperClasses[j], 500));
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

  ccp = vtkWrapText_FormatComment(comment, 70);
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
              vtkWrapText_QuoteString(temp, 500));
      }
    else
      { /* just for the last time */
      fprintf(fp,
              "    \"%s\\n\",\n",
              vtkWrapText_QuoteString(temp, 500));
      }
    }

  /* for special objects, add constructor signatures to the doc */
  if (!is_vtkobject)
    {
    for (j = 0; j < data->NumberOfFunctions; j++)
      {
      if (vtkWrapPython_MethodCheck(data->Functions[j], hinfo) &&
          vtkWrap_IsConstructor(data, data->Functions[j]))
        {
        fprintf(fp,"    \"%s\\n\",\n",
                vtkWrapText_FormatSignature(
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
    if (vtkWrapPython_MethodCheck(currentFunction, hinfo))
      {
      classname = "void";
      aType = VTK_PARSE_VOID;
      if (currentFunction->ReturnValue)
        {
        classname = currentFunction->ReturnValue->Class;
        aType = currentFunction->ReturnValue->Type;
        }

      m = vtkWrap_CountWrappedArgs(currentFunction);

      for (j = -1; j < m; j++)
        {
        if (j >= 0)
          {
          classname = currentFunction->Arguments[j]->Class;
          aType = currentFunction->Arguments[j]->Type;
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

  /* get our own include file (returns NULL if hinfo is NULL) */
  ownincfile = vtkWrapPython_ClassHeader(hinfo, data->Name);

  /* for each unique type found in the file */
  for (i = 0; i < numTypes; i++)
    {
    const char *incfile;
    incfile = vtkWrapPython_ClassHeader(hinfo, types[i]);

    if (incfile)
      {
      /* make sure it doesn't share our header file */
      if (ownincfile == 0 || strcmp(incfile, ownincfile) != 0)
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
           (theFunc->Arguments[0]->Type != VTK_PARSE_UNSIGNED_LONG)) ||
          (((strcmp(theFunc->Name, "RemoveObservers") == 0) ||
            (strcmp(theFunc->Name, "HasObserver") == 0)) &&
           (((theFunc->Arguments[0]->Type != VTK_PARSE_UNSIGNED_LONG) &&
             (theFunc->Arguments[0]->Type !=
              (VTK_PARSE_CHAR_PTR|VTK_PARSE_CONST))) ||
            (theFunc->NumberOfArguments > 1))) ||
          ((strcmp(theFunc->Name, "RemoveAllObservers") == 0) &&
           (theFunc->NumberOfArguments > 0)))
        {
        data->Functions[i]->Name = NULL;
        }
      }

    /* Add the AddObserver method to vtkObject. */
    fprintf(fp,
            "static PyObject *\n"
            "PyvtkObject_AddObserver(PyObject *self, PyObject *args)\n"
            "{\n"
            "  vtkPythonArgs ap(self, args, \"AddObserver\");\n"
            "  vtkObjectBase *vp = ap.GetSelfPointer(self, args);\n"
            "  vtkObject *op = static_cast<vtkObject *>(vp);\n"
            "\n"
            "  char *temp0s = NULL;\n"
            "  int temp0i = 0;\n"
            "  PyObject *temp1 = NULL;\n"
            "  float temp2 = 0.0f;\n"
            "  unsigned long tempr;\n"
            "  PyObject *result = NULL;\n"
            "  int argtype = 0;\n"
            "\n");

    fprintf(fp,
            "  if (op)\n"
            "    {\n"
            "    if (ap.CheckArgCount(2,3) &&\n"
            "        ap.GetValue(temp0i) &&\n"
            "        ap.GetFunction(temp1) &&\n"
            "        (ap.NoArgsLeft() || ap.GetValue(temp2)))\n"
            "      {\n"
            "      argtype = 1;\n"
            "      }\n"
            "    }\n"
            "\n"
            "  if (op && !argtype)\n"
            "    {\n"
            "    PyErr_Clear();\n"
            "    ap.Reset();\n"
            "\n"
            "    if (ap.CheckArgCount(2,3) &&\n"
            "        ap.GetValue(temp0s) &&\n"
            "        ap.GetFunction(temp1) &&\n"
            "        (ap.NoArgsLeft() || ap.GetValue(temp2)))\n"
            "      {\n"
            "      argtype = 2;\n"
            "      }\n"
            "    }\n"
            "\n");

    fprintf(fp,
            "  if (argtype)\n"
            "    {\n"
            "    vtkPythonCommand *cbc = vtkPythonCommand::New();\n"
            "    Py_INCREF(temp1);\n"
            "    cbc->SetObject(temp1);\n"
            "    cbc->SetThreadState(PyThreadState_Get());\n"
            "\n"
            "    if (argtype == 1)\n"
            "      {\n"
            "      if (ap.IsBound())\n"
            "        {\n"
            "        tempr = op->AddObserver(temp0i, cbc, temp2);\n"
            "        }\n"
            "      else\n"
            "        {\n"
            "        tempr = op->vtkObject::AddObserver(temp0i, cbc, temp2);\n"
            "        }\n"
            "      }\n"
            "    else\n"
            "      {\n"
            "      if (ap.IsBound())\n"
            "        {\n"
            "        tempr = op->AddObserver(temp0s, cbc, temp2);\n"
            "        }\n"
            "      else\n"
            "        {\n"
            "        tempr = op->vtkObject::AddObserver(temp0s, cbc, temp2);\n"
            "        }\n"
            "      }\n"
            "\n");

    fprintf(fp,
            "    cbc->Delete();\n"
            "\n"
            "    if (!ap.ErrorOccurred())\n"
            "      {\n"
            "      result = ap.BuildValue(tempr);\n"
            "      }\n"
            "    }\n"
            "\n"
            "  return result;\n"
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
            "static PyObject *\n"
            "PyvtkObjectBase_GetAddressAsString(PyObject *self, PyObject *args)\n"
            "{\n"
            "  vtkPythonArgs ap(self, args, \"GetAddressAsString\");\n"
            "  vtkObjectBase *vp = ap.GetSelfPointer(self, args);\n"
            "  %s *op = static_cast<%s *>(vp);\n"
            "\n"
            "  char *temp0;\n"
            "  char tempr[256];\n"
            "  PyObject *result = NULL;\n"
            "\n"
            "  if (op && ap.CheckArgCount(1) &&\n"
            "      ap.GetValue(temp0))\n"
            "    {\n"
            "    sprintf(tempr, \"Addr=%%p\", op);\n"
            "\n"
            "    result = ap.BuildValue(tempr);\n"
            "    }\n"
            "\n"
            "  return result;\n"
            "}\n"
            "\n",
            data->Name, data->Name);

    /* add the PrintRevisions method to vtkObjectBase. */
    fprintf(fp,
            "static PyObject *\n"
            "PyvtkObjectBase_PrintRevisions(PyObject *self, PyObject *args)\n"
            "{\n"
            "  vtkPythonArgs ap(self, args, \"PrintRevisions\");\n"
            "  vtkObjectBase *vp = ap.GetSelfPointer(self, args);\n"
            "  %s *op = static_cast<%s *>(vp);\n"
            "\n"
            "  const char *tempr;\n"
            "  PyObject *result = NULL;\n"
            "\n"
            "  if (op && ap.CheckArgCount(0))\n"
            "    {\n"
            "    vtksys_ios::ostringstream vtkmsg_with_warning_C4701;\n"
            "    op->PrintRevisions(vtkmsg_with_warning_C4701);\n"
            "    vtkmsg_with_warning_C4701.put('\\0');\n"
            "    tempr = vtkmsg_with_warning_C4701.str().c_str();\n"
            "\n"
            "    result = ap.BuildValue(tempr);\n"
            "    }\n"
            "\n"
            "  return result;\n"
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
            "static vtkObjectBase *%s_StaticNew()\n"
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
            "  return PyVTKClass_New(&%s_StaticNew,\n",
            data->Name);
    }
  else
    {
    fprintf(fp,
            "  return PyVTKClass_New(NULL,\n");
    }

  fprintf(fp,
          "                        Py%s_Methods,\n"
          "                        \"%s\",modulename,\n"
          "                        Py%s_Doc(),",
          data->Name, data->Name, data->Name);

  /* find the first superclass that is a VTK class */
  for (i = 0; i < data->NumberOfSuperClasses; i++)
    {
    if (vtkWrap_IsVTKObjectBaseType(hinfo, data->SuperClasses[i]))
      {
      break;
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
    "static PyObject *\n"
    "Py%s_New(PyTypeObject *, PyObject *args, PyObject *kwds)\n"
    "{\n"
    "  if (kwds && PyDict_Size(kwds))\n"
    "    {\n"
    "    PyErr_SetString(PyExc_TypeError,\n"
    "                    \"this function takes no keyword arguments\");\n"
    "    return NULL;\n"
    "    }\n"
    "\n"
    "  return Py%s_%s(NULL, args);\n"
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
          (func->Arguments[0]->Type & VTK_PARSE_UNQUALIFIED_TYPE) ==
              VTK_PARSE_OSTREAM_REF &&
          (func->Arguments[1]->Type & VTK_PARSE_BASE_TYPE) ==
              VTK_PARSE_OBJECT &&
          (func->Arguments[1]->Type & VTK_PARSE_POINTER_MASK) == 0 &&
          strcmp(func->Arguments[1]->Class, data->Name) == 0)
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
      "  if (result == -1)\n"
      "    {\n"
      "    PyErr_SetString(PyExc_TypeError, (char *)\"operation not available\");\n"
      "    return NULL;\n"
      "    }\n"
      "\n"
      "#if PY_VERSION_HEX >= 0x02030000\n"
      "  // avoids aliasing issues with Py_INCREF(Py_False)\n"
      "  return PyBool_FromLong((long)result);\n"
      "#else\n"
      "  if (result == 0)\n"
      "    {\n"
      "    Py_INCREF(Py_False);\n"
      "    return Py_False;\n"
      "    }\n"
      "  Py_INCREF(Py_True);\n"
      "  return Py_True;\n"
      "#endif\n"
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
    "static PyMethodDef Py%s_NewMethod = \\\n"
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
    "  Py%s_Methods, // tp_methods\n"
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
    "#if PY_VERSION_HEX >= 0x02030000\n"
    "  PyObject_Del, // tp_free\n"
    "#else\n"
    "  _PyObject_Del, // tp_free\n"
    "#endif\n"
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
    "    Py%s_Methods,\n"
    "    Py%s_%s_Methods,\n"
    "    &Py%s_NewMethod,\n"
    "    Py%s_Doc(), &%s_Copy);\n"
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

    case VTK_PARSE_UNSIGNED_INT:
      fprintf(fp,
              "#if VTK_SIZEOF_INT < VTK_SIZEOF_LONG\n"
              "%s%s = PyInt_FromLong(%s);\n"
              "#else\n"
              "%s%s = PyLong_FromUnsignedLong(%s);\n"
              "#endif\n",
              indent, objvar, valstring, indent, objvar, valstring);
      objcreated = 1;
      break;

    case VTK_PARSE_UNSIGNED_LONG:
      fprintf(fp,
              "%s%s = PyLong_FromUnsignedLong(%s);\n",
              indent, objvar, valstring);
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
            (name[j-1] >= '0' && name[j-1] <= '9') ||
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

  /* use the hierarchy file to expand typedefs */
  if (data && hinfo)
    {
    vtkWrap_ExpandTypedefs(data, hinfo);
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

  /* lots of important utility functions are defined in vtkPythonArgs.h */
  fprintf(fp,
          "#include \"vtkPythonArgs.h\"\n"
          "#include \"vtkPythonOverload.h\"\n"
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
      if (vtkWrap_IsClassWrapped(hinfo, data->SuperClasses[i]) &&
          vtkWrap_IsVTKObjectBaseType(hinfo, data->SuperClasses[i]))
        {
        fprintf(fp,
          "extern \"C\" { PyObject *PyVTKClass_%sNew(const char *); }\n",
          data->SuperClasses[i]);
        }
      }
    }

  /* prototype for the docstring function */
  if (data)
    {
    fprintf(fp,
            "\n"
            "static const char **Py%s_Doc();\n"
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
          "  PyObject *dict, const char *%s)\n"
          "{\n"
          "  PyObject *o;\n",
          name, (data ? "modulename" : ""));

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
            "const char **Py%s_Doc()\n"
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
