/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParseJavaBeans.c
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
  
  if (currentFunction->ArgTypes[i]%1000 == 303)
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
      (currentFunction->ArgTypes[i]%1000 != 303)&&
      (currentFunction->ArgTypes[i]%1000 != 309))
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
	    !(((fi->ArgTypes[j]%1000 == 309)&&
	       (currentFunction->ArgTypes[j]%1000 == 109)) ||
	      ((fi->ArgTypes[j]%1000 == 109)&&
	       (currentFunction->ArgTypes[j]%1000 == 309)) ||
	      ((fi->ArgTypes[j]%1000 == 301)&&
	       (currentFunction->ArgTypes[j]%1000 == 307)) ||
	      ((fi->ArgTypes[j]%1000 == 307)&&
	       (currentFunction->ArgTypes[j]%1000 == 301)) ||
	      ((fi->ArgTypes[j]%1000 == 304)&&
	       (currentFunction->ArgTypes[j]%1000 == 306)) ||
	      ((fi->ArgTypes[j]%1000 == 306)&&
	       (currentFunction->ArgTypes[j]%1000 == 304)) ||
	      ((fi->ArgTypes[j]%1000 == 1)&&
	       (currentFunction->ArgTypes[j]%1000 == 7)) ||
	      ((fi->ArgTypes[j]%1000 == 7)&&
	       (currentFunction->ArgTypes[j]%1000 == 1)) ||
	      ((fi->ArgTypes[j]%1000 == 4)&&
	       (currentFunction->ArgTypes[j]%1000 == 6)) ||
	      ((fi->ArgTypes[j]%1000 == 6)&&
	       (currentFunction->ArgTypes[j]%1000 == 4))))
	  {
	  match = 0;
	  }
	else
	  {
	  if (fi->ArgTypes[j]%1000 == 309 || fi->ArgTypes[j]%1000 == 109)
	    {
	    if (strcmp(fi->ArgClasses[j],currentFunction->ArgClasses[j]))
	      {
	      match = 0;
	      }
	    }
	  }
	}
      if ((fi->ReturnType != currentFunction->ReturnType) &&
	  !(((fi->ReturnType%1000 == 309)&&
	     (currentFunction->ReturnType%1000 == 109)) ||
	    ((fi->ReturnType%1000 == 109)&&
	     (currentFunction->ReturnType%1000 == 309)) ||
	    ((fi->ReturnType%1000 == 301)&&
	     (currentFunction->ReturnType%1000 == 307)) ||
	    ((fi->ReturnType%1000 == 307)&&
	     (currentFunction->ReturnType%1000 == 301)) ||
	    ((fi->ReturnType%1000 == 304)&&
	     (currentFunction->ReturnType%1000 == 306)) ||
	    ((fi->ReturnType%1000 == 306)&&
	     (currentFunction->ReturnType%1000 == 304)) ||
	    ((fi->ReturnType%1000 == 1)&&
	     (currentFunction->ReturnType%1000 == 7)) ||
	    ((fi->ReturnType%1000 == 7)&&
	     (currentFunction->ReturnType%1000 == 1)) ||
	    ((fi->ReturnType%1000 == 4)&&
	     (currentFunction->ReturnType%1000 == 6)) ||
	    ((fi->ReturnType%1000 == 6)&&
	     (currentFunction->ReturnType%1000 == 4))))
	{
	match = 0;
	}
      else
	{
	if (fi->ReturnType%1000 == 309 || fi->ReturnType%1000 == 109)
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
    if (currentFunction->ArgTypes[i]%1000 == 9) args_ok = 0;
    if ((currentFunction->ArgTypes[i]%10) == 8) args_ok = 0;
    if (((currentFunction->ArgTypes[i]%1000)/100 != 3)&&
	(currentFunction->ArgTypes[i]%1000 != 109)&&
	((currentFunction->ArgTypes[i]%1000)/100)) args_ok = 0;
    if (currentFunction->ArgTypes[i]%1000 == 313) args_ok = 0;
    if (currentFunction->ArgTypes[i]%1000 == 314) args_ok = 0;
    if (currentFunction->ArgTypes[i]%1000 == 315) args_ok = 0;
    if (currentFunction->ArgTypes[i]%1000 == 316) args_ok = 0;
    }
  if ((currentFunction->ReturnType%10) == 8) args_ok = 0;
  if (currentFunction->ReturnType%1000 == 9) args_ok = 0;
  if (((currentFunction->ReturnType%1000)/100 != 3)&&
      (currentFunction->ReturnType%1000 != 109)&&
      ((currentFunction->ReturnType%1000)/100)) args_ok = 0;


  /* eliminate unsigned char * and unsigned short * */
  if (currentFunction->ReturnType%1000 == 313) args_ok = 0;
  if (currentFunction->ReturnType%1000 == 314) args_ok = 0;
  if (currentFunction->ReturnType%1000 == 315) args_ok = 0;
  if (currentFunction->ReturnType%1000 == 316) args_ok = 0;

  if (currentFunction->NumberOfArguments && 
      (currentFunction->ArgTypes[0] == 5000)
      &&(currentFunction->NumberOfArguments != 1)) args_ok = 0;

  /* make sure we have all the info we need for array arguments in */
  for (i = 0; i < currentFunction->NumberOfArguments; i++)
    {
    if (((currentFunction->ArgTypes[i]%1000)/100 == 3)&&
	(currentFunction->ArgCounts[i] <= 0)&&
	(currentFunction->ArgTypes[i]%1000 != 309)&&
	(currentFunction->ArgTypes[i]%1000 != 303)) args_ok = 0;
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

      /* stick in secret beanie code for set methods */
      if (currentFunction->ReturnType%1000 == 2)
	{
	/* only care about set methods and On/Off methods */
	if (!strncmp(beanfunc,"set",3) && 
	    currentFunction->NumberOfArguments == 1 && 
	    (currentFunction->ArgTypes[0]%1000 < 10 || 
	     currentFunction->ArgTypes[0]%1000 == 303 || 
	     currentFunction->ArgTypes[0]%10 == 9))
	  {
	  char prop[256];
	  
	  strncpy(prop,beanfunc+3,strlen(beanfunc)-3);
	  prop[strlen(beanfunc)-3] = '\0';
	  if (isupper(prop[0])) prop[0] = prop[0] + 32;
	  fprintf(fp,");\n      changes.firePropertyChange(\"%s\",null,",prop);
	  
	  /* handle basic types */
	  if (currentFunction->ArgTypes[0]%1000 == 303)
	    {
	    fprintf(fp," id0");
	    }
	  else
	    {
	    switch (currentFunction->ArgTypes[0]%10)
	      {
	      case 1:
	      case 7:   fprintf(fp," new Double(id0)"); break;
	      case 4:   
	      case 5:   
	      case 6:   fprintf(fp," new Integer(id0)"); break;
	      case 9:   fprintf(fp," id0"); break;
	      case 3:   /* not implemented yet */ 
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

