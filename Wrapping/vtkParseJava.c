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

int numberOfWrappedFunctions = 0;
FunctionInfo *wrappedFunctions[1000];
extern FunctionInfo *currentFunction;

void output_temp(FILE *fp,int i)
{
  int aType = (currentFunction->ArgTypes[i] & VTK_PARSE_UNQUALIFIED_TYPE);

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
      case VTK_PARSE_VTK_OBJECT:  fprintf(fp,"%s ",currentFunction->ArgClasses[i]); break;
      case VTK_PARSE_UNKNOWN: return;
      }
    }

  fprintf(fp,"id%i",i);
  if (((aType & VTK_PARSE_INDIRECT) == VTK_PARSE_POINTER) &&
      (aType != VTK_PARSE_CHAR_PTR) &&
      (aType != VTK_PARSE_VTK_OBJECT_PTR))
    {
    fprintf(fp,"[]");
    }
}

void return_result(FILE *fp)
{
  int rType = (currentFunction->ReturnType & VTK_PARSE_UNQUALIFIED_TYPE);

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
    case VTK_PARSE_VTK_OBJECT_PTR:
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
  int rType = (currentFunction->ReturnType & VTK_PARSE_UNQUALIFIED_TYPE);

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
    case VTK_PARSE_VTK_OBJECT_PTR:
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

/* have we done one of these yet */
int DoneOne()
{
  int aType = 0;
  int fType = 0;
  int rType = 0;
  int qType = 0;
  int i,j;
  int match;
  FunctionInfo *fi;

  for (i = 0; i < numberOfWrappedFunctions; i++)
    {
    fi = wrappedFunctions[i];
    rType = (currentFunction->ReturnType & VTK_PARSE_UNQUALIFIED_TYPE);
    qType = (fi->ReturnType & VTK_PARSE_UNQUALIFIED_TYPE);

    if ((!strcmp(fi->Name,currentFunction->Name))
        &&(fi->NumberOfArguments == currentFunction->NumberOfArguments))
      {
      match = 1;
      for (j = 0; j < fi->NumberOfArguments; j++)
        {
        aType = (currentFunction->ArgTypes[j] & VTK_PARSE_UNQUALIFIED_TYPE);
        fType = (fi->ArgTypes[j] & VTK_PARSE_UNQUALIFIED_TYPE);

        if ((fi->ArgTypes[j] != currentFunction->ArgTypes[j]) &&
            !(((fType == VTK_PARSE_FLOAT_PTR)&&
               (aType == VTK_PARSE_DOUBLE_PTR)) ||
              ((fType == VTK_PARSE_DOUBLE_PTR)&&
               (aType == VTK_PARSE_FLOAT_PTR)) ||
              ((fType == VTK_PARSE_INT_PTR)&&
               (aType == VTK_PARSE_LONG_PTR)) ||
              ((fType == VTK_PARSE_LONG_PTR)&&
               (aType == VTK_PARSE_INT_PTR)) ||
              ((fType == VTK_PARSE_INT_PTR)&&
               (aType == VTK_PARSE_ID_TYPE_PTR)) ||
              ((fType == VTK_PARSE_ID_TYPE_PTR)&&
               (aType == VTK_PARSE_INT_PTR)) ||
              ((fType == VTK_PARSE_ID_TYPE_PTR)&&
               (aType == VTK_PARSE_LONG_PTR)) ||
              ((fType == VTK_PARSE_LONG_PTR)&&
               (aType == VTK_PARSE_ID_TYPE_PTR)) ||
              ((fType == VTK_PARSE_INT_PTR)&&
               (aType == VTK_PARSE_LONG_LONG_PTR)) ||
              ((fType == VTK_PARSE_LONG_LONG_PTR)&&
               (aType == VTK_PARSE_INT_PTR)) ||
              ((fType == VTK_PARSE_LONG_LONG_PTR)&&
               (aType == VTK_PARSE_LONG_PTR)) ||
              ((fType == VTK_PARSE_LONG_PTR)&&
               (aType == VTK_PARSE_LONG_LONG_PTR)) ||
              ((fType == VTK_PARSE_INT_PTR)&&
               (aType == VTK_PARSE___INT64_PTR)) ||
              ((fType == VTK_PARSE___INT64_PTR)&&
               (aType == VTK_PARSE_INT_PTR)) ||
              ((fType == VTK_PARSE___INT64_PTR)&&
               (aType == VTK_PARSE_LONG_PTR)) ||
              ((fType == VTK_PARSE_LONG_PTR)&&
               (aType == VTK_PARSE___INT64_PTR)) ||
              ((fType == VTK_PARSE_FLOAT)&&
               (aType == VTK_PARSE_DOUBLE)) ||
              ((fType == VTK_PARSE_DOUBLE)&&
               (aType == VTK_PARSE_FLOAT)) ||
              ((fType == VTK_PARSE_INT)&&
               (aType == VTK_PARSE_LONG)) ||
              ((fType == VTK_PARSE_LONG)&&
               (aType == VTK_PARSE_INT)) ||
              ((fType == VTK_PARSE_INT)&&
               (aType == VTK_PARSE_ID_TYPE)) ||
              ((fType == VTK_PARSE_ID_TYPE)&&
               (aType == VTK_PARSE_INT)) ||
              ((fType == VTK_PARSE_ID_TYPE)&&
               (aType == VTK_PARSE_LONG)) ||
              ((fType == VTK_PARSE_LONG)&&
               (aType == VTK_PARSE_ID_TYPE)) ||
              ((fType == VTK_PARSE_INT)&&
               (aType == VTK_PARSE_LONG_LONG)) ||
              ((fType == VTK_PARSE_LONG_LONG)&&
               (aType == VTK_PARSE_INT)) ||
              ((fType == VTK_PARSE_LONG_LONG)&&
               (aType == VTK_PARSE_LONG)) ||
              ((fType == VTK_PARSE_LONG)&&
               (aType == VTK_PARSE_LONG_LONG)) ||
              ((fType == VTK_PARSE_INT)&&
               (aType == VTK_PARSE___INT64)) ||
              ((fType == VTK_PARSE___INT64)&&
               (aType == VTK_PARSE_INT)) ||
              ((fType == VTK_PARSE___INT64)&&
               (aType == VTK_PARSE_LONG)) ||
              ((fType == VTK_PARSE_LONG)&&
               (aType == VTK_PARSE___INT64)) ||
              ((fType == VTK_PARSE_CHAR_PTR)&&
               (aType == VTK_PARSE_STRING_REF)) ||
              ((fType == VTK_PARSE_STRING_REF)&&
               (aType == VTK_PARSE_CHAR_PTR)) ||
              ((fType == VTK_PARSE_CHAR_PTR)&&
               (aType == VTK_PARSE_STRING)) ||
              ((fType == VTK_PARSE_STRING)&&
               (aType == VTK_PARSE_CHAR_PTR))))
          {
          match = 0;
          }
        else
          {
          if (fType == VTK_PARSE_VTK_OBJECT_PTR)
            {
            if (strcmp(fi->ArgClasses[j],currentFunction->ArgClasses[j]))
              {
              match = 0;
              }
            }
          }
        }
      if ((fi->ReturnType != currentFunction->ReturnType) &&
          !(((qType == VTK_PARSE_FLOAT_PTR)&&
             (rType == VTK_PARSE_DOUBLE_PTR)) ||
            ((qType == VTK_PARSE_DOUBLE_PTR)&&
             (rType == VTK_PARSE_FLOAT_PTR)) ||
            ((qType == VTK_PARSE_INT_PTR)&&
             (rType == VTK_PARSE_LONG_PTR)) ||
            ((qType == VTK_PARSE_LONG_PTR)&&
             (rType == VTK_PARSE_INT_PTR)) ||
            ((qType == VTK_PARSE_ID_TYPE_PTR)&&
             (rType == VTK_PARSE_LONG_PTR)) ||
            ((qType == VTK_PARSE_LONG_PTR)&&
             (rType == VTK_PARSE_ID_TYPE_PTR)) ||
            ((qType == VTK_PARSE_INT_PTR)&&
             (rType == VTK_PARSE_ID_TYPE_PTR)) ||
            ((qType == VTK_PARSE_ID_TYPE_PTR)&&
             (rType == VTK_PARSE_INT_PTR)) ||
            ((qType == VTK_PARSE_LONG_LONG_PTR)&&
             (rType == VTK_PARSE_LONG_PTR)) ||
            ((qType == VTK_PARSE_LONG_PTR)&&
             (rType == VTK_PARSE_LONG_LONG_PTR)) ||
            ((qType == VTK_PARSE_INT_PTR)&&
             (rType == VTK_PARSE_LONG_LONG_PTR)) ||
            ((qType == VTK_PARSE_LONG_LONG_PTR)&&
             (rType == VTK_PARSE_INT_PTR)) ||
            ((qType == VTK_PARSE___INT64_PTR)&&
             (rType == VTK_PARSE_LONG_PTR)) ||
            ((qType == VTK_PARSE_LONG_PTR)&&
             (rType == VTK_PARSE___INT64_PTR)) ||
            ((qType == VTK_PARSE_INT_PTR)&&
             (rType == VTK_PARSE___INT64_PTR)) ||
            ((qType == VTK_PARSE___INT64_PTR)&&
             (rType == VTK_PARSE_INT_PTR)) ||
            ((qType == VTK_PARSE_CHAR_PTR)&&
             (rType == VTK_PARSE_STRING_REF)) ||
            ((qType == VTK_PARSE_STRING_REF)&&
             (rType == VTK_PARSE_CHAR_PTR)) ||
            ((qType == VTK_PARSE_CHAR_PTR)&&
             (rType == VTK_PARSE_STRING)) ||
            ((qType == VTK_PARSE_STRING)&&
             (rType == VTK_PARSE_CHAR_PTR)) ||
            ((qType == VTK_PARSE_FLOAT)&&
             (rType == VTK_PARSE_DOUBLE)) ||
            ((qType == VTK_PARSE_DOUBLE)&&
             (rType == VTK_PARSE_FLOAT)) ||
            ((qType == VTK_PARSE_INT)&&
             (rType == VTK_PARSE_LONG)) ||
            ((qType == VTK_PARSE_LONG)&&
             (rType == VTK_PARSE_INT)) ||
            ((qType == VTK_PARSE_INT)&&
             (rType == VTK_PARSE_ID_TYPE)) ||
            ((qType == VTK_PARSE_ID_TYPE)&&
             (rType == VTK_PARSE_INT)) ||
            ((qType == VTK_PARSE_ID_TYPE)&&
             (rType == VTK_PARSE_LONG)) ||
            ((qType == VTK_PARSE_LONG)&&
             (rType == VTK_PARSE_ID_TYPE)) ||
            ((qType == VTK_PARSE_INT)&&
             (rType == VTK_PARSE_LONG_LONG)) ||
            ((qType == VTK_PARSE_LONG_LONG)&&
             (rType == VTK_PARSE_INT)) ||
            ((qType == VTK_PARSE_LONG_LONG)&&
             (rType == VTK_PARSE_LONG)) ||
            ((qType == VTK_PARSE_LONG)&&
             (rType == VTK_PARSE_LONG_LONG)) ||
            ((qType == VTK_PARSE_INT)&&
             (rType == VTK_PARSE___INT64)) ||
            ((qType == VTK_PARSE___INT64)&&
             (rType == VTK_PARSE_INT)) ||
            ((qType == VTK_PARSE___INT64)&&
             (rType == VTK_PARSE_LONG)) ||
            ((qType == VTK_PARSE_LONG)&&
             (rType == VTK_PARSE___INT64))))
        {
        match = 0;
        }
      else
        {
        if (qType == VTK_PARSE_VTK_OBJECT_PTR)
          {
          if (strcmp(fi->ReturnClass,currentFunction->ReturnClass))
            {
            match = 0;
            }
          }
        }
      if (match) return 1;
      }
    }
  return 0;
}

void HandleDataReader(FILE *fp, FileInfo *data)
{
    fprintf(fp,"\n  private native void ");
    fprintf(fp,"%s_%i(byte id0[],int id1);\n",
            currentFunction->Name,numberOfWrappedFunctions);
    fprintf(fp,"\n  public void ");
    fprintf(fp,"%s(byte id0[],int id1)\n",currentFunction->Name);
    fprintf(fp,"    { %s_%i(id0,id1); }\n",
            currentFunction->Name,numberOfWrappedFunctions);
    data = 0;
}

void HandleDataArray(FILE *fp, FileInfo *data)
{
  const char *type = 0;

  if (!strcmp("vtkCharArray",data->ClassName) )
    {
    type = "char";
    }
  else if (!strcmp("vtkDoubleArray",data->ClassName) )
    {
    type = "double";
    }
  else if (!strcmp("vtkFloatArray",data->ClassName) )
    {
    type = "float";
    }
  else if (!strcmp("vtkIntArray",data->ClassName) )
    {
    type = "int";
    }
  else if (!strcmp("vtkLongArray",data->ClassName) )
    {
    type = "long";
    }
  else if (!strcmp("vtkShortArray",data->ClassName) )
    {
    type = "short";
    }
  else if (!strcmp("vtkUnsignedCharArray",data->ClassName) )
    {
    type = "byte";
    }
  else if (!strcmp("vtkUnsignedIntArray",data->ClassName) )
    {
    type = "int";
    }
  else if (!strcmp("vtkUnsignedLongArray",data->ClassName) )
    {
    type = "long";
    }
  else if (!strcmp("vtkUnsignedShortArray",data->ClassName) )
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

void outputFunction(FILE *fp, FileInfo *data)
{
  int i;
  int args_ok = 1;
  int rType = (currentFunction->ReturnType & VTK_PARSE_UNQUALIFIED_TYPE);
  int aType = 0;

  /* some functions will not get wrapped no matter what else */
  if (currentFunction->IsOperator ||
      currentFunction->ArrayFailure ||
      !currentFunction->IsPublic ||
      !currentFunction->Name)
    {
    return;
    }

  /* NewInstance and SafeDownCast can not be wrapped because it is a
     (non-virtual) method which returns a pointer of the same type as
     the current pointer. Since all methods are virtual in Java, this
     looks like polymorphic return type.  */
  if (!strcmp("NewInstance",currentFunction->Name))
    {
    return ;
    }

  if (!strcmp("SafeDownCast",currentFunction->Name))
    {
    return ;
    }

  /* check to see if we can handle the args */
  for (i = 0; i < currentFunction->NumberOfArguments; i++)
    {
    aType = (currentFunction->ArgTypes[i] & VTK_PARSE_UNQUALIFIED_TYPE);

    if (aType == VTK_PARSE_VTK_OBJECT) args_ok = 0;
    if ((aType & VTK_PARSE_BASE_TYPE) == VTK_PARSE_UNKNOWN) args_ok = 0;
    if (((aType & VTK_PARSE_INDIRECT) != VTK_PARSE_POINTER)&&
        ((aType & VTK_PARSE_INDIRECT) != 0)) args_ok = 0;
    if (aType == VTK_PARSE_UNSIGNED_CHAR_PTR) args_ok = 0;
    if (aType == VTK_PARSE_UNSIGNED_INT_PTR) args_ok = 0;
    if (aType == VTK_PARSE_UNSIGNED_SHORT_PTR) args_ok = 0;
    if (aType == VTK_PARSE_UNSIGNED_LONG_PTR) args_ok = 0;
    if (aType == VTK_PARSE_UNSIGNED_ID_TYPE_PTR) args_ok = 0;
    if (aType == VTK_PARSE_UNSIGNED_LONG_LONG_PTR) args_ok = 0;
    if (aType == VTK_PARSE_UNSIGNED___INT64_PTR) args_ok = 0;
    if ((aType & VTK_PARSE_BASE_TYPE) == VTK_PARSE_UNICODE_STRING) args_ok = 0;
    }
  if ((rType & VTK_PARSE_BASE_TYPE) == VTK_PARSE_UNKNOWN) args_ok = 0;
  if (rType == VTK_PARSE_VTK_OBJECT) args_ok = 0;
  if (((rType & VTK_PARSE_INDIRECT) != VTK_PARSE_POINTER)&&
      ((rType & VTK_PARSE_INDIRECT) != 0)) args_ok = 0;


  /* eliminate unsigned char * and unsigned short * */
  if (rType == VTK_PARSE_UNSIGNED_INT_PTR) args_ok = 0;
  if (rType == VTK_PARSE_UNSIGNED_SHORT_PTR) args_ok = 0;
  if (rType == VTK_PARSE_UNSIGNED_LONG_PTR) args_ok = 0;
  if (rType == VTK_PARSE_UNSIGNED_ID_TYPE_PTR) args_ok = 0;
  if (rType == VTK_PARSE_UNSIGNED_LONG_LONG_PTR) args_ok = 0;
  if (rType == VTK_PARSE_UNSIGNED___INT64_PTR) args_ok = 0;

  if ((rType & VTK_PARSE_BASE_TYPE) == VTK_PARSE_UNICODE_STRING) args_ok = 0;

  if (currentFunction->NumberOfArguments &&
      (currentFunction->ArgTypes[0] == VTK_PARSE_FUNCTION)
      &&(currentFunction->NumberOfArguments != 1)) args_ok = 0;

  /* make sure we have all the info we need for array arguments in */
  for (i = 0; i < currentFunction->NumberOfArguments; i++)
    {
    aType = (currentFunction->ArgTypes[i] & VTK_PARSE_UNQUALIFIED_TYPE);

    if (((aType & VTK_PARSE_INDIRECT) == VTK_PARSE_POINTER)&&
        (currentFunction->ArgCounts[i] <= 0)&&
        (aType != VTK_PARSE_VTK_OBJECT_PTR)&&
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

  /* make sure it isn't a Delete or New function */
  if (!strcmp("Delete",currentFunction->Name) ||
      !strcmp("New",currentFunction->Name))
    {
    args_ok = 0;
    }

  /* handle DataReader SetBinaryInputString as a special case */
  if (!strcmp("SetBinaryInputString",currentFunction->Name) &&
      (!strcmp("vtkDataReader",data->ClassName) ||
       !strcmp("vtkStructuredGridReader",data->ClassName) ||
       !strcmp("vtkRectilinearGridReader",data->ClassName) ||
       !strcmp("vtkUnstructuredGridReader",data->ClassName) ||
       !strcmp("vtkStructuredPointsReader",data->ClassName) ||
       !strcmp("vtkPolyDataReader",data->ClassName)))
      {
          HandleDataReader(fp,data);
          wrappedFunctions[numberOfWrappedFunctions] = currentFunction;
          numberOfWrappedFunctions++;
      }

  if (currentFunction->IsPublic && args_ok &&
      strcmp(data->ClassName,currentFunction->Name) &&
      strcmp(data->ClassName, currentFunction->Name + 1))
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
        }
      /* if returning object, lookup in global hash */
      if (rType == VTK_PARSE_VTK_OBJECT_PTR)
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
        fprintf(fp,"\n    %s obj = null;", currentFunction->ReturnClass);
        fprintf(fp,"\n    java.lang.ref.WeakReference ref = (java.lang.ref.WeakReference)vtkGlobalJavaHash.PointerToReference.get(new Long(temp));");
        fprintf(fp,"\n    if (ref != null) {");
        fprintf(fp,"\n      obj = (%s)ref.get();", currentFunction->ReturnClass);
        fprintf(fp,"\n    }");
        fprintf(fp,"\n    if (obj == null) {");
        fprintf(fp,"\n      %s tempObj = new %s(temp);", currentFunction->ReturnClass, currentFunction->ReturnClass);
        fprintf(fp,"\n      String className = tempObj.GetClassName();");
        fprintf(fp,"\n      try {");
        fprintf(fp,"\n        Class c = Class.forName(\"vtk.\" + className);");
        fprintf(fp,"\n        java.lang.reflect.Constructor cons = c.getConstructor(new Class[] {long.class} );");
        fprintf(fp,"\n        obj = (%s)cons.newInstance(new Object[] {new Long(temp)});", currentFunction->ReturnClass);
        fprintf(fp,"\n      } catch (Exception e) {");
        fprintf(fp,"\n        e.printStackTrace();");
        fprintf(fp,"\n      }");
        fprintf(fp,"\n      vtkObjectBase.VTKDeleteReference(temp);");
        fprintf(fp,"\n    }");
        fprintf(fp,"\n    return obj;");
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
        if ((currentFunction->NumberOfArguments == 1) &&
            (currentFunction->ArgTypes[0] == VTK_PARSE_FUNCTION))
          {
          fprintf(fp,",id1");
          }
        fprintf(fp,"); }\n");
        }

      wrappedFunctions[numberOfWrappedFunctions] = currentFunction;
      numberOfWrappedFunctions++;
      }
    }
}

/* print the parsed structures */
void vtkParseOutput(FILE *fp, FileInfo *data)
{
  int i;

  fprintf(fp,"// java wrapper for %s object\n//\n",data->ClassName);
  fprintf(fp,"\npackage vtk;\n");

  if (strcmp("vtkObjectBase",data->ClassName))
    {
    fprintf(fp,"import vtk.*;\n");
    }
  fprintf(fp,"\npublic class %s",data->ClassName);
  if (strcmp("vtkObjectBase",data->ClassName))
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
    currentFunction = data->Functions + i;
    outputFunction(fp, data);
    }

  HandleDataArray(fp, data);

  if (!data->NumberOfSuperClasses)
    {
    if (data->IsConcrete)
      {
      fprintf(fp,"\n  public %s() {", data->ClassName);
      fprintf(fp,"\n    this.vtkId = this.VTKInit();");
      fprintf(fp,"\n    vtkGlobalJavaHash.PointerToReference.put(new Long(this.vtkId), new java.lang.ref.WeakReference(this));");
      fprintf(fp,"\n  }\n");
      }
    else
      {
      fprintf(fp,"\n  public %s() { super(); }\n",data->ClassName);
      }
    fprintf(fp,"\n  public %s(long id) {", data->ClassName);
    fprintf(fp,"\n    super();");
    fprintf(fp,"\n    this.vtkId = id;");
    fprintf(fp,"\n    this.VTKRegister();");
    fprintf(fp,"\n    vtkGlobalJavaHash.PointerToReference.put(new Long(this.vtkId), new java.lang.ref.WeakReference(this));");
    fprintf(fp,"\n  }\n");
    fprintf(fp,"\n  protected long vtkId = 0;\n");
    fprintf(fp,"\n  protected boolean vtkDeleted = false;\n");
    fprintf(fp,"\n  public long GetVTKId() { return this.vtkId; }");

    /* if we are a base class and have a delete method */
    if (data->HasDelete)
      {
      fprintf(fp,"\n  public static native void VTKDeleteReference(long id);");
      fprintf(fp,"\n  protected native void VTKDelete();");
      fprintf(fp,"\n  protected native void VTKRegister();");
      fprintf(fp,"\n  public void Delete() {");
      fprintf(fp,"\n    int refCount = this.GetReferenceCount();");
      fprintf(fp,"\n    vtkGlobalJavaHash.PointerToReference.remove(new Long(this.vtkId));");
      fprintf(fp,"\n    this.VTKDelete();");
      fprintf(fp,"\n    this.vtkDeleted = true;");
      fprintf(fp,"\n    if (refCount == 1) {");
      fprintf(fp,"\n      this.vtkId = 0;");
      fprintf(fp,"\n    }");
      fprintf(fp,"\n  }");
      }
    }
  /* Special case for vtkObject */
  else if ( strcmp("vtkObject",data->ClassName) == 0 )
    {
    fprintf(fp,"\n  public %s() {", data->ClassName);
    fprintf(fp,"\n    super();");
    fprintf(fp,"\n    this.vtkId = this.VTKInit();");
    fprintf(fp,"\n    vtkGlobalJavaHash.PointerToReference.put(new Long(this.vtkId), new java.lang.ref.WeakReference(this));");
    fprintf(fp,"\n  }\n");
    fprintf(fp,"\n  public %s(long id) { super(id); }\n",data->ClassName);
    }
  else
    {
    fprintf(fp,"\n  public %s() { super(); }\n",data->ClassName);
    fprintf(fp,"\n  public %s(long id) { super(id); }\n",data->ClassName);
    }

  if (data->IsConcrete)
    {
    fprintf(fp,"  public native long   VTKInit();\n");
    }

  /* fprintf(fp,"  protected native void   VTKCastInit();\n"); */

  if (!strcmp("vtkObject",data->ClassName))
    {
    /* Add the Print method to vtkObject. */
    fprintf(fp,"  public native String Print();\n");
    /* Add the PrintRevisions method to vtkObject. */
    fprintf(fp,"  public native String PrintRevisions();\n");
    /* Add the default toString from java object */
    fprintf(fp,"  public String toString() { return Print(); }\n");
    }

  if (!strcmp("vtkObject",data->ClassName))
    {
    fprintf(fp,"  public native int AddObserver(String id0, Object id1, String id2);\n");
    }
  fprintf(fp,"\n}\n");
  {
  size_t cc;
  size_t len;
  char *dir;
  char *fname;
  /*const */char javaDone[] = "VTKJavaWrapped";
  FILE* tfp;
  fname = data->OutputFileName;
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
}

