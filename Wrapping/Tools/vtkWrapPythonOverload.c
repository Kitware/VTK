/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWrapPythonOverload.c

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkWrapPythonOverload.h"
#include "vtkWrapPythonMethod.h"
#include "vtkWrapPythonTemplate.h"

#include "vtkWrap.h"

/* required for VTK_USE_64BIT_IDS */
#include "vtkConfigure.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* -------------------------------------------------------------------- */
/* prototypes for utility methods */

/* Get the python format char for the give type */
static char vtkWrapPython_FormatChar(
  unsigned int argtype);

/* create a format string for PyArg_ParseTuple */
static char *vtkWrapPython_FormatString(
  FunctionInfo *currentFunction);

/* create a string for checking arguments against available signatures */
static char *vtkWrapPython_ArgCheckString(
  int isvtkobjmethod, FunctionInfo *currentFunction);


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
#ifndef VTK_PYTHON_LEGACY_FORMAT
      typeChar = 'I';
      break;
#endif
    case VTK_PARSE_INT:
      typeChar = 'i';
      break;
    case VTK_PARSE_UNSIGNED_SHORT:
#ifndef VTK_PYTHON_LEGACY_FORMAT
      typeChar = 'H';
      break;
#endif
    case VTK_PARSE_SHORT:
      typeChar = 'h';
      break;
    case VTK_PARSE_UNSIGNED_LONG:
#ifndef VTK_PYTHON_LEGACY_FORMAT
      typeChar = 'k';
      break;
#endif
    case VTK_PARSE_LONG:
      typeChar = 'l';
      break;
    case VTK_PARSE_UNSIGNED_ID_TYPE:
#ifndef VTK_PYTHON_LEGACY_FORMAT
#ifdef VTK_USE_64BIT_IDS
#ifndef VTK_PYTHON_NO_LONG_LONG
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
#ifndef VTK_PYTHON_NO_LONG_LONG
      typeChar = 'L';
#else
      typeChar = 'l';
#endif
#else
      typeChar = 'i';
#endif
      break;
#ifndef VTK_PYTHON_NO_LONG_LONG
    case VTK_PARSE_SIZE_T:
    case VTK_PARSE_UNSIGNED_LONG_LONG:
    case VTK_PARSE_UNSIGNED___INT64:
#ifndef VTK_PYTHON_LEGACY_FORMAT
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
#ifndef VTK_PYTHON_LEGACY_FORMAT
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
#ifndef VTK_PYTHON_LEGACY_FORMAT
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

  totalArgs = vtkWrap_CountWrappedParameters(currentFunction);
  requiredArgs = vtkWrap_CountRequiredArguments(currentFunction);

  for (i = 0; i < totalArgs; i++)
    {
    arg = currentFunction->Parameters[i];
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
  char pythonname[1024];
  size_t currPos = 0;
  ValueInfo *arg;
  unsigned int argtype;
  int i, j, k;
  int totalArgs;

  totalArgs = vtkWrap_CountWrappedParameters(currentFunction);

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
    arg = currentFunction->Parameters[i];
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
        argtype == VTK_PARSE_UNKNOWN ||
        argtype == VTK_PARSE_UNKNOWN_REF ||
        argtype == VTK_PARSE_UNKNOWN_PTR ||
        argtype == VTK_PARSE_QOBJECT ||
        argtype == VTK_PARSE_QOBJECT_REF ||
        argtype == VTK_PARSE_QOBJECT_PTR)
      {
      vtkWrapPython_PythonicName(arg->Class, pythonname);

      result[currPos++] = ' ';
      if ((argtype == VTK_PARSE_OBJECT_REF ||
           argtype == VTK_PARSE_UNKNOWN_REF) &&
          (arg->Type & VTK_PARSE_CONST) == 0)
        {
        result[currPos++] = '&';
        }
      else if (argtype == VTK_PARSE_QOBJECT_REF)
        {
        result[currPos++] = '&';
        }
      else if (argtype == VTK_PARSE_OBJECT_PTR ||
               argtype == VTK_PARSE_UNKNOWN_PTR ||
               argtype == VTK_PARSE_QOBJECT_PTR)
        {
        result[currPos++] = '*';
        }
      strcpy(&result[currPos], pythonname);
      currPos += strlen(pythonname);
      }

    else if (vtkWrap_IsArray(arg) || vtkWrap_IsNArray(arg) ||
             vtkWrap_IsPODPointer(arg))
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
/* Generate an int array that maps arg counts to overloads.
 * Each element in the array will either contain the index of the
 * overload that it maps to, or -1 if it maps to multiple overloads,
 * or zero if it does not map to any.  The length of the array is
 * returned in "nmax". The value of "overlap" is set to 1 if there
 * are some arg counts that map to more than one method. */

int *vtkWrapPython_ArgCountToOverloadMap(
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

    totalArgs = vtkWrap_CountWrappedParameters(theOccurrence);
    requiredArgs = vtkWrap_CountRequiredArguments(theOccurrence);

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
/* output the method table for all overloads of a particular method,
 * this is also used to write out all constructors for the class */

void vtkWrapPython_OverloadMethodDef(
  FILE *fp, const char *classname, ClassInfo *data, int *overloadMap,
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
          classname, theFunc->Name);

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

    totalArgs = vtkWrap_CountWrappedParameters(theOccurrence);
    requiredArgs = vtkWrap_CountRequiredArguments(theOccurrence);

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
            "  {NULL, Py%s_%s%s, METH_VARARGS%s,\n"
            "   (char*)\"%s\"},\n",
            classname, wrappedFunctions[occ]->Name,
            occSuffix,
            theOccurrence->IsStatic ? " | METH_STATIC" : "",
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

void vtkWrapPython_OverloadMasterMethod(
  FILE *fp, const char *classname, int *overloadMap, int maxArgs,
  FunctionInfo **wrappedFunctions, int numberOfWrappedFunctions, int fnum,
  int is_vtkobject, int all_legacy)
{
  FunctionInfo *currentFunction;
  FunctionInfo *theOccurrence;
  int overlap = 0;
  int occ, occCounter;
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
           classname, currentFunction->Name);

  if (overlap)
    {
    fprintf(fp,
          "  PyMethodDef *methods = Py%s_%s_Methods;\n",
           classname, currentFunction->Name);
    }

  fprintf(fp,
          "  int nargs = vtkPythonArgs::GetArgCount(%sargs);\n"
          "\n",
          ((is_vtkobject && !any_static) ? "self, " : ""));

  fprintf(fp,
          "  switch(nargs)\n"
          "    {\n");

  /* find all occurrences of this method */
  occCounter = 0;
  for (occ = fnum; occ < numberOfWrappedFunctions; occ++)
    {
    theOccurrence = wrappedFunctions[occ];

    /* is it the same name */
    if (theOccurrence->Name &&
        strcmp(currentFunction->Name, theOccurrence->Name) == 0)
      {
      occCounter++;

      foundOne = 0;
      for (i = 0; i <= maxArgs; i++)
        {
        if (overloadMap[i] == occCounter)
          {
          if (!foundOne && theOccurrence->IsLegacy && !all_legacy)
            {
            fprintf(fp,
                 "#if !defined(VTK_LEGACY_REMOVE)\n");
            }
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
                classname, currentFunction->Name, occCounter);
        if (theOccurrence->IsLegacy && !all_legacy)
          {
          fprintf(fp,
                "#endif\n");
          }
        }
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
