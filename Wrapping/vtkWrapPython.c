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
#include "vtkConfigure.h"

int numberOfWrappedFunctions = 0;
FunctionInfo *wrappedFunctions[1000];
extern FunctionInfo *currentFunction;

static int class_has_new = 0;

/* when the cpp file doesn't have enough info use the hint file */
void use_hints(FILE *fp)
{
  int  i;

  fprintf(fp,"    if(temp%i)\n",MAX_ARGS);
  fprintf(fp,"      {\n");

  switch (currentFunction->ReturnType % 0x1000)
    {
    case 0x301:
      fprintf(fp,"    return Py_BuildValue((char*)\"");
      for (i = 0; i < currentFunction->HintSize; i++) fprintf(fp,"f");
      fprintf(fp,"\"");
      for (i = 0; i < currentFunction->HintSize; i++)
        {
        fprintf(fp,",temp%i[%d]",MAX_ARGS,i);
        }
      fprintf(fp,");\n");
      break;
    case 0x307:  
      fprintf(fp,"    return Py_BuildValue((char*)\"");
      for (i = 0; i < currentFunction->HintSize; i++) fprintf(fp,"d");
      fprintf(fp,"\"");
      for (i = 0; i < currentFunction->HintSize; i++)
        {
        fprintf(fp,",temp%i[%d]",MAX_ARGS,i);
        }
      fprintf(fp,");\n");
      break;
    case 0x304:
    case 0x30D:
      fprintf(fp,"    return Py_BuildValue((char*)\"");
      for (i = 0; i < currentFunction->HintSize; i++) fprintf(fp,"i");
      fprintf(fp,"\"");
      for (i = 0; i < currentFunction->HintSize; i++) 
        {
        fprintf(fp,",temp%i[%d]",MAX_ARGS,i);
        }
      fprintf(fp,");\n");
      break;
    case 0x30E:
      fprintf(fp,"    return Py_BuildValue((char*)\"");
      for (i = 0; i < currentFunction->HintSize; i++) fprintf(fp,"b");
      fprintf(fp,"\"");
      for (i = 0; i < currentFunction->HintSize; i++)
        {
        fprintf(fp,",(int)temp%i[%d]",MAX_ARGS,i);
        }
      fprintf(fp,");\n");
      break;
    case 0x30A:
      fprintf(fp,"    return Py_BuildValue((char*)\"");
#ifdef VTK_USE_64BIT_IDS
#ifdef PY_LONG_LONG
      for (i = 0; i < currentFunction->HintSize; i++) fprintf(fp,"L");
#else
      for (i = 0; i < currentFunction->HintSize; i++) fprintf(fp,"l");
#endif
#else
      for (i = 0; i < currentFunction->HintSize; i++) fprintf(fp,"i");
#endif
      fprintf(fp,"\"");
      for (i = 0; i < currentFunction->HintSize; i++) 
        {
        fprintf(fp,",temp%i[%d]",MAX_ARGS,i);
        }
      fprintf(fp,");\n");
      break;
    case 0x30B: case 0x30C:
      fprintf(fp,"    return Py_BuildValue((char*)\"");
#ifdef PY_LONG_LONG
      for (i = 0; i < currentFunction->HintSize; i++) fprintf(fp,"L");
#else
      for (i = 0; i < currentFunction->HintSize; i++) fprintf(fp,"l");
#endif
      fprintf(fp,"\"");
      for (i = 0; i < currentFunction->HintSize; i++) 
        {
        fprintf(fp,",temp%i[%d]",MAX_ARGS,i);
        }
      fprintf(fp,");\n");
      break;
    case 0x305: case 0x306: case 0x313: case 0x314:
    case 0x31A: case 0x31B: case 0x31C: case 0x315: case 0x316:
      break;
    }

  fprintf(fp,"      }\n");
  fprintf(fp,"    else\n");
  fprintf(fp,"      {\n");
  fprintf(fp,"      return Py_BuildValue((char*)\"\");\n");
  fprintf(fp,"      }\n");

  return;
}


void output_temp(FILE *fp, int i, int aType, char *Id, int aCount)
{
  /* handle VAR FUNCTIONS */
  if (aType == 0x5000)
    {
    fprintf(fp,"  PyObject *temp%i;\n",i); 
    return;
    }
  
  if (((aType % 0x10) == 0x2)&&(!((aType % 0x1000)/0x100)))
    {
    return;
    }

  /* for const * return types prototype with const */
  if ((i == MAX_ARGS)&&(aType % 0x2000 >= 0x1000))
    {
    fprintf(fp,"  const ");
    }
  else
    {
    fprintf(fp,"  ");
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
    case 0x9:     
      fprintf(fp,"%s ",Id); break;
    case 0xA:     fprintf(fp,"vtkIdType "); break;
    case 0xB:     fprintf(fp,"long long "); break;
    case 0xC:     fprintf(fp,"__int64 "); break;
    case 0xD:     fprintf(fp,"signed char "); break;
    case 0xE:     fprintf(fp,"bool "); break;
    case 0x8: return;
    }
  
  switch ((aType % 0x1000)/0x100)
    {
    case 0x1: fprintf(fp, " *"); break; /* act " &" */
    case 0x2: fprintf(fp, "&&"); break;
    case 0x3: 
      if ((i == MAX_ARGS)||(aType % 0x10 == 0x9)||(aType % 0x1000 == 0x303)
          ||(aType % 0x1000 == 0x302))
        {
        fprintf(fp, " *"); 
        }
      break;
    case 0x4: fprintf(fp, "&*"); break;
    case 0x5: fprintf(fp, "*&"); break;
    case 0x7: fprintf(fp, "**"); break;
    default: fprintf(fp,"  "); break;
    }
  fprintf(fp,"temp%i",i);
  
  /* handle arrays */
  if ((aType % 0x1000/0x100 == 0x3)&&
      (i != MAX_ARGS)&&(aType % 0x10 != 0x9)&&(aType % 0x1000 != 0x303)
      &&(aType % 0x1000 != 0x302))
    {
    fprintf(fp,"[%i]",aCount);
    }

  fprintf(fp,";\n");
  if (aType % 0x1000 == 0x302 && i != MAX_ARGS)
    {
    fprintf(fp,"  int      size%d;\n",i);
    }
  if ((i != MAX_ARGS) && ((aType % 0x1000 == 0x309)||(aType % 0x1000 == 0x109)))
    {
    fprintf(fp,"  PyObject *tempH%d;\n",i);
    }
}

void do_return(FILE *fp)
{
  /* ignore void */
  if (((currentFunction->ReturnType % 0x10) == 0x2)&&
      (!((currentFunction->ReturnType % 0x1000)/0x100)))
    {
    fprintf(fp,"    Py_INCREF(Py_None);\n");
    fprintf(fp,"    return Py_None;\n");
    return;
    }
  
  switch (currentFunction->ReturnType % 0x1000)
    {
    case 0x303:
      fprintf(fp,"    if (temp%i == NULL) {\n",MAX_ARGS);
      fprintf(fp,"      Py_INCREF(Py_None);\n");
      fprintf(fp,"      return Py_None;\n      }\n");
      fprintf(fp,"    else {\n");
      fprintf(fp,"      return PyString_FromString(temp%i);\n      }\n",MAX_ARGS);
    break;
    case 0x109:
    case 0x309:  
      {
      fprintf(fp,"    return vtkPythonGetObjectFromPointer((vtkObjectBase *)temp%i);\n",
              MAX_ARGS);
      break;
      }
      
    /* handle functions returning vectors */
    /* this is done by looking them up in a hint file */
    case 0x301: case 0x307: case 0x30A: case 0x30B: case 0x30C: case 0x30D:
    case 0x304: case 0x305: case 0x306: case 0x30E:
      use_hints(fp);
      break;
    case 0x302:
      {
      fprintf(fp,"    if (temp%i == NULL)\n        {\n",MAX_ARGS);
      fprintf(fp,"      Py_INCREF(Py_None);\n");
      fprintf(fp,"      return Py_None;\n        }\n");
      fprintf(fp,"    else\n        {\n");
      fprintf(fp,"      return PyString_FromString(vtkPythonManglePointer(temp%i,\"void_p\"));\n        }\n",
              MAX_ARGS);
      break;
      }
    case 0x1:
    case 0x7:
      {
      fprintf(fp,"    return PyFloat_FromDouble(temp%i);\n",
                      MAX_ARGS);
      break;
      }
    case 0x13:
    case 0x14:
    case 0x15:
    case 0x4:
    case 0x5:
    case 0x6:
    case 0xD:
      {
      fprintf(fp,"    return PyInt_FromLong(temp%i);\n", MAX_ARGS); 
      break;
      }
    case 0xE:
      {
      /* PyBool_FromLong was introduced in Python 2.3.
         Use PyInt_FromLong as a bool substitute in
         earlier versions of python...
      */
      fprintf(fp,"#if PY_VERSION_HEX >= 0x02030000\n");
      fprintf(fp,"    return PyBool_FromLong(temp%i);\n", MAX_ARGS);
      fprintf(fp,"#else\n");
      fprintf(fp,"    return PyInt_FromLong((long)temp%i);\n", MAX_ARGS);
      fprintf(fp,"#endif\n");
      break;
      }
    case 0x16:
      {
#if (PY_VERSION_HEX >= 0x02020000)
      fprintf(fp,"    return PyLong_FromUnsignedLong(temp%i);\n",
              MAX_ARGS);
#else
      fprintf(fp,"    return PyInt_FromLong((long)temp%i);\n",
              MAX_ARGS);
#endif
      break;
      }
#if defined(VTK_USE_64BIT_IDS) && defined(PY_LONG_LONG) && (VTK_SIZEOF_LONG != VTK_SIZEOF_ID_TYPE)
    case 0xA:
      {
      fprintf(fp,"    return PyLong_FromLongLong(temp%i);\n", MAX_ARGS);
      break;
      }
    case 0x1A:
      {
      fprintf(fp,"    return PyLong_FromUnsignedLongLong(temp%i);\n",
              MAX_ARGS);
      break;
      }
#else
    case 0xA:
      {
      fprintf(fp,"    return PyInt_FromLong((long)temp%i);\n", MAX_ARGS);
      break;
      }
    case 0x1A:
      {
#if (PY_VERSION_HEX >= 0x02020000)
      fprintf(fp,"    return PyLong_FromUnsignedLong((unsigned long)temp%i);\n",
              MAX_ARGS);
#else
      fprintf(fp,"    return PyInt_FromLong((long)temp%i);\n",
              MAX_ARGS);
#endif
      break;
      }
#endif
#if defined(VTK_SIZEOF_LONG_LONG)
# if defined(PY_LONG_LONG) && (VTK_SIZEOF_LONG != VTK_SIZEOF_LONG_LONG)
    case 0xB:
      {
      fprintf(fp,"    return PyLong_FromLongLong(temp%i);\n", MAX_ARGS);
      break;
      }
    case 0x1B:
      {
      fprintf(fp,"    return PyLong_FromUnsignedLongLong(temp%i);\n",
              MAX_ARGS);
      break;
      }
# else
    case 0xB:
      {
      fprintf(fp,"    return PyLong_FromLong(temp%i);\n", MAX_ARGS);
      break;
      }
    case 0x1B:
      {
      fprintf(fp,"    return PyLong_FromUnsignedLong(temp%i);\n",
              MAX_ARGS);
      break;
      }
# endif
#endif
#if defined(VTK_SIZEOF___INT64)
# if defined(PY_LONG_LONG) && (VTK_SIZEOF_LONG != VTK_SIZEOF___INT64)
    case 0xC:
      {
      fprintf(fp,"    return PyLong_FromLongLong(temp%i);\n", MAX_ARGS);
      break;
      }
    case 0x1C:
      {
      fprintf(fp,"    return PyLong_FromUnsignedLongLong(temp%i);\n",
              MAX_ARGS);
      break;
      }
# else
    case 0xC:
      {
      fprintf(fp,"    return PyLong_FromLong(temp%i);\n", MAX_ARGS);
      break;
      }
    case 0x1C:
      {
      fprintf(fp,"    return PyLong_FromUnsignedLong(temp%i);\n",
              MAX_ARGS);
      break;
      }
# endif
#endif
    case 0x3:   
      {
      fprintf(fp,"    char temp%iString[2];\n"
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
  
  if (currentFunction->ArgTypes[0] == 0x5000)
    {
    result[currPos] = 'O'; currPos++; 
    result[currPos] = '\0';
    return result;
    }
  
  for (i = 0; i < currentFunction->NumberOfArguments; i++)
    {
    argtype = currentFunction->ArgTypes[i] % 0x1000;

    switch (argtype)
      {
      case 0x301:
        result[currPos] = '('; currPos++;
        for (j = 0; j < currentFunction->ArgCounts[i]; j++) 
          {
          result[currPos] = 'f'; currPos++;
          }
        result[currPos] = ')'; currPos++;
        break;
      case 0x307:  
        result[currPos] = '('; currPos++;
        for (j = 0; j < currentFunction->ArgCounts[i]; j++) 
          {
          result[currPos] = 'd'; currPos++;
          }
        result[currPos] = ')'; currPos++;
        break;
      case 0x304:
        result[currPos] = '('; currPos++;
        for (j = 0; j < currentFunction->ArgCounts[i]; j++) 
          {
          result[currPos] = 'i'; currPos++;
          }
        result[currPos] = ')'; currPos++;
        break;
      case 0x30A:
        result[currPos] = '('; currPos++;
        for (j = 0; j < currentFunction->ArgCounts[i]; j++) 
          {
#ifdef VTK_USE_64BIT_IDS
#ifdef PY_LONG_LONG
          result[currPos] = 'L'; currPos++;
#else
          result[currPos] = 'l'; currPos++;
#endif
#else
          result[currPos] = 'i'; currPos++;
#endif
          }
        result[currPos] = ')'; currPos++;
        break;
      case 0x30B: case 0x30C:
        result[currPos] = '('; currPos++;
        for (j = 0; j < currentFunction->ArgCounts[i]; j++) 
          {
#ifdef PY_LONG_LONG
          result[currPos] = 'L'; currPos++;
#else
          result[currPos] = 'l'; currPos++;
#endif
          }
        result[currPos] = ')'; currPos++;
        break;
      case 0x109:
      case 0x309: result[currPos] = 'O'; currPos++; break;
      case 0x303: result[currPos] = 'z'; currPos++; break;
      case 0x302: result[currPos] = 's'; currPos++; 
                result[currPos] = '#'; currPos++; break; 
      case 0x1:   result[currPos] = 'f'; currPos++; break;
      case 0x7:   result[currPos] = 'd'; currPos++; break;
      case 0x14:
      case 0x4:   result[currPos] = 'i'; currPos++; break;
      case 0x15:
      case 0x5:   result[currPos] = 'h'; currPos++; break;
      case 0x16:
      case 0x6:   result[currPos] = 'l'; currPos++; break;
      case 0x1A:
      case 0xA:
#ifdef VTK_USE_64BIT_IDS
#ifdef PY_LONG_LONG
        result[currPos] = 'L'; currPos++; break;
#else
        result[currPos] = 'l'; currPos++; break;
#endif
#else
        result[currPos] = 'i'; currPos++; break;
#endif
#ifdef PY_LONG_LONG
      case 0x1B: case 0x1C:
      case 0xB: case 0xC:
        result[currPos] = 'L'; currPos++; break;
#else
      case 0x1B: case 0x1C:
      case 0xB: case 0xC:
        result[currPos] = 'l'; currPos++; break;
#endif
      case 0xD:   result[currPos] = 'i'; currPos++; break;
      case 0x3:   result[currPos] = 'c'; currPos++; break;
      case 0x13:   result[currPos] = 'b'; currPos++; break;
      case 0xE:   result[currPos] = 'b'; currPos++; break;
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
    if (currentFunction->ArgTypes[i] == 0x5000)
      {
      add_to_sig(result,"function",&currPos); 
      }
    
    argtype = currentFunction->ArgTypes[i] % 0x1000;

    if (i != 0)
      {
      add_to_sig(result,", ",&currPos);
      }

    switch (argtype)
      {
      case 0x301:
      case 0x307:
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
      case 0x304:
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
      case 0x30A:
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
      case 0x30B: case 0x30C:
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
      case 0x109:
      case 0x309: add_to_sig(result,currentFunction->ArgClasses[i],&currPos); break;
      case 0x302:
      case 0x303: add_to_sig(result,"string",&currPos); break;
      case 0x1:
      case 0x7:   add_to_sig(result,"float",&currPos); break;
      case 0xD:
      case 0xA:
      case 0x1B:
      case 0xB:
      case 0x1C:
      case 0xC:
      case 0x14:
      case 0x4:
      case 0x15:
      case 0x5:
      case 0x16:
      case 0x6:   add_to_sig(result,"int",&currPos); break;
      case 0x3:   add_to_sig(result,"char",&currPos); break;
      case 0x13:  add_to_sig(result,"int",&currPos); break;
      case 0xE:   add_to_sig(result,"bool",&currPos); break;
      }
    }

  add_to_sig(result,")",&currPos);

  /* if this is a void method, we are finished */
  /* otherwise, print "->" and the return type */
  if ((!((currentFunction->ReturnType % 0x10) == 0x2)) ||
      ((currentFunction->ReturnType % 0x1000)/0x100))
    {
    add_to_sig(result," -> ",&currPos);

    switch (currentFunction->ReturnType % 0x1000)
      {
      case 0x302:
      case 0x303: add_to_sig(result,"string",&currPos); break;
      case 0x109:
      case 0x309: add_to_sig(result,currentFunction->ReturnClass,&currPos); break;
      case 0x301:
      case 0x307:
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
      case 0x304:
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
      case 0x30A:
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
      case 0x30B: case 0x30C:
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
      case 0x1:
      case 0x7: add_to_sig(result,"float",&currPos); break;
      case 0xA:
      case 0xB:
      case 0xC:
      case 0xD:
      case 0x1B:
      case 0x1C:
      case 0x13:
      case 0x14:
      case 0x15:
      case 0x16:
      case 0x4:
      case 0x5:
      case 0x6: add_to_sig(result,"int",&currPos); break;
      case 0x3: add_to_sig(result,"char",&currPos); break;
      case 0xE: add_to_sig(result,"bool",&currPos); break;
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

    /* check for object return types */
    if ((theFunc->ReturnType % 0x1000 == 0x309)||
        (theFunc->ReturnType % 0x1000 == 0x109))
      {
      /* check that we haven't done this type (no duplicate declarations) */
      for (backnum = fnum-1; backnum >= 0; backnum--) 
        {
        backFunc = wrappedFunctions[backnum];
        if (((backFunc->ReturnType % 0x1000 == 0x309)||
             (backFunc->ReturnType % 0x1000 == 0x109)) &&
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
          if (((wrappedFunctions[occ]->ReturnType/0x1000) & 0x2) != 0x2)
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
        fprintf(fp,"#if !defined(VTK_LEGACY_REMOVE)\n");
        }
      fprintf(fp,"static PyObject *Py%s_%s(PyObject *%s, PyObject *args)\n",
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
          if (((wrappedFunctions[occ]->ReturnType/0x1000) & 0x2) == 0x2)
            {
            is_static = 1;
            }

          currentFunction = wrappedFunctions[occ];

          if(currentFunction->IsLegacy && !all_legacy)
            {
            fprintf(fp,"#if defined(VTK_LEGACY_REMOVE)\n");

            /* avoid warnings if all signatures are legacy and removed */
            if(!is_static)
              {
              fprintf(fp,
                      "  (void)self;"
                      " /* avoid warning if all signatures removed */\n");
              }
            fprintf(fp,
                    "  (void)args;"
                    " /* avoid warning if all signatures removed */\n");
            fprintf(fp,"#else\n");
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
            fprintf(fp,"  if ((PyArg_ParseTuple(args, (char*)\"%s\"",
                    get_format_string());
            }
          else
            {
            fprintf(fp,"  op = (%s *)PyArg_VTKParseTuple(self, args, (char*)\"%s\"",
                    data->ClassName,get_format_string());
            }
          for (i = 0; i < currentFunction->NumberOfArguments; i++)
            {
            if ((currentFunction->ArgTypes[i] % 0x1000 == 0x309)||
                (currentFunction->ArgTypes[i] % 0x1000 == 0x109))
              {
              fprintf(fp,", &tempH%d",i);
              }
            else if (currentFunction->ArgTypes[i] % 0x1000 == 0x302)
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
            fprintf(fp,")))\n    {\n");
            }
          else
            {
            fprintf(fp,");\n");
            fprintf(fp,"  if (op)\n    {\n");
            }

          /* lookup and required objects */
          for (i = 0; i < currentFunction->NumberOfArguments; i++)
            {
            if ((currentFunction->ArgTypes[i] % 0x1000 == 0x309)||
                (currentFunction->ArgTypes[i] % 0x1000 == 0x109))
              {
              fprintf(fp,"    temp%d = (%s *)vtkPythonGetPointerFromObject(tempH%d,(char*)\"%s\");\n",
                      i, currentFunction->ArgClasses[i], i, 
                      currentFunction->ArgClasses[i]);
              fprintf(fp,"    if (!temp%d && tempH%d != Py_None) goto break%d;\n",i,i,occ);
              goto_used = 1;
              }
            }
          
          /* make sure passed method is callable  for VAR functions */
          if (currentFunction->NumberOfArguments == 0x1 &&
              currentFunction->ArgTypes[0] == 0x5000)
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
            if (currentFunction->ArgTypes[i] % 0x1000 == 0x302)
              {
              fprintf(fp,"    temp%i = vtkPythonUnmanglePointer((char *)temp%i,&size%i,(char*)\"%s\");\n",i,i,i,"void_p");
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
                if (currentFunction->IsPureVirtual)
                  {
                  fprintf(fp,"    if (PyVTKClass_Check(self))\n      {\n");
                  fprintf(fp,"      PyErr_SetString(PyExc_TypeError,\"pure virtual method call\");\n");
                  fprintf(fp,"      return NULL;\n    }\n");
                  continue;
                  }
                else
                  {
                  fprintf(fp,"    if (PyVTKClass_Check(self))\n      {\n");
                  sprintf(methodname,"op->%s::%s",
                    data->ClassName,currentFunction->Name);
                  }
                }
              }
            else
              {
              fprintf(fp,"    else\n      {\n");
              sprintf(methodname,"op->%s",currentFunction->Name);
              }
                
            switch (currentFunction->ReturnType % 0x1000)
              {
              case 0x2:
                fprintf(fp,"      %s(",methodname);
                break;
              case 0x109:
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
              if (currentFunction->ArgTypes[i] % 0x1000 == 0x109)
                {
                fprintf(fp,"*(temp%i)",i);
                }
              else if (currentFunction->NumberOfArguments == 1 
                       && currentFunction->ArgTypes[i] == 0x5000)
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
                && currentFunction->ArgTypes[0] == 0x5000)
              {
              fprintf(fp,"      %sArgDelete(vtkPythonVoidFuncArgDelete);\n",
                      methodname);
              }
            fprintf(fp,"      }\n");
            }

          for (i = 0; i < currentFunction->NumberOfArguments; i++)
            {
            if (currentFunction->ArgCounts[i] &&  /* array */
                currentFunction->ArgTypes[i] % 0x10 != 0 && /* not a special type */
                currentFunction->ArgTypes[i] % 0x10 != 0x9 && /* not class pointer */
                currentFunction->ArgTypes[i] % 0x10 != 0x8 && 
                currentFunction->ArgTypes[i] % 0x10 != 0x2 && /* not void pointer */
                (currentFunction->ArgTypes[i] % 0x2000 < 0x1000)) /* not const */
              {
              fprintf(fp,"    if (vtkPythonCheckArray(args,%d,temp%d,%d)) {\n"
                         "      return 0;\n"
                         "      }\n", i, i, currentFunction->ArgCounts[i]);
              }
            }
          do_return(fp);
          fprintf(fp,"    }\n  }\n");
          if (goto_used) 
            {
            fprintf(fp," break%d:\n",occ);
            }
          if(currentFunction->IsLegacy && !all_legacy)
            {
            fprintf(fp,"#endif\n");
            }
          }
        }
      fprintf(fp,"  return NULL;\n}\n");
      if(all_legacy)
        {
        fprintf(fp,"#endif\n");
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
  fprintf(fp,"static PyMethodDef Py%sMethods[] = {\n",data->ClassName);
  
  for (fnum = 0; fnum < numberOfWrappedFunctions; fnum++)
    {
    if(wrappedFunctions[fnum]->IsLegacy)
      {
      fprintf(fp,"#if !defined(VTK_LEGACY_REMOVE)\n");
      }
    if (wrappedFunctions[fnum]->Name)
      {
      fprintf(fp,"  {(char*)\"%s\",                (PyCFunction)Py%s_%s, 1,\n   (char*)\"%s\\n\\n%s\"},\n",
              wrappedFunctions[fnum]->Name, data->ClassName, 
              wrappedFunctions[fnum]->Name, wrappedFunctions[fnum]->Signature,
              quote_string(wrappedFunctions[fnum]->Comment,1000));
      }
    if(wrappedFunctions[fnum]->IsLegacy)
      {
      fprintf(fp,"#endif\n");
      }
    }
  
  if (!strcmp("vtkObject",data->ClassName))
    {
    fprintf(fp,"  {(char*)\"AddObserver\",  (PyCFunction)Py%s_AddObserver, 1,\n   (char*)\"V.AddObserver(int, function) -> int\\n\\n Add an event callback function(vtkObject, int) for an event type.\\n Returns a handle that can be used with RemoveEvent(int).\"},\n", data->ClassName);
    }
  else if (!strcmp("vtkObjectBase",data->ClassName))
    {
    fprintf(fp,"  {(char*)\"GetAddressAsString\",  (PyCFunction)Py%s_GetAddressAsString, 1,\n   (char*)\"V.GetAddressAsString(string) -> string\\n\\n Get address of C++ object in format 'Addr=%%p' after casting to\\n the specified type.  You can get the same information from V.__this__.\"},\n", data->ClassName);
    fprintf(fp,"  {(char*)\"PrintRevisions\",  (PyCFunction)Py%s_PrintRevisions, 1,\n   (char*)\"V.PrintRevisions() -> string\\n\\n Prints the .cxx file CVS revisions of the classes in the\\n object's inheritance chain.\"},\n", data->ClassName);
    }
  
  fprintf(fp,"  {NULL,                       NULL, 0, NULL}\n};\n\n");
}



void outputFunction(FILE *fp, FileInfo *data)
{
  int i;
  int args_ok = 1;
 
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
  
  /* check to see if we can handle the args */
  for (i = 0; i < currentFunction->NumberOfArguments; i++)
    {
    if (currentFunction->ArgTypes[i] % 0x1000 == 9) args_ok = 0;
    if ((currentFunction->ArgTypes[i] % 0x10) == 8) args_ok = 0;
    if (((currentFunction->ArgTypes[i] % 0x1000)/0x100 != 0x3)&&
        (currentFunction->ArgTypes[i] % 0x1000 != 0x109)&&
        ((currentFunction->ArgTypes[i] % 0x1000)/0x100)) args_ok = 0;
    if (currentFunction->ArgTypes[i] % 0x1000 == 0x313) args_ok = 0;
    if (currentFunction->ArgTypes[i] % 0x1000 == 0x314) args_ok = 0;
    if (currentFunction->ArgTypes[i] % 0x1000 == 0x31A) args_ok = 0;
    if (currentFunction->ArgTypes[i] % 0x1000 == 0x31B) args_ok = 0;
    if (currentFunction->ArgTypes[i] % 0x1000 == 0x31C) args_ok = 0;
    if (currentFunction->ArgTypes[i] % 0x1000 == 0x315) args_ok = 0;
    if (currentFunction->ArgTypes[i] % 0x1000 == 0x316) args_ok = 0;
    }
  if ((currentFunction->ReturnType % 0x10) == 0x8) args_ok = 0;
  if (currentFunction->ReturnType % 0x1000 == 0x9) args_ok = 0;
  if (((currentFunction->ReturnType % 0x1000)/0x100 != 0x3)&&
      (currentFunction->ReturnType % 0x1000 != 0x109)&&
      ((currentFunction->ReturnType % 0x1000)/0x100)) args_ok = 0;


  /* eliminate unsigned char * and unsigned short * */
  if (currentFunction->ReturnType % 0x1000 == 0x313) args_ok = 0;
  if (currentFunction->ReturnType % 0x1000 == 0x314) args_ok = 0;
  if (currentFunction->ReturnType % 0x1000 == 0x31A) args_ok = 0;
  if (currentFunction->ReturnType % 0x1000 == 0x31B) args_ok = 0;
  if (currentFunction->ReturnType % 0x1000 == 0x31C) args_ok = 0;
  if (currentFunction->ReturnType % 0x1000 == 0x315) args_ok = 0;
  if (currentFunction->ReturnType % 0x1000 == 0x316) args_ok = 0;

  if (currentFunction->NumberOfArguments && 
      (currentFunction->ArgTypes[0] == 0x5000)
      &&(currentFunction->NumberOfArguments != 0x1)) args_ok = 0;

  /* make sure we have all the info we need for array arguments in */
  for (i = 0; i < currentFunction->NumberOfArguments; i++)
    {
    if (((currentFunction->ArgTypes[i] % 0x1000)/0x100 == 0x3)&&
        (currentFunction->ArgCounts[i] <= 0)&&
        (currentFunction->ArgTypes[i] % 0x1000 != 0x309)&&
        (currentFunction->ArgTypes[i] % 0x1000 != 0x303)&&
        (currentFunction->ArgTypes[i] % 0x1000 != 0x302)) args_ok = 0;
    }

  /* if we need a return type hint make sure we have one */
  switch (currentFunction->ReturnType % 0x1000)
    {
    case 0x301: case 0x307: case 0x30A: case 0x30B: case 0x30C: case 0x30D: case 0x30E:
    case 0x304: case 0x305: case 0x306:
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
    fprintf(fp,"  (char*)\"%s\\n\\n\",\n",quote_string(text,500));
    }
  else
    {
    fprintf(fp,"  (char*)\"%s - no description provided.\\n\\n\",\n",
            quote_string(data->ClassName,500));
    }

  if (data->NumberOfSuperClasses > 0)
    {
    fprintf(fp,"  \"Super Class:\\n\\n %s\\n\\n\",\n",
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
        fprintf(fp,"  (char*)\"%s\",\n",quote_string(temp,500));
        }
      else
        { /* just for the last time */
        fprintf(fp,"  (char*)\"%s\\n\",\n",quote_string(temp,500));
        }
      }
    }
  else
    {
    fprintf(fp,"  (char*)\"%s\\n\",\n", "None provided.\\n");
    }

  if (data->Caveats)
    {
    fprintf(fp,"  \"Caveats:\\n\\n");
    fprintf(fp,"%s\\n\",\n", quote_string(data->Caveats,500));
    }

  if (data->SeeAlso)
    {
    char *sdup, *tok;
    
    fprintf(fp,"  \"See Also:\\n\\n");
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

  fprintf(fp,"  NULL\n");
}

/* print the parsed structures */
void vtkParseOutput(FILE *fp, FileInfo *data)
{
  int i;
  
  fprintf(fp,"// python wrapper for %s object\n//\n",data->ClassName);
  fprintf(fp,"#define VTK_WRAPPING_CXX\n");
  if (strcmp("vtkObjectBase",data->ClassName) != 0)
    {
    /* Block inclusion of full streams.  */
    fprintf(fp,"#define VTK_STREAMS_FWD_ONLY\n");
    }
  #if !defined(__APPLE__)
  fprintf(fp,"#include \"vtkPython.h\"\n");
  fprintf(fp,"#undef _XOPEN_SOURCE /* Conflicts with standards.h.  */\n");
  fprintf(fp,"#undef _THREAD_SAFE /* Conflicts with pthread.h.  */\n");
  #endif
  fprintf(fp,"#include \"vtkPythonUtil.h\"\n");
  fprintf(fp,"#include <vtksys/ios/sstream>\n");
  fprintf(fp,"#include \"%s.h\"\n",data->ClassName);

  fprintf(fp,"#if defined(WIN32)\n");
  fprintf(fp,"extern \"C\" { __declspec( dllexport ) PyObject *PyVTKClass_%sNew(char *); }\n",
          data->ClassName);
  fprintf(fp,"#else\n");
  fprintf(fp,"extern \"C\" { PyObject *PyVTKClass_%sNew(char *); }\n",
          data->ClassName);
  fprintf(fp,"#endif\n\n");
  for (i = 0; i < data->NumberOfSuperClasses; i++)
    {
    fprintf(fp,"extern \"C\" { PyObject *PyVTKClass_%sNew(char *); }\n",
            data->SuperClasses[i]);
    }
  
  if (!strcmp("vtkObject",data->ClassName))
    {
    /* Add the AddObserver method to vtkObject. */
    fprintf(fp,"static PyObject *PyvtkObject_AddObserver(PyObject *self, PyObject *args)\n");
    fprintf(fp,"{\n");
    fprintf(fp,"  vtkObject *op;\n");
    fprintf(fp,"  char *temp0;\n");
    fprintf(fp,"  PyObject *temp1;\n");
    fprintf(fp,"  float temp2;\n");
    fprintf(fp,"  unsigned long     temp20 = 0;\n");
    fprintf(fp,"  op = (vtkObject *)PyArg_VTKParseTuple(self, args, (char*)\"zO\", &temp0, &temp1);\n");
    fprintf(fp,"  if (op)\n");
    fprintf(fp,"    {\n");
    fprintf(fp,"    if (!PyCallable_Check(temp1) && temp1 != Py_None)\n");
    fprintf(fp,"      {\n");
    fprintf(fp,"      PyErr_SetString(PyExc_ValueError,\"vtk callback method passed to AddObserver was not callable.\");\n");
    fprintf(fp,"      return NULL;\n");
    fprintf(fp,"      }\n");
    fprintf(fp,"    Py_INCREF(temp1);\n");
    fprintf(fp,"    vtkPythonCommand *cbc = vtkPythonCommand::New();\n");
    fprintf(fp,"    cbc->SetObject(temp1);\n");
    fprintf(fp,"    cbc->SetThreadState(PyThreadState_Get());\n");
    fprintf(fp,"    temp20 = op->AddObserver(temp0,cbc);\n");
    fprintf(fp,"    cbc->Delete();\n");
    fprintf(fp,"    return PyInt_FromLong((long)temp20);\n");
    fprintf(fp,"    }\n");
    fprintf(fp,"  PyErr_Clear();\n");
    fprintf(fp,"  op = (vtkObject *)PyArg_VTKParseTuple(self, args, (char*)\"zOf\", &temp0, &temp1, &temp2);\n");
    fprintf(fp,"  if (op)\n");
    fprintf(fp,"    {\n");
    fprintf(fp,"    if (!PyCallable_Check(temp1) && temp1 != Py_None)\n");
    fprintf(fp,"      {\n");
    fprintf(fp,"      PyErr_SetString(PyExc_ValueError,\"vtk callback method passed to AddObserver was not callable.\");\n");
    fprintf(fp,"      return NULL;\n");
    fprintf(fp,"      }\n");
    fprintf(fp,"    Py_INCREF(temp1);\n");
    fprintf(fp,"    vtkPythonCommand *cbc = vtkPythonCommand::New();\n");
    fprintf(fp,"    cbc->SetObject(temp1);\n");
    fprintf(fp,"    cbc->SetThreadState(PyThreadState_Get());\n");
    fprintf(fp,"    temp20 = op->AddObserver(temp0,cbc,temp2);\n");
    fprintf(fp,"    cbc->Delete();\n");
    fprintf(fp,"    return PyInt_FromLong((long)temp20);\n");
    fprintf(fp,"    }\n");
    fprintf(fp,"  return NULL;\n");
    fprintf(fp,"}\n\n");
    }

  if (!strcmp("vtkObjectBase",data->ClassName))
    {
    /* while we are at it spit out the GetStringFromObject method */
    fprintf(fp,"PyObject *PyvtkObjectBase_GetAddressAsString(PyObject *self, PyObject *args)\n");
    fprintf(fp,"{\n");

    /* declare the variables */
    fprintf(fp,"  %s *op;\n",data->ClassName);

    /* handle unbound method call if 'self' is a PyVTKClass */
    fprintf(fp,"  char *typecast;\n\n");
    fprintf(fp,"  op = (%s *)PyArg_VTKParseTuple(self, args, (char*)\"s\", &typecast);\n",data->ClassName);
    fprintf(fp,"  if (op)\n");
    fprintf(fp,"    {\n    char temp20[256];\n");
    fprintf(fp,"    sprintf(temp20,\"Addr=%%p\",op);\n");
    fprintf(fp,"    return PyString_FromString(temp20);\n");
    fprintf(fp,"    }\n");
    fprintf(fp,"  return NULL;\n}\n\n");

    /* Add the PrintRevisions method to vtkObjectBase. */
    fprintf(fp,"PyObject *PyvtkObjectBase_PrintRevisions(PyObject *self, PyObject *args)\n");
    fprintf(fp,"{\n");
    fprintf(fp,"  %s *op;\n",data->ClassName);
    fprintf(fp,"  op = (%s *)PyArg_VTKParseTuple(self, args, (char*)\"\");\n",data->ClassName);
    fprintf(fp,"  if (op)\n");
    fprintf(fp,"    {\n");
    fprintf(fp,"    vtksys_ios::ostringstream vtkmsg_with_warning_C4701;\n");
    fprintf(fp,"    op->PrintRevisions(vtkmsg_with_warning_C4701);\n");
    fprintf(fp,"    vtkmsg_with_warning_C4701.put('\\0');\n");
    fprintf(fp,"    PyObject *result = PyString_FromString(vtkmsg_with_warning_C4701.str().c_str());\n");
    fprintf(fp,"    return result;\n");
    fprintf(fp,"    }\n");
    fprintf(fp,"  return NULL;\n}\n\n");

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
    fprintf(fp,"static const char *%sDoc[] = {\n",data->ClassName); 
    create_class_doc(fp,data);
    fprintf(fp,"};\n\n");
    }
  
  /* output the class initilization function */
  if (strcmp(data->ClassName,"vtkObjectBase") == 0)
    { /* special wrapping for vtkObject */
    if (class_has_new)
      {
      fprintf(fp,"static vtkObjectBase *%sStaticNew()\n",data->ClassName);
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
    fprintf(fp,"                        (char*)\"%s\",modulename,\n",data->ClassName);
    fprintf(fp,"                        (char**)%sDoc,0);\n}\n\n",data->ClassName);
    }
  else if (data->NumberOfSuperClasses)
    { /* wrapping of descendants of vtkObjectBase */
    if (class_has_new)
      {
      fprintf(fp,"static vtkObjectBase *%sStaticNew()\n",data->ClassName);
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
    fprintf(fp,"                        (char*)\"%s\",modulename,\n",data->ClassName);
    fprintf(fp,"                        (char**)%sDoc,\n",data->ClassName);
    fprintf(fp,"                        PyVTKClass_%sNew(modulename));\n}\n\n",
            data->SuperClasses[0]);
    }
  else if (!data->IsAbstract)
    { /* wrapping of 'special' non-vtkObject classes */
    fprintf(fp,"PyObject *PyVTKObject_%sNew(PyObject *, PyObject *args)\n{\n",data->ClassName);
    fprintf(fp,"  if (!(PyArg_ParseTuple(args, (char*)\"\")))\n    {\n");
    fprintf(fp,"    return NULL;\n    }\n\n");
    fprintf(fp,"  %s *obj = new %s;\n",data->ClassName,data->ClassName);
    fprintf(fp,"  return PyVTKSpecialObject_New(obj, Py%sMethods, (char*)\"%s\",(char**)%sDoc);\n",data->ClassName,data->ClassName,data->ClassName);
    fprintf(fp,"}\n\n");

    fprintf(fp,"static PyMethodDef Py%sNewMethod = \\\n",data->ClassName);
    fprintf(fp,"{ (char*)\"%s\",  (PyCFunction)PyVTKObject_%sNew, 1,\n",
            data->ClassName,data->ClassName);
    fprintf(fp,"  (char*)%sDoc[0] };\n\n",data->ClassName);

    fprintf(fp,"PyObject *PyVTKClass_%sNew(char *)\n{\n",data->ClassName);
    fprintf(fp,"  return PyCFunction_New(&Py%sNewMethod,Py_None);\n}\n\n",
            data->ClassName);
    }
  else
    { /* un-wrappable classes */
    fprintf(fp,"PyObject *PyVTKClass_%sNew(char *)\n{\n",data->ClassName);
    fprintf(fp,"  return NULL;\n}\n\n");
    }
}

