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

/* modify the array count for vtkDataArray methods */
static void vtkWrapPython_DiscoverPointerCounts(
  FILE *fp, ClassInfo *data, HierarchyInfo *hinfo);

/* print out all methods and the method table */
static void vtkWrapPython_GenerateMethods(
  FILE *fp, ClassInfo *data, HierarchyInfo *hinfo,
  int is_vtkobject, int do_constructors);

/* make a variable for an arg value, or a return value if i = -1 */
static void vtkWrapPython_MakeVariable(
  FILE *fp, ValueInfo *val, const char *name, int i);

/* make a static array containing array dimensions */
static void vtkWrapPython_MakeVariableDims(
  FILE *fp, ValueInfo *val, const char *name, int i);

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

/* Delete object created by conversion constructors */
static void vtkWrapPython_FreeConstructedObjects(
  FILE *fp, FunctionInfo *currentFunction);

/* output the method table for all overloads of a particular method */
static void vtkWrapPython_OverloadMethodDef(
  FILE *fp, ClassInfo *data, int *overloadMap, int maxArgs,
  FunctionInfo **wrappedFunctions, int numberOfWrappedFunctions, int fnum,
  int numberOfOccurrences, int is_vtkobject, int all_legacy);

/* a master method to choose which overload to call */
static void vtkWrapPython_OverloadMasterMethod(
  FILE *fp, ClassInfo *data, FunctionInfo *currentFunction,
  int numberOfOccurrences, int *overloadMap, int maxArgs, int all_legacy);

/* output the MethodDef table for this class */
static void vtkWrapPython_ClassMethodDef(
  FILE *fp, ClassInfo *data,
  FunctionInfo **wrappedFunctions, int numberOfWrappedFunctions, int fnum);

/* write out a python type object */
static void vtkWrapPython_GenerateSpecialType(
  FILE *fp, ClassInfo *data, FileInfo *finfo, HierarchyInfo *hinfo);

/* -------------------------------------------------------------------- */
/* prototypes for utility methods */

/* Make a guess about whether a class is wrapped */
int vtkWrapPython_IsClassWrapped(
  HierarchyInfo *hinfo, const char *classname);

/* check whether a method is wrappable */
static int vtkWrapPython_MethodCheck(
  ClassInfo *data, FunctionInfo *currentFunction, HierarchyInfo *hinfo);

/* is the method a constructor of the class */
static int vtkWrapPython_IsConstructor(
  ClassInfo *data, FunctionInfo *currentFunction);

/* is the method a destructor of the class */
static int vtkWrapPython_IsDestructor(
  ClassInfo *data, FunctionInfo *currentFunction);

/* Check if a method is from a SetVector method */
static int vtkWrapPython_IsSetVectorMethod(
  FunctionInfo *currentFunction);

/* Get the python format char for the give type */
static char vtkWrapPython_FormatChar(
  unsigned int argtype);

/* create a format string for PyArg_ParseTuple */
static char *vtkWrapPython_FormatString(
  FunctionInfo *currentFunction, int *requiredArgs);

/* weed out methods that will never be called */
static void vtkWrapPython_RemovePreceededMethods(
  FunctionInfo *wrappedFunctions[],
  int numberWrapped, int fnum);

/* look for all signatures of the specified method */
static int vtkWrapPython_CountAllOccurrences(
  ClassInfo *data, FunctionInfo **wrappedFunctions, int n,
  int fnum, int *all_static, int *all_legacy);

/* adjust number of args for SetFunction methods */
static int vtkWrapPython_GetNumberOfWrappedArgs(
  FunctionInfo *currentFunction);

/* count the number of required arguments */
static int vtkWrapPython_GetNumberOfRequiredArgs(
  FunctionInfo *currentFunction, int totalArgs);

/* generate an int array that maps arg counts to overloads */
static int *vtkWrapPython_ArgCountToOverloadMap(
  FunctionInfo **wrappedFunctions, int numberOfWrappedFunctions,
  int fnum, int numberOfOccurrences, int *nmax, int *overlap);

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

/* expand all typedef types that are used in function arguments */
static void vtkWrapPython_ExpandTypedefs(
  ClassInfo *data, HierarchyInfo *hinfo);

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
  HierarchyInfo *hinfo, const char *classname)
{
  if (hinfo)
    {
    HierarchyEntry *entry;
    entry = vtkParseHierarchy_FindEntry(hinfo, classname);

    if (entry)
      {
      if (!vtkParseHierarchy_GetProperty(entry, "WRAP_EXCLUDE") ||
          vtkParseHierarchy_GetProperty(entry, "WRAP_SPECIAL"))
        {
        return 1;
        }
      }
    }
  else if (strncmp("vtk", classname, 3) == 0)
    {
    return 1;
    }

  return 0;
}

/* -------------------------------------------------------------------- */
/* Check if the class is derived from superclass */
static int vtkWrapPython_IsTypeOf(
  HierarchyInfo *hinfo, const char *classname, const char *superclass)
{
  HierarchyEntry *entry;

  if (strcmp(classname, superclass) == 0)
    {
    return 1;
    }

  if (hinfo)
    {
    entry = vtkParseHierarchy_FindEntry(hinfo, classname);
    if (entry && vtkParseHierarchy_IsTypeOf(hinfo, entry, superclass))
      {
      return 1;
      }
    }

  return 0;
}

/* -------------------------------------------------------------------- */
/* Check if the WRAP_SPECIAL flag is set for the class. */
static int vtkWrapPython_IsSpecialType(
  HierarchyInfo *hinfo, const char *classname)
{
  HierarchyEntry *entry;

  if (hinfo)
    {
    entry = vtkParseHierarchy_FindEntry(hinfo, classname);
    if (vtkParseHierarchy_GetProperty(entry, "WRAP_SPECIAL"))
      {
      return 1;
      }
    return 0;
    }

  /* fallback if no HierarchyInfo */
  if (strncmp("vtk", classname, 3) == 0)
    {
    return -1;
    }

  return 0;
}

/* -------------------------------------------------------------------- */
/* Check whether the class is derived from vtkObjectBase. */
static int vtkWrapPython_IsObjectBaseType(
  HierarchyInfo *hinfo, const char *classname)
{
  HierarchyEntry *entry;

  if (hinfo)
    {
    entry = vtkParseHierarchy_FindEntry(hinfo, classname);
    if (entry)
      {
      if (vtkParseHierarchy_IsTypeOf(hinfo, entry, "vtkObjectBase"))
        {
        return 1;
        }
      return 0;
      }
    }

  /* fallback if no HierarchyInfo */
  if (strncmp("vtk", classname, 3) == 0)
    {
    return 1;
    }

  return 0;
}

/* -------------------------------------------------------------------- */
/* A type starting with "Qt::" is assumed to be a Qt enum */
static int vtkWrapPython_IsQtEnum(const char* type)
{
  if(type[0] == 'Q' && type[1] == 't' && type[2] == ':' && type[3] == ':')
    {
    return 1;
    }
  return 0;
}

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
/* This method produces a temporary variable of the required type:
 * "i" is the argument id, to keep the various temps unique, and
 * if do_return is set, then declare as return type instead
 * as an arg type */

static void vtkWrapPython_MakeVariable(
  FILE *fp, ValueInfo *val, const char *name, int i)
{
  int do_return = (i < 0 ? 1 : 0);
  unsigned int aType;
  const char *aClass;
  int j;

  if (val == NULL)
    {
    return;
    }

  aType = (val->Type & VTK_PARSE_UNQUALIFIED_TYPE);
  aClass = val->Class;

  /* do nothing for void */
  if (aType == VTK_PARSE_VOID)
    {
    return;
    }

  /* add a couple spaces */
  fprintf(fp,"  ");

  /* for const * return types, prepend with const */
  if (do_return)
    {
    if ((val->Type & VTK_PARSE_CONST) != 0 &&
        (aType & VTK_PARSE_INDIRECT) != 0)
      {
      fprintf(fp,"const ");
      }
    }
  /* do the same for "const char *" with initializer */
  else
    {
    if ((val->Type & VTK_PARSE_CONST) != 0 &&
        aType == VTK_PARSE_CHAR_PTR &&
        val->Value &&
        strcmp(val->Value, "0") != 0 &&
        strcmp(val->Value, "NULL") != 0)
      {
      fprintf(fp,"const ");
      }
    }

  /* for unsigned, prepend with "unsigned" */
  if ((aType & VTK_PARSE_UNSIGNED) != 0 && aType != VTK_PARSE_SIZE_T)
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
    case VTK_PARSE_QOBJECT:
    case VTK_PARSE_OBJECT:      fprintf(fp,"%s ", aClass); break;
    case VTK_PARSE_ID_TYPE:     fprintf(fp,"vtkIdType "); break;
    case VTK_PARSE_LONG_LONG:   fprintf(fp,"long long "); break;
    case VTK_PARSE___INT64:     fprintf(fp,"__int64 "); break;
    case VTK_PARSE_SIGNED_CHAR: fprintf(fp,"signed char "); break;
    case VTK_PARSE_BOOL:        fprintf(fp,"bool "); break;
    case VTK_PARSE_STRING:      fprintf(fp,"%s ", aClass); break;
    case VTK_PARSE_UNICODE_STRING: fprintf(fp, "vtkUnicodeString "); break;
    case VTK_PARSE_SSIZE_T:
      fprintf(fp,"%ssize_t ",((aType == VTK_PARSE_SIZE_T) ? "" : "s")); break;
    case VTK_PARSE_FUNCTION:    fprintf(fp,"PyObject "); break;
    case VTK_PARSE_UNKNOWN:     return;
    }

  /* indirection */
  if (do_return)
    {
    /* ref and pointer return values are stored as pointers */
    if ((aType & VTK_PARSE_INDIRECT) == VTK_PARSE_POINTER ||
        (aType & VTK_PARSE_INDIRECT) == VTK_PARSE_REF ||
        (aType & VTK_PARSE_BASE_TYPE) == VTK_PARSE_FUNCTION)
      {
      fprintf(fp, "*");
      }
    }
  else
    {
    /* objects refs and pointers are always handled via pointers,
     * other refs are passed by value */
    if (aType == VTK_PARSE_CHAR_PTR ||
        aType == VTK_PARSE_VOID_PTR ||
        aType == VTK_PARSE_OBJECT_PTR ||
        aType == VTK_PARSE_OBJECT_REF ||
        aType == VTK_PARSE_OBJECT ||
        aType == VTK_PARSE_QOBJECT_PTR ||
        ((aType == VTK_PARSE_QOBJECT_REF ||
          aType == VTK_PARSE_QOBJECT) &&
         !vtkWrapPython_IsQtEnum(aClass)) ||
        aType == VTK_PARSE_FUNCTION)
      {
      fprintf(fp, "*");
      }
    }

  /* the variable name */
  if (do_return)
    {
    fprintf(fp,"%s", name);
    }
  else
    {
    fprintf(fp,"%s%i", name, i);
    }

  if (!do_return)
    {
    /* print the array decorators */
    if (((aType & VTK_PARSE_POINTER_MASK) != 0) &&
        aType != VTK_PARSE_CHAR_PTR &&
        aType != VTK_PARSE_VOID_PTR &&
        aType != VTK_PARSE_OBJECT_PTR &&
        aType != VTK_PARSE_QOBJECT_PTR)
      {
      if (val->Count == -1)
        {
        /* array size is unknown */
        fprintf(fp, "[%i]", 20);
        }
      else
        {
        for (j = 0; j < val->NumberOfDimensions; j++)
          {
          fprintf(fp, "[%s]", val->Dimensions[j]);
          }
        }
      }

    /* add a default value */
    else if (val->Value)
      {
      fprintf(fp, " = %s", val->Value);
      }
    else if (aType == VTK_PARSE_CHAR_PTR ||
             aType == VTK_PARSE_VOID_PTR ||
             aType == VTK_PARSE_OBJECT_PTR ||
             aType == VTK_PARSE_OBJECT_REF ||
             aType == VTK_PARSE_OBJECT ||
             aType == VTK_PARSE_QOBJECT_PTR ||
             ((aType == VTK_PARSE_QOBJECT_REF ||
               aType == VTK_PARSE_QOBJECT) &&
              !vtkWrapPython_IsQtEnum(aClass)))
      {
      fprintf(fp, " = NULL");
      }
    else if (aType == VTK_PARSE_BOOL)
      {
      fprintf(fp, " = false");
      }
    }

  /* finish off with a semicolon */
  fprintf(fp, ";\n");
}

/* -------------------------------------------------------------------- */
/* Write the dimensions of an array into a static variable */

static void vtkWrapPython_MakeVariableDims(
  FILE *fp, ValueInfo *val, const char *name, int i)
{
  int j;

  if (val->NumberOfDimensions > 1)
    {
    fprintf(fp,
            "  static int %s%d[%d] = ",
            name, i, val->NumberOfDimensions);

    for (j = 0; j < val->NumberOfDimensions; j++)
      {
      fprintf(fp, "%c %s", ((j == 0) ? '{' : ','), val->Dimensions[j]);
      }

    fprintf(fp, " };\n");
    }
}

/* -------------------------------------------------------------------- */
/* Declare all local variables used by the wrapper method */
static void vtkWrapPython_DeclareVariables(
  FILE *fp, FunctionInfo *theFunc)
{
  ValueInfo *arg;
  int i;

  /* temp variables for arg values */
  for (i = 0; i < theFunc->NumberOfArguments; i++)
    {
    arg = theFunc->Arguments[i];

    vtkWrapPython_MakeVariable(fp, arg, "temp", i);

    if (arg->Type == VTK_PARSE_FUNCTION)
      {
      break;
      }

    /* temps for conversion constructed objects, which only occur
     * for special objects */
    if ((arg->Type & VTK_PARSE_BASE_TYPE) == VTK_PARSE_OBJECT &&
        ((arg->Type & VTK_PARSE_INDIRECT) == 0 ||
         ((arg->Type & VTK_PARSE_INDIRECT) == VTK_PARSE_REF &&
          (arg->Type & VTK_PARSE_CONST) != 0)))
      {
      fprintf(fp,
              "  PyObject *pobj%d = NULL;\n",
              i);
      }

    /* temps for arrays */
    if (arg->Count != 0)
      {
      if ((arg->Type & VTK_PARSE_CONST) == 0 &&
          !vtkWrapPython_IsSetVectorMethod(theFunc))
        {
        /* for saving a copy of the array */
        vtkWrapPython_MakeVariable(fp, arg, "save", i);
        }
      if (arg->NumberOfDimensions > 1)
        {
        /* write an int array containing the dimensions */
        vtkWrapPython_MakeVariableDims(fp, arg, "size", i);
        }
      else
        {
        /* the size for a one-dimensional array */
        fprintf(fp,
                "  %sint size%d = %d;\n",
                (arg->Count > 0 ? "const " : ""), i,
                (arg->Count < 0 ? 0 : arg->Count));
        }
      }
    }

  if (theFunc->ReturnValue)
    {
    /* temp variable for C++-type return value */
    vtkWrapPython_MakeVariable(fp, theFunc->ReturnValue,
      "tempr", -1);

    /* the size for a one-dimensional array */
    if (theFunc->ReturnValue->Count != 0)
      {
      fprintf(fp,
              "  int sizer = %d;\n",
              (theFunc->ReturnValue->Count < 0 ?
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
  const char *sizeMethod = "GetNumberOfComponents";
  int i, j;

  j = (is_vtkobject ? 1 : 0);
  for (i = 0; i < theFunc->NumberOfArguments; i++)
    {
    if (theFunc->Arguments[i]->Count == -1)
      {
      if (j == 1)
        {
        fprintf(fp,
                "  if (op)\n"
                "    {\n");
        }
      j += 2;
      fprintf(fp,
              "  %ssize%d = op->%s();\n",
              ((j & 1) != 0 ? "  " : ""), i, sizeMethod);
      }
    }
  if (theFunc->ReturnValue && theFunc->ReturnValue->Count == -1)
    {
    if (j == 1)
      {
      fprintf(fp,
              "  if (op)\n"
              "    {\n");
      }
    j += 2;
    fprintf(fp,
            "  %ssizer = op->%s();\n",
            ((j & 1) != 0 ? "  " : ""), sizeMethod);
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
  unsigned int argType;
  int requiredArgs, totalArgs;
  int i;

  totalArgs = vtkWrapPython_GetNumberOfWrappedArgs(currentFunction);
  requiredArgs = vtkWrapPython_GetNumberOfRequiredArgs(currentFunction,
                                                       totalArgs);

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

    /* remove const and ref for easier checking */
    argType = (arg->Type & VTK_PARSE_UNQUALIFIED_TYPE);
    argType = (argType & ~VTK_PARSE_REF);

    if (argType == VTK_PARSE_OBJECT_PTR)
      {
      fprintf(fp, "ap.GetVTKObject(temp%d, \"%s\")",
              i, arg->Class);
      }
    else if (argType == VTK_PARSE_OBJECT &&
             ((arg->Type & VTK_PARSE_REF) == 0 ||
              (arg->Type & VTK_PARSE_CONST) != 0))
      {
      fprintf(fp, "ap.GetSpecialObject(temp%d, pobj%d, \"%s\")",
              i, i, arg->Class);
      }
    else if (argType == VTK_PARSE_OBJECT)
      {
      fprintf(fp, "ap.GetSpecialObject(temp%d, \"%s\")",
              i, arg->Class);
      }
    else if (argType == VTK_PARSE_QOBJECT &&
             vtkWrapPython_IsQtEnum(arg->Class))
      {
      fprintf(fp, "ap.GetSIPEnumValue(temp%d, \"%s\")",
              i, arg->Class);
      }
    else if (argType == VTK_PARSE_QOBJECT ||
             argType == VTK_PARSE_QOBJECT_PTR)
      {
      fprintf(fp, "ap.GetSIPObject(temp%d, \"%s\")",
              i, arg->Class);
      }
    else if (argType == VTK_PARSE_FUNCTION)
      {
      fprintf(fp, "ap.GetFunction(temp%d)",
              i);
      break;
      }
    else if (argType == VTK_PARSE_VOID_PTR)
      {
      fprintf(fp, "ap.GetValue(temp%d)",
              i);
      }
    else if (argType == VTK_PARSE_BOOL)
      {
      fprintf(fp, "ap.GetValue(temp%d)",
              i);
      }
    else if (argType == VTK_PARSE_STRING ||
             argType == VTK_PARSE_UNICODE_STRING)
      {
      fprintf(fp, "ap.GetValue(temp%d)",
              i);
      }
    else if (argType == VTK_PARSE_CHAR_PTR)
      {
      fprintf(fp, "ap.GetValue(temp%d)",
              i);
      }
    else if (argType == VTK_PARSE_CHAR)
      {
      fprintf(fp, "ap.GetValue(temp%d)",
              i);
      }
    else if (argType == VTK_PARSE_FLOAT ||
             argType == VTK_PARSE_DOUBLE)
      {
      fprintf(fp, "ap.GetValue(temp%d)",
              i);
      }
    else if (argType == VTK_PARSE_SIGNED_CHAR ||
             argType == VTK_PARSE_UNSIGNED_CHAR ||
             argType == VTK_PARSE_SHORT ||
             argType == VTK_PARSE_UNSIGNED_SHORT ||
             argType == VTK_PARSE_INT ||
             argType == VTK_PARSE_UNSIGNED_INT ||
             argType == VTK_PARSE_LONG ||
             argType == VTK_PARSE_UNSIGNED_LONG ||
             argType == VTK_PARSE_LONG_LONG ||
             argType == VTK_PARSE_UNSIGNED_LONG_LONG ||
             argType == VTK_PARSE___INT64 ||
             argType == VTK_PARSE_UNSIGNED___INT64 ||
             argType == VTK_PARSE_ID_TYPE ||
             argType == VTK_PARSE_UNSIGNED_ID_TYPE ||
             argType == VTK_PARSE_SSIZE_T ||
             argType == VTK_PARSE_SIZE_T)
      {
      fprintf(fp, "ap.GetValue(temp%d)",
              i);
      }
    else if (arg->NumberOfDimensions > 1)
      {
      fprintf(fp, "ap.GetNArray(%.*stemp%d, %d, size%d)",
              (int)(arg->NumberOfDimensions-1), "**********",
              i, arg->NumberOfDimensions, i);
      }
    else if (arg->Count != 0)
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
  unsigned int returnType = VTK_PARSE_VOID;
  const char *returnClass = NULL;
  const char *deref = "";

  fprintf(fp,
          "    if (!PyErr_Occurred())\n"
          "      {\n");

  /* default type is VOID if val == NULL */
  if (val)
    {
    returnType = (val->Type & VTK_PARSE_UNQUALIFIED_TYPE);
    returnClass = val->Class;
    }

  if ((returnType & VTK_PARSE_INDIRECT) == VTK_PARSE_REF)
    {
    deref = "*";
    }

  if (returnType == VTK_PARSE_VOID)
    {
    fprintf(fp,
            "      result = ap.BuildNone();\n");
    }
  else if (returnType == VTK_PARSE_OBJECT_PTR)
    {
    fprintf(fp,
            "      result = ap.BuildVTKObject(tempr);\n");
    }
  else if (returnType == VTK_PARSE_OBJECT_REF)
    {
    fprintf(fp,
            "      result = ap.BuildSpecialObject(tempr, \"%s\");\n",
            returnClass);
    }
  else if (returnType == VTK_PARSE_OBJECT)
    {
    fprintf(fp,
            "      result = ap.BuildSpecialObject(&tempr, \"%s\");\n",
            returnClass);
    }
  else if ((returnType == VTK_PARSE_QOBJECT_PTR ||
            returnType == VTK_PARSE_QOBJECT_REF) &&
           !vtkWrapPython_IsQtEnum(returnClass))
    {
    fprintf(fp,
            "      result = ap.BuildSIPObject(tempr, \"%s\", false);\n",
            returnClass);
    }
  else if (returnType == VTK_PARSE_QOBJECT &&
           !vtkWrapPython_IsQtEnum(returnClass))
    {
    fprintf(fp,
            "      result = ap.BuildSIPObject(new %s(tempr), \"%s\", false);\n",
            returnClass, returnClass);
    }
  else if ((returnType == VTK_PARSE_QOBJECT ||
            returnType == VTK_PARSE_QOBJECT_REF) &&
           vtkWrapPython_IsQtEnum(returnClass))
    {
    fprintf(fp,
            "      result = ap.BuildSIPEnumValue(tempr, \"%s\");\n",
            returnClass);
    }
  else if (returnType == VTK_PARSE_CHAR_PTR)
    {
    fprintf(fp,
            "      result = ap.BuildValue(tempr);\n");
    }
  else if (returnType == VTK_PARSE_VOID_PTR)
    {
    fprintf(fp,
            "      result = ap.BuildValue(tempr);\n");
    }
  else if ((returnType & VTK_PARSE_INDIRECT) == VTK_PARSE_POINTER)
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
 * has a default value, and requiredArgs is set to the minimum number
 * of required arguments.
 *
 * If any new format characters are added here, they must also be
 * added to vtkPythonUtil::CheckArg() in vtkPythonUtil.cxx
 */

static char *vtkWrapPython_FormatString(
  FunctionInfo *currentFunction, int *requiredArgs)
{
  static char result[1024];
  size_t currPos = 0;
  ValueInfo *arg;
  unsigned int argtype;
  int i;

  *requiredArgs = currentFunction->NumberOfArguments;

  if (currentFunction->NumberOfArguments > 0 &&
      currentFunction->Arguments[0]->Type == VTK_PARSE_FUNCTION)
    {
    result[currPos++] = 'O';
    result[currPos] = '\0';
    *requiredArgs = 1;
    return result;
    }

  for (i = 0; i < currentFunction->NumberOfArguments; i++)
    {
    arg = currentFunction->Arguments[i];
    argtype = (arg->Type & VTK_PARSE_UNQUALIFIED_TYPE);

    if (arg->Value && arg->Count == 0 &&
        *requiredArgs == currentFunction->NumberOfArguments)
      {
      /* make all arguments optional after this one */
      result[currPos++] = '|';
      *requiredArgs = i;
      }

    /* add the format char to the string */
    result[currPos++] = vtkWrapPython_FormatChar(argtype);

    if (((argtype & VTK_PARSE_INDIRECT) == VTK_PARSE_POINTER ||
         (argtype & VTK_PARSE_INDIRECT) == VTK_PARSE_ARRAY) &&
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
  static char result[1024];
  size_t currPos = 0;
  ValueInfo *arg;
  unsigned int argtype;
  int i, j, k, requiredArgs;

  if (currentFunction->IsExplicit)
    {
    result[currPos++] = '-';
    }

  if (isvtkobjmethod)
    {
    result[currPos++] = '@';
    }

  strcpy(&result[currPos],
         vtkWrapPython_FormatString(currentFunction, &requiredArgs));
  currPos = strlen(result);

  if (currentFunction->NumberOfArguments > 0 &&
      currentFunction->Arguments[0]->Type == VTK_PARSE_FUNCTION)
    {
    strcpy(&result[currPos], " func");
    return result;
    }

  for (i = 0; i < currentFunction->NumberOfArguments; i++)
    {
    arg = currentFunction->Arguments[i];
    argtype = (arg->Type & VTK_PARSE_UNQUALIFIED_TYPE);

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
      strcpy(&result[currPos], arg->Class);
      currPos += strlen(arg->Class);
      }

    else if (arg->Count != 0)
      {
      result[currPos++] = ' ';
      result[currPos++] = '*';
      result[currPos++] = vtkWrapPython_FormatChar(argtype);
      if (arg->NumberOfDimensions > 1)
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

void vtkWrapPython_ArraySignature(
  struct vtkWPString *result, const char *classname,
  int ndim, const char **dims)
{
  int j, n;

  vtkWPString_Append(result, "(");
  n = (int)strtoul(dims[0], 0, 0);
  if (ndim > 1)
    {
    for (j = 0; j < n; j++)
      {
      if (j != 0) { vtkWPString_Append(result, ", "); }
      vtkWrapPython_ArraySignature(result, classname, ndim-1, dims+1);
      }
    }
  else
    {
    for (j = 0; j < n; j++)
      {
      if (j != 0) { vtkWPString_Append(result, ", "); }
      vtkWPString_Append(result, classname);
      }
    }
  vtkWPString_Append(result, ")");
}

static void vtkWrapPython_TypeSignature(
  struct vtkWPString *result, ValueInfo *arg)
{
  const char *classname = "";

  switch (arg->Type & VTK_PARSE_BASE_TYPE)
    {
    case VTK_PARSE_FLOAT:
    case VTK_PARSE_DOUBLE:
      classname = "float";
      break;
    case VTK_PARSE_UNSIGNED_CHAR:
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
    case VTK_PARSE_SIZE_T:
    case VTK_PARSE_SSIZE_T:
      classname = "int";
      break;
    case VTK_PARSE_CHAR:
      classname = "char";
      break;
    case VTK_PARSE_BOOL:
      classname = "bool";
      break;
    case VTK_PARSE_OBJECT:
    case VTK_PARSE_QOBJECT:
      classname = arg->Class;
      break;
    case VTK_PARSE_STRING:
      classname = "string";
      break;
    case VTK_PARSE_UNICODE_STRING:
      classname = "unicode";
      break;
    }

  switch (arg->Type & VTK_PARSE_UNQUALIFIED_TYPE)
    {
    case VTK_PARSE_VOID_PTR:
    case VTK_PARSE_CHAR_PTR:
      classname = "string";
      break;
    }

  if (arg->Count == -1)
    {
    vtkWPString_Append(result, "(");
    vtkWPString_Append(result, classname);
    vtkWPString_Append(result, ", ...)");
    }
  else if (arg->NumberOfDimensions)
    {
    vtkWrapPython_ArraySignature(result, classname,
      arg->NumberOfDimensions, arg->Dimensions);
    }
  else
    {
    vtkWPString_Append(result, classname);
    }
}

static const char *vtkWrapPython_PythonSignature(
  FunctionInfo *currentFunction)
{
  /* string is intentionally not freed until the program exits */
  static struct vtkWPString staticString = { NULL, 0, 0 };
  struct vtkWPString *result;
  ValueInfo *arg, *ret;
  int i;

  result = &staticString;
  result->len = 0;

  /* print out the name of the method */
  vtkWPString_Append(result, "V.");
  vtkWPString_Append(result, currentFunction->Name);

  /* print the arg list */
  vtkWPString_Append(result, "(");

  for (i = 0; i < currentFunction->NumberOfArguments; i++)
    {
    arg = currentFunction->Arguments[i];

    /* args after function arg are ignored */
    if (arg->Type == VTK_PARSE_FUNCTION)
      {
      vtkWPString_Append(result, "function");
      break;
      }

    if (i != 0)
      {
      vtkWPString_Append(result, ", ");
      }

    vtkWrapPython_TypeSignature(result, arg);
    }

  vtkWPString_Append(result, ")");

  /* if this is a void method, we are finished */
  /* otherwise, print "->" and the return type */
  ret = currentFunction->ReturnValue;
  if (ret && (ret->Type & VTK_PARSE_UNQUALIFIED_TYPE) != VTK_PARSE_VOID)
    {
    vtkWPString_Append(result, " -> ");

    vtkWrapPython_TypeSignature(result, ret);
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
  ValueInfo *val1;
  ValueInfo *val2;
  int vote1 = 0;
  int vote2 = 0;
  int occ1, occ2;
  unsigned int baseType1, baseType2;
  unsigned int unsigned1, unsigned2;
  unsigned int indirect1, indirect2;
  int i, n;
  int argmatch, allmatch;

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
          allmatch = 1;
          n = sig1->NumberOfArguments;
          for (i = 0; i < n; i++)
            {
            argmatch = 0;
            val1 = sig1->Arguments[i];
            val2 = sig2->Arguments[i];
            if (val1->NumberOfDimensions != val2->NumberOfDimensions)
              {
              vote1 = 0;
              vote2 = 0;
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
int vtkWrapPython_CountAllOccurrences(
  ClassInfo *data, FunctionInfo **wrappedFunctions, int n,
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
/* adjust number of args for SetFunction methods */
static int vtkWrapPython_GetNumberOfWrappedArgs(
  FunctionInfo *currentFunction)
{
  int totalArgs = currentFunction->NumberOfArguments;

  if (totalArgs > 0 &&
      (currentFunction->Arguments[0]->Type & VTK_PARSE_BASE_TYPE)
       == VTK_PARSE_FUNCTION)
    {
    totalArgs = 1;
    }

  return totalArgs;
}

/* -------------------------------------------------------------------- */
/* count the number of required arguments */
static int vtkWrapPython_GetNumberOfRequiredArgs(
  FunctionInfo *currentFunction, int totalArgs)
{
  int requiredArgs = 0;
  int i;

  for (i = 0; i < totalArgs; i++)
    {
    if (currentFunction->Arguments[i]->Value == NULL ||
        currentFunction->Arguments[i]->Count != 0)
      {
      requiredArgs = i+1;
      }
    }

  return requiredArgs;
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
  int fnum, int numberOfOccurrences, int *nmax, int *overlap)
{
  static int overloadMap[100];
  int totalArgs, requiredArgs;
  int occ, occCounter;
  FunctionInfo *theOccurrence;
  FunctionInfo *theFunc;
  int i;

  *nmax = 0;
  *overlap = 0;

  theFunc = wrappedFunctions[fnum];

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

    totalArgs = vtkWrapPython_GetNumberOfWrappedArgs(theOccurrence);
    requiredArgs = vtkWrapPython_GetNumberOfRequiredArgs(theOccurrence,
                                                         totalArgs);

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
  int i, j, n;
  int noneDone = 1;

  /* do nothing for SetVector macros */
  if (vtkWrapPython_IsSetVectorMethod(currentFunction))
    {
    return;
    }

  /* save arrays for args that are non-const */
  for (i = 0; i < currentFunction->NumberOfArguments; i++)
    {
    arg = currentFunction->Arguments[i];
    n = arg->NumberOfDimensions;
    if (n < 1 && arg->Count != 0)
      {
      n = 1;
      }

    if (arg->Count &&
        (arg->Type & VTK_PARSE_CONST) == 0)
      {
      noneDone = 0;

      fprintf(fp,
              "    ap.SaveArray(%.*stemp%d, %.*ssave%d, ",
              (n-1), asterisks, i, (n-1), asterisks, i);

      if (arg->NumberOfDimensions > 1)
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
  unsigned int argType;
  unsigned int returnType;
  int totalArgs;
  int is_pure_virtual;
  int is_static;
  int is_constructor;
  int i, k, n;

  totalArgs = vtkWrapPython_GetNumberOfWrappedArgs(currentFunction);

  is_static = currentFunction->IsStatic;
  is_pure_virtual = currentFunction->IsPureVirtual;
  is_constructor = vtkWrapPython_IsConstructor(data, currentFunction);

  returnType = VTK_PARSE_VOID;
  if (currentFunction->ReturnValue)
    {
    returnType = (currentFunction->ReturnValue->Type &
                  VTK_PARSE_UNQUALIFIED_TYPE);
    }

  /* for vtkobjects, do a bound call and an unbound call */
  n = 1;
  if (is_vtkobject && !is_static && !is_pure_virtual && !is_constructor)
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
    else if (is_static)
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
    else if (returnType == VTK_PARSE_VOID)
      {
      fprintf(fp,
              "    %s(",
              methodname);
      }
    else if ((returnType & VTK_PARSE_INDIRECT) == VTK_PARSE_REF)
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
      argType = (arg->Type & VTK_PARSE_UNQUALIFIED_TYPE);
      argType = (argType & ~VTK_PARSE_REF);

      if (argType == VTK_PARSE_FUNCTION)
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

      if (argType == VTK_PARSE_OBJECT ||
          (argType == VTK_PARSE_QOBJECT &&
           !vtkWrapPython_IsQtEnum(arg->Class)))
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
  int i, j, n;

  /* do nothing for SetVector macros */
  if (vtkWrapPython_IsSetVectorMethod(currentFunction))
    {
    return;
    }

  /* check array value change for args that are non-const */
  for (i = 0; i < currentFunction->NumberOfArguments; i++)
    {
    arg = currentFunction->Arguments[i];
    n = arg->NumberOfDimensions;
    if (n < 1 && arg->Count != 0)
      {
      n = 1;
      }

    if (((arg->Type & VTK_PARSE_CONST) == 0) &&
        ((arg->Type & VTK_PARSE_INDIRECT) == VTK_PARSE_REF) &&
        ((arg->Type & VTK_PARSE_BASE_TYPE) != VTK_PARSE_OBJECT) &&
        ((arg->Type & VTK_PARSE_BASE_TYPE) != VTK_PARSE_QOBJECT))
      {
      fprintf(fp,
              "    if (!PyErr_Occurred())\n"
              "      {\n"
              "      ap.SetArgValue(%d, temp%d);\n"
              "      }\n",
              i, i);
      }

    else if (arg->Count &&
             (arg->Type & VTK_PARSE_CONST) == 0)
      {
      fprintf(fp,
              "    if (!PyErr_Occurred() &&\n"
              "        ap.ArrayHasChanged(%.*stemp%d, %.*ssave%d, ",
              (n-1), asterisks, i, (n-1), asterisks, i);

      if (arg->NumberOfDimensions > 1)
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

      fprintf(fp, "))\n"
              "      {\n");

      if (arg->NumberOfDimensions > 1)
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
/* If any conversion constructors might have been used, then delete
 * the objects that were created */
static void vtkWrapPython_FreeConstructedObjects(
  FILE *fp, FunctionInfo *currentFunction)
{
  ValueInfo *arg;
  int i, j;

  /* check array value change for args that are non-const */
  j = 0;
  for (i = 0; i < currentFunction->NumberOfArguments; i++)
    {
    arg = currentFunction->Arguments[i];

    if ((arg->Type & VTK_PARSE_BASE_TYPE) == VTK_PARSE_OBJECT &&
        ((arg->Type & VTK_PARSE_INDIRECT) == 0 ||
         ((arg->Type & VTK_PARSE_INDIRECT) == VTK_PARSE_REF &&
          (arg->Type & VTK_PARSE_CONST) != 0)))
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
  FILE *fp, ClassInfo *data, int *overloadMap, int maxArgs,
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

    totalArgs = vtkWrapPython_GetNumberOfWrappedArgs(theOccurrence);
    requiredArgs = vtkWrapPython_GetNumberOfRequiredArgs(theOccurrence,
                                                         totalArgs);

    putInTable = 0;

    /* all conversion constructors must go into the table */
    if (vtkWrapPython_IsConstructor(data, theOccurrence) &&
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
  FILE *fp, ClassInfo *data, FunctionInfo *currentFunction,
  int numberOfOccurrences, int *overloadMap, int maxArgs, int all_legacy)
{
  int overlap = 0;
  int occ;
  int i;
  int foundOne;

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
          "  Py%s_%s(PyObject *self, PyObject *args)\n"
          "{\n",
           data->Name, currentFunction->Name);

  if (overlap)
    {
    fprintf(fp,
          "  PyMethodDef *methods = Py%s_%s_Methods;\n",
           data->Name, currentFunction->Name);
    }

  fprintf(fp,
          "  switch (PyTuple_GET_SIZE(args))\n"
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
            "      return vtkPythonUtil::CallOverloadedMethod(methods, self, args);\n");
    }

  fprintf(fp,
          "    }\n"
          "\n");

  fprintf(fp,
          "  vtkPythonArgs::ArgCountError(\"%.200s\");\n",
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
  vtkWrapPython_DiscoverPointerCounts(fp, data, hinfo);

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
        data, wrappedFunctions, numberOfWrappedFunctions, fnum,
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
        fnum, numberOfOccurrences, &maxArgs, &overlap);

      if (overlap || do_constructors)
        {
        /* output the method table for the signatures */
        vtkWrapPython_OverloadMethodDef(
          fp, data, overloadMap, maxArgs,
          wrappedFunctions, numberOfWrappedFunctions,
          fnum, numberOfOccurrences, is_vtkobject, all_legacy);
        }

      if (numberOfOccurrences > 1)
        {
        /* declare a "master method" to choose among the overloads */
        vtkWrapPython_OverloadMasterMethod(
          fp, data, theFunc, numberOfOccurrences, overloadMap, maxArgs,
          all_legacy);
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
          "  {NULL, NULL, 0, NULL}\n"
          "};\n"
          "\n");
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
static int vtkWrapPython_IsSetVectorMethod(
  FunctionInfo *currentFunction)
{
  if (currentFunction->Macro &&
      strncmp(currentFunction->Macro, "vtkSetVector", 12) == 0)
    {
    return 1;
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

  int i, j;
  int args_ok = 1;
  unsigned int argType = 0;
  const char *argClass = NULL;
  unsigned int baseType = 0;
  unsigned int returnType = 0;
  const char *returnClass = NULL;

  /* some functions will not get wrapped no matter what else,
     and some really common functions will appear only in vtkObjectPython */
  if (currentFunction->IsOperator ||
      currentFunction->Access != VTK_ACCESS_PUBLIC ||
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

  /* function pointer arguments for callbacks */
  if (currentFunction->NumberOfArguments == 2 &&
      currentFunction->Arguments[0]->Type == VTK_PARSE_FUNCTION &&
      currentFunction->Arguments[1]->Type == VTK_PARSE_VOID_PTR &&
      currentFunction->ReturnValue &&
      (currentFunction->ReturnValue->Type & VTK_PARSE_UNQUALIFIED_TYPE)
      == VTK_PARSE_VOID)
    {
    return 1;
    }

  /* check to see if we can handle all the args */
  for (i = 0; i < currentFunction->NumberOfArguments; i++)
    {
    argClass = currentFunction->Arguments[i]->Class;
    argType = (currentFunction->Arguments[i]->Type &
               VTK_PARSE_UNQUALIFIED_TYPE);
    baseType = (argType & VTK_PARSE_BASE_TYPE);

    for (j = 0; supported_types[j] != 0; j++)
      {
      if (baseType == supported_types[j]) { break; }
      }
    if (supported_types[j] == 0)
      {
      args_ok = 0;
      }

    if (((argType & VTK_PARSE_INDIRECT) != VTK_PARSE_POINTER) &&
        ((argType & VTK_PARSE_INDIRECT) != VTK_PARSE_ARRAY) &&
        ((argType & VTK_PARSE_INDIRECT) != VTK_PARSE_REF) &&
        ((argType & VTK_PARSE_INDIRECT) != 0)) args_ok = 0;

    if (((argType & VTK_PARSE_INDIRECT) == VTK_PARSE_REF) &&
        (currentFunction->Arguments[i]->Type & VTK_PARSE_CONST) == 0)
      {
      if (baseType != VTK_PARSE_OBJECT &&
          baseType != VTK_PARSE_STRING &&
          baseType != VTK_PARSE_UNICODE_STRING &&
          baseType != VTK_PARSE_BOOL &&
          baseType != VTK_PARSE_FLOAT &&
          baseType != VTK_PARSE_DOUBLE &&
          baseType != VTK_PARSE_SIGNED_CHAR &&
          baseType != VTK_PARSE_SHORT &&
          baseType != VTK_PARSE_INT &&
          baseType != VTK_PARSE_LONG &&
          baseType != VTK_PARSE_LONG_LONG &&
          baseType != VTK_PARSE___INT64 &&
          baseType != VTK_PARSE_UNSIGNED_CHAR &&
          baseType != VTK_PARSE_UNSIGNED_SHORT &&
          baseType != VTK_PARSE_UNSIGNED_INT &&
          baseType != VTK_PARSE_UNSIGNED_LONG &&
          baseType != VTK_PARSE_UNSIGNED_LONG_LONG &&
          baseType != VTK_PARSE___INT64) args_ok = 0;
      }

    if (argType == VTK_PARSE_CHAR_PTR &&
        currentFunction->Arguments[i]->Count > 0) args_ok = 0;

    if ((argType == VTK_PARSE_OBJECT_REF ||
         argType == VTK_PARSE_OBJECT) &&
        !vtkWrapPython_IsSpecialType(hinfo, argClass)) args_ok = 0;

    if (argType == VTK_PARSE_OBJECT_PTR &&
        !vtkWrapPython_IsObjectBaseType(hinfo, argClass)) args_ok = 0;

    if ((argType & VTK_PARSE_INDIRECT) == VTK_PARSE_ARRAY)
      {
      if (baseType != VTK_PARSE_BOOL &&
          baseType != VTK_PARSE_FLOAT &&
          baseType != VTK_PARSE_DOUBLE &&
          baseType != VTK_PARSE_SIGNED_CHAR &&
          baseType != VTK_PARSE_SHORT &&
          baseType != VTK_PARSE_INT &&
          baseType != VTK_PARSE_LONG &&
          baseType != VTK_PARSE_LONG_LONG &&
          baseType != VTK_PARSE___INT64 &&
          baseType != VTK_PARSE_UNSIGNED_CHAR &&
          baseType != VTK_PARSE_UNSIGNED_SHORT &&
          baseType != VTK_PARSE_UNSIGNED_INT &&
          baseType != VTK_PARSE_UNSIGNED_LONG &&
          baseType != VTK_PARSE_UNSIGNED_LONG_LONG &&
          baseType != VTK_PARSE_UNSIGNED___INT64) args_ok = 0;

      for (j = 0; j < currentFunction->Arguments[i]->NumberOfDimensions; j++)
        {
        if (strtoul(currentFunction->Arguments[i]->Dimensions[j], 0, 0) == 0)
          {
          args_ok = 0;
          }
        }
      }

    if (argType == VTK_PARSE_SSIZE_T_PTR) args_ok = 0;
    if (argType == VTK_PARSE_SIZE_T_PTR) args_ok = 0;

    if (argType == VTK_PARSE_STRING_PTR) args_ok = 0;
    if (argType == VTK_PARSE_UNICODE_STRING_PTR) args_ok = 0;
    }

  /* make sure we have all the info we need for array arguments */
  for (i = 0; i < currentFunction->NumberOfArguments; i++)
    {
    argType = (currentFunction->Arguments[i]->Type &
               VTK_PARSE_UNQUALIFIED_TYPE);

    if (((argType & VTK_PARSE_INDIRECT) == VTK_PARSE_POINTER) &&
        (currentFunction->Arguments[i]->Count == 0) &&
        (argType != VTK_PARSE_OBJECT_PTR) &&
        (argType != VTK_PARSE_QOBJECT_PTR) &&
        (argType != VTK_PARSE_CHAR_PTR) &&
        (argType != VTK_PARSE_VOID_PTR)) args_ok = 0;
    }

  /* check the return type */
  returnType = VTK_PARSE_VOID;
  returnClass = "void";
  if (currentFunction->ReturnValue)
    {
    returnType = (currentFunction->ReturnValue->Type &
                  VTK_PARSE_UNQUALIFIED_TYPE);
    returnClass = currentFunction->ReturnValue->Class;
    }
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

  if (currentFunction->ReturnValue &&
      currentFunction->ReturnValue->NumberOfDimensions > 1) args_ok = 0;

  if ((returnType == VTK_PARSE_OBJECT_REF ||
       returnType == VTK_PARSE_OBJECT) &&
      !vtkWrapPython_IsSpecialType(hinfo, returnClass)) args_ok = 0;

  if (returnType == VTK_PARSE_OBJECT_PTR &&
      !vtkWrapPython_IsObjectBaseType(hinfo, returnClass)) args_ok = 0;

#if PY_VERSION_HEX < 0x02030000
  /* eliminate "unsigned char *" and "unsigned short *" */
  if (returnType == VTK_PARSE_UNSIGNED_CHAR_PTR) args_ok = 0;
  if (returnType == VTK_PARSE_UNSIGNED_INT_PTR) args_ok = 0;
  if (returnType == VTK_PARSE_UNSIGNED_LONG_LONG_PTR) args_ok = 0;
  if (returnType == VTK_PARSE_UNSIGNED___INT64_PTR) args_ok = 0;
  if (returnType == VTK_PARSE_UNSIGNED_SHORT_PTR) args_ok = 0;
  if (returnType == VTK_PARSE_UNSIGNED_LONG_PTR) args_ok = 0;
#endif

  if (returnType == VTK_PARSE_SSIZE_T_PTR) args_ok = 0;
  if (returnType == VTK_PARSE_SIZE_T_PTR) args_ok = 0;

  if (returnType == VTK_PARSE_STRING_PTR) args_ok = 0;
  if (returnType == VTK_PARSE_UNICODE_STRING_PTR) args_ok = 0;

  /* if we need a return type hint make sure we have one */
  if (args_ok)
    {
    switch (returnType)
      {
      case VTK_PARSE_BOOL_PTR:
      case VTK_PARSE_FLOAT_PTR:
      case VTK_PARSE_DOUBLE_PTR:
      case VTK_PARSE_SIGNED_CHAR_PTR:
      case VTK_PARSE_ID_TYPE_PTR:
      case VTK_PARSE_LONG_LONG_PTR:
      case VTK_PARSE___INT64_PTR:
      case VTK_PARSE_INT_PTR:
      case VTK_PARSE_SHORT_PTR:
      case VTK_PARSE_LONG_PTR:
      case VTK_PARSE_UNSIGNED_CHAR_PTR:
      case VTK_PARSE_UNSIGNED_ID_TYPE_PTR:
      case VTK_PARSE_UNSIGNED_LONG_LONG_PTR:
      case VTK_PARSE_UNSIGNED___INT64_PTR:
      case VTK_PARSE_UNSIGNED_INT_PTR:
      case VTK_PARSE_UNSIGNED_SHORT_PTR:
      case VTK_PARSE_UNSIGNED_LONG_PTR:
        args_ok = (currentFunction->ReturnValue->Count != 0);
      break;
      }
    }

  /* char * is always treated as a string, not an array */
  if (returnType == VTK_PARSE_CHAR_PTR &&
      currentFunction->ReturnValue->Count > 0) args_ok = 0;

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
    if (vtkWrapPython_IsClassWrapped(hinfo, data->SuperClasses[j]))
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
      classname = "void";
      aType = VTK_PARSE_VOID;
      if (currentFunction->ReturnValue)
        {
        classname = currentFunction->ReturnValue->Class;
        aType = currentFunction->ReturnValue->Type;
        }
      m = currentFunction->NumberOfArguments;
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
            "    if (!PyErr_Occurred())\n"
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
/* This sets the arg Count to -1 for vtkDataArray methods where the
 * array size is equal to GetNumberOfComponents.  Maybe, eventually,
 * this can be automated using hints. */
void vtkWrapPython_DiscoverPointerCounts(
  FILE *fp, ClassInfo *data, HierarchyInfo *hinfo)
{
  int i;
  FunctionInfo *theFunc;

  for (i = 0; i < data->NumberOfFunctions; i++)
    {
    theFunc = data->Functions[i];

    /* add hints for array GetTuple methods */
    if (vtkWrapPython_IsTypeOf(hinfo, data->Name, "vtkDataArray"))
      {
      if ((strcmp(theFunc->Name, "GetTuple") == 0 ||
           strcmp(theFunc->Name, "GetTupleValue") == 0) &&
          theFunc->ReturnValue && theFunc->ReturnValue->Count == 0 &&
          theFunc->NumberOfArguments == 1 &&
          theFunc->Arguments[0]->Type == VTK_PARSE_ID_TYPE &&
          (theFunc->ReturnValue->Type & VTK_PARSE_BASE_TYPE) != VTK_PARSE_CHAR)
        {
        theFunc->ReturnValue->Count = -1;
        }
      else if ((strcmp(theFunc->Name, "SetTuple") == 0 ||
                strcmp(theFunc->Name, "SetTupleValue") == 0 ||
                strcmp(theFunc->Name, "GetTuple") == 0 ||
                strcmp(theFunc->Name, "GetTupleValue") == 0 ||
                strcmp(theFunc->Name, "InsertTuple") == 0 ||
                strcmp(theFunc->Name, "InsertTupleValue") == 0) &&
               theFunc->NumberOfArguments == 2 &&
               theFunc->Arguments[0]->Type == VTK_PARSE_ID_TYPE &&
               theFunc->Arguments[1]->Count == 0 &&
               (theFunc->Arguments[1]->Type & VTK_PARSE_BASE_TYPE) !=
                  VTK_PARSE_CHAR)
        {
        theFunc->Arguments[1]->Count = -1;
        }
      else if ((strcmp(theFunc->Name, "InsertNextTuple") == 0 ||
                strcmp(theFunc->Name, "InsertNextTupleValue") == 0) &&
               theFunc->NumberOfArguments == 1 &&
               theFunc->Arguments[0]->Count == 0 &&
               (theFunc->Arguments[0]->Type & VTK_PARSE_BASE_TYPE)
                != VTK_PARSE_CHAR)
        {
        theFunc->Arguments[0]->Count = -1;
        }
      }
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
    if (vtkWrapPython_IsObjectBaseType(hinfo, data->SuperClasses[i]))
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
/* Expand all typedef types that are used in function arguments */
static void vtkWrapPython_ExpandTypedefs(
  ClassInfo *data, HierarchyInfo *hinfo)
{
  int i, j, n;
  FunctionInfo *funcInfo;

  n = data->NumberOfFunctions;
  for (i = 0; i < n; i++)
    {
    funcInfo = data->Functions[i];
    if (funcInfo->Access == VTK_ACCESS_PUBLIC)
      {
      for (j = 0; j < funcInfo->NumberOfArguments; j++)
        {
        vtkParseHierarchy_ExpandTypedefs(
          hinfo, funcInfo->Arguments[j], data->Name);
        }
      if (funcInfo->ReturnValue)
        {
        vtkParseHierarchy_ExpandTypedefs(
          hinfo, funcInfo->ReturnValue, data->Name);
        }
      }
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
    vtkWrapPython_ExpandTypedefs(data, hinfo);
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
      if (vtkWrapPython_IsClassWrapped(hinfo, data->SuperClasses[i]) &&
          vtkWrapPython_IsObjectBaseType(hinfo, data->SuperClasses[i]))
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
