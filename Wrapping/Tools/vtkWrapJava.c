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
#include "vtkParseMain.h"
#include "vtkParseHierarchy.h"
#include "vtkWrap.h"

HierarchyInfo *hierarchyInfo = NULL;
StringCache *stringCache = NULL;
int numberOfWrappedFunctions = 0;
FunctionInfo *wrappedFunctions[1000];
extern FunctionInfo *currentFunction;
ClassInfo *CurrentData;

void output_proto_vars(FILE *fp, int i)
{
  unsigned int aType =
    (currentFunction->ArgTypes[i] & VTK_PARSE_UNQUALIFIED_TYPE);

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

  if ((aType == VTK_PARSE_CHAR_PTR) ||
      (aType == VTK_PARSE_STRING) ||
      (aType == VTK_PARSE_STRING_REF))
  {
    fprintf(fp,"jstring ");
    fprintf(fp,"id%i",i);
    return;
  }

  if ((aType == VTK_PARSE_FLOAT_PTR) ||
      (aType == VTK_PARSE_DOUBLE_PTR))
  {
    fprintf(fp,"jdoubleArray ");
    fprintf(fp,"id%i",i);
    return;
  }

  if ((aType == VTK_PARSE_INT_PTR) ||
      (aType == VTK_PARSE_SHORT_PTR) ||
      (aType == VTK_PARSE_SIGNED_CHAR_PTR) ||
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
    case VTK_PARSE_OBJECT:   fprintf(fp,"jobject "); break;
    case VTK_PARSE_UNKNOWN: fprintf(fp,"jint "); break;
  }

  fprintf(fp,"id%i",i);
}

/* when the cpp file doesn't have enough info use the hint file */
void use_hints(FILE *fp)
{
  unsigned int rType =
    (currentFunction->ReturnType & VTK_PARSE_UNQUALIFIED_TYPE);

  /* use the hint */
  switch (rType)
  {
    case VTK_PARSE_UNSIGNED_CHAR_PTR:
      /* for vtkDataWriter we want to handle this case specially */
      if (strcmp(currentFunction->Name,"GetBinaryOutputString") ||
          strcmp(CurrentData->Name,"vtkDataWriter"))
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
  unsigned int rType =
    (currentFunction->ReturnType & VTK_PARSE_UNQUALIFIED_TYPE);

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
    case VTK_PARSE_UNKNOWN:
      fprintf(fp,"jint ");
      break;
    case VTK_PARSE_BOOL:
      fprintf(fp,"jboolean ");
      break;
    case VTK_PARSE_CHAR_PTR:
    case VTK_PARSE_STRING:
    case VTK_PARSE_STRING_REF:
      fprintf(fp,"jstring ");
      break;
    case VTK_PARSE_OBJECT_PTR:
      fprintf(fp,"jlong ");
      break;
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
      fprintf(fp,"jarray ");
      break;
  }
}


void output_temp(FILE *fp, int i, unsigned int aType, const char *Id,
                 int aCount)
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
  if ((i == MAX_ARGS) &&
      ((aType & VTK_PARSE_INDIRECT) != 0) &&
      ((aType & VTK_PARSE_CONST) != 0))
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
    case VTK_PARSE_OBJECT:     fprintf(fp,"%s ",Id); break;
    case VTK_PARSE_STRING:  fprintf(fp,"%s ",Id); break;
    case VTK_PARSE_UNKNOWN: fprintf(fp,"%s ",Id); break;
  }

  switch (aType & VTK_PARSE_INDIRECT)
  {
    case VTK_PARSE_REF:
      if (i == MAX_ARGS)
      {
        fprintf(fp, " *"); /* act " &" */
      }
      break;
    case VTK_PARSE_POINTER:
      if ((i == MAX_ARGS) ||
          ((aType & VTK_PARSE_UNQUALIFIED_TYPE) == VTK_PARSE_OBJECT_PTR) ||
          ((aType & VTK_PARSE_UNQUALIFIED_TYPE) == VTK_PARSE_CHAR_PTR))
      {
        fprintf(fp, " *");
      }
      break;
    default:
      fprintf(fp,"  ");
      break;
  }
  fprintf(fp,"temp%i",i);

  /* handle arrays */
  if (((aType & VTK_PARSE_INDIRECT) == VTK_PARSE_POINTER)&&
      (i != MAX_ARGS) &&
      ((aType & VTK_PARSE_UNQUALIFIED_TYPE) != VTK_PARSE_OBJECT_PTR) &&
      ((aType & VTK_PARSE_UNQUALIFIED_TYPE) != VTK_PARSE_CHAR_PTR))
  {
    fprintf(fp,"[%i]",aCount);
    fprintf(fp,";\n  void *tempArray%i",i);
  }

  fprintf(fp,";\n");
}

void get_args(FILE *fp, int i)
{
  unsigned int aType =
    (currentFunction->ArgTypes[i] & VTK_PARSE_UNQUALIFIED_TYPE);
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
    case VTK_PARSE_STRING:
    case VTK_PARSE_STRING_REF:
      fprintf(fp,"  vtkJavaUTFToString(env,id%i,temp%i);\n",i,i);
      break;
    case VTK_PARSE_OBJECT_PTR:
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
    case VTK_PARSE_SHORT_PTR:
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
    case VTK_PARSE_UNKNOWN:
      fprintf(fp,"  temp%i = static_cast<%s>(id%i);\n",
              i,currentFunction->ArgClasses[i],i);
      break;
    case VTK_PARSE_VOID:
    case VTK_PARSE_OBJECT:
    case VTK_PARSE_OBJECT_REF: break;
    default: fprintf(fp,"  temp%i = id%i;\n",i,i); break;
  }
}


void copy_and_release_args(FILE *fp, int i)
{
  unsigned int aType =
    (currentFunction->ArgTypes[i] & VTK_PARSE_UNQUALIFIED_TYPE);
  int j;

  /* handle VAR FUNCTIONS */
  if (currentFunction->ArgTypes[i] == VTK_PARSE_FUNCTION)
  {
    fprintf(fp,"  delete[] temp%i_str;\n",i);
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
      fprintf(fp,"  delete[] temp%i;\n",i);
      break;
    case VTK_PARSE_INT_PTR:
    case VTK_PARSE_LONG_PTR:
    case VTK_PARSE_SHORT_PTR:
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
  unsigned int rType =
    (currentFunction->ReturnType & VTK_PARSE_UNQUALIFIED_TYPE);

  /* ignore void */
  if (rType == VTK_PARSE_VOID)
  {
    return;
  }

  switch (rType)
  {
    case VTK_PARSE_CHAR_PTR:
    {
      fprintf(fp,"  return vtkJavaMakeJavaString(env,temp%i);\n", MAX_ARGS);
      break;
    }
    case VTK_PARSE_STRING:
    {
      fprintf(fp,"  return vtkJavaMakeJavaString(env,temp%i.c_str());\n", MAX_ARGS);
      break;
    }
    case VTK_PARSE_STRING_REF:
    {
      fprintf(fp,"  return vtkJavaMakeJavaString(env,temp%i->c_str());\n", MAX_ARGS);
      break;
    }
    case VTK_PARSE_OBJECT_PTR:
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

/* Check to see if two types will map to the same Java type,
 * return 1 if type1 should take precedence,
 * return 2 if type2 should take precedence,
 * return 0 if the types do not map to the same type */
static int CheckMatch(
  unsigned int type1, unsigned int type2, const char *c1, const char *c2)
{
  static unsigned int floatTypes[] = {
    VTK_PARSE_DOUBLE, VTK_PARSE_FLOAT, 0 };

  static unsigned int intTypes[] = {
    VTK_PARSE_UNSIGNED_LONG_LONG, VTK_PARSE_UNSIGNED___INT64,
    VTK_PARSE_LONG_LONG, VTK_PARSE___INT64, VTK_PARSE_ID_TYPE,
    VTK_PARSE_UNSIGNED_LONG, VTK_PARSE_LONG,
    VTK_PARSE_UNSIGNED_INT, VTK_PARSE_INT,
    VTK_PARSE_UNSIGNED_SHORT, VTK_PARSE_SHORT,
    VTK_PARSE_UNSIGNED_CHAR, VTK_PARSE_SIGNED_CHAR, 0 };

  static unsigned int stringTypes[] = {
    VTK_PARSE_CHAR_PTR, VTK_PARSE_STRING_REF, VTK_PARSE_STRING, 0 };

  static unsigned int *numericTypes[] = { floatTypes, intTypes, 0 };

  int i, j;
  int hit1, hit2;

  if ((type1 & VTK_PARSE_UNQUALIFIED_TYPE) ==
      (type2 & VTK_PARSE_UNQUALIFIED_TYPE))
  {
    if ((type1 & VTK_PARSE_BASE_TYPE) == VTK_PARSE_OBJECT)
    {
      if (strcmp(c1, c2) == 0)
      {
        return 1;
      }
      return 0;
    }
    else
    {
      return 1;
    }
  }

  for (i = 0; numericTypes[i]; i++)
  {
    hit1 = 0;
    hit2 = 0;
    for (j = 0; numericTypes[i][j]; j++)
    {
      if ((type1 & VTK_PARSE_BASE_TYPE) == numericTypes[i][j])
      {
        hit1 = j+1;
      }
      if ((type2 & VTK_PARSE_BASE_TYPE) == numericTypes[i][j])
      {
        hit2 = j+1;
      }
    }
    if (hit1 && hit2 &&
        (type1 & VTK_PARSE_INDIRECT) == (type2 & VTK_PARSE_INDIRECT))
    {
      if (hit1 < hit2)
      {
        return 1;
      }
      else
      {
        return 2;
      }
    }
  }

  hit1 = 0;
  hit2 = 0;
  for (j = 0; stringTypes[j]; j++)
  {
    if ((type1 & VTK_PARSE_UNQUALIFIED_TYPE) == stringTypes[j])
    {
      hit1 = j+1;
    }
    if ((type2 & VTK_PARSE_UNQUALIFIED_TYPE) == stringTypes[j])
    {
      hit2 = j+1;
    }
  }
  if (hit1 && hit2)
  {
    if (hit1 < hit2)
    {
      return 1;
    }
    else
    {
      return 2;
    }
  }

  return 0;
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
        if (!CheckMatch(currentFunction->ArgTypes[j], fi->ArgTypes[j],
                        currentFunction->ArgClasses[j],fi->ArgClasses[j]))
        {
          match = 0;
        }
      }
      if (!CheckMatch(currentFunction->ReturnType, fi->ReturnType,
                      currentFunction->ReturnClass, fi->ReturnClass))
      {
        match = 0;
      }
      if (match) return 1;
    }
  }
  return 0;
}

void HandleDataReader(FILE *fp, ClassInfo *data)
{
    fprintf(fp,"\n");
    fprintf(fp,"extern \"C\" JNIEXPORT void");
    fprintf(fp," JNICALL Java_vtk_%s_%s_1%i(JNIEnv *env, jobject obj, jbyteArray id0, jint id1)\n",
            data->Name,currentFunction->Name, numberOfWrappedFunctions);
    fprintf(fp,"{\n");
    fprintf(fp,"  %s *op;\n",data->Name);
    fprintf(fp,"  op = (%s *)vtkJavaGetPointerFromObject(env,obj);\n",
            data->Name);
    fprintf(fp,"  jboolean isCopy;\n");
    fprintf(fp,"  jbyte *data = env->GetByteArrayElements(id0,&isCopy);\n");
    fprintf(fp,"  op->SetBinaryInputString((const char *)data,id1);\n");
    fprintf(fp,"  env->ReleaseByteArrayElements(id0,data,JNI_ABORT);\n");
    fprintf(fp,"}\n");
}

void HandleDataArray(FILE *fp, ClassInfo *data)
{
  const char *type = 0;
  const char *jtype = 0;
  const char *fromtype = 0;
  const char *jfromtype = 0;

  if (!strcmp("vtkCharArray",data->Name) )
  {
    type = "char";
    fromtype = "Char";
    jtype = "byte";
    jfromtype = "Byte";
  }
  else if (!strcmp("vtkDoubleArray",data->Name) )
  {
    type = "double";
    fromtype = "Double";
    jtype = type;
    jfromtype = fromtype;
  }
  else if (!strcmp("vtkFloatArray",data->Name) )
  {
    type = "float";
    fromtype = "Float";
    jtype = type;
    jfromtype = fromtype;
  }
  else if (!strcmp("vtkIntArray",data->Name) )
  {
    type = "int";
    fromtype = "Int";
    jtype = type;
    jfromtype = fromtype;
  }
  else if (!strcmp("vtkLongArray",data->Name) )
  {
    type = "long";
    fromtype = "Long";
    jtype = type;
    jfromtype = fromtype;
  }
  else if (!strcmp("vtkShortArray",data->Name) )
  {
    type = "short";
    fromtype = "Short";
    jtype = type;
    jfromtype = fromtype;
  }
  else if (!strcmp("vtkUnsignedCharArray",data->Name) )
  {
    type = "unsigned char";
    fromtype = "UnsignedChar";
    jtype = "byte";
    jfromtype = "Byte";
  }
  else if (!strcmp("vtkUnsignedIntArray",data->Name) )
  {
    type = "unsigned int";
    fromtype = "UnsignedInt";
    jtype = "int";
    jfromtype = "Int";
  }
  else if (!strcmp("vtkUnsignedLongArray",data->Name) )
  {
    type = "unsigned long";
    fromtype = "UnsignedLong";
    jtype = "long";
    jfromtype = "Long";
  }
  else if (!strcmp("vtkUnsignedShortArray",data->Name) )
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
    data->Name);
  fprintf(fp,"{\n");
  fprintf(fp,"  %s *op;\n", data->Name);
  fprintf(fp,"  %s  *temp20;\n", type);
  fprintf(fp,"  vtkIdType size;\n");
  fprintf(fp,"\n");
  fprintf(fp,"  op = (%s *)vtkJavaGetPointerFromObject(env,obj);\n",
    data->Name);
  fprintf(fp,"  temp20 = static_cast<%s*>(op->GetVoidPointer(0));\n", type);
  fprintf(fp,"  size = op->GetMaxId()+1;\n");
  fprintf(fp,"  return vtkJavaMakeJArrayOf%sFrom%s(env,temp20,size);\n", fromtype, fromtype);
  fprintf(fp,"}\n");

  fprintf(fp,"extern \"C\" JNIEXPORT void  JNICALL Java_vtk_%s_SetJavaArray_10("
    "JNIEnv *env, jobject obj,j%sArray id0)\n", data->Name, jtype);
  fprintf(fp,"{\n");
  fprintf(fp,"  %s *op;\n", data->Name);
  fprintf(fp,"  %s *tempArray0;\n", type);
  fprintf(fp,"  int length;\n");
  fprintf(fp,"  tempArray0 = (%s *)(env->Get%sArrayElements(id0,NULL));\n", type, jfromtype);
  fprintf(fp,"  length = env->GetArrayLength(id0);\n");
  fprintf(fp,"  op = (%s *)vtkJavaGetPointerFromObject(env,obj);\n",
    data->Name);
  fprintf(fp,"  op->SetNumberOfTuples(length/op->GetNumberOfComponents());\n");
  fprintf(fp,"  memcpy(op->GetVoidPointer(0), tempArray0, length*sizeof(%s));\n", type);
  fprintf(fp,"  env->Release%sArrayElements(id0,(j%s *)tempArray0,0);\n", jfromtype, jtype);
  fprintf(fp,"}\n");
}

static int isClassWrapped(const char *classname)
{
  HierarchyEntry *entry;

  if (hierarchyInfo)
  {
    entry = vtkParseHierarchy_FindEntry(hierarchyInfo, classname);

    if (entry == 0 ||
        vtkParseHierarchy_GetProperty(entry, "WRAP_EXCLUDE") ||
        !vtkParseHierarchy_IsTypeOf(hierarchyInfo, entry, "vtkObjectBase"))
    {
      return 0;
    }
  }

  return 1;
}

int checkFunctionSignature(ClassInfo *data)
{
  static unsigned int supported_types[] = {
    VTK_PARSE_VOID, VTK_PARSE_BOOL, VTK_PARSE_FLOAT, VTK_PARSE_DOUBLE,
    VTK_PARSE_CHAR, VTK_PARSE_UNSIGNED_CHAR, VTK_PARSE_SIGNED_CHAR,
    VTK_PARSE_INT, VTK_PARSE_UNSIGNED_INT,
    VTK_PARSE_SHORT, VTK_PARSE_UNSIGNED_SHORT,
    VTK_PARSE_LONG, VTK_PARSE_UNSIGNED_LONG,
    VTK_PARSE_ID_TYPE, VTK_PARSE_UNSIGNED_ID_TYPE,
    VTK_PARSE_LONG_LONG, VTK_PARSE_UNSIGNED_LONG_LONG,
    VTK_PARSE___INT64, VTK_PARSE_UNSIGNED___INT64,
    VTK_PARSE_OBJECT, VTK_PARSE_STRING, VTK_PARSE_UNKNOWN,
    0
  };

  int i, j;
  int args_ok = 1;
  unsigned int rType =
    (currentFunction->ReturnType & VTK_PARSE_UNQUALIFIED_TYPE);
  unsigned int aType = 0;
  unsigned int baseType = 0;

  /* some functions will not get wrapped no matter what else */
  if (currentFunction->IsOperator ||
      currentFunction->ArrayFailure ||
      !currentFunction->IsPublic ||
      !currentFunction->Name)
  {
    return 0;
  }

  /* NewInstance and SafeDownCast can not be wrapped because it is a
     (non-virtual) method which returns a pointer of the same type as
     the current pointer. Since all methods are virtual in Java, this
     looks like polymorphic return type.  */
  if (!strcmp("NewInstance",currentFunction->Name))
  {
    return 0;
  }

  if (!strcmp("SafeDownCast",currentFunction->Name))
  {
    return 0;
  }

  /* The GetInput() in vtkMapper cannot be overridden with a
   * different return type, Java doesn't allow this */
  if (strcmp(data->Name, "vtkMapper") == 0 &&
      strcmp(currentFunction->Name, "GetInput") == 0)
  {
    return 0;
  }

  /* function pointer arguments for callbacks */
  if (currentFunction->NumberOfArguments == 2 &&
      currentFunction->ArgTypes[0] == VTK_PARSE_FUNCTION &&
      currentFunction->ArgTypes[1] == VTK_PARSE_VOID_PTR &&
      rType == VTK_PARSE_VOID)
  {
    return 1;
  }

  /* check to see if we can handle the args */
  for (i = 0; i < currentFunction->NumberOfArguments; i++)
  {
    aType = (currentFunction->ArgTypes[i] & VTK_PARSE_UNQUALIFIED_TYPE);
    baseType = (aType & VTK_PARSE_BASE_TYPE);

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
      if ((aType & VTK_PARSE_INDIRECT) == 0)
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

    if (baseType == VTK_PARSE_OBJECT)
    {
      if ((aType & VTK_PARSE_INDIRECT) != VTK_PARSE_POINTER)
      {
        args_ok = 0;
      }
      else if (!isClassWrapped(currentFunction->ArgClasses[i]))
      {
        args_ok = 0;
      }
    }

    if (aType == VTK_PARSE_OBJECT) args_ok = 0;
    if (((aType & VTK_PARSE_INDIRECT) != VTK_PARSE_POINTER) &&
        ((aType & VTK_PARSE_INDIRECT) != 0) &&
        (aType != VTK_PARSE_STRING_REF)) args_ok = 0;
    if (aType == VTK_PARSE_STRING_PTR) args_ok = 0;
    if (aType == VTK_PARSE_UNSIGNED_CHAR_PTR) args_ok = 0;
    if (aType == VTK_PARSE_UNSIGNED_INT_PTR) args_ok = 0;
    if (aType == VTK_PARSE_UNSIGNED_SHORT_PTR) args_ok = 0;
    if (aType == VTK_PARSE_UNSIGNED_LONG_PTR) args_ok = 0;
    if (aType == VTK_PARSE_UNSIGNED_ID_TYPE_PTR) args_ok = 0;
    if (aType == VTK_PARSE_UNSIGNED_LONG_LONG_PTR) args_ok = 0;
    if (aType == VTK_PARSE_UNSIGNED___INT64_PTR) args_ok = 0;
  }

  baseType = (rType & VTK_PARSE_BASE_TYPE);

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
    if ((rType & VTK_PARSE_INDIRECT) == 0)
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

  if (baseType == VTK_PARSE_OBJECT)
  {
    if ((rType & VTK_PARSE_INDIRECT) != VTK_PARSE_POINTER)
    {
      args_ok = 0;
    }
    else if (!isClassWrapped(currentFunction->ReturnClass))
    {
      args_ok = 0;
    }
  }

  if (((rType & VTK_PARSE_INDIRECT) != VTK_PARSE_POINTER) &&
      ((rType & VTK_PARSE_INDIRECT) != 0) &&
      (rType != VTK_PARSE_STRING_REF)) args_ok = 0;
  if (rType == VTK_PARSE_STRING_PTR) args_ok = 0;

  /* eliminate unsigned char * and unsigned short * */
  if (rType == VTK_PARSE_UNSIGNED_INT_PTR) args_ok = 0;
  if (rType == VTK_PARSE_UNSIGNED_SHORT_PTR) args_ok = 0;
  if (rType == VTK_PARSE_UNSIGNED_LONG_PTR) args_ok = 0;
  if (rType == VTK_PARSE_UNSIGNED_ID_TYPE_PTR) args_ok = 0;
  if (rType == VTK_PARSE_UNSIGNED_LONG_LONG_PTR) args_ok = 0;
  if (rType == VTK_PARSE_UNSIGNED___INT64_PTR) args_ok = 0;

  /* make sure we have all the info we need for array arguments in */
  for (i = 0; i < currentFunction->NumberOfArguments; i++)
  {
    aType = (currentFunction->ArgTypes[i] & VTK_PARSE_UNQUALIFIED_TYPE);

    if (((aType & VTK_PARSE_INDIRECT) == VTK_PARSE_POINTER)&&
        (currentFunction->ArgCounts[i] <= 0)&&
        (aType != VTK_PARSE_OBJECT_PTR)&&
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

  /* make sure there isn't a Java-specific override */
  if (!strcmp("vtkObject",data->Name))
  {
    /* remove the original vtkCommand observer methods */
    if (!strcmp(currentFunction->Name,"AddObserver") ||
        !strcmp(currentFunction->Name,"GetCommand") ||
        (!strcmp(currentFunction->Name,"RemoveObserver") &&
         (currentFunction->ArgTypes[0] != VTK_PARSE_UNSIGNED_LONG)) ||
        ((!strcmp(currentFunction->Name,"RemoveObservers") ||
          !strcmp(currentFunction->Name,"HasObserver")) &&
         (((currentFunction->ArgTypes[0] != VTK_PARSE_UNSIGNED_LONG) &&
           (currentFunction->ArgTypes[0] !=
            (VTK_PARSE_CHAR_PTR|VTK_PARSE_CONST))) ||
          (currentFunction->NumberOfArguments > 1))) ||
        (!strcmp(currentFunction->Name,"RemoveAllObservers") &&
         (currentFunction->NumberOfArguments > 0)))
    {
      args_ok = 0;
    }
  }
  else if (!strcmp("vtkObjectBase",data->Name))
  {
    /* remove the special vtkObjectBase methods */
    if (!strcmp(currentFunction->Name,"Print"))
    {
      args_ok = 0;
    }
  }

  /* make sure it isn't a Delete or New function */
  if (!strcmp("Delete",currentFunction->Name) ||
      !strcmp("New",currentFunction->Name))
  {
    args_ok = 0;
  }

  return args_ok;
}

void outputFunction(FILE *fp, ClassInfo *data)
{
  int i;
  int args_ok;
  unsigned int rType =
    (currentFunction->ReturnType & VTK_PARSE_UNQUALIFIED_TYPE);
  const char *jniFunction = 0;
  char *jniFunctionNew = 0;
  char *jniFunctionOld = 0;
  size_t j;
  CurrentData = data;

  args_ok = checkFunctionSignature(data);

  /* handle DataReader SetBinaryInputString as a special case */
  if (!strcmp("SetBinaryInputString",currentFunction->Name) &&
      (!strcmp("vtkDataReader",data->Name) ||
       !strcmp("vtkStructuredGridReader",data->Name) ||
       !strcmp("vtkRectilinearGridReader",data->Name) ||
       !strcmp("vtkUnstructuredGridReader",data->Name) ||
       !strcmp("vtkStructuredPointsReader",data->Name) ||
       !strcmp("vtkPolyDataReader",data->Name)))
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
      strcmp(data->Name,currentFunction->Name) &&
      strcmp(data->Name, currentFunction->Name + 1))
  {
    /* make sure we haven't already done one of these */
    if (!DoneOne())
    {
      fprintf(fp,"\n");

      /* Underscores are escaped in method names, see
           http://java.sun.com/javase/6/docs/technotes/guides/jni/spec/design.html#wp133
         VTK class names contain no underscore and do not need to be escaped.  */
      jniFunction = currentFunction->Name;
      jniFunctionOld = 0;
      j = 0;
      while (jniFunction[j] != '\0')
      {
        /* replace "_" with "_1" */
        if (jniFunction[j] == '_')
        {
          j++;
          jniFunctionNew = (char *)malloc(strlen(jniFunction) + 2);
          strncpy(jniFunctionNew, jniFunction, j);
          jniFunctionNew[j] = '1';
          strcpy(&jniFunctionNew[j+1], &jniFunction[j]);
          free(jniFunctionOld);
          jniFunctionOld = jniFunctionNew;
          jniFunction = jniFunctionNew;
        }
        j++;
      }

      if(currentFunction->IsLegacy)
      {
        fprintf(fp,"#if !defined(VTK_LEGACY_REMOVE)\n");
      }
      fprintf(fp,"extern \"C\" JNIEXPORT ");
      return_result(fp);
      fprintf(fp," JNICALL Java_vtk_%s_%s_1%i(JNIEnv *env, jobject obj",
              data->Name, jniFunction, numberOfWrappedFunctions);

      for (i = 0; i < currentFunction->NumberOfArguments; i++)
      {
        fprintf(fp,",");
        output_proto_vars(fp, i);

        /* ignore args after function pointer */
        if (currentFunction->ArgTypes[i] == VTK_PARSE_FUNCTION)
        {
          break;
        }
      }
      fprintf(fp,")\n{\n");

      /* get the object pointer */
      fprintf(fp,"  %s *op;\n",data->Name);

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
                  currentFunction->ReturnClass,0);

      /* now get the required args from the stack */
      for (i = 0; i < currentFunction->NumberOfArguments; i++)
      {
        get_args(fp, i);

        /* ignore args after function pointer */
        if (currentFunction->ArgTypes[i] == VTK_PARSE_FUNCTION)
        {
          break;
        }
      }

      fprintf(fp,"\n  op = (%s *)vtkJavaGetPointerFromObject(env,obj);\n",
              data->Name);


      switch (rType)
      {
        case VTK_PARSE_VOID:
          fprintf(fp,"  op->%s(",currentFunction->Name);
          break;
        default:
          if ((rType & VTK_PARSE_INDIRECT) == VTK_PARSE_REF)
          {
            fprintf(fp,"  temp%i = &(op)->%s(",MAX_ARGS,currentFunction->Name);
          }
          else
          {
            fprintf(fp,"  temp%i = (op)->%s(",MAX_ARGS,currentFunction->Name);
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
          fprintf(fp,"vtkJavaVoidFunc,(void *)temp%i",i);
          break;
        }
        else
        {
          fprintf(fp,"temp%i",i);
        }
      } /* for */

      fprintf(fp,");\n");

      if (currentFunction->NumberOfArguments == 2 &&
          currentFunction->ArgTypes[0] == VTK_PARSE_FUNCTION)
      {
        fprintf(fp,"  op->%sArgDelete(vtkJavaVoidFuncArgDelete);\n",
                jniFunction);
      }

      /* now copy and release any arrays */
      for (i = 0; i < currentFunction->NumberOfArguments; i++)
      {
        copy_and_release_args(fp, i);

        /* ignore args after function pointer */
        if (currentFunction->ArgTypes[i] == VTK_PARSE_FUNCTION)
        {
          break;
        }
      }
      do_return(fp);
      fprintf(fp,"}\n");
      if(currentFunction->IsLegacy)
      {
        fprintf(fp,"#endif\n");
      }

      wrappedFunctions[numberOfWrappedFunctions] = currentFunction;
      numberOfWrappedFunctions++;
      if (jniFunctionNew)
      {
        free(jniFunctionNew);
        jniFunctionNew = 0;
      }
    } /* isDone() */
  } /* isAbstract */
}

/* print the parsed structures */
int main(int argc, char *argv[])
{
  OptionInfo *options;
  FileInfo *file_info;
  ClassInfo *data;
  FILE *fp;
  int i;

  /* pre-define a macro to identify the language */
  vtkParse_DefineMacro("__VTK_WRAP_JAVA__", 0);

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
    exit(1);
  }

  /* get the hierarchy info for accurate typing */
  if (options->HierarchyFileNames)
  {
    hierarchyInfo = vtkParseHierarchy_ReadFiles(
      options->NumberOfHierarchyFileNames, options->HierarchyFileNames);
    if (hierarchyInfo)
    {
      /* resolve using declarations within the header files */
      vtkWrap_ApplyUsingDeclarations(data, file_info, hierarchyInfo);

      /* expand typedefs */
      vtkWrap_ExpandTypedefs(data, file_info, hierarchyInfo);
    }
  }

  fprintf(fp,"// java wrapper for %s object\n//\n",data->Name);
  fprintf(fp,"#define VTK_WRAPPING_CXX\n");
  if (strcmp("vtkObjectBase",data->Name) != 0)
  {
    /* Block inclusion of full streams.  */
    fprintf(fp,"#define VTK_STREAMS_FWD_ONLY\n");
  }
  fprintf(fp,"#include \"vtkSystemIncludes.h\"\n");
  fprintf(fp,"#include \"%s.h\"\n",data->Name);
  fprintf(fp,"#include \"vtkJavaUtil.h\"\n\n");
  fprintf(fp,"#include \"vtkStdString.h\"\n\n");
  fprintf(fp,"#include <sstream>\n");

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

    fprintf(fp,"extern \"C\" JNIEXPORT void* %s_Typecast(void *op,char *dType);\n",
            safe_superclass);

    free(safe_name);
  }

  fprintf(fp,"\nextern \"C\" JNIEXPORT void* %s_Typecast(void *me,char *dType)\n{\n",data->Name);
  if (data->NumberOfSuperClasses > 0)
  {
    fprintf(fp,"  void* res;\n");
  }
  fprintf(fp,"  if (!strcmp(\"%s\",dType)) { return me; }\n", data->Name);
  /* check our superclasses */
  for (i = 0; i < data->NumberOfSuperClasses; i++)
  {
    char *safe_name = vtkWrap_SafeSuperclassName(data->SuperClasses[i]);
    const char *safe_superclass = safe_name ? safe_name : data->SuperClasses[i];

    fprintf(fp,"  if ((res= %s_Typecast(me,dType)) != NULL)",
            safe_superclass);
    fprintf(fp," { return res; }\n");

    free(safe_name);
  }
  fprintf(fp,"  return NULL;\n");
  fprintf(fp,"}\n\n");

  HandleDataArray(fp, data);

  /* insert function handling code here */
  for (i = 0; i < data->NumberOfFunctions; i++)
  {
    currentFunction = data->Functions[i];
    outputFunction(fp, data);
  }

  if ((!data->NumberOfSuperClasses)&&(data->HasDelete))
  {
    fprintf(fp,"\nextern \"C\" JNIEXPORT void JNICALL Java_vtk_%s_VTKDeleteReference(JNIEnv *,jclass,jlong id)\n",
            data->Name);
    fprintf(fp,"{\n  %s *op;\n",data->Name);
    fprintf(fp,"  op = reinterpret_cast<%s*>(id);\n",
            data->Name);
    fprintf(fp,"  op->Delete();\n");
    fprintf(fp,"}\n");

    fprintf(fp,"\nextern \"C\" JNIEXPORT jstring JNICALL Java_vtk_%s_VTKGetClassNameFromReference(JNIEnv *env,jclass,jlong id)\n",
            data->Name);
    fprintf(fp,"{\n");
    fprintf(fp,"  const char* name = \"\";\n");
    fprintf(fp,"  %s *op;\n", data->Name);
    fprintf(fp,"  if(id != 0)\n");
    fprintf(fp,"  {\n");
    fprintf(fp,"    op = reinterpret_cast<%s*>(id);\n", data->Name);
    //fprintf(fp,"    std::cout << \"cast pointer \" << id << std::endl;\n");
    fprintf(fp,"    name = op->GetClassName();\n");
    fprintf(fp,"  }\n");
    fprintf(fp,"  return vtkJavaMakeJavaString(env,name);\n");
    fprintf(fp,"}\n");

    fprintf(fp,"\nextern \"C\" JNIEXPORT void JNICALL Java_vtk_%s_VTKDelete(JNIEnv *env,jobject obj)\n",
            data->Name);
    fprintf(fp,"{\n  %s *op;\n",data->Name);
    fprintf(fp,"  op = (%s *)vtkJavaGetPointerFromObject(env,obj);\n",
            data->Name);
    fprintf(fp,"  op->Delete();\n");
    fprintf(fp,"}\n");

    fprintf(fp,"\nextern \"C\" JNIEXPORT void JNICALL Java_vtk_%s_VTKRegister(JNIEnv *env,jobject obj)\n",
            data->Name);
    fprintf(fp,"{\n  %s *op;\n",data->Name);
    fprintf(fp,"  op = (%s *)vtkJavaGetPointerFromObject(env,obj);\n",
            data->Name);
    fprintf(fp,"  op->Register(op);\n");
    fprintf(fp,"}\n");
  }
  if (!data->IsAbstract)
  {
    fprintf(fp,"\nextern \"C\" JNIEXPORT jlong JNICALL Java_vtk_%s_VTKInit(JNIEnv *, jobject)",
            data->Name);
    fprintf(fp,"\n{");
    fprintf(fp,"\n  %s *aNewOne = %s::New();",data->Name, data->Name);
    fprintf(fp,"\n  return (jlong)(size_t)(void*)aNewOne;");
    fprintf(fp,"\n}\n");
  }

  /* for vtkRenderWindow we want to add a special method to support
   * native AWT rendering
   *
   * Including vtkJavaAwt.h provides inline implementations of
   * Java_vtk_vtkPanel_RenderCreate, Java_vtk_vtkPanel_Lock and
   * Java_vtk_vtkPanel_UnLock. */
  if (!strcmp("vtkRenderWindow",data->Name))
  {
    fprintf(fp,"\n#include \"vtkJavaAwt.h\"\n\n");
  }

  if (!strcmp("vtkObject",data->Name))
  {
    /* Add the Print method to vtkObjectBase. */
    fprintf(fp,"\nextern \"C\" JNIEXPORT jstring JNICALL Java_vtk_vtkObjectBase_Print(JNIEnv *env,jobject obj)\n");
    fprintf(fp,"{\n  vtkObjectBase *op;\n");
    fprintf(fp,"  jstring tmp;\n\n");
    fprintf(fp,"  op = (vtkObjectBase *)vtkJavaGetPointerFromObject(env,obj);\n");

    fprintf(fp,"  std::ostringstream vtkmsg_with_warning_C4701;\n");
    fprintf(fp,"  op->Print(vtkmsg_with_warning_C4701);\n");
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
    fprintf(fp,"  delete[] temp0;\n");
    fprintf(fp,"  delete[] temp2;\n");
    fprintf(fp,"  cbc->Delete();\n");
    fprintf(fp,"  return temp20;\n}\n");
  }

  vtkParse_Free(file_info);

  fclose(fp);

  return 0;
}
