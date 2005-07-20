/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWrapTcl.c

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
#include "vtkConfigure.h"

int numberOfWrappedFunctions = 0;
FunctionInfo *wrappedFunctions[1000];
extern FunctionInfo *currentFunction;


void output_temp(FILE *fp, int i, int aType, char *Id, int count)
{
  /* handle VAR FUNCTIONS */
  if (aType == 0x5000)
    {
    fprintf(fp,"    vtkTclVoidFuncArg *temp%i = new vtkTclVoidFuncArg;\n",i);
    return;
    }
  
  /* ignore void */
  if (((aType % 0x10) == 0x2)&&(!((aType % 0x1000)/0x100)))
    {
    return;
    }

  /* for const * return types prototype with const */
  if ((i == MAX_ARGS) && (aType % 0x2000 >= 0x1000))
    {
    fprintf(fp,"    const ");
    }
  else
    {
    fprintf(fp,"    ");
    }

  if ((aType % 0x100)/0x10 == 0x1)
    {
    fprintf(fp,"unsigned ");
    }

  switch (aType % 0x10)
    {
    case 0x1:   fprintf(fp,"float  "); break;
    case 0x7:   fprintf(fp,"double "); break;
    case 0x4:   fprintf(fp,"int    "); break;
    case 0x5:   fprintf(fp,"short  "); break;
    case 0x6:   fprintf(fp,"long   "); break;
    case 0x2:     fprintf(fp,"void   "); break;
    case 0x3:     fprintf(fp,"char   "); break;
    case 0x9:     fprintf(fp,"%s ",Id); break;
    case 0xA:   fprintf(fp,"vtkIdType "); break;
    case 0xB:   fprintf(fp,"long long "); break;
    case 0xC:   fprintf(fp,"__int64 "); break;
    case 0xD:   fprintf(fp,"signed char "); break;
    case 0x8: return;
    }

  /* handle array arguements */
  if (count > 0x1)
    {
    fprintf(fp,"temp%i[%i];\n",i,count);
    return;
    }
  
  switch ((aType % 0x1000)/0x100)
    {
    case 0x1: fprintf(fp, " *"); break; /* act " &" */
    case 0x2: fprintf(fp, "&&"); break;
    case 0x3: fprintf(fp, " *"); break;
    case 0x4: fprintf(fp, "&*"); break;
    case 0x5: fprintf(fp, "*&"); break;
    case 0x7: fprintf(fp, "**"); break;
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
  switch (currentFunction->ReturnType % 0x1000)
    {
    case 0x301: case 0x307:  
      fprintf(fp,"    char tempResult[1024];\n");
      fprintf(fp,"    sprintf(tempResult,\"");
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
      fprintf(fp,"    Tcl_SetResult(interp, tempResult, TCL_VOLATILE);\n");
      break;
    case 0x304: case 0x305: case 0x30D:
#ifndef VTK_USE_64BIT_IDS
    case 0x30A:
#endif
      fprintf(fp,"    char tempResult[1024];\n");
      fprintf(fp,"    sprintf(tempResult,\"");
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
      fprintf(fp,"    Tcl_SetResult(interp, tempResult, TCL_VOLATILE);\n");
      break;
    case 0x306:
      fprintf(fp,"    char tempResult[1024];\n");
      fprintf(fp,"    sprintf(tempResult,\"");
      for (i = 0; i < currentFunction->HintSize; i++)
        {
        fprintf(fp,"%%li ");
        }
      fprintf(fp,"\"");
      for (i = 0; i < currentFunction->HintSize; i++)
        {
        fprintf(fp,",temp%i[%i]",MAX_ARGS,i);
        }
      fprintf(fp,");\n");
      fprintf(fp,"    Tcl_SetResult(interp, tempResult, TCL_VOLATILE);\n");
      break;
#ifdef VTK_USE_64BIT_IDS
    case 0x30A:
      fprintf(fp,"    char tempResult[1024];\n");
      fprintf(fp,"    sprintf(tempResult,\"");
      for (i = 0; i < currentFunction->HintSize; i++)
        {
#  if defined(_MSC_VER)
        fprintf(fp,"%%I64i ");
#  else
        fprintf(fp,"%%lli ");
#  endif
        }
      fprintf(fp,"\"");
      for (i = 0; i < currentFunction->HintSize; i++)
        {
        fprintf(fp,",temp%i[%i]",MAX_ARGS,i);
        }
      fprintf(fp,");\n");
      fprintf(fp,"    Tcl_SetResult(interp, tempResult, TCL_VOLATILE);\n");
      break;
#endif
    case 0x30B:
      fprintf(fp,"    char tempResult[1024];\n");
      fprintf(fp,"    sprintf(tempResult,\"");
      for (i = 0; i < currentFunction->HintSize; i++)
        {
        fprintf(fp,"%%lli ");
        }
      fprintf(fp,"\"");
      for (i = 0; i < currentFunction->HintSize; i++)
        {
        fprintf(fp,",temp%i[%i]",MAX_ARGS,i);
        }
      fprintf(fp,");\n");
      fprintf(fp,"    Tcl_SetResult(interp, tempResult, TCL_VOLATILE);\n");
      break;
    case 0x30C:
      fprintf(fp,"    char tempResult[1024];\n");
      fprintf(fp,"    sprintf(tempResult,\"");
      for (i = 0; i < currentFunction->HintSize; i++)
        {
        fprintf(fp,"%%I64i ");
        }
      fprintf(fp,"\"");
      for (i = 0; i < currentFunction->HintSize; i++)
        {
        fprintf(fp,",temp%i[%i]",MAX_ARGS,i);
        }
      fprintf(fp,");\n");
      fprintf(fp,"    Tcl_SetResult(interp, tempResult, TCL_VOLATILE);\n");
      break;
    case 0x313: case 0x314: case 0x315:
#ifndef VTK_USE_64BIT_IDS
    case 0x31A:
#endif
      fprintf(fp,"    char tempResult[1024];\n");
      fprintf(fp,"    sprintf(tempResult,\"");
      for (i = 0; i < currentFunction->HintSize; i++)
        {
        fprintf(fp,"%%u ");
        }
      fprintf(fp,"\"");
      for (i = 0; i < currentFunction->HintSize; i++)
        {
        fprintf(fp,",temp%i[%i]",MAX_ARGS,i);
        }
      fprintf(fp,");\n");
      fprintf(fp,"    Tcl_SetResult(interp, tempResult, TCL_VOLATILE);\n");
      break;
    case 0x316:
      fprintf(fp,"    char tempResult[1024];\n");
      fprintf(fp,"    sprintf(tempResult,\"");
      for (i = 0; i < currentFunction->HintSize; i++)
        {
        fprintf(fp,"%%lu ");
        }
      fprintf(fp,"\"");
      for (i = 0; i < currentFunction->HintSize; i++)
        {
        fprintf(fp,",temp%i[%i]",MAX_ARGS,i);
        }
      fprintf(fp,");\n");
      fprintf(fp,"    Tcl_SetResult(interp, tempResult, TCL_VOLATILE);\n");
      break;
#ifdef VTK_USE_64BIT_IDS
    case 0x31A:
      fprintf(fp,"    char tempResult[1024];\n");
      fprintf(fp,"    sprintf(tempResult,\"");
      for (i = 0; i < currentFunction->HintSize; i++)
        {
#  if defined(_MSC_VER)
        fprintf(fp,"%%I64u ");
#  else
        fprintf(fp,"%%llu ");
#  endif
        }
      fprintf(fp,"\"");
      for (i = 0; i < currentFunction->HintSize; i++)
        {
        fprintf(fp,",temp%i[%i]",MAX_ARGS,i);
        }
      fprintf(fp,");\n");
      fprintf(fp,"    Tcl_SetResult(interp, tempResult, TCL_VOLATILE);\n");
      break;
#endif
    case 0x31B:
      fprintf(fp,"    char tempResult[1024];\n");
      fprintf(fp,"    sprintf(tempResult,\"");
      for (i = 0; i < currentFunction->HintSize; i++)
        {
        fprintf(fp,"%%llu ");
        }
      fprintf(fp,"\"");
      for (i = 0; i < currentFunction->HintSize; i++)
        {
        fprintf(fp,",temp%i[%i]",MAX_ARGS,i);
        }
      fprintf(fp,");\n");
      fprintf(fp,"    Tcl_SetResult(interp, tempResult, TCL_VOLATILE);\n");
      break;
    case 0x31C:
      fprintf(fp,"    char tempResult[1024];\n");
      fprintf(fp,"    sprintf(tempResult,\"");
      for (i = 0; i < currentFunction->HintSize; i++)
        {
        fprintf(fp,"%%I64u ");
        }
      fprintf(fp,"\"");
      for (i = 0; i < currentFunction->HintSize; i++)
        {
        fprintf(fp,",temp%i[%i]",MAX_ARGS,i);
        }
      fprintf(fp,");\n");
      fprintf(fp,"    Tcl_SetResult(interp, tempResult, TCL_VOLATILE);\n");
      break;
    }
}

void return_result(FILE *fp)
{
  switch (currentFunction->ReturnType % 0x1000)
    {
    case 0x2:
      fprintf(fp,"    Tcl_ResetResult(interp);\n"); 
      break;
    case 0x1: case 0x7: 
      fprintf(fp,"    char tempResult[1024];\n");
      fprintf(fp,"    sprintf(tempResult,\"%%g\",temp%i);\n",
              MAX_ARGS); 
      fprintf(fp,"    Tcl_SetResult(interp, tempResult, TCL_VOLATILE);\n");
      break;
    case 0x4:  
#ifndef VTK_USE_64BIT_IDS
    case 0xA:
#endif
      fprintf(fp,"    char tempResult[1024];\n");
      fprintf(fp,"    sprintf(tempResult,\"%%i\",temp%i);\n",
              MAX_ARGS); 
      fprintf(fp,"    Tcl_SetResult(interp, tempResult, TCL_VOLATILE);\n");
      break;
    case 0x5:
      fprintf(fp,"    char tempResult[1024];\n");
      fprintf(fp,"    sprintf(tempResult,\"%%hi\",temp%i);\n",
              MAX_ARGS); 
      fprintf(fp,"    Tcl_SetResult(interp, tempResult, TCL_VOLATILE);\n");
      break;
    case 0x6:
      fprintf(fp,"    char tempResult[1024];\n");
      fprintf(fp,"    sprintf(tempResult,\"%%li\",temp%i);\n",
              MAX_ARGS);
      fprintf(fp,"    Tcl_SetResult(interp, tempResult, TCL_VOLATILE);\n");
      break;
#ifdef VTK_USE_64BIT_IDS
    case 0xA:
      fprintf(fp,"    char tempResult[1024];\n");
#  if defined(_MSC_VER)
      fprintf(fp,"    sprintf(tempResult,\"%%I64i\",temp%i);\n",
              MAX_ARGS); 
#  else
      fprintf(fp,"    sprintf(tempResult,\"%%lli\",temp%i);\n",
              MAX_ARGS); 
#  endif
      fprintf(fp,"    Tcl_SetResult(interp, tempResult, TCL_VOLATILE);\n");
      break;
#endif
    case 0xB:
      fprintf(fp,"    char tempResult[1024];\n");
      fprintf(fp,"    sprintf(tempResult,\"%%lli\",temp%i);\n",
              MAX_ARGS);
      fprintf(fp,"    Tcl_SetResult(interp, tempResult, TCL_VOLATILE);\n");
      break;
    case 0xC:
      fprintf(fp,"    char tempResult[1024];\n");
      fprintf(fp,"    sprintf(tempResult,\"%%I64i\",temp%i);\n",
              MAX_ARGS);
      fprintf(fp,"    Tcl_SetResult(interp, tempResult, TCL_VOLATILE);\n");
      break;
    case 0x14:
#ifndef VTK_USE_64BIT_IDS
    case 0x1A:
#endif
      fprintf(fp,"    char tempResult[1024];\n");
      fprintf(fp,"    sprintf(tempResult,\"%%u\",temp%i);\n",
              MAX_ARGS); 
      fprintf(fp,"    Tcl_SetResult(interp, tempResult, TCL_VOLATILE);\n");
      break;
    case 0x15: 
      fprintf(fp,"    char tempResult[1024];\n");
      fprintf(fp,"    sprintf(tempResult,\"%%hu\",temp%i);\n",
              MAX_ARGS);  
      fprintf(fp,"    Tcl_SetResult(interp, tempResult, TCL_VOLATILE);\n");
      break;
    case 0x16:  
      fprintf(fp,"    char tempResult[1024];\n");
      fprintf(fp,"    sprintf(tempResult,\"%%lu\",temp%i);\n",
              MAX_ARGS);
      fprintf(fp,"    Tcl_SetResult(interp, tempResult, TCL_VOLATILE);\n");
      break;
    case 0x13:
      fprintf(fp,"    char tempResult[1024];\n");
      fprintf(fp,"    sprintf(tempResult,\"%%hu\",temp%i);\n",
              MAX_ARGS); 
      fprintf(fp,"    Tcl_SetResult(interp, tempResult, TCL_VOLATILE);\n");
      break;
#ifdef VTK_USE_64BIT_IDS
    case 0x1A:  
      fprintf(fp,"    char tempResult[1024];\n");
#  if defined(_MSC_VER)
      fprintf(fp,"    sprintf(tempResult,\"%%I64u\",temp%i);\n",
              MAX_ARGS); 
#  else
      fprintf(fp,"    sprintf(tempResult,\"%%llu\",temp%i);\n",
              MAX_ARGS); 
#  endif
      fprintf(fp,"    Tcl_SetResult(interp, tempResult, TCL_VOLATILE);\n");
      break;
#endif
    case 0x1B:
      fprintf(fp,"    char tempResult[1024];\n");
      fprintf(fp,"    sprintf(tempResult,\"%%llu\",temp%i);\n",
              MAX_ARGS);
      fprintf(fp,"    Tcl_SetResult(interp, tempResult, TCL_VOLATILE);\n");
      break;
    case 0x1C:
      fprintf(fp,"    char tempResult[1024];\n");
      fprintf(fp,"    sprintf(tempResult,\"%%I64u\",temp%i);\n",
              MAX_ARGS);
      fprintf(fp,"    Tcl_SetResult(interp, tempResult, TCL_VOLATILE);\n");
      break;
    case 0x303:
      fprintf(fp,"    if (temp%i)\n      {\n      Tcl_SetResult(interp, (char*)temp%i, TCL_VOLATILE);\n",MAX_ARGS,MAX_ARGS); 
      fprintf(fp,"      }\n    else\n      {\n");
      fprintf(fp,"      Tcl_ResetResult(interp);\n      }\n"); 
      break;
    case 0x3:
      fprintf(fp,"    char tempResult[1024];\n");
      fprintf(fp,"    sprintf(tempResult,\"%%c\",temp%i);\n",
              MAX_ARGS); 
      fprintf(fp,"    Tcl_SetResult(interp, tempResult, TCL_VOLATILE);\n");
      break;
    case 0x109:
    case 0x309:  
      fprintf(fp,"      vtkTclGetObjectFromPointer(interp,(void *)temp%i,\"%s\");\n",MAX_ARGS,currentFunction->ReturnClass);
      break;

    /* handle functions returning vectors */
    /* this is done by looking them up in a hint file */
    case 0x301: case 0x307:
    case 0x304: case 0x305: case 0x306: case 0x30A: case 0x30B: case 0x30C: case 0x30D:
    case 0x313: case 0x314: case 0x315: case 0x316: case 0x31A: case 0x31B: case 0x31C:
      use_hints(fp);
      break;
    default:
      fprintf(fp,"    Tcl_SetResult(interp, (char *) \"unable to return result.\", TCL_VOLATILE);\n");
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
  if (currentFunction->ArgTypes[i] == 0x5000)
    {
    fprintf(fp,"    temp%i->interp = interp;\n",i);
    fprintf(fp,"    temp%i->command = strcpy(new char [strlen(argv[2])+1],argv[2]);\n",i);
    return;
    }

  /* ignore void */
  if (((currentFunction->ArgTypes[i] % 0x10) == 0x2)&&
      (!((currentFunction->ArgTypes[i] % 0x1000)/0x100)))
    {
    return;
    }
  
  switch (currentFunction->ArgTypes[i] % 0x1000)
    {
    case 0x1: case 0x7:  
      fprintf(fp,
              "    if (Tcl_GetDouble(interp,argv[%i],&tempd) != TCL_OK) error = 1;\n",
              start_arg); 
      fprintf(fp,"    temp%i = tempd;\n",i);
      break;
    case 0x4: case 0x5: case 0x6: case 0xA: case 0xB: case 0xC: case 0xD:
      fprintf(fp,"    if (Tcl_GetInt(interp,argv[%i],&tempi) != TCL_OK) error = 1;\n",
              start_arg); 
      fprintf(fp,"    temp%i = tempi;\n",i);
      break;
    case 0x3:
      fprintf(fp,"    temp%i = *(argv[%i]);\n",i,start_arg);
      break;
    case 0x13:
      fprintf(fp,"    if (Tcl_GetInt(interp,argv[%i],&tempi) != TCL_OK) error = 1;\n",
              start_arg); 
      fprintf(fp,"    temp%i = (unsigned char)tempi;\n",i);
      break;
    case 0x14: case 0x1A: case 0x1B: case 0x1C:
      fprintf(fp,"    if (Tcl_GetInt(interp,argv[%i],&tempi) != TCL_OK) error = 1;\n",
              start_arg); 
      fprintf(fp,"    temp%i = (unsigned int)tempi;\n",i);
      break;
    case 0x15:
      fprintf(fp,"    if (Tcl_GetInt(interp,argv[%i],&tempi) != TCL_OK) error = 1;\n",
              start_arg); 
      fprintf(fp,"    temp%i = (unsigned short)tempi;\n",i);
      break;
    case 0x16:
      fprintf(fp,"    if (Tcl_GetInt(interp,argv[%i],&tempi) != TCL_OK) error = 1;\n",
              start_arg); 
      fprintf(fp,"    temp%i = (unsigned long)tempi;\n",i);
      break;
    case 0x303:
      fprintf(fp,"    temp%i = argv[%i];\n",i,start_arg);
      break;
    case 0x109:
    case 0x309:
      fprintf(fp,"    temp%i = (%s *)(vtkTclGetPointerFromObject(argv[%i],(char *) \"%s\",interp,error));\n",i,currentFunction->ArgClasses[i],start_arg,
              currentFunction->ArgClasses[i]);
      break;
    case 0x2:    
    case 0x9:
      break;
    default:
      if (currentFunction->ArgCounts[i] > 1)
        {
        for (j = 0; j < currentFunction->ArgCounts[i]; j++)
          {
          switch (currentFunction->ArgTypes[i] % 0x100)
            {
            case 0x1: case 0x7:  
              fprintf(fp,
                      "    if (Tcl_GetDouble(interp,argv[%i],&tempd) != TCL_OK) error = 1;\n",
                      start_arg); 
              fprintf(fp,"    temp%i[%i] = tempd;\n",i,j);
              break;
            case 0x4: case 0x5: case 0x6: case 0xA: case 0xB: case 0xC: case 0xD:
              fprintf(fp,"    if (Tcl_GetInt(interp,argv[%i],&tempi) != TCL_OK) error = 1;\n",
                      start_arg); 
              fprintf(fp,"    temp%i[%i] = tempi;\n",i,j);
              break;
            case 0x3:
              fprintf(fp,"    temp%i[%i] = *(argv[%i]);\n",i,j,start_arg);
              break;
            case 0x13:
              fprintf(fp,"    if (Tcl_GetInt(interp,argv[%i],&tempi) != TCL_OK) error = 1;\n",
                      start_arg); 
              fprintf(fp,"    temp%i[%i] = (unsigned char)tempi;\n",i,j);
              break;
            case 0x14: case 0x1A:
              fprintf(fp,"    if (Tcl_GetInt(interp,argv[%i],&tempi) != TCL_OK) error = 1;\n",
                      start_arg); 
              fprintf(fp,"    temp%i[%i] = (unsigned int)tempi;\n",i,j);
              break;
            case 0x15:
              fprintf(fp,"    if (Tcl_GetInt(interp,argv[%i],&tempi) != TCL_OK) error = 1;\n",
                      start_arg); 
              fprintf(fp,"    temp%i[%i] = (unsigned short)tempi;\n",i,j);
              break;
            case 0x16:
              fprintf(fp,"    if (Tcl_GetInt(interp,argv[%i],&tempi) != TCL_OK) error = 1;\n",
                      start_arg); 
              fprintf(fp,"    temp%i[%i] = (unsigned long)tempi;\n",i,j);
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
    if ((currentFunction->ArgTypes[i] % 0x10) == 0x8) args_ok = 0;
    /* if its a pointer arg make sure we have the ArgCount */
    if ((currentFunction->ArgTypes[i] % 0x1000 >= 0x100) &&
        (currentFunction->ArgTypes[i] % 0x1000 != 0x303)&&
        (currentFunction->ArgTypes[i] % 0x1000 != 0x309)&&
        (currentFunction->ArgTypes[i] % 0x1000 != 0x109)) 
      {
      if (currentFunction->NumberOfArguments > 1 ||
          !currentFunction->ArgCounts[i])
        {
        args_ok = 0;
        }
      }
    if ((currentFunction->ArgTypes[i] % 0x100 >= 0x10)&&
        (currentFunction->ArgTypes[i] != 0x13)&&
        (currentFunction->ArgTypes[i] != 0x14)&&
        (currentFunction->ArgTypes[i] != 0x15)&&
        (currentFunction->ArgTypes[i] != 0x16)&&
        (currentFunction->ArgTypes[i] != 0x1A)) args_ok = 0;
    }
  if ((currentFunction->ReturnType % 0x10) == 0x8) args_ok = 0;
  if (((currentFunction->ReturnType % 0x1000)/0x100 != 0x3)&&
      ((currentFunction->ReturnType % 0x1000)/0x100 != 0x1)&&
      ((currentFunction->ReturnType % 0x1000)/0x100)) args_ok = 0;
  if (currentFunction->NumberOfArguments && 
      (currentFunction->ArgTypes[0] == 0x5000)
      &&(currentFunction->NumberOfArguments != 1)) args_ok = 0;

  /* we can't handle void * return types */
  if ((currentFunction->ReturnType % 0x1000) == 0x302) 
    {
    args_ok = 0;
    }
  
  /* watch out for functions that dont have enough info */
  switch (currentFunction->ReturnType % 0x1000)
    {
    case 0x301: case 0x307:
    case 0x304: case 0x305: case 0x306: case 0x30A: case 0x30B: case 0x30C: case 0x30D:
    case 0x313: case 0x314: case 0x315: case 0x316: case 0x31A: case 0x31B: case 0x31C:
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
    
    if(currentFunction->IsLegacy)
      {
      fprintf(fp,"#if !defined(VTK_LEGACY_REMOVE)\n");
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

    /* only use the error variable if we have arguments to parse */
    if (currentFunction->NumberOfArguments)
      {
      fprintf(fp,"    error = 0;\n\n");
      /* now get the required args from the stack */
      for (i = 0; i < currentFunction->NumberOfArguments; i++)
        {
        get_args(fp,i);
        }
      fprintf(fp,"    if (!error)\n    {\n");
      }
    
    switch (currentFunction->ReturnType % 0x1000)
      {
      case 0x2:
        fprintf(fp,"    op->%s(",currentFunction->Name);
        break;
      case 0x109:
        fprintf(fp,"    temp%i = &(op)->%s(",MAX_ARGS,currentFunction->Name);
        break;
      default:
        fprintf(fp,"    temp%i = (op)->%s(",MAX_ARGS,currentFunction->Name);
      }
    for (i = 0; i < currentFunction->NumberOfArguments; i++)
      {
      if (i)
        {
        fprintf(fp,",");
        }
      if (currentFunction->ArgTypes[i] == 0x109)
        {
        fprintf(fp,"*(temp%i)",i);
        }
      else if (currentFunction->ArgTypes[i] == 0x5000)
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
        (currentFunction->ArgTypes[0] == 0x5000))
      {
      fprintf(fp,"    op->%sArgDelete(vtkTclVoidFuncArgDelete);\n",
              currentFunction->Name);
      }
    return_result(fp);
    fprintf(fp,"    return TCL_OK;\n");
    
    /* close the if error */
    if (currentFunction->NumberOfArguments)
      {
      fprintf(fp,"    }\n");
      }
    
    fprintf(fp,"    }\n");
    if(currentFunction->IsLegacy)
      {
      fprintf(fp,"#endif\n");
      }
    
    wrappedFunctions[numberOfWrappedFunctions] = currentFunction;
    numberOfWrappedFunctions++;
    }
}

/* print the parsed structures */
void vtkParseOutput(FILE *fp, FileInfo *data)
{
  int i,j;
  
  fprintf(fp,"// tcl wrapper for %s object\n//\n",data->ClassName);
  fprintf(fp,"#define VTK_WRAPPING_CXX\n");
  if (strcmp("vtkObjectBase",data->ClassName) != 0)
    {
      /* Block inclusion of full streams. */
    fprintf(fp,"#define VTK_STREAMS_FWD_ONLY\n");
    }
  fprintf(fp,"#include \"vtkSystemIncludes.h\"\n");
  fprintf(fp,"#include \"%s.h\"\n\n",data->ClassName);
  fprintf(fp,"#include \"vtkTclUtil.h\"\n");
  fprintf(fp,"#include <vtkstd/stdexcept>\n");
  if (data->IsConcrete)
    {
    if (strcmp(data->ClassName, "vtkRenderWindowInteractor") == 0)
      {
      fprintf(fp,"#include \"vtkToolkits.h\"\n");
      fprintf(fp,"#ifdef VTK_USE_X\n");
      fprintf(fp,"# include \"vtkXRenderWindowTclInteractor.h\"\n");
      fprintf(fp,"#endif\n");

      fprintf(fp,"\nClientData %sNewCommand()\n{\n",data->ClassName);

      fprintf(fp,"#ifdef VTK_USE_X\n");
      fprintf(fp,"  %s *temp = vtkXRenderWindowTclInteractor::New();\n",
              data->ClassName);
      fprintf(fp,"#else\n");
      fprintf(fp,"  %s *temp = %s::New();\n",data->ClassName,data->ClassName);
      fprintf(fp,"#endif\n");
      fprintf(fp,"  return ((ClientData)temp);\n}\n\n");
      }
    else
      {
      fprintf(fp,"\nClientData %sNewCommand()\n{\n",data->ClassName);
      fprintf(fp,"  %s *temp = %s::New();\n",data->ClassName,data->ClassName);
      fprintf(fp,"  return ((ClientData)temp);\n}\n\n");
      }
    }
  
  for (i = 0; i < data->NumberOfSuperClasses; i++)
    {
    fprintf(fp,"int %sCppCommand(%s *op, Tcl_Interp *interp,\n             int argc, char *argv[]);\n",data->SuperClasses[i],data->SuperClasses[i]);
    }
  fprintf(fp,"int VTKTCL_EXPORT %sCppCommand(%s *op, Tcl_Interp *interp,\n             int argc, char *argv[]);\n",data->ClassName,data->ClassName);
  fprintf(fp,"\nint VTKTCL_EXPORT %sCommand(ClientData cd, Tcl_Interp *interp,\n             int argc, char *argv[])\n{\n",data->ClassName);
  fprintf(fp,"  if ((argc == 2)&&(!strcmp(\"Delete\",argv[1]))&& !vtkTclInDelete(interp))\n    {\n");
  fprintf(fp,"    Tcl_DeleteCommand(interp,argv[0]);\n");
  fprintf(fp,"    return TCL_OK;\n    }\n");
  fprintf(fp,"   return %sCppCommand((%s *)(((vtkTclCommandArgStruct *)cd)->Pointer),interp, argc, argv);\n}\n",data->ClassName,data->ClassName);
  
  fprintf(fp,"\nint VTKTCL_EXPORT %sCppCommand(%s *op, Tcl_Interp *interp,\n             int argc, char *argv[])\n{\n",data->ClassName,data->ClassName);
  fprintf(fp,"  int    tempi;\n");
  fprintf(fp,"  double tempd;\n");
  fprintf(fp,"  static char temps[80];\n");
  fprintf(fp,"  int    error;\n\n");
  fprintf(fp,"  error = 0; error = error;\n");
  fprintf(fp,"  tempi = 0; tempi = tempi;\n");
  fprintf(fp,"  tempd = 0; tempd = tempd;\n");
  fprintf(fp,"  temps[0] = 0; temps[0] = temps[0];\n\n");

  fprintf(fp,"  if (argc < 2)\n    {\n    Tcl_SetResult(interp, (char *) \"Could not find requested method.\", TCL_VOLATILE);\n    return TCL_ERROR;\n    }\n");

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
  
  /* add the GetSuperClassName */
  if (data->NumberOfSuperClasses)
    {
    fprintf(fp,"  if (!strcmp(\"GetSuperClassName\",argv[1]))\n");
    fprintf(fp,"    {\n");
    fprintf(fp,"    Tcl_SetResult(interp,(char *) \"%s\", TCL_VOLATILE);\n",data->SuperClasses[0]);
    fprintf(fp,"    return TCL_OK;\n");
    fprintf(fp,"    }\n\n");      
    }
  
  fprintf(fp,"  try\n    {\n");

  /* insert function handling code here */
  for (i = 0; i < data->NumberOfFunctions; i++)
    {
    currentFunction = data->Functions + i;
    outputFunction(fp, data);
    }
  
  /* add the ListInstances method */
  fprintf(fp,"\n  if (!strcmp(\"ListInstances\",argv[1]))\n    {\n");
  fprintf(fp,"    vtkTclListInstances(interp,(ClientData)%sCommand);\n",data->ClassName);
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
  fprintf(fp,"    Tcl_AppendResult(interp,\"  GetSuperClassName\\n\",NULL);\n");
  for (i = 0; i < numberOfWrappedFunctions; i++)
    {
    int numArgs = 0;

    currentFunction = wrappedFunctions[i];
    if(currentFunction->IsLegacy)
      {
      fprintf(fp,"#if !defined(VTK_LEGACY_REMOVE)\n");
      }
            
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

    if(currentFunction->IsLegacy)
      {
      fprintf(fp,"#endif\n");
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
  
  /* Add the Print method to vtkObjectBase. */
  if (!strcmp("vtkObjectBase",data->ClassName))
    {
    fprintf(fp,"  if ((!strcmp(\"Print\",argv[1]))&&(argc == 2))\n    {\n");
    fprintf(fp,"    ostrstream buf_with_warning_C4701;\n");
    fprintf(fp,"    op->Print(buf_with_warning_C4701);\n");
    fprintf(fp,"    buf_with_warning_C4701.put('\\0');\n");
    fprintf(fp,"    Tcl_SetResult(interp,buf_with_warning_C4701.str(),\n");
    fprintf(fp,"      TCL_VOLATILE);\n");
    fprintf(fp,"    buf_with_warning_C4701.rdbuf()->freeze(0);\n");
    fprintf(fp,"    return TCL_OK;\n    }\n");
    /* Add the PrintRevisions method to vtkObjectBase. */
    fprintf(fp,"  if ((!strcmp(\"PrintRevisions\",argv[1]))&&(argc == 2))\n    {\n");
    fprintf(fp,"    ostrstream buf_with_warning_C4701;\n");
    fprintf(fp,"    op->PrintRevisions(buf_with_warning_C4701);\n");
    fprintf(fp,"    buf_with_warning_C4701.put('\\0');\n");
    fprintf(fp,"    Tcl_SetResult(interp,buf_with_warning_C4701.str(),\n");
    fprintf(fp,"      TCL_VOLATILE);\n");
    fprintf(fp,"    delete [] buf_with_warning_C4701.str();\n");
    fprintf(fp,"    return TCL_OK;\n    }\n");    
    }
  /* Add the AddObserver method to vtkObject. */
  if (!strcmp("vtkObject",data->ClassName))
    {
    fprintf(fp,"  if ((!strcmp(\"AddObserver\",argv[1]))&&(argc >= 4))\n    {\n");
    fprintf(fp,"    error = 0;\n");
    fprintf(fp,"    if (argc > 4 && Tcl_GetDouble(interp,argv[4],&tempd) != TCL_OK) error = 1;\n");
    fprintf(fp,"    if (!error)\n      {\n");
    fprintf(fp,"      vtkTclCommand *cbc = vtkTclCommand::New();\n");
    fprintf(fp,"      cbc->SetInterp(interp);\n");
    fprintf(fp,"      cbc->SetStringCommand(argv[3]);\n");
    fprintf(fp,"      unsigned long      temp20;\n");
    fprintf(fp,"      if (argc > 4)\n        {\n");
    fprintf(fp,"        temp20 = op->AddObserver(argv[2],cbc,tempd);\n");
    fprintf(fp,"        }\n      else\n        {\n");
    fprintf(fp,"        temp20 = op->AddObserver(argv[2],cbc);\n");
    fprintf(fp,"        }\n");
    fprintf(fp,"      cbc->Delete();\n");
    fprintf(fp,"      char tempResult[1024];\n");
    fprintf(fp,"      sprintf(tempResult,\"%%li\",temp20);\n");
    fprintf(fp,"      Tcl_SetResult(interp,tempResult,TCL_VOLATILE);\n");
    fprintf(fp,"      return TCL_OK;\n      }\n");
    fprintf(fp,"    }\n");
    }
  fprintf(fp,"\n  if ((argc >= 2)&&(!strstr(interp->result,\"Object named:\")))\n    {\n");
  fprintf(fp,"    char temps2[256];\n    sprintf(temps2,\"Object named: %%s, could not find requested method: %%s\\nor the method was called with incorrect arguments.\\n\",argv[0],argv[1]);\n    Tcl_AppendResult(interp,temps2,NULL);\n    }\n");
  fprintf(fp,"    }\n");
  fprintf(fp,"  catch (vtkstd::exception &e)\n");
  fprintf(fp,"    {\n");
  fprintf(fp,"    Tcl_AppendResult(interp, \"Uncaught exception: \",  e.what(), \"\\n\", NULL);\n");
  fprintf(fp,"    return TCL_ERROR;\n");
  fprintf(fp,"    }\n");
  fprintf(fp,"  return TCL_ERROR;\n}\n");

}
