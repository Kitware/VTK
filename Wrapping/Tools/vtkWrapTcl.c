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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vtkParse.h"
#include "vtkParseMain.h"
#include "vtkParseHierarchy.h"
#include "vtkConfigure.h"
#include "vtkWrap.h"

HierarchyInfo *hierarchyInfo = NULL;
StringCache *stringCache = NULL;
int numberOfWrappedFunctions = 0;
FunctionInfo *wrappedFunctions[1000];
extern FunctionInfo *currentFunction;

/* convert special characters in a string into their escape codes,
   so that the string can be quoted in a source file (the specified
   maxlen must be at least 32 chars)*/
static const char *quote_string(const char *comment, size_t maxlen)
{
  static char *result = 0;
  static size_t oldmaxlen = 0;
  size_t i, j, n;

  if (maxlen > oldmaxlen)
    {
    if (result)
      {
      free(result);
      }
    result = (char *)malloc(maxlen+1);
    oldmaxlen = maxlen;
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
    else if (comment[i] == ']')
      {
      strcpy(&result[j],"\\\\]");
      j += 3;
      }
    else if (comment[i] == '[')
      {
      strcpy(&result[j],"\\\\[");
      j += 3;
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
      j += (int)strlen(" ...\\n [Truncated]\\n");
      break;
      }
    }

  result[j] = '\0';

  return result;
}

void output_temp(FILE *fp, int i, unsigned int aType,
                 const char *Id, int count)
{
  /* handle VAR FUNCTIONS */
  if (aType == VTK_PARSE_FUNCTION)
    {
    fprintf(fp,"    vtkTclVoidFuncArg *temp%i = new vtkTclVoidFuncArg;\n",i);
    return;
    }

  /* ignore void */
  if ((aType & VTK_PARSE_UNQUALIFIED_TYPE) == VTK_PARSE_VOID)
    {
    return;
    }

  /* for const * return types prototype with const */
  if ((i == MAX_ARGS) &&
      ((aType & VTK_PARSE_INDIRECT) != 0) &&
      ((aType & VTK_PARSE_CONST) != 0))
    {
    fprintf(fp,"    const ");
    }
  else
    {
    fprintf(fp,"    ");
    }

  if ((aType & VTK_PARSE_UNSIGNED) != 0)
    {
    fprintf(fp,"unsigned ");
    }

  switch ((aType & VTK_PARSE_BASE_TYPE) & ~VTK_PARSE_UNSIGNED)
    {
    case VTK_PARSE_FLOAT:       fprintf(fp,"float  "); break;
    case VTK_PARSE_DOUBLE:      fprintf(fp,"double "); break;
    case VTK_PARSE_INT:         fprintf(fp,"int    "); break;
    case VTK_PARSE_SHORT:       fprintf(fp,"short  "); break;
    case VTK_PARSE_LONG:        fprintf(fp,"long   "); break;
    case VTK_PARSE_VOID:        fprintf(fp,"void   "); break;
    case VTK_PARSE_CHAR:        fprintf(fp,"char   "); break;
    case VTK_PARSE_OBJECT:      fprintf(fp,"%s ",Id); break;
    case VTK_PARSE_ID_TYPE:     fprintf(fp,"vtkIdType "); break;
    case VTK_PARSE_LONG_LONG:   fprintf(fp,"long long "); break;
    case VTK_PARSE___INT64:     fprintf(fp,"__int64 "); break;
    case VTK_PARSE_SIGNED_CHAR: fprintf(fp,"signed char "); break;
    case VTK_PARSE_BOOL:        fprintf(fp,"bool "); break;
    case VTK_PARSE_STRING:      fprintf(fp,"%s ",Id); break;
    case VTK_PARSE_UNKNOWN:     fprintf(fp,"%s ",Id); break;
    }

  /* handle array arguments */
  if (count > 1)
    {
    fprintf(fp,"temp%i[%i];\n",i,count);
    return;
    }

  switch (aType & VTK_PARSE_INDIRECT)
    {
    case VTK_PARSE_REF:
      if (i == MAX_ARGS)
        {
        fprintf(fp, " *"); /* act " &" */
        }
      break;
    case VTK_PARSE_POINTER:         fprintf(fp, " *"); break;
    case VTK_PARSE_POINTER_REF:     fprintf(fp, "*&"); break;
    case VTK_PARSE_POINTER_POINTER: fprintf(fp, "**"); break;
    default:                        fprintf(fp,"  "); break;
    }

  fprintf(fp,"temp%i",i);
  fprintf(fp,";\n");
}

/* when the cpp file doesn't have enough info use the hint file */
void use_hints(FILE *fp)
{
  int  i;

#define INDENT "    "

  fprintf(fp,INDENT "if(temp%i)\n",MAX_ARGS);
  fprintf(fp,INDENT "  {\n");
  fprintf(fp,INDENT "  char tempResult[1024];\n");
  fprintf(fp,INDENT "  *tempResult = '\\0';\n");

  /* special case for double: use Tcl_PrintDouble to control precision */
  if (((currentFunction->ReturnType & VTK_PARSE_UNQUALIFIED_TYPE)
       != VTK_PARSE_FLOAT_PTR) &&
      ((currentFunction->ReturnType & VTK_PARSE_UNQUALIFIED_TYPE)
       != VTK_PARSE_DOUBLE_PTR))
    {
    fprintf(fp,INDENT "  sprintf(tempResult,\"");
    }

  /* use the hint */
  switch (currentFunction->ReturnType & VTK_PARSE_UNQUALIFIED_TYPE)
    {
    case VTK_PARSE_FLOAT_PTR:
    case VTK_PARSE_DOUBLE_PTR:
      fprintf(fp,INDENT "  char converted[1024];\n");
      fprintf(fp,INDENT "  *converted = '\\0';\n");
      for (i = 0; i < currentFunction->HintSize; i++)
        {
        fprintf(fp,INDENT "  Tcl_PrintDouble(interp,temp%i[%i], converted);\n",MAX_ARGS,i);
        fprintf(fp,INDENT "  strcat(tempResult, \" \");\n");
        fprintf(fp,INDENT "  strcat(tempResult, converted);\n");
        }
      break;

    case VTK_PARSE_INT_PTR:
    case VTK_PARSE_SHORT_PTR:
    case VTK_PARSE_SIGNED_CHAR_PTR:
#ifndef VTK_USE_64BIT_IDS
    case VTK_PARSE_ID_TYPE_PTR:
#endif
      for (i = 0; i < currentFunction->HintSize; i++)
        {
        fprintf(fp,"%%i ");
        }
      break;

    case VTK_PARSE_BOOL_PTR:
      for (i = 0; i < currentFunction->HintSize; i++)
        {
        fprintf(fp,"%%i ");
        }
      break;

    case VTK_PARSE_LONG_PTR:
      for (i = 0; i < currentFunction->HintSize; i++)
        {
        fprintf(fp,"%%li ");
        }
      break;

#ifdef VTK_USE_64BIT_IDS
    case VTK_PARSE_ID_TYPE_PTR:
      for (i = 0; i < currentFunction->HintSize; i++)
        {
#  if defined(_MSC_VER)
        fprintf(fp,"%%I64i ");
#  else
        fprintf(fp,"%%lli ");
#  endif
        }
      break;
#endif

    case VTK_PARSE_LONG_LONG_PTR:
      for (i = 0; i < currentFunction->HintSize; i++)
        {
        fprintf(fp,"%%lli ");
        }
      break;

    case VTK_PARSE___INT64_PTR:
      for (i = 0; i < currentFunction->HintSize; i++)
        {
        fprintf(fp,"%%I64i ");
        }
      break;

    case VTK_PARSE_UNSIGNED_CHAR_PTR:
    case VTK_PARSE_UNSIGNED_INT_PTR:
    case VTK_PARSE_UNSIGNED_SHORT_PTR:
#ifndef VTK_USE_64BIT_IDS
    case VTK_PARSE_UNSIGNED_ID_TYPE_PTR:
#endif
      for (i = 0; i < currentFunction->HintSize; i++)
        {
        fprintf(fp,"%%u ");
        }
      break;

    case VTK_PARSE_UNSIGNED_LONG_PTR:
      for (i = 0; i < currentFunction->HintSize; i++)
        {
        fprintf(fp,"%%lu ");
        }
      break;

#ifdef VTK_USE_64BIT_IDS
    case VTK_PARSE_UNSIGNED_ID_TYPE_PTR:
      for (i = 0; i < currentFunction->HintSize; i++)
        {
#  if defined(_MSC_VER)
        fprintf(fp,"%%I64u ");
#  else
        fprintf(fp,"%%llu ");
#  endif
        }
      break;
#endif

    case VTK_PARSE_UNSIGNED_LONG_LONG_PTR:
      for (i = 0; i < currentFunction->HintSize; i++)
        {
        fprintf(fp,"%%llu ");
        }
      break;

    case VTK_PARSE_UNSIGNED___INT64_PTR:
      for (i = 0; i < currentFunction->HintSize; i++)
        {
        fprintf(fp,"%%I64u ");
        }
      break;
    }

  if (((currentFunction->ReturnType & VTK_PARSE_UNQUALIFIED_TYPE)
       != VTK_PARSE_FLOAT_PTR) &&
      ((currentFunction->ReturnType & VTK_PARSE_UNQUALIFIED_TYPE)
       != VTK_PARSE_DOUBLE_PTR))
    {
    fprintf(fp,"\"");
    for (i = 0; i < currentFunction->HintSize; i++)
      {
      fprintf(fp,",temp%i[%i]",MAX_ARGS,i);
      }
    fprintf(fp,");\n");
    }

  fprintf(fp,INDENT "  Tcl_SetResult(interp, tempResult, TCL_VOLATILE);\n");
  fprintf(fp,INDENT "  }\n");
  fprintf(fp,INDENT "else\n");
  fprintf(fp,INDENT "  {\n");
  fprintf(fp,INDENT "  Tcl_SetResult(interp, const_cast<char *>(\"\"), TCL_VOLATILE);\n");
  fprintf(fp,INDENT "  }\n");

#undef INDENT
}

void return_result(FILE *fp)
{
  switch (currentFunction->ReturnType & VTK_PARSE_UNQUALIFIED_TYPE)
    {
    case VTK_PARSE_VOID:
      fprintf(fp,"    Tcl_ResetResult(interp);\n");
      break;
    case VTK_PARSE_FLOAT:
    case VTK_PARSE_DOUBLE:
      fprintf(fp,"    char tempResult[1024];\n");
       /*
        * use tcl's print double function to support variable
        * precision at runtime
        */
      fprintf(fp,"    Tcl_PrintDouble(interp,temp%i,tempResult);\n",
              MAX_ARGS);
      fprintf(fp,"    Tcl_SetResult(interp, tempResult, TCL_VOLATILE);\n");
      break;
    case VTK_PARSE_INT:
#ifndef VTK_USE_64BIT_IDS
    case VTK_PARSE_ID_TYPE:
#endif
    case VTK_PARSE_SIGNED_CHAR:
      /* rely on promotion to integer, since "%hhi" is non-standard */
      fprintf(fp,"    char tempResult[1024];\n");
      fprintf(fp,"    sprintf(tempResult,\"%%i\",temp%i);\n",
              MAX_ARGS);
      fprintf(fp,"    Tcl_SetResult(interp, tempResult, TCL_VOLATILE);\n");
      break;
    case VTK_PARSE_BOOL:
      fprintf(fp,"    char tempResult[1024];\n");
      fprintf(fp,"    sprintf(tempResult,\"%%i\",(int)temp%i);\n",
              MAX_ARGS);
      fprintf(fp,"    Tcl_SetResult(interp, tempResult, TCL_VOLATILE);\n");
      break;
    case VTK_PARSE_SHORT:
      fprintf(fp,"    char tempResult[1024];\n");
      fprintf(fp,"    sprintf(tempResult,\"%%hi\",temp%i);\n",
              MAX_ARGS);
      fprintf(fp,"    Tcl_SetResult(interp, tempResult, TCL_VOLATILE);\n");
      break;
    case VTK_PARSE_LONG:
      fprintf(fp,"    char tempResult[1024];\n");
      fprintf(fp,"    sprintf(tempResult,\"%%li\",temp%i);\n",
              MAX_ARGS);
      fprintf(fp,"    Tcl_SetResult(interp, tempResult, TCL_VOLATILE);\n");
      break;
#ifdef VTK_USE_64BIT_IDS
    case VTK_PARSE_ID_TYPE:
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
    case VTK_PARSE_LONG_LONG:
      fprintf(fp,"    char tempResult[1024];\n");
      fprintf(fp,"    sprintf(tempResult,\"%%lli\",temp%i);\n",
              MAX_ARGS);
      fprintf(fp,"    Tcl_SetResult(interp, tempResult, TCL_VOLATILE);\n");
      break;
    case VTK_PARSE___INT64:
      fprintf(fp,"    char tempResult[1024];\n");
      fprintf(fp,"    sprintf(tempResult,\"%%I64i\",temp%i);\n",
              MAX_ARGS);
      fprintf(fp,"    Tcl_SetResult(interp, tempResult, TCL_VOLATILE);\n");
      break;
    case VTK_PARSE_UNSIGNED_INT:
#ifndef VTK_USE_64BIT_IDS
    case VTK_PARSE_UNSIGNED_ID_TYPE:
#endif
      fprintf(fp,"    char tempResult[1024];\n");
      fprintf(fp,"    sprintf(tempResult,\"%%u\",temp%i);\n",
              MAX_ARGS);
      fprintf(fp,"    Tcl_SetResult(interp, tempResult, TCL_VOLATILE);\n");
      break;
    case VTK_PARSE_UNSIGNED_SHORT:
      fprintf(fp,"    char tempResult[1024];\n");
      fprintf(fp,"    sprintf(tempResult,\"%%hu\",temp%i);\n",
              MAX_ARGS);
      fprintf(fp,"    Tcl_SetResult(interp, tempResult, TCL_VOLATILE);\n");
      break;
    case VTK_PARSE_UNSIGNED_LONG:
      fprintf(fp,"    char tempResult[1024];\n");
      fprintf(fp,"    sprintf(tempResult,\"%%lu\",temp%i);\n",
              MAX_ARGS);
      fprintf(fp,"    Tcl_SetResult(interp, tempResult, TCL_VOLATILE);\n");
      break;
    case VTK_PARSE_UNSIGNED_CHAR:
      /* rely on promotion to integer, since "%hhu" is non-standard */
      fprintf(fp,"    char tempResult[1024];\n");
      fprintf(fp,"    sprintf(tempResult,\"%%i\",temp%i);\n",
              MAX_ARGS);
      fprintf(fp,"    Tcl_SetResult(interp, tempResult, TCL_VOLATILE);\n");
      break;
#ifdef VTK_USE_64BIT_IDS
    case VTK_PARSE_UNSIGNED_ID_TYPE:
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
    case VTK_PARSE_UNSIGNED_LONG_LONG:
      fprintf(fp,"    char tempResult[1024];\n");
      fprintf(fp,"    sprintf(tempResult,\"%%llu\",temp%i);\n",
              MAX_ARGS);
      fprintf(fp,"    Tcl_SetResult(interp, tempResult, TCL_VOLATILE);\n");
      break;
    case VTK_PARSE_UNSIGNED___INT64:
      fprintf(fp,"    char tempResult[1024];\n");
      fprintf(fp,"    sprintf(tempResult,\"%%I64u\",temp%i);\n",
              MAX_ARGS);
      fprintf(fp,"    Tcl_SetResult(interp, tempResult, TCL_VOLATILE);\n");
      break;
    case VTK_PARSE_UNKNOWN:
      fprintf(fp,"    char tempResult[1024];\n");
      fprintf(fp,"    sprintf(tempResult,\"%%i\",static_cast<int>(temp%i));\n",
              MAX_ARGS);
      fprintf(fp,"    Tcl_SetResult(interp, tempResult, TCL_VOLATILE);\n");
      break;
    case VTK_PARSE_STRING:
      fprintf(fp,"    Tcl_SetResult(interp, const_cast<char *>(temp%i.c_str()), TCL_VOLATILE);\n",MAX_ARGS);
      break;
    case VTK_PARSE_STRING_REF:
      fprintf(fp,"    Tcl_SetResult(interp, const_cast<char *>(temp%i->c_str()), TCL_VOLATILE);\n",MAX_ARGS);
      break;
    case VTK_PARSE_CHAR_PTR:
      fprintf(fp,"    if (temp%i)\n      {\n      Tcl_SetResult(interp, const_cast<char *>(temp%i), TCL_VOLATILE);\n",MAX_ARGS,MAX_ARGS);
      fprintf(fp,"      }\n    else\n      {\n");
      fprintf(fp,"      Tcl_ResetResult(interp);\n      }\n");
      break;
    case VTK_PARSE_CHAR:
      fprintf(fp,"    char tempResult[1024];\n");
      fprintf(fp,"    sprintf(tempResult,\"%%c\",temp%i);\n",
              MAX_ARGS);
      fprintf(fp,"    Tcl_SetResult(interp, tempResult, TCL_VOLATILE);\n");
      break;
    case VTK_PARSE_OBJECT_PTR:
      fprintf(fp,"      vtkTclGetObjectFromPointer(interp,(void *)(temp%i),\"%s\");\n",MAX_ARGS,currentFunction->ReturnClass);
      break;

    /* handle functions returning vectors */
    /* this is done by looking them up in a hint file */
    case VTK_PARSE_FLOAT_PTR:
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
    case VTK_PARSE_UNSIGNED_INT_PTR:
    case VTK_PARSE_UNSIGNED_SHORT_PTR:
    case VTK_PARSE_UNSIGNED_LONG_PTR:
    case VTK_PARSE_UNSIGNED_ID_TYPE_PTR:
    case VTK_PARSE_UNSIGNED_LONG_LONG_PTR:
    case VTK_PARSE_UNSIGNED___INT64_PTR:
      use_hints(fp);
      break;
    default:
      fprintf(fp,"    Tcl_SetResult(interp, const_cast<char *>(\"unable to return result.\"), TCL_VOLATILE);\n");
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
  if (currentFunction->ArgTypes[i] == VTK_PARSE_FUNCTION)
    {
    fprintf(fp,"    temp%i->interp = interp;\n",i);
    fprintf(fp,"    temp%i->command = strcpy(new char [strlen(argv[2])+1],argv[2]);\n",i);
    return;
    }

  /* ignore void */
  if ((currentFunction->ArgTypes[i] & VTK_PARSE_UNQUALIFIED_TYPE)
      == VTK_PARSE_VOID)
    {
    return;
    }

  switch (currentFunction->ArgTypes[i] & VTK_PARSE_UNQUALIFIED_TYPE)
    {
    case VTK_PARSE_FLOAT:
    case VTK_PARSE_DOUBLE:
      fprintf(fp,
              "    if (Tcl_GetDouble(interp,argv[%i],&tempd) != TCL_OK) error = 1;\n",
              start_arg);
      fprintf(fp,"    temp%i = tempd;\n",i);
      break;
    case VTK_PARSE_INT:
    case VTK_PARSE_SHORT:
    case VTK_PARSE_LONG:
    case VTK_PARSE_ID_TYPE:
    case VTK_PARSE_LONG_LONG:
    case VTK_PARSE___INT64:
    case VTK_PARSE_SIGNED_CHAR:
      fprintf(fp,"    if (Tcl_GetInt(interp,argv[%i],&tempi) != TCL_OK) error = 1;\n",
              start_arg);
      fprintf(fp,"    temp%i = tempi;\n",i);
      break;
    case VTK_PARSE_BOOL:
      fprintf(fp,"    if (Tcl_GetInt(interp,argv[%i],&tempi) != TCL_OK) error = 1;\n",
              start_arg);
      fprintf(fp,"    temp%i = tempi ? true : false;\n",i);
      break;
    case VTK_PARSE_CHAR:
      fprintf(fp,"    temp%i = *(argv[%i]);\n",i,start_arg);
      break;
    case VTK_PARSE_UNSIGNED_CHAR:
      fprintf(fp,"    if (Tcl_GetInt(interp,argv[%i],&tempi) != TCL_OK) error = 1;\n",
              start_arg);
      fprintf(fp,"    temp%i = static_cast<unsigned char>(tempi);\n",i);
      break;
    case VTK_PARSE_UNSIGNED_INT: case VTK_PARSE_UNSIGNED_ID_TYPE:
      fprintf(fp,"    if (Tcl_GetInt(interp,argv[%i],&tempi) != TCL_OK) error = 1;\n",
              start_arg);
      fprintf(fp,"    temp%i = static_cast<unsigned int>(tempi);\n",i);
      break;
    case VTK_PARSE_UNSIGNED_SHORT:
      fprintf(fp,"    if (Tcl_GetInt(interp,argv[%i],&tempi) != TCL_OK) error = 1;\n",
              start_arg);
      fprintf(fp,"    temp%i = static_cast<unsigned short>(tempi);\n",i);
      break;
    case VTK_PARSE_UNSIGNED_LONG:
      fprintf(fp,"    if (Tcl_GetInt(interp,argv[%i],&tempi) != TCL_OK) error = 1;\n",
              start_arg);
      fprintf(fp,"    temp%i = static_cast<unsigned long>(tempi);\n",i);
      break;
    case VTK_PARSE_UNSIGNED_LONG_LONG:
    case VTK_PARSE_UNSIGNED___INT64:
      fprintf(fp,"    if (Tcl_GetInt(interp,argv[%i],&tempi) != TCL_OK) error = 1;\n",
              start_arg);
      fprintf(fp,"    temp%i = static_cast<unsigned long long>(tempi);\n",i);
      break;
    case VTK_PARSE_UNKNOWN:
      fprintf(fp,"    if (Tcl_GetInt(interp,argv[%i],&tempi) != TCL_OK) error = 1;\n",
              start_arg);
      fprintf(fp,"    temp%i = static_cast<%s>(tempi);\n",i,
              currentFunction->ArgClasses[i]);
      break;
    case VTK_PARSE_STRING:
    case VTK_PARSE_STRING_REF:
      fprintf(fp,"    temp%i = argv[%i];\n",i,start_arg);
      break;
    case VTK_PARSE_CHAR_PTR:
      fprintf(fp,"    temp%i = argv[%i];\n",i,start_arg);
      break;
    case VTK_PARSE_OBJECT_PTR:
      fprintf(fp,"    temp%i = (%s *)(vtkTclGetPointerFromObject(argv[%i],const_cast<char *>(\"%s\"),interp,error));\n",i,currentFunction->ArgClasses[i],start_arg,
              currentFunction->ArgClasses[i]);
      break;
    case VTK_PARSE_VOID:
    case VTK_PARSE_OBJECT:
    case VTK_PARSE_OBJECT_REF:
      break;
    default:
      if (currentFunction->ArgCounts[i] > 1)
        {
        for (j = 0; j < currentFunction->ArgCounts[i]; j++)
          {
          switch (currentFunction->ArgTypes[i] & VTK_PARSE_BASE_TYPE)
            {
            case VTK_PARSE_FLOAT:
            case VTK_PARSE_DOUBLE:
              fprintf(fp,
                      "    if (Tcl_GetDouble(interp,argv[%i],&tempd) != TCL_OK) error = 1;\n",
                      start_arg);
              fprintf(fp,"    temp%i[%i] = tempd;\n",i,j);
              break;
            case VTK_PARSE_INT:
            case VTK_PARSE_SHORT:
            case VTK_PARSE_LONG:
            case VTK_PARSE_ID_TYPE:
            case VTK_PARSE_LONG_LONG:
            case VTK_PARSE___INT64:
            case VTK_PARSE_SIGNED_CHAR:
            case VTK_PARSE_UNKNOWN:
              fprintf(fp,"    if (Tcl_GetInt(interp,argv[%i],&tempi) != TCL_OK) error = 1;\n",
                      start_arg);
              fprintf(fp,"    temp%i[%i] = tempi;\n",i,j);
              break;
            case VTK_PARSE_BOOL:
              fprintf(fp,"    if (Tcl_GetInt(interp,argv[%i],&tempi) != TCL_OK) error = 1;\n",
                      start_arg);
              fprintf(fp,"    temp%i[%i] = tempi ? true : false;\n",i,j);
              break;
            case VTK_PARSE_CHAR:
              fprintf(fp,"    temp%i[%i] = *(argv[%i]);\n",i,j,start_arg);
              break;
            case VTK_PARSE_UNSIGNED_CHAR:
              fprintf(fp,"    if (Tcl_GetInt(interp,argv[%i],&tempi) != TCL_OK) error = 1;\n",
                      start_arg);
              fprintf(fp,"    temp%i[%i] = static_cast<unsigned char>(tempi);\n",i,j);
              break;
            case VTK_PARSE_UNSIGNED_INT:
            case VTK_PARSE_UNSIGNED_ID_TYPE:
              fprintf(fp,"    if (Tcl_GetInt(interp,argv[%i],&tempi) != TCL_OK) error = 1;\n",
                      start_arg);
              fprintf(fp,"    temp%i[%i] = static_cast<unsigned int>(tempi);\n",i,j);
              break;
            case VTK_PARSE_UNSIGNED_SHORT:
              fprintf(fp,"    if (Tcl_GetInt(interp,argv[%i],&tempi) != TCL_OK) error = 1;\n",
                      start_arg);
              fprintf(fp,"    temp%i[%i] = static_cast<unsigned short>(tempi);\n",i,j);
              break;
            case VTK_PARSE_UNSIGNED_LONG:
              fprintf(fp,"    if (Tcl_GetInt(interp,argv[%i],&tempi) != TCL_OK) error = 1;\n",
                      start_arg);
              fprintf(fp,"    temp%i[%i] = static_cast<unsigned long>(tempi);\n",i,j);
              break;
            }
          start_arg++;
          }
        }

    }
}

/* make a guess about whether a class is wrapped */
static int isClassWrapped(const char *classname)
{
  HierarchyEntry *entry;

  if (hierarchyInfo)
    {
    entry = vtkParseHierarchy_FindEntry(hierarchyInfo, classname);

    if (entry)
      {
      /* only allow non-excluded vtkObjects as args */
      if (vtkParseHierarchy_GetProperty(entry, "WRAP_EXCLUDE") ||
          !vtkParseHierarchy_IsTypeOf(hierarchyInfo, entry, "vtkObject"))
        {
        /* make a special exemption for vtkObjectBase */
        if (strcmp(classname, "vtkObjectBase") != 0)
          {
          return 0;
          }
        }
      }
    }

  return 1;
}

/* check whether a function is wrappable */
int checkFunctionSignature(ClassInfo *data)
{
  static unsigned int supported_types[] = {
    VTK_PARSE_VOID, VTK_PARSE_BOOL, VTK_PARSE_FLOAT, VTK_PARSE_DOUBLE,
    VTK_PARSE_CHAR, VTK_PARSE_UNSIGNED_CHAR, VTK_PARSE_SIGNED_CHAR,
    VTK_PARSE_INT, VTK_PARSE_UNSIGNED_INT,
    VTK_PARSE_SHORT, VTK_PARSE_UNSIGNED_SHORT,
    VTK_PARSE_LONG, VTK_PARSE_UNSIGNED_LONG,
    VTK_PARSE_ID_TYPE, VTK_PARSE_UNSIGNED_ID_TYPE,
#ifdef VTK_TYPE_USE_LONG_LONG
    VTK_PARSE_LONG_LONG, VTK_PARSE_UNSIGNED_LONG_LONG,
#endif
#ifdef VTK_TYPE_USE___INT64
    VTK_PARSE___INT64, VTK_PARSE_UNSIGNED___INT64,
#endif
    VTK_PARSE_OBJECT, VTK_PARSE_STRING, VTK_PARSE_UNKNOWN,
    0
  };

  int i, j;
  int args_ok = 1;
  unsigned int returnType = 0;
  unsigned int argType = 0;
  unsigned int baseType = 0;

  /* some functions will not get wrapped no matter what else */
  if (currentFunction->IsOperator ||
      currentFunction->ArrayFailure ||
      !currentFunction->IsPublic ||
      !currentFunction->Name)
    {
    return 0;
    }

  /* function pointer arguments for callbacks */
  if (currentFunction->NumberOfArguments == 2 &&
      currentFunction->ArgTypes[0] == VTK_PARSE_FUNCTION &&
      currentFunction->ArgTypes[1] == VTK_PARSE_VOID_PTR &&
      (currentFunction->ReturnType & VTK_PARSE_UNQUALIFIED_TYPE)
      == VTK_PARSE_VOID)
    {
    return 1;
    }

  /* check to see if we can handle the args */
  for (i = 0; i < currentFunction->NumberOfArguments; i++)
    {
    argType = (currentFunction->ArgTypes[i] & VTK_PARSE_UNQUALIFIED_TYPE);
    baseType = (argType & VTK_PARSE_BASE_TYPE);

    for (j = 0; supported_types[j] != 0; j++)
      {
      if (baseType == supported_types[j]) { break; }
      }
    if (supported_types[j] == 0)
      {
      args_ok = 0;
      }

    if (baseType == VTK_PARSE_UNKNOWN)
      {
      const char *qualified_name = 0;
      if ((argType & VTK_PARSE_INDIRECT) == 0)
        {
        qualified_name = vtkParseHierarchy_QualifiedEnumName(
          hierarchyInfo, data, stringCache,
          currentFunction->ArgClasses[i]);
        }
      if (qualified_name)
        {
        currentFunction->ArgClasses[i] = qualified_name;
        }
      else
        {
        args_ok = 0;
        }
      }

    if (baseType == VTK_PARSE_STRING &&
        (argType & VTK_PARSE_INDIRECT) != 0 &&
        (argType & VTK_PARSE_INDIRECT) != VTK_PARSE_REF)
      {
      args_ok = 0;
      }

    if (baseType == VTK_PARSE_OBJECT)
      {
      if ((argType & VTK_PARSE_INDIRECT) != VTK_PARSE_POINTER)
        {
        args_ok = 0;
        }
      else if (!isClassWrapped(currentFunction->ArgClasses[i]))
        {
        args_ok = 0;
        }
      }

    if (((argType & VTK_PARSE_INDIRECT) != VTK_PARSE_POINTER) &&
        ((argType & VTK_PARSE_INDIRECT) != VTK_PARSE_REF) &&
        ((argType & VTK_PARSE_INDIRECT) != 0)) args_ok = 0;

    /* if its a pointer arg make sure we have the ArgCount */
    if (((argType & VTK_PARSE_INDIRECT) != 0) &&
        (argType != VTK_PARSE_CHAR_PTR) &&
        (baseType != VTK_PARSE_OBJECT))
      {
      if (currentFunction->NumberOfArguments > 1 ||
          !currentFunction->ArgCounts[i])
        {
        args_ok = 0;
        }
      }
    if (((argType & VTK_PARSE_UNSIGNED) != 0) &&
        (argType != VTK_PARSE_UNSIGNED_CHAR) &&
        (argType != VTK_PARSE_UNSIGNED_INT) &&
        (argType != VTK_PARSE_UNSIGNED_SHORT) &&
        (argType != VTK_PARSE_UNSIGNED_LONG) &&
        (argType != VTK_PARSE_UNSIGNED_LONG_LONG) &&
        (argType != VTK_PARSE_UNSIGNED_ID_TYPE))
      {
      args_ok = 0;
      }

    /* don't allow "char []", only allow "char *" */
    if (argType == VTK_PARSE_CHAR_PTR && currentFunction->ArgCounts[i] != 0)
      {
      args_ok = 0;
      }
    }

  /* check the return type */
  returnType = (currentFunction->ReturnType & VTK_PARSE_UNQUALIFIED_TYPE);
  baseType = (returnType & VTK_PARSE_BASE_TYPE);

  for (j = 0; supported_types[j] != 0; j++)
    {
    if (baseType == supported_types[j]) { break; }
    }
  if (supported_types[j] == 0)
    {
    args_ok = 0;
    }

  if (baseType == VTK_PARSE_UNKNOWN)
    {
    const char *qualified_name = 0;
    if ((returnType & VTK_PARSE_INDIRECT) == 0)
      {
      qualified_name = vtkParseHierarchy_QualifiedEnumName(
        hierarchyInfo, data, stringCache,
        currentFunction->ReturnClass);
      }
    if (qualified_name)
      {
      currentFunction->ReturnClass = qualified_name;
      }
    else
      {
      args_ok = 0;
      }
    }

  if (baseType == VTK_PARSE_STRING &&
      (returnType & VTK_PARSE_INDIRECT) != 0 &&
      (returnType & VTK_PARSE_INDIRECT) != VTK_PARSE_REF)
    {
    args_ok = 0;
    }

  if (baseType == VTK_PARSE_OBJECT)
    {
    if ((returnType & VTK_PARSE_INDIRECT) != VTK_PARSE_POINTER)
      {
      args_ok = 0;
      }
    else if (!isClassWrapped(currentFunction->ReturnClass))
      {
      args_ok = 0;
      }
    }

  if (((returnType & VTK_PARSE_INDIRECT) != VTK_PARSE_POINTER) &&
      ((returnType & VTK_PARSE_INDIRECT) != VTK_PARSE_REF) &&
      ((returnType & VTK_PARSE_INDIRECT) != 0))
    {
    args_ok = 0;
    }

  /* we can't handle void * return types */
  if (returnType == VTK_PARSE_VOID_PTR)
    {
    args_ok = 0;
    }

  /* watch out for functions that dont have enough info */
  switch (baseType)
    {
    case VTK_PARSE_FLOAT:
    case VTK_PARSE_DOUBLE:
    case VTK_PARSE_INT:
    case VTK_PARSE_SHORT:
    case VTK_PARSE_LONG:
    case VTK_PARSE_ID_TYPE:
    case VTK_PARSE_LONG_LONG:
    case VTK_PARSE___INT64:
    case VTK_PARSE_SIGNED_CHAR:
    case VTK_PARSE_BOOL:
    case VTK_PARSE_UNSIGNED_CHAR:
    case VTK_PARSE_UNSIGNED_INT:
    case VTK_PARSE_UNSIGNED_SHORT:
    case VTK_PARSE_UNSIGNED_LONG:
    case VTK_PARSE_UNSIGNED_ID_TYPE:
    case VTK_PARSE_UNSIGNED_LONG_LONG:
    case VTK_PARSE_UNSIGNED___INT64:
      if ((returnType & VTK_PARSE_INDIRECT) == VTK_PARSE_POINTER)
        {
        args_ok = currentFunction->HaveHint;
        }
      else if ((returnType & VTK_PARSE_INDIRECT) == VTK_PARSE_REF)
        {
        args_ok = 0;
        }
      break;
    }

  /* don't allow "char []", only allow "char *" */
  if (returnType == VTK_PARSE_CHAR_PTR && currentFunction->HaveHint)
    {
    args_ok = 0;
    }

  /* check for methods that will be overriden especially for Tcl */
  if (!strcmp("vtkObject",data->Name))
    {
    if (!strcmp(currentFunction->Name,"AddObserver"))
      {
      args_ok = 0;
      }
    }
  else if (!strcmp("vtkObjectBase",data->Name))
    {
    /* remove the special vtkObjectBase methods */
    if (!strcmp(currentFunction->Name,"Print")
#ifndef VTK_LEGACY_REMOVE
        || !strcmp(currentFunction->Name,"PrintRevisions")
#endif
        )
      {
      args_ok = 0;
      }
    }

  return args_ok;
}

void outputFunction(FILE *fp, ClassInfo *data)
{
  int i;
  int required_args = 0;

  /* if the args are OK and it is not a constructor or destructor */
  if (checkFunctionSignature(data) &&
      strcmp(data->Name,currentFunction->Name) &&
      strcmp(data->Name,currentFunction->Name + 1))
    {
    /* calc the total required args */
    for (i = 0; i < currentFunction->NumberOfArguments; i++)
      {
      required_args = required_args +
        (currentFunction->ArgCounts[i] ? currentFunction->ArgCounts[i] : 1);

      /* ignore args after function pointer */
      if (currentFunction->ArgTypes[i] == VTK_PARSE_FUNCTION)
        {
        break;
        }
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

      /* ignore args after function pointer */
      if (currentFunction->ArgTypes[i] == VTK_PARSE_FUNCTION)
        {
        break;
        }
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

    switch (currentFunction->ReturnType & VTK_PARSE_UNQUALIFIED_TYPE)
      {
      case VTK_PARSE_VOID:
        fprintf(fp,"    op->%s(",currentFunction->Name);
        break;
      default:
        if ((currentFunction->ReturnType & VTK_PARSE_INDIRECT)
            == VTK_PARSE_REF)
          {
          fprintf(fp,"    temp%i = &(op)->%s(",MAX_ARGS,currentFunction->Name);
          }
        else
          {
          fprintf(fp,"    temp%i = (op)->%s(",MAX_ARGS,currentFunction->Name);
          }
        break;
      }
    for (i = 0; i < currentFunction->NumberOfArguments; i++)
      {
      if (i)
        {
        fprintf(fp,",");
        }
      if (currentFunction->ArgTypes[i] == VTK_PARSE_FUNCTION)
        {
        fprintf(fp,"vtkTclVoidFunc,static_cast<void *>(temp%i)",i);
        break;
        }
      else
        {
        fprintf(fp,"temp%i",i);
        }
      }
    fprintf(fp,");\n");
    if (currentFunction->NumberOfArguments &&
        (currentFunction->ArgTypes[0] == VTK_PARSE_FUNCTION))
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
int main(int argc, char *argv[])
{
  OptionInfo *options;
  FileInfo *file_info;
  ClassInfo *data;
  FILE *fp;
  int i,j,k;

  /* get command-line args and parse the header file */
  file_info = vtkParse_Main(argc, argv);

  /* some utility functions require the string cache */
  stringCache = file_info->Strings;

  /* get the command-line options */
  options = vtkParse_GetCommandLineOptions();

  /* get the output file */
  fp = fopen(options->OutputFileName, "w");

  if (!fp)
    {
    fprintf(stderr, "Error opening output file %s\n", options->OutputFileName);
    exit(1);
    }

  /* get the main class */
  if ((data = file_info->MainClass) == NULL)
    {
    fclose(fp);
    exit(0);
    }

  /* get the hierarchy info for accurate typing */
  if (options->HierarchyFileName)
    {
    hierarchyInfo = vtkParseHierarchy_ReadFile(options->HierarchyFileName);
    }

  fprintf(fp,"// tcl wrapper for %s object\n//\n",data->Name);
  fprintf(fp,"#define VTK_WRAPPING_CXX\n");
  if (strcmp("vtkObjectBase",data->Name) != 0)
    {
      /* Block inclusion of full streams. */
    fprintf(fp,"#define VTK_STREAMS_FWD_ONLY\n");
    }
  fprintf(fp,"#include \"vtkSystemIncludes.h\"\n");
  fprintf(fp,"#include \"%s.h\"\n\n",data->Name);
  fprintf(fp,"#include \"vtkTclUtil.h\"\n");
  fprintf(fp,"#include \"vtkStdString.h\"\n");
  fprintf(fp,"#include <stdexcept>\n");
  fprintf(fp,"#include <vtksys/ios/sstream>\n");
  if (!data->IsAbstract && strcmp(data->Name, "vtkObjectBase") != 0)
    {
    if (strcmp(data->Name, "vtkRenderWindowInteractor") == 0)
      {
      fprintf(fp,"#include \"vtkToolkits.h\"\n");
      fprintf(fp,"#if defined( VTK_USE_X ) && defined( VTK_USE_TK )\n");
      fprintf(fp,"# include \"vtkXRenderWindowTclInteractor.h\"\n");
      fprintf(fp,"#endif\n");

      fprintf(fp,"\nClientData %sNewCommand()\n{\n",data->Name);

      fprintf(fp,"#if defined( VTK_USE_X ) && defined( VTK_USE_TK )\n");
      fprintf(fp,"  %s *temp = vtkXRenderWindowTclInteractor::New();\n",
              data->Name);
      fprintf(fp,"#else\n");
      fprintf(fp,"  %s *temp = %s::New();\n",data->Name,data->Name);
      fprintf(fp,"#endif\n");
      fprintf(fp,"  return static_cast<ClientData>(temp);\n}\n\n");
      }
    else
      {
      fprintf(fp,"\nClientData %sNewCommand()\n{\n",data->Name);
      fprintf(fp,"  %s *temp = %s::New();\n",data->Name,data->Name);
      fprintf(fp,"  return static_cast<ClientData>(temp);\n}\n\n");
      }
    }

  for (i = 0; i < data->NumberOfSuperClasses; i++)
    {
    char *safe_name = vtkWrap_SafeSuperclassName(data->SuperClasses[i]);
    const char *safe_superclass = safe_name ? safe_name : data->SuperClasses[i];

    /* if a template class is detected add a typedef */
    if (safe_name)
      {
      fprintf(fp,"typedef %s %s;\n",
              data->SuperClasses[i], safe_name);
      }

    fprintf(fp,"int %sCppCommand(%s *op, Tcl_Interp *interp,\n             int argc, char *argv[]);\n",
            safe_superclass, safe_superclass);

    free(safe_name);
    }
  fprintf(fp,"int VTKTCL_EXPORT %sCppCommand(%s *op, Tcl_Interp *interp,\n             int argc, char *argv[]);\n",data->Name,data->Name);
  fprintf(fp,"\nint %sCommand(ClientData cd, Tcl_Interp *interp,\n             int argc, char *argv[])\n{\n",data->Name);
  fprintf(fp,"  if ((argc == 2)&&(!strcmp(\"Delete\",argv[1]))&& !vtkTclInDelete(interp))\n    {\n");
  fprintf(fp,"    Tcl_DeleteCommand(interp,argv[0]);\n");
  fprintf(fp,"    return TCL_OK;\n    }\n");
  fprintf(fp,"   return %sCppCommand(static_cast<%s *>(static_cast<vtkTclCommandArgStruct *>(cd)->Pointer),interp, argc, argv);\n}\n",data->Name,data->Name);

  fprintf(fp,"\nint VTKTCL_EXPORT %s_TclCreate(Tcl_Interp *interp)\n{\n",data->Name);
  if (!data->IsAbstract && strcmp(data->Name, "vtkObjectBase") != 0)
    {
    fprintf(fp,"  vtkTclCreateNew(interp,const_cast<char *>(\"%s\"),%sNewCommand,%sCommand);\n",data->Name,data->Name,data->Name);
    }
  else
    {
    fprintf(fp,"  (void)interp;\n");
    }
  fprintf(fp,"  return 0;\n}\n");

  fprintf(fp,"\nint VTKTCL_EXPORT %sCppCommand(%s *op, Tcl_Interp *interp,\n             int argc, char *argv[])\n{\n",data->Name,data->Name);
  fprintf(fp,"  int    tempi = 0;      (void)tempi;\n");
  fprintf(fp,"  double tempd = 0.0;    (void)tempd;\n");
  fprintf(fp,"  static char temps[80]; (void)temps;\n");
  fprintf(fp,"  int    error = 0;      (void)error;\n");
  fprintf(fp,"  temps[0] = 0;\n");
  fprintf(fp,"\n");

  fprintf(fp,"  if (argc < 2)\n    {\n    Tcl_SetResult(interp,const_cast<char *>(\"Could not find requested method.\"), TCL_VOLATILE);\n    return TCL_ERROR;\n    }\n");

  /* stick in the typecasting and delete functionality here */
  fprintf(fp,"  if (!interp)\n    {\n");
  fprintf(fp,"    if (!strcmp(\"DoTypecasting\",argv[0]))\n      {\n");
  fprintf(fp,"      if (!strcmp(\"%s\",argv[1]))\n        {\n",
          data->Name);
  fprintf(fp,"        argv[2] = static_cast<char *>(static_cast<void *>(op));\n");
  fprintf(fp,"        return TCL_OK;\n        }\n");

  /* check our superclasses */
  for (i = 0; i < data->NumberOfSuperClasses; i++)
    {
    char *safe_name = vtkWrap_SafeSuperclassName(data->SuperClasses[i]);
    const char *safe_superclass = safe_name ? safe_name : data->SuperClasses[i];

    fprintf(fp,"      if (%sCppCommand(static_cast<%s *>(op),interp,argc,argv) == TCL_OK)\n        {\n",
            safe_superclass, data->SuperClasses[i]);
    fprintf(fp,"        return TCL_OK;\n        }\n");

    free(safe_name);
    }
  fprintf(fp,"      }\n    return TCL_ERROR;\n    }\n\n");

  /* add the GetSuperClassName */
  if (data->NumberOfSuperClasses)
    {
    fprintf(fp,"  if (!strcmp(\"GetSuperClassName\",argv[1]))\n");
    fprintf(fp,"    {\n");
    fprintf(fp,"    Tcl_SetResult(interp,const_cast<char *>(\"%s\"), TCL_VOLATILE);\n",data->SuperClasses[0]);
    fprintf(fp,"    return TCL_OK;\n");
    fprintf(fp,"    }\n\n");
    }

  fprintf(fp,"  try\n    {\n");

  /* insert function handling code here */
  for (i = 0; i < data->NumberOfFunctions; i++)
    {
    currentFunction = data->Functions[i];
    outputFunction(fp, data);
    }

  /* add the ListInstances method */
  fprintf(fp,"\n  if (!strcmp(\"ListInstances\",argv[1]))\n    {\n");
  fprintf(fp,"    vtkTclListInstances(interp,(ClientData)(%sCommand));\n",data->Name);
  fprintf(fp,"    return TCL_OK;\n    }\n");

  /* add the ListMethods method */
  fprintf(fp,"\n  if (!strcmp(\"ListMethods\",argv[1]))\n    {\n");
  /* recurse up the tree */
  for (i = 0; i < data->NumberOfSuperClasses; i++)
    {
    char *safe_name = vtkWrap_SafeSuperclassName(data->SuperClasses[i]);
    const char *safe_superclass = safe_name ? safe_name : data->SuperClasses[i];

    fprintf(fp,"    %sCppCommand(op,interp,argc,argv);\n",
            safe_superclass);

    free(safe_name);
    }
  /* now list our methods */
  fprintf(fp,"    Tcl_AppendResult(interp,\"Methods from %s:\\n\",NULL);\n",data->Name);
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
      if (currentFunction->ArgTypes[j] == VTK_PARSE_FUNCTION)
        {
        break;
        }
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


  /* add the DescribeMethods method */
  fprintf(fp,"\n  if (!strcmp(\"DescribeMethods\",argv[1]))\n    {\n");
  fprintf(fp,"    if(argc>3) {\n" );
  fprintf(fp,"      Tcl_SetResult ( interp, const_cast<char*>(\"Wrong number of arguments: object DescribeMethods <MethodName>\"), TCL_VOLATILE ); \n" );
  fprintf(fp,"      return TCL_ERROR;\n }\n" );

  fprintf(fp,"    if(argc==2) {\n" );
  /* Return a list of methods */
  fprintf(fp,"\n  Tcl_DString dString, dStringParent;\n");
  fprintf(fp,"\n  Tcl_DStringInit ( &dString );\n" );
  fprintf(fp,"\n  Tcl_DStringInit ( &dStringParent );\n" );
  /* recurse up the tree */
  for (i = 0; i < data->NumberOfSuperClasses; i++)
    {
    char *safe_name = vtkWrap_SafeSuperclassName(data->SuperClasses[i]);
    const char *safe_superclass = safe_name ? safe_name : data->SuperClasses[i];

    fprintf(fp,"    %sCppCommand(op,interp,argc,argv);\n",
            safe_superclass);
    /* append the result to our string */
    fprintf(fp,"    Tcl_DStringGetResult ( interp, &dStringParent );\n" );
    fprintf(fp,"    Tcl_DStringAppend ( &dString, Tcl_DStringValue ( &dStringParent ), -1 );\n" );

    free(safe_name);
    }
  for (k = 0; k < numberOfWrappedFunctions; k++)
    {
      currentFunction = wrappedFunctions[k];
      if(currentFunction->IsLegacy)
        {
          fprintf(fp,"#if !defined(VTK_LEGACY_REMOVE)\n");
        }
      fprintf(fp,"    Tcl_DStringAppendElement ( &dString, \"%s\" );\n", currentFunction->Name );
     if(currentFunction->IsLegacy)
        {
          fprintf(fp,"#endif\n");
        }
    }
  fprintf(fp,"  Tcl_DStringResult ( interp, &dString );\n" );
  fprintf(fp,"  Tcl_DStringFree ( &dString );\n" );
  fprintf(fp,"  Tcl_DStringFree ( &dStringParent );\n" );
  fprintf(fp,"    return TCL_OK;\n    }\n");

  /* Now handle if we are asked for a specific function */
  fprintf(fp,"    if(argc==3) {\n" );
  if (numberOfWrappedFunctions > 0)
    {
    fprintf(fp,"      Tcl_DString dString;\n");
    }
  if (data->NumberOfSuperClasses > 0)
  {
    fprintf(fp,"      int SuperClassStatus;\n" );
  }
  /* recurse up the tree */
  for (i = 0; i < data->NumberOfSuperClasses; i++)
    {
    char *safe_name = vtkWrap_SafeSuperclassName(data->SuperClasses[i]);
    const char *safe_superclass = safe_name ? safe_name : data->SuperClasses[i];

    fprintf(fp,"    SuperClassStatus = %sCppCommand(op,interp,argc,argv);\n",
            safe_superclass);
    fprintf(fp,"    if ( SuperClassStatus == TCL_OK ) { return TCL_OK; }\n" );

    free(safe_name);
    }
  /* Now we handle it ourselves */
  for (k = 0; k < numberOfWrappedFunctions; k++)
    {
      currentFunction = wrappedFunctions[k];
      if(currentFunction->IsLegacy)
        {
          fprintf(fp,"#if !defined(VTK_LEGACY_REMOVE)\n");
        }
      fprintf(fp,"    /* Starting function: %s */\n", currentFunction->Name );
      fprintf(fp,"    if ( strcmp ( argv[2], \"%s\" ) == 0 ) {\n", currentFunction->Name );
      fprintf(fp,"    Tcl_DStringInit ( &dString );\n" );

      fprintf(fp,"    Tcl_DStringAppendElement ( &dString, \"%s\" );\n", currentFunction->Name );

      /* calc the total required args */
      fprintf(fp,"    /* Arguments */\n" );
      fprintf(fp,"    Tcl_DStringStartSublist ( &dString );\n" );
      for (i = 0; i < currentFunction->NumberOfArguments; i++)
        {
          unsigned int argtype;

          if (currentFunction->ArgTypes[i] == VTK_PARSE_FUNCTION)
            {
              fprintf(fp,"    Tcl_DStringAppendElement ( &dString, \"function\" );\n" );
              break;
            }

          argtype =
            (currentFunction->ArgTypes[i] & VTK_PARSE_UNQUALIFIED_TYPE);

          switch (argtype)
            {
            case VTK_PARSE_FLOAT_PTR:
            case VTK_PARSE_DOUBLE_PTR:
              /* Vector */
              fprintf(fp,"    Tcl_DStringStartSublist ( &dString );\n" );
              for (j = 0; j < currentFunction->ArgCounts[i]; j++)
                {
                  fprintf(fp,"    Tcl_DStringAppendElement ( &dString, \"float\" );\n" );
                }
              fprintf(fp,"    Tcl_DStringEndSublist ( &dString );\n" );
              break;
            case VTK_PARSE_INT_PTR:
              /* Vector */
              fprintf(fp,"    Tcl_DStringStartSublist ( &dString );\n" );
              for (j = 0; j < currentFunction->ArgCounts[i]; j++)
                {
                  fprintf(fp,"    Tcl_DStringAppendElement ( &dString, \"int\" );\n" );
                }
              fprintf(fp,"    Tcl_DStringEndSublist ( &dString );\n" );
              break;
            case VTK_PARSE_ID_TYPE_PTR:
              /* Vector */
              fprintf(fp,"    Tcl_DStringStartSublist ( &dString );\n" );
              for (j = 0; j < currentFunction->ArgCounts[i]; j++)
                {
                  fprintf(fp,"    Tcl_DStringAppendElement ( &dString, \"int\" );\n" );
                }
              fprintf(fp,"    Tcl_DStringEndSublist ( &dString );\n" );
              break;
            case VTK_PARSE_LONG_LONG_PTR:
            case VTK_PARSE___INT64_PTR:
              /* Vector */
              fprintf(fp,"    Tcl_DStringStartSublist ( &dString );\n" );
              for (j = 0; j < currentFunction->ArgCounts[i]; j++)
                {
                  fprintf(fp,"    Tcl_DStringAppendElement ( &dString, \"int\" );\n" );
                }
              fprintf(fp,"    Tcl_DStringEndSublist ( &dString );\n" );
              break;
            case VTK_PARSE_OBJECT_PTR:
              fprintf(fp,"    Tcl_DStringAppendElement ( &dString, \"%s\" );\n", currentFunction->ArgClasses[i] );
              break;
            case VTK_PARSE_VOID_PTR:
            case VTK_PARSE_CHAR_PTR:
            case VTK_PARSE_STRING:
            case VTK_PARSE_STRING_REF:
              fprintf(fp,"    Tcl_DStringAppendElement ( &dString, \"string\" );\n" );
              break;
            case VTK_PARSE_FLOAT:
            case VTK_PARSE_DOUBLE:
              fprintf(fp,"    Tcl_DStringAppendElement ( &dString, \"float\" );\n" );
              break;
            case VTK_PARSE_SIGNED_CHAR:
            case VTK_PARSE_ID_TYPE:
            case VTK_PARSE_UNSIGNED_LONG_LONG:
            case VTK_PARSE_LONG_LONG:
            case VTK_PARSE_UNSIGNED___INT64:
            case VTK_PARSE___INT64:
            case VTK_PARSE_UNSIGNED_INT:
            case VTK_PARSE_INT:
            case VTK_PARSE_UNSIGNED_SHORT:
            case VTK_PARSE_SHORT:
            case VTK_PARSE_UNSIGNED_LONG:
            case VTK_PARSE_LONG:
              fprintf(fp,"    Tcl_DStringAppendElement ( &dString, \"int\" );\n" );
              break;
            case VTK_PARSE_CHAR:
              fprintf(fp,"    Tcl_DStringAppendElement ( &dString, \"char\" );\n" );
              break;
            case VTK_PARSE_UNSIGNED_CHAR:
              fprintf(fp,"    Tcl_DStringAppendElement ( &dString, \"int\" );\n" );
              break;
            case VTK_PARSE_BOOL:
              fprintf(fp,"    Tcl_DStringAppendElement ( &dString, \"bool\" );\n" );
              break;
            }
        }
      fprintf(fp,"    Tcl_DStringEndSublist ( &dString );\n" );

     /* Documentation */
     fprintf(fp,"    /* Documentation for %s */\n", currentFunction->Name );
     fprintf(fp,"    Tcl_DStringAppendElement ( &dString, \"%s\" );\n", quote_string ( currentFunction->Comment, 500 ) );
     fprintf(fp,"    Tcl_DStringAppendElement ( &dString, \"%s\" );\n", quote_string ( currentFunction->Signature, 500 ) );
     fprintf(fp,"    Tcl_DStringAppendElement ( &dString, \"%s\" );\n", quote_string ( data->Name, 500 ) );
     fprintf(fp,"    /* Closing for %s */\n\n", currentFunction->Name );
     fprintf(fp,"    Tcl_DStringResult ( interp, &dString );\n" );
     fprintf(fp,"    Tcl_DStringFree ( &dString );\n" );
     fprintf(fp,"    return TCL_OK;\n    }\n");

     if(currentFunction->IsLegacy)
        {
        fprintf(fp,"#endif\n");
        }
    }
  /* Didn't find anything, return an error */
  fprintf(fp,"   Tcl_SetResult ( interp, const_cast<char*>(\"Could not find method\"), TCL_VOLATILE ); \n" );
  fprintf(fp,"   return TCL_ERROR;\n" );
  fprintf(fp,"   }\n" );
  fprintf(fp," }\n" );


  /* try superclasses */
  for (i = 0; i < data->NumberOfSuperClasses; i++)
    {
    char *safe_name = vtkWrap_SafeSuperclassName(data->SuperClasses[i]);
    const char *safe_superclass = safe_name ? safe_name : data->SuperClasses[i];

    fprintf(fp,"\n  if (%sCppCommand(static_cast<%s *>(op),interp,argc,argv) == TCL_OK)\n",
            safe_superclass, data->SuperClasses[i]);
    fprintf(fp,"    {\n    return TCL_OK;\n    }\n");

    free(safe_name);
    }


  /* Add the Print method to vtkObjectBase. */
  if (!strcmp("vtkObjectBase",data->Name))
    {
    fprintf(fp,"  if ((!strcmp(\"Print\",argv[1]))&&(argc == 2))\n    {\n");
    fprintf(fp,"    vtksys_ios::ostringstream buf_with_warning_C4701;\n");
    fprintf(fp,"    op->Print(buf_with_warning_C4701);\n");
    fprintf(fp,"    buf_with_warning_C4701.put('\\0');\n");
    fprintf(fp,"    Tcl_SetResult(interp,const_cast<char *>(buf_with_warning_C4701.str().c_str()),\n");
    fprintf(fp,"      TCL_VOLATILE);\n");
    fprintf(fp,"    return TCL_OK;\n    }\n");
#ifndef VTK_LEGACY_REMOVE
    /* Add the PrintRevisions method to vtkObjectBase. */
    fprintf(fp,"  if ((!strcmp(\"PrintRevisions\",argv[1]))&&(argc == 2))\n    {\n");
    fprintf(fp,"    vtksys_ios::ostringstream buf_with_warning_C4701;\n");
    fprintf(fp,"    op->PrintRevisions(buf_with_warning_C4701);\n");
    fprintf(fp,"    buf_with_warning_C4701.put('\\0');\n");
    fprintf(fp,"    Tcl_SetResult(interp,const_cast<char *>(buf_with_warning_C4701.str().c_str()),\n");
    fprintf(fp,"      TCL_VOLATILE);\n");
    fprintf(fp,"    return TCL_OK;\n    }\n");
#endif
    }

  /* Add the AddObserver method to vtkObject. */
  if (!strcmp("vtkObject",data->Name))
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

  /* i.e. If this is vtkObjectBase (or whatever the top of the class hierarchy will be) */
  /* then report the error */
  if (data->NumberOfSuperClasses == 0)
    {
    fprintf(fp,"\n  if (argc >= 2)\n    {\n");
    fprintf(fp,"    char temps2[256];\n    sprintf(temps2,\"Object named: %%s, could not find requested method: %%s\\nor the method was called with incorrect arguments.\\n\",argv[0],argv[1]);\n    Tcl_SetResult(interp,temps2,TCL_VOLATILE);\n    return TCL_ERROR;\n    }\n");
    }

  fprintf(fp,"    }\n");
  fprintf(fp,"  catch (std::exception &e)\n");
  fprintf(fp,"    {\n");
  fprintf(fp,"    Tcl_AppendResult(interp, \"Uncaught exception: \",  e.what(), \"\\n\", NULL);\n");
  fprintf(fp,"    return TCL_ERROR;\n");
  fprintf(fp,"    }\n");
  fprintf(fp,"  return TCL_ERROR;\n}\n");

  vtkParse_Free(file_info);

  fclose(fp);

  return 0;
}
