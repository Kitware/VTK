/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWrapPython.c

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPython.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "vtkParse.h"
#include "vtkParseType.h"
#include "vtkConfigure.h"

int numberOfWrappedFunctions = 0;
FunctionInfo *wrappedFunctions[1000];
extern FunctionInfo *currentFunction;

static int class_has_new = 0;

/* when the cpp file doesn't have enough info use the hint file */
void use_hints(FILE *fp)
{
  int  i;

  fprintf(fp,
          "    if(temp%i)\n"
          "      {\n",
          MAX_ARGS);

  switch (currentFunction->ReturnType & VTK_PARSE_UNQUALIFIED_TYPE)
    {
    case VTK_PARSE_FLOAT_PTR:
      {
      fprintf(fp,
              "    return Py_BuildValue((char*)\"");

      for (i = 0; i < currentFunction->HintSize; i++)
        {
        fprintf(fp, "f");
        }
      fprintf(fp, "\"");

      for (i = 0; i < currentFunction->HintSize; i++)
        {
        fprintf(fp, ",temp%i[%d]",MAX_ARGS,i);
        }
      fprintf(fp, ");\n");

      break;
      }
    case VTK_PARSE_DOUBLE_PTR:
      {
      fprintf(fp,
              "    return Py_BuildValue((char*)\"");

      for (i = 0; i < currentFunction->HintSize; i++)
        {
        fprintf(fp, "d");
        }
      fprintf(fp, "\"");

      for (i = 0; i < currentFunction->HintSize; i++)
        {
        fprintf(fp,",temp%i[%d]",MAX_ARGS,i);
        }
      fprintf(fp,");\n");

      break;
      }
    case VTK_PARSE_INT_PTR:
    case VTK_PARSE_SIGNED_CHAR_PTR:
      {
      fprintf(fp,
              "    return Py_BuildValue((char*)\"");

      for (i = 0; i < currentFunction->HintSize; i++)
        {
        fprintf(fp, "i");
        }
      fprintf(fp,"\"");

      for (i = 0; i < currentFunction->HintSize; i++) 
        {
        fprintf(fp,",temp%i[%d]",MAX_ARGS,i);
        }
      fprintf(fp,");\n");

      break;
      }
    case VTK_PARSE_BOOL_PTR:
      {
      fprintf(fp,
              "    return Py_BuildValue((char*)\"");

      for (i = 0; i < currentFunction->HintSize; i++)
        {
        fprintf(fp, "b");
        }
      fprintf(fp,"\"");

      for (i = 0; i < currentFunction->HintSize; i++)
        {
        fprintf(fp,",(int)temp%i[%d]",MAX_ARGS,i);
        }
      fprintf(fp,");\n");

      break;
      }
    case VTK_PARSE_ID_TYPE_PTR:
      {
      fprintf(fp,
              "    return Py_BuildValue((char*)\"");

      for (i = 0; i < currentFunction->HintSize; i++)
        {
#ifdef VTK_USE_64BIT_IDS
#ifdef PY_LONG_LONG
        fprintf(fp, "L");
#else
        fprintf(fp, "l");
#endif
#else
        fprintf(fp, "i");
#endif
        }
      fprintf(fp,"\"");

      for (i = 0; i < currentFunction->HintSize; i++) 
        {
        fprintf(fp,",temp%i[%d]",MAX_ARGS,i);
        }
      fprintf(fp,");\n");

      break;
      }
    case VTK_PARSE_LONG_LONG_PTR:
    case VTK_PARSE___INT64_PTR:
      {
      fprintf(fp,
              "    return Py_BuildValue((char*)\"");

      for (i = 0; i < currentFunction->HintSize; i++)
        {
#ifdef PY_LONG_LONG
        fprintf(fp,"L");
#else
        fprintf(fp,"l");
#endif
        }

      fprintf(fp,"\"");
      for (i = 0; i < currentFunction->HintSize; i++) 
        {
        fprintf(fp,",temp%i[%d]",MAX_ARGS,i);
        }
      fprintf(fp,");\n");

      break;
      }
    case VTK_PARSE_SHORT_PTR:
    case VTK_PARSE_LONG_PTR:
    case VTK_PARSE_UNSIGNED_CHAR_PTR:
    case VTK_PARSE_UNSIGNED_INT_PTR:
    case VTK_PARSE_UNSIGNED___INT64_PTR:
    case VTK_PARSE_UNSIGNED_SHORT_PTR:
    case VTK_PARSE_UNSIGNED_LONG_PTR:
      break;
    }

  fprintf(fp,
          "      }\n"
          "    else\n"
          "      {\n"
          "      return Py_BuildValue((char*)\"\");\n"
          "      }\n");

  return;
}


void output_temp(FILE *fp, int i, int aType, char *Id, int aCount)
{
  /* handle VAR FUNCTIONS */
  if (aType == VTK_PARSE_FUNCTION)
    {
    fprintf(fp,
            "  PyObject *temp%i;\n",
            i);
    return;
    }
  
  if (((aType & VTK_PARSE_BASE_TYPE) == VTK_PARSE_VOID) &&
      ((aType & VTK_PARSE_INDIRECT) == 0))
    {
    return;
    }

  /* for const * return types prototype with const */
  if ((i == MAX_ARGS) && ((aType & VTK_PARSE_CONST) != 0))
    {
    fprintf(fp,"  const ");
    }
  else
    {
    fprintf(fp,"  ");
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
    case VTK_PARSE_VTK_OBJECT:  fprintf(fp,"%s ",Id); break;
    case VTK_PARSE_ID_TYPE:     fprintf(fp,"vtkIdType "); break;
    case VTK_PARSE_LONG_LONG:   fprintf(fp,"long long "); break;
    case VTK_PARSE___INT64:     fprintf(fp,"__int64 "); break;
    case VTK_PARSE_SIGNED_CHAR: fprintf(fp,"signed char "); break;
    case VTK_PARSE_BOOL:        fprintf(fp,"bool "); break;
    case VTK_PARSE_UNKNOWN:     return;
    }
  
  switch (aType & VTK_PARSE_INDIRECT)
    {
    case VTK_PARSE_REF: fprintf(fp, " *"); break; /* act " &" */
    case VTK_PARSE_POINTER:
      if ((i == MAX_ARGS) ||
          ((aType & VTK_PARSE_BASE_TYPE) == VTK_PARSE_VTK_OBJECT) ||
          ((aType & VTK_PARSE_UNQUALIFIED_TYPE) == VTK_PARSE_CHAR_PTR) ||
          ((aType & VTK_PARSE_UNQUALIFIED_TYPE) == VTK_PARSE_VOID_PTR))
        {
        fprintf(fp, " *");
        }
      break;
    case VTK_PARSE_POINTER_REF: fprintf(fp, "*&"); break;
    case VTK_PARSE_POINTER_POINTER: fprintf(fp, "**"); break;
    default: fprintf(fp,"  "); break;
    }
  fprintf(fp,"temp%i",i);
  
  /* handle arrays */
  if (((aType & VTK_PARSE_INDIRECT) == VTK_PARSE_POINTER) &&
      (i != MAX_ARGS) &&
      ((aType & VTK_PARSE_BASE_TYPE) != VTK_PARSE_VTK_OBJECT) &&
      ((aType & VTK_PARSE_UNQUALIFIED_TYPE) != VTK_PARSE_CHAR_PTR) &&
      ((aType & VTK_PARSE_UNQUALIFIED_TYPE) != VTK_PARSE_VOID_PTR))
    {
    fprintf(fp,"[%i]",aCount);
    }

  fprintf(fp,";\n");
  if (((aType & VTK_PARSE_UNQUALIFIED_TYPE) == VTK_PARSE_VOID_PTR) &&
      (i != MAX_ARGS))
    {
    fprintf(fp,
            "  int      size%d;\n",
            i);
    }
  if ((i != MAX_ARGS) &&
      (((aType & VTK_PARSE_UNQUALIFIED_TYPE) == VTK_PARSE_VTK_OBJECT_PTR) ||
       ((aType & VTK_PARSE_UNQUALIFIED_TYPE) == VTK_PARSE_VTK_OBJECT_REF)))
    {
    fprintf(fp,
            "  PyObject *tempH%d;\n",
            i);
    }
}

void do_return(FILE *fp)
{
  /* ignore void */
  if (((currentFunction->ReturnType & VTK_PARSE_BASE_TYPE) == VTK_PARSE_VOID)
      && ((currentFunction->ReturnType & VTK_PARSE_INDIRECT) == 0))
    {
    fprintf(fp,
            "    Py_INCREF(Py_None);\n"
            "    return Py_None;\n");
    return;
    }
  
  switch (currentFunction->ReturnType & VTK_PARSE_UNQUALIFIED_TYPE)
    {
    case VTK_PARSE_CHAR_PTR:
      fprintf(fp,
              "    if (temp%i == NULL) {\n"
              "      Py_INCREF(Py_None);\n"
              "      return Py_None;\n      }\n"
              "    else {\n"
              "      return PyString_FromString(temp%i);\n      }\n",
              MAX_ARGS, MAX_ARGS);
    break;
    case VTK_PARSE_VTK_OBJECT_REF:
    case VTK_PARSE_VTK_OBJECT_PTR:
      {
      fprintf(fp,
              "    return vtkPythonGetObjectFromPointer((vtkObjectBase *)temp%i);\n",
              MAX_ARGS);
      break;
      }
      
    /* handle functions returning vectors */
    /* this is done by looking them up in a hint file */
    case VTK_PARSE_FLOAT_PTR:
    case VTK_PARSE_DOUBLE_PTR:
    case VTK_PARSE_ID_TYPE_PTR:
    case VTK_PARSE_LONG_LONG_PTR:
    case VTK_PARSE___INT64_PTR:
    case VTK_PARSE_SIGNED_CHAR_PTR:
    case VTK_PARSE_INT_PTR:
    case VTK_PARSE_SHORT_PTR:
    case VTK_PARSE_LONG_PTR:
    case VTK_PARSE_BOOL_PTR:
      use_hints(fp);
      break;
    case VTK_PARSE_VOID_PTR:
      {
      fprintf(fp,
              "    if (temp%i == NULL)\n"
              "      {\n"
              "      Py_INCREF(Py_None);\n"
              "      return Py_None;\n"
              "      }\n"
              "    else\n"
              "      {\n"
              "      return PyString_FromString(vtkPythonManglePointer(temp%i,\"void_p\"));\n"
              "      }\n",
              MAX_ARGS, MAX_ARGS);
      break;
      }
    case VTK_PARSE_FLOAT:
    case VTK_PARSE_DOUBLE:
      {
      fprintf(fp,
              "    return PyFloat_FromDouble(temp%i);\n",
              MAX_ARGS);
      break;
      }
    case VTK_PARSE_UNSIGNED_CHAR:
    case VTK_PARSE_UNSIGNED_INT:
    case VTK_PARSE_UNSIGNED_SHORT:
    case VTK_PARSE_INT:
    case VTK_PARSE_SHORT:
    case VTK_PARSE_LONG:
    case VTK_PARSE_SIGNED_CHAR:
      {
      fprintf(fp,
              "    return PyInt_FromLong(temp%i);\n",
              MAX_ARGS);
      break;
      }
    case VTK_PARSE_BOOL:
      {
      /* PyBool_FromLong was introduced in Python 2.3.
         Use PyInt_FromLong as a bool substitute in
         earlier versions of python...
      */
      fprintf(fp,
              "#if PY_VERSION_HEX >= 0x02030000\n"
              "    return PyBool_FromLong(temp%i);\n"
              "#else\n"
              "    return PyInt_FromLong((long)temp%i);\n"
              "#endif\n",
              MAX_ARGS, MAX_ARGS);
      break;
      }
    case VTK_PARSE_UNSIGNED_LONG:
      {
#if (PY_VERSION_HEX >= 0x02020000)
      fprintf(fp,
              "    return PyLong_FromUnsignedLong(temp%i);\n",
              MAX_ARGS);
#else
      fprintf(fp,
              "    return PyInt_FromLong((long)temp%i);\n",
              MAX_ARGS);
#endif
      break;
      }
#if defined(VTK_USE_64BIT_IDS) && defined(PY_LONG_LONG) && (VTK_SIZEOF_LONG != VTK_SIZEOF_ID_TYPE)
    case VTK_PARSE_ID_TYPE:
      {
      fprintf(fp,
              "    return PyLong_FromLongLong(temp%i);\n",
              MAX_ARGS);
      break;
      }
    case VTK_PARSE_UNSIGNED_ID_TYPE:
      {
      fprintf(fp,
              "    return PyLong_FromUnsignedLongLong(temp%i);\n",
              MAX_ARGS);
      break;
      }
#else
    case VTK_PARSE_ID_TYPE:
      {
      fprintf(fp,
              "    return PyInt_FromLong((long)temp%i);\n",
              MAX_ARGS);
      break;
      }
    case VTK_PARSE_UNSIGNED_ID_TYPE:
      {
#if (PY_VERSION_HEX >= 0x02020000)
      fprintf(fp,
              "    return PyLong_FromUnsignedLong((unsigned long)temp%i);\n",
              MAX_ARGS);
#else
      fprintf(fp,
              "    return PyInt_FromLong((long)temp%i);\n",
              MAX_ARGS);
#endif
      break;
      }
#endif
#if defined(VTK_SIZEOF_LONG_LONG)
# if defined(PY_LONG_LONG) && (VTK_SIZEOF_LONG != VTK_SIZEOF_LONG_LONG)
    case VTK_PARSE_LONG_LONG:
      {
      fprintf(fp,
              "    return PyLong_FromLongLong(temp%i);\n",
              MAX_ARGS);
      break;
      }
    case VTK_PARSE_UNSIGNED_LONG_LONG:
      {
      fprintf(fp,
              "    return PyLong_FromUnsignedLongLong(temp%i);\n",
              MAX_ARGS);
      break;
      }
# else
    case VTK_PARSE_LONG_LONG:
      {
      fprintf(fp,
              "    return PyLong_FromLong(temp%i);\n",
              MAX_ARGS);
      break;
      }
    case VTK_PARSE_UNSIGNED_LONG_LONG:
      {
      fprintf(fp,
              "    return PyLong_FromUnsignedLong(temp%i);\n",
              MAX_ARGS);
      break;
      }
# endif
#endif
#if defined(VTK_SIZEOF___INT64)
# if defined(PY_LONG_LONG) && (VTK_SIZEOF_LONG != VTK_SIZEOF___INT64)
    case VTK_PARSE___INT64:
      {
      fprintf(fp,
              "    return PyLong_FromLongLong(temp%i);\n",
              MAX_ARGS);
      break;
      }
    case VTK_PARSE_UNSIGNED___INT64:
      {
      fprintf(fp,
              "    return PyLong_FromUnsignedLongLong(temp%i);\n",
              MAX_ARGS);
      break;
      }
# else
    case VTK_PARSE___INT64:
      {
      fprintf(fp,
              "    return PyLong_FromLong(temp%i);\n",
              MAX_ARGS);
      break;
      }
    case VTK_PARSE_UNSIGNED___INT64:
      {
      fprintf(fp,
              "    return PyLong_FromUnsignedLong(temp%i);\n",
              MAX_ARGS);
      break;
      }
# endif
#endif
    case VTK_PARSE_CHAR:
      {
      fprintf(fp,
              "    char temp%iString[2];\n"
              "    temp%iString[0] = temp%i;\n"
              "    temp%iString[1] = \'\\0\';\n"
              "    return PyString_FromStringAndSize(temp%iString,1);\n",
              MAX_ARGS, MAX_ARGS, MAX_ARGS, MAX_ARGS, MAX_ARGS);
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
  
  if (currentFunction->ArgTypes[0] == VTK_PARSE_FUNCTION)
    {
    result[currPos++] = 'O';
    result[currPos] = '\0';
    return result;
    }
  
  for (i = 0; i < currentFunction->NumberOfArguments; i++)
    {
    argtype = (currentFunction->ArgTypes[i] & VTK_PARSE_UNQUALIFIED_TYPE);

    switch (argtype)
      {
      case VTK_PARSE_FLOAT_PTR:
        result[currPos++] = '(';
        for (j = 0; j < currentFunction->ArgCounts[i]; j++) 
          {
          result[currPos++] = 'f';
          }
        result[currPos++] = ')';
        break;
      case VTK_PARSE_DOUBLE_PTR:
        result[currPos++] = '(';
        for (j = 0; j < currentFunction->ArgCounts[i]; j++) 
          {
          result[currPos++] = 'd';
          }
        result[currPos++] = ')';
        break;
      case VTK_PARSE_INT_PTR:
        result[currPos++] = '(';
        for (j = 0; j < currentFunction->ArgCounts[i]; j++) 
          {
          result[currPos++] = 'i';
          }
        result[currPos++] = ')';
        break;
      case VTK_PARSE_ID_TYPE_PTR:
        result[currPos++] = '(';
        for (j = 0; j < currentFunction->ArgCounts[i]; j++) 
          {
#ifdef VTK_USE_64BIT_IDS
#ifdef PY_LONG_LONG
          result[currPos++] = 'L';
#else
          result[currPos++] = 'l';
#endif
#else
          result[currPos++] = 'i';
#endif
          }
        result[currPos++] = ')';
        break;
      case VTK_PARSE_LONG_LONG_PTR:
      case VTK_PARSE___INT64_PTR:
        result[currPos++] = '(';
        for (j = 0; j < currentFunction->ArgCounts[i]; j++) 
          {
#ifdef PY_LONG_LONG
          result[currPos++] = 'L';
#else
          result[currPos++] = 'l';
#endif
          }
        result[currPos++] = ')';
        break;
      case VTK_PARSE_VTK_OBJECT_REF:
      case VTK_PARSE_VTK_OBJECT_PTR:
        result[currPos++] = 'O';
        break;
      case VTK_PARSE_CHAR_PTR:
        result[currPos++] = 'z';
        break;
      case VTK_PARSE_VOID_PTR:
        result[currPos++] = 's';
        result[currPos++] = '#';
        break;
      case VTK_PARSE_FLOAT:
        result[currPos++] = 'f';
        break;
      case VTK_PARSE_DOUBLE:
        result[currPos++] = 'd';
        break;
      case VTK_PARSE_UNSIGNED_INT:
      case VTK_PARSE_INT:
        result[currPos++] = 'i';
        break;
      case VTK_PARSE_UNSIGNED_SHORT:
      case VTK_PARSE_SHORT:
        result[currPos++] = 'h';
        break;
      case VTK_PARSE_UNSIGNED_LONG:
      case VTK_PARSE_LONG:
        result[currPos++] = 'l';
        break;
      case VTK_PARSE_UNSIGNED_ID_TYPE:
      case VTK_PARSE_ID_TYPE:
#ifdef VTK_USE_64BIT_IDS
#ifdef PY_LONG_LONG
        result[currPos++] = 'L';
#else
        result[currPos++] = 'l';
#endif
#else
        result[currPos++] = 'i';
#endif
        break;
#ifdef PY_LONG_LONG
      case VTK_PARSE_UNSIGNED_LONG_LONG:
      case VTK_PARSE_UNSIGNED___INT64:
      case VTK_PARSE_LONG_LONG:
      case VTK_PARSE___INT64:
        result[currPos++] = 'L';
        break;
#else
      case VTK_PARSE_UNSIGNED_LONG_LONG:
      case VTK_PARSE_UNSIGNED___INT64:
      case VTK_PARSE_LONG_LONG:
      case VTK_PARSE___INT64:
        result[currPos++] = 'l';
        break;
#endif
      case VTK_PARSE_SIGNED_CHAR:
        result[currPos++] = 'i';
        break;
      case VTK_PARSE_CHAR:
        result[currPos++] = 'c';
        break;
      case VTK_PARSE_UNSIGNED_CHAR:
        result[currPos++] = 'b';
        break;
      case VTK_PARSE_BOOL:
        result[currPos++] = 'b';
        break;
      }
    }

  result[currPos] = '\0';
  return result;
}

static void add_to_sig(char *sig, const char *add, int *i)
{
  strcpy(&sig[*i],add);
  *i += (int)strlen(add);
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
    if (currentFunction->ArgTypes[i] == VTK_PARSE_FUNCTION)
      {
      add_to_sig(result,"function",&currPos); 
      }
    
    argtype = (currentFunction->ArgTypes[i] & VTK_PARSE_UNQUALIFIED_TYPE);

    if (i != 0)
      {
      add_to_sig(result,", ",&currPos);
      }

    switch (argtype)
      {
      case VTK_PARSE_FLOAT_PTR:
      case VTK_PARSE_DOUBLE_PTR:
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
      case VTK_PARSE_INT_PTR:
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
      case VTK_PARSE_ID_TYPE_PTR:
        add_to_sig(result,"(",&currPos);
        for (j = 0; j < currentFunction->ArgCounts[i]; j++) 
          {
          if (j != 0)
            {
            add_to_sig(result,", ",&currPos);
            }
#if defined(VTK_USE_64BIT_IDS) && (VTK_SIZEOF_LONG != VTK_SIZEOF_ID_TYPE)
          add_to_sig(result,"long",&currPos);
#else
          add_to_sig(result,"int",&currPos);
#endif
          }
        add_to_sig(result,")",&currPos);
        break;
      case VTK_PARSE_LONG_LONG_PTR:
      case VTK_PARSE___INT64_PTR:
        add_to_sig(result,"(",&currPos);
        for (j = 0; j < currentFunction->ArgCounts[i]; j++) 
          {
          if (j != 0)
            {
            add_to_sig(result,", ",&currPos);
            }
          add_to_sig(result,"long",&currPos);
          }
        add_to_sig(result,")",&currPos);
        break;
      case VTK_PARSE_VTK_OBJECT_REF:
      case VTK_PARSE_VTK_OBJECT_PTR:
        add_to_sig(result,currentFunction->ArgClasses[i],&currPos);
        break;
      case VTK_PARSE_VOID_PTR:
      case VTK_PARSE_CHAR_PTR:
        add_to_sig(result,"string",&currPos);
        break;
      case VTK_PARSE_FLOAT:
      case VTK_PARSE_DOUBLE:
        add_to_sig(result,"float",&currPos);
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
        add_to_sig(result,"int",&currPos);
        break;
      case VTK_PARSE_CHAR:
        add_to_sig(result,"char",&currPos);
        break;
      case VTK_PARSE_UNSIGNED_CHAR:
        add_to_sig(result,"int",&currPos);
        break;
      case VTK_PARSE_BOOL:
        add_to_sig(result,"bool",&currPos);
        break;
      }
    }

  add_to_sig(result,")",&currPos);

  /* if this is a void method, we are finished */
  /* otherwise, print "->" and the return type */
  if (!((currentFunction->ReturnType & VTK_PARSE_BASE_TYPE) == VTK_PARSE_VOID)
      || (currentFunction->ReturnType & VTK_PARSE_INDIRECT))
    {
    add_to_sig(result," -> ",&currPos);

    switch (currentFunction->ReturnType & VTK_PARSE_UNQUALIFIED_TYPE)
      {
      case VTK_PARSE_VOID_PTR:
      case VTK_PARSE_CHAR_PTR:
        add_to_sig(result,"string",&currPos);
        break;
      case VTK_PARSE_VTK_OBJECT_REF:
      case VTK_PARSE_VTK_OBJECT_PTR:
        add_to_sig(result,currentFunction->ReturnClass,&currPos);
        break;
      case VTK_PARSE_FLOAT_PTR:
      case VTK_PARSE_DOUBLE_PTR:
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
      case VTK_PARSE_INT_PTR:
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
      case VTK_PARSE_ID_TYPE_PTR:
        add_to_sig(result,"(",&currPos);
        for (j = 0; j < currentFunction->HintSize; j++) 
          {
          if (j != 0)
            {
            add_to_sig(result,", ",&currPos);
            }
#if defined(VTK_USE_64BIT_IDS) && (VTK_SIZEOF_LONG != VTK_SIZEOF_ID_TYPE)
          add_to_sig(result,"long",&currPos);
#else
          add_to_sig(result,"int",&currPos);
#endif
          }
        add_to_sig(result,")",&currPos);
        break;
      case VTK_PARSE_LONG_LONG_PTR:
      case VTK_PARSE___INT64_PTR:
        add_to_sig(result,"(",&currPos);
        for (j = 0; j < currentFunction->HintSize; j++) 
          {
          if (j != 0)
            {
            add_to_sig(result,", ",&currPos);
            }
          add_to_sig(result,"long",&currPos);
          }
        add_to_sig(result,")",&currPos);
        break;
      case VTK_PARSE_FLOAT:
      case VTK_PARSE_DOUBLE:
        add_to_sig(result,"float",&currPos);
        break;
      case VTK_PARSE_ID_TYPE:
      case VTK_PARSE_LONG_LONG:
      case VTK_PARSE___INT64:
      case VTK_PARSE_SIGNED_CHAR:
      case VTK_PARSE_UNSIGNED_LONG_LONG:
      case VTK_PARSE_UNSIGNED___INT64:
      case VTK_PARSE_UNSIGNED_CHAR:
      case VTK_PARSE_UNSIGNED_INT:
      case VTK_PARSE_UNSIGNED_SHORT:
      case VTK_PARSE_UNSIGNED_LONG:
      case VTK_PARSE_INT:
      case VTK_PARSE_SHORT:
      case VTK_PARSE_LONG:
        add_to_sig(result,"int",&currPos);
        break;
      case VTK_PARSE_CHAR:
        add_to_sig(result,"char",&currPos);
        break;
      case VTK_PARSE_BOOL:
        add_to_sig(result,"bool",&currPos);
        break;
      }
    }
  
  if (currentFunction->Signature)
    {
    add_to_sig(result,"\\nC++: ",&currPos);
    add_to_sig(result,currentFunction->Signature,&currPos);
    }

  currentFunction->Signature = realloc(currentFunction->Signature,
                                       (size_t)(currPos+1));
  strcpy(currentFunction->Signature,result);
  /* fprintf(stderr,"%s\n",currentFunction->Signature); */
}

/* convert special characters in a string into their escape codes,
   so that the string can be quoted in a source file (the specified
   maxlen must be at least 32 chars)*/
static const char *quote_string(const char *comment, int maxlen)
{
  static char *result = 0;
  static int oldmaxlen = 0;
  int i, j, n;

  if (maxlen > oldmaxlen)
    {
    if (result)
      {
      free(result);
      }
    result = (char *)malloc((size_t)(maxlen+1));
    oldmaxlen = maxlen;
    }

  if (comment == NULL)
    {
    return "";
    }

  j = 0;

  n = (int)strlen(comment);
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
      j += (int)strlen(" ...\\n [Truncated]\\n");
      break;
      }
    }
  result[j] = '\0';

  return result;
}
  

void outputFunction2(FILE *fp, FileInfo *data)
{
  int i, j, k, is_static, is_vtkobject, fnum, occ, backnum, goto_used;
  int all_legacy;
  FunctionInfo *theFunc;
  FunctionInfo *backFunc;
  int returnType = 0;
  int backType = 0;
  int argType = 0;

  is_vtkobject = ((strcmp(data->ClassName,"vtkObjectBase") == 0) || 
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
    returnType = (theFunc->ReturnType & VTK_PARSE_UNQUALIFIED_TYPE);

    /* check for object return types */
    if ((returnType == VTK_PARSE_VTK_OBJECT_PTR) ||
        (returnType == VTK_PARSE_VTK_OBJECT_REF))
      {
      /* check that we haven't done this type (no duplicate declarations) */
      for (backnum = fnum-1; backnum >= 0; backnum--) 
        {
        backFunc = wrappedFunctions[backnum];
        backType = (backFunc->ReturnType & VTK_PARSE_UNQUALIFIED_TYPE);

        if (((backType == VTK_PARSE_VTK_OBJECT_PTR) ||
             (backType == VTK_PARSE_VTK_OBJECT_REF)) &&
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
    returnType = (theFunc->ReturnType & VTK_PARSE_UNQUALIFIED_TYPE);

    if (theFunc->Name)
      {
      fprintf(fp,"\n");

      /* check whether all signatures are static methods or legacy */
      is_static = 1;
      all_legacy = 1;
      for (occ = fnum; occ < numberOfWrappedFunctions; occ++)
        {
        /* is it the same name */
        if (wrappedFunctions[occ]->Name &&
            !strcmp(theFunc->Name,wrappedFunctions[occ]->Name))
          {
          /* check for static methods */
          if ((wrappedFunctions[occ]->ReturnType & VTK_PARSE_STATIC) == 0)
            {
            is_static = 0;
            }

          /* check for legacy */
          if (!wrappedFunctions[occ]->IsLegacy)
            {
            all_legacy = 0;
            }
          }
        }

      if(all_legacy)
        {
        fprintf(fp,
                "#if !defined(VTK_LEGACY_REMOVE)\n");
        }
      fprintf(fp,
              "static PyObject *Py%s_%s(PyObject *%s, PyObject *args)\n",
              data->ClassName,currentFunction->Name,
              (is_static ? "" : "self"));
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
          if ((wrappedFunctions[occ]->ReturnType & VTK_PARSE_STATIC) != 0)
            {
            is_static = 1;
            }

          currentFunction = wrappedFunctions[occ];
          returnType = (currentFunction->ReturnType &
                        VTK_PARSE_UNQUALIFIED_TYPE);

          if(currentFunction->IsLegacy && !all_legacy)
            {
            fprintf(fp,
                    "#if defined(VTK_LEGACY_REMOVE)\n");

            /* avoid warnings if all signatures are legacy and removed */
            if(!is_static)
              {
              fprintf(fp,
                      "  (void)self;"
                      " /* avoid warning if all signatures removed */\n");
              }
            fprintf(fp,
                    "  (void)args;"
                    " /* avoid warning if all signatures removed */\n"
                    "#else\n");
            }

          fprintf(fp,
                  "  /* handle an occurrence */\n  {\n");
          /* declare the variables */
          if (!is_static)
            {
            if (is_vtkobject)
              {
              fprintf(fp,
                      "  %s *op;\n\n",data->ClassName);
              }
            else 
              {
              fprintf(fp,
                      "  %s *op = (%s *)((PyVTKSpecialObject *)self)->vtk_ptr;\n\n",
                      data->ClassName, data->ClassName);
              }
            }

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
            fprintf(fp,
                    "  PyErr_Clear();\n");
            }
          if (is_static || !is_vtkobject)
            {
            fprintf(fp,
                    "  if ((PyArg_ParseTuple(args, (char*)\"%s\"",
                    get_format_string());
            }
          else
            {
            fprintf(fp,
                    "  op = (%s *)PyArg_VTKParseTuple(self, args, (char*)\"%s\"",
                    data->ClassName, get_format_string());
            }
          for (i = 0; i < currentFunction->NumberOfArguments; i++)
            {
            argType = (currentFunction->ArgTypes[i] &
                       VTK_PARSE_UNQUALIFIED_TYPE);

            if ((argType == VTK_PARSE_VTK_OBJECT_PTR) ||
                (argType == VTK_PARSE_VTK_OBJECT_REF))
              {
              fprintf(fp,", &tempH%d",i);
              }
            else if (argType == VTK_PARSE_VOID_PTR)
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
          if (is_static || !is_vtkobject)
            {
            fprintf(fp, ")))\n"
                    "    {\n");
            }
          else
            {
            fprintf(fp,");\n"
                    "  if (op)\n    {\n");
            }

          /* lookup and required objects */
          for (i = 0; i < currentFunction->NumberOfArguments; i++)
            {
            argType = (currentFunction->ArgTypes[i] &
                       VTK_PARSE_UNQUALIFIED_TYPE);

            if ((argType == VTK_PARSE_VTK_OBJECT_PTR) ||
                (argType == VTK_PARSE_VTK_OBJECT_REF))
              {
              fprintf(fp,
                      "    temp%d = (%s *)vtkPythonGetPointerFromObject(tempH%d,(char*)\"%s\");\n",
                      i, currentFunction->ArgClasses[i], i, 
                      currentFunction->ArgClasses[i]);
              fprintf(fp,
                      "    if (!temp%d && tempH%d != Py_None) goto break%d;\n",
                      i,i,occ);
              goto_used = 1;
              }
            }
          
          /* make sure passed method is callable  for VAR functions */
          if (currentFunction->NumberOfArguments == 1 &&
              currentFunction->ArgTypes[0] == VTK_PARSE_FUNCTION)
            {
            fprintf(fp,
                    "    if (!PyCallable_Check(temp0) && temp0 != Py_None)\n"
                    "      {\n"
                    "      PyErr_SetString(PyExc_ValueError,\"vtk callback method passed to %s in %s was not callable.\");\n"
                    "      return NULL;\n"
                    "      }\n"
                    "    Py_INCREF(temp0);\n",
                    currentFunction->Name, data->ClassName);
            }
          
          /* check for void pointers and pass appropriate info*/
          for (i = 0; i < currentFunction->NumberOfArguments; i++)
            {
            argType = (currentFunction->ArgTypes[i] &
                       VTK_PARSE_UNQUALIFIED_TYPE);

            if (argType == VTK_PARSE_VOID_PTR)
              {
              fprintf(fp,
                      "    temp%i = vtkPythonUnmanglePointer((char *)temp%i,&size%i,(char*)\"%s\");\n",
                      i,i,i,"void_p");

              fprintf(fp,
                      "    if (size%i == -1) {\n"
                      "      PyErr_SetString(PyExc_ValueError,\"mangled pointer to %s in %s was of incorrect type.\");\n"
                      "      return NULL;\n"
                      "      }\n",
                      i,currentFunction->Name,data->ClassName);

              fprintf(fp,
                      "    else if (size%i == -2) {\n"
                      "      PyErr_SetString(PyExc_ValueError,\"mangled pointer to %s in %s was poorly formed.\");\n"
                      "      return NULL;\n"
                      "      }\n",
                      i,currentFunction->Name,data->ClassName);
              }
            }

          for (k = 0; k < (2 - (is_static || !is_vtkobject)); k++)
            {
            char methodname[256]; 
            if (k == 0)
              {
              if (is_static)
                {
                fprintf(fp,
                        "      {\n");

                sprintf(methodname,"%s::%s",
                        data->ClassName,currentFunction->Name);
                }
              else if (!is_vtkobject)
                {
                fprintf(fp,
                        "      {\n");

                sprintf(methodname,"op->%s",currentFunction->Name);
                }
              else
                {
                if (currentFunction->IsPureVirtual)
                  {
                  fprintf(fp,
                          "    if (PyVTKClass_Check(self))\n"
                          "      {\n"
                          "      PyErr_SetString(PyExc_TypeError,\"pure virtual method call\");\n"
                          "      return NULL;\n"
                          "      }\n");
                  continue;
                  }
                else
                  {
                  fprintf(fp,
                          "    if (PyVTKClass_Check(self))\n"
                          "      {\n");

                  sprintf(methodname,"op->%s::%s",
                    data->ClassName,currentFunction->Name);
                  }
                }
              }
            else
              {
              fprintf(fp,
                      "    else\n      {\n");

              sprintf(methodname,"op->%s",currentFunction->Name);
              }
                
            switch (returnType)
              {
              case VTK_PARSE_VOID:
                fprintf(fp,
                        "      %s(",methodname);
                break;
              case VTK_PARSE_VTK_OBJECT_REF:
                fprintf(fp,
                        "      temp%i = &%s(",
                        MAX_ARGS, methodname);
                break;
              default:
                fprintf(fp,
                        "      temp%i = %s(",
                        MAX_ARGS, methodname);
              }

            for (i = 0; i < currentFunction->NumberOfArguments; i++)
              {
              argType = (currentFunction->ArgTypes[i] &
                         VTK_PARSE_UNQUALIFIED_TYPE);

              if (i)
                {
                fprintf(fp,",");
                }
              if (argType == VTK_PARSE_VTK_OBJECT_REF)
                {
                fprintf(fp,"*(temp%i)",i);
                }
              else if (currentFunction->NumberOfArguments == 1 
                       && currentFunction->ArgTypes[i] == VTK_PARSE_FUNCTION)
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
                && currentFunction->ArgTypes[0] == VTK_PARSE_FUNCTION)
              {
              fprintf(fp,
                      "      %sArgDelete(vtkPythonVoidFuncArgDelete);\n",
                      methodname);
              }
            fprintf(fp,"      }\n");
            }

          for (i = 0; i < currentFunction->NumberOfArguments; i++)
            {
            argType = (currentFunction->ArgTypes[i] &
                       VTK_PARSE_UNQUALIFIED_TYPE);

            if (currentFunction->ArgCounts[i] &&  /* array */
                (argType & VTK_PARSE_BASE_TYPE) != 0 && /* not special type */
                (argType & VTK_PARSE_BASE_TYPE) != VTK_PARSE_VTK_OBJECT &&
                (argType & VTK_PARSE_BASE_TYPE) != VTK_PARSE_UNKNOWN &&
                (argType & VTK_PARSE_BASE_TYPE) != VTK_PARSE_VOID &&
                ((currentFunction->ArgTypes[i] & VTK_PARSE_CONST) == 0))
              {
              fprintf(fp,
                      "    if (vtkPythonCheckArray(args,%d,temp%d,%d)) {\n"
                      "      return 0;\n"
                      "      }\n",
                      i, i, currentFunction->ArgCounts[i]);
              }
            }
          do_return(fp);
          fprintf(fp,
                  "    }\n"
                  "  }\n");

          if (goto_used) 
            {
            fprintf(fp,
                    " break%d:\n",
                    occ);
            }
          if(currentFunction->IsLegacy && !all_legacy)
            {
            fprintf(fp,
                    "#endif\n");
            }
          }
        }
      fprintf(fp,
              "  return NULL;\n}"
              "\n");

      if(all_legacy)
        {
        fprintf(fp,
                "#endif\n");
        }
      fprintf(fp,"\n");

      /* clear all occurances of this method from further consideration */
      for (occ = fnum + 1; occ < numberOfWrappedFunctions; occ++)
        {
        /* is it the same name */
        if (wrappedFunctions[occ]->Name && 
            !strcmp(theFunc->Name,wrappedFunctions[occ]->Name))
          {
          size_t siglen = strlen(wrappedFunctions[fnum]->Signature);
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
  fprintf(fp,
          "static PyMethodDef Py%sMethods[] = {\n",
          data->ClassName);
  
  for (fnum = 0; fnum < numberOfWrappedFunctions; fnum++)
    {
    if(wrappedFunctions[fnum]->IsLegacy)
      {
      fprintf(fp,
              "#if !defined(VTK_LEGACY_REMOVE)\n");
      }
    if (wrappedFunctions[fnum]->Name)
      {
      fprintf(fp,
              "  {(char*)\"%s\",                (PyCFunction)Py%s_%s, 1,\n"
              "   (char*)\"%s\\n\\n%s\"},\n",
              wrappedFunctions[fnum]->Name, data->ClassName, 
              wrappedFunctions[fnum]->Name, wrappedFunctions[fnum]->Signature,
              quote_string(wrappedFunctions[fnum]->Comment,1000));
      }
    if(wrappedFunctions[fnum]->IsLegacy)
      {
      fprintf(fp,
              "#endif\n");
      }
    }
  
  if (!strcmp("vtkObject",data->ClassName))
    {
    fprintf(fp,
            "  {(char*)\"AddObserver\",  (PyCFunction)Py%s_AddObserver, 1,\n"
            "   (char*)\"V.AddObserver(int, function) -> int\\n\\n Add an event callback function(vtkObject, int) for an event type.\\n Returns a handle that can be used with RemoveEvent(int).\"},\n",
            data->ClassName);
    }
  else if (!strcmp("vtkObjectBase",data->ClassName))
    {
    fprintf(fp,
            "  {(char*)\"GetAddressAsString\",  (PyCFunction)Py%s_GetAddressAsString, 1,\n"
            "   (char*)\"V.GetAddressAsString(string) -> string\\n\\n Get address of C++ object in format 'Addr=%%p' after casting to\\n the specified type.  You can get the same information from V.__this__.\"},\n"
            "  {(char*)\"PrintRevisions\",  (PyCFunction)Py%s_PrintRevisions, 1,\n"
            "   (char*)\"V.PrintRevisions() -> string\\n\\n Prints the .cxx file CVS revisions of the classes in the\\n object's inheritance chain.\"},\n",
            data->ClassName, data->ClassName);
    }
  
  fprintf(fp,
          "  {NULL,                       NULL, 0, NULL}\n"
          "};\n"
          "\n");
}



void outputFunction(FILE *fp, FileInfo *data)
{
  int i;
  int args_ok = 1;
  int argType = 0;
  int returnType = 0;
 
  fp = fp;
  /* some functions will not get wrapped no matter what else,
     and some really common functions will appear only in vtkObjectPython */
  if (currentFunction->IsOperator || 
      currentFunction->ArrayFailure ||
      !currentFunction->IsPublic ||
      !currentFunction->Name)
    {
    return;
    }
  
  returnType = (currentFunction->ReturnType & VTK_PARSE_UNQUALIFIED_TYPE);

  /* check to see if we can handle the args */
  for (i = 0; i < currentFunction->NumberOfArguments; i++)
    {
    argType = (currentFunction->ArgTypes[i] & VTK_PARSE_UNQUALIFIED_TYPE);

    if (argType == VTK_PARSE_VTK_OBJECT) args_ok = 0;
    if ((argType & VTK_PARSE_BASE_TYPE) == VTK_PARSE_UNKNOWN) args_ok = 0;
    if (((argType & VTK_PARSE_INDIRECT) != VTK_PARSE_POINTER) &&
        (argType != VTK_PARSE_VTK_OBJECT_REF) &&
        ((argType & VTK_PARSE_INDIRECT) != 0)) args_ok = 0;
    if (argType == VTK_PARSE_UNSIGNED_CHAR_PTR) args_ok = 0;
    if (argType == VTK_PARSE_UNSIGNED_INT_PTR) args_ok = 0;
    if (argType == VTK_PARSE_UNSIGNED___INT64_PTR) args_ok = 0;
    if (argType == VTK_PARSE_UNSIGNED_SHORT_PTR) args_ok = 0;
    if (argType == VTK_PARSE_UNSIGNED_LONG_PTR) args_ok = 0;
    }
  if ((returnType & VTK_PARSE_BASE_TYPE) == VTK_PARSE_UNKNOWN) args_ok = 0;
  if (returnType == VTK_PARSE_VTK_OBJECT) args_ok = 0;
  if (((returnType & VTK_PARSE_INDIRECT) != VTK_PARSE_POINTER) &&
      (returnType != VTK_PARSE_VTK_OBJECT_REF) &&
      ((returnType & VTK_PARSE_INDIRECT) != 0)) args_ok = 0;


  /* eliminate unsigned char * and unsigned short * */
  if (returnType == VTK_PARSE_UNSIGNED_CHAR_PTR) args_ok = 0;
  if (returnType == VTK_PARSE_UNSIGNED_INT_PTR) args_ok = 0;
  if (returnType == VTK_PARSE_UNSIGNED___INT64_PTR) args_ok = 0;
  if (returnType == VTK_PARSE_UNSIGNED_SHORT_PTR) args_ok = 0;
  if (returnType == VTK_PARSE_UNSIGNED_LONG_PTR) args_ok = 0;

  if (currentFunction->NumberOfArguments && 
      (currentFunction->ArgTypes[0] == VTK_PARSE_FUNCTION) &&
      (currentFunction->NumberOfArguments != 1)) args_ok = 0;

  /* make sure we have all the info we need for array arguments in */
  for (i = 0; i < currentFunction->NumberOfArguments; i++)
    {
    argType = (currentFunction->ArgTypes[i] & VTK_PARSE_UNQUALIFIED_TYPE);

    if (((argType & VTK_PARSE_INDIRECT) == VTK_PARSE_POINTER) &&
        (currentFunction->ArgCounts[i] <= 0) &&
        (argType != VTK_PARSE_VTK_OBJECT_PTR) &&
        (argType != VTK_PARSE_CHAR_PTR) &&
        (argType != VTK_PARSE_VOID_PTR)) args_ok = 0;
    }

  /* if we need a return type hint make sure we have one */
  switch (returnType)
    {
    case VTK_PARSE_FLOAT_PTR:
    case VTK_PARSE_DOUBLE_PTR:
    case VTK_PARSE_ID_TYPE_PTR:
    case VTK_PARSE_LONG_LONG_PTR:
    case VTK_PARSE___INT64_PTR:
    case VTK_PARSE_SIGNED_CHAR_PTR:
    case VTK_PARSE_BOOL_PTR:
    case VTK_PARSE_INT_PTR:
    case VTK_PARSE_SHORT_PTR:
    case VTK_PARSE_LONG_PTR:
      args_ok = currentFunction->HaveHint;
      break;
    }
  
  /* make sure it isn't a Delete or New function */
  if (!strcmp("Delete",currentFunction->Name) ||
      !strcmp("New",currentFunction->Name))
    {
    args_ok = 0;
    }
  
  /* check for New() function */
  if (!strcmp("New",currentFunction->Name) &&
      currentFunction->NumberOfArguments == 0)
    {
    class_has_new = 1;
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
  size_t i, n;
  char temp[500];

  if (data->NameComment) 
    {
    text = data->NameComment;
    while (*text == ' ')
      {
      text++;
      }
    fprintf(fp,
            "  (char*)\"%s\\n\\n\",\n",
            quote_string(text,500));
    }
  else
    {
    fprintf(fp,
            "  (char*)\"%s - no description provided.\\n\\n\",\n",
            quote_string(data->ClassName,500));
    }

  if (data->NumberOfSuperClasses > 0)
    {
    fprintf(fp,
            "  \"Super Class:\\n\\n %s\\n\\n\",\n",
            quote_string(data->SuperClasses[0],500));
    }

  if (data->Description)
    {
    n = (strlen(data->Description) + 400-1)/400;
    for (i = 0; i < n; i++)
      {
      strncpy(temp, &data->Description[400*i], 400);
      temp[400] = '\0';
      if (i < n-1)
        {
        fprintf(fp,
                "  (char*)\"%s\",\n",
                quote_string(temp,500));
        }
      else
        { /* just for the last time */
        fprintf(fp,
                "  (char*)\"%s\\n\",\n",
                quote_string(temp,500));
        }
      }
    }
  else
    {
    fprintf(fp,
            "  (char*)\"%s\\n\",\n",
            "None provided.\\n");
    }

  if (data->Caveats)
    {
    fprintf(fp,
            "  \"Caveats:\\n\\n"
            "%s\\n\",\n",
            quote_string(data->Caveats,500));
    }

  if (data->SeeAlso)
    {
    char *sdup, *tok;
    
    fprintf(fp,
            "  \"See Also:\\n\\n");

    sdup = strdup(data->SeeAlso);
    tok = strtok(sdup," ");
    while (tok)
      {
      fprintf(fp," %s",quote_string(tok,120));
      tok = strtok(NULL," ");
      }
    free(sdup);
    fprintf(fp,"\\n\",\n");
    }

  fprintf(fp,
          "  NULL\n");
}

/* print the parsed structures */
void vtkParseOutput(FILE *fp, FileInfo *data)
{
  int i;
  
  fprintf(fp,
          "// python wrapper for %s object\n//\n"
          "#define VTK_WRAPPING_CXX\n",
          data->ClassName);

  if (strcmp("vtkObjectBase",data->ClassName) != 0)
    {
    /* Block inclusion of full streams.  */
    fprintf(fp,
            "#define VTK_STREAMS_FWD_ONLY\n");
    }

  #if !defined(__APPLE__)
  fprintf(fp,
          "#include \"vtkPython.h\"\n"
          "#undef _XOPEN_SOURCE /* Conflicts with standards.h.  */\n"
          "#undef _THREAD_SAFE /* Conflicts with pthread.h.  */\n");
  #endif

  fprintf(fp,
          "#include \"vtkPythonUtil.h\"\n"
          "#include <vtksys/ios/sstream>\n"
          "#include \"%s.h\"\n",
          data->ClassName);

  fprintf(fp,
          "#if defined(WIN32)\n"
          "extern \"C\" { __declspec( dllexport ) PyObject *PyVTKClass_%sNew(char *); }\n"
          "#else\n"
          "extern \"C\" { PyObject *PyVTKClass_%sNew(char *); }\n"
          "#endif\n"
          "\n",
          data->ClassName, data->ClassName);

  for (i = 0; i < data->NumberOfSuperClasses; i++)
    {
    fprintf(fp,
           "extern \"C\" { PyObject *PyVTKClass_%sNew(char *); }\n",
            data->SuperClasses[i]);
    }
  
  if (!strcmp("vtkObject",data->ClassName))
    {
    /* Add the AddObserver method to vtkObject. */
    fprintf(fp,
            "static PyObject *PyvtkObject_AddObserver(PyObject *self, PyObject *args)\n"
            "{\n"
            "  vtkObject *op;\n"
            "  char *temp0;\n"
            "  PyObject *temp1;\n"
            "  float temp2;\n"
            "  unsigned long     temp20 = 0;\n");

    fprintf(fp,
            "  op = (vtkObject *)PyArg_VTKParseTuple(self, args, (char*)\"zO\", &temp0, &temp1);\n"
            "  if (op)\n"
            "    {\n"
            "    if (!PyCallable_Check(temp1) && temp1 != Py_None)\n"
            "      {\n"
            "      PyErr_SetString(PyExc_ValueError,\"vtk callback method passed to AddObserver was not callable.\");\n"
            "      return NULL;\n"
            "      }\n"
            "    Py_INCREF(temp1);\n"
            "    vtkPythonCommand *cbc = vtkPythonCommand::New();\n"
            "    cbc->SetObject(temp1);\n"
            "    cbc->SetThreadState(PyThreadState_Get());\n"
            "    temp20 = op->AddObserver(temp0,cbc);\n"
            "    cbc->Delete();\n"
            "    return PyInt_FromLong((long)temp20);\n"
            "    }\n"
            "  PyErr_Clear();\n");

    fprintf(fp,
            "  op = (vtkObject *)PyArg_VTKParseTuple(self, args, (char*)\"zOf\", &temp0, &temp1, &temp2);\n"
            "  if (op)\n"
            "    {\n"
            "    if (!PyCallable_Check(temp1) && temp1 != Py_None)\n"
            "      {\n"
            "      PyErr_SetString(PyExc_ValueError,\"vtk callback method passed to AddObserver was not callable.\");\n"
            "      return NULL;\n"
            "      }\n"
            "    Py_INCREF(temp1);\n"
            "    vtkPythonCommand *cbc = vtkPythonCommand::New();\n"
            "    cbc->SetObject(temp1);\n"
            "    cbc->SetThreadState(PyThreadState_Get());\n"
            "    temp20 = op->AddObserver(temp0,cbc,temp2);\n"
            "    cbc->Delete();\n"
            "    return PyInt_FromLong((long)temp20);\n"
            "    }\n");

    fprintf(fp,
            "  return NULL;\n"
            "}\n"
            "\n");
    }

  if (!strcmp("vtkObjectBase",data->ClassName))
    {
    /* while we are at it spit out the GetStringFromObject method */
    fprintf(fp,
            "PyObject *PyvtkObjectBase_GetAddressAsString(PyObject *self, PyObject *args)\n"
            "{\n"
            "  %s *op;\n"
            "  char *typecast;\n"
            "\n"
            "  op = (%s *)PyArg_VTKParseTuple(self, args, (char*)\"s\", &typecast);\n"
            "  if (op)\n"
            "    {\n    char temp20[256];\n"
            "    sprintf(temp20,\"Addr=%%p\",op);\n"
            "    return PyString_FromString(temp20);\n"
            "    }\n"
            "  return NULL;\n"
            "}\n"
            "\n",
            data->ClassName, data->ClassName);

    /* Add the PrintRevisions method to vtkObjectBase. */
    fprintf(fp,
            "PyObject *PyvtkObjectBase_PrintRevisions(PyObject *self, PyObject *args)\n"
            "{\n"
            "  %s *op;\n"
            "  op = (%s *)PyArg_VTKParseTuple(self, args, (char*)\"\");\n"
            "  if (op)\n"
            "    {\n"
            "    vtksys_ios::ostringstream vtkmsg_with_warning_C4701;\n"
            "    op->PrintRevisions(vtkmsg_with_warning_C4701);\n"
            "    vtkmsg_with_warning_C4701.put('\\0');\n"
            "    PyObject *result = PyString_FromString(vtkmsg_with_warning_C4701.str().c_str());\n"
            "    return result;\n"
            "    }\n"
            "  return NULL;\n"
            "}\n"
            "\n",
            data->ClassName, data->ClassName);

    }
  
  /* insert function handling code here */
  for (i = 0; i < data->NumberOfFunctions; i++)
    {
    currentFunction = data->Functions + i;
    outputFunction(fp, data);
    }
  if (data->NumberOfSuperClasses || !data->IsAbstract)
    {
    outputFunction2(fp, data);
    }
  
  /* the docstring for the class */
  if (data->NumberOfSuperClasses || !data->IsAbstract)
    {
    fprintf(fp,
            "static const char *%sDoc[] = {\n",
            data->ClassName);

    create_class_doc(fp,data);

    fprintf(fp,
            "};\n"
            "\n");
    }
  
  /* output the class initilization function */
  if (strcmp(data->ClassName,"vtkObjectBase") == 0)
    { /* special wrapping for vtkObject */
    if (class_has_new)
      {
      fprintf(fp,
              "static vtkObjectBase *%sStaticNew()\n"
              "{\n"
              "  return %s::New();\n"
              "}\n"
              "\n",
              data->ClassName, data->ClassName);
      }

    fprintf(fp,
            "PyObject *PyVTKClass_%sNew(char *modulename)\n"
            "{\n",
            data->ClassName);

    if (class_has_new)
      {
      fprintf(fp,
              "  return PyVTKClass_New(&%sStaticNew,\n",
              data->ClassName);
      }
    else
      {
      fprintf(fp,
              "  return PyVTKClass_New(NULL,\n");
      }      
    fprintf(fp,
            "                        Py%sMethods,\n"
            "                        (char*)\"%s\",modulename,\n"
            "                        (char**)%sDoc,0);\n"
            "}\n"
            "\n",
            data->ClassName, data->ClassName, data->ClassName);
    }
  else if (data->NumberOfSuperClasses)
    { /* wrapping of descendants of vtkObjectBase */
    if (class_has_new)
      {
      fprintf(fp,
              "static vtkObjectBase *%sStaticNew()\n"
              "{\n"
              "  return %s::New();\n"
              "}\n"
              "\n",
              data->ClassName, data->ClassName);
      }
    fprintf(fp,
            "PyObject *PyVTKClass_%sNew(char *modulename)\n"
            "{\n",
            data->ClassName);

    if (class_has_new)
      {
      fprintf(fp,
              "  return PyVTKClass_New(&%sStaticNew,\n",
              data->ClassName);
      }
    else
      {
      fprintf(fp,
              "  return PyVTKClass_New(NULL,\n");
      }      

    fprintf(fp,
            "                        Py%sMethods,\n"
            "                        (char*)\"%s\",modulename,\n"
            "                        (char**)%sDoc,\n"
            "                        PyVTKClass_%sNew(modulename));\n"
            "}\n"
            "\n",
            data->ClassName, data->ClassName, data->ClassName,
            data->SuperClasses[0]);
    }
  else if (!data->IsAbstract)
    { /* wrapping of 'special' non-vtkObject classes */
    fprintf(fp,
            "PyObject *PyVTKObject_%sNew(PyObject *, PyObject *args)\n"
            "{\n"
            "  if (!(PyArg_ParseTuple(args, (char*)\"\")))\n"
            "    {\n"
            "    return NULL;\n    }\n"
            "\n"
            "  %s *obj = new %s;\n"
            "  return PyVTKSpecialObject_New(obj, Py%sMethods, (char*)\"%s\",(char**)%sDoc);\n"
            "}\n"
            "\n",
            data->ClassName, data->ClassName, data->ClassName,
            data->ClassName, data->ClassName, data->ClassName);

    fprintf(fp,
            "static PyMethodDef Py%sNewMethod = \\\n"
            "{ (char*)\"%s\",  (PyCFunction)PyVTKObject_%sNew, 1,\n"
            "  (char*)%sDoc[0] };\n"
            "\n",
            data->ClassName, data->ClassName, data->ClassName,
            data->ClassName);

    fprintf(fp,
            "PyObject *PyVTKClass_%sNew(char *)\n"
            "{\n"
            "  return PyCFunction_New(&Py%sNewMethod,Py_None);\n"
            "}\n"
            "\n",
            data->ClassName, data->ClassName);
    }
  else
    { /* un-wrappable classes */
    fprintf(fp,
            "PyObject *PyVTKClass_%sNew(char *)\n"
            "{\n"
            "  return NULL;\n"
            "}\n"
            "\n",
            data->ClassName);
    }
}

