/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWrapTcl.c
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


void output_temp(FILE *fp, int i, int aType, char *Id, int count)
{
  /* handle VAR FUNCTIONS */
  if (aType == 5000)
    {
    fprintf(fp,"    vtkTclVoidFuncArg *temp%i = new vtkTclVoidFuncArg;\n",i);
    return;
    }
  
  /* ignore void */
  if (((aType % 10) == 2)&&(!((aType%1000)/100)))
    {
    return;
    }

  /* for const * return types prototype with const */
  if ((i == MAX_ARGS) && (aType%2000 >= 1000))
    {
    fprintf(fp,"    const ");
    }
  else
    {
    fprintf(fp,"    ");
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
    case 9:     fprintf(fp,"%s ",Id); break;
    case 8: return;
    }

  /* handle array arguements */
  if (count > 1)
    {
    fprintf(fp,"temp%i[%i];\n",i,count);
    return;
    }
  
  switch ((aType%1000)/100)
    {
    case 1: fprintf(fp, " *"); break; /* act " &" */
    case 2: fprintf(fp, "&&"); break;
    case 3: fprintf(fp, " *"); break;
    case 4: fprintf(fp, "&*"); break;
    case 5: fprintf(fp, "*&"); break;
    case 7: fprintf(fp, "**"); break;
    default: fprintf(fp,"  "); break;
    }
  
  fprintf(fp,"temp%i",i);
  fprintf(fp,";\n");
}

/* when the cpp file doesn't have enough info use the hint file */
void use_hints(FILE *fp)
{
  int  i;

  /* use the hint */
  switch (currentFunction->ReturnType%1000)
    {
    case 301: case 307:  
      fprintf(fp,"    sprintf(interp->result,\"");
      for (i = 0; i < currentFunction->HintSize; i++)
	{
	fprintf(fp,"%%g ");
	}
      fprintf(fp,"\"");
      for (i = 0; i < currentFunction->HintSize; i++)
	{
	fprintf(fp,",temp%i[%i]",MAX_ARGS,i);
	}
      fprintf(fp,");\n");
      break;
    case 304: case 305: case 306: case 313:
      fprintf(fp,"    sprintf(interp->result,\"");
      for (i = 0; i < currentFunction->HintSize; i++)
	{
	fprintf(fp,"%%i ");
	}
      fprintf(fp,"\"");
      for (i = 0; i < currentFunction->HintSize; i++)
	{
	fprintf(fp,",temp%i[%i]",MAX_ARGS,i);
	}
      fprintf(fp,");\n");
      break;
    }
}

void return_result(FILE *fp)
{
  switch (currentFunction->ReturnType%1000)
    {
    case 2:
      fprintf(fp,"      interp->result[0] = '\\0';\n"); 
      break;
    case 1: case 7:
      fprintf(fp,"      sprintf(interp->result,\"%%g\",temp%i);\n",
	      MAX_ARGS); 
      break;
    case 4: 
      fprintf(fp,"      sprintf(interp->result,\"%%i\",temp%i);\n",
	      MAX_ARGS); 
      break;
    case 5:
      fprintf(fp,"      sprintf(interp->result,\"%%hi\",temp%i);\n",
	      MAX_ARGS); 
      break;
    case 6:
      fprintf(fp,"      sprintf(interp->result,\"%%li\",temp%i);\n",
	      MAX_ARGS); 
      break;
    case 14: 
      fprintf(fp,"      sprintf(interp->result,\"%%u\",temp%i);\n",
	      MAX_ARGS); 
      break;
    case 15:
      fprintf(fp,"      sprintf(interp->result,\"%%hu\",temp%i);\n",
	      MAX_ARGS); 
      break;
    case 16:
      fprintf(fp,"      sprintf(interp->result,\"%%lu\",temp%i);\n",
	      MAX_ARGS); 
      break;
    case 13:
      fprintf(fp,"      sprintf(interp->result,\"%%hu\",temp%i);\n",
	      MAX_ARGS); 
      break;
    case 303:
      fprintf(fp,"      if (temp%i)\n        {\n        sprintf(interp->result,\"%%s\",temp%i);\n",MAX_ARGS,MAX_ARGS); 
      fprintf(fp,"        }\n      else\n        {\n");
      fprintf(fp,"        interp->result[0] = '\\0';\n        }\n"); 
      break;
    case 3:
      fprintf(fp,"      sprintf(interp->result,\"%%c\",temp%i);\n",
	      MAX_ARGS); 
      break;
    case 109:
    case 309:  
      fprintf(fp,"      vtkTclGetObjectFromPointer(interp,(void *)temp%i,%sCommand);\n",MAX_ARGS,currentFunction->ReturnClass);
      break;

    /* handle functions returning vectors */
    /* this is done by looking them up in a hint file */
    case 301: case 307:
    case 304: case 305: case 306:
      use_hints(fp);
      break;
    default:
      fprintf(fp,"      sprintf(interp->result,\"unable to return result.\");\n"); 
      break;
    }
}

void handle_return_prototype(FILE *fp)
{
  switch (currentFunction->ReturnType%1000)
    {
    case 109:
    case 309:  
      fprintf(fp,"    int %sCommand(ClientData, Tcl_Interp *, int, char *[]);\n",currentFunction->ReturnClass);
      break;
    }
}

void get_args(FILE *fp, int i)
{
  int j;
  int start_arg = 2;
  
  /* what arg do we start with */
  for (j = 0; j < i; j++)
    {
    start_arg = start_arg + 
      (currentFunction->ArgCounts[j] ? currentFunction->ArgCounts[j] : 1);
    }
  
  /* handle VAR FUNCTIONS */
  if (currentFunction->ArgTypes[i] == 5000)
    {
    fprintf(fp,"    temp%i->interp = interp;\n",i);
    fprintf(fp,"    temp%i->command = strcpy(new char [strlen(argv[2])+1],argv[2]);\n",i);
    return;
    }

  /* ignore void */
  if (((currentFunction->ArgTypes[i] % 10) == 2)&&
      (!((currentFunction->ArgTypes[i]%1000)/100)))
    {
    return;
    }
  
  switch (currentFunction->ArgTypes[i]%1000)
    {
    case 1: case 7:  
      fprintf(fp,
	      "    if (Tcl_GetDouble(interp,argv[%i],&tempd) != TCL_OK) error = 1;\n",
	      start_arg); 
      fprintf(fp,"    temp%i = tempd;\n",i);
      break;
    case 4: case 5: case 6: 
      fprintf(fp,"    if (Tcl_GetInt(interp,argv[%i],&tempi) != TCL_OK) error = 1;\n",
	      start_arg); 
      fprintf(fp,"    temp%i = tempi;\n",i);
      break;
    case 3:
      fprintf(fp,"    temp%i = *(argv[%i]);\n",i,start_arg);
      break;
    case 13:
      fprintf(fp,"    if (Tcl_GetInt(interp,argv[%i],&tempi) != TCL_OK) error = 1;\n",
	      start_arg); 
      fprintf(fp,"    temp%i = (unsigned char)tempi;\n",i);
      break;
    case 14:
      fprintf(fp,"    if (Tcl_GetInt(interp,argv[%i],&tempi) != TCL_OK) error = 1;\n",
	      start_arg); 
      fprintf(fp,"    temp%i = (unsigned int)tempi;\n",i);
      break;
    case 15:
      fprintf(fp,"    if (Tcl_GetInt(interp,argv[%i],&tempi) != TCL_OK) error = 1;\n",
	      start_arg); 
      fprintf(fp,"    temp%i = (unsigned short)tempi;\n",i);
      break;
    case 303:
      fprintf(fp,"    temp%i = argv[%i];\n",i,start_arg);
      break;
    case 109:
    case 309:
      fprintf(fp,"    temp%i = (%s *)(vtkTclGetPointerFromObject(argv[%i],\"%s\",interp,error));\n",i,currentFunction->ArgClasses[i],start_arg,
	      currentFunction->ArgClasses[i]);
      break;
    case 2:    
    case 9:
      break;
    default:
      if (currentFunction->ArgCounts[i] > 1)
	{
	for (j = 0; j < currentFunction->ArgCounts[i]; j++)
	  {
	  switch (currentFunction->ArgTypes[i]%100)
	    {
	    case 1: case 7:  
	      fprintf(fp,
		      "    if (Tcl_GetDouble(interp,argv[%i],&tempd) != TCL_OK) error = 1;\n",
		      start_arg); 
	      fprintf(fp,"    temp%i[%i] = tempd;\n",i,j);
	      break;
	    case 4: case 5: case 6: 
	      fprintf(fp,"    if (Tcl_GetInt(interp,argv[%i],&tempi) != TCL_OK) error = 1;\n",
		      start_arg); 
	      fprintf(fp,"    temp%i[%i] = tempi;\n",i,j);
	      break;
	    case 3:
	      fprintf(fp,"    temp%i[%i] = *(argv[%i]);\n",i,j,start_arg);
	      break;
	    case 13:
	      fprintf(fp,"    if (Tcl_GetInt(interp,argv[%i],&tempi) != TCL_OK) error = 1;\n",
		      start_arg); 
	      fprintf(fp,"    temp%i[%i] = (unsigned char)tempi;\n",i,j);
	      break;
	    case 14:
	      fprintf(fp,"    if (Tcl_GetInt(interp,argv[%i],&tempi) != TCL_OK) error = 1;\n",
		      start_arg); 
	      fprintf(fp,"    temp%i[%i] = (unsigned int)tempi;\n",i,j);
	      break;
	    case 15:
	      fprintf(fp,"    if (Tcl_GetInt(interp,argv[%i],&tempi) != TCL_OK) error = 1;\n",
		      start_arg); 
	      fprintf(fp,"    temp%i[%i] = (unsigned short)tempi;\n",i,j);
	      break;
	    }
	  start_arg++;
	  }
	}
      
    }
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
  
  /* check to see if we can handle the args */
  for (i = 0; i < currentFunction->NumberOfArguments; i++)
    {
    if ((currentFunction->ArgTypes[i]%10) == 8) args_ok = 0;
    /* if its a pointer arg make sure we have the ArgCount */
    if ((currentFunction->ArgTypes[i]%1000 >= 100) &&
	(currentFunction->ArgTypes[i]%1000 != 303)&&
	(currentFunction->ArgTypes[i]%1000 != 309)&&
	(currentFunction->ArgTypes[i]%1000 != 109)) 
      {
      if (currentFunction->NumberOfArguments > 1 ||
	  !currentFunction->ArgCounts[i])
	{
	args_ok = 0;
	}
      }
    if ((currentFunction->ArgTypes[i]%100 >= 10)&&
	(currentFunction->ArgTypes[i] != 13)&&
	(currentFunction->ArgTypes[i] != 14)&&
	(currentFunction->ArgTypes[i] != 15)) args_ok = 0;
    }
  if ((currentFunction->ReturnType%10) == 8) args_ok = 0;
  if (((currentFunction->ReturnType%1000)/100 != 3)&&
      ((currentFunction->ReturnType%1000)/100 != 1)&&
      ((currentFunction->ReturnType%1000)/100)) args_ok = 0;
  if (currentFunction->NumberOfArguments && 
      (currentFunction->ArgTypes[0] == 5000)
      &&(currentFunction->NumberOfArguments != 1)) args_ok = 0;

  /* watch out for functions that dont have enough info */
  switch (currentFunction->ReturnType%1000)
    {
    case 301: case 307:
    case 304: case 305: case 306:
      args_ok = currentFunction->HaveHint;
      break;
    }
  
  /* if the args are OK and it is not a constructor or destructor */
  if (args_ok && 
      strcmp(data->ClassName,currentFunction->Name) &&
      strcmp(data->ClassName,currentFunction->Name + 1))
    {
    int required_args = 0;
    
    /* calc the total required args */
    for (i = 0; i < currentFunction->NumberOfArguments; i++)
      {
      required_args = required_args + 
	(currentFunction->ArgCounts[i] ? currentFunction->ArgCounts[i] : 1);
      }
    
    fprintf(fp,"  if ((!strcmp(\"%s\",argv[1]))&&(argc == %i))\n    {\n",
	    currentFunction->Name, required_args + 2);
    
    /* process the args */
    for (i = 0; i < currentFunction->NumberOfArguments; i++)
      {
      output_temp(fp, i, currentFunction->ArgTypes[i],
		  currentFunction->ArgClasses[i], 
		  currentFunction->ArgCounts[i]);
      }
    output_temp(fp, MAX_ARGS,currentFunction->ReturnType,
		currentFunction->ReturnClass, 0);
    handle_return_prototype(fp);
    fprintf(fp,"    error = 0;\n\n");
    
    /* now get the required args from the stack */
    for (i = 0; i < currentFunction->NumberOfArguments; i++)
      {
      get_args(fp,i);
      }
    
    fprintf(fp,"    if (!error)\n      {\n");
    switch (currentFunction->ReturnType%1000)
      {
      case 2:
	fprintf(fp,"      op->%s(",currentFunction->Name);
	break;
      case 109:
	fprintf(fp,"      temp%i = &(op)->%s(",MAX_ARGS,currentFunction->Name);
	break;
      default:
	fprintf(fp,"      temp%i = (op)->%s(",MAX_ARGS,currentFunction->Name);
      }
    for (i = 0; i < currentFunction->NumberOfArguments; i++)
      {
      if (i)
	{
	fprintf(fp,",");
	}
      if (currentFunction->ArgTypes[i] == 109)
	{
	fprintf(fp,"*(temp%i)",i);
	}
      else if (currentFunction->ArgTypes[i] == 5000)
	{
	fprintf(fp,"vtkTclVoidFunc,(void *)temp%i",i);
	}
      else
	{
	fprintf(fp,"temp%i",i);
	}
      }
    fprintf(fp,");\n");
    if (currentFunction->NumberOfArguments && 
	(currentFunction->ArgTypes[0] == 5000))
      {
      fprintf(fp,"      op->%sArgDelete(vtkTclVoidFuncArgDelete);\n",
	      currentFunction->Name);
      }
    return_result(fp);
    fprintf(fp,"      return TCL_OK;\n      }\n");
    fprintf(fp,"    }\n");
    
    wrappedFunctions[numberOfWrappedFunctions] = currentFunction;
    numberOfWrappedFunctions++;
    }
}

/* print the parsed structures */
void vtkParseOutput(FILE *fp, FileInfo *data)
{
  int i,j;
  
  fprintf(fp,"// tcl wrapper for %s object\n//\n",data->ClassName);
  fprintf(fp,"#ifdef _WIN32\n");
  fprintf(fp,"#include <strstrea.h>\n");
  fprintf(fp,"#else\n");
  fprintf(fp,"#include <strstream.h>\n");
  fprintf(fp,"#endif\n");
  fprintf(fp,"#include \"%s.h\"\n\n",data->ClassName);
  fprintf(fp,"#include \"vtkTclUtil.h\"\n");
  if (data->IsConcrete)
    {
    fprintf(fp,"\nClientData %sNewCommand()\n{\n",data->ClassName);
    fprintf(fp,"  %s *temp = %s::New();\n",data->ClassName,data->ClassName);
    fprintf(fp,"  return ((ClientData)temp);\n}\n\n");
    }
  
  for (i = 0; i < data->NumberOfSuperClasses; i++)
    {
    fprintf(fp,"int %sCppCommand(%s *op, Tcl_Interp *interp,\n             int argc, char *argv[]);\n",data->SuperClasses[i],data->SuperClasses[i]);
    }
  fprintf(fp,"int VTKTCL_EXPORT %sCppCommand(%s *op, Tcl_Interp *interp,\n             int argc, char *argv[]);\n",data->ClassName,data->ClassName);
  fprintf(fp,"\nint VTKTCL_EXPORT %sCommand(ClientData cd, Tcl_Interp *interp,\n             int argc, char *argv[])\n{\n",data->ClassName);
  fprintf(fp,"  if ((argc == 2)&&(!strcmp(\"Delete\",argv[1]))&& !vtkTclInDelete())\n    {\n");
  fprintf(fp,"    Tcl_DeleteCommand(interp,argv[0]);\n");
  fprintf(fp,"    return TCL_OK;\n    }\n");
  fprintf(fp,"   return %sCppCommand((%s *)cd,interp, argc, argv);\n}\n",data->ClassName,data->ClassName);
  
  fprintf(fp,"\nint VTKTCL_EXPORT %sCppCommand(%s *op, Tcl_Interp *interp,\n             int argc, char *argv[])\n{\n",data->ClassName,data->ClassName);
  fprintf(fp,"  int    tempi;\n");
  fprintf(fp,"  double tempd;\n");
  fprintf(fp,"  static char temps[80];\n");
  fprintf(fp,"  int    error;\n\n");
  fprintf(fp,"  error = 0;\n");
  fprintf(fp,"  tempi = 0;\n");
  fprintf(fp,"  tempd = 0;\n");
  fprintf(fp,"  temps[0] = 0;\n\n");

  fprintf(fp,"  if (argc < 2)\n    {\n    sprintf(interp->result,\"Could not find requested method.\");\n    return TCL_ERROR;\n    }\n");

  /* stick in the typecasting and delete functionality here */
  fprintf(fp,"  if (!interp)\n    {\n");
  fprintf(fp,"    if (!strcmp(\"DoTypecasting\",argv[0]))\n      {\n");
  fprintf(fp,"      if (!strcmp(\"%s\",argv[1]))\n        {\n",
	  data->ClassName);
  fprintf(fp,"        argv[2] = (char *)((void *)op);\n");
  fprintf(fp,"        return TCL_OK;\n        }\n");

  /* check our superclasses */
  for (i = 0; i < data->NumberOfSuperClasses; i++)
    {
    fprintf(fp,"      if (%sCppCommand((%s *)op,interp,argc,argv) == TCL_OK)\n        {\n",
	    data->SuperClasses[i],data->SuperClasses[i]);
    fprintf(fp,"        return TCL_OK;\n        }\n");      
    }
  fprintf(fp,"      }\n    return TCL_ERROR;\n    }\n\n");
  
  
  
  /* insert function handling code here */
  for (i = 0; i < data->NumberOfFunctions; i++)
    {
    currentFunction = data->Functions + i;
    outputFunction(fp, data);
    }
  
  /* add the ListInstances method */
  fprintf(fp,"\n  if (!strcmp(\"ListInstances\",argv[1]))\n    {\n");
  fprintf(fp,"    vtkTclListInstances(interp,%sCommand);\n",data->ClassName);
  fprintf(fp,"    return TCL_OK;\n    }\n");
  
  /* add the ListMethods method */
  fprintf(fp,"\n  if (!strcmp(\"ListMethods\",argv[1]))\n    {\n");
  /* recurse up the tree */
  for (i = 0; i < data->NumberOfSuperClasses; i++)
    {
    fprintf(fp,"    %sCppCommand(op,interp,argc,argv);\n",
	    data->SuperClasses[i]);
    }
  /* now list our methods */
  fprintf(fp,"    Tcl_AppendResult(interp,\"Methods from %s:\\n\",NULL);\n",data->ClassName);
  for (i = 0; i < numberOfWrappedFunctions; i++)
    {
    int numArgs = 0;

    currentFunction = wrappedFunctions[i];
    	
    /* calc the total required args */
    for (j = 0; j < currentFunction->NumberOfArguments; j++)
      {
      numArgs = numArgs + 
	(currentFunction->ArgCounts[j] ? currentFunction->ArgCounts[j] : 1);
      }

    if (numArgs > 1)
      {
      fprintf(fp,"    Tcl_AppendResult(interp,\"  %s\\t with %i args\\n\",NULL);\n",
	      currentFunction->Name, numArgs);
      }
    if (numArgs == 1)
      {
	  fprintf(fp,"    Tcl_AppendResult(interp,\"  %s\\t with 1 arg\\n\",NULL);\n",
		  currentFunction->Name);
      }
    if (numArgs == 0)
      {
      fprintf(fp,"    Tcl_AppendResult(interp,\"  %s\\n\",NULL);\n",
	      currentFunction->Name);
      }
    }
  fprintf(fp,"    return TCL_OK;\n    }\n");
  
  /* try superclasses */
  for (i = 0; i < data->NumberOfSuperClasses; i++)
    {
    fprintf(fp,"\n  if (%sCppCommand((%s *)op,interp,argc,argv) == TCL_OK)\n",
	    data->SuperClasses[i], data->SuperClasses[i]);
    fprintf(fp,"    {\n    return TCL_OK;\n    }\n");
    }
  
  /* add the default print method to Object */
  if (!strcmp("vtkObject",data->ClassName))
    {
    fprintf(fp,"  if ((!strcmp(\"Print\",argv[1]))&&(argc == 2))\n    {\n");
    fprintf(fp,"    ostrstream buf;\n");
    fprintf(fp,"    op->Print(buf);\n");
    fprintf(fp,"    buf.put('\\0');\n");
    fprintf(fp,"    Tcl_SetResult(interp,buf.str(),TCL_VOLATILE);\n");
    fprintf(fp,"    delete buf.str();\n");
    fprintf(fp,"    return TCL_OK;\n    }\n");
    }
  fprintf(fp,"\n  if ((argc >= 2)&&(!strstr(interp->result,\"Object named:\")))\n    {\n");
  fprintf(fp,"    char temps2[256];\n    sprintf(temps2,\"Object named: %%s, could not find requested method: %%s\\nor the method was called with incorrect arguments.\\n\",argv[0],argv[1]);\n    Tcl_AppendResult(interp,temps2,NULL);\n    }\n");
  fprintf(fp,"  return TCL_ERROR;\n}\n");

}
