/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWrapPython.c
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
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "vtkParse.h"

int numberOfWrappedFunctions = 0;
FunctionInfo *wrappedFunctions[1000];
extern FunctionInfo *currentFunction;

static int class_has_new = 0;

/* when the cpp file doesn't have enough info use the hint file */
void use_hints(FILE *fp)
{
  int  i;
  
  switch (currentFunction->ReturnType%1000)
    {
    case 301:
      fprintf(fp,"    return Py_BuildValue(\"");
      for (i = 0; i < currentFunction->HintSize; i++) fprintf(fp,"f");
      fprintf(fp,"\"");
      for (i = 0; i < currentFunction->HintSize; i++) 
	{
	fprintf(fp,",temp%i[%d]",MAX_ARGS,i);
	}
      fprintf(fp,");\n");
      break;
    case 307:  
      fprintf(fp,"    return Py_BuildValue(\"");
      for (i = 0; i < currentFunction->HintSize; i++) fprintf(fp,"d");
      fprintf(fp,"\"");
      for (i = 0; i < currentFunction->HintSize; i++) 
	{
	fprintf(fp,",temp%i[%d]",MAX_ARGS,i);
	}
      fprintf(fp,");\n");
      break;
    case 304: 
      fprintf(fp,"    return Py_BuildValue(\"");
      for (i = 0; i < currentFunction->HintSize; i++) fprintf(fp,"i");
      fprintf(fp,"\"");
      for (i = 0; i < currentFunction->HintSize; i++) 
	{
	fprintf(fp,",temp%i[%d]",MAX_ARGS,i);
	}
      fprintf(fp,");\n");
      break;
    case 305: case 306: case 313: case 314: case 315: case 316:
      break;
    }
  return;
}


void output_temp(FILE *fp, int i, int aType, char *Id, int aCount)
{
  /* handle VAR FUNCTIONS */
  if (aType == 5000)
    {
    fprintf(fp,"  PyObject *temp%i;\n",i); 
    return;
    }
  
  if (((aType % 10) == 2)&&(!((aType%1000)/100)))
    {
    return;
    }

  /* for const * return types prototype with const */
  if ((i == MAX_ARGS)&&(aType%2000 >= 1000))
    {
    fprintf(fp,"  const ");
    }
  else
    {
    fprintf(fp,"  ");
    }
  
  if ((aType%100)/10 == 1)
    {
    fprintf(fp,"unsigned ");
    }
  
  switch (aType%10)
    {
    case 1:   fprintf(fp,"float  "); break;
    case 7:   fprintf(fp,"double "); break;
    case 4:   fprintf(fp,"int    "); break;
    case 5:   fprintf(fp,"short  "); break;
    case 6:   fprintf(fp,"long   "); break;
    case 2:     fprintf(fp,"void   "); break;
    case 3:     fprintf(fp,"char   "); break;
    case 9:     
      fprintf(fp,"%s ",Id); break;
    case 8: return;
    }
  
  switch ((aType%1000)/100)
    {
    case 1: fprintf(fp, " *"); break; /* act " &" */
    case 2: fprintf(fp, "&&"); break;
    case 3: 
      if ((i == MAX_ARGS)||(aType%10 == 9)||(aType%1000 == 303)
	  ||(aType%1000 == 302))
	{
	fprintf(fp, " *"); 
	}
      break;
    case 4: fprintf(fp, "&*"); break;
    case 5: fprintf(fp, "*&"); break;
    case 7: fprintf(fp, "**"); break;
    default: fprintf(fp,"  "); break;
    }
  fprintf(fp,"temp%i",i);
  
  /* handle arrays */
  if ((aType%1000/100 == 3)&&
      (i != MAX_ARGS)&&(aType%10 != 9)&&(aType%1000 != 303)
      &&(aType%1000 != 302))
    {
    fprintf(fp,"[%i]",aCount);
    }

  fprintf(fp,";\n");
  if (aType%1000 == 302 && i != MAX_ARGS)
    {
    fprintf(fp,"  int      size%d;\n",i);
    }
  if ((i != MAX_ARGS) && ((aType%1000 == 309)||(aType%1000 == 109)))
    {
    fprintf(fp,"  PyObject *tempH%d;\n",i);
    }
}

void do_return(FILE *fp)
{
  /* ignore void */
  if (((currentFunction->ReturnType % 10) == 2)&&
      (!((currentFunction->ReturnType%1000)/100)))
    {
    fprintf(fp,"    Py_INCREF(Py_None);\n");
    fprintf(fp,"    return Py_None;\n");
    return;
    }
  
  switch (currentFunction->ReturnType%1000)
    {
    case 303:
      fprintf(fp,"    if (temp%i == NULL) {\n",MAX_ARGS);
      fprintf(fp,"      Py_INCREF(Py_None);\n");
      fprintf(fp,"      return Py_None;\n      }\n");
      fprintf(fp,"    else {\n");
      fprintf(fp,"      return PyString_FromString(temp%i);\n      }\n",MAX_ARGS);
    break;
    case 109:
    case 309:  
      {
      fprintf(fp,"    return vtkPythonGetObjectFromPointer((vtkObject *)temp%i);\n",
	      MAX_ARGS);
      break;
      }
      
    /* handle functions returning vectors */
    /* this is done by looking them up in a hint file */
    case 301: case 307:
    case 304: case 305: case 306:
      use_hints(fp);
      break;
    case 302:
      {
      fprintf(fp,"    if (temp%i == NULL)\n        {\n",MAX_ARGS);
      fprintf(fp,"      Py_INCREF(Py_None);\n");
      fprintf(fp,"      return Py_None;\n        }\n");
      fprintf(fp,"    else\n        {\n");
      fprintf(fp,"      return PyString_FromString(vtkPythonManglePointer(temp%i,\"void_p\"));\n        }\n",
	      MAX_ARGS);
      break;
      }
    case 1:
    case 7:
      {
      fprintf(fp,"    return PyFloat_FromDouble(temp%i);\n",
		      MAX_ARGS);
      break;
      }
    case 13:
    case 14:
    case 15:
    case 4:
    case 5:
    case 6:
      {
      fprintf(fp,"    return PyInt_FromLong(temp%i);\n", MAX_ARGS); 
      break;
      }
    case 16:   
      {
      fprintf(fp,"    return PyInt_FromLong((long)temp%i);\n",
	      MAX_ARGS); 
      break;
      }
    case 3:   
      {
      fprintf(fp,"    return PyString_FromStringAndSize((char *)&temp%i,1);\n",
	      MAX_ARGS);
      break;
      }
    }
}

char *get_format_string()
{
  static char result[1024];
  int currPos = 0;
  int argtype;
  int i, j;
  
  if (currentFunction->ArgTypes[0] == 5000)
    {
    result[currPos] = 'O'; currPos++; 
    result[currPos] = '\0';
    return result;
    }
  
  for (i = 0; i < currentFunction->NumberOfArguments; i++)
    {
    argtype = currentFunction->ArgTypes[i]%1000;

    switch (argtype)
      {
      case 301:
	result[currPos] = '('; currPos++;
	for (j = 0; j < currentFunction->ArgCounts[i]; j++) 
	  {
	  result[currPos] = 'f'; currPos++;
	  }
	result[currPos] = ')'; currPos++;
	break;
      case 307:  
	result[currPos] = '('; currPos++;
	for (j = 0; j < currentFunction->ArgCounts[i]; j++) 
	  {
	  result[currPos] = 'd'; currPos++;
	  }
	result[currPos] = ')'; currPos++;
	break;
      case 304: 
	result[currPos] = '('; currPos++;
	for (j = 0; j < currentFunction->ArgCounts[i]; j++) 
	  {
	  result[currPos] = 'i'; currPos++;
	  }
	result[currPos] = ')'; currPos++;
	break;
      case 109:
      case 309: result[currPos] = 'O'; currPos++; break;
      case 303: result[currPos] = 'z'; currPos++; break;
      case 302: result[currPos] = 's'; currPos++; 
                result[currPos] = '#'; currPos++; break; 
      case 1:   result[currPos] = 'f'; currPos++; break;
      case 7:   result[currPos] = 'd'; currPos++; break;
      case 14:
      case 4:   result[currPos] = 'i'; currPos++; break;
      case 15:
      case 5:   result[currPos] = 'h'; currPos++; break;
      case 16:
      case 6:   result[currPos] = 'l'; currPos++; break;
      case 3:   result[currPos] = 'c'; currPos++; break;
      case 13:   result[currPos] = 'b'; currPos++; break;
      }
    }

  result[currPos] = '\0';
  return result;
}

static void add_to_sig(char *sig, char *add, int *i)
{
  strcpy(&sig[*i],add);
  *i += strlen(add);
}

void get_python_signature()
{
  static char result[1024];
  int currPos = 0;
  int argtype;
  int i, j;

  /* print out the name of the method */
  add_to_sig(result,"V.",&currPos);
  add_to_sig(result,currentFunction->Name,&currPos);

  /* print the arg list */
  add_to_sig(result,"(",&currPos);
  
  for (i = 0; i < currentFunction->NumberOfArguments; i++)
    {
    if (currentFunction->ArgTypes[i] == 5000)
      {
      add_to_sig(result,"function",&currPos); 
      }
    
    argtype = currentFunction->ArgTypes[i]%1000;

    if (i != 0)
      {
      add_to_sig(result,", ",&currPos);
      }

    switch (argtype)
      {
      case 301:
      case 307:
	add_to_sig(result,"(",&currPos);
	for (j = 0; j < currentFunction->ArgCounts[i]; j++) 
	  {
	  if (j != 0)
	    {
	    add_to_sig(result,", ",&currPos);
	    }
	  add_to_sig(result,"float",&currPos);
	  }
	add_to_sig(result,")",&currPos);
	break;
      case 304: 
	add_to_sig(result,"(",&currPos);
	for (j = 0; j < currentFunction->ArgCounts[i]; j++) 
	  {
	  if (j != 0)
	    {
	    add_to_sig(result,", ",&currPos);
	    }
	  add_to_sig(result,"int",&currPos);
	  }
	add_to_sig(result,")",&currPos);
	break;
      case 109:
      case 309: add_to_sig(result,currentFunction->ArgClasses[i],&currPos); break;
      case 302:
      case 303: add_to_sig(result,"string",&currPos); break;
      case 1:
      case 7:   add_to_sig(result,"float",&currPos); break;
      case 14:
      case 4:
      case 15:
      case 5:
      case 16:
      case 6:   add_to_sig(result,"int",&currPos); break;
      case 3:   add_to_sig(result,"char",&currPos); break;
      case 13:  add_to_sig(result,"int",&currPos); break;
      }
    }

  add_to_sig(result,")",&currPos);

  /* if this is a void method, we are finished */
  /* otherwise, print "->" and the return type */
  if ((!((currentFunction->ReturnType % 10) == 2)) ||
      ((currentFunction->ReturnType%1000)/100))
    {
    add_to_sig(result," -> ",&currPos);

    switch (currentFunction->ReturnType%1000)
      {
      case 302:
      case 303: add_to_sig(result,"string",&currPos); break;
      case 109:
      case 309: add_to_sig(result,currentFunction->ReturnClass,&currPos); break;
      case 301:
      case 307:
	add_to_sig(result,"(",&currPos);
	for (j = 0; j < currentFunction->HintSize; j++) 
	  {
	  if (j != 0)
	    {
	    add_to_sig(result,", ",&currPos);
	    }
	  add_to_sig(result,"float",&currPos);
	  }
	add_to_sig(result,")",&currPos);
	break;
      case 304: 
	add_to_sig(result,"(",&currPos);
	for (j = 0; j < currentFunction->HintSize; j++) 
	  {
	  if (j != 0)
	    {
	    add_to_sig(result,", ",&currPos);
	    }
	  add_to_sig(result,"int",&currPos);
	  }
	add_to_sig(result,")",&currPos);
	break;
      case 1:
      case 7: add_to_sig(result,"float",&currPos); break;
      case 13:
      case 14:
      case 15:
      case 16:
      case 4:
      case 5:
      case 6: add_to_sig(result,"int",&currPos); break;
      case 3: add_to_sig(result,"char",&currPos); break;
      }
    }
  
  if (currentFunction->Signature)
    {
    add_to_sig(result,"\\nC++: ",&currPos);
    add_to_sig(result,currentFunction->Signature,&currPos);
    }

  currentFunction->Signature = realloc(currentFunction->Signature,currPos+1);
  strcpy(currentFunction->Signature,result);
  /* fprintf(stderr,"%s\n",currentFunction->Signature); */
}

/* convert special characters in a string into their escape codes,
   so that the string can be quoted in a source file (the specified
   maxlen must be at least 32 chars)*/
static const char *quote_string(const char *comment, int maxlen)
{
  static char result[4096];
  int i, j, n;

  if (maxlen > 4096)
    {
    maxlen = 4096;
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
      j += strlen(" ...\\n [Truncated]\\n");
      break;
      }
    }
  result[j] = '\0';

  return result;
}
  

void outputFunction2(FILE *fp, FileInfo *data)
{
  int i, j, k, is_static, is_vtkobject, fnum, occ, backnum, goto_used;
  FunctionInfo *theFunc;
  FunctionInfo *backFunc;

  is_vtkobject = ((strcmp(data->ClassName,"vtkObject") == 0) || 
		  (data->NumberOfSuperClasses != 0));

  /* create a python-type signature for each method (for use in docstring) */
  for (fnum = 0; fnum < numberOfWrappedFunctions; fnum++)
    {
    theFunc = wrappedFunctions[fnum];
    currentFunction = theFunc;
    get_python_signature();
    }

  /* create external type declarations for all object
     return types */
  for (fnum = 0; fnum < numberOfWrappedFunctions; fnum++)
    {
    theFunc = wrappedFunctions[fnum];
    currentFunction = theFunc;

    /* check for object return types */
    if ((theFunc->ReturnType%1000 == 309)||
	(theFunc->ReturnType%1000 == 109))
      {
      /* check that we haven't done this type (no duplicate declarations) */
      for (backnum = fnum-1; backnum >= 0; backnum--) 
	{
        backFunc = wrappedFunctions[backnum];
	if (((backFunc->ReturnType%1000 == 309)||
	     (backFunc->ReturnType%1000 == 109)) &&
	    (strcmp(theFunc->ReturnClass,backFunc->ReturnClass) == 0))
	  {
	  break;
	  }
	}
      }
    }

  /* for each function in the array */
  for (fnum = 0; fnum < numberOfWrappedFunctions; fnum++)
    {
    /* make sure we haven't already done one of these */
    theFunc = wrappedFunctions[fnum];
    currentFunction = theFunc;

    if (theFunc->Name)
      {
      fprintf(fp,"\n");
      
      fprintf(fp,"static PyObject *Py%s_%s(PyObject *self, PyObject *args)\n",
	      data->ClassName,currentFunction->Name);
      fprintf(fp,"{\n");
      
      /* find all occurances of this method */
      for (occ = fnum; occ < numberOfWrappedFunctions; occ++)
	{
	goto_used = 0;
	is_static = 0;

	/* is it the same name */
	if (wrappedFunctions[occ]->Name && 
	    !strcmp(theFunc->Name,wrappedFunctions[occ]->Name))
	  {
	  /* check for static methods */
	  if (((wrappedFunctions[occ]->ReturnType/1000) & 2) == 2)
	    {
	    is_static = 1;
	    }

	  fprintf(fp,"  /* handle an occurrence */\n  {\n");
	  /* declare the variables */
	  if (!is_static)
	    {
	    if (is_vtkobject)
	      {
	      fprintf(fp,"  %s *op;\n\n",data->ClassName);
	      }
	    else 
	      {
	      fprintf(fp,"  %s *op = (%s *)((PyVTKSpecialObject *)self)->vtk_ptr;\n\n",data->ClassName,data->ClassName);
	      }
	    }

	  currentFunction = wrappedFunctions[occ];
	  /* process the args */
	  for (i = 0; i < currentFunction->NumberOfArguments; i++)
	    {
	    output_temp(fp, i, currentFunction->ArgTypes[i],
			currentFunction->ArgClasses[i],
			currentFunction->ArgCounts[i]);
	    }
	  output_temp(fp, MAX_ARGS,currentFunction->ReturnType,
		      currentFunction->ReturnClass,0);
	  /* don't clear error first time around */
	  if (occ != fnum)
	    {
	    fprintf(fp,"  PyErr_Clear();\n");
	    }
	  if (is_static || !is_vtkobject)
	    {
            fprintf(fp,"  if ((PyArg_ParseTuple(args, \"%s\"",
		    get_format_string());
	    }
	  else
	    {
            fprintf(fp,"  if ((op = (%s *)PyArg_VTKParseTuple(self, args, \"%s\"",
		    data->ClassName,get_format_string());
	    }
	  for (i = 0; i < currentFunction->NumberOfArguments; i++)
	    {
	    if ((currentFunction->ArgTypes[i]%1000 == 309)||
		(currentFunction->ArgTypes[i]%1000 == 109))
	      {
	      fprintf(fp,", &tempH%d",i);
	      }
            else if (currentFunction->ArgTypes[i]%1000 == 302)
              {
              fprintf(fp,", &temp%d, &size%d",i,i);
              }
	    else
	      {
	      if (currentFunction->ArgCounts[i])
		{
		for (j = 0; j < currentFunction->ArgCounts[i]; j++)
		  {
		  fprintf(fp,", temp%d + %d",i,j);
		  }
		}
	      else
		{
		fprintf(fp,", &temp%d",i);
		}
	      }
	    }
	  fprintf(fp,")))\n    {\n");

	  /* lookup and required objects */
	  for (i = 0; i < currentFunction->NumberOfArguments; i++)
	    {
	    if ((currentFunction->ArgTypes[i]%1000 == 309)||
		(currentFunction->ArgTypes[i]%1000 == 109))
	      {
	      fprintf(fp,"    temp%d = (%s *)vtkPythonGetPointerFromObject(tempH%d,\"%s\");\n",
		      i, currentFunction->ArgClasses[i], i, 
		      currentFunction->ArgClasses[i]);
	      fprintf(fp,"    if (!temp%d && tempH%d != Py_None) goto break%d;\n",i,i,occ);
	      goto_used = 1;
	      }
	    }
	  
	  /* make sure passed method is callable  for VAR functions */
	  if (currentFunction->NumberOfArguments == 1 &&
	      currentFunction->ArgTypes[0] == 5000)
	    {
	    fprintf(fp,"    if (!PyCallable_Check(temp0) && temp0 != Py_None)\n");
	    fprintf(fp,"      {\n      PyErr_SetString(PyExc_ValueError,\"vtk callback method passed to %s in %s was not callable.\");\n",
		    currentFunction->Name,data->ClassName);
	    fprintf(fp,"      return NULL;\n      }\n");
	    fprintf(fp,"    Py_INCREF(temp0);\n");
	    }
	  
	  /* check for void pointers and pass appropriate info*/
	  for (i = 0; i < currentFunction->NumberOfArguments; i++)
	    {
	    if (currentFunction->ArgTypes[i]%1000 == 302)
	      {
	      fprintf(fp,"    temp%i = vtkPythonUnmanglePointer((char *)temp%i,&size%i,\"%s\");\n",i,i,i,"void_p");
	      fprintf(fp,"    if (size%i == -1) {\n      PyErr_SetString(PyExc_ValueError,\"mangled pointer to %s in %s was of incorrect type.\");\n",
                      i,currentFunction->Name,data->ClassName);
	      fprintf(fp,"      return NULL;\n      }\n");
              fprintf(fp,"    else if (size%i == -2) {\n      PyErr_SetString(PyExc_ValueError,\"mangled pointer to %s in %s was poorly formed.\");\n",
		      i,currentFunction->Name,data->ClassName);
	      fprintf(fp,"      return NULL;\n      }\n"); 
	      }
	    }

	  for (k = 0; k < (2 - (is_static || !is_vtkobject)); k++)
	    {
	    char methodname[256]; 
	    if (k == 0)
	      {
	      if (is_static)
		{
		fprintf(fp,"      {\n");
		sprintf(methodname,"%s::%s",
			data->ClassName,currentFunction->Name);
		}
	      else if (!is_vtkobject)
		{
		fprintf(fp,"      {\n");
		sprintf(methodname,"op->%s",currentFunction->Name);
		}
	      else
		{
		fprintf(fp,"    if (PyVTKClass_Check(self)) {\n");
		sprintf(methodname,"op->%s::%s",
			data->ClassName,currentFunction->Name);
		}
	      }
	    else
	      {
	      fprintf(fp,"    else {\n");
	      sprintf(methodname,"op->%s",currentFunction->Name);
	      }
		
	    switch (currentFunction->ReturnType%1000)
	      {
	      case 2:
		fprintf(fp,"      %s(",methodname);
		break;
	      case 109:
		fprintf(fp,"      temp%i = &%s(",MAX_ARGS,methodname);
		break;
	      default:
		fprintf(fp,"      temp%i = %s(",MAX_ARGS,methodname);
	      }

	    for (i = 0; i < currentFunction->NumberOfArguments; i++)
	      {
	      if (i)
		{
	        fprintf(fp,",");
		}
	      if (currentFunction->ArgTypes[i]%1000 == 109)
		{
	        fprintf(fp,"*(temp%i)",i);
		}
	      else if (currentFunction->NumberOfArguments == 1 
		       && currentFunction->ArgTypes[i] == 5000)
		{
	        fprintf(fp,"((temp0 != Py_None) ? vtkPythonVoidFunc : NULL),(void *)temp%i",i);
		}
	      else
		{
	        fprintf(fp,"temp%i",i);
		}
	      }
	    fprintf(fp,");\n");
	  
	    if (currentFunction->NumberOfArguments == 1 
		&& currentFunction->ArgTypes[0] == 5000)
	      {
	      fprintf(fp,"      %sArgDelete(vtkPythonVoidFuncArgDelete);\n",
		      methodname);
	      }
	    fprintf(fp,"      }\n");
	    }
	  do_return(fp);
	  fprintf(fp,"    }\n  }\n");
	  if (goto_used) 
	    {
	    fprintf(fp," break%d:\n",occ);
	    }
	  }
	}
      fprintf(fp,"  return NULL;\n}\n\n");

      /* clear all occurances of this method from further consideration */
      for (occ = fnum + 1; occ < numberOfWrappedFunctions; occ++)
	{
	/* is it the same name */
	if (wrappedFunctions[occ]->Name && 
	    !strcmp(theFunc->Name,wrappedFunctions[occ]->Name))
	  {
	  int siglen = strlen(wrappedFunctions[fnum]->Signature);
	  /* memory leak here but ... */
	  wrappedFunctions[occ]->Name = NULL;
	  wrappedFunctions[fnum]->Signature = (char *)
	    realloc(wrappedFunctions[fnum]->Signature,siglen+3+
		    strlen(wrappedFunctions[occ]->Signature));
	  strcpy(&wrappedFunctions[fnum]->Signature[siglen],"\\n");
	  strcpy(&wrappedFunctions[fnum]->Signature[siglen+2],
		 wrappedFunctions[occ]->Signature);
	  }
	}
      } /* is this method non NULL */
    } /* loop over all methods */
  
  /* output the method table */
  /* for each function in the array */
  if (data->NumberOfSuperClasses || !data->IsAbstract)
    {
    fprintf(fp,"static PyMethodDef Py%sMethods[] = {\n",data->ClassName);
  
    for (fnum = 0; fnum < numberOfWrappedFunctions; fnum++)
      {
      if (wrappedFunctions[fnum]->Name)
        {
        fprintf(fp,"  {\"%s\",		(PyCFunction)Py%s_%s, 1,\n   \"%s\\n\\n%s\"},\n",
                wrappedFunctions[fnum]->Name, data->ClassName, 
                wrappedFunctions[fnum]->Name, wrappedFunctions[fnum]->Signature,
                quote_string(wrappedFunctions[fnum]->Comment,1000));
        }
      }
  
    if (!strcmp("vtkObject",data->ClassName))
      {
      fprintf(fp,"  {\"GetAddressAsString\",  (PyCFunction)Py%s_GetAddressAsString, 1,\n   \"V.GetAddressAsString(string) -> string\\n\\n Get address of C++ object in format 'Addr=%%p' after casting to\\n the specified type.  You can get the same information from V.__this__.\"},\n", data->ClassName);
      fprintf(fp,"  {\"AddObserver\",  (PyCFunction)Py%s_AddObserver, 1,\n   \"V.AddObserver(int, function) -> int\\n\\n Add an event callback function(vtkObject, int) for an event type.\\n Returns a handle that can be used with RemoveEvent(int).\"},\n", data->ClassName);
      }
    
    fprintf(fp,"  {NULL,	       	NULL}\n};\n\n");
    }
}



void outputFunction(FILE *fp, FileInfo *data)
{
  int i;
  int args_ok = 1;
 
  fp = fp;
  /* some functions will not get wrapped no matter what else,
     and some really common functions will appear only in vtkObjectPython */
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
	(currentFunction->ArgTypes[i]%1000 != 303)&&
        (currentFunction->ArgTypes[i]%1000 != 302)) args_ok = 0;
    }

  /* if we need a return type hint make sure we have one */
  switch (currentFunction->ReturnType%1000)
    {
    case 301: case 307:
    case 304: case 305: case 306:
      args_ok = currentFunction->HaveHint;
      break;
    }
  
  /* make sure it isn't a Delete or New function */
  if (!strcmp("Delete",currentFunction->Name) ||
      !strcmp("New",currentFunction->Name))
    {
    args_ok = 0;
    if (!strcmp("New",currentFunction->Name) &&
	currentFunction->NumberOfArguments == 0)
      {
      class_has_new = 1;
      }
    }
  
  if (currentFunction->IsPublic && args_ok && 
      strcmp(data->ClassName,currentFunction->Name) &&
      strcmp(data->ClassName, currentFunction->Name + 1))
    {
    wrappedFunctions[numberOfWrappedFunctions] = currentFunction;
    numberOfWrappedFunctions++;
    }
}

static void create_class_doc(FILE *fp, FileInfo *data)
{
  const char *text;

  if (data->NameComment) 
    {
    text = data->NameComment;
    while (*text == ' ')
      {
      text++;
      }
    fprintf(fp,"%s\\n\\n",quote_string(text,120));
    }
  else
    {
    fprintf(fp,"%s - no description provided.\\n\\n",
	    quote_string(data->ClassName,120));
    }

  if (data->NumberOfSuperClasses > 0)
    {
    fprintf(fp,"Super Class:\\n\\n %s\\n\\n",
	    quote_string(data->SuperClasses[0],120));
    }

  fprintf(fp,"Description:\\n\\n");
  fprintf(fp,"%s\\n",  
	  data->Description ? quote_string(data->Description,1000) : 
	                      "None provided.\\n");

  if (data->Caveats)
    {
    fprintf(fp,"Caveats:\\n\\n");
    fprintf(fp,"%s\\n", quote_string(data->Caveats,500));
    }

  if (data->SeeAlso)
    {
    char *dup, *tok;
    
    fprintf(fp,"See Also:\\n\\n");
    dup = strdup(data->SeeAlso);
    tok = strtok(dup," ");
    while (tok)
      {
      fprintf(fp," %s",quote_string(tok,120));
      tok = strtok(NULL," ");
      }
    free(dup);
    fprintf(fp,"\\n");
    }
}

/* print the parsed structures */
void vtkParseOutput(FILE *fp, FileInfo *data)
{
  int i;
  
  fprintf(fp,"// python wrapper for %s object\n//\n",data->ClassName);
  fprintf(fp,"#include \"vtkSystemIncludes.h\"\n");
  fprintf(fp,"#include \"%s.h\"\n",data->ClassName);
  fprintf(fp,"#include \"vtkPythonUtil.h\"\n\n");
  
  fprintf(fp,"extern \"C\" { VTK_EXPORT PyObject *PyVTKClass_%sNew(char *); }\n",
	  data->ClassName);
  for (i = 0; i < data->NumberOfSuperClasses; i++)
    {
    fprintf(fp,"extern \"C\" { PyObject *PyVTKClass_%sNew(char *); }\n",
	    data->SuperClasses[i]);
    }
  
  if (!strcmp("vtkObject",data->ClassName))
    {
    fprintf(fp,"static PyObject *PyvtkObject_AddObserver(PyObject *self, PyObject *args)\n");
    fprintf(fp,"{\n");
    fprintf(fp,"  vtkObject *op;\n");
    fprintf(fp,"  char *temp0;\n");
    fprintf(fp,"  PyObject *temp1;\n");
    fprintf(fp,"  unsigned long     temp20 = 0;\n");
    fprintf(fp,"  if ((op = (vtkObject *)PyArg_VTKParseTuple(self, args, \"zO\", &temp0, &temp1)))\n");
    fprintf(fp,"    {\n");
    fprintf(fp,"    if (!PyCallable_Check(temp1) && temp1 != Py_None)\n");
    fprintf(fp,"      {\n");
    fprintf(fp,"      PyErr_SetString(PyExc_ValueError,\"vtk callback method passed to AddObserver was not callable.\");\n");
    fprintf(fp,"      return NULL;\n");
    fprintf(fp,"      }\n");
    fprintf(fp,"    Py_INCREF(temp1);\n");
    fprintf(fp,"    vtkPythonCommand *cbc = new vtkPythonCommand;\n");
    fprintf(fp,"    cbc->SetObject(temp1);\n");
    fprintf(fp,"    temp20 = op->AddObserver(temp0,cbc);\n");
    fprintf(fp,"    return PyInt_FromLong((long)temp20);\n");
    fprintf(fp,"    }\n");
    fprintf(fp,"  return NULL;\n");
    fprintf(fp,"}\n\n");
 
/* while we are at it spit out the GetStringFromObject method */
    fprintf(fp,"PyObject *PyvtkObject_GetAddressAsString(PyObject *self, PyObject *args)\n");
    fprintf(fp,"{\n");

    /* declare the variables */
    fprintf(fp,"  %s *op;\n",data->ClassName);

    /* handle unbound method call if 'self' is a PyVTKClass */
    fprintf(fp,"  char *typecast;\n\n");
    fprintf(fp,"  if ((op = (%s *)PyArg_VTKParseTuple(self, args, \"s\", &typecast)))\n",data->ClassName);
    fprintf(fp,"    {\n    char temp20[256];\n");
    fprintf(fp,"    sprintf(temp20,\"Addr=%%p\",op);\n");
    fprintf(fp,"    return PyString_FromString(temp20);\n");
    fprintf(fp,"    }\n");
    fprintf(fp,"  return NULL;\n}\n\n");
    }
  
  /* insert function handling code here */
  for (i = 0; i < data->NumberOfFunctions; i++)
    {
    currentFunction = data->Functions + i;
    outputFunction(fp, data);
    }
  outputFunction2(fp, data);
  
  /* the docstring for the class */
  if (data->NumberOfSuperClasses || !data->IsAbstract)
    {
    fprintf(fp,"static char %sDoc[] = \"",data->ClassName); 
    create_class_doc(fp,data);
    fprintf(fp,"\";\n\n");
    }
  
  /* output the class initilization function */
  if (data->NumberOfSuperClasses)
    { 
    if (class_has_new)
      {
      fprintf(fp,"static vtkObject *%sStaticNew()\n",data->ClassName);
      fprintf(fp,"{\n  return %s::New();\n}\n\n",data->ClassName);
      }
    fprintf(fp,"PyObject *PyVTKClass_%sNew(char *modulename)\n{\n",data->ClassName);
    if (class_has_new)
      {
      fprintf(fp,"  return PyVTKClass_New(&%sStaticNew,\n",data->ClassName);
      }
    else
      {
      fprintf(fp,"  return PyVTKClass_New(NULL,\n");
      }      
    fprintf(fp,"                        Py%sMethods,\n",data->ClassName);
    fprintf(fp,"                        \"%s\",modulename,\n",data->ClassName);
    fprintf(fp,"                        %sDoc,\n",data->ClassName);
    fprintf(fp,"                        PyVTKClass_%sNew(modulename));\n}\n\n",
	    data->SuperClasses[0]);
    }
  else if (strcmp(data->ClassName,"vtkObject") == 0)
    {
    if (class_has_new)
      {
      fprintf(fp,"static vtkObject *%sStaticNew()\n",data->ClassName);
      fprintf(fp,"{\n  return %s::New();\n}\n\n",data->ClassName);
      }
    fprintf(fp,"PyObject *PyVTKClass_%sNew(char *modulename)\n{\n",data->ClassName);
    if (class_has_new)
      {
      fprintf(fp,"  return PyVTKClass_New(&%sStaticNew,\n",data->ClassName);
      }
    else
      {
      fprintf(fp,"  return PyVTKClass_New(NULL,\n");
      }      
    fprintf(fp,"                        Py%sMethods,\n",
            data->ClassName);
    fprintf(fp,"                        \"%s\",modulename,%sDoc,0);\n}\n\n",
            data->ClassName, data->ClassName);
    }
  else if (!data->IsAbstract)
    {
    fprintf(fp,"PyObject *PyVTKObject_%sNew(PyObject *vtkNotUsed(self), PyObject *args)\n{\n",data->ClassName);
    fprintf(fp,"  if (!(PyArg_ParseTuple(args, \"\")))\n    {\n");
    fprintf(fp,"    return NULL;\n    }\n\n");
    fprintf(fp,"  %s *obj = new %s;\n",data->ClassName,data->ClassName);
    fprintf(fp,"  return PyVTKSpecialObject_New(obj, Py%sMethods, \"%s\",%sDoc);\n",data->ClassName,data->ClassName,data->ClassName);
    fprintf(fp,"}\n\n");

    fprintf(fp,"static PyMethodDef Py%sNewMethod = \\\n",data->ClassName);
    fprintf(fp,"{ \"%s\",  (PyCFunction)PyVTKObject_%sNew, 1,\n",
	    data->ClassName,data->ClassName);
    fprintf(fp,"  %sDoc };\n\n",data->ClassName);

    fprintf(fp,"PyObject *PyVTKClass_%sNew(char *vtkNotUsed(modulename))\n{\n",data->ClassName);
    fprintf(fp,"  return PyCFunction_New(&Py%sNewMethod,Py_None);\n}\n\n",
	    data->ClassName);
    }
  else
    {
    fprintf(fp,"PyObject *PyVTKClass_%sNew(char *vtkNotUsed(modulename))\n{\n",data->ClassName);
    fprintf(fp,"  return NULL;\n}\n\n");
    }
}

