/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParseJava.c

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "vtkParse.h"
#include "vtkParseMain.h"
#include "vtkParseHierarchy.h"

HierarchyInfo *hierarchyInfo = NULL;
int numberOfWrappedFunctions = 0;
FunctionInfo *wrappedFunctions[1000];
extern FunctionInfo *currentFunction;

void output_temp(FILE *fp,int i)
{
  unsigned int aType =
    (currentFunction->ArgTypes[i] & VTK_PARSE_UNQUALIFIED_TYPE);

  /* ignore void */
  if (aType == VTK_PARSE_VOID)
    {
    return;
    }

  if (currentFunction->ArgTypes[i] == VTK_PARSE_FUNCTION)
    {
    fprintf(fp,"Object id0, String id1");
    return;
    }

  if ((aType == VTK_PARSE_CHAR_PTR) ||
      (aType == VTK_PARSE_STRING) ||
      (aType == VTK_PARSE_STRING_REF))
    {
    fprintf(fp,"String ");
    }
  else
    {
    switch ((aType & VTK_PARSE_BASE_TYPE) & ~VTK_PARSE_UNSIGNED)
      {
      case VTK_PARSE_FLOAT:       fprintf(fp,"double "); break;
      case VTK_PARSE_DOUBLE:      fprintf(fp,"double "); break;
      case VTK_PARSE_INT:         fprintf(fp,"int "); break;
      case VTK_PARSE_SHORT:       fprintf(fp,"int "); break;
      case VTK_PARSE_LONG:        fprintf(fp,"int "); break;
      case VTK_PARSE_ID_TYPE:     fprintf(fp,"int "); break;
      case VTK_PARSE_LONG_LONG:   fprintf(fp,"int "); break;
      case VTK_PARSE___INT64:     fprintf(fp,"int "); break;
      case VTK_PARSE_SIGNED_CHAR: fprintf(fp,"char "); break;
      case VTK_PARSE_BOOL:        fprintf(fp,"boolean "); break;
      case VTK_PARSE_VOID:        fprintf(fp,"void "); break;
      case VTK_PARSE_CHAR:        fprintf(fp,"char "); break;
      case VTK_PARSE_OBJECT:  fprintf(fp,"%s ",currentFunction->ArgClasses[i]); break;
      case VTK_PARSE_UNKNOWN: return;
      }
    }

  fprintf(fp,"id%i",i);
  if (((aType & VTK_PARSE_INDIRECT) == VTK_PARSE_POINTER) &&
      (aType != VTK_PARSE_CHAR_PTR) &&
      (aType != VTK_PARSE_OBJECT_PTR))
    {
    fprintf(fp,"[]");
    }
}

void return_result(FILE *fp)
{
  unsigned int rType =
    (currentFunction->ReturnType & VTK_PARSE_UNQUALIFIED_TYPE);

  switch (rType)
    {
    case VTK_PARSE_FLOAT:
      fprintf(fp,"double ");
      break;
    case VTK_PARSE_VOID:
      fprintf(fp,"void ");
      break;
    case VTK_PARSE_CHAR:
      fprintf(fp,"char ");
      break;
    case VTK_PARSE_DOUBLE:
      fprintf(fp,"double ");
      break;
    case VTK_PARSE_INT:
    case VTK_PARSE_SHORT:
    case VTK_PARSE_LONG:
    case VTK_PARSE_ID_TYPE:
    case VTK_PARSE_LONG_LONG:
    case VTK_PARSE___INT64:
    case VTK_PARSE_SIGNED_CHAR:
    case VTK_PARSE_UNSIGNED_CHAR:
    case VTK_PARSE_UNSIGNED_INT:
    case VTK_PARSE_UNSIGNED_SHORT:
    case VTK_PARSE_UNSIGNED_LONG:
    case VTK_PARSE_UNSIGNED_ID_TYPE:
    case VTK_PARSE_UNSIGNED_LONG_LONG:
    case VTK_PARSE_UNSIGNED___INT64:
      fprintf(fp,"int ");
      break;
    case VTK_PARSE_BOOL:
      fprintf(fp,"boolean ");
      break;
    case VTK_PARSE_CHAR_PTR:
    case VTK_PARSE_STRING:
    case VTK_PARSE_STRING_REF:
      fprintf(fp,"String ");
      break;
    case VTK_PARSE_OBJECT_PTR:
      fprintf(fp,"%s ",currentFunction->ReturnClass);
      break;

      /* handle functions returning vectors */
      /* this is done by looking them up in a hint file */
    case VTK_PARSE_FLOAT_PTR:
    case VTK_PARSE_DOUBLE_PTR:
      fprintf(fp,"double[] ");
      break;
    case VTK_PARSE_UNSIGNED_CHAR_PTR:
      fprintf(fp,"byte[] ");
      break;
    case VTK_PARSE_INT_PTR:
    case VTK_PARSE_SHORT_PTR:
    case VTK_PARSE_LONG_PTR:
    case VTK_PARSE_ID_TYPE_PTR:
    case VTK_PARSE_LONG_LONG_PTR:
    case VTK_PARSE___INT64_PTR:
    case VTK_PARSE_SIGNED_CHAR_PTR:
    case VTK_PARSE_UNSIGNED_INT_PTR:
    case VTK_PARSE_UNSIGNED_SHORT_PTR:
    case VTK_PARSE_UNSIGNED_LONG_PTR:
    case VTK_PARSE_UNSIGNED_ID_TYPE_PTR:
    case VTK_PARSE_UNSIGNED_LONG_LONG_PTR:
    case VTK_PARSE_UNSIGNED___INT64_PTR:
      fprintf(fp,"int[]  ");
      break;
    case VTK_PARSE_BOOL_PTR:
      fprintf(fp,"boolean[]  ");
      break;
    }
}

/* same as return_result except we return a long (the c++ pointer) rather than an object */
void return_result_native(FILE *fp)
{
  unsigned int rType =
    (currentFunction->ReturnType & VTK_PARSE_UNQUALIFIED_TYPE);

  switch (rType)
    {
    case VTK_PARSE_FLOAT:
      fprintf(fp,"double ");
      break;
    case VTK_PARSE_VOID:
      fprintf(fp,"void ");
      break;
    case VTK_PARSE_CHAR:
      fprintf(fp,"char ");
      break;
    case VTK_PARSE_DOUBLE:
      fprintf(fp,"double ");
      break;
    case VTK_PARSE_INT:
    case VTK_PARSE_SHORT:
    case VTK_PARSE_LONG:
    case VTK_PARSE_ID_TYPE:
    case VTK_PARSE_LONG_LONG:
    case VTK_PARSE___INT64:
    case VTK_PARSE_SIGNED_CHAR:
    case VTK_PARSE_UNSIGNED_CHAR:
    case VTK_PARSE_UNSIGNED_INT:
    case VTK_PARSE_UNSIGNED_SHORT:
    case VTK_PARSE_UNSIGNED_LONG:
    case VTK_PARSE_UNSIGNED_ID_TYPE:
    case VTK_PARSE_UNSIGNED_LONG_LONG:
    case VTK_PARSE_UNSIGNED___INT64:
      fprintf(fp,"int ");
      break;
    case VTK_PARSE_BOOL:
      fprintf(fp,"boolean ");
      break;
    case VTK_PARSE_CHAR_PTR:
    case VTK_PARSE_STRING:
    case VTK_PARSE_STRING_REF:
      fprintf(fp,"String ");
      break;
    case VTK_PARSE_OBJECT_PTR:
      fprintf(fp,"long ");
      break;

      /* handle functions returning vectors */
      /* this is done by looking them up in a hint file */
    case VTK_PARSE_FLOAT_PTR:
    case VTK_PARSE_DOUBLE_PTR:
      fprintf(fp,"double[] ");
      break;
    case VTK_PARSE_UNSIGNED_CHAR_PTR:
      fprintf(fp,"byte[] ");
      break;
    case VTK_PARSE_INT_PTR:
    case VTK_PARSE_SHORT_PTR:
    case VTK_PARSE_LONG_PTR:
    case VTK_PARSE_ID_TYPE_PTR:
    case VTK_PARSE_LONG_LONG_PTR:
    case VTK_PARSE___INT64_PTR:
    case VTK_PARSE_SIGNED_CHAR_PTR:
    case VTK_PARSE_UNSIGNED_INT_PTR:
    case VTK_PARSE_UNSIGNED_SHORT_PTR:
    case VTK_PARSE_UNSIGNED_LONG_PTR:
    case VTK_PARSE_UNSIGNED_ID_TYPE_PTR:
    case VTK_PARSE_UNSIGNED_LONG_LONG_PTR:
    case VTK_PARSE_UNSIGNED___INT64_PTR:
      fprintf(fp,"int[]  ");
      break;
    case VTK_PARSE_BOOL_PTR:
      fprintf(fp,"boolean[]  ");
      break;
    }
}

/* Check to see if two types will map to the same Java type,
 * return 1 if type1 should take precedence,
 * return 2 if type2 should take precedence,
 * return 0 if the types do not map to the same type */
static int CheckMatch(
  unsigned int type1, unsigned int type2, const char *c1, const char *c2)
{
  static unsigned int floatTypes[] = {
    VTK_PARSE_DOUBLE, VTK_PARSE_FLOAT, 0 };

  static unsigned int intTypes[] = {
    VTK_PARSE_UNSIGNED_LONG_LONG, VTK_PARSE_UNSIGNED___INT64,
    VTK_PARSE_LONG_LONG, VTK_PARSE___INT64, VTK_PARSE_ID_TYPE,
    VTK_PARSE_UNSIGNED_LONG, VTK_PARSE_LONG,
    VTK_PARSE_UNSIGNED_INT, VTK_PARSE_INT,
    VTK_PARSE_UNSIGNED_SHORT, VTK_PARSE_SHORT,
    VTK_PARSE_UNSIGNED_CHAR, VTK_PARSE_SIGNED_CHAR, 0 };

  static unsigned int stringTypes[] = {
    VTK_PARSE_CHAR_PTR, VTK_PARSE_STRING_REF, VTK_PARSE_STRING, 0 };

  static unsigned int *numericTypes[] = { floatTypes, intTypes, 0 };

  int i, j;
  int hit1, hit2;

  if ((type1 & VTK_PARSE_UNQUALIFIED_TYPE) ==
      (type2 & VTK_PARSE_UNQUALIFIED_TYPE))
    {
    if ((type1 & VTK_PARSE_BASE_TYPE) == VTK_PARSE_OBJECT)
      {
      if (strcmp(c1, c2) == 0)
        {
        return 1;
        }
      return 0;
      }
    else
      {
      return 1;
      }
    }

  for (i = 0; numericTypes[i]; i++)
    {
    hit1 = 0;
    hit2 = 0;
    for (j = 0; numericTypes[i][j]; j++)
      {
      if ((type1 & VTK_PARSE_BASE_TYPE) == numericTypes[i][j])
        {
        hit1 = j+1;
        }
      if ((type2 & VTK_PARSE_BASE_TYPE) == numericTypes[i][j])
        {
        hit2 = j+1;
        }
      }
    if (hit1 && hit2 &&
        (type1 & VTK_PARSE_INDIRECT) == (type2 & VTK_PARSE_INDIRECT))
      {
      if (hit1 < hit2)
        {
        return 1;
        }
      else
        {
        return 2;
        }
      }
    }

  hit1 = 0;
  hit2 = 0;
  for (j = 0; stringTypes[j]; j++)
    {
    if ((type1 & VTK_PARSE_UNQUALIFIED_TYPE) == stringTypes[j])
      {
      hit1 = j+1;
      }
    if ((type2 & VTK_PARSE_UNQUALIFIED_TYPE) == stringTypes[j])
      {
      hit2 = j+1;
      }
    }
  if (hit1 && hit2)
    {
    if (hit1 < hit2)
      {
      return 1;
      }
    else
      {
      return 2;
      }
    }

  return 0;
}

/* have we done one of these yet */
int DoneOne()
{
  int i,j;
  int match;
  FunctionInfo *fi;

  for (i = 0; i < numberOfWrappedFunctions; i++)
    {
    fi = wrappedFunctions[i];

    if ((!strcmp(fi->Name,currentFunction->Name))
        &&(fi->NumberOfArguments == currentFunction->NumberOfArguments))
      {
      match = 1;
      for (j = 0; j < fi->NumberOfArguments; j++)
        {
        if (!CheckMatch(currentFunction->ArgTypes[j], fi->ArgTypes[j],
                        currentFunction->ArgClasses[j],fi->ArgClasses[j]))
          {
          match = 0;
          }
        }
      if (!CheckMatch(currentFunction->ReturnType, fi->ReturnType,
                      currentFunction->ReturnClass, fi->ReturnClass))
        {
        match = 0;
        }
      if (match) return 1;
      }
    }
  return 0;
}

void HandleDataReader(FILE *fp)
{
    fprintf(fp,"\n  private native void ");
    fprintf(fp,"%s_%i(byte id0[],int id1);\n",
            currentFunction->Name,numberOfWrappedFunctions);
    fprintf(fp,"\n  public void ");
    fprintf(fp,"%s(byte id0[],int id1)\n",currentFunction->Name);
    fprintf(fp,"    { %s_%i(id0,id1); }\n",
            currentFunction->Name,numberOfWrappedFunctions);
}

void HandleDataArray(FILE *fp, ClassInfo *data)
{
  const char *type = 0;

  if (!strcmp("vtkCharArray",data->Name) )
    {
    type = "char";
    }
  else if (!strcmp("vtkDoubleArray",data->Name) )
    {
    type = "double";
    }
  else if (!strcmp("vtkFloatArray",data->Name) )
    {
    type = "float";
    }
  else if (!strcmp("vtkIntArray",data->Name) )
    {
    type = "int";
    }
  else if (!strcmp("vtkLongArray",data->Name) )
    {
    type = "long";
    }
  else if (!strcmp("vtkShortArray",data->Name) )
    {
    type = "short";
    }
  else if (!strcmp("vtkUnsignedCharArray",data->Name) )
    {
    type = "byte";
    }
  else if (!strcmp("vtkUnsignedIntArray",data->Name) )
    {
    type = "int";
    }
  else if (!strcmp("vtkUnsignedLongArray",data->Name) )
    {
    type = "long";
    }
  else if (!strcmp("vtkUnsignedShortArray",data->Name) )
    {
    type = "short";
    }
  else
    {
    return;
    }

  fprintf(fp,"\n");
  fprintf(fp,"  private native %s[] GetJavaArray_0();\n", type);
  fprintf(fp,"  public %s[] GetJavaArray()\n", type);
  fprintf(fp,"    { return GetJavaArray_0(); }\n");
  fprintf(fp,"\n");
  fprintf(fp,"  private native void SetJavaArray_0(%s[] arr);\n", type);
  fprintf(fp,"  public void SetJavaArray(%s[] arr)\n", type);
  fprintf(fp,"    { SetJavaArray_0(arr); }\n");
}

static int isClassWrapped(const char *classname)
{
  HierarchyEntry *entry;

  if (hierarchyInfo)
    {
    entry = vtkParseHierarchy_FindEntry(hierarchyInfo, classname);

    if (entry == 0 ||
        vtkParseHierarchy_GetProperty(entry, "WRAP_EXCLUDE") ||
        !vtkParseHierarchy_IsTypeOf(hierarchyInfo, entry, "vtkObjectBase"))
      {
      return 0;
      }
    }

  return 1;
}

int checkFunctionSignature(ClassInfo *data)
{
  static unsigned int supported_types[] = {
    VTK_PARSE_VOID, VTK_PARSE_BOOL, VTK_PARSE_FLOAT, VTK_PARSE_DOUBLE,
    VTK_PARSE_CHAR, VTK_PARSE_UNSIGNED_CHAR, VTK_PARSE_SIGNED_CHAR,
    VTK_PARSE_INT, VTK_PARSE_UNSIGNED_INT,
    VTK_PARSE_SHORT, VTK_PARSE_UNSIGNED_SHORT,
    VTK_PARSE_LONG, VTK_PARSE_UNSIGNED_LONG,
    VTK_PARSE_ID_TYPE, VTK_PARSE_UNSIGNED_ID_TYPE,
    VTK_PARSE_LONG_LONG, VTK_PARSE_UNSIGNED_LONG_LONG,
    VTK_PARSE___INT64, VTK_PARSE_UNSIGNED___INT64,
    VTK_PARSE_OBJECT, VTK_PARSE_STRING,
    0
  };

  int i, j;
  int args_ok = 1;
  unsigned int rType =
    (currentFunction->ReturnType & VTK_PARSE_UNQUALIFIED_TYPE);
  unsigned int aType = 0;
  unsigned int baseType = 0;

  /* some functions will not get wrapped no matter what else */
  if (currentFunction->IsOperator ||
      currentFunction->ArrayFailure ||
      !currentFunction->IsPublic ||
      !currentFunction->Name)
    {
    return 0;
    }

  /* NewInstance and SafeDownCast can not be wrapped because it is a
     (non-virtual) method which returns a pointer of the same type as
     the current pointer. Since all methods are virtual in Java, this
     looks like polymorphic return type.  */
  if (!strcmp("NewInstance",currentFunction->Name))
    {
    return 0;
    }

  if (!strcmp("SafeDownCast",currentFunction->Name))
    {
    return 0;
    }

  /* The GetInput() in vtkMapper cannot be overriden with a
   * different return type, Java doesn't allow this */
  if (strcmp(data->Name, "vtkMapper") == 0 &&
      strcmp(currentFunction->Name, "GetInput") == 0)
    {
    return 0;
    }

  /* function pointer arguments for callbacks */
  if (currentFunction->NumberOfArguments == 2 &&
      currentFunction->ArgTypes[0] == VTK_PARSE_FUNCTION &&
      currentFunction->ArgTypes[1] == VTK_PARSE_VOID_PTR &&
      rType == VTK_PARSE_VOID)
    {
    return 1;
    }

  /* check to see if we can handle the args */
  for (i = 0; i < currentFunction->NumberOfArguments; i++)
    {
    aType = (currentFunction->ArgTypes[i] & VTK_PARSE_UNQUALIFIED_TYPE);
    baseType = (aType & VTK_PARSE_BASE_TYPE);

    for (j = 0; supported_types[j] != 0; j++)
      {
      if (baseType == supported_types[j]) { break; }
      }
    if (supported_types[j] == 0)
      {
      args_ok = 0;
      }

    if (baseType == VTK_PARSE_OBJECT)
      {
      if ((aType & VTK_PARSE_INDIRECT) != VTK_PARSE_POINTER)
        {
        args_ok = 0;
        }
      else if (!isClassWrapped(currentFunction->ArgClasses[i]))
        {
        args_ok = 0;
        }
      }

    if (aType == VTK_PARSE_OBJECT) args_ok = 0;
    if (((aType & VTK_PARSE_INDIRECT) != VTK_PARSE_POINTER) &&
        ((aType & VTK_PARSE_INDIRECT) != 0) &&
        (aType != VTK_PARSE_STRING_REF)) args_ok = 0;
    if (aType == VTK_PARSE_STRING_PTR) args_ok = 0;
    if (aType == VTK_PARSE_UNSIGNED_CHAR_PTR) args_ok = 0;
    if (aType == VTK_PARSE_UNSIGNED_INT_PTR) args_ok = 0;
    if (aType == VTK_PARSE_UNSIGNED_SHORT_PTR) args_ok = 0;
    if (aType == VTK_PARSE_UNSIGNED_LONG_PTR) args_ok = 0;
    if (aType == VTK_PARSE_UNSIGNED_ID_TYPE_PTR) args_ok = 0;
    if (aType == VTK_PARSE_UNSIGNED_LONG_LONG_PTR) args_ok = 0;
    if (aType == VTK_PARSE_UNSIGNED___INT64_PTR) args_ok = 0;
    }

  baseType = (rType & VTK_PARSE_BASE_TYPE);

  for (j = 0; supported_types[j] != 0; j++)
    {
    if (baseType == supported_types[j]) { break; }
    }
  if (supported_types[j] == 0)
    {
    args_ok = 0;
    }

  if (baseType == VTK_PARSE_OBJECT)
    {
    if ((rType & VTK_PARSE_INDIRECT) != VTK_PARSE_POINTER)
      {
      args_ok = 0;
      }
    else if (!isClassWrapped(currentFunction->ReturnClass))
      {
      args_ok = 0;
      }
    }

  if (((rType & VTK_PARSE_INDIRECT) != VTK_PARSE_POINTER) &&
      ((rType & VTK_PARSE_INDIRECT) != 0) &&
      (rType != VTK_PARSE_STRING_REF)) args_ok = 0;
  if (rType == VTK_PARSE_STRING_PTR) args_ok = 0;

  /* eliminate unsigned char * and unsigned short * */
  if (rType == VTK_PARSE_UNSIGNED_INT_PTR) args_ok = 0;
  if (rType == VTK_PARSE_UNSIGNED_SHORT_PTR) args_ok = 0;
  if (rType == VTK_PARSE_UNSIGNED_LONG_PTR) args_ok = 0;
  if (rType == VTK_PARSE_UNSIGNED_ID_TYPE_PTR) args_ok = 0;
  if (rType == VTK_PARSE_UNSIGNED_LONG_LONG_PTR) args_ok = 0;
  if (rType == VTK_PARSE_UNSIGNED___INT64_PTR) args_ok = 0;

  /* make sure we have all the info we need for array arguments in */
  for (i = 0; i < currentFunction->NumberOfArguments; i++)
    {
    aType = (currentFunction->ArgTypes[i] & VTK_PARSE_UNQUALIFIED_TYPE);

    if (((aType & VTK_PARSE_INDIRECT) == VTK_PARSE_POINTER)&&
        (currentFunction->ArgCounts[i] <= 0)&&
        (aType != VTK_PARSE_OBJECT_PTR)&&
        (aType != VTK_PARSE_CHAR_PTR)) args_ok = 0;
    }

  /* if we need a return type hint make sure we have one */
  switch (rType)
    {
    case VTK_PARSE_FLOAT_PTR:
    case VTK_PARSE_VOID_PTR:
    case VTK_PARSE_DOUBLE_PTR:
    case VTK_PARSE_INT_PTR:
    case VTK_PARSE_SHORT_PTR:
    case VTK_PARSE_LONG_PTR:
    case VTK_PARSE_ID_TYPE_PTR:
    case VTK_PARSE_LONG_LONG_PTR:
    case VTK_PARSE___INT64_PTR:
    case VTK_PARSE_SIGNED_CHAR_PTR:
    case VTK_PARSE_BOOL_PTR:
    case VTK_PARSE_UNSIGNED_CHAR_PTR:
      args_ok = currentFunction->HaveHint;
      break;
    }

  /* make sure there isn't a Java-specific override */
  if (!strcmp("vtkObject",data->Name))
    {
    /* remove the original vtkCommand observer methods */
    if (!strcmp(currentFunction->Name,"AddObserver") ||
        !strcmp(currentFunction->Name,"GetCommand") ||
        (!strcmp(currentFunction->Name,"RemoveObserver") &&
         (currentFunction->ArgTypes[0] != VTK_PARSE_UNSIGNED_LONG)) ||
        ((!strcmp(currentFunction->Name,"RemoveObservers") ||
          !strcmp(currentFunction->Name,"HasObserver")) &&
         (((currentFunction->ArgTypes[0] != VTK_PARSE_UNSIGNED_LONG) &&
           (currentFunction->ArgTypes[0] !=
            (VTK_PARSE_CHAR_PTR|VTK_PARSE_CONST))) ||
          (currentFunction->NumberOfArguments > 1))) ||
        (!strcmp(currentFunction->Name,"RemoveAllObservers") &&
         (currentFunction->NumberOfArguments > 0)))
      {
      args_ok = 0;
      }
    }
  else if (!strcmp("vtkObjectBase",data->Name))
    {
    /* remove the special vtkObjectBase methods */
    if (!strcmp(currentFunction->Name,"Print")
#ifndef VTK_LEGACY_REMOVE
        || !strcmp(currentFunction->Name,"PrintRevisions")
#endif
        )
      {
      args_ok = 0;
      }
    }

  /* make sure it isn't a Delete or New function */
  if (!strcmp("Delete",currentFunction->Name) ||
      !strcmp("New",currentFunction->Name))
    {
    args_ok = 0;
    }

  return args_ok;
}

void outputFunction(FILE *fp, ClassInfo *data)
{
  int i;
  unsigned int rType =
    (currentFunction->ReturnType & VTK_PARSE_UNQUALIFIED_TYPE);
  int args_ok = checkFunctionSignature(data);

  /* handle DataReader SetBinaryInputString as a special case */
  if (!strcmp("SetBinaryInputString",currentFunction->Name) &&
      (!strcmp("vtkDataReader",data->Name) ||
       !strcmp("vtkStructuredGridReader",data->Name) ||
       !strcmp("vtkRectilinearGridReader",data->Name) ||
       !strcmp("vtkUnstructuredGridReader",data->Name) ||
       !strcmp("vtkStructuredPointsReader",data->Name) ||
       !strcmp("vtkPolyDataReader",data->Name)))
      {
          HandleDataReader(fp);
          wrappedFunctions[numberOfWrappedFunctions] = currentFunction;
          numberOfWrappedFunctions++;
      }

  if (currentFunction->IsPublic && args_ok &&
      strcmp(data->Name,currentFunction->Name) &&
      strcmp(data->Name, currentFunction->Name + 1))
    {
    /* make sure we haven't already done one of these */
    if (!DoneOne())
      {
      fprintf(fp,"\n  private native ");
      return_result_native(fp);
      fprintf(fp,"%s_%i(",currentFunction->Name,numberOfWrappedFunctions);

      for (i = 0; i < currentFunction->NumberOfArguments; i++)
        {
        if (i)
          {
          fprintf(fp,",");
          }
        output_temp(fp,i);

        /* ignore args after function pointer */
        if (currentFunction->ArgTypes[i] == VTK_PARSE_FUNCTION)
          {
          break;
          }
        }
      fprintf(fp,");\n");
      fprintf(fp,"  public ");
      return_result(fp);
      fprintf(fp,"%s(",currentFunction->Name);

      for (i = 0; i < currentFunction->NumberOfArguments; i++)
        {
        if (i)
          {
          fprintf(fp,",");
          }
        output_temp(fp,i);

        /* ignore args after function pointer */
        if (currentFunction->ArgTypes[i] == VTK_PARSE_FUNCTION)
          {
          break;
          }
        }

      /* if returning object, lookup in global hash */
      if (rType == VTK_PARSE_OBJECT_PTR)
        {
        fprintf(fp,") {");
        fprintf(fp,"\n    long temp = %s_%i(",currentFunction->Name, numberOfWrappedFunctions);
        for (i = 0; i < currentFunction->NumberOfArguments; i++)
          {
          if (i)
            {
            fprintf(fp,",");
            }
          fprintf(fp,"id%i",i);
          }
        fprintf(fp,");\n");
        fprintf(fp,"\n    if (temp == 0) return null;");
        fprintf(fp,"\n    return (%s)vtkObjectBase.JAVA_OBJECT_MANAGER.getJavaObject(temp);", currentFunction->ReturnClass);
        fprintf(fp,"\n  }\n");
        }
      else
        {
        /* if not void then need return otherwise none */
        if (rType == VTK_PARSE_VOID)
          {
          fprintf(fp,")\n    { %s_%i(",currentFunction->Name,
                  numberOfWrappedFunctions);
          }
        else
          {
          fprintf(fp,")\n    { return %s_%i(",currentFunction->Name,
                  numberOfWrappedFunctions);
          }
        for (i = 0; i < currentFunction->NumberOfArguments; i++)
          {
          if (i)
            {
            fprintf(fp,",");
            }
          fprintf(fp,"id%i",i);
          }
        fprintf(fp,"); }\n");
        }

      wrappedFunctions[numberOfWrappedFunctions] = currentFunction;
      numberOfWrappedFunctions++;
      }
    }
}

/* print the parsed structures */
int main(int argc, char *argv[])
{
  OptionInfo *options;
  FileInfo *file_info;
  ClassInfo *data;
  FILE *fp;
  int i;

  /* get command-line args and parse the header file */
  file_info = vtkParse_Main(argc, argv);

  /* get the command-line options */
  options = vtkParse_GetCommandLineOptions();

  /* get the output file */
  fp = fopen(options->OutputFileName, "w");

  if (!fp)
    {
    fprintf(stderr, "Error opening output file %s\n", options->OutputFileName);
    exit(1);
    }

  /* get the main class */
  if ((data = file_info->MainClass) == NULL)
    {
    fclose(fp);
    exit(0);
    }

  /* get the hierarchy info for accurate typing */
  if (options->HierarchyFileName)
    {
    hierarchyInfo = vtkParseHierarchy_ReadFile(options->HierarchyFileName);
    }

  fprintf(fp,"// java wrapper for %s object\n//\n",data->Name);
  fprintf(fp,"\npackage vtk;\n");

  if (strcmp("vtkObjectBase",data->Name))
    {
    fprintf(fp,"import vtk.*;\n");
    }
  fprintf(fp,"\npublic class %s",data->Name);
  if (strcmp("vtkObjectBase",data->Name))
    {
    if (data->NumberOfSuperClasses)
      {
      fprintf(fp," extends %s",data->SuperClasses[0]);
      }
    }
  fprintf(fp,"\n{\n");

  /* insert function handling code here */
  for (i = 0; i < data->NumberOfFunctions; i++)
    {
    currentFunction = data->Functions[i];
    outputFunction(fp, data);
    }

  HandleDataArray(fp, data);

  if (!data->NumberOfSuperClasses)
    {
    if ( strcmp("vtkObjectBase",data->Name) == 0 )
      {
      fprintf(fp,"\n  public static vtk.vtkJavaMemoryManager JAVA_OBJECT_MANAGER = new vtk.vtkJavaMemoryManagerImpl();");
      }
    if (!data->IsAbstract)
      {
      fprintf(fp,"\n  public %s() {", data->Name);
      fprintf(fp,"\n    this.vtkId = this.VTKInit();");
      fprintf(fp,"\n    vtkObjectBase.JAVA_OBJECT_MANAGER.registerJavaObject(this.vtkId, this);");
      fprintf(fp,"\n  }\n");
      }
    else
      {
      fprintf(fp,"\n  public %s() { super(); }\n",data->Name);
      }
    fprintf(fp,"\n  public %s(long id) {", data->Name);
    fprintf(fp,"\n    super();");
    fprintf(fp,"\n    this.vtkId = id;");
    fprintf(fp,"\n    this.VTKRegister();");
    fprintf(fp,"\n    vtkObjectBase.JAVA_OBJECT_MANAGER.registerJavaObject(this.vtkId, this);");
    fprintf(fp,"\n  }\n");
    fprintf(fp,"\n  protected long vtkId;\n");
    fprintf(fp,"\n  public long GetVTKId() { return this.vtkId; }");

    /* if we are a base class and have a delete method */
    if (data->HasDelete)
      {
      fprintf(fp,"\n  public static native void VTKDeleteReference(long id);");
      fprintf(fp,"\n  public static native String VTKGetClassNameFromReference(long id);");
      fprintf(fp,"\n  protected native void VTKDelete();");
      fprintf(fp,"\n  protected native void VTKRegister();");
      fprintf(fp,"\n  public void Delete() {");
      fprintf(fp,"\n    vtkObjectBase.JAVA_OBJECT_MANAGER.unRegisterJavaObject(this.vtkId);");
      fprintf(fp,"\n    this.vtkId = 0;");
      fprintf(fp,"\n  }");
      }
    }
  else
    {
    fprintf(fp,"\n  public %s() { super(); }\n",data->Name);
    fprintf(fp,"\n  public %s(long id) { super(id); }\n",data->Name);
    }

  if (!data->IsAbstract)
    {
    fprintf(fp,"  public native long   VTKInit();\n");
    }

  /* fprintf(fp,"  protected native void   VTKCastInit();\n"); */

  if (!strcmp("vtkObjectBase",data->Name))
    {
    /* Add the Print method to vtkObjectBase. */
    fprintf(fp,"  public native String Print();\n");
#ifndef VTK_LEGACY_REMOVE
    /* Add the PrintRevisions method to vtkObject. */
    fprintf(fp,"  public native String PrintRevisions();\n");
#endif
    /* Add the default toString from java object */
    fprintf(fp,"  public String toString() { return Print(); }\n");
    }

  if (!strcmp("vtkObject",data->Name))
    {
    fprintf(fp,"  public native int AddObserver(String id0, Object id1, String id2);\n");
    }
  fprintf(fp,"\n}\n");
  fclose(fp);
  {
  size_t cc;
  size_t len;
  char *dir;
  char *fname;
  /*const */char javaDone[] = "VTKJavaWrapped";
  FILE* tfp;
  fname = options->OutputFileName;
  dir = (char*)malloc(strlen(fname) + strlen(javaDone) + 2);
  sprintf(dir, "%s", fname);
  len = strlen(dir);
  for ( cc = len-1; cc > 0; cc -- )
    {
    if ( dir[cc] == '/' || dir[cc] == '\\' )
      {
      dir[cc+1] = 0;
      break;
      }
    }
  strcat(dir, javaDone);
  tfp = fopen(dir, "w");
  if ( tfp )
    {
    fprintf(tfp, "File: %s\n", fname);
    fclose(tfp);
    }
  free(dir);
  }

  vtkParse_Free(file_info);

  return 0;
}
