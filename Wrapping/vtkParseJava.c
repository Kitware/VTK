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
  /* ignore void */
  if (((currentFunction->ArgTypes[i] % 0x10) == 0x2)&&
      (!((currentFunction->ArgTypes[i] % 0x1000)/0x100)))
    {
    return;
    }
  
  if (currentFunction->ArgTypes[i] == 0x5000)
    {
    fprintf(fp,"Object id0, String id1");  
    return;
    }
  
  if (currentFunction->ArgTypes[i] % 0x1000 == 0x303)
    {
    fprintf(fp,"String ");
    }
  else
    {
    switch (currentFunction->ArgTypes[i] % 0x10)
      {
      case 0x1:   fprintf(fp,"double "); break;
      case 0x7:   fprintf(fp,"double "); break;
      case 0x4:   fprintf(fp,"int "); break;
      case 0x5:   fprintf(fp,"int "); break;
      case 0x6:   fprintf(fp,"int "); break;
      case 0xA:   fprintf(fp,"int "); break;
      case 0xB:   fprintf(fp,"int "); break;
      case 0xC:   fprintf(fp,"int "); break;
      case 0xD:   fprintf(fp,"char "); break;
      case 0xE:   fprintf(fp,"boolean "); break;
      case 0x2:     fprintf(fp,"void "); break;
      case 0x3:     fprintf(fp,"char "); break;
      case 0x9:     fprintf(fp,"%s ",currentFunction->ArgClasses[i]); break;
      case 0x8: return;
      }
    }

  fprintf(fp,"id%i",i);
  if (((currentFunction->ArgTypes[i] % 0x1000)/0x100 == 0x3)&&
      (currentFunction->ArgTypes[i] % 0x1000 != 0x303)&&
      (currentFunction->ArgTypes[i] % 0x1000 != 0x309))
    {
    fprintf(fp,"[]");
    }
}

void return_result(FILE *fp)
{
  switch (currentFunction->ReturnType % 0x1000)
    {
    case 0x1: fprintf(fp,"double "); break;
    case 0x2: fprintf(fp,"void "); break;
    case 0x3: fprintf(fp,"char "); break;
    case 0x7: fprintf(fp,"double "); break;
    case 0x4: case 0x5: case 0x6: case 0xA: case 0xB: case 0xC: case 0xD:
    case 0x13: case 0x14: case 0x15: case 0x16: case 0x1A: case 0x1B: case 0x1C:
      fprintf(fp,"int "); 
      break;
    case 0xE:
      fprintf(fp,"boolean ");
      break;
    case 0x303: fprintf(fp,"String "); break;
    case 0x109:  
    case 0x309:  
      fprintf(fp,"%s ",currentFunction->ReturnClass);
      break;
      
      /* handle functions returning vectors */
      /* this is done by looking them up in a hint file */
    case 0x301: case 0x307:
      fprintf(fp,"double[] "); 
      break;
    case 0x313:
        fprintf(fp,"byte[] ");
        break;
    case 0x304: case 0x305: case 0x306: case 0x30A: case 0x30B: case 0x30C: case 0x30D:
    case 0x314: case 0x315: case 0x316: case 0x31A: case 0x31B: case 0x31C:
      fprintf(fp,"int[]  "); break;
    case 0x30E:
      fprintf(fp,"boolean[]  "); break;
    }
}

/* same as return_result except we return a long (the c++ pointer) rather than an object */
void return_result_native(FILE *fp)
{
  switch (currentFunction->ReturnType % 0x1000)
    {
    case 0x1: fprintf(fp,"double "); break;
    case 0x2: fprintf(fp,"void "); break;
    case 0x3: fprintf(fp,"char "); break;
    case 0x7: fprintf(fp,"double "); break;
    case 0x4: case 0x5: case 0x6: case 0xA: case 0xB: case 0xC: case 0xD:
    case 0x13: case 0x14: case 0x15: case 0x16: case 0x1A: case 0x1B: case 0x1C:
      fprintf(fp,"int "); 
      break;
    case 0xE:
      fprintf(fp,"boolean ");
      break;
    case 0x303: fprintf(fp,"String "); break;
    case 0x109:  
    case 0x309:  
      fprintf(fp,"long "); 
      break;
      
      /* handle functions returning vectors */
      /* this is done by looking them up in a hint file */
    case 0x301: case 0x307:
      fprintf(fp,"double[] "); 
      break;
    case 0x313:
        fprintf(fp,"byte[] ");
        break;
    case 0x304: case 0x305: case 0x306: case 0x30A: case 0x30B: case 0x30C: case 0x30D:
    case 0x314: case 0x315: case 0x316: case 0x31A: case 0x31B: case 0x31C:
      fprintf(fp,"int[]  "); break;
    case 0x30E:
      fprintf(fp,"boolean[]  "); break;
    }
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
        if ((fi->ArgTypes[j] != currentFunction->ArgTypes[j]) &&
            !(((fi->ArgTypes[j] % 0x1000 == 0x309)&&
               (currentFunction->ArgTypes[j] % 0x1000 == 0x109)) ||
              ((fi->ArgTypes[j] % 0x1000 == 0x109)&&
               (currentFunction->ArgTypes[j] % 0x1000 == 0x309)) ||
              ((fi->ArgTypes[j] % 0x1000 == 0x301)&&
               (currentFunction->ArgTypes[j] % 0x1000 == 0x307)) ||
              ((fi->ArgTypes[j] % 0x1000 == 0x307)&&
               (currentFunction->ArgTypes[j] % 0x1000 == 0x301)) ||
              ((fi->ArgTypes[j] % 0x1000 == 0x304)&&
               (currentFunction->ArgTypes[j] % 0x1000 == 0x306)) ||
              ((fi->ArgTypes[j] % 0x1000 == 0x306)&&
               (currentFunction->ArgTypes[j] % 0x1000 == 0x304)) ||
              ((fi->ArgTypes[j] % 0x1000 == 0x304)&&
               (currentFunction->ArgTypes[j] % 0x1000 == 0x30A)) ||
              ((fi->ArgTypes[j] % 0x1000 == 0x30A)&&
               (currentFunction->ArgTypes[j] % 0x1000 == 0x304)) ||
              ((fi->ArgTypes[j] % 0x1000 == 0x30A)&&
               (currentFunction->ArgTypes[j] % 0x1000 == 0x306)) ||
              ((fi->ArgTypes[j] % 0x1000 == 0x306)&&
               (currentFunction->ArgTypes[j] % 0x1000 == 0x30A)) ||
              ((fi->ArgTypes[j] % 0x1000 == 0x304)&&
               (currentFunction->ArgTypes[j] % 0x1000 == 0x30B)) ||
              ((fi->ArgTypes[j] % 0x1000 == 0x30B)&&
               (currentFunction->ArgTypes[j] % 0x1000 == 0x304)) ||
              ((fi->ArgTypes[j] % 0x1000 == 0x30B)&&
               (currentFunction->ArgTypes[j] % 0x1000 == 0x306)) ||
              ((fi->ArgTypes[j] % 0x1000 == 0x306)&&
               (currentFunction->ArgTypes[j] % 0x1000 == 0x30B)) ||
              ((fi->ArgTypes[j] % 0x1000 == 0x304)&&
               (currentFunction->ArgTypes[j] % 0x1000 == 0x30C)) ||
              ((fi->ArgTypes[j] % 0x1000 == 0x30C)&&
               (currentFunction->ArgTypes[j] % 0x1000 == 0x304)) ||
              ((fi->ArgTypes[j] % 0x1000 == 0x30C)&&
               (currentFunction->ArgTypes[j] % 0x1000 == 0x306)) ||
              ((fi->ArgTypes[j] % 0x1000 == 0x306)&&
               (currentFunction->ArgTypes[j] % 0x1000 == 0x30C)) ||
              ((fi->ArgTypes[j] % 0x1000 == 0x1)&&
               (currentFunction->ArgTypes[j] % 0x1000 == 0x7)) ||
              ((fi->ArgTypes[j] % 0x1000 == 0x7)&&
               (currentFunction->ArgTypes[j] % 0x1000 == 0x1)) ||
              ((fi->ArgTypes[j] % 0x1000 == 0x4)&&
               (currentFunction->ArgTypes[j] % 0x1000 == 0x6)) ||
              ((fi->ArgTypes[j] % 0x1000 == 0x6)&&
               (currentFunction->ArgTypes[j] % 0x1000 == 0x4)) ||
              ((fi->ArgTypes[j] % 0x1000 == 0x4)&&
               (currentFunction->ArgTypes[j] % 0x1000 == 0xA)) ||
              ((fi->ArgTypes[j] % 0x1000 == 0xA)&&
               (currentFunction->ArgTypes[j] % 0x1000 == 0x4)) ||
              ((fi->ArgTypes[j] % 0x1000 == 0xA)&&
               (currentFunction->ArgTypes[j] % 0x1000 == 0x6)) ||
              ((fi->ArgTypes[j] % 0x1000 == 0x6)&&
               (currentFunction->ArgTypes[j] % 0x1000 == 0xA)) ||
              ((fi->ArgTypes[j] % 0x1000 == 0x4)&&
               (currentFunction->ArgTypes[j] % 0x1000 == 0xB)) ||
              ((fi->ArgTypes[j] % 0x1000 == 0xB)&&
               (currentFunction->ArgTypes[j] % 0x1000 == 0x4)) ||
              ((fi->ArgTypes[j] % 0x1000 == 0xB)&&
               (currentFunction->ArgTypes[j] % 0x1000 == 0x6)) ||
              ((fi->ArgTypes[j] % 0x1000 == 0x6)&&
               (currentFunction->ArgTypes[j] % 0x1000 == 0xB)) ||
              ((fi->ArgTypes[j] % 0x1000 == 0x4)&&
               (currentFunction->ArgTypes[j] % 0x1000 == 0xC)) ||
              ((fi->ArgTypes[j] % 0x1000 == 0xC)&&
               (currentFunction->ArgTypes[j] % 0x1000 == 0x4)) ||
              ((fi->ArgTypes[j] % 0x1000 == 0xC)&&
               (currentFunction->ArgTypes[j] % 0x1000 == 0x6)) ||
              ((fi->ArgTypes[j] % 0x1000 == 0x6)&&
               (currentFunction->ArgTypes[j] % 0x1000 == 0xC))))
          {
          match = 0;
          }
        else
          {
          if (fi->ArgTypes[j] % 0x1000 == 0x309 || fi->ArgTypes[j] % 0x1000 == 0x109)
            {
            if (strcmp(fi->ArgClasses[j],currentFunction->ArgClasses[j]))
              {
              match = 0;
              }
            }
          }
        }
      if ((fi->ReturnType != currentFunction->ReturnType) &&
          !(((fi->ReturnType % 0x1000 == 0x309)&&
             (currentFunction->ReturnType % 0x1000 == 0x109)) ||
            ((fi->ReturnType % 0x1000 == 0x109)&&
             (currentFunction->ReturnType % 0x1000 == 0x309)) ||
            ((fi->ReturnType % 0x1000 == 0x301)&&
             (currentFunction->ReturnType % 0x1000 == 0x307)) ||
            ((fi->ReturnType % 0x1000 == 0x307)&&
             (currentFunction->ReturnType % 0x1000 == 0x301)) ||
            ((fi->ReturnType % 0x1000 == 0x304)&&
             (currentFunction->ReturnType % 0x1000 == 0x306)) ||
            ((fi->ReturnType % 0x1000 == 0x306)&&
             (currentFunction->ReturnType % 0x1000 == 0x304)) ||
            ((fi->ReturnType % 0x1000 == 0x30A)&&
             (currentFunction->ReturnType % 0x1000 == 0x306)) ||
            ((fi->ReturnType % 0x1000 == 0x306)&&
             (currentFunction->ReturnType % 0x1000 == 0x30A)) ||
            ((fi->ReturnType % 0x1000 == 0x304)&&
             (currentFunction->ReturnType % 0x1000 == 0x30A)) ||
            ((fi->ReturnType % 0x1000 == 0x30A)&&
             (currentFunction->ReturnType % 0x1000 == 0x304)) ||
            ((fi->ReturnType % 0x1000 == 0x30B)&&
             (currentFunction->ReturnType % 0x1000 == 0x306)) ||
            ((fi->ReturnType % 0x1000 == 0x306)&&
             (currentFunction->ReturnType % 0x1000 == 0x30B)) ||
            ((fi->ReturnType % 0x1000 == 0x304)&&
             (currentFunction->ReturnType % 0x1000 == 0x30B)) ||
            ((fi->ReturnType % 0x1000 == 0x30B)&&
             (currentFunction->ReturnType % 0x1000 == 0x304)) ||
            ((fi->ReturnType % 0x1000 == 0x30C)&&
             (currentFunction->ReturnType % 0x1000 == 0x306)) ||
            ((fi->ReturnType % 0x1000 == 0x306)&&
             (currentFunction->ReturnType % 0x1000 == 0x30C)) ||
            ((fi->ReturnType % 0x1000 == 0x304)&&
             (currentFunction->ReturnType % 0x1000 == 0x30C)) ||
            ((fi->ReturnType % 0x1000 == 0x30C)&&
             (currentFunction->ReturnType % 0x1000 == 0x304)) ||
            ((fi->ReturnType % 0x1000 == 0x1)&&
             (currentFunction->ReturnType % 0x1000 == 0x7)) ||
            ((fi->ReturnType % 0x1000 == 0x7)&&
             (currentFunction->ReturnType % 0x1000 == 0x1)) ||
            ((fi->ReturnType % 0x1000 == 0x4)&&
             (currentFunction->ReturnType % 0x1000 == 0x6)) ||
            ((fi->ReturnType % 0x1000 == 0x6)&&
             (currentFunction->ReturnType % 0x1000 == 0x4)) ||
            ((fi->ReturnType % 0x1000 == 0x4)&&
             (currentFunction->ReturnType % 0x1000 == 0xA)) ||
            ((fi->ReturnType % 0x1000 == 0xA)&&
             (currentFunction->ReturnType % 0x1000 == 0x4)) ||
            ((fi->ReturnType % 0x1000 == 0xA)&&
             (currentFunction->ReturnType % 0x1000 == 0x6)) ||
            ((fi->ReturnType % 0x1000 == 0x6)&&
             (currentFunction->ReturnType % 0x1000 == 0xA)) ||
            ((fi->ReturnType % 0x1000 == 0x4)&&
             (currentFunction->ReturnType % 0x1000 == 0xB)) ||
            ((fi->ReturnType % 0x1000 == 0xB)&&
             (currentFunction->ReturnType % 0x1000 == 0x4)) ||
            ((fi->ReturnType % 0x1000 == 0xB)&&
             (currentFunction->ReturnType % 0x1000 == 0x6)) ||
            ((fi->ReturnType % 0x1000 == 0x6)&&
             (currentFunction->ReturnType % 0x1000 == 0xB)) ||
            ((fi->ReturnType % 0x1000 == 0x4)&&
             (currentFunction->ReturnType % 0x1000 == 0xC)) ||
            ((fi->ReturnType % 0x1000 == 0xC)&&
             (currentFunction->ReturnType % 0x1000 == 0x4)) ||
            ((fi->ReturnType % 0x1000 == 0xC)&&
             (currentFunction->ReturnType % 0x1000 == 0x6)) ||
            ((fi->ReturnType % 0x1000 == 0x6)&&
             (currentFunction->ReturnType % 0x1000 == 0xC))))
        {
        match = 0;
        }
      else
        {
        if (fi->ReturnType % 0x1000 == 0x309 || fi->ReturnType % 0x1000 == 0x109)
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
    if (currentFunction->ArgTypes[i] % 0x1000 == 0x9) args_ok = 0;
    if ((currentFunction->ArgTypes[i] % 0x10) == 0x8) args_ok = 0;
    if (((currentFunction->ArgTypes[i] % 0x1000)/0x100 != 0x3)&&
        (currentFunction->ArgTypes[i] % 0x1000 != 0x109)&&
        ((currentFunction->ArgTypes[i] % 0x1000)/0x100)) args_ok = 0;
    if (currentFunction->ArgTypes[i] % 0x1000 == 0x313) args_ok = 0;
    if (currentFunction->ArgTypes[i] % 0x1000 == 0x314) args_ok = 0;
    if (currentFunction->ArgTypes[i] % 0x1000 == 0x315) args_ok = 0;
    if (currentFunction->ArgTypes[i] % 0x1000 == 0x316) args_ok = 0;
    if (currentFunction->ArgTypes[i] % 0x1000 == 0x31A) args_ok = 0;
    if (currentFunction->ArgTypes[i] % 0x1000 == 0x31B) args_ok = 0;
    if (currentFunction->ArgTypes[i] % 0x1000 == 0x31C) args_ok = 0;
    }
  if ((currentFunction->ReturnType % 0x10) == 0x8) args_ok = 0;
  if (currentFunction->ReturnType % 0x1000 == 0x9) args_ok = 0;
  if (((currentFunction->ReturnType % 0x1000)/0x100 != 0x3)&&
      (currentFunction->ReturnType % 0x1000 != 0x109)&&
      ((currentFunction->ReturnType % 0x1000)/0x100)) args_ok = 0;


  /* eliminate unsigned char * and unsigned short * */
  if (currentFunction->ReturnType % 0x1000 == 0x314) args_ok = 0;
  if (currentFunction->ReturnType % 0x1000 == 0x315) args_ok = 0;
  if (currentFunction->ReturnType % 0x1000 == 0x316) args_ok = 0;
  if (currentFunction->ReturnType % 0x1000 == 0x31A) args_ok = 0;
  if (currentFunction->ReturnType % 0x1000 == 0x31B) args_ok = 0;
  if (currentFunction->ReturnType % 0x1000 == 0x31C) args_ok = 0;

  if (currentFunction->NumberOfArguments && 
      (currentFunction->ArgTypes[0] == 0x5000)
      &&(currentFunction->NumberOfArguments != 1)) args_ok = 0;

  /* make sure we have all the info we need for array arguments in */
  for (i = 0; i < currentFunction->NumberOfArguments; i++)
    {
    if (((currentFunction->ArgTypes[i] % 0x1000)/0x100 == 0x3)&&
  (currentFunction->ArgCounts[i] <= 0)&&
  (currentFunction->ArgTypes[i] % 0x1000 != 0x309)&&
  (currentFunction->ArgTypes[i] % 0x1000 != 0x303)) args_ok = 0;
    }

  /* if we need a return type hint make sure we have one */
  switch (currentFunction->ReturnType % 0x1000)
    {
    case 0x301: case 0x302: case 0x307:
    case 0x304: case 0x305: case 0x306:
    case 0x30A: case 0x30B: case 0x30C: case 0x30D: case 0x30E:
    case 0x313:
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
      if (currentFunction->ReturnType % 0x1000 == 0x109 ||
          currentFunction->ReturnType % 0x1000 == 0x309)
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
        if (currentFunction->ReturnType % 0x1000 == 0x2)
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
            (currentFunction->ArgTypes[0] == 0x5000)) fprintf(fp,",id1");
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

