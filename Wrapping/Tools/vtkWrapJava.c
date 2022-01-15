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

#include "vtkParse.h"
#include "vtkParseHierarchy.h"
#include "vtkParseMain.h"
#include "vtkParseSystem.h"
#include "vtkWrap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

HierarchyInfo* hierarchyInfo = NULL;
StringCache* stringCache = NULL;
int numberOfWrappedFunctions = 0;
FunctionInfo* wrappedFunctions[1000];
FunctionInfo* thisFunction;
ClassInfo* thisClass;

void OutputParamDeclarations(FILE* fp, int i)
{
  unsigned int aType = (thisFunction->ArgTypes[i] & VTK_PARSE_UNQUALIFIED_TYPE);

  /* ignore void */
  if (aType == VTK_PARSE_VOID)
  {
    return;
  }

  if (thisFunction->ArgTypes[i] == VTK_PARSE_FUNCTION)
  {
    fprintf(fp, "jobject id0, jbyteArray id1, jint len1");
    return;
  }

  if (((thisFunction->Parameters[i]->CountHint == NULL) && (aType == VTK_PARSE_CHAR_PTR)) ||
    (aType == VTK_PARSE_STRING) || (aType == VTK_PARSE_STRING_REF))
  {
    fprintf(fp, " jbyteArray id%i, jint len%i", i, i);
    return;
  }

  if (aType == VTK_PARSE_BOOL_PTR)
  {
    fprintf(fp, "jbooleanArray id%i", i);
    return;
  }

  if (aType == VTK_PARSE_FLOAT_PTR)
  {
    fprintf(fp, "jfloatArray id%i", i);
    return;
  }

  if (aType == VTK_PARSE_DOUBLE_PTR)
  {
    fprintf(fp, "jdoubleArray id%i", i);
    return;
  }

  if (((thisFunction->Parameters[i]->CountHint != NULL) && (aType == VTK_PARSE_CHAR_PTR)) ||
    (aType == VTK_PARSE_SIGNED_CHAR_PTR) || (aType == VTK_PARSE_UNSIGNED_CHAR_PTR))
  {
    fprintf(fp, "jbyteArray id%i", i);
    return;
  }

  if (aType == VTK_PARSE_SHORT_PTR)
  {
    fprintf(fp, "jshortArray id%i", i);
    return;
  }

  if (aType == VTK_PARSE_INT_PTR)
  {
    fprintf(fp, "jintArray id%i", i);
    return;
  }

  if ((aType == VTK_PARSE_LONG_PTR) || (aType == VTK_PARSE_LONG_LONG_PTR) ||
    (aType == VTK_PARSE___INT64_PTR))
  {
    fprintf(fp, "jlongArray id%i", i);
    return;
  }

  switch (aType & VTK_PARSE_BASE_TYPE)
  {
    case VTK_PARSE_SIGNED_CHAR:
    case VTK_PARSE_UNSIGNED_CHAR:
      fprintf(fp, "jbyte ");
      break;
    case VTK_PARSE_CHAR:
      fprintf(fp, "jchar ");
      break;
  }

  switch ((aType & VTK_PARSE_BASE_TYPE) & ~VTK_PARSE_UNSIGNED)
  {
    case VTK_PARSE_FLOAT:
      fprintf(fp, "jfloat ");
      break;
    case VTK_PARSE_DOUBLE:
      fprintf(fp, "jdouble ");
      break;
    case VTK_PARSE_SHORT:
      fprintf(fp, "jshort ");
      break;
    case VTK_PARSE_INT:
      fprintf(fp, "jint ");
      break;
    case VTK_PARSE_LONG:
    case VTK_PARSE_LONG_LONG:
    case VTK_PARSE___INT64:
      fprintf(fp, "jlong ");
      break;
    case VTK_PARSE_BOOL:
      fprintf(fp, "jboolean ");
      break;
    case VTK_PARSE_VOID:
      fprintf(fp, "void ");
      break;
    case VTK_PARSE_OBJECT:
      fprintf(fp, "jobject ");
      break;
    case VTK_PARSE_UNKNOWN: /* enum */
      fprintf(fp, "jint ");
      break;
  }

  fprintf(fp, "id%i", i);
}

/* when the cpp file doesn't have enough info use the hint file */
void use_hints(FILE* fp)
{
  unsigned int rawType = thisFunction->ReturnType;

  const char* qualifier = "";
  if ((rawType & VTK_PARSE_CONST) != 0)
  {
    qualifier = "const ";
  }

  unsigned int basicType = rawType & VTK_PARSE_UNQUALIFIED_TYPE;
  /* use the hint */
  switch (basicType)
  {
    case VTK_PARSE_UNSIGNED_CHAR_PTR:
      /* for vtkDataWriter we want to handle this case specially */
      if (strcmp(thisFunction->Name, "GetBinaryOutputString") != 0 ||
        strcmp(thisClass->Name, "vtkDataWriter") != 0)
      {
        fprintf(fp,
          "  return vtkJavaMakeJArrayOfByte(env, reinterpret_cast<%sjbyte*>(temp%i), %i);\n",
          qualifier, MAX_ARGS, thisFunction->ReturnValue->Count);
      }
      else
      {
        fprintf(fp,
          "  return vtkJavaMakeJArrayOfByte(env, reinterpret_cast<%sjbyte*>(temp%i), "
          "op->GetOutputStringLength());\n",
          qualifier, MAX_ARGS);
      }
      break;
    case VTK_PARSE_BOOL_PTR:
      fprintf(fp,
        "  return vtkJavaMakeJArrayOfBoolean(env, reinterpret_cast<%sjboolean*>(temp%i), %i);\n",
        qualifier, MAX_ARGS, thisFunction->ReturnValue->Count);
      break;
    case VTK_PARSE_FLOAT_PTR:
      fprintf(fp, "  return vtkJavaMakeJArrayOfFloat(env, temp%i, %i);\n", MAX_ARGS,
        thisFunction->ReturnValue->Count);
      break;
    case VTK_PARSE_DOUBLE_PTR:
      fprintf(fp, "  return vtkJavaMakeJArrayOfDouble(env, temp%i, %i);\n", MAX_ARGS,
        thisFunction->ReturnValue->Count);
      break;
    case VTK_PARSE_CHAR_PTR:
    case VTK_PARSE_SIGNED_CHAR_PTR:
      fprintf(fp,
        "  return vtkJavaMakeJArrayOfByte(env, reinterpret_cast<%sjbyte*>(temp%i), %i);\n",
        qualifier, MAX_ARGS, thisFunction->ReturnValue->Count);
      break;
    case VTK_PARSE_SHORT_PTR:
    case VTK_PARSE_UNSIGNED_SHORT_PTR:
      fprintf(fp,
        "  return vtkJavaMakeJArrayOfShort(env, reinterpret_cast<%sjshort*>(temp%i), %i);\n",
        qualifier, MAX_ARGS, thisFunction->ReturnValue->Count);
      break;
    case VTK_PARSE_INT_PTR:
    case VTK_PARSE_UNSIGNED_INT_PTR:
      fprintf(fp, "  return vtkJavaMakeJArrayOfInt(env, reinterpret_cast<%sjint*>(temp%i), %i);\n",
        qualifier, MAX_ARGS, thisFunction->ReturnValue->Count);
      break;
    case VTK_PARSE_LONG_PTR:
    case VTK_PARSE_UNSIGNED_LONG_PTR:
    case VTK_PARSE_LONG_LONG_PTR:
    case VTK_PARSE_UNSIGNED_LONG_LONG_PTR:
    case VTK_PARSE___INT64_PTR:
    case VTK_PARSE_UNSIGNED___INT64_PTR:
      fprintf(fp,
        "  return vtkJavaMakeJArrayOfLong(env, reinterpret_cast<%sjlong*>(temp%i), %i);\n",
        qualifier, MAX_ARGS, thisFunction->ReturnValue->Count);
      break;
  }
}

void return_result(FILE* fp)
{
  unsigned int rType = (thisFunction->ReturnType & VTK_PARSE_UNQUALIFIED_TYPE);

  switch (rType)
  {
    case VTK_PARSE_FLOAT:
      fprintf(fp, "jfloat ");
      break;
    case VTK_PARSE_VOID:
      fprintf(fp, "void ");
      break;
    case VTK_PARSE_CHAR:
      fprintf(fp, "jchar ");
      break;
    case VTK_PARSE_DOUBLE:
      fprintf(fp, "jdouble ");
      break;
    case VTK_PARSE_SIGNED_CHAR:
    case VTK_PARSE_UNSIGNED_CHAR:
      fprintf(fp, "jbyte ");
      break;
    case VTK_PARSE_SHORT:
    case VTK_PARSE_UNSIGNED_SHORT:
      fprintf(fp, "jshort ");
      break;
    case VTK_PARSE_INT:
    case VTK_PARSE_UNSIGNED_INT:
      fprintf(fp, "jint ");
      break;
    case VTK_PARSE_UNKNOWN: /* enum */
      fprintf(fp, "jint ");
      break;
    case VTK_PARSE_LONG:
    case VTK_PARSE_LONG_LONG:
    case VTK_PARSE___INT64:
    case VTK_PARSE_UNSIGNED_LONG:
    case VTK_PARSE_UNSIGNED_LONG_LONG:
    case VTK_PARSE_UNSIGNED___INT64:
      fprintf(fp, "jlong ");
      break;
    case VTK_PARSE_BOOL:
      fprintf(fp, "jboolean ");
      break;
    case VTK_PARSE_SIGNED_CHAR_PTR:
    case VTK_PARSE_UNSIGNED_CHAR_PTR:
    case VTK_PARSE_CHAR_PTR:
    case VTK_PARSE_STRING:
    case VTK_PARSE_STRING_REF:
      fprintf(fp, "jbyteArray ");
      break;
    case VTK_PARSE_OBJECT_PTR:
      fprintf(fp, "jlong ");
      break;
    case VTK_PARSE_FLOAT_PTR:
      fprintf(fp, "jfloatArray ");
      break;
    case VTK_PARSE_DOUBLE_PTR:
      fprintf(fp, "jdoubleArray ");
      break;
    case VTK_PARSE_INT_PTR:
    case VTK_PARSE_UNSIGNED_INT_PTR:
      fprintf(fp, "jintArray ");
      break;
    case VTK_PARSE_SHORT_PTR:
    case VTK_PARSE_UNSIGNED_SHORT_PTR:
      fprintf(fp, "jshortArray ");
      break;
    case VTK_PARSE_LONG_PTR:
    case VTK_PARSE_LONG_LONG_PTR:
    case VTK_PARSE___INT64_PTR:
    case VTK_PARSE_UNSIGNED_LONG_LONG_PTR:
    case VTK_PARSE_UNSIGNED_LONG_PTR:
    case VTK_PARSE_UNSIGNED___INT64_PTR:
      fprintf(fp, "jlongArray ");
      break;
    case VTK_PARSE_BOOL_PTR:
      fprintf(fp, "jbooleanArray ");
      break;
  }
}

void OutputLocalVariableDeclarations(
  FILE* fp, int i, unsigned int aType, const char* Id, int aCount)
{
  /* handle VAR FUNCTIONS */
  if (aType == VTK_PARSE_FUNCTION)
  {
    fprintf(fp, "  vtkJavaVoidFuncArg* fstruct = new vtkJavaVoidFuncArg;\n");
    return;
  }

  /* ignore void */
  if ((aType & VTK_PARSE_UNQUALIFIED_TYPE) == VTK_PARSE_VOID)
  {
    return;
  }

  /* for const * return types prototype with const */
  if ((i == MAX_ARGS) && ((aType & VTK_PARSE_INDIRECT) != 0) && ((aType & VTK_PARSE_CONST) != 0))
  {
    fprintf(fp, "  const ");
  }
  else
  {
    fprintf(fp, "  ");
  }

  if ((aType & VTK_PARSE_UNSIGNED) != 0)
  {
    fprintf(fp, " unsigned ");
  }

  switch ((aType & VTK_PARSE_BASE_TYPE) & ~VTK_PARSE_UNSIGNED)
  {
    case VTK_PARSE_FLOAT:
      fprintf(fp, "float ");
      break;
    case VTK_PARSE_DOUBLE:
      fprintf(fp, "double ");
      break;
    case VTK_PARSE_INT:
      fprintf(fp, "int ");
      break;
    case VTK_PARSE_SHORT:
      fprintf(fp, "short ");
      break;
    case VTK_PARSE_LONG:
      fprintf(fp, "long ");
      break;
    case VTK_PARSE_VOID:
      fprintf(fp, "void ");
      break;
    case VTK_PARSE_CHAR:
      fprintf(fp, "char ");
      break;
    case VTK_PARSE_LONG_LONG:
      fprintf(fp, "long long ");
      break;
    case VTK_PARSE___INT64:
      fprintf(fp, "__int64 ");
      break;
    case VTK_PARSE_SIGNED_CHAR:
      fprintf(fp, "signed char ");
      break;
    case VTK_PARSE_BOOL:
      fprintf(fp, "bool ");
      break;
    case VTK_PARSE_OBJECT:
      fprintf(fp, "%s ", Id);
      break;
    case VTK_PARSE_STRING:
      fprintf(fp, "%s ", Id);
      break;
    case VTK_PARSE_UNKNOWN:
      fprintf(fp, "%s ", Id);
      break;
  }

  int FunctionReturnsObjectOrString = (i == MAX_ARGS) ||
    ((aType & VTK_PARSE_UNQUALIFIED_TYPE) == VTK_PARSE_OBJECT_PTR) ||
    ((aType & VTK_PARSE_UNQUALIFIED_TYPE) == VTK_PARSE_CHAR_PTR);

  switch (aType & VTK_PARSE_INDIRECT)
  {
    case VTK_PARSE_REF:
      if (i == MAX_ARGS)
      {
        fprintf(fp, "* "); /* act " &" */
      }
      break;
    case VTK_PARSE_POINTER:
      if (FunctionReturnsObjectOrString)
      {
        fprintf(fp, "* ");
      }
      break;
    default:
      fprintf(fp, "  ");
      break;
  }
  fprintf(fp, "temp%i", i);

  /* handle arrays */
  if (((aType & VTK_PARSE_INDIRECT) == VTK_PARSE_POINTER) && !FunctionReturnsObjectOrString)
  {
    fprintf(fp, "[%i]", aCount);
  }

  fprintf(fp, ";\n");
}

void OutputLocalVariableAssignments(FILE* fp, int i)
{
  unsigned int rawType = thisFunction->ArgTypes[i];

  /* handle VAR FUNCTIONS */
  if (rawType == VTK_PARSE_FUNCTION)
  {
    fprintf(fp, "  env->GetJavaVM(&(fstruct->vm));\n");
    fprintf(fp, "  fstruct->uobj = env->NewGlobalRef(id0);\n");
    fprintf(fp, "  char* handler = vtkJavaUTF8ToChars(env ,id1, len1);\n");
    fprintf(fp, "  fstruct->mid = env->GetMethodID(env->GetObjectClass(id0), handler, \"()V\");\n");
    fprintf(fp, "  delete[] handler;\n");
    return;
  }

  unsigned int basicType = rawType & VTK_PARSE_UNQUALIFIED_TYPE;
  /* ignore void */
  if (basicType == VTK_PARSE_VOID)
  {
    return;
  }

  switch (basicType)
  {
    case VTK_PARSE_CHAR:
      fprintf(fp, "  temp%i = static_cast<char>(0xff & id%i);\n", i, i);
      break;
    case VTK_PARSE_BOOL:
      fprintf(fp, "  temp%i = (id%i != 0) ? true : false;\n", i, i);
      break;
    case VTK_PARSE_CHAR_PTR:
      if (thisFunction->Parameters[i]->CountHint == NULL)
      {
        fprintf(fp, "  temp%i = vtkJavaUTF8ToChars(env, id%i, len%i);\n", i, i, i);
      }
      else
      {
        fprintf(fp,
          "  env->GetByteArrayRegion(id%i, 0, %i, reinterpret_cast<jbyte*>(&temp%i[0]));\n", i,
          thisFunction->Parameters[i]->Count, i);
      }
      break;
    case VTK_PARSE_STRING:
    case VTK_PARSE_STRING_REF:
      fprintf(fp, "  temp%i = vtkJavaUTF8ToString(env, id%i, len%i);\n", i, i, i);
      break;
    case VTK_PARSE_OBJECT_PTR:
      fprintf(fp, "  temp%i = static_cast<%s*>(vtkJavaGetPointerFromObject(env, id%i));\n", i,
        thisFunction->ArgClasses[i], i);
      break;
    case VTK_PARSE_FLOAT_PTR:
      fprintf(fp, "  env->GetFloatArrayRegion(id%i, 0, %i, &temp%i[0]);\n", i,
        thisFunction->Parameters[i]->Count, i);
      break;
    case VTK_PARSE_DOUBLE_PTR:
      fprintf(fp, "  env->GetDoubleArrayRegion(id%i, 0, %i, &temp%i[0]);\n", i,
        thisFunction->Parameters[i]->Count, i);
      break;
    case VTK_PARSE_SIGNED_CHAR_PTR:
    case VTK_PARSE_UNSIGNED_CHAR_PTR:
      fprintf(fp, "  env->GetByteArrayRegion(id%i, 0, %i, reinterpret_cast<jbyte*>(&temp%i[0]));\n",
        i, thisFunction->Parameters[i]->Count, i);
      break;
    case VTK_PARSE_SHORT_PTR:
    case VTK_PARSE_UNSIGNED_SHORT_PTR:
      fprintf(fp,
        "  env->GetShortArrayRegion(id%i, 0, %i, reinterpret_cast<jshort*>(&temp%i[0]));\n", i,
        thisFunction->Parameters[i]->Count, i);
      break;
    case VTK_PARSE_INT_PTR:
    case VTK_PARSE_UNSIGNED_INT_PTR:
      fprintf(fp, "  env->GetIntArrayRegion(id%i, 0, %i, reinterpret_cast<jint*>(&temp%i[0]));\n",
        i, thisFunction->Parameters[i]->Count, i);
      break;
    case VTK_PARSE_BOOL_PTR:
      fprintf(fp,
        "  env->GetBooleanArrayRegion(id%i, 0, %i, reinterpret_cast<jboolean*>(&temp%i[0]));\n", i,
        thisFunction->Parameters[i]->Count, i);
      break;
    case VTK_PARSE_LONG_PTR:
    case VTK_PARSE_UNSIGNED_LONG_PTR:
    case VTK_PARSE_LONG_LONG_PTR:
    case VTK_PARSE_UNSIGNED_LONG_LONG_PTR:
    case VTK_PARSE___INT64_PTR:
    case VTK_PARSE_UNSIGNED___INT64_PTR:
      fprintf(fp, "  env->GetLongArrayRegion(id%i, 0, %i, reinterpret_cast<jlong*>(&temp%i[0]));\n",
        i, thisFunction->Parameters[i]->Count, i);
      break;
    case VTK_PARSE_UNKNOWN:
      fprintf(fp, "  temp%i = static_cast<%s>(id%i);\n", i, thisFunction->ArgClasses[i], i);
      break;
    case VTK_PARSE_VOID:
    case VTK_PARSE_OBJECT:
    case VTK_PARSE_OBJECT_REF:
      break;
    default:
      fprintf(fp, "  temp%i = id%i;\n", i, i);
      break;
  }
}

void OutputCopyAndReleaseLocalVariables(FILE* fp, int i)
{
  unsigned int rawType = thisFunction->ArgTypes[i];

  /* handle VAR FUNCTIONS */
  if (rawType == VTK_PARSE_FUNCTION)
  {
    return;
  }

  unsigned int basicType = rawType & VTK_PARSE_UNQUALIFIED_TYPE;
  /* ignore void */
  if (basicType == VTK_PARSE_VOID)
  {
    return;
  }

  if ((basicType == VTK_PARSE_CHAR_PTR) && (thisFunction->Parameters[i]->CountHint == NULL))
  {
    fprintf(fp, "  delete[] temp%i;\n", i);
  }
  else
  {
    // only return values for non-const arrays
    if ((rawType & VTK_PARSE_CONST) == 0)
    {
      switch (basicType)
      {
        case VTK_PARSE_FLOAT_PTR:
          fprintf(fp, "  env->SetFloatArrayRegion(id%i, 0, %i, &temp%i[0]);\n", i,
            thisFunction->Parameters[i]->Count, i);
          break;
        case VTK_PARSE_DOUBLE_PTR:
          fprintf(fp, "  env->SetDoubleArrayRegion(id%i, 0, %i, &temp%i[0]);\n", i,
            thisFunction->Parameters[i]->Count, i);
          break;
        case VTK_PARSE_CHAR_PTR:
        case VTK_PARSE_SIGNED_CHAR_PTR:
        case VTK_PARSE_UNSIGNED_CHAR_PTR:
          fprintf(fp,
            "  env->SetByteArrayRegion(id%i, 0, %i, reinterpret_cast<jbyte*>(&temp%i[0]));\n", i,
            thisFunction->Parameters[i]->Count, i);
          break;
        case VTK_PARSE_SHORT_PTR:
        case VTK_PARSE_UNSIGNED_SHORT_PTR:
          fprintf(fp,
            "  env->SetShortArrayRegion(id%i, 0, %i, reinterpret_cast<jshort*>(&temp%i[0]));\n", i,
            thisFunction->Parameters[i]->Count, i);
          break;
        case VTK_PARSE_INT_PTR:
        case VTK_PARSE_UNSIGNED_INT_PTR:
          fprintf(fp,
            "  env->SetIntArrayRegion(id%i, 0, %i, reinterpret_cast<jint*>(&temp%i[0]));\n", i,
            thisFunction->Parameters[i]->Count, i);
          break;
        case VTK_PARSE_BOOL_PTR:
          fprintf(fp,
            "  env->SetBooleanArrayRegion(id%i, 0, %i, reinterpret_cast<jboolean*>(&temp%i[0]));\n",
            i, thisFunction->Parameters[i]->Count, i);
          break;
        case VTK_PARSE_LONG_PTR:
        case VTK_PARSE_UNSIGNED_LONG_PTR:
        case VTK_PARSE_LONG_LONG_PTR:
        case VTK_PARSE_UNSIGNED_LONG_LONG_PTR:
        case VTK_PARSE___INT64_PTR:
        case VTK_PARSE_UNSIGNED___INT64_PTR:
          fprintf(fp,
            "  env->SetLongArrayRegion(id%i, 0, %i, reinterpret_cast<jlong*>(&temp%i[0]));\n", i,
            thisFunction->Parameters[i]->Count, i);
          break;
        default:
          break;
      }
    }
  }
}

void OutputFunctionResult(FILE* fp)
{
  unsigned int rType = (thisFunction->ReturnType & VTK_PARSE_UNQUALIFIED_TYPE);

  /* ignore void */
  if (rType == VTK_PARSE_VOID)
  {
    return;
  }

  switch (rType)
  {
    case VTK_PARSE_CHAR_PTR:
    {
      if (thisFunction->ReturnValue->Count > 0)
      {
        use_hints(fp);
      }
      else
      {
        fprintf(fp,
          "  return (temp%i == nullptr) ? env->NewByteArray(0) : vtkJavaCharsToUTF8(env, temp%i, "
          "strlen(temp%i));\n",
          MAX_ARGS, MAX_ARGS, MAX_ARGS);
      }
      break;
    }
    case VTK_PARSE_STRING:
    {
      fprintf(fp, "  return vtkJavaStringToUTF8(env, temp%i);\n", MAX_ARGS);
      break;
    }
    case VTK_PARSE_STRING_REF:
    {
      fprintf(fp, "  return vtkJavaStringToUTF8(env, *temp%i);\n", MAX_ARGS);
      break;
    }
    case VTK_PARSE_OBJECT_PTR:
    {
      fprintf(fp, "  return reinterpret_cast<jlong>(temp%i);", MAX_ARGS);
      break;
    }

    /* handle functions returning vectors */
    /* this is done by looking them up in a hint file */
    case VTK_PARSE_FLOAT_PTR:
    case VTK_PARSE_DOUBLE_PTR:
    case VTK_PARSE_SIGNED_CHAR_PTR:
    case VTK_PARSE_UNSIGNED_CHAR_PTR:
    case VTK_PARSE_SHORT_PTR:
    case VTK_PARSE_UNSIGNED_SHORT_PTR:
    case VTK_PARSE_INT_PTR:
    case VTK_PARSE_UNSIGNED_INT_PTR:
    case VTK_PARSE_LONG_PTR:
    case VTK_PARSE_UNSIGNED_LONG_PTR:
    case VTK_PARSE_LONG_LONG_PTR:
    case VTK_PARSE_UNSIGNED_LONG_LONG_PTR:
    case VTK_PARSE___INT64_PTR:
    case VTK_PARSE_UNSIGNED___INT64_PTR:
    case VTK_PARSE_BOOL_PTR:
      use_hints(fp);
      break;

    /* handle enums, they are the only 'UNKNOWN' these wrappers use */
    case VTK_PARSE_UNKNOWN:
      fprintf(fp, "  return static_cast<jint>(temp%i);\n", MAX_ARGS);
      break;

    default:
      fprintf(fp, "  return temp%i;\n", MAX_ARGS);
      break;
  }
}

/* Check to see if two types will map to the same Java type,
 * return 1 if type1 should take precedence,
 * return 2 if type2 should take precedence,
 * return 0 if the types do not map to the same type */
static int CheckMatch(unsigned int type1, unsigned int type2, const char* c1, const char* c2)
{
  static unsigned int byteTypes[] = { VTK_PARSE_UNSIGNED_CHAR, VTK_PARSE_SIGNED_CHAR, 0 };
  static unsigned int shortTypes[] = { VTK_PARSE_UNSIGNED_SHORT, VTK_PARSE_SHORT, 0 };
  static unsigned int intTypes[] = { VTK_PARSE_UNSIGNED_INT, VTK_PARSE_INT, 0 };
  static unsigned int longTypes[] = { VTK_PARSE_UNSIGNED_LONG, VTK_PARSE_UNSIGNED_LONG_LONG,
    VTK_PARSE_UNSIGNED___INT64, VTK_PARSE_LONG, VTK_PARSE_LONG_LONG, VTK_PARSE___INT64, 0 };

  static unsigned int stringTypes[] = { VTK_PARSE_CHAR_PTR, VTK_PARSE_STRING_REF, VTK_PARSE_STRING,
    0 };

  static unsigned int* numericTypes[] = { byteTypes, shortTypes, intTypes, longTypes, 0 };

  int i, j;
  int hit1, hit2;

  if ((type1 & VTK_PARSE_UNQUALIFIED_TYPE) == (type2 & VTK_PARSE_UNQUALIFIED_TYPE))
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
        hit1 = j + 1;
      }
      if ((type2 & VTK_PARSE_BASE_TYPE) == numericTypes[i][j])
      {
        hit2 = j + 1;
      }
    }
    if (hit1 && hit2 && (type1 & VTK_PARSE_INDIRECT) == (type2 & VTK_PARSE_INDIRECT))
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
      hit1 = j + 1;
    }
    if ((type2 & VTK_PARSE_UNQUALIFIED_TYPE) == stringTypes[j])
    {
      hit2 = j + 1;
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
int DoneOne(void)
{
  int i, j;
  int match;
  FunctionInfo* fi;

  for (i = 0; i < numberOfWrappedFunctions; i++)
  {
    fi = wrappedFunctions[i];

    if ((!strcmp(fi->Name, thisFunction->Name)) &&
      (fi->NumberOfArguments == thisFunction->NumberOfArguments))
    {
      match = 1;
      for (j = 0; j < fi->NumberOfArguments; j++)
      {
        if (!CheckMatch(thisFunction->ArgTypes[j], fi->ArgTypes[j], thisFunction->ArgClasses[j],
              fi->ArgClasses[j]))
        {
          match = 0;
        }
      }
      if (!CheckMatch(
            thisFunction->ReturnType, fi->ReturnType, thisFunction->ReturnClass, fi->ReturnClass))
      {
        match = 0;
      }
      if (match)
        return 1;
    }
  }
  return 0;
}

void HandleDataReader(FILE* fp, ClassInfo* data)
{
  fprintf(fp, "\n");
  fprintf(fp, "extern \"C\" JNIEXPORT void");
  fprintf(fp, " JNICALL Java_vtk_%s_%s_1%i(JNIEnv* env, jobject obj, jbyteArray id0, jint id1)\n",
    data->Name, thisFunction->Name, numberOfWrappedFunctions);
  fprintf(fp, "{\n");
  fprintf(fp, "  %s* op = static_cast<%s*>(vtkJavaGetPointerFromObject(env, obj));\n", data->Name,
    data->Name);
  fprintf(fp, "  char* data = new char[id1];\n");
  fprintf(fp, "  env->GetByteArrayRegion(id0, 0, id1, reinterpret_cast<jbyte*>(&data[0]));\n");
  fprintf(fp, "  op->SetBinaryInputString(data, id1);\n");
  fprintf(fp, "  delete[] data;\n");
  fprintf(fp, "}\n");
}

void HandleDataArray(FILE* fp, ClassInfo* data)
{
  const char* type = 0;
  const char* jtype = 0;
  const char* jfromtype = 0;

  if (!strcmp("vtkCharArray", data->Name))
  {
    type = "char";
    jtype = "byte";
    jfromtype = "Byte";
  }
  else if (!strcmp("vtkDoubleArray", data->Name))
  {
    type = "double";
    jtype = type;
    jfromtype = "Double";
  }
  else if (!strcmp("vtkFloatArray", data->Name))
  {
    type = "float";
    jtype = type;
    jfromtype = "Float";
  }
  else if (!strcmp("vtkIntArray", data->Name))
  {
    type = "int";
    jtype = type;
    jfromtype = "Int";
  }
  else if (!strcmp("vtkLongArray", data->Name))
  {
    type = "long";
    jtype = type;
    jfromtype = "Long";
  }
  else if (!strcmp("vtkShortArray", data->Name))
  {
    type = "short";
    jtype = type;
    jfromtype = "Short";
  }
  else if (!strcmp("vtkSignedCharArray", data->Name))
  {
    type = "signed char";
    jtype = "byte";
    jfromtype = "Byte";
  }
  else if (!strcmp("vtkUnsignedCharArray", data->Name))
  {
    type = "unsigned char";
    jtype = "byte";
    jfromtype = "Byte";
  }
  else if (!strcmp("vtkUnsignedIntArray", data->Name))
  {
    type = "unsigned int";
    jtype = "int";
    jfromtype = "Int";
  }
  else if (!strcmp("vtkUnsignedLongArray", data->Name))
  {
    type = "unsigned long";
    jtype = "long";
    jfromtype = "Long";
  }
  else if (!strcmp("vtkUnsignedShortArray", data->Name))
  {
    type = "unsigned short";
    jtype = "short";
    jfromtype = "Short";
  }
  else
  {
    return;
  }

  fprintf(fp, "// Array conversion routines\n");
  fprintf(fp, "extern \"C\" JNIEXPORT ");
  fprintf(fp, "j%sArray JNICALL Java_vtk_%s_GetJavaArray_10(JNIEnv* env, jobject obj)\n", jtype,
    data->Name);
  fprintf(fp, "{\n");
  fprintf(fp, "  %s* op = static_cast<%s*>(vtkJavaGetPointerFromObject(env, obj));\n", data->Name,
    data->Name);
  fprintf(fp, "  %s* buffer = op->GetPointer(0);\n", type);
  fprintf(fp,
    "  return vtkJavaMakeJArrayOf%s(env, reinterpret_cast<j%s*>(buffer), op->GetSize());\n",
    jfromtype, jtype);
  fprintf(fp, "}\n\n");

  fprintf(fp, "extern \"C\" JNIEXPORT ");
  fprintf(fp,
    "void  JNICALL Java_vtk_%s_SetJavaArray_10(JNIEnv* env, jobject obj, j%sArray id0, jint "
    "len0)\n",
    data->Name, jtype);
  fprintf(fp, "{\n");
  fprintf(fp, "  %s* op = static_cast<%s*>(vtkJavaGetPointerFromObject(env, obj));\n", data->Name,
    data->Name);
  fprintf(fp, "  op->SetNumberOfTuples(len0 / op->GetNumberOfComponents());\n");
  fprintf(fp, "  %s* buffer = op->GetPointer(0);\n", type);
  fprintf(fp, "  env->Get%sArrayRegion(id0, 0, len0, reinterpret_cast<j%s*>(buffer));\n", jfromtype,
    jtype);
  fprintf(fp, "}\n");
}

static int isClassWrapped(const char* classname)
{
  HierarchyEntry* entry;

  if (hierarchyInfo)
  {
    entry = vtkParseHierarchy_FindEntry(hierarchyInfo, classname);

    if (entry == 0 || !vtkParseHierarchy_IsTypeOf(hierarchyInfo, entry, "vtkObjectBase"))
    {
      return 0;
    }
  }

  return 1;
}

int checkFunctionSignature(ClassInfo* data)
{
  static const unsigned int supported_types[] = { VTK_PARSE_VOID, VTK_PARSE_BOOL, VTK_PARSE_FLOAT,
    VTK_PARSE_DOUBLE, VTK_PARSE_CHAR, VTK_PARSE_UNSIGNED_CHAR, VTK_PARSE_SIGNED_CHAR, VTK_PARSE_INT,
    VTK_PARSE_UNSIGNED_INT, VTK_PARSE_SHORT, VTK_PARSE_UNSIGNED_SHORT, VTK_PARSE_LONG,
    VTK_PARSE_UNSIGNED_LONG, VTK_PARSE_LONG_LONG, VTK_PARSE_UNSIGNED_LONG_LONG, VTK_PARSE___INT64,
    VTK_PARSE_UNSIGNED___INT64, VTK_PARSE_OBJECT, VTK_PARSE_STRING, VTK_PARSE_UNKNOWN, 0 };

  int i, j;
  int args_ok = 1;
  unsigned int rType = (thisFunction->ReturnType & VTK_PARSE_UNQUALIFIED_TYPE);
  unsigned int aType = 0;
  unsigned int baseType = 0;

  /* some functions will not get wrapped no matter what else */
  if (thisFunction->IsOperator || thisFunction->ArrayFailure || thisFunction->IsExcluded ||
    thisFunction->IsDeleted || !thisFunction->IsPublic || !thisFunction->Name)
  {
    return 0;
  }

  /* NewInstance and SafeDownCast can not be wrapped because it is a
     (non-virtual) method which returns a pointer of the same type as
     the current pointer. Since all methods are virtual in Java, this
     looks like polymorphic return type.  */
  if (!strcmp("NewInstance", thisFunction->Name))
  {
    return 0;
  }

  if (!strcmp("SafeDownCast", thisFunction->Name))
  {
    return 0;
  }

  /* The GetInput() in vtkMapper cannot be overridden with a
   * different return type, Java doesn't allow this */
  if (strcmp(data->Name, "vtkMapper") == 0 && strcmp(thisFunction->Name, "GetInput") == 0)
  {
    return 0;
  }

  /* function pointer arguments for callbacks */
  if (thisFunction->NumberOfArguments == 2 && thisFunction->ArgTypes[0] == VTK_PARSE_FUNCTION &&
    thisFunction->ArgTypes[1] == VTK_PARSE_VOID_PTR && rType == VTK_PARSE_VOID)
  {
    return 1;
  }

  /* check to see if we can handle the args */
  for (i = 0; i < thisFunction->NumberOfArguments; i++)
  {
    aType = (thisFunction->ArgTypes[i] & VTK_PARSE_UNQUALIFIED_TYPE);
    baseType = (aType & VTK_PARSE_BASE_TYPE);

    for (j = 0; supported_types[j] != 0; j++)
    {
      if (baseType == supported_types[j])
      {
        break;
      }
    }
    if (supported_types[j] == 0)
    {
      args_ok = 0;
    }

    if (baseType == VTK_PARSE_UNKNOWN)
    {
      const char* qualified_name = 0;
      if ((aType & VTK_PARSE_INDIRECT) == 0)
      {
        qualified_name = vtkParseHierarchy_QualifiedEnumName(
          hierarchyInfo, data, stringCache, thisFunction->ArgClasses[i]);
      }
      if (qualified_name)
      {
        thisFunction->ArgClasses[i] = qualified_name;
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
      else if (!isClassWrapped(thisFunction->ArgClasses[i]))
      {
        args_ok = 0;
      }
    }

    if (aType == VTK_PARSE_OBJECT)
      args_ok = 0;
    if (((aType & VTK_PARSE_INDIRECT) != VTK_PARSE_POINTER) &&
      ((aType & VTK_PARSE_INDIRECT) != 0) && (aType != VTK_PARSE_STRING_REF))
      args_ok = 0;
    if (aType == VTK_PARSE_STRING_PTR)
      args_ok = 0;
    if (aType == VTK_PARSE_UNSIGNED_CHAR_PTR)
      args_ok = 0;
    if (aType == VTK_PARSE_UNSIGNED_INT_PTR)
      args_ok = 0;
    if (aType == VTK_PARSE_UNSIGNED_SHORT_PTR)
      args_ok = 0;
    if (aType == VTK_PARSE_UNSIGNED_LONG_PTR)
      args_ok = 0;
    if (aType == VTK_PARSE_UNSIGNED_LONG_LONG_PTR)
      args_ok = 0;
    if (aType == VTK_PARSE_UNSIGNED___INT64_PTR)
      args_ok = 0;
  }

  baseType = (rType & VTK_PARSE_BASE_TYPE);

  for (j = 0; supported_types[j] != 0; j++)
  {
    if (baseType == supported_types[j])
    {
      break;
    }
  }
  if (supported_types[j] == 0)
  {
    args_ok = 0;
  }

  if (baseType == VTK_PARSE_UNKNOWN)
  {
    const char* qualified_name = 0;
    if ((rType & VTK_PARSE_INDIRECT) == 0)
    {
      qualified_name = vtkParseHierarchy_QualifiedEnumName(
        hierarchyInfo, data, stringCache, thisFunction->ReturnClass);
    }
    if (qualified_name)
    {
      thisFunction->ReturnClass = qualified_name;
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
    else if (!isClassWrapped(thisFunction->ReturnClass))
    {
      args_ok = 0;
    }
  }

  if (((rType & VTK_PARSE_INDIRECT) != VTK_PARSE_POINTER) && ((rType & VTK_PARSE_INDIRECT) != 0) &&
    (rType != VTK_PARSE_STRING_REF))
    args_ok = 0;
  if (rType == VTK_PARSE_STRING_PTR)
    args_ok = 0;

  /* eliminate unsigned char/short/int/long/int64 pointers */
  if (rType == VTK_PARSE_UNSIGNED_CHAR_PTR)
    args_ok = 0;
  if (rType == VTK_PARSE_UNSIGNED_INT_PTR)
    args_ok = 0;
  if (rType == VTK_PARSE_UNSIGNED_SHORT_PTR)
    args_ok = 0;
  if (rType == VTK_PARSE_UNSIGNED_LONG_PTR)
    args_ok = 0;
  if (rType == VTK_PARSE_UNSIGNED_LONG_LONG_PTR)
    args_ok = 0;
  if (rType == VTK_PARSE_UNSIGNED___INT64_PTR)
    args_ok = 0;

  /* make sure we have all the info we need for array arguments in */
  for (i = 0; i < thisFunction->NumberOfArguments; i++)
  {
    aType = (thisFunction->ArgTypes[i] & VTK_PARSE_UNQUALIFIED_TYPE);

    if (((aType & VTK_PARSE_INDIRECT) == VTK_PARSE_POINTER) &&
      (thisFunction->Parameters[i]->Count <= 0) && (aType != VTK_PARSE_OBJECT_PTR) &&
      (aType != VTK_PARSE_CHAR_PTR))
      args_ok = 0;
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
    case VTK_PARSE_LONG_LONG_PTR:
    case VTK_PARSE___INT64_PTR:
    case VTK_PARSE_SIGNED_CHAR_PTR:
    case VTK_PARSE_BOOL_PTR:
    case VTK_PARSE_UNSIGNED_CHAR_PTR:
      args_ok = thisFunction->HaveHint;
      break;
  }

  /* make sure there isn't a Java-specific override */
  if (!strcmp("vtkObject", data->Name))
  {
    /* remove the original vtkCommand observer methods */
    if (!strcmp(thisFunction->Name, "AddObserver") || !strcmp(thisFunction->Name, "GetCommand") ||
      (!strcmp(thisFunction->Name, "RemoveObserver") &&
        (thisFunction->ArgTypes[0] != VTK_PARSE_UNSIGNED_LONG)) ||
      ((!strcmp(thisFunction->Name, "RemoveObservers") ||
         !strcmp(thisFunction->Name, "HasObserver")) &&
        (((thisFunction->ArgTypes[0] != VTK_PARSE_UNSIGNED_LONG) &&
           (thisFunction->ArgTypes[0] != (VTK_PARSE_CHAR_PTR | VTK_PARSE_CONST))) ||
          (thisFunction->NumberOfArguments > 1))) ||
      (!strcmp(thisFunction->Name, "RemoveAllObservers") && (thisFunction->NumberOfArguments > 0)))
    {
      args_ok = 0;
    }
  }
  else if (!strcmp("vtkObjectBase", data->Name))
  {
    /* remove the special vtkObjectBase methods */
    if (!strcmp(thisFunction->Name, "Print"))
    {
      args_ok = 0;
    }
  }

  /* make sure it isn't a Delete or New function */
  if (!strcmp("Delete", thisFunction->Name) || !strcmp("New", thisFunction->Name))
  {
    args_ok = 0;
  }

  return args_ok;
}

void outputFunction(FILE* fp, ClassInfo* data)
{
  int i;
  int args_ok;
  unsigned int rType = (thisFunction->ReturnType & VTK_PARSE_UNQUALIFIED_TYPE);
  const char* jniFunction = 0;
  char* jniFunctionNew = 0;
  char* jniFunctionOld = 0;
  size_t j;
  thisClass = data;

  args_ok = checkFunctionSignature(data);

  /* handle DataReader SetBinaryInputString as a special case */
  if (!strcmp("SetBinaryInputString", thisFunction->Name) &&
    (!strcmp("vtkDataReader", data->Name) || !strcmp("vtkStructuredGridReader", data->Name) ||
      !strcmp("vtkRectilinearGridReader", data->Name) ||
      !strcmp("vtkUnstructuredGridReader", data->Name) ||
      !strcmp("vtkStructuredPointsReader", data->Name) || !strcmp("vtkPolyDataReader", data->Name)))
  {
    HandleDataReader(fp, data);
    wrappedFunctions[numberOfWrappedFunctions] = thisFunction;
    numberOfWrappedFunctions++;
  }

  if (!thisFunction->IsExcluded && thisFunction->IsPublic && args_ok &&
    strcmp(data->Name, thisFunction->Name) != 0 && strcmp(data->Name, thisFunction->Name + 1) != 0)
  {
    /* make sure we haven't already done one of these */
    if (!DoneOne())
    {
      fprintf(fp, "\n");

      /* Underscores are escaped in method names, see
           http://java.sun.com/javase/6/docs/technotes/guides/jni/spec/design.html#wp133
         VTK class names contain no underscore and do not need to be escaped.  */
      jniFunction = thisFunction->Name;
      jniFunctionOld = 0;
      j = 0;
      while (jniFunction[j] != '\0')
      {
        /* replace "_" with "_1" */
        if (jniFunction[j] == '_')
        {
          j++;
          jniFunctionNew = (char*)malloc(strlen(jniFunction) + 2);
          strncpy(jniFunctionNew, jniFunction, j);
          jniFunctionNew[j] = '1';
          strcpy(&jniFunctionNew[j + 1], &jniFunction[j]);
          free(jniFunctionOld);
          jniFunctionOld = jniFunctionNew;
          jniFunction = jniFunctionNew;
        }
        j++;
      }

      fprintf(fp, "extern \"C\" JNIEXPORT ");
      return_result(fp);
      fprintf(fp, " JNICALL Java_vtk_%s_%s_1%i(JNIEnv* env, jobject obj", data->Name, jniFunction,
        numberOfWrappedFunctions);

      for (i = 0; i < thisFunction->NumberOfArguments; i++)
      {
        fprintf(fp, ",");
        OutputParamDeclarations(fp, i);

        /* ignore args after function pointer */
        if (thisFunction->ArgTypes[i] == VTK_PARSE_FUNCTION)
        {
          break;
        }
      }
      fprintf(fp, ")\n{\n");

      /* process the args */
      for (i = 0; i < thisFunction->NumberOfArguments; i++)
      {
        OutputLocalVariableDeclarations(fp, i, thisFunction->ArgTypes[i],
          thisFunction->ArgClasses[i], thisFunction->ArgCounts[i]);

        /* ignore args after function pointer */
        if (thisFunction->ArgTypes[i] == VTK_PARSE_FUNCTION)
        {
          break;
        }
      }
      OutputLocalVariableDeclarations(
        fp, MAX_ARGS, thisFunction->ReturnType, thisFunction->ReturnClass, 0);

      /* now get the required args from the stack */
      for (i = 0; i < thisFunction->NumberOfArguments; i++)
      {
        OutputLocalVariableAssignments(fp, i);

        /* ignore args after function pointer */
        if (thisFunction->ArgTypes[i] == VTK_PARSE_FUNCTION)
        {
          break;
        }
      }

      fprintf(fp, "\n  %s* op = static_cast<%s*>(vtkJavaGetPointerFromObject(env, obj));\n",
        data->Name, data->Name);

      switch (rType)
      {
        case VTK_PARSE_VOID:
          fprintf(fp, "  op->%s(", thisFunction->Name);
          break;
        default:
          if ((rType & VTK_PARSE_INDIRECT) == VTK_PARSE_REF)
          {
            fprintf(fp, "  temp%i = &(op)->%s(", MAX_ARGS, thisFunction->Name);
          }
          else
          {
            fprintf(fp, "  temp%i = op->%s(", MAX_ARGS, thisFunction->Name);
          }
          break;
      }

      for (i = 0; i < thisFunction->NumberOfArguments; i++)
      {
        if (i)
        {
          fprintf(fp, ",");
        }
        if (thisFunction->ArgTypes[i] == VTK_PARSE_FUNCTION)
        {
          fprintf(fp, "vtkJavaVoidFunc,fstruct");
          break;
        }
        else
        {
          fprintf(fp, "temp%i", i);
        }
      } /* for */

      fprintf(fp, ");\n");

      if (thisFunction->NumberOfArguments == 2 && thisFunction->ArgTypes[0] == VTK_PARSE_FUNCTION)
      {
        fprintf(fp, "  op->%sArgDelete(vtkJavaVoidFuncArgDelete);\n", jniFunction);
      }

      /* now copy and release any arrays */
      for (i = 0; i < thisFunction->NumberOfArguments; i++)
      {
        OutputCopyAndReleaseLocalVariables(fp, i);

        /* ignore args after function pointer */
        if (thisFunction->ArgTypes[i] == VTK_PARSE_FUNCTION)
        {
          break;
        }
      }
      OutputFunctionResult(fp);
      fprintf(fp, "\n}\n");

      wrappedFunctions[numberOfWrappedFunctions] = thisFunction;
      numberOfWrappedFunctions++;
      if (jniFunctionNew)
      {
        free(jniFunctionNew);
        jniFunctionNew = 0;
      }
    } /* isDone() */
  }   /* isAbstract */
}

/* print the parsed structures */
int VTK_PARSE_MAIN(int argc, char* argv[])
{
  OptionInfo* options;
  FileInfo* file_info;
  ClassInfo* data;
  FILE* fp;
  int i;

  /* pre-define a macro to identify the language */
  vtkParse_DefineMacro("__VTK_WRAP_JAVA__", 0);

  /* get command-line args and parse the header file */
  file_info = vtkParse_Main(argc, argv);

  /* some utility functions require the string cache */
  stringCache = file_info->Strings;

  /* get the command-line options */
  options = vtkParse_GetCommandLineOptions();

  /* get the hierarchy info for accurate typing */
  if (options->HierarchyFileNames)
  {
    hierarchyInfo =
      vtkParseHierarchy_ReadFiles(options->NumberOfHierarchyFileNames, options->HierarchyFileNames);
  }

  /* get the output file */
  fp = vtkParse_FileOpen(options->OutputFileName, "w");

  if (!fp)
  {
    fprintf(stderr, "Error opening output file %s\n", options->OutputFileName);
    exit(1);
  }

  /* get the main class */
  data = file_info->MainClass;
  if (data == NULL || data->IsExcluded)
  {
    fclose(fp);
    exit(0);
  }

  if (data->Template)
  {
    fclose(fp);
    exit(0);
  }

  for (i = 0; i < data->NumberOfSuperClasses; ++i)
  {
    if (strchr(data->SuperClasses[i], '<'))
    {
      fclose(fp);
      exit(0);
    }
  }

  if (hierarchyInfo)
  {
    if (!vtkWrap_IsTypeOf(hierarchyInfo, data->Name, "vtkObjectBase"))
    {
      fclose(fp);
      exit(0);
    }

    /* resolve using declarations within the header files */
    vtkWrap_ApplyUsingDeclarations(data, file_info, hierarchyInfo);

    /* expand typedefs */
    vtkWrap_ExpandTypedefs(data, file_info, hierarchyInfo);
  }

  fprintf(fp, "// java wrapper for %s object\n//\n", data->Name);
  fprintf(fp, "#define VTK_WRAPPING_CXX\n");
  if (strcmp("vtkObjectBase", data->Name) != 0)
  {
    /* Block inclusion of full streams.  */
    fprintf(fp, "#define VTK_STREAMS_FWD_ONLY\n");
  }
  fprintf(fp, "#include \"vtkSystemIncludes.h\"\n");
  fprintf(fp, "#include \"%s.h\"\n", data->Name);
  fprintf(fp, "#include \"vtkJavaUtil.h\"\n\n");
  fprintf(fp, "#include \"vtkStdString.h\"\n\n");
  fprintf(fp, "#include <sstream>\n");

  for (i = 0; i < data->NumberOfSuperClasses; i++)
  {
    char* safe_name = vtkWrap_SafeSuperclassName(data->SuperClasses[i]);
    const char* safe_superclass = safe_name ? safe_name : data->SuperClasses[i];

    /* if a template class is detected add a typedef */
    if (safe_name)
    {
      fprintf(fp, "typedef %s %s;\n", data->SuperClasses[i], safe_name);
    }

    fprintf(
      fp, "extern \"C\" JNIEXPORT void* %s_Typecast(void* op,char* dType);\n", safe_superclass);

    free(safe_name);
  }

  fprintf(fp, "\nextern \"C\" JNIEXPORT void* %s_Typecast(void* me,char* dType)\n{\n", data->Name);
  if (data->NumberOfSuperClasses > 0)
  {
    fprintf(fp, "  void* res;\n");
  }
  fprintf(fp, "  if (!strcmp(\"%s\",dType)) { return me; }\n", data->Name);
  /* check our superclasses */
  for (i = 0; i < data->NumberOfSuperClasses; i++)
  {
    char* safe_name = vtkWrap_SafeSuperclassName(data->SuperClasses[i]);
    const char* safe_superclass = safe_name ? safe_name : data->SuperClasses[i];

    fprintf(fp, "  if ((res= %s_Typecast(me,dType)) != nullptr)", safe_superclass);
    fprintf(fp, " { return res; }\n");

    free(safe_name);
  }
  fprintf(fp, "  return nullptr;\n");
  fprintf(fp, "}\n\n");

  HandleDataArray(fp, data);

  /* insert function handling code here */
  for (i = 0; i < data->NumberOfFunctions; i++)
  {
    thisFunction = data->Functions[i];
    outputFunction(fp, data);
  }

  if ((!data->NumberOfSuperClasses) && (data->HasDelete))
  {
    fprintf(fp, "\nextern \"C\" JNIEXPORT ");
    fprintf(
      fp, "void JNICALL Java_vtk_%s_VTKDeleteReference(JNIEnv*,jclass,jlong id)\n", data->Name);
    fprintf(fp, "  {\n");
    fprintf(fp, "  %s* op = reinterpret_cast<%s*>(id);\n", data->Name, data->Name);
    fprintf(fp, "  op->Delete();\n");
    fprintf(fp, "}\n");

    fprintf(fp, "\nextern \"C\" JNIEXPORT ");
    fprintf(fp,
      "jbyteArray JNICALL Java_vtk_%s_VTKGetClassNameBytesFromReference(JNIEnv* env,jclass,jlong "
      "id)\n",
      data->Name);
    fprintf(fp, "{\n");
    fprintf(fp, "  const char* name = \"\";\n");
    fprintf(fp, "  if(id != 0)\n");
    fprintf(fp, "  {\n");
    fprintf(fp, "    %s* op = reinterpret_cast<%s*>(id);\n", data->Name, data->Name);
    fprintf(fp, "    name = op->GetClassName();\n");
    fprintf(fp, "  }\n");
    fprintf(fp,
      "  return (name == nullptr) ? env->NewByteArray(0) : vtkJavaCharsToUTF8(env, name, "
      "strlen(name));\n");
    fprintf(fp, "}\n");

    fprintf(fp, "\nextern \"C\" JNIEXPORT ");
    fprintf(fp, "void JNICALL Java_vtk_%s_VTKDelete(JNIEnv* env,jobject obj)\n", data->Name);
    fprintf(fp, "  {\n");
    fprintf(fp, "  %s* op = static_cast<%s*>(vtkJavaGetPointerFromObject(env, obj));\n", data->Name,
      data->Name);
    fprintf(fp, "  op->Delete();\n");
    fprintf(fp, "}\n");

    fprintf(fp, "\nextern \"C\" JNIEXPORT ");
    fprintf(fp, "void JNICALL Java_vtk_%s_VTKRegister(JNIEnv* env,jobject obj)\n", data->Name);
    fprintf(fp, "  {\n");
    fprintf(fp, " %s*  op = static_cast<%s*>(vtkJavaGetPointerFromObject(env, obj));\n", data->Name,
      data->Name);
    fprintf(fp, "  op->Register(op);\n");
    fprintf(fp, "}\n");
  }
  if (!data->IsAbstract)
  {
    fprintf(fp, "\nextern \"C\" JNIEXPORT ");
    fprintf(fp, "jlong JNICALL Java_vtk_%s_VTKInit(JNIEnv*, jobject)", data->Name);
    fprintf(fp, "\n{");
    fprintf(fp, "\n  return reinterpret_cast<jlong>(%s::New());", data->Name);
    fprintf(fp, "\n}\n");
  }

  /* for vtkRenderWindow we want to add a special method to support
   * native AWT rendering
   *
   * Including vtkJavaAwt.h provides inline implementations of
   * Java_vtk_vtkPanel_RenderCreate, Java_vtk_vtkPanel_Lock and
   * Java_vtk_vtkPanel_UnLock. */
  if (!strcmp("vtkRenderWindow", data->Name))
  {
    fprintf(fp, "\n#include \"vtkJavaAwt.h\"\n\n");
  }

  if (!strcmp("vtkObjectBase", data->Name))
  {
    /* Add the Print method to vtkObjectBase. */
    fprintf(fp, "\nextern \"C\" JNIEXPORT ");
    fprintf(fp, "jbyteArray JNICALL Java_vtk_vtkObjectBase_PrintBytes(JNIEnv* env, jobject obj)\n");
    fprintf(fp, "{\n");
    fprintf(fp,
      "  vtkObjectBase* op = static_cast<vtkObjectBase*>(vtkJavaGetPointerFromObject(env, "
      "obj));\n");
    fprintf(fp, "  std::ostringstream stream;\n");
    fprintf(fp, "  op->Print(stream);\n");
    fprintf(fp, "  stream.put('\\0');\n");
    fprintf(fp, "  return vtkJavaStringToUTF8(env, stream.str());\n");
    fprintf(fp, "}\n");
  }

  if (!strcmp("vtkObject", data->Name))
  {
    fprintf(fp, "\nextern \"C\" JNIEXPORT ");
    fprintf(fp,
      "jlong JNICALL Java_vtk_vtkObject_AddObserver(JNIEnv* env, jobject "
      "obj, jbyteArray id0, jint len0, jobject id1, jbyteArray id2, jint len2)\n");
    fprintf(fp, "{\n");
    fprintf(fp, "  vtkJavaCommand* command = vtkJavaCommand::New();\n");
    fprintf(fp, "  command->AssignJavaVM(env);\n");
    fprintf(fp, "  command->SetGlobalRef(env->NewGlobalRef(id1));\n");
    fprintf(fp, "  char* handler = vtkJavaUTF8ToChars(env, id2, len2);\n");
    fprintf(fp, "  jclass classtype = env->GetObjectClass(id1);\n");
    fprintf(fp, "  command->SetMethodID(env->GetMethodID(classtype, handler,\"()V\"));\n");
    fprintf(fp, "  delete[] handler;\n");
    fprintf(fp, "  char* event = vtkJavaUTF8ToChars(env, id0, len0);\n");
    fprintf(
      fp, "  vtkObject* op = static_cast<vtkObject*>(vtkJavaGetPointerFromObject(env, obj));\n");
    fprintf(fp, "  unsigned long result = op->AddObserver(event, command);\n");
    fprintf(fp, "  delete[] event;\n");
    fprintf(fp, "  command->Delete();\n");
    fprintf(fp, "  return result;\n");
    fprintf(fp, "}\n");
  }

  vtkParse_Free(file_info);

  fclose(fp);

  return 0;
}
