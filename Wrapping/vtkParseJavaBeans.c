/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParseJavaBeans.c

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <stdio.h>
#include <string.h>
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

  if ((aType == VTK_PARSE_CHAR_PTR) || (VTK_PARSE_STRING))
    {
    fprintf(fp,"String ");
    }
  else
    {
    switch ((aType & VTK_PARSE_BASE_TYPE) & ~VTK_PARSE_UNSIGNED)
      {
      case VTK_PARSE_FLOAT:   fprintf(fp,"double "); break;
      case VTK_PARSE_DOUBLE:   fprintf(fp,"double "); break;
      case VTK_PARSE_INT:   fprintf(fp,"int "); break;
      case VTK_PARSE_SHORT:   fprintf(fp,"int "); break;
      case VTK_PARSE_LONG:   fprintf(fp,"int "); break;
      case VTK_PARSE_ID_TYPE:   fprintf(fp,"int "); break;
      case VTK_PARSE_LONG_LONG:   fprintf(fp,"int "); break;
      case VTK_PARSE___INT64:   fprintf(fp,"int "); break;
      case VTK_PARSE_VOID:     fprintf(fp,"void "); break;
      case VTK_PARSE_SIGNED_CHAR:   fprintf(fp,"char "); break;
      case VTK_PARSE_CHAR:     fprintf(fp,"char "); break;
      case VTK_PARSE_VTK_OBJECT:     fprintf(fp,"%s ",currentFunction->ArgClasses[i]); break;
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
  switch (currentFunction->ReturnType & VTK_PARSE_UNQUALIFIED_TYPE)
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
    case VTK_PARSE_UNSIGNED_CHAR:
    case VTK_PARSE_UNSIGNED_INT:
    case VTK_PARSE_UNSIGNED_SHORT:
    case VTK_PARSE_UNSIGNED_LONG:
    case VTK_PARSE_UNSIGNED_ID_TYPE:
    case VTK_PARSE_UNSIGNED_LONG_LONG:
    case VTK_PARSE_UNSIGNED___INT64:
      fprintf(fp,"int ");
      break;
    case VTK_PARSE_CHAR_PTR:
    case VTK_PARSE_STRING:
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
    case VTK_PARSE_INT_PTR:
    case VTK_PARSE_SHORT_PTR:
    case VTK_PARSE_LONG_PTR:
    case VTK_PARSE_ID_TYPE_PTR:
    case VTK_PARSE_LONG_LONG_PTR:
    case VTK_PARSE___INT64_PTR:
    case VTK_PARSE_SIGNED_CHAR_PTR:
    case VTK_PARSE_UNSIGNED_CHAR_PTR:
    case VTK_PARSE_UNSIGNED_INT_PTR:
    case VTK_PARSE_UNSIGNED_SHORT_PTR:
    case VTK_PARSE_UNSIGNED_LONG_PTR:
    case VTK_PARSE_UNSIGNED_ID_TYPE_PTR:
    case VTK_PARSE_UNSIGNED_LONG_LONG_PTR:
    case VTK_PARSE_UNSIGNED___INT64_PTR:
      fprintf(fp,"int[]  "); break;
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
               (aType == VTK_PARSE___INT64))))
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

void outputFunction(FILE *fp, FileInfo *data)
{
  int rType = (currentFunction->ReturnType & VTK_PARSE_UNQUALIFIED_TYPE);
  int aType = 0;
  int i;
  int args_ok = 1;
  /* beans */
  char *beanfunc;

  /* some functions will not get wrapped no matter what else */
  if (currentFunction->IsPureVirtual ||
      currentFunction->IsOperator ||
      currentFunction->ArrayFailure ||
      !currentFunction->IsPublic ||
      !currentFunction->Name)
    {
    return;
    }

  /* make the first letter lowercase for set get methods */
  beanfunc = strdup(currentFunction->Name);
  if (isupper(beanfunc[0])) beanfunc[0] = beanfunc[0] + 32;

  /* check to see if we can handle the args */
  for (i = 0; i < currentFunction->NumberOfArguments; i++)
    {
    aType = (currentFunction->ArgTypes[i] & VTK_PARSE_UNQUALIFIED_TYPE);

    if (aType == VTK_PARSE_VTK_OBJECT) args_ok = 0;
    if ((currentFunction->ArgTypes[i] % VTK_PARSE_FLOAT0) == VTK_PARSE_UNKNOWN) args_ok = 0;
    if (((aType & VTK_PARSE_INDIRECT) != VTK_PARSE_POINTER) &&
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
  if (((rType & VTK_PARSE_INDIRECT) != VTK_PARSE_POINTER) &&
      ((rType & VTK_PARSE_INDIRECT) != 0)) args_ok = 0;


  /* eliminate unsigned char * and unsigned short * */
  if (rType == VTK_PARSE_UNSIGNED_CHAR_PTR) args_ok = 0;
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
      args_ok = currentFunction->HaveHint;
      break;
    }

  /* make sure it isn't a Delete or New function */
  if (!strcmp("Delete",currentFunction->Name) ||
      !strcmp("New",currentFunction->Name))
    {
    args_ok = 0;
    }

  if (currentFunction->IsPublic && args_ok &&
      strcmp(data->ClassName,currentFunction->Name) &&
      strcmp(data->ClassName, currentFunction->Name + 1))
    {
    /* make sure we haven't already done one of these */
    if (!DoneOne())
      {
      fprintf(fp,"\n  private native ");
      return_result(fp);
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
      fprintf(fp,"%s(",beanfunc);

      for (i = 0; i < currentFunction->NumberOfArguments; i++)
        {
        if (i)
          {
          fprintf(fp,",");
          }
        output_temp(fp,i);
        }
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
          (currentFunction->ArgTypes[0] == VTK_PARSE_FUNCTION)) fprintf(fp,",id1");

      /* stick in secret beanie code for set methods */
      if (rType == VTK_PARSE_VOID)
        {
        aType = (currentFunction->ArgTypes[0] & VTK_PARSE_UNQUALIFIED_TYPE);

        /* only care about set methods and On/Off methods */
        if (!strncmp(beanfunc,"set",3) &&
            currentFunction->NumberOfArguments == 1 &&
            (((aType & VTK_PARSE_INDIRECT) == 0 &&
              (aType & VTK_PARSE_UNSIGNED) == 0)||
             aType == VTK_PARSE_CHAR_PTR ||
             (aType & VTK_PARSE_BASE_TYPE) == VTK_PARSE_VTK_OBJECT))
          {
          char prop[256];

          strncpy(prop,beanfunc+3,strlen(beanfunc)-3);
          prop[strlen(beanfunc)-3] = '\0';
          if (isupper(prop[0])) prop[0] = prop[0] + 32;
          fprintf(fp,");\n      changes.firePropertyChange(\"%s\",null,",prop);

          /* handle basic types */
          if ((aType == VTK_PARSE_CHAR_PTR) || (aType == VTK_PARSE_STRING))
            {
            fprintf(fp," id0");
            }
          else
            {
            switch ((aType & VTK_PARSE_BASE_TYPE) & ~VTK_PARSE_UNSIGNED)
              {
              case VTK_PARSE_FLOAT:
              case VTK_PARSE_DOUBLE:   fprintf(fp," new Double(id0)"); break;
              case VTK_PARSE_INT:
              case VTK_PARSE_SHORT:
              case VTK_PARSE_LONG:   fprintf(fp," new Integer(id0)"); break;
              case VTK_PARSE_VTK_OBJECT:   fprintf(fp," id0"); break;
              case VTK_PARSE_CHAR:   /* not implemented yet */
              default:  fprintf(fp," null");
              }
            }
          }
        /* not a set method is it an On/Off method ? */
        else
          {
          if (!strncmp(beanfunc + strlen(beanfunc) - 2, "On",2))
            {
            /* OK we think this is a Boolean method so need to fire a change */
            char prop[256];
            strncpy(prop,beanfunc,strlen(beanfunc)-2);
            prop[strlen(beanfunc)-2] = '\0';
            fprintf(fp,");\n      changes.firePropertyChange(\"%s\",null,new Integer(1)",
                    prop);
            }
          if (!strncmp(beanfunc + strlen(beanfunc) - 3, "Off",3))
            {
            /* OK we think this is a Boolean method so need to fire a change */
            char prop[256];
            strncpy(prop,beanfunc,strlen(beanfunc)-3);
            prop[strlen(beanfunc)-3] = '\0';
            fprintf(fp,");\n      changes.firePropertyChange(\"%s\",null,new Integer(0)",
                    prop);
            }
          }
        }
      fprintf(fp,"); }\n");

      wrappedFunctions[numberOfWrappedFunctions] = currentFunction;
      numberOfWrappedFunctions++;
      }
    }
  free(beanfunc);
}

/* print the parsed structures */
void vtkParseOutput(FILE *fp, FileInfo *data)
{
  int i;

  fprintf(fp,"// java wrapper for %s object\n//\n",data->ClassName);
  fprintf(fp,"\npackage vtk;\n");

  /* beans */
  if (!data->NumberOfSuperClasses)
    {
    fprintf(fp,"import java.beans.*;\n");
    }

if (strcmp("vtkObject",data->ClassName))
    {
    fprintf(fp,"import vtk.*;\n");
    }
  fprintf(fp,"\npublic class %s",data->ClassName);
  if (strcmp("vtkObject",data->ClassName))
    {
    if (data->NumberOfSuperClasses)
      fprintf(fp," extends %s",data->SuperClasses[0]);
    }
  fprintf(fp,"\n{\n");

  fprintf(fp,"  public %s getThis%s() { return this;}\n\n",
          data->ClassName, data->ClassName+3);

  /* insert function handling code here */
  for (i = 0; i < data->NumberOfFunctions; i++)
    {
    currentFunction = data->Functions + i;
    outputFunction(fp, data);
    }

if (!data->NumberOfSuperClasses)
    {
    fprintf(fp,"\n  public %s() { this.VTKInit();};\n",data->ClassName);
    fprintf(fp,"  protected int vtkId = 0;\n");

    /* beans */
    fprintf(fp,"  public void addPropertyChangeListener(PropertyChangeListener l)\n    {\n");
    fprintf(fp,"    changes.addPropertyChangeListener(l);\n    }\n");
    fprintf(fp,"  public void removePropertyChangeListener(PropertyChangeListener l)\n    {\n");
    fprintf(fp,"    changes.removePropertyChangeListener(l);\n    }\n");
    fprintf(fp,"  protected PropertyChangeSupport changes = new PropertyChangeSupport(this);\n\n");

    /* if we are a base class and have a delete method */
    if (data->HasDelete)
      {
      fprintf(fp,"\n  public native void VTKDelete();\n");
      fprintf(fp,"  protected void finalize() { this.VTKDelete();};\n");
      }
    }
  if ((!data->IsAbstract)&&
      strcmp(data->ClassName,"vtkDataWriter") &&
      strcmp(data->ClassName,"vtkPointSet") &&
      strcmp(data->ClassName,"vtkDataSetSource")
      )
    {
    fprintf(fp,"  public native void   VTKInit();\n");
    }
  if (!strcmp("vtkObject",data->ClassName))
    {
    fprintf(fp,"  public native String Print();\n");
    }
  fprintf(fp,"}\n");
}

