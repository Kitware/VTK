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
      case 0x2:     fprintf(fp,"void "); break;
      case 0xD:   fprintf(fp,"char "); break;
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
    case 0x4: case 0x5: case 0x6: case 0xA: case 0xB: case 0xC:
    case 0x13: case 0x14: case 0x15: case 0x16: case 0x1A: case 0x1B: case 0x1C:
      fprintf(fp,"int "); 
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
    case 0x304: case 0x305: case 0x306: case 0x30A: case 0x30B: case 0x30C: case 0x30D:
    case 0x313: case 0x314: case 0x315: case 0x316: case 0x31A: case 0x31B: case 0x31C:
      fprintf(fp,"int[]  "); break;
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

void outputFunction(FILE *fp, FileInfo *data)
{
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
  if (currentFunction->ReturnType % 0x1000 == 0x313) args_ok = 0;
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
    case 0x30A: case 0x30B: case 0x30C:
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

      /* stick in secret beanie code for set methods */
      if (currentFunction->ReturnType % 0x1000 == 0x2)
        {
        /* only care about set methods and On/Off methods */
        if (!strncmp(beanfunc,"set",3) && 
            currentFunction->NumberOfArguments == 1 && 
            (currentFunction->ArgTypes[0] % 0x1000 < 0x10 || 
             currentFunction->ArgTypes[0] % 0x1000 == 0x303 || 
             currentFunction->ArgTypes[0] % 0x10 == 0x9))
          {
          char prop[256];
          
          strncpy(prop,beanfunc+3,strlen(beanfunc)-3);
          prop[strlen(beanfunc)-3] = '\0';
          if (isupper(prop[0])) prop[0] = prop[0] + 32;
          fprintf(fp,");\n      changes.firePropertyChange(\"%s\",null,",prop);
          
          /* handle basic types */
          if (currentFunction->ArgTypes[0] % 0x1000 == 0x303)
            {
            fprintf(fp," id0");
            }
          else
            {
            switch (currentFunction->ArgTypes[0] % 0x10)
              {
              case 0x1:
              case 0x7:   fprintf(fp," new Double(id0)"); break;
              case 0x4:   
              case 0x5:   
              case 0x6:   fprintf(fp," new Integer(id0)"); break;
              case 0x9:   fprintf(fp," id0"); break;
              case 0x3:   /* not implemented yet */ 
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

