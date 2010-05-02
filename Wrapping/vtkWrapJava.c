/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWrapJava.c

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "vtkParse.h"

int numberOfWrappedFunctions = 0;
FunctionInfo *wrappedFunctions[1000];
extern FunctionInfo *currentFunction;
FileInfo *CurrentData;

void output_proto_vars(FILE *fp, int i)
{
  int aType = (currentFunction->ArgTypes[i] & VTK_PARSE_UNQUALIFIED_TYPE);

  /* ignore void */
  if (aType == VTK_PARSE_VOID)
    {
    return;
    }

  if (currentFunction->ArgTypes[i] == VTK_PARSE_FUNCTION)
    {
    fprintf(fp,"jobject id0, jstring id1");
    return;
    }

  if (aType == VTK_PARSE_CHAR_PTR)
    {
    fprintf(fp,"jstring ");
    fprintf(fp,"id%i",i);
    return;
    }

  if ((aType == VTK_PARSE_FLOAT_PTR) || (aType == VTK_PARSE_DOUBLE_PTR))
    {
    fprintf(fp,"jdoubleArray ");
    fprintf(fp,"id%i",i);
    return;
    }

  if ((aType == VTK_PARSE_INT_PTR) ||
      (aType == VTK_PARSE_LONG_PTR) ||
      (aType == VTK_PARSE_ID_TYPE_PTR) ||
      (aType == VTK_PARSE_LONG_LONG_PTR) ||
      (aType == VTK_PARSE___INT64_PTR))
    {
    fprintf(fp,"jintArray ");
    fprintf(fp,"id%i",i);
    return;
    }


  switch ((aType & VTK_PARSE_BASE_TYPE) & ~VTK_PARSE_UNSIGNED)
    {
    case VTK_PARSE_FLOAT:   fprintf(fp,"jdouble "); break;
    case VTK_PARSE_DOUBLE:   fprintf(fp,"jdouble "); break;
    case VTK_PARSE_INT:   fprintf(fp,"jint "); break;
    case VTK_PARSE_SHORT:   fprintf(fp,"jint "); break;
    case VTK_PARSE_LONG:   fprintf(fp,"jint "); break;
    case VTK_PARSE_ID_TYPE:   fprintf(fp,"jint "); break;
    case VTK_PARSE_LONG_LONG:   fprintf(fp,"jint "); break;
    case VTK_PARSE___INT64:   fprintf(fp,"jint "); break;
    case VTK_PARSE_SIGNED_CHAR:   fprintf(fp,"jint "); break;
    case VTK_PARSE_BOOL:   fprintf(fp,"jboolean "); break;
    case VTK_PARSE_VOID:   fprintf(fp,"void "); break;
    case VTK_PARSE_CHAR:   fprintf(fp,"jchar "); break;
    case VTK_PARSE_VTK_OBJECT:   fprintf(fp,"jobject "); break;
    case VTK_PARSE_UNKNOWN: return;
    }

  fprintf(fp,"id%i",i);
}

/* when the cpp file doesn't have enough info use the hint file */
void use_hints(FILE *fp)
{
  int rType = (currentFunction->ReturnType & VTK_PARSE_UNQUALIFIED_TYPE);

  /* use the hint */
  switch (rType)
    {
    case VTK_PARSE_UNSIGNED_CHAR_PTR:
      /* for vtkDataWriter we want to handle this case specially */
      if (strcmp(currentFunction->Name,"GetBinaryOutputString") ||
          strcmp(CurrentData->ClassName,"vtkDataWriter"))
        {
        fprintf(fp,"    return vtkJavaMakeJArrayOfByteFromUnsignedChar(env,temp%i,%i);\n",
                MAX_ARGS, currentFunction->HintSize);
        }
      else
        {
        fprintf(fp,"    return vtkJavaMakeJArrayOfByteFromUnsignedChar(env,temp%i,op->GetOutputStringLength());\n", MAX_ARGS);
        }
      break;

    case VTK_PARSE_FLOAT_PTR:
      fprintf(fp,"    return vtkJavaMakeJArrayOfDoubleFromFloat(env,temp%i,%i);\n",
              MAX_ARGS, currentFunction->HintSize);
      break;

    case VTK_PARSE_DOUBLE_PTR:
      fprintf(fp,"    return vtkJavaMakeJArrayOfDoubleFromDouble(env,temp%i,%i);\n",
              MAX_ARGS, currentFunction->HintSize);
      break;

    case VTK_PARSE_INT_PTR:
      fprintf(fp,"    return vtkJavaMakeJArrayOfIntFromInt(env,temp%i,%i);\n",
              MAX_ARGS, currentFunction->HintSize);
      break;

    case VTK_PARSE_ID_TYPE_PTR:
      fprintf(fp,"    return vtkJavaMakeJArrayOfIntFromIdType(env,temp%i,%i);\n",
              MAX_ARGS, currentFunction->HintSize);
      break;

    case VTK_PARSE_LONG_LONG_PTR:
      fprintf(fp,"    return vtkJavaMakeJArrayOfIntFromLongLong(env,temp%i,%i);\n",
              MAX_ARGS, currentFunction->HintSize);
      break;

    case VTK_PARSE___INT64_PTR:
      fprintf(fp,"    return vtkJavaMakeJArrayOfIntFrom__Int64(env,temp%i,%i);\n",
              MAX_ARGS, currentFunction->HintSize);
      break;

    case VTK_PARSE_SIGNED_CHAR_PTR:
      fprintf(fp,"    return vtkJavaMakeJArrayOfIntFromSignedChar(env,temp%i,%i);\n",
              MAX_ARGS, currentFunction->HintSize);
      break;

    case VTK_PARSE_BOOL_PTR:
      fprintf(fp,"    return vtkJavaMakeJArrayOfIntFromBool(env,temp%i,%i);\n",
              MAX_ARGS, currentFunction->HintSize);
      break;

    case VTK_PARSE_SHORT_PTR:
              fprintf(fp,"    return vtkJavaMakeJArrayOfShortFromShort(env,temp%i,%i);\n",
              MAX_ARGS, currentFunction->HintSize);
      break;

    case VTK_PARSE_LONG_PTR:
              fprintf(fp,"    return vtkJavaMakeJArrayOfLongFromLong(env,temp%i,%i);\n",
              MAX_ARGS, currentFunction->HintSize);
      break;

    case VTK_PARSE_UNSIGNED_INT_PTR:
    case VTK_PARSE_UNSIGNED_SHORT_PTR:
    case VTK_PARSE_UNSIGNED_LONG_PTR:
    case VTK_PARSE_UNSIGNED_ID_TYPE_PTR:
    case VTK_PARSE_UNSIGNED_LONG_LONG_PTR:
    case VTK_PARSE_UNSIGNED___INT64_PTR:
      break;
    }
}

void return_result(FILE *fp)
{
  int rType = (currentFunction->ReturnType & VTK_PARSE_UNQUALIFIED_TYPE);

  switch (rType)
    {
    case VTK_PARSE_FLOAT:
      fprintf(fp,"jdouble ");
      break;
    case VTK_PARSE_VOID:
      fprintf(fp,"void ");
      break;
    case VTK_PARSE_CHAR:
      fprintf(fp,"jchar ");
      break;
    case VTK_PARSE_DOUBLE:
      fprintf(fp,"jdouble ");
      break;
    case VTK_PARSE_INT:
    case VTK_PARSE_SHORT:
    case VTK_PARSE_LONG:
    case VTK_PARSE_ID_TYPE:
    case VTK_PARSE_LONG_LONG:
    case VTK_PARSE___INT64:
    case VTK_PARSE_SIGNED_CHAR:
    case VTK_PARSE_UNSIGNED_CHAR:
    case VTK_PARSE_UNSIGNED_INT:
    case VTK_PARSE_UNSIGNED_SHORT:
    case VTK_PARSE_UNSIGNED_LONG:
    case VTK_PARSE_UNSIGNED_ID_TYPE:
    case VTK_PARSE_UNSIGNED_LONG_LONG:
    case VTK_PARSE_UNSIGNED___INT64:
      fprintf(fp,"jint ");
      break;
    case VTK_PARSE_BOOL:
      fprintf(fp,"jboolean ");
      break;
    case VTK_PARSE_CHAR_PTR: fprintf(fp,"jstring "); break;
    case VTK_PARSE_VTK_OBJECT_PTR:
      fprintf(fp,"jlong "); break;

    case VTK_PARSE_FLOAT_PTR:
    case VTK_PARSE_DOUBLE_PTR:
    case VTK_PARSE_UNSIGNED_CHAR_PTR:
    case VTK_PARSE_INT_PTR:
    case VTK_PARSE_SHORT_PTR:
    case VTK_PARSE_LONG_PTR:
    case VTK_PARSE_ID_TYPE_PTR:
    case VTK_PARSE_LONG_LONG_PTR:
    case VTK_PARSE___INT64_PTR:
    case VTK_PARSE_SIGNED_CHAR_PTR:
    case VTK_PARSE_BOOL_PTR:
    case VTK_PARSE_UNSIGNED_ID_TYPE_PTR:
    case VTK_PARSE_UNSIGNED_LONG_LONG_PTR:
    case VTK_PARSE_UNSIGNED___INT64_PTR:
      fprintf(fp,"jarray "); break;
    }
}


void output_temp(FILE *fp, int i, int aType, char *Id, int aCount)
{
  /* handle VAR FUNCTIONS */
  if (aType == VTK_PARSE_FUNCTION)
    {
    fprintf(fp,"  vtkJavaVoidFuncArg *temp%i = new vtkJavaVoidFuncArg;\n",i);
    return;
    }

  /* ignore void */
  if ((aType & VTK_PARSE_UNQUALIFIED_TYPE) == VTK_PARSE_VOID)
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
    fprintf(fp," unsigned ");
    }

  switch ((aType & VTK_PARSE_BASE_TYPE) & ~VTK_PARSE_UNSIGNED)
    {
    case VTK_PARSE_FLOAT:   fprintf(fp,"float  "); break;
    case VTK_PARSE_DOUBLE:   fprintf(fp,"double "); break;
    case VTK_PARSE_INT:   fprintf(fp,"int    "); break;
    case VTK_PARSE_SHORT:   fprintf(fp,"short  "); break;
    case VTK_PARSE_LONG:   fprintf(fp,"long   "); break;
    case VTK_PARSE_VOID:     fprintf(fp,"void   "); break;
    case VTK_PARSE_CHAR:     fprintf(fp,"char   "); break;
    case VTK_PARSE_ID_TYPE:   fprintf(fp,"vtkIdType "); break;
    case VTK_PARSE_LONG_LONG:   fprintf(fp,"long long "); break;
    case VTK_PARSE___INT64:   fprintf(fp,"__int64 "); break;
    case VTK_PARSE_SIGNED_CHAR:     fprintf(fp,"signed char "); break;
    case VTK_PARSE_BOOL:     fprintf(fp,"bool "); break;
    case VTK_PARSE_VTK_OBJECT:     fprintf(fp,"%s ",Id); break;
    case VTK_PARSE_UNKNOWN: return;
    }

  switch (aType & VTK_PARSE_INDIRECT)
    {
    case VTK_PARSE_REF:
      fprintf(fp, " *"); /* act " &" */
      break;
    case VTK_PARSE_POINTER:
      if ((i == MAX_ARGS) ||
          ((aType & VTK_PARSE_UNQUALIFIED_TYPE) == VTK_PARSE_VTK_OBJECT_PTR) ||
          ((aType & VTK_PARSE_UNQUALIFIED_TYPE) == VTK_PARSE_CHAR_PTR))
        {
        fprintf(fp, " *");
        }
      break;
    case VTK_PARSE_POINTER_REF:
      fprintf(fp, "*&");
      break;
    case VTK_PARSE_POINTER_POINTER:
      fprintf(fp, "**");
      break;
    default:
      fprintf(fp,"  ");
      break;
    }
  fprintf(fp,"temp%i",i);

  /* handle arrays */
  if (((aType & VTK_PARSE_INDIRECT) == VTK_PARSE_POINTER)&&
      (i != MAX_ARGS) &&
      ((aType & VTK_PARSE_UNQUALIFIED_TYPE) != VTK_PARSE_VTK_OBJECT_PTR) &&
      ((aType & VTK_PARSE_UNQUALIFIED_TYPE) != VTK_PARSE_CHAR_PTR))
    {
    fprintf(fp,"[%i]",aCount);
    fprintf(fp,";\n  void *tempArray%i",i);
    }

  fprintf(fp,";\n");
}

void get_args(FILE *fp, int i)
{
  int aType = (currentFunction->ArgTypes[i] & VTK_PARSE_UNQUALIFIED_TYPE);
  int j;

  /* handle VAR FUNCTIONS */
  if (currentFunction->ArgTypes[i] == VTK_PARSE_FUNCTION)
    {
    fprintf(fp,"  env->GetJavaVM(&(temp%i->vm));\n",i);
    fprintf(fp,"  temp%i->uobj = env->NewGlobalRef(id0);\n",i);
    fprintf(fp,"  char *temp%i_str;\n",i);
    fprintf(fp,"  temp%i_str = vtkJavaUTFToChar(env,id1);\n",i);
    fprintf(fp,"  temp%i->mid = env->GetMethodID(env->GetObjectClass(id0),temp%i_str,\"()V\");\n",i,i);
    return;
    }

  /* ignore void */
  if (aType == VTK_PARSE_VOID)
    {
    return;
    }

  switch (aType)
    {
    case VTK_PARSE_CHAR:
      fprintf(fp,"  temp%i = (char)(0xff & id%i);\n",i,i);
      break;
    case VTK_PARSE_BOOL:
      fprintf(fp,"  temp%i = (id%i != 0) ? true : false;\n",i,i);
      break;
    case VTK_PARSE_CHAR_PTR:
      fprintf(fp,"  temp%i = vtkJavaUTFToChar(env,id%i);\n",i,i);
      break;
    case VTK_PARSE_VTK_OBJECT_PTR:
      fprintf(fp,"  temp%i = (%s *)(vtkJavaGetPointerFromObject(env,id%i));\n",i,currentFunction->ArgClasses[i],i);
      break;
    case VTK_PARSE_FLOAT_PTR:
    case VTK_PARSE_DOUBLE_PTR:
      fprintf(fp,"  tempArray%i = (void *)(env->GetDoubleArrayElements(id%i,NULL));\n",i,i);
      for (j = 0; j < currentFunction->ArgCounts[i]; j++)
        {
        fprintf(fp,"  temp%i[%i] = ((jdouble *)tempArray%i)[%i];\n",i,j,i,j);
        }
      break;
    case VTK_PARSE_INT_PTR:
    case VTK_PARSE_LONG_PTR:
    case VTK_PARSE_ID_TYPE_PTR:
    case VTK_PARSE_LONG_LONG_PTR:
    case VTK_PARSE___INT64_PTR:
    case VTK_PARSE_SIGNED_CHAR_PTR:
    case VTK_PARSE_BOOL_PTR:
      fprintf(fp,"  tempArray%i = (void *)(env->GetIntArrayElements(id%i,NULL));\n",i,i);
      for (j = 0; j < currentFunction->ArgCounts[i]; j++)
        {
        fprintf(fp,"  temp%i[%i] = ((jint *)tempArray%i)[%i];\n",i,j,i,j);
        }
      break;
    case VTK_PARSE_VOID:
    case VTK_PARSE_VTK_OBJECT:
    case VTK_PARSE_VTK_OBJECT_REF: break;
    default: fprintf(fp,"  temp%i = id%i;\n",i,i); break;
    }
}


void copy_and_release_args(FILE *fp, int i)
{
  int aType = (currentFunction->ArgTypes[i] & VTK_PARSE_UNQUALIFIED_TYPE);
  int j;

  /* handle VAR FUNCTIONS */
  if (currentFunction->ArgTypes[i] == VTK_PARSE_FUNCTION)
    {
    fprintf(fp,"  if (temp%i_str) delete[] temp%i_str;\n",i,i);
    return;
    }

  /* ignore void */
  if (aType == VTK_PARSE_VOID)
    {
    return;
    }

  switch (aType)
    {
    case VTK_PARSE_FLOAT_PTR:
    case VTK_PARSE_DOUBLE_PTR:
      for (j = 0; j < currentFunction->ArgCounts[i]; j++)
        {
        fprintf(fp,"  ((jdouble *)tempArray%i)[%i] = temp%i[%i];\n",i,j,i,j);
        }
      fprintf(fp,"  env->ReleaseDoubleArrayElements(id%i,(jdouble *)tempArray%i,0);\n",i,i);
      break;
    case VTK_PARSE_CHAR_PTR:
      fprintf(fp,"  if (temp%i) delete[] temp%i;\n",i,i);
      break;
    case VTK_PARSE_INT_PTR:
    case VTK_PARSE_LONG_PTR:
    case VTK_PARSE_ID_TYPE_PTR:
    case VTK_PARSE_LONG_LONG_PTR:
    case VTK_PARSE___INT64_PTR:
    case VTK_PARSE_SIGNED_CHAR_PTR:
    case VTK_PARSE_BOOL_PTR:
      for (j = 0; j < currentFunction->ArgCounts[i]; j++)
        {
        fprintf(fp,"  ((jint *)tempArray%i)[%i] = temp%i[%i];\n",i,j,i,j);
        }
      fprintf(fp,"  env->ReleaseIntArrayElements(id%i,(jint *)tempArray%i,0);\n",i,i);
      break;
    default:
      break;
    }
}

void do_return(FILE *fp)
{
  int rType = (currentFunction->ReturnType & VTK_PARSE_UNQUALIFIED_TYPE);

  /* ignore void */
  if (rType == VTK_PARSE_VOID)
    {
    return;
    }

  switch (rType)
    {
    case VTK_PARSE_CHAR_PTR:
      {
      fprintf(fp,"  return vtkJavaMakeJavaString(env,temp%i);\n",
              MAX_ARGS);
      break;
      }
    case VTK_PARSE_VTK_OBJECT_PTR:
      {
      fprintf(fp,"  return (jlong)(size_t)temp%i;", MAX_ARGS);
      break;
      }

    /* handle functions returning vectors */
    /* this is done by looking them up in a hint file */
    case VTK_PARSE_FLOAT_PTR:
    case VTK_PARSE_DOUBLE_PTR:
    case VTK_PARSE_UNSIGNED_CHAR_PTR:
    case VTK_PARSE_INT_PTR:
    case VTK_PARSE_SHORT_PTR:
    case VTK_PARSE_LONG_PTR:
    case VTK_PARSE_ID_TYPE_PTR:
    case VTK_PARSE_LONG_LONG_PTR:
    case VTK_PARSE___INT64_PTR:
    case VTK_PARSE_SIGNED_CHAR_PTR:
    case VTK_PARSE_BOOL_PTR:
      use_hints(fp);
      break;
    default: fprintf(fp,"  return temp%i;\n", MAX_ARGS); break;
    }
}

/* have we done one of these yet */
int DoneOne()
{
  int aType = 0;
  int fType = 0;
  int rType = 0;
  int qType = 0;
  int i,j;
  int match;
  FunctionInfo *fi;

  for (i = 0; i < numberOfWrappedFunctions; i++)
    {
    fi = wrappedFunctions[i];
    rType = (currentFunction->ReturnType & VTK_PARSE_UNQUALIFIED_TYPE);
    qType = (fi->ReturnType & VTK_PARSE_UNQUALIFIED_TYPE);

    if ((!strcmp(fi->Name,currentFunction->Name))
        &&(fi->NumberOfArguments == currentFunction->NumberOfArguments))
      {
      match = 1;
      for (j = 0; j < fi->NumberOfArguments; j++)
        {
        aType = (currentFunction->ArgTypes[j] & VTK_PARSE_UNQUALIFIED_TYPE);
        fType = (fi->ArgTypes[j] & VTK_PARSE_UNQUALIFIED_TYPE);

        if ((fi->ArgTypes[j] != currentFunction->ArgTypes[j]) &&
            !(((fType == VTK_PARSE_FLOAT_PTR)&&
               (aType == VTK_PARSE_DOUBLE_PTR)) ||
              ((fType == VTK_PARSE_DOUBLE_PTR)&&
               (aType == VTK_PARSE_FLOAT_PTR)) ||
              ((fType == VTK_PARSE_INT_PTR)&&
               (aType == VTK_PARSE_LONG_PTR)) ||
              ((fType == VTK_PARSE_LONG_PTR)&&
               (aType == VTK_PARSE_INT_PTR)) ||
              ((fType == VTK_PARSE_ID_TYPE_PTR)&&
               (aType == VTK_PARSE_INT_PTR)) ||
              ((fType == VTK_PARSE_INT_PTR)&&
               (aType == VTK_PARSE_ID_TYPE_PTR)) ||
              ((fType == VTK_PARSE_ID_TYPE_PTR)&&
               (aType == VTK_PARSE_LONG_PTR)) ||
              ((fType == VTK_PARSE_LONG_PTR)&&
               (aType == VTK_PARSE_ID_TYPE_PTR)) ||
              ((fType == VTK_PARSE_LONG_LONG_PTR)&&
               (aType == VTK_PARSE_INT_PTR)) ||
              ((fType == VTK_PARSE_INT_PTR)&&
               (aType == VTK_PARSE_LONG_LONG_PTR)) ||
              ((fType == VTK_PARSE_LONG_LONG_PTR)&&
               (aType == VTK_PARSE_LONG_PTR)) ||
              ((fType == VTK_PARSE_LONG_PTR)&&
               (aType == VTK_PARSE_LONG_LONG_PTR)) ||
              ((fType == VTK_PARSE___INT64_PTR)&&
               (aType == VTK_PARSE_INT_PTR)) ||
              ((fType == VTK_PARSE_INT_PTR)&&
               (aType == VTK_PARSE___INT64_PTR)) ||
              ((fType == VTK_PARSE___INT64_PTR)&&
               (aType == VTK_PARSE_LONG_PTR)) ||
              ((fType == VTK_PARSE_LONG_PTR)&&
               (aType == VTK_PARSE___INT64_PTR)) ||
              ((fType == VTK_PARSE_FLOAT)&&
               (aType == VTK_PARSE_DOUBLE)) ||
              ((fType == VTK_PARSE_DOUBLE)&&
               (aType == VTK_PARSE_FLOAT)) ||
              ((fType == VTK_PARSE_INT)&&
               (aType == VTK_PARSE_LONG)) ||
              ((fType == VTK_PARSE_LONG)&&
               (aType == VTK_PARSE_INT)) ||
              ((fType == VTK_PARSE_INT)&&
               (aType == VTK_PARSE_ID_TYPE)) ||
              ((fType == VTK_PARSE_ID_TYPE)&&
               (aType == VTK_PARSE_INT)) ||
              ((fType == VTK_PARSE_ID_TYPE)&&
               (aType == VTK_PARSE_LONG)) ||
              ((fType == VTK_PARSE_LONG)&&
               (aType == VTK_PARSE_ID_TYPE)) ||
              ((fType == VTK_PARSE_INT)&&
               (aType == VTK_PARSE_LONG_LONG)) ||
              ((fType == VTK_PARSE_LONG_LONG)&&
               (aType == VTK_PARSE_INT)) ||
              ((fType == VTK_PARSE_LONG_LONG)&&
               (aType == VTK_PARSE_LONG)) ||
              ((fType == VTK_PARSE_LONG)&&
               (aType == VTK_PARSE_LONG_LONG)) ||
              ((fType == VTK_PARSE_INT)&&
               (aType == VTK_PARSE___INT64)) ||
              ((fType == VTK_PARSE___INT64)&&
               (aType == VTK_PARSE_INT)) ||
              ((fType == VTK_PARSE___INT64)&&
               (aType == VTK_PARSE_LONG)) ||
              ((fType == VTK_PARSE_LONG)&&
               (aType == VTK_PARSE___INT64))))
          {
          match = 0;
          }
        else
          {
          if (fType == VTK_PARSE_VTK_OBJECT_PTR)
            {
            if (strcmp(fi->ArgClasses[j],currentFunction->ArgClasses[j]))
              {
              match = 0;
              }
            }
          }
        }
      if ((fi->ReturnType != currentFunction->ReturnType) &&
          !(((qType == VTK_PARSE_FLOAT_PTR)&&
             (rType == VTK_PARSE_DOUBLE_PTR)) ||
            ((qType == VTK_PARSE_DOUBLE_PTR)&&
             (rType == VTK_PARSE_FLOAT_PTR)) ||
            ((qType == VTK_PARSE_INT_PTR)&&
             (rType == VTK_PARSE_LONG_PTR)) ||
            ((qType == VTK_PARSE_LONG_PTR)&&
             (rType == VTK_PARSE_INT_PTR)) ||
            ((qType == VTK_PARSE_INT_PTR)&&
             (rType == VTK_PARSE_ID_TYPE_PTR)) ||
            ((qType == VTK_PARSE_ID_TYPE_PTR)&&
             (rType == VTK_PARSE_INT_PTR)) ||
            ((qType == VTK_PARSE_LONG_PTR)&&
             (rType == VTK_PARSE_ID_TYPE_PTR)) ||
            ((qType == VTK_PARSE_ID_TYPE_PTR)&&
             (rType == VTK_PARSE_LONG_PTR)) ||
            ((qType == VTK_PARSE_INT_PTR)&&
             (rType == VTK_PARSE_LONG_LONG_PTR)) ||
            ((qType == VTK_PARSE_LONG_LONG_PTR)&&
             (rType == VTK_PARSE_INT_PTR)) ||
            ((qType == VTK_PARSE_LONG_PTR)&&
             (rType == VTK_PARSE_LONG_LONG_PTR)) ||
            ((qType == VTK_PARSE_LONG_LONG_PTR)&&
             (rType == VTK_PARSE_LONG_PTR)) ||
            ((qType == VTK_PARSE_INT_PTR)&&
             (rType == VTK_PARSE___INT64_PTR)) ||
            ((qType == VTK_PARSE___INT64_PTR)&&
             (rType == VTK_PARSE_INT_PTR)) ||
            ((qType == VTK_PARSE_LONG_PTR)&&
             (rType == VTK_PARSE___INT64_PTR)) ||
            ((qType == VTK_PARSE___INT64_PTR)&&
             (rType == VTK_PARSE_LONG_PTR)) ||
            ((qType == VTK_PARSE_FLOAT)&&
             (rType == VTK_PARSE_DOUBLE)) ||
            ((qType == VTK_PARSE_DOUBLE)&&
             (rType == VTK_PARSE_FLOAT)) ||
            ((qType == VTK_PARSE_INT)&&
             (rType == VTK_PARSE_LONG)) ||
            ((qType == VTK_PARSE_LONG)&&
             (rType == VTK_PARSE_INT)) ||
            ((qType == VTK_PARSE_ID_TYPE)&&
             (rType == VTK_PARSE_LONG)) ||
            ((qType == VTK_PARSE_LONG)&&
             (rType == VTK_PARSE_ID_TYPE)) ||
            ((qType == VTK_PARSE_INT)&&
             (rType == VTK_PARSE_ID_TYPE)) ||
            ((qType == VTK_PARSE_ID_TYPE)&&
             (rType == VTK_PARSE_INT)) ||
            ((qType == VTK_PARSE_LONG_LONG)&&
             (rType == VTK_PARSE_LONG)) ||
            ((qType == VTK_PARSE_LONG)&&
             (rType == VTK_PARSE_LONG_LONG)) ||
            ((qType == VTK_PARSE_INT)&&
             (rType == VTK_PARSE_LONG_LONG)) ||
            ((qType == VTK_PARSE_LONG_LONG)&&
             (rType == VTK_PARSE_INT)) ||
            ((qType == VTK_PARSE___INT64)&&
             (rType == VTK_PARSE_LONG)) ||
            ((qType == VTK_PARSE_LONG)&&
             (rType == VTK_PARSE___INT64)) ||
            ((qType == VTK_PARSE_INT)&&
             (rType == VTK_PARSE___INT64)) ||
            ((qType == VTK_PARSE___INT64)&&
             (rType == VTK_PARSE_INT))))
        {
        match = 0;
        }
      else
        {
        if (rType == VTK_PARSE_VTK_OBJECT_PTR)
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

void HandleDataReader(FILE *fp, FileInfo *data)
{
    fprintf(fp,"\n");
    fprintf(fp,"extern \"C\" JNIEXPORT void");
    fprintf(fp," JNICALL Java_vtk_%s_%s_1%i(JNIEnv *env, jobject obj, jbyteArray id0, jint id1)\n",
            data->ClassName,currentFunction->Name, numberOfWrappedFunctions);
    fprintf(fp,"{\n");
    fprintf(fp,"  %s *op;\n",data->ClassName);
    fprintf(fp,"  op = (%s *)vtkJavaGetPointerFromObject(env,obj);\n",
            data->ClassName);
    fprintf(fp,"  jboolean isCopy;\n");
    fprintf(fp,"  jbyte *data = env->GetByteArrayElements(id0,&isCopy);\n");
    fprintf(fp,"  op->SetBinaryInputString((const char *)data,id1);\n");
    fprintf(fp,"  env->ReleaseByteArrayElements(id0,data,JNI_ABORT);\n");
    fprintf(fp,"}\n");
}

void HandleDataArray(FILE *fp, FileInfo *data)
{
  const char *type = 0;
  const char *jtype = 0;
  const char *fromtype = 0;
  const char *jfromtype = 0;

  if (!strcmp("vtkCharArray",data->ClassName) )
    {
    type = "char";
    fromtype = "Char";
    jtype = "byte";
    jfromtype = "Byte";
    }
  else if (!strcmp("vtkDoubleArray",data->ClassName) )
    {
    type = "double";
    fromtype = "Double";
    jtype = type;
    jfromtype = fromtype;
    }
  else if (!strcmp("vtkFloatArray",data->ClassName) )
    {
    type = "float";
    fromtype = "Float";
    jtype = type;
    jfromtype = fromtype;
    }
  else if (!strcmp("vtkIntArray",data->ClassName) )
    {
    type = "int";
    fromtype = "Int";
    jtype = type;
    jfromtype = fromtype;
    }
  else if (!strcmp("vtkLongArray",data->ClassName) )
    {
    type = "long";
    fromtype = "Long";
    jtype = type;
    jfromtype = fromtype;
    }
  else if (!strcmp("vtkShortArray",data->ClassName) )
    {
    type = "short";
    fromtype = "Short";
    jtype = type;
    jfromtype = fromtype;
    }
  else if (!strcmp("vtkUnsignedCharArray",data->ClassName) )
    {
    type = "unsigned char";
    fromtype = "UnsignedChar";
    jtype = "byte";
    jfromtype = "Byte";
    }
  else if (!strcmp("vtkUnsignedIntArray",data->ClassName) )
    {
    type = "unsigned int";
    fromtype = "UnsignedInt";
    jtype = "int";
    jfromtype = "Int";
    }
  else if (!strcmp("vtkUnsignedLongArray",data->ClassName) )
    {
    type = "unsigned long";
    fromtype = "UnsignedLong";
    jtype = "long";
    jfromtype = "Long";
    }
  else if (!strcmp("vtkUnsignedShortArray",data->ClassName) )
    {
    type = "unsigned short";
    fromtype = "UnsignedShort";
    jtype = "short";
    jfromtype = "Short";
    }
  else
    {
    return;
    }

  fprintf(fp,"// Array conversion routines\n");
  fprintf(fp,"extern \"C\" JNIEXPORT jarray JNICALL Java_vtk_%s_GetJavaArray_10("
    "JNIEnv *env, jobject obj)\n",
    data->ClassName);
  fprintf(fp,"{\n");
  fprintf(fp,"  %s *op;\n", data->ClassName);
  fprintf(fp,"  %s  *temp20;\n", type);
  fprintf(fp,"  vtkIdType size;\n");
  fprintf(fp,"\n");
  fprintf(fp,"  op = (%s *)vtkJavaGetPointerFromObject(env,obj);\n",
    data->ClassName);
  fprintf(fp,"  temp20 = static_cast<%s*>(op->GetVoidPointer(0));\n", type);
  fprintf(fp,"  size = op->GetMaxId()+1;\n");
  fprintf(fp,"  return vtkJavaMakeJArrayOf%sFrom%s(env,temp20,size);\n", fromtype, fromtype);
  fprintf(fp,"}\n");

  fprintf(fp,"extern \"C\" JNIEXPORT void  JNICALL Java_vtk_%s_SetJavaArray_10("
    "JNIEnv *env, jobject obj,j%sArray id0)\n", data->ClassName, jtype);
  fprintf(fp,"{\n");
  fprintf(fp,"  %s *op;\n", data->ClassName);
  fprintf(fp,"  %s *tempArray0;\n", type);
  fprintf(fp,"  int length;\n");
  fprintf(fp,"  tempArray0 = (%s *)(env->Get%sArrayElements(id0,NULL));\n", type, jfromtype);
  fprintf(fp,"  length = env->GetArrayLength(id0);\n");
  fprintf(fp,"  op = (%s *)vtkJavaGetPointerFromObject(env,obj);\n",
    data->ClassName);
  fprintf(fp,"  op->SetNumberOfTuples(length/op->GetNumberOfComponents());\n");
  fprintf(fp,"  memcpy(op->GetVoidPointer(0), tempArray0, length*sizeof(%s));\n", type);
  fprintf(fp,"  env->Release%sArrayElements(id0,(j%s *)tempArray0,0);\n", jfromtype, jtype);
  fprintf(fp,"}\n");
}


void outputFunction(FILE *fp, FileInfo *data)
{
  int rType = (currentFunction->ReturnType & VTK_PARSE_UNQUALIFIED_TYPE);
  int aType = 0;
  int i;
  int args_ok = 1;
  char *jniFunction = 0;
  char *begPtr = 0;
  char *endPtr = 0;
  CurrentData = data;

  /* some functions will not get wrapped no matter what else */
  if (currentFunction->IsOperator ||
      currentFunction->ArrayFailure ||
      !currentFunction->IsPublic ||
      !currentFunction->Name)
    {
    return;
    }

  /* NewInstance and SafeDownCast can not be wrapped because it is a
     (non-virtual) method which returns a pointer of the same type as
     the current pointer. Since all methods are virtual in Java, this
     looks like polymorphic return type.  */
  if (!strcmp("NewInstance",currentFunction->Name))
    {
    return ;
    }

  if (!strcmp("SafeDownCast",currentFunction->Name))
    {
    return ;
    }

  /* check to see if we can handle the args */
  for (i = 0; i < currentFunction->NumberOfArguments; i++)
    {
    aType = (currentFunction->ArgTypes[i] & VTK_PARSE_UNQUALIFIED_TYPE);

    if (aType == VTK_PARSE_VTK_OBJECT) args_ok = 0;
    if ((aType & VTK_PARSE_BASE_TYPE) == VTK_PARSE_UNKNOWN) args_ok = 0;
    if (((aType & VTK_PARSE_INDIRECT) != VTK_PARSE_POINTER) &&
        ((aType & VTK_PARSE_INDIRECT) != 0)) args_ok = 0;
    if (aType == VTK_PARSE_UNSIGNED_CHAR_PTR) args_ok = 0;
    if (aType == VTK_PARSE_UNSIGNED_INT_PTR) args_ok = 0;
    if (aType == VTK_PARSE_UNSIGNED_SHORT_PTR) args_ok = 0;
    if (aType == VTK_PARSE_UNSIGNED_LONG_PTR) args_ok = 0;
    if (aType == VTK_PARSE_UNSIGNED_ID_TYPE_PTR) args_ok = 0;
    if (aType == VTK_PARSE_UNSIGNED_LONG_LONG_PTR) args_ok = 0;
    if (aType == VTK_PARSE_UNSIGNED___INT64_PTR) args_ok = 0;
    }
  if ((rType & VTK_PARSE_BASE_TYPE) == VTK_PARSE_UNKNOWN) args_ok = 0;
  if (rType == VTK_PARSE_VTK_OBJECT) args_ok = 0;
  if (((rType & VTK_PARSE_INDIRECT) != VTK_PARSE_POINTER) &&
      ((rType & VTK_PARSE_INDIRECT) != 0)) args_ok = 0;


  /* eliminate unsigned short * usigned int * etc */
  if (rType == VTK_PARSE_UNSIGNED_INT_PTR) args_ok = 0;
  if (rType == VTK_PARSE_UNSIGNED_SHORT_PTR) args_ok = 0;
  if (rType == VTK_PARSE_UNSIGNED_LONG_PTR) args_ok = 0;
  if (rType == VTK_PARSE_UNSIGNED_ID_TYPE_PTR) args_ok = 0;
  if (rType == VTK_PARSE_UNSIGNED_LONG_LONG_PTR) args_ok = 0;
  if (rType == VTK_PARSE_UNSIGNED___INT64_PTR) args_ok = 0;

  if (currentFunction->NumberOfArguments &&
      (currentFunction->ArgTypes[0] == VTK_PARSE_FUNCTION)
      &&(currentFunction->NumberOfArguments != 1)) args_ok = 0;

  /* make sure we have all the info we need for array arguments in */
  for (i = 0; i < currentFunction->NumberOfArguments; i++)
    {
    aType = (currentFunction->ArgTypes[i] & VTK_PARSE_UNQUALIFIED_TYPE);

    if (((aType & VTK_PARSE_INDIRECT) == VTK_PARSE_POINTER)&&
        (currentFunction->ArgCounts[i] <= 0)&&
        (aType != VTK_PARSE_VTK_OBJECT_PTR)&&
        (aType != VTK_PARSE_CHAR_PTR)) args_ok = 0;
    }

  /* if we need a return type hint make sure we have one */
  switch (rType)
    {
    case VTK_PARSE_FLOAT_PTR:
    case VTK_PARSE_VOID_PTR:
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
      args_ok = currentFunction->HaveHint;
      break;
    }

  /* make sure it isn't a Delete or New function */
  if (!strcmp("Delete",currentFunction->Name) ||
      !strcmp("New",currentFunction->Name))
    {
    args_ok = 0;
    }

  /* handle DataReader SetBinaryInputString as a special case */
  if (!strcmp("SetBinaryInputString",currentFunction->Name) &&
      (!strcmp("vtkDataReader",data->ClassName) ||
       !strcmp("vtkStructuredGridReader",data->ClassName) ||
       !strcmp("vtkRectilinearGridReader",data->ClassName) ||
       !strcmp("vtkUnstructuredGridReader",data->ClassName) ||
       !strcmp("vtkStructuredPointsReader",data->ClassName) ||
       !strcmp("vtkPolyDataReader",data->ClassName)))
    {
    if(currentFunction->IsLegacy)
      {
      fprintf(fp,"#if !defined(VTK_LEGACY_REMOVE)\n");
      }
    HandleDataReader(fp,data);
    if(currentFunction->IsLegacy)
      {
      fprintf(fp,"#endif\n");
      }
    wrappedFunctions[numberOfWrappedFunctions] = currentFunction;
    numberOfWrappedFunctions++;
    }


  if (currentFunction->IsPublic && args_ok &&
      strcmp(data->ClassName,currentFunction->Name) &&
      strcmp(data->ClassName, currentFunction->Name + 1))
    {
    /* make sure we haven't already done one of these */
    if (!DoneOne())
      {
      fprintf(fp,"\n");

      /* Underscores are escaped in method names, see
           http://java.sun.com/javase/6/docs/technotes/guides/jni/spec/design.html#wp133
         VTK class names contain no underscore and do not need to be escaped.  */
      jniFunction = currentFunction->Name;
      begPtr = currentFunction->Name;
      endPtr = strchr(begPtr, '_');
      if(endPtr)
        {
        jniFunction = (char *)malloc(2*strlen(currentFunction->Name) + 1);
        jniFunction[0] = '\0';
        while (endPtr)
          {
          strncat(jniFunction, begPtr, endPtr - begPtr + 1);
          strcat(jniFunction, "1");
          begPtr = endPtr + 1;
          endPtr = strchr(begPtr, '_');
          }
        strcat(jniFunction, begPtr);
        }

      if(currentFunction->IsLegacy)
        {
        fprintf(fp,"#if !defined(VTK_LEGACY_REMOVE)\n");
        }
      fprintf(fp,"extern \"C\" JNIEXPORT ");
      return_result(fp);
      fprintf(fp," JNICALL Java_vtk_%s_%s_1%i(JNIEnv *env, jobject obj",
              data->ClassName, jniFunction, numberOfWrappedFunctions);

      for (i = 0; i < currentFunction->NumberOfArguments; i++)
        {
        fprintf(fp,",");
        output_proto_vars(fp, i);
        }
      fprintf(fp,")\n{\n");

      /* get the object pointer */
      fprintf(fp,"  %s *op;\n",data->ClassName);
      /* process the args */
      for (i = 0; i < currentFunction->NumberOfArguments; i++)
        {
        output_temp(fp, i, currentFunction->ArgTypes[i],
                   currentFunction->ArgClasses[i],
                   currentFunction->ArgCounts[i]);
        }
      output_temp(fp, MAX_ARGS,currentFunction->ReturnType,
                  currentFunction->ReturnClass,0);

      /* now get the required args from the stack */
      for (i = 0; i < currentFunction->NumberOfArguments; i++)
        {
        get_args(fp, i);
        }

      fprintf(fp,"\n  op = (%s *)vtkJavaGetPointerFromObject(env,obj);\n",
              data->ClassName);


      switch (rType)
        {
        case VTK_PARSE_VOID:
          fprintf(fp,"  op->%s(",currentFunction->Name);
          break;
        default:
          fprintf(fp,"  temp%i = (op)->%s(",MAX_ARGS, currentFunction->Name);
        }

      for (i = 0; i < currentFunction->NumberOfArguments; i++)
        {
        aType = (currentFunction->ArgTypes[i] & VTK_PARSE_UNQUALIFIED_TYPE);

        if (i)
          {
          fprintf(fp,",");
          }
        if (currentFunction->ArgTypes[i] == VTK_PARSE_FUNCTION)
          {
          fprintf(fp,"vtkJavaVoidFunc,(void *)temp%i",i);
          }
        else
          {
          fprintf(fp,"temp%i",i);
          }
        } /* for */

      fprintf(fp,");\n");

      if (currentFunction->NumberOfArguments == 1 &&
          currentFunction->ArgTypes[0] == VTK_PARSE_FUNCTION)
        {
        fprintf(fp,"  op->%sArgDelete(vtkJavaVoidFuncArgDelete);\n",
                jniFunction);
        }

      /* now copy and release any arrays */
      for (i = 0; i < currentFunction->NumberOfArguments; i++)
        {
        copy_and_release_args(fp, i);
        }
      do_return(fp);
      fprintf(fp,"}\n");
      if(currentFunction->IsLegacy)
        {
        fprintf(fp,"#endif\n");
        }

      wrappedFunctions[numberOfWrappedFunctions] = currentFunction;
      numberOfWrappedFunctions++;
      if (jniFunction != currentFunction->Name)
        {
        free(jniFunction);
        }
    } /* isDone() */
  } /* isAbstract */
}

/* print the parsed structures */
void vtkParseOutput(FILE *fp, FileInfo *data)
{
  int i;

  fprintf(fp,"// java wrapper for %s object\n//\n",data->ClassName);
  fprintf(fp,"#define VTK_WRAPPING_CXX\n");
  if (strcmp("vtkObject",data->ClassName) != 0)
    {
    /* Block inclusion of full streams.  */
    fprintf(fp,"#define VTK_STREAMS_FWD_ONLY\n");
    }
  fprintf(fp,"#include \"vtkSystemIncludes.h\"\n");
  fprintf(fp,"#include \"%s.h\"\n",data->ClassName);
  fprintf(fp,"#include \"vtkJavaUtil.h\"\n\n");
  fprintf(fp,"#include <vtksys/ios/sstream>\n");

  for (i = 0; i < data->NumberOfSuperClasses; i++)
    {
    fprintf(fp,"extern \"C\" JNIEXPORT void* %s_Typecast(void *op,char *dType);\n",
            data->SuperClasses[i]);
    }

  fprintf(fp,"\nextern \"C\" JNIEXPORT void* %s_Typecast(void *me,char *dType)\n{\n",data->ClassName);
  if (data->NumberOfSuperClasses > 0)
    {
    fprintf(fp,"  void* res;\n");
    }
  fprintf(fp,"  if (!strcmp(\"%s\",dType)) { return me; }\n", data->ClassName);
  /* check our superclasses */
  for (i = 0; i < data->NumberOfSuperClasses; i++)
    {
    fprintf(fp,"  if ((res= %s_Typecast(me,dType)) != NULL)",
            data->SuperClasses[i]);
    fprintf(fp," { return res; }\n");
    }
  fprintf(fp,"  return NULL;\n");
  fprintf(fp,"}\n\n");

  HandleDataArray(fp, data);

  /* insert function handling code here */
  for (i = 0; i < data->NumberOfFunctions; i++)
    {
    currentFunction = data->Functions + i;
    outputFunction(fp, data);
    }

  if ((!data->NumberOfSuperClasses)&&(data->HasDelete))
    {
    fprintf(fp,"\nextern \"C\" JNIEXPORT void JNICALL Java_vtk_%s_VTKDeleteReference(JNIEnv *,jclass,jlong id)\n",
            data->ClassName);
    fprintf(fp,"{\n  %s *op;\n",data->ClassName);
    fprintf(fp,"  op = reinterpret_cast<%s*>(id);\n",
            data->ClassName);
    fprintf(fp,"  op->Delete();\n");
    fprintf(fp,"}\n");

    fprintf(fp,"\nextern \"C\" JNIEXPORT void JNICALL Java_vtk_%s_VTKDelete(JNIEnv *env,jobject obj)\n",
            data->ClassName);
    fprintf(fp,"{\n  %s *op;\n",data->ClassName);
    fprintf(fp,"  op = (%s *)vtkJavaGetPointerFromObject(env,obj);\n",
            data->ClassName);
    fprintf(fp,"  op->Delete();\n");
    fprintf(fp,"}\n");

    fprintf(fp,"\nextern \"C\" JNIEXPORT void JNICALL Java_vtk_%s_VTKRegister(JNIEnv *env,jobject obj)\n",
            data->ClassName);
    fprintf(fp,"{\n  %s *op;\n",data->ClassName);
    fprintf(fp,"  op = (%s *)vtkJavaGetPointerFromObject(env,obj);\n",
            data->ClassName);
    fprintf(fp,"  op->Register(op);\n");
    fprintf(fp,"}\n");
    }
  if (data->IsConcrete)
    {
    fprintf(fp,"\nextern \"C\" JNIEXPORT jlong JNICALL Java_vtk_%s_VTKInit(JNIEnv *, jobject)",
            data->ClassName);
    fprintf(fp,"\n{");
    fprintf(fp,"\n  %s *aNewOne = %s::New();",data->ClassName, data->ClassName);
    fprintf(fp,"\n  return (jlong)(size_t)(void*)aNewOne;");
    fprintf(fp,"\n}\n");
    }

  /* for vtkRenderWindow we want to add a special method to support
   * native AWT rendering
   *
   * Including vtkJavaAwt.h provides inline implementations of
   * Java_vtk_vtkPanel_RenderCreate, Java_vtk_vtkPanel_Lock and
   * Java_vtk_vtkPanel_UnLock. */
  if (!strcmp("vtkRenderWindow",data->ClassName))
    {
    fprintf(fp,"\n#include \"vtkJavaAwt.h\"\n\n");
    }

  if (!strcmp("vtkObject",data->ClassName))
    {
    /* Add the Print method to vtkObject. */
    fprintf(fp,"\nextern \"C\" JNIEXPORT jstring JNICALL Java_vtk_vtkObject_Print(JNIEnv *env,jobject obj)\n");
    fprintf(fp,"{\n  vtkObject *op;\n");
    fprintf(fp,"  jstring tmp;\n\n");
    fprintf(fp,"  op = (vtkObject *)vtkJavaGetPointerFromObject(env,obj);\n");

    fprintf(fp,"  vtksys_ios::ostringstream vtkmsg_with_warning_C4701;\n");
    fprintf(fp,"  op->Print(vtkmsg_with_warning_C4701);\n");
    fprintf(fp,"  vtkmsg_with_warning_C4701.put('\\0');\n");
    fprintf(fp,"  tmp = vtkJavaMakeJavaString(env,vtkmsg_with_warning_C4701.str().c_str());\n");

    fprintf(fp,"  return tmp;\n");
    fprintf(fp,"}\n");

    /* Add the PrintRevisions method to vtkObject. */
    fprintf(fp,"\nextern \"C\" JNIEXPORT jstring JNICALL Java_vtk_vtkObject_PrintRevisions(JNIEnv *env,jobject obj)\n");
    fprintf(fp,"{\n  vtkObject *op;\n");
    fprintf(fp,"  jstring tmp;\n\n");
    fprintf(fp,"  op = (vtkObject *)vtkJavaGetPointerFromObject(env,obj);\n");

    fprintf(fp,"  vtksys_ios::ostringstream vtkmsg_with_warning_C4701;\n");
    fprintf(fp,"  op->PrintRevisions(vtkmsg_with_warning_C4701);\n");
    fprintf(fp,"  vtkmsg_with_warning_C4701.put('\\0');\n");
    fprintf(fp,"  tmp = vtkJavaMakeJavaString(env,vtkmsg_with_warning_C4701.str().c_str());\n");

    fprintf(fp,"  return tmp;\n");
    fprintf(fp,"}\n");

    fprintf(fp,"\nextern \"C\" JNIEXPORT jint JNICALL Java_vtk_vtkObject_AddObserver(JNIEnv *env,jobject obj, jstring id0, jobject id1, jstring id2)\n");
    fprintf(fp,"{\n  vtkObject *op;\n");

    fprintf(fp,"  vtkJavaCommand *cbc = vtkJavaCommand::New();\n");
    fprintf(fp,"  cbc->AssignJavaVM(env);\n");
    fprintf(fp,"  cbc->SetGlobalRef(env->NewGlobalRef(id1));\n");
    fprintf(fp,"  char    *temp2;\n");
    fprintf(fp,"  temp2 = vtkJavaUTFToChar(env,id2);\n");
    fprintf(fp,"  cbc->SetMethodID(env->GetMethodID(env->GetObjectClass(id1),temp2,\"()V\"));\n");
    fprintf(fp,"  char    *temp0;\n");
    fprintf(fp,"  temp0 = vtkJavaUTFToChar(env,id0);\n");
    fprintf(fp,"  op = (vtkObject *)vtkJavaGetPointerFromObject(env,obj);\n");
    fprintf(fp,"  unsigned long     temp20;\n");
    fprintf(fp,"  temp20 = op->AddObserver(temp0,cbc);\n");
    fprintf(fp,"  if (temp0) delete[] temp0;\n");
    fprintf(fp,"  if (temp2) delete[] temp2;\n");
    fprintf(fp,"  cbc->Delete();\n");
    fprintf(fp,"  return temp20;\n}\n");
   }
}
