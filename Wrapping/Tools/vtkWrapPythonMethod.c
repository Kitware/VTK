/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWrapPythonMethod.c

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkWrapPythonMethod.h"
#include "vtkWrapPythonOverload.h"
#include "vtkWrapPythonTemplate.h"

#include "vtkWrap.h"
#include "vtkWrapText.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* -------------------------------------------------------------------- */
/* prototypes for the methods used by the python wrappers */

/* Write the code to convert the arguments with vtkPythonArgs */
static void vtkWrapPython_GetAllParameters(
  FILE* fp, ClassInfo* data, FunctionInfo* currentFunction);

/* Write code for execution after the method parameters are evaluated */
static void vtkWrapPython_SubstituteCode(
  FILE* fp, ClassInfo* data, FunctionInfo* func, const char* code);

/* Check any "expects" preconditions prior to calling the function */
static void vtkWrapPython_CheckPreconds(FILE* fp, ClassInfo* data, FunctionInfo* currentFunction);

/* save the contents of all arrays prior to calling the function */
static void vtkWrapPython_SaveArgs(FILE* fp, FunctionInfo* currentFunction);

/* generate the code that calls the C++ method */
static void vtkWrapPython_GenerateMethodCall(
  FILE* fp, FunctionInfo* currentFunction, ClassInfo* data, HierarchyInfo* hinfo, int is_vtkobject);

/* Write back to all the reference arguments and array arguments */
static void vtkWrapPython_WriteBackToArgs(FILE* fp, ClassInfo* data, FunctionInfo* currentFunction);

/* Free any arrays, object, or buffers that were allocated */
static void vtkWrapPython_FreeTemporaries(FILE* fp, FunctionInfo* currentFunction);

/* -------------------------------------------------------------------- */
/* prototypes for utility methods */

/* look for all signatures of the specified method */
static int vtkWrapPython_CountAllOccurrences(
  FunctionInfo** wrappedFunctions, int n, int fnum, int* all_static, int* all_legacy);

/* -------------------------------------------------------------------- */
/* Declare all local variables used by the wrapper method */
void vtkWrapPython_DeclareVariables(FILE* fp, ClassInfo* data, FunctionInfo* theFunc)
{
  ValueInfo* arg;
  int i, n;

  n = vtkWrap_CountWrappedParameters(theFunc);

  /* temp variables for arg values */
  for (i = 0; i < n; i++)
  {
    arg = theFunc->Parameters[i];

    /* a callable python object for function args */
    if (vtkWrap_IsFunction(arg))
    {
      fprintf(fp, "  PyObject *temp%d = nullptr;\n", i);
      /* ignore further arguments */
      break;
    }

    /* a PyObject argument will simply be passed through */
    if (vtkWrap_IsPythonObject(arg))
    {
      fprintf(fp, "  PyObject *temp%d;\n", i);
      continue;
    }

    /* temps for arrays */
    if (vtkWrap_IsArray(arg) || vtkWrap_IsNArray(arg) || vtkWrap_IsPODPointer(arg) ||
      (vtkWrap_IsCharPointer(arg) && !vtkWrap_IsConst(arg)))
    {
      /* for non-const arrays, alloc twice as much space */
      const char* mtwo = "";
      if (!vtkWrap_IsConst(arg) && !vtkWrap_IsSetVectorMethod(theFunc))
      {
        mtwo = "2*";
      }
      if (vtkWrap_IsCharPointer(arg))
      {
        /* prepare for "char *" arg for non-const char pointer */
        fprintf(fp,
          "  size_t size%d = ap.GetStringSize(%d);\n"
          "  vtkPythonArgs::Array<char> store%d(%ssize%d + 1);\n"
          "  char *temp%d = store%d.Data();\n",
          i, i, i, mtwo, i, i, i);
        if (!vtkWrap_IsRef(arg))
        {
          fprintf(fp, "  char *save%d = temp%d + size%d + 1;\n", i, i, i);
        }
      }
      else if (arg->CountHint || vtkWrap_IsPODPointer(arg))
      {
        /* prepare for "T *" arg, where T is a plain type */
        fprintf(fp,
          "  size_t size%d = ap.GetArgSize(%d);\n"
          "  vtkPythonArgs::Array<%s> store%d(%ssize%d);\n"
          "  %s *temp%d = store%d.Data();\n",
          i, i, vtkWrap_GetTypeName(arg), i, mtwo, i, vtkWrap_GetTypeName(arg), i, i);
        if (!vtkWrap_IsConst(arg) && !vtkWrap_IsRef(arg))
        {
          fprintf(fp, "  %s *save%d = (size%d == 0 ? nullptr : temp%d + size%d);\n",
            vtkWrap_GetTypeName(arg), i, i, i, i);
        }
        else if (vtkWrap_IsConst(arg) && vtkWrap_IsRef(arg))
        {
          fprintf(fp, "  const %s *temp%dc = temp%d;\n", vtkWrap_GetTypeName(arg), i, i);
        }
      }
      else if (vtkWrap_IsArray(arg) && arg->Value)
      {
        /* prepare for "T a[n] = nullptr" arg (array with default of NULL) */
        fprintf(fp,
          "  size_t size%d = 0;\n"
          "  %s store%d[%s%d];\n"
          "  %s *temp%d = nullptr;\n",
          i, vtkWrap_GetTypeName(arg), i, mtwo, arg->Count, vtkWrap_GetTypeName(arg), i);
        if (!vtkWrap_IsConst(arg))
        {
          fprintf(fp, "  %s *save%d = nullptr;\n", vtkWrap_GetTypeName(arg), i);
        }
        fprintf(fp,
          "  if (ap.GetArgSize(%d) > 0)\n"
          "  {\n"
          "    size%d = %d;\n"
          "    temp%d = store%d;\n",
          i, i, arg->Count, i, i);
        if (!vtkWrap_IsConst(arg))
        {
          fprintf(fp, "    save%d = store%d + %d;\n", i, i, arg->Count);
        }
        fprintf(fp, "  }\n");
      }
      else
      {
        /* prepare for "T a[n]" or "T a[n][m]" array arg */
        vtkWrap_DeclareVariableSize(fp, arg, "size", i);
        vtkWrap_DeclareVariable(fp, data, arg, "temp", i, VTK_WRAP_ARG);

        if (!vtkWrap_IsConst(arg) && !vtkWrap_IsSetVectorMethod(theFunc))
        {
          /* for saving a copy of the array */
          vtkWrap_DeclareVariable(fp, data, arg, "save", i, VTK_WRAP_ARG);
        }
      }
    }
    else if (vtkWrap_IsStdVector(arg))
    {
      fprintf(fp, "  %s temp%d(ap.GetArgSize(%d));\n", arg->Class, i, i);
    }
    else
    {
      /* make a "temp" variable for any other kind of argument */
      vtkWrap_DeclareVariable(fp, data, arg, "temp", i, VTK_WRAP_ARG);
    }

    /* temps for buffer objects */
    if (vtkWrap_IsVoidPointer(arg) || vtkWrap_IsZeroCopyPointer(arg))
    {
      fprintf(fp, "  Py_buffer pbuf%d = VTK_PYBUFFER_INITIALIZER;\n", i);
    }

    /* temps for conversion constructed objects, which only occur
     * for special objects */
    if (vtkWrap_IsSpecialObject(arg) && !vtkWrap_IsNonConstRef(arg))
    {
      fprintf(fp, "  PyObject *pobj%d = nullptr;\n", i);
    }
  }

  if (theFunc->ReturnValue)
  {
    /* the size for a one-dimensional array */
    if (vtkWrap_IsArray(theFunc->ReturnValue) && !theFunc->ReturnValue->CountHint)
    {
      fprintf(fp, "  size_t sizer = %d;\n", theFunc->ReturnValue->Count);
    }
  }

  /* temp variable for the Python return value */
  fprintf(fp,
    "  PyObject *result = nullptr;\n"
    "\n");
}

/* -------------------------------------------------------------------- */
/* Write the code to convert one argument with vtkPythonArgs */
void vtkWrapPython_GetSingleArgument(
  FILE* fp, ClassInfo* data, int i, ValueInfo* arg, int static_call)
{
  const char* prefix = "ap.";
  const char* cp;
  char argname[32];
  char pythonname[1024];
  size_t l;
  argname[0] = '\0';

  if (static_call)
  {
    prefix = "vtkPythonArgs::";
    sprintf(argname, "arg%d, ", i);
  }

  if (vtkWrap_IsEnumMember(data, arg))
  {
    fprintf(fp, "%sGetEnumValue(%stemp%d, \"%s.%s\")", prefix, argname, i, data->Name, arg->Class);
  }
  else if (arg->IsEnum)
  {
    cp = arg->Class;
    for (l = 0; cp[l] != '\0'; l++)
    {
      if (cp[l] == ':')
      {
        break;
      }
    }
    if (cp[l] == ':' && cp[l + 1] == ':')
    {
      fprintf(fp, "%sGetEnumValue(%stemp%d, \"%*.*s.%s\")", prefix, argname, i, (int)l, (int)l, cp,
        &cp[l + 2]);
    }
    else
    {
      fprintf(fp, "%sGetEnumValue(%stemp%d, \"%s\")", prefix, argname, i, cp);
    }
  }
  else if (vtkWrap_IsPythonObject(arg))
  {
    fprintf(fp, "%s%sGetPythonObject(temp%d)", prefix, argname, i);
  }
  else if (vtkWrap_IsVTKObject(arg))
  {
    vtkWrapText_PythonName(arg->Class, pythonname);
    if (strcmp(arg->Class, pythonname) != 0)
    {
      /* use typeid() for templated names */
      fprintf(fp, "%sGetVTKObject(%stemp%d, typeid(%s).name())", prefix, argname, i, arg->Class);
    }
    else
    {
      fprintf(fp, "%sGetVTKObject(%stemp%d, \"%s\")", prefix, argname, i, pythonname);
    }
  }
  else if (vtkWrap_IsSpecialObject(arg) && !vtkWrap_IsNonConstRef(arg))
  {
    vtkWrapText_PythonName(arg->Class, pythonname);
    fprintf(fp, "%sGetSpecialObject(%stemp%d, pobj%d, \"%s\")", prefix, argname, i, i, pythonname);
  }
  else if (vtkWrap_IsSpecialObject(arg) && vtkWrap_IsNonConstRef(arg))
  {
    vtkWrapText_PythonName(arg->Class, pythonname);
    fprintf(fp, "%sGetSpecialObject(%stemp%d, \"%s\")", prefix, argname, i, pythonname);
  }
  else if (vtkWrap_IsFunction(arg))
  {
    fprintf(fp, "%sGetFunction(%stemp%d)", prefix, argname, i);
  }
  else if (vtkWrap_IsVoidPointer(arg) || vtkWrap_IsZeroCopyPointer(arg))
  {
    fprintf(fp, "%sGetBuffer(%stemp%d, &pbuf%d)", prefix, argname, i, i);
  }
  else if (vtkWrap_IsString(arg) || (vtkWrap_IsCharPointer(arg) && vtkWrap_IsConst(arg)))
  {
    fprintf(fp, "%sGetValue(%stemp%d)", prefix, argname, i);
  }
  else if (vtkWrap_IsNumeric(arg) && vtkWrap_IsScalar(arg))
  {
    fprintf(fp, "%sGetValue(%stemp%d)", prefix, argname, i);
  }
  else if (vtkWrap_IsNArray(arg))
  {
    fprintf(fp, "%sGetNArray(%s%.*stemp%d, %d, size%d)", prefix, argname,
      (int)(arg->NumberOfDimensions - 1), "**********", i, arg->NumberOfDimensions, i);
  }
  else if (vtkWrap_IsArray(arg))
  {
    fprintf(fp, "%sGetArray(%stemp%d, size%d)", prefix, argname, i, i);
  }
  else if (vtkWrap_IsPODPointer(arg) || vtkWrap_IsCharPointer(arg))
  {
    fprintf(fp, "%sGetArray(%stemp%d, size%d)", prefix, argname, i, i);
  }
  else if (vtkWrap_IsStdVector(arg))
  {
    fprintf(fp, "%sGetArray(%stemp%d.data(), temp%d.size())", prefix, argname, i, i);
  }
}

/* -------------------------------------------------------------------- */
/* Write the code to convert the arguments with vtkPythonArgs */
static void vtkWrapPython_GetAllParameters(FILE* fp, ClassInfo* data, FunctionInfo* currentFunction)
{
  ValueInfo* arg;
  int requiredArgs, totalArgs;
  int i;

  totalArgs = vtkWrap_CountWrappedParameters(currentFunction);
  requiredArgs = vtkWrap_CountRequiredArguments(currentFunction);

  if (requiredArgs == totalArgs)
  {
    fprintf(fp, "ap.CheckArgCount(%d)", totalArgs);
  }
  else
  {
    fprintf(fp, "ap.CheckArgCount(%d, %d)", requiredArgs, totalArgs);
  }

  for (i = 0; i < totalArgs; i++)
  {
    arg = currentFunction->Parameters[i];

    fprintf(fp,
      " &&\n"
      "      ");

    if (i >= requiredArgs)
    {
      fprintf(fp, "(ap.NoArgsLeft() || ");
    }

    vtkWrapPython_GetSingleArgument(fp, data, i, arg, 0);

    if (i >= requiredArgs)
    {
      fprintf(fp, ")");
    }

    if (vtkWrap_IsFunction(arg))
    {
      break;
    }
  }

  /* loop again, check sizes against any size hints */
  for (i = 0; i < totalArgs; i++)
  {
    arg = currentFunction->Parameters[i];

    if (arg->CountHint && !vtkWrap_IsRef(arg))
    {
      fprintf(fp,
        " &&\n"
        "      ap.CheckSizeHint(%d, size%d, ",
        i, i);

      /* write out the code that gives the size */
      vtkWrapPython_SubstituteCode(fp, data, currentFunction, arg->CountHint);

      fprintf(fp, ")");
    }

    if (vtkWrap_IsFunction(arg))
    {
      break;
    }
  }
}

/* -------------------------------------------------------------------- */
/* Write code for execution after the method parameters are evaluated */
static void vtkWrapPython_SubstituteCode(
  FILE* fp, ClassInfo* data, FunctionInfo* func, const char* code)
{
  StringTokenizer t;
  int qualified = 0;
  int matched;
  int j;

  /* tokenize the code according to C/C++ rules */
  vtkParse_InitTokenizer(&t, code, WS_DEFAULT);
  do
  {
    /* check whether we have found an unqualified identifier */
    matched = 0;
    if ((t.tok == TOK_ID || t.tok == '#') && !qualified)
    {
      /* check for "this" */
      if (t.len == 4 && strncmp(t.text, "this", 4) == 0)
      {
        fprintf(fp, "op");
        matched = 1;
      }

      if (!matched) /* check for parameters */
      {
        ValueInfo* arg = NULL;

        /* check for positional parameter "#n" */
        if (t.tok == '#' && vtkParse_NextToken(&t) && t.tok == TOK_NUMBER)
        {
          j = (int)atol(t.text);
          arg = func->Parameters[j];
        }
        else
        {
          for (j = 0; j < func->NumberOfParameters; j++)
          {
            const char* name;
            arg = func->Parameters[j];
            name = arg->Name;
            if (name && strlen(name) == t.len && strncmp(name, t.text, t.len) == 0)
            {
              break;
            }
            arg = NULL;
          }
        }

        if (arg)
        {
          matched = 1;
          if (vtkWrap_IsSpecialObject(arg) && !vtkWrap_IsPointer(arg))
          {
            fprintf(fp, "(*temp%d)", j);
          }
          else
          {
            fprintf(fp, "temp%d", j);
          }
        }
      }

      if (!matched) /* check for class members */
      {
        for (j = 0; j < data->NumberOfItems; j++)
        {
          ItemInfo* item = &data->Items[j];
          const char* name = NULL;
          int is_static = 0;

          if (item->Type == VTK_FUNCTION_INFO)
          {
            /* methods */
            name = data->Functions[item->Index]->Name;
            is_static = data->Functions[item->Index]->IsStatic;
          }
          else if (item->Type == VTK_VARIABLE_INFO)
          {
            /* member variables */
            name = data->Variables[item->Index]->Name;
            is_static = data->Variables[item->Index]->IsStatic;
          }
          else if (item->Type == VTK_CONSTANT_INFO)
          {
            /* enum values and other constants */
            name = data->Constants[item->Index]->Name;
            is_static = 1;
          }

          if (name && strlen(name) == t.len && strncmp(name, t.text, t.len) == 0)
          {
            if (is_static)
            {
              fprintf(fp, "%s::%s", data->Name, name);
            }
            else
            {
              fprintf(fp, "op->%s", name);
            }
            matched = 1;
            break;
          }
        }
      }
    }

    if (!matched)
    {
      fprintf(fp, "%*.*s", (int)t.len, (int)t.len, t.text);
    }

    /* if next character is whitespace, add a space */
    if (vtkParse_CharType(t.text[t.len], CPRE_WHITE))
    {
      fprintf(fp, " ");
    }

    /* check whether the next identifier is qualified */
    qualified = (t.tok == TOK_SCOPE || t.tok == TOK_ARROW || t.tok == '.');
  } while (vtkParse_NextToken(&t));
}

/* -------------------------------------------------------------------- */
/* Check "expects" preconditions prior to calling the function */
static void vtkWrapPython_CheckPreconds(FILE* fp, ClassInfo* data, FunctionInfo* func)
{
  int i;

  /* parse the preconditions */
  for (i = 0; i < func->NumberOfPreconds; i++)
  {
    const char* precond = func->Preconds[i];

    /* write out the start of the check for the precondition */
    fprintf(fp,
      " &&\n"
      "      ap.CheckPrecond((");

    /* write out the code that checks the condition */
    vtkWrapPython_SubstituteCode(fp, data, func, precond);

    /* write out the end of the check for the precondition */
    fprintf(fp, "),%s\"%s\")", (strlen(precond) < 24 ? " " : "\n                      "),
      vtkWrapText_QuoteString(precond, 200));
  }
}

/* -------------------------------------------------------------------- */
/* Convert values into python object and return them within python */
void vtkWrapPython_ReturnValue(FILE* fp, ClassInfo* data, ValueInfo* val, int static_call)
{
  char pythonname[1024];
  const char* deref = "";
  const char* member = ".";
  const char* prefix = "ap.";

  if (static_call)
  {
    prefix = "vtkPythonArgs::";

    fprintf(fp,
      "    if (PyErr_Occurred() == nullptr)\n"
      "    {\n");
  }
  else
  {
    fprintf(fp,
      "    if (!ap.ErrorOccurred())\n"
      "    {\n");
  }

  if (val && vtkWrap_IsRef(val))
  {
    deref = "*";
    member = "->";
  }

  if (vtkWrap_IsVoid(val))
  {
    fprintf(fp, "      result = %sBuildNone();\n", prefix);
  }
  else if (vtkWrap_IsEnumMember(data, val))
  {
    vtkWrapText_PythonName(data->Name, pythonname);
    fprintf(fp, "      result = Py%s_%s_FromEnum(tempr);\n", pythonname, val->Class);
  }
  else if (val->IsEnum)
  {
    const char* cp = val->Class;
    size_t l;
    /* search for scope operator */
    for (l = 0; cp[l] != '\0'; l++)
    {
      if (cp[l] == ':')
      {
        break;
      }
    }
    if (cp[l] == ':' && cp[l + 1] == ':')
    {
      fprintf(fp, "      result = %sBuildEnumValue(tempr, \"%*.*s.%s\");\n", prefix, (int)l, (int)l,
        cp, &cp[l + 2]);
    }
    else
    {
      fprintf(fp, "      result = %sBuildEnumValue(tempr, \"%s\");\n", prefix, cp);
    }
  }
  else if (vtkWrap_IsPythonObject(val))
  {
    fprintf(fp, "      result = tempr;\n");
  }
  else if (vtkWrap_IsVTKObject(val))
  {
    fprintf(fp, "      result = %sBuildVTKObject(tempr);\n", prefix);

    if (vtkWrap_IsNewInstance(val))
    {
      fprintf(fp,
        "      if (result && PyVTKObject_Check(result))\n"
        "      {\n"
        "        PyVTKObject_GetObject(result)->UnRegister(nullptr);\n"
        "        PyVTKObject_SetFlag(result, VTK_PYTHON_IGNORE_UNREGISTER, 1);\n"
        "      }\n");
    }
  }
  else if (vtkWrap_IsSpecialObject(val) && vtkWrap_IsRef(val))
  {
    vtkWrapText_PythonName(val->Class, pythonname);
    fprintf(fp, "      result = %sBuildSpecialObject(tempr, \"%s\");\n", prefix, pythonname);
  }
  else if (vtkWrap_IsSpecialObject(val) && !vtkWrap_IsRef(val))
  {
    vtkWrapText_PythonName(val->Class, pythonname);
    fprintf(fp, "      result = %sBuildSpecialObject(&tempr, \"%s\");\n", prefix, pythonname);
  }
  else if (vtkWrap_IsCharPointer(val))
  {
    fprintf(fp, "      result = %sBuildValue(tempr);\n", prefix);
  }
  else if (vtkWrap_IsVoidPointer(val))
  {
    fprintf(fp, "      result = %sBuildValue(tempr);\n", prefix);
  }
  else if (vtkWrap_IsChar(val) && vtkWrap_IsArray(val))
  {
    fprintf(fp, "      result = %sBuildBytes(tempr, sizer);\n", prefix);
  }
  else if (vtkWrap_IsArray(val))
  {
    fprintf(fp, "      result = vtkPythonArgs::BuildTuple(tempr, sizer);\n");
  }
  else if (vtkWrap_IsStdVector(val))
  {
    fprintf(fp,
      "      if (tempr%ssize() == 0)\n"
      "      {\n"
      "        result = PyTuple_New(0);\n"
      "      }\n"
      "      else\n"
      "      {\n"
      "        result = %sBuildTuple(tempr%sdata(), tempr%ssize());\n"
      "      }\n",
      member, prefix, member, member);
  }
  else
  {
    fprintf(fp, "      result = %sBuildValue(%stempr);\n", prefix, deref);
  }

  fprintf(fp, "    }\n");
}

/* -------------------------------------------------------------------- */
/* Look for all signatures of the specified method.  Return the number
 * found, as well as whether all signatures were static or legacy */

static int vtkWrapPython_CountAllOccurrences(
  FunctionInfo** wrappedFunctions, int n, int fnum, int* all_static, int* all_legacy)
{
  const char* name;
  int occ;
  int numberOfOccurrences = 0;

  *all_static = 1;
  *all_legacy = 1;

  name = wrappedFunctions[fnum]->Name;

  for (occ = fnum; occ < n; occ++)
  {
    /* is it the same name */
    if (wrappedFunctions[occ]->Name && !strcmp(name, wrappedFunctions[occ]->Name))
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
/* Save a copy of each non-const array arg, so that we can check
 * if they were changed by the method call */
void vtkWrapPython_SaveArgs(FILE* fp, FunctionInfo* currentFunction)
{
  const char* asterisks = "**********";
  ValueInfo* arg;
  int i, j, n, m;
  int noneDone = 1;

  /* do nothing for SetVector macros */
  if (vtkWrap_IsSetVectorMethod(currentFunction))
  {
    return;
  }

  m = vtkWrap_CountWrappedParameters(currentFunction);

  /* save arrays for args that are non-const */
  for (i = 0; i < m; i++)
  {
    arg = currentFunction->Parameters[i];
    n = arg->NumberOfDimensions;
    if (n < 1 && (vtkWrap_IsArray(arg) || vtkWrap_IsPODPointer(arg) || vtkWrap_IsCharPointer(arg)))
    {
      n = 1;
    }

    if ((vtkWrap_IsArray(arg) || vtkWrap_IsNArray(arg) || vtkWrap_IsPODPointer(arg) ||
          vtkWrap_IsCharPointer(arg)) &&
      (arg->Type & VTK_PARSE_CONST) == 0 && !vtkWrap_IsRef(arg))
    {
      noneDone = 0;

      fprintf(fp, "    vtkPythonArgs::Save(%.*stemp%d, %.*ssave%d, ", (n - 1), asterisks, i,
        (n - 1), asterisks, i);

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
    fprintf(fp, "\n");
  }
}

/* -------------------------------------------------------------------- */
/* generate the code that calls the C++ method */
static void vtkWrapPython_GenerateMethodCall(
  FILE* fp, FunctionInfo* currentFunction, ClassInfo* data, HierarchyInfo* hinfo, int is_vtkobject)
{
  char methodname[256];
  ValueInfo* arg;
  int totalArgs;
  int is_constructor;
  int i, k, n;

  totalArgs = vtkWrap_CountWrappedParameters(currentFunction);

  is_constructor = vtkWrap_IsConstructor(data, currentFunction);

  /* for vtkobjects, do a bound call and an unbound call */
  n = 1;
  if (is_vtkobject && !currentFunction->IsStatic && !currentFunction->IsPureVirtual &&
    !is_constructor)
  {
    n = 2;
  }

  if (!is_constructor && !vtkWrap_IsVoid(currentFunction->ReturnValue))
  {
    /* temp variable for C++-type return value */
    fprintf(fp, "  ");
    vtkWrap_DeclareVariable(
      fp, data, currentFunction->ReturnValue, "tempr", -1, VTK_WRAP_RETURN | VTK_WRAP_NOSEMI);
    fprintf(fp, " =");
  }

  /* handle both bound and unbound calls */
  if (n == 2)
  {
    if (!is_constructor && !vtkWrap_IsVoid(currentFunction->ReturnValue))
    {
      fprintf(fp,
        " (ap.IsBound() ?\n"
        "     ");
    }
    else
    {
      fprintf(fp,
        "    if (ap.IsBound())\n"
        "    {\n"
        "  ");
    }
  }

  /* print the code that calls the method */
  for (k = 0; k < n; k++)
  {
    if (k == 1)
    {
      /* unbound method call */
      sprintf(methodname, "op->%s::%s", data->Name, currentFunction->Name);
    }
    else if (currentFunction->IsStatic)
    {
      /* static method call */
      sprintf(methodname, "%s::%s", data->Name, currentFunction->Name);
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

    if (is_constructor)
    {
      fprintf(fp, "    %s *op = new %s(", data->Name, data->Name);
    }
    else if (vtkWrap_IsVoid(currentFunction->ReturnValue))
    {
      fprintf(fp, "    %s(", methodname);
    }
    else if (vtkWrap_IsRef(currentFunction->ReturnValue))
    {
      fprintf(fp, " &%s(", methodname);
    }
    else
    {
      fprintf(fp, " %s(", methodname);
    }

    /* print all the arguments in the call */
    for (i = 0; i < totalArgs; i++)
    {
      arg = currentFunction->Parameters[i];

      if (vtkWrap_IsFunction(arg))
      {
        fprintf(fp,
          "\n"
          "        (temp%d == Py_None ? nullptr : vtkPythonVoidFunc),\n"
          "        (temp%d == Py_None ? nullptr : temp%d));\n",
          i, i, i);
        fprintf(fp,
          "      if (temp%d != Py_None)\n"
          "      {\n"
          "        Py_INCREF(temp%d);\n"
          "      }\n"
          "      %sArgDelete(\n"
          "        (temp%d == Py_None ? nullptr : vtkPythonVoidFuncArgDelete)",
          i, i, methodname, i);
        break;
      }

      if (i)
      {
        fprintf(fp, ", ");
      }

      if (vtkWrap_IsSpecialObject(arg) && !vtkWrap_IsPointer(arg))
      {
        fprintf(fp, "*temp%i", i);
      }
      else if (vtkWrap_IsConst(arg) && vtkWrap_IsRef(arg) &&
        (arg->CountHint || vtkWrap_IsPODPointer(arg)))
      {
        fprintf(fp, "temp%ic", i);
      }
      else
      {
        fprintf(fp, "temp%i", i);
      }
    }
    fprintf(fp, ")");

    /* handle ternary operator for ap.IsBound() */
    if (n == 2)
    {
      if (!is_constructor && !vtkWrap_IsVoid(currentFunction->ReturnValue))
      {
        fprintf(fp, (k == 0 ? " :\n     " : ");\n"));
      }
      else if (k == 0)
      {
        fprintf(fp,
          ";\n"
          "    }\n"
          "    else\n"
          "    {\n"
          "  ");
      }
      else
      {
        fprintf(fp,
          ";\n"
          "    }\n");
      }
    }
    else
    {
      fprintf(fp, ";\n");
    }
  }

  if (is_constructor)
  {
    /* initialize tuples created with default constructor */
    if (currentFunction->NumberOfParameters == 0 && hinfo)
    {
      n = vtkWrap_GetTupleSize(data, hinfo);
      for (i = 0; i < n; i++)
      {
        fprintf(fp, "    (*op)[%d] = 0;\n", i);
      }
    }
  }

  fprintf(fp, "\n");
}

/* -------------------------------------------------------------------- */
/* Write back to all the reference arguments and array arguments that
 * were passed, but only write to arrays if the array has changed and
 * the array arg was non-const */
static void vtkWrapPython_WriteBackToArgs(FILE* fp, ClassInfo* data, FunctionInfo* currentFunction)
{
  const char* asterisks = "**********";
  ValueInfo* arg;
  int i, j, n, m;

  /* do nothing for SetVector macros */
  if (vtkWrap_IsSetVectorMethod(currentFunction))
  {
    return;
  }

  m = vtkWrap_CountWrappedParameters(currentFunction);

  /* check array value change for args that are non-const */
  for (i = 0; i < m; i++)
  {
    arg = currentFunction->Parameters[i];
    n = arg->NumberOfDimensions;
    if (n < 1 &&
      (vtkWrap_IsArray(arg) || vtkWrap_IsPODPointer(arg) ||
        (vtkWrap_IsCharPointer(arg) && !vtkWrap_IsConst(arg))))
    {
      n = 1;
    }

    if (vtkWrap_IsNonConstRef(arg) && !vtkWrap_IsStdVector(arg) && !vtkWrap_IsObject(arg))
    {
      fprintf(fp,
        "    if (!ap.ErrorOccurred())\n"
        "    {\n");

      if (vtkWrap_IsArray(arg) || vtkWrap_IsPODPointer(arg))
      {
        fprintf(fp, "      ap.SetArgValue(%d, temp%d%s, ", i, i, (vtkWrap_IsConst(arg) ? "c" : ""));
        if (arg->CountHint)
        {
          vtkWrapPython_SubstituteCode(fp, data, currentFunction, arg->CountHint);
        }
        else
        {
          fprintf(fp, "size%d", i);
        }
        fprintf(fp, ");\n");
      }
      else
      {
        fprintf(fp, "      ap.SetArgValue(%d, temp%d);\n", i, i);
      }
      fprintf(fp, "    }\n");
    }
    else if ((vtkWrap_IsArray(arg) || vtkWrap_IsNArray(arg) || vtkWrap_IsPODPointer(arg) ||
               vtkWrap_IsCharPointer(arg)) &&
      !vtkWrap_IsConst(arg) && !vtkWrap_IsSetVectorMethod(currentFunction))
    {
      fprintf(fp, "    if (vtkPythonArgs::HasChanged(%.*stemp%d, %.*ssave%d, ", (n - 1), asterisks,
        i, (n - 1), asterisks, i);

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

      fprintf(fp,
        ") &&\n"
        "        !ap.ErrorOccurred())\n"
        "    {\n");

      if (vtkWrap_IsNArray(arg))
      {
        fprintf(
          fp, "      ap.SetNArray(%d, %.*stemp%d, %d, size%d);\n", i, (n - 1), asterisks, i, n, i);
      }
      else
      {
        fprintf(fp, "      ap.SetArray(%d, temp%d, size%d);\n", i, i, i);
      }

      fprintf(fp,
        "    }\n"
        "\n");
    }
    else if (vtkWrap_IsStdVector(arg) && !vtkWrap_IsConst(arg))
    {
      fprintf(fp,
        "    if (!ap.ErrorOccurred())\n"
        "    {\n"
        "      PyObject *vec = (temp%d.size() == 0 ?\n"
        "        PyTuple_New(0) :\n"
        "        vtkPythonArgs::BuildTuple(temp%d.data(), temp%d.size()));\n"
        "      ap.SetContents(%d, vec);\n"
        "      Py_DECREF(vec);\n"
        "    }\n"
        "\n",
        i, i, i, i);
    }
  }
}

/* -------------------------------------------------------------------- */
/* Free any temporaries that were needed for the C++ method call*/
static void vtkWrapPython_FreeTemporaries(FILE* fp, FunctionInfo* currentFunction)
{
  ValueInfo* arg;
  int i, j, n;

  n = vtkWrap_CountWrappedParameters(currentFunction);

  /* check array value change for args that are non-const */
  j = 0;
  for (i = 0; i < n; i++)
  {
    arg = currentFunction->Parameters[i];

    if (vtkWrap_IsVoidPointer(arg) || vtkWrap_IsZeroCopyPointer(arg))
    {
      /* release Py_buffer objects */
      fprintf(fp,
        "#if PY_VERSION_HEX >= 0x02060000\n"
        "  if (pbuf%d.obj != 0)\n"
        "  {\n"
        "    PyBuffer_Release(&pbuf%d);\n"
        "  }\n"
        "#endif\n",
        i, i);
    }
    else if (vtkWrap_IsSpecialObject(arg) && !vtkWrap_IsNonConstRef(arg))
    {
      /* decref any PyObjects created via conversion constructors */
      fprintf(fp, "  Py_XDECREF(pobj%d);\n", i);
      j = 1;
    }
  }

  if (j)
  {
    fprintf(fp, "\n");
  }
}

/* -------------------------------------------------------------------- */
/* Write out the code for one method (including all its overloads) */

void vtkWrapPython_GenerateOneMethod(FILE* fp, const char* classname, ClassInfo* data,
  HierarchyInfo* hinfo, FunctionInfo* wrappedFunctions[], int numberOfWrappedFunctions, int fnum,
  int is_vtkobject, int do_constructors)
{
  FunctionInfo* theFunc;
  char occSuffix[16];
  FunctionInfo* theOccurrence;
  int occ, numberOfOccurrences;
  int occCounter;
  int all_static = 0;
  int all_legacy = 0;
  char* cp;
  int* overloadMap = NULL;
  int maxArgs = 0;
  int overlap = 0;

  theFunc = wrappedFunctions[fnum];

  /* count all signatures, see if they are static methods or legacy */
  numberOfOccurrences = vtkWrapPython_CountAllOccurrences(
    wrappedFunctions, numberOfWrappedFunctions, fnum, &all_static, &all_legacy);

  /* find all occurrences of this method */
  occCounter = 0;
  for (occ = fnum; occ < numberOfWrappedFunctions; occ++)
  {
    theOccurrence = wrappedFunctions[occ];

    /* is it the same name */
    if (theOccurrence->Name && strcmp(theFunc->Name, theOccurrence->Name) == 0)
    {
      occCounter++;

      if (theOccurrence->IsLegacy)
      {
        fprintf(fp, "#if !defined(VTK_LEGACY_REMOVE)\n");
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
        classname, theOccurrence->Name, occSuffix,
        ((theOccurrence->IsStatic | do_constructors) ? " /*unused*/" : "self"));

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
          "  vtkPythonArgs ap(self, args, \"%s\");\n"
          "  void *vp = ap.GetSelfSpecialPointer(self, args);\n"
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
      vtkWrapPython_DeclareVariables(fp, data, theOccurrence);

      /* open the "if" for getting all the args */
      fprintf(fp, "  if (");

      if (!theOccurrence->IsStatic && !do_constructors)
      {
        /* if not static, make sure the object is not null */
        fprintf(fp, "op && ");

        if (is_vtkobject)
        {
          /* special things for vtkObject methods */
          if (theOccurrence->IsPureVirtual)
          {
            fprintf(fp, "!ap.IsPureVirtual() && ");
          }
        }
      }

      /* get all the arguments */
      vtkWrapPython_GetAllParameters(fp, data, theOccurrence);

      /* check preconditions */
      if (theOccurrence->NumberOfPreconds > 0)
      {
        vtkWrapPython_CheckPreconds(fp, data, theOccurrence);
      }

      /* finished getting all the arguments */
      fprintf(fp,
        ")\n"
        "  {\n");

      /* get size for variable-size return arrays */
      if (theOccurrence->ReturnValue && theOccurrence->ReturnValue->CountHint)
      {
        fprintf(fp, "    size_t sizer = ");
        vtkWrapPython_SubstituteCode(
          fp, data, theOccurrence, theOccurrence->ReturnValue->CountHint);
        fprintf(fp, ";\n");
      }

      /* save a copy of all non-const array arguments */
      vtkWrapPython_SaveArgs(fp, theOccurrence);

      /* generate the code that calls the C++ method */
      vtkWrapPython_GenerateMethodCall(fp, theOccurrence, data, hinfo, is_vtkobject);

      /* write back to all array args */
      vtkWrapPython_WriteBackToArgs(fp, data, theOccurrence);

      /* generate the code that builds the return value */
      if (do_constructors && !is_vtkobject)
      {
        fprintf(fp, "    result = PyVTKSpecialObject_New(\"%s\", op);\n", classname);
      }
      else
      {
        vtkWrapPython_ReturnValue(fp, data, theOccurrence->ReturnValue, 0);
      }

      /* close off the big "if" */
      fprintf(fp,
        "  }\n"
        "\n");

      /* free any temporary values that were constructed or allocated */
      vtkWrapPython_FreeTemporaries(fp, theOccurrence);

      /* it's all over... return the result */
      fprintf(fp,
        "  return result;\n"
        "}\n");

      if (theOccurrence->IsLegacy)
      {
        fprintf(fp, "#endif\n");
      }

      fprintf(fp, "\n");
    }
  }

  /* check for overloads */
  overloadMap = vtkWrapPython_ArgCountToOverloadMap(
    wrappedFunctions, numberOfWrappedFunctions, fnum, is_vtkobject, &maxArgs, &overlap);

  if (overlap || do_constructors)
  {
    /* output the method table for the signatures */
    vtkWrapPython_OverloadMethodDef(fp, classname, data, overloadMap, wrappedFunctions,
      numberOfWrappedFunctions, fnum, numberOfOccurrences, all_legacy);
  }

  if (numberOfOccurrences > 1)
  {
    /* declare a "master method" to choose among the overloads */
    vtkWrapPython_OverloadMasterMethod(fp, classname, overloadMap, maxArgs, wrappedFunctions,
      numberOfWrappedFunctions, fnum, is_vtkobject, all_legacy);
  }

  /* set the legacy flag */
  theFunc->IsLegacy = all_legacy;

  /* clear all occurrences of this method from further consideration */
  for (occ = fnum + 1; occ < numberOfWrappedFunctions; occ++)
  {
    theOccurrence = wrappedFunctions[occ];

    /* is it the same name */
    if (theOccurrence->Name && !strcmp(theFunc->Name, theOccurrence->Name))
    {
      size_t siglen = strlen(theFunc->Signature);

      /* memory leak here but ... */
      theOccurrence->Name = NULL;
      cp = (char*)malloc(siglen + 2 + strlen(theOccurrence->Signature));
      strcpy(cp, theFunc->Signature);
      strcpy(&cp[siglen], "\n");
      strcpy(&cp[siglen + 1], theOccurrence->Signature);
      theFunc->Signature = cp;
    }
  }
}
