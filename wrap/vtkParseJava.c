/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParseJava.c
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/

#include <stdio.h>
#include "vtkParse.h"

int numberOfWrappedFunctions = 0;
FunctionInfo *wrappedFunctions[1000];
FunctionInfo *currentFunction;

void output_temp(FILE *fp,int i)
{
  /* ignore void */
  if (((currentFunction->ArgTypes[i] % 10) == 2)&&
      (!((currentFunction->ArgTypes[i]%1000)/100)))
    {
    return;
    }
  
  if (currentFunction->ArgTypes[i] == 5000)
    {
    fprintf(fp,"Object id0, String id1");  
    return;
    }
  
  if (currentFunction->ArgTypes[i] == 303)
    {
    fprintf(fp,"String ");
    }
  else
    {
    switch (currentFunction->ArgTypes[i]%10)
      {
      case 1:   fprintf(fp,"double "); break;
      case 7:   fprintf(fp,"double "); break;
      case 4:   fprintf(fp,"int "); break;
      case 5:   fprintf(fp,"int "); break;
      case 6:   fprintf(fp,"int "); break;
      case 2:     fprintf(fp,"void "); break;
      case 3:     fprintf(fp,"char "); break;
      case 9:     fprintf(fp,"%s ",currentFunction->ArgClasses[i]); break;
      case 8: return;
      }
    }

  fprintf(fp,"id%i",i);
  if (((currentFunction->ArgTypes[i]%1000)/100 == 3)&&
      (currentFunction->ArgTypes[i] != 303)&&
      (currentFunction->ArgTypes[i] != 309))
    {
    fprintf(fp,"[]");
    }
}

void return_result(FILE *fp)
{
  switch (currentFunction->ReturnType%1000)
    {
    case 1: fprintf(fp,"double "); break;
    case 2: fprintf(fp,"void "); break;
    case 3: fprintf(fp,"char "); break;
    case 7: fprintf(fp,"double "); break;
    case 4: case 5: case 6: case 13: case 14: case 15: case 16:
      fprintf(fp,"int "); 
      break;
    case 303: fprintf(fp,"String "); break;
    case 109:  
    case 309:  
      fprintf(fp,"%s ",currentFunction->ReturnClass);
      break;
      
      /* handle functions returning vectors */
      /* this is done by looking them up in a hint file */
    case 301: case 307:
      fprintf(fp,"double[] "); 
      break;
    case 304: case 305: case 306: case 313: case 314: case 315: case 316:
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
	    !(((fi->ArgTypes[j] == 309)&&
	       (currentFunction->ArgTypes[j] == 109)) ||
	      ((fi->ArgTypes[j] == 109)&&
	       (currentFunction->ArgTypes[j] == 309))))
	  {
	  match = 0;
	  }
	else
	  {
	  if (fi->ArgTypes[j] == 309 || fi->ArgTypes[j] == 109)
	    {
	    if (strcmp(fi->ArgClasses[j],currentFunction->ArgClasses[j]))
	      {
	      match = 0;
	      }
	    }
	  }
	}
      if ((fi->ReturnType != currentFunction->ReturnType) &&
	  !(((fi->ReturnType == 309)&&(currentFunction->ReturnType == 109)) ||
	    ((fi->ReturnType == 109)&&(currentFunction->ReturnType == 309))))
	{
	match = 0;
	}
      else
	{
	if (fi->ReturnType == 309 || fi->ReturnType == 109)
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
 
  /* some functions will not get wrapped no matter what else */
  if (currentFunction->IsPureVirtual ||
      currentFunction->IsOperator || 
      currentFunction->ArrayFailure ||
      !currentFunction->IsPublic ||
      !currentFunction->Name) 
    {
    return;
    }
  
  /* check to see if we can handle the args */
  for (i = 0; i < currentFunction->NumberOfArguments; i++)
    {
    if (currentFunction->ArgTypes[i] == 9) args_ok = 0;
    if ((currentFunction->ArgTypes[i]%10) == 8) args_ok = 0;
    if (((currentFunction->ArgTypes[i]%1000)/100 != 3)&&
	(currentFunction->ArgTypes[i]%1000 != 109)&&
	((currentFunction->ArgTypes[i]%1000)/100)) args_ok = 0;
    if (currentFunction->ArgTypes[i] == 313) args_ok = 0;
    if (currentFunction->ArgTypes[i] == 314) args_ok = 0;
    if (currentFunction->ArgTypes[i] == 315) args_ok = 0;
    if (currentFunction->ArgTypes[i] == 316) args_ok = 0;
    }
  if ((currentFunction->ReturnType%10) == 8) args_ok = 0;
  if (currentFunction->ReturnType == 9) args_ok = 0;
  if (((currentFunction->ReturnType%1000)/100 != 3)&&
      (currentFunction->ReturnType%1000 != 109)&&
      ((currentFunction->ReturnType%1000)/100)) args_ok = 0;


  /* eliminate unsigned char * and unsigned short * */
  if (currentFunction->ReturnType == 313) args_ok = 0;
  if (currentFunction->ReturnType == 314) args_ok = 0;
  if (currentFunction->ReturnType == 315) args_ok = 0;
  if (currentFunction->ReturnType == 316) args_ok = 0;

  if (currentFunction->NumberOfArguments && 
      (currentFunction->ArgTypes[0] == 5000)
      &&(currentFunction->NumberOfArguments != 1)) args_ok = 0;

  /* make sure we have all the info we need for array arguments in */
  for (i = 0; i < currentFunction->NumberOfArguments; i++)
    {
    if (((currentFunction->ArgTypes[i]%1000)/100 == 3)&&
	(currentFunction->ArgCounts[i] <= 0)&&
	(currentFunction->ArgTypes[i] != 309)&&
	(currentFunction->ArgTypes[i] != 303)) args_ok = 0;
    }

  /* if we need a return type hint make sure we have one */
  switch (currentFunction->ReturnType%1000)
    {
    case 301: case 302: case 307:
    case 304: case 305: case 306:
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
      fprintf(fp,"%s(",currentFunction->Name);
      
      for (i = 0; i < currentFunction->NumberOfArguments; i++)
	{
	if (i)
	  {
	  fprintf(fp,",");
	  }
	output_temp(fp,i);
	}
      /* if not void then need return otherwise none */
      if (currentFunction->ReturnType%1000 == 2)
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
	  (currentFunction->ArgTypes[0] == 5000)) fprintf(fp,",id1");
      fprintf(fp,"); }\n");
      
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
  fprintf(fp,"}\n");
}

