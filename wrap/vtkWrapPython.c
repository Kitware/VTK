/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWrapPython.c
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
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
FunctionInfo *currentFunction;

/* when the cpp file doesn't have enough info use the hint file */
void use_hints(FILE *fp)
{
  int  i;
  
  switch (currentFunction->ReturnType%1000)
    {
    case 301:
      fprintf(fp,"      return Py_BuildValue(\"");
      for (i = 0; i < currentFunction->HintSize; i++) fprintf(fp,"f");
      fprintf(fp,"\"");
      for (i = 0; i < currentFunction->HintSize; i++) 
	{
	fprintf(fp,",temp%i[%d]",MAX_ARGS,i);
	}
      fprintf(fp,");\n");
      break;
    case 307:  
      fprintf(fp,"      return Py_BuildValue(\"");
      for (i = 0; i < currentFunction->HintSize; i++) fprintf(fp,"d");
      fprintf(fp,"\"");
      for (i = 0; i < currentFunction->HintSize; i++) 
	{
	fprintf(fp,",temp%i[%d]",MAX_ARGS,i);
	}
      fprintf(fp,");\n");
      break;
    case 304: 
      fprintf(fp,"      return Py_BuildValue(\"");
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

  if ((i == MAX_ARGS) && ((aType%1000 == 309)||(aType%1000 == 109)))
    {
    fprintf(fp,"  PyObject *tempH;\n");
    }
}

void do_return(FILE *fp)
{
  /* ignore void */
  if (((currentFunction->ReturnType % 10) == 2)&&
      (!((currentFunction->ReturnType%1000)/100)))
    {
    fprintf(fp,"      Py_INCREF(Py_None);\n");
    fprintf(fp,"      return Py_None;\n");
    return;
    }
  
  switch (currentFunction->ReturnType%1000)
    {
    case 303:
      fprintf(fp,"      if (temp%i == NULL) {\n",MAX_ARGS);
      fprintf(fp,"        Py_INCREF(Py_None);\n");
      fprintf(fp,"        return Py_None;\n        }\n");
      fprintf(fp,"      return PyString_FromString(temp%i);\n",MAX_ARGS);
    break;
    case 109:
    case 309:  
      {
      fprintf(fp,"      if (temp%i == NULL)\n        {\n",MAX_ARGS);
      fprintf(fp,"        Py_INCREF(Py_None);\n");
      fprintf(fp,"        return Py_None;\n        }\n");
      fprintf(fp,"      tempH = vtkPythonGetObjectFromPointer((vtkObject *)temp%i);\n",
	      MAX_ARGS);
      fprintf(fp,"      Py_INCREF(tempH);\n");
      fprintf(fp,"      return tempH;\n");
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
      fprintf(fp,"      if (temp%i == NULL)\n        {\n",MAX_ARGS);
      fprintf(fp,"        Py_INCREF(Py_None);\n");
      fprintf(fp,"        return Py_None;\n        }\n");
      fprintf(fp,"      return PyString_FromString(vtkPythonManglePointer(temp%i,\"void_p\"));\n",
	      MAX_ARGS);
      break;
      }
    case 1:
    case 7:   fprintf(fp,"      return PyFloat_FromDouble(temp%i);\n",
		      MAX_ARGS); break;
    case 13:
    case 14:
    case 15:
    case 4:
    case 5:
    case 6:   fprintf(fp,"      return PyInt_FromLong(temp%i);\n", MAX_ARGS); 
      break;
    case 16:   fprintf(fp,"      return PyInt_FromLong((long)temp%i);\n",
		       MAX_ARGS); break;
    case 3:   fprintf(fp,"      return PyString_FromStringAndSize((char *)&temp%i,1);\n",
		      MAX_ARGS); break;
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

void outputFunction2(FILE *fp, FileInfo *data)
{
  int i, j, fnum, occ, backnum;
  FunctionInfo *theFunc;
  FunctionInfo *backFunc;

  /* first create external type declarations for all object
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
      
      /* local error flag */
      fprintf(fp,"  int error;\n");
      
      /* get the object pointer */
      fprintf(fp,"  %s *op;\n",data->ClassName);
      fprintf(fp,"  op = (%s *)((PyVTKObject *)self)->ptr;\n\n",
	      data->ClassName);
      
      /* find all occurances of this method */
      for (occ = fnum; occ < numberOfWrappedFunctions; occ++)
	{
	/* is it the same name */
	if (wrappedFunctions[occ]->Name && 
	    !strcmp(theFunc->Name,wrappedFunctions[occ]->Name))
	  {
	  fprintf(fp,"  /* handle an occurrence */\n  {\n");
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
	  
	  fprintf(fp,"\n  PyErr_Clear();\n");
	  fprintf(fp,"  error = 0;\n");
	  fprintf(fp,"  if (PyArg_ParseTuple(args, \"%s\"",
		  get_format_string());
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
	  fprintf(fp,"))\n    {\n");

	  /* lookup and required objects */
	  for (i = 0; i < currentFunction->NumberOfArguments; i++)
	    {
	    if ((currentFunction->ArgTypes[i]%1000 == 309)||
		(currentFunction->ArgTypes[i]%1000 == 109))
	      {
	      fprintf(fp,"    temp%d = (%s *)vtkPythonGetPointerFromObject(tempH%d,\"%s\");\n",
		      i, currentFunction->ArgClasses[i], i, 
		      currentFunction->ArgClasses[i]);
	      fprintf(fp,"    if (!temp%d && tempH%d != Py_None) error = 1;\n",i,i);
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
	  
	  fprintf(fp,"    if (!error)\n      {\n");
	  
	  /* check for void pointers and pass appropriate info*/
	  for (i = 0; i < currentFunction->NumberOfArguments; i++)
	    {
	    if (currentFunction->ArgTypes[i]%1000 == 302)
	      {
	      fprintf(fp,"      temp%i = vtkPythonUnmanglePointer((char *)temp%i,&size%i,\"%s\");\n",i,i,i,"void_p");
	      fprintf(fp,"      if (size%i == -1) {\n        PyErr_SetString(PyExc_ValueError,\"mangled pointer to %s in %s was of incorrect type.\");\n",
                      i,currentFunction->Name,data->ClassName);
              fprintf(fp,"       return NULL;\n      }\n");
              fprintf(fp,"      else if (size%i == -2) {\n        PyErr_SetString(PyExc_ValueError,\"mangled pointer to %s in %s was poorly formed.\");\n",
		      i,currentFunction->Name,data->ClassName);
	      fprintf(fp,"       return NULL;\n      }\n"); 
	      }
	    }

	  switch (currentFunction->ReturnType%1000)
	    {
	    case 2:
	      fprintf(fp,"      op->%s(",currentFunction->Name);
	      break;
	    case 109:
	      fprintf(fp,"      temp%i = &(op)->%s(",MAX_ARGS,
		      currentFunction->Name);
	      break;
	    default:
	      fprintf(fp,"      temp%i = op->%s(",MAX_ARGS,
		      currentFunction->Name);
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
	    fprintf(fp,"      op->%sArgDelete(vtkPythonVoidFuncArgDelete);\n",
		    currentFunction->Name);
	    }
	  do_return(fp);
	  fprintf(fp,"      }\n    }\n  }\n");
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
	  /* memory leak here but ... */
	  wrappedFunctions[occ]->Name = NULL;
	  }
	}
      } /* is this method non NULL */
    } /* loop over all methods */
  
  /* output the method table */
  /* for each function in the array */
  fprintf(fp,"static PyMethodDef Py%sMethods[] = {\n",data->ClassName);

  for (fnum = 0; fnum < numberOfWrappedFunctions; fnum++)
    {
    if (wrappedFunctions[fnum]->Name)
      {
      fprintf(fp,"  {\"%s\",		(PyCFunction)Py%s_%s, 1},\n",
	      wrappedFunctions[fnum]->Name, data->ClassName, 
	      wrappedFunctions[fnum]->Name);
      }
    }
  if (!strcmp("vtkObject",data->ClassName))
    {
    fprintf(fp,"  {\"GetAddressAsString\",  (PyCFunction)Py%s_GetAddressAsString, 1},\n", data->ClassName);
    }
  
  fprintf(fp,"  {NULL,	       	NULL}\n};\n\n");
}



void outputFunction(FILE *fp, FileInfo *data)
{
  int i;
  int args_ok = 1;
 
  fp = fp;
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
    }
  
  if (currentFunction->IsPublic && args_ok && 
      strcmp(data->ClassName,currentFunction->Name) &&
      strcmp(data->ClassName, currentFunction->Name + 1))
    {
    wrappedFunctions[numberOfWrappedFunctions] = currentFunction;
    numberOfWrappedFunctions++;
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
  
  for (i = 0; i < data->NumberOfSuperClasses; i++)
    {
    fprintf(fp,"PyObject *Py%s_PyGetAttr(PyObject *self,char *name);\n",
	    data->SuperClasses[i]);
    }
  
  if (!strcmp("vtkObject",data->ClassName))
    {
    /* while we are at it spit out the GetStringFromObject method */
    fprintf(fp,"PyObject *PyvtkObject_GetAddressAsString(PyObject *self, PyObject *args)\n");
    fprintf(fp,"{\n  char *typecast;\n\n  PyErr_Clear();\n");
    fprintf(fp,"  if (PyArg_ParseTuple(args, \"s\", &typecast))\n");
    fprintf(fp,"    {\n    char temp20[256];\n");
    fprintf(fp,"    sprintf(temp20,\"Addr=%%p\",((PyVTKObject *)self)->ptr);\n");
    fprintf(fp,"    return PyString_FromString(temp20);\n");
    fprintf(fp,"    }\n  return NULL;\n}\n\n");
    }
  
  /* insert function handling code here */
  for (i = 0; i < data->NumberOfFunctions; i++)
    {
    currentFunction = data->Functions + i;
    outputFunction(fp, data);
    }
  outputFunction2(fp, data);
  
  if (data->NumberOfSuperClasses)
    {
    fprintf(fp,"extern int Py%s_PyPrint(PyObject *,FILE *,int);\n",
	    data->SuperClasses[0]);
    fprintf(fp,"int Py%s_PyPrint(PyObject *self, FILE *fp, int)\n",
	    data->ClassName);
    fprintf(fp,"{\n  return Py%s_PyPrint(self, fp, 0);\n}\n\n",
	    data->SuperClasses[0]);

    fprintf(fp,"extern PyObject *Py%s_PyRepr(PyObject *);\n",
	    data->SuperClasses[0]);
    fprintf(fp,"PyObject *Py%s_PyRepr(PyObject *self)\n",data->ClassName);
    fprintf(fp,"{\n  return Py%s_PyRepr(self);\n}\n\n", data->SuperClasses[0]);
    }
  else
    {
    fprintf(fp,"int Py%s_PyPrint(PyObject *self, FILE *fp, int)\n",data->ClassName);
    if (!strcmp("vtkObject",data->ClassName))
      {
      fprintf(fp,"{\n  %s *op;\n  ostrstream buf;\n\n",data->ClassName);
      fprintf(fp,"  op = (%s *)((PyVTKObject *)self)->ptr;\n",data->ClassName);
      fprintf(fp,"  op->Print(buf);\n  buf.put('\\0');\n");
      fprintf(fp,"  fprintf(fp,\"%%s\",buf.str());\n");
      fprintf(fp,"  delete buf.str();\n  return 0;\n}\n\n");

      fprintf(fp,"PyObject *Py%s_PyRepr(PyObject *self)\n",data->ClassName);
      fprintf(fp,"{\n  %s *op;\n  PyObject *tempH;\n  ostrstream buf;\n\n",data->ClassName);
      fprintf(fp,"  op = (%s *)((PyVTKObject *)self)->ptr;\n",data->ClassName);
      fprintf(fp,"  op->Print(buf);\n  buf.put('\\0');\n");
      fprintf(fp,"  tempH = PyString_FromString(buf.str());\n");
      fprintf(fp,"  delete buf.str();\n  return tempH;\n}\n\n");
      }
    else
      {
      fprintf(fp,"{\n  fprintf(fp,\"<no print method>\");\n");
      fprintf(fp,"  return 0;\n}\n\n");

      fprintf(fp,"PyObject *Py%s_PyRepr(PyObject *self)\n",data->ClassName);
      fprintf(fp,"{\n  return PyString_FromString(\"<no print method>\");\n}\n");
      }
    }
	

  fprintf(fp,"static void Py%s_PyDelete(PyObject *self)\n",data->ClassName);
  fprintf(fp,"{\n  %s *op;\n",data->ClassName);
  fprintf(fp,"  op = (%s *)((PyVTKObject *)self)->ptr;\n",data->ClassName);
  fprintf(fp,"  vtkPythonDeleteObjectFromHash(self);\n");
  fprintf(fp,"  op->Delete();\n");
  fprintf(fp,"  PyMem_DEL(self);\n}\n\n");

  fprintf(fp,"PyObject *Py%s_PyGetAttr(PyObject *self, char *name)\n",
	  data->ClassName);
  fprintf(fp,"{\n  PyObject *result;\n\n");
  fprintf(fp,"  result = Py_FindMethod(Py%sMethods, self, name);\n",data->ClassName);
  if (data->NumberOfSuperClasses)
    {
    fprintf(fp,"  if (!result) return Py%s_PyGetAttr(self,name);\n",
	    data->SuperClasses[0]);
    }
  fprintf(fp,"  return result;\n}\n\n");
	
  fprintf(fp,"PyTypeObject Py%sType = {\n",data->ClassName);
  fprintf(fp,"  PyObject_HEAD_INIT(NULL)\n  0,\n  \"%s\",sizeof(PyVTKObject),\n  0,\n",
	  data->ClassName);
  fprintf(fp,"  (destructor)Py%s_PyDelete,\n  	(printfunc)Py%s_PyPrint,\n",data->ClassName, data->ClassName);
  fprintf(fp,"  (getattrfunc)Py%s_PyGetAttr,\n  0, 0, (reprfunc)Py%s_PyRepr, 0, 0, 0,\n};\n\n",
	  data->ClassName, data->ClassName);

  fprintf(fp,"static PyObject *Py%s_PyNew(PyObject *vtkNotUsed(self),PyObject *vtkNotUsed(args))\n",
          data->ClassName);
  fprintf(fp,"  {\n");
  fprintf(fp,"  PyObject *obj;\n\n");
  if (data->NumberOfSuperClasses == 0 &&
      strcmp(data->ClassName,"vtkObject") != 0) 
    {
    fprintf(fp,"  PyErr_SetString(PyExc_RuntimeError,\"%s is not derived from vtkObject, and not available from Python.\");\n  obj = 0;\n\n",data->ClassName);
    }
  else
    {
    fprintf(fp,"  if ((obj = PyObject_NEW(PyObject, &Py%sType)) == NULL)\n",
	    data->ClassName);
    fprintf(fp,"    return NULL;\n\n");
    fprintf(fp,"  vtkPythonAddObjectToHash(obj,%s::New());\n",
	    data->ClassName);
    }
  fprintf(fp,"  return obj;\n}\n\n");	
  
  fprintf(fp,"static PyMethodDef Py%s_ClassMethods[] = {\n",data->ClassName);
  fprintf(fp,"  {\"New\", (PyCFunction)Py%s_PyNew},\n",data->ClassName);
  fprintf(fp,"  {NULL, NULL}\n};\n\n");
	
  /* output the class initilization function */
  fprintf(fp,"extern \"C\" { void init%s();}\n",data->ClassName);
  fprintf(fp,"void init%s()\n{\n",data->ClassName);
  fprintf(fp,"  Py_InitModule(\"%s\", Py%s_ClassMethods);\n",
	  data->ClassName, data->ClassName);
  fprintf(fp,"  vtkPythonAddTypeToHash(&Py%sType,\"%s\");\n}\n\n",
	  data->ClassName, data->ClassName);
}

