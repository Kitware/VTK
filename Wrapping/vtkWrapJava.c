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
  /* ignore void */
  if (((currentFunction->ArgTypes[i] % 0x10) == 0x2)&&
      (!((currentFunction->ArgTypes[i] % 0x1000)/0x100)))
    {
    return;
    }
  
  if (currentFunction->ArgTypes[i] == 0x5000)
    {
    fprintf(fp,"jobject id0, jstring id1");
    return;
    }
  
  if (currentFunction->ArgTypes[i] % 0x1000 == 0x303)
    {
    fprintf(fp,"jstring ");
    fprintf(fp,"id%i",i);
    return;
    }
  
  if ((currentFunction->ArgTypes[i] % 0x1000 == 0x301)||(currentFunction->ArgTypes[i] % 0x1000 == 0x307))
    {
    fprintf(fp,"jdoubleArray ");
    fprintf(fp,"id%i",i);
    return;
    }
  
  if ((currentFunction->ArgTypes[i] % 0x1000 == 0x304)||
      (currentFunction->ArgTypes[i] % 0x1000 == 0x306)||
      (currentFunction->ArgTypes[i] % 0x1000 == 0x30A)||
      (currentFunction->ArgTypes[i] % 0x1000 == 0x30B)||
      (currentFunction->ArgTypes[i] % 0x1000 == 0x30C))
    {
    fprintf(fp,"jintArray ");
    fprintf(fp,"id%i",i);
    return;
    }


  switch (currentFunction->ArgTypes[i] % 0x10)
    {
    case 0x1:   fprintf(fp,"jdouble "); break;
    case 0x7:   fprintf(fp,"jdouble "); break;
    case 0x4:   fprintf(fp,"jint "); break;
    case 0x5:   fprintf(fp,"jint "); break;
    case 0x6:   fprintf(fp,"jint "); break;
    case 0xA:   fprintf(fp,"jint "); break;
    case 0xB:   fprintf(fp,"jint "); break;
    case 0xC:   fprintf(fp,"jint "); break;
    case 0xD:     fprintf(fp,"jint "); break;
    case 0xE:     fprintf(fp,"jboolean "); break;
    case 0x2:     fprintf(fp,"void "); break;
    case 0x3:     fprintf(fp,"jchar "); break;
    case 0x9:     fprintf(fp,"jobject "); break;
    case 0x8: return;
    }
  
  fprintf(fp,"id%i",i);
}

/* when the cpp file doesn't have enough info use the hint file */
void use_hints(FILE *fp)
{
  /* use the hint */
  switch (currentFunction->ReturnType % 0x1000)
    {
    case 0x313:
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
    case 0x301:
      fprintf(fp,"    return vtkJavaMakeJArrayOfDoubleFromFloat(env,temp%i,%i);\n",
              MAX_ARGS, currentFunction->HintSize);
      break;
      
    case 0x307:  
      fprintf(fp,"    return vtkJavaMakeJArrayOfDoubleFromDouble(env,temp%i,%i);\n",
              MAX_ARGS, currentFunction->HintSize);
      break;
      
    case 0x304: 
      fprintf(fp,"    return vtkJavaMakeJArrayOfIntFromInt(env,temp%i,%i);\n",
              MAX_ARGS, currentFunction->HintSize);
      break;
      
    case 0x30A: 
      fprintf(fp,"    return vtkJavaMakeJArrayOfIntFromIdType(env,temp%i,%i);\n",
              MAX_ARGS, currentFunction->HintSize);
      break;
    case 0x30B:
      fprintf(fp,"    return vtkJavaMakeJArrayOfIntFromLongLong(env,temp%i,%i);\n",
              MAX_ARGS, currentFunction->HintSize);
      break;
    case 0x30C:
      fprintf(fp,"    return vtkJavaMakeJArrayOfIntFrom__Int64(env,temp%i,%i);\n",
              MAX_ARGS, currentFunction->HintSize);
      break;
    case 0x30D:
      fprintf(fp,"    return vtkJavaMakeJArrayOfIntFromSignedChar(env,temp%i,%i);\n",
              MAX_ARGS, currentFunction->HintSize);
      break;
    case 0x30E:
      fprintf(fp,"    return vtkJavaMakeJArrayOfIntFromBool(env,temp%i,%i);\n",
              MAX_ARGS, currentFunction->HintSize);
      break;
    case 0x305: case 0x306: case 0x314: case 0x315: case 0x316:
    case 0x31A: case 0x31B: case 0x31C:
      break;
    }
}

void return_result(FILE *fp)
{
  switch (currentFunction->ReturnType % 0x1000)
    {
    case 0x1: fprintf(fp,"jdouble "); break;
    case 0x2: fprintf(fp,"void "); break;
    case 0x3: fprintf(fp,"jchar "); break;
    case 0x7: fprintf(fp,"jdouble "); break;
    case 0x4: case 0x5: case 0x6: case 0xA: case 0xB: case 0xC: case 0xD:
    case 0x13: case 0x14: case 0x15: case 0x16: case 0x1A: case 0x1B: case 0x1C:
      fprintf(fp,"jint "); 
      break;
    case 0xE:
      fprintf(fp,"jboolean ");
      break;
    case 0x303: fprintf(fp,"jstring "); break;
    case 0x109:
    case 0x309:  
      fprintf(fp,"jlong "); break;
      
    case 0x301: case 0x307: case 0x313:
    case 0x304: case 0x305: case 0x306: case 0x30A: case 0x30B: case 0x30C:
    case 0x30D: case 0x30E: case 0x31A: case 0x31B: case 0x31C:
      fprintf(fp,"jarray "); break;
    }
}


void output_temp(FILE *fp, int i, int aType, char *Id, int aCount)
{
  /* handle VAR FUNCTIONS */
  if (aType == 0x5000)
    {
    fprintf(fp,"  vtkJavaVoidFuncArg *temp%i = new vtkJavaVoidFuncArg;\n",i);
    return;
    }
  
  /* ignore void */
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
    fprintf(fp," unsigned ");
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
    case 0xA:   fprintf(fp,"vtkIdType "); break;
    case 0xB:   fprintf(fp,"long long "); break;
    case 0xC:   fprintf(fp,"__int64 "); break;
    case 0xD:     fprintf(fp,"signed char "); break;
    case 0xE:     fprintf(fp,"bool "); break;
    case 0x9:     
      fprintf(fp,"%s ",Id); break;
    case 0x8: return;
    }
  
  switch ((aType % 0x1000)/0x100)
    {
    case 0x1: fprintf(fp, " *"); break; /* act " &" */
    case 0x2: fprintf(fp, "&&"); break;
    case 0x3: 
      if ((i == MAX_ARGS)||(aType % 0x10 == 0x9)||(aType % 0x1000 == 0x303)) 
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
      (i != MAX_ARGS)&&(aType % 0x10 != 0x9)&&(aType % 0x1000 != 0x303))
    {
    fprintf(fp,"[%i]",aCount);
    fprintf(fp,";\n  void *tempArray%i",i);
    }

  fprintf(fp,";\n");
}

void get_args(FILE *fp, int i)
{
  int j;
  
  /* handle VAR FUNCTIONS */
  if (currentFunction->ArgTypes[i] == 0x5000)
    {
    fprintf(fp,"  env->GetJavaVM(&(temp%i->vm));\n",i);
    fprintf(fp,"  temp%i->uobj = env->NewGlobalRef(id0);\n",i);
    fprintf(fp,"  char *temp%i_str;\n",i);
    fprintf(fp,"  temp%i_str = vtkJavaUTFToChar(env,id1);\n",i);
    fprintf(fp,"  temp%i->mid = env->GetMethodID(env->GetObjectClass(id0),temp%i_str,\"()V\");\n",i,i);
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
    case 0x3:
      fprintf(fp,"  temp%i = (char)(0xff & id%i);\n",i,i);
      break;
    case 0xE:
      fprintf(fp,"  temp%i = (id%i != 0) ? true : false;\n",i,i);
      break;
    case 0x303:
      fprintf(fp,"  temp%i = vtkJavaUTFToChar(env,id%i);\n",i,i);
      break;
    case 0x109:
    case 0x309:
      fprintf(fp,"  temp%i = (%s *)(vtkJavaGetPointerFromObject(env,id%i));\n",i,currentFunction->ArgClasses[i],i);
      break;
    case 0x301:
    case 0x307:
      fprintf(fp,"  tempArray%i = (void *)(env->GetDoubleArrayElements(id%i,NULL));\n",i,i);
      for (j = 0; j < currentFunction->ArgCounts[i]; j++)
        {
        fprintf(fp,"  temp%i[%i] = ((jdouble *)tempArray%i)[%i];\n",i,j,i,j);
        }
      break;
    case 0x304:
    case 0x306:
    case 0x30A:
    case 0x30B:
    case 0x30C:
    case 0x30D:
    case 0x30E:
      fprintf(fp,"  tempArray%i = (void *)(env->GetIntArrayElements(id%i,NULL));\n",i,i);
      for (j = 0; j < currentFunction->ArgCounts[i]; j++)
        {
        fprintf(fp,"  temp%i[%i] = ((jint *)tempArray%i)[%i];\n",i,j,i,j);
        }
      break;
    case 0x2:    
    case 0x9: break;
    default: fprintf(fp,"  temp%i = id%i;\n",i,i); break;
    }
}


void copy_and_release_args(FILE *fp, int i)
{
  int j;
  
  /* handle VAR FUNCTIONS */
  if (currentFunction->ArgTypes[i] == 0x5000)
    {
    fprintf(fp,"  if (temp%i_str) delete[] temp%i_str;\n",i,i);
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
    case 0x301:
    case 0x307:
      for (j = 0; j < currentFunction->ArgCounts[i]; j++)
        {
        fprintf(fp,"  ((jdouble *)tempArray%i)[%i] = temp%i[%i];\n",i,j,i,j);
        }
      fprintf(fp,"  env->ReleaseDoubleArrayElements(id%i,(jdouble *)tempArray%i,0);\n",i,i);      
      break;
    case 0x303:
      fprintf(fp,"  if (temp%i) delete[] temp%i;\n",i,i);
      break;
    case 0x304:
    case 0x306:
    case 0x30A:
    case 0x30B:
    case 0x30C:
    case 0x30D:
    case 0x30E:
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
  /* ignore void */
  if (((currentFunction->ReturnType % 0x10) == 0x2)&&(!((currentFunction->ReturnType % 0x1000)/0x100)))
    {
    return;
    }

  switch (currentFunction->ReturnType % 0x1000)
    {
    case 0x303: 
      {
      fprintf(fp,"  return vtkJavaMakeJavaString(env,temp%i);\n",
              MAX_ARGS); 
      }
    break;
    case 0x109:
    case 0x309:  
      {
      fprintf(fp,"  return (jlong)(size_t)temp%i;", MAX_ARGS);
      break;
      }
      
    /* handle functions returning vectors */
    /* this is done by looking them up in a hint file */
    case 0x301: case 0x307: case 0x313:
    case 0x304: case 0x305: case 0x306:
    case 0x30A: case 0x30B: case 0x30C: case 0x30D: case 0x30E:
      use_hints(fp);
      break;
    default: fprintf(fp,"  return temp%i;\n", MAX_ARGS); break;
    }
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
        if ((fi->ArgTypes[j] != currentFunction->ArgTypes[j]) &&
            !(((fi->ArgTypes[j] % 0x1000 == 0x309)&&
               (currentFunction->ArgTypes[j] % 0x1000 == 0x109)) ||
              ((fi->ArgTypes[j] % 0x1000 == 0x109)&&
               (currentFunction->ArgTypes[j] % 0x1000 == 0x309)) ||
              ((fi->ArgTypes[j] % 0x1000 == 0x301)&&
               (currentFunction->ArgTypes[j] % 0x1000 == 0x307)) ||
              ((fi->ArgTypes[j] % 0x1000 == 0x307)&&
               (currentFunction->ArgTypes[j] % 0x1000 == 0x301)) ||
              ((fi->ArgTypes[j] % 0x1000 == 0x304)&&
               (currentFunction->ArgTypes[j] % 0x1000 == 0x306)) ||
              ((fi->ArgTypes[j] % 0x1000 == 0x306)&&
               (currentFunction->ArgTypes[j] % 0x1000 == 0x304)) ||
              ((fi->ArgTypes[j] % 0x1000 == 0x30A)&&
               (currentFunction->ArgTypes[j] % 0x1000 == 0x304)) ||
              ((fi->ArgTypes[j] % 0x1000 == 0x304)&&
               (currentFunction->ArgTypes[j] % 0x1000 == 0x30A)) ||
              ((fi->ArgTypes[j] % 0x1000 == 0x30A)&&
               (currentFunction->ArgTypes[j] % 0x1000 == 0x306)) ||
              ((fi->ArgTypes[j] % 0x1000 == 0x306)&&
               (currentFunction->ArgTypes[j] % 0x1000 == 0x30A)) ||
              ((fi->ArgTypes[j] % 0x1000 == 0x30B)&&
               (currentFunction->ArgTypes[j] % 0x1000 == 0x304)) ||
              ((fi->ArgTypes[j] % 0x1000 == 0x304)&&
               (currentFunction->ArgTypes[j] % 0x1000 == 0x30B)) ||
              ((fi->ArgTypes[j] % 0x1000 == 0x30B)&&
               (currentFunction->ArgTypes[j] % 0x1000 == 0x306)) ||
              ((fi->ArgTypes[j] % 0x1000 == 0x306)&&
               (currentFunction->ArgTypes[j] % 0x1000 == 0x30B)) ||
              ((fi->ArgTypes[j] % 0x1000 == 0x30C)&&
               (currentFunction->ArgTypes[j] % 0x1000 == 0x304)) ||
              ((fi->ArgTypes[j] % 0x1000 == 0x304)&&
               (currentFunction->ArgTypes[j] % 0x1000 == 0x30C)) ||
              ((fi->ArgTypes[j] % 0x1000 == 0x30C)&&
               (currentFunction->ArgTypes[j] % 0x1000 == 0x306)) ||
              ((fi->ArgTypes[j] % 0x1000 == 0x306)&&
               (currentFunction->ArgTypes[j] % 0x1000 == 0x30C)) ||
              ((fi->ArgTypes[j] % 0x1000 == 0x1)&&
               (currentFunction->ArgTypes[j] % 0x1000 == 0x7)) ||
              ((fi->ArgTypes[j] % 0x1000 == 0x7)&&
               (currentFunction->ArgTypes[j] % 0x1000 == 0x1)) ||
              ((fi->ArgTypes[j] % 0x1000 == 0x4)&&
               (currentFunction->ArgTypes[j] % 0x1000 == 0x6)) ||
              ((fi->ArgTypes[j] % 0x1000 == 0x6)&&
               (currentFunction->ArgTypes[j] % 0x1000 == 0x4)) ||
              ((fi->ArgTypes[j] % 0x1000 == 0x4)&&
               (currentFunction->ArgTypes[j] % 0x1000 == 0xA)) ||
              ((fi->ArgTypes[j] % 0x1000 == 0xA)&&
               (currentFunction->ArgTypes[j] % 0x1000 == 0x4)) ||
              ((fi->ArgTypes[j] % 0x1000 == 0xA)&&
               (currentFunction->ArgTypes[j] % 0x1000 == 0x6)) ||
              ((fi->ArgTypes[j] % 0x1000 == 0x6)&&
               (currentFunction->ArgTypes[j] % 0x1000 == 0xA)) ||
              ((fi->ArgTypes[j] % 0x1000 == 0x4)&&
               (currentFunction->ArgTypes[j] % 0x1000 == 0xB)) ||
              ((fi->ArgTypes[j] % 0x1000 == 0xB)&&
               (currentFunction->ArgTypes[j] % 0x1000 == 0x4)) ||
              ((fi->ArgTypes[j] % 0x1000 == 0xB)&&
               (currentFunction->ArgTypes[j] % 0x1000 == 0x6)) ||
              ((fi->ArgTypes[j] % 0x1000 == 0x6)&&
               (currentFunction->ArgTypes[j] % 0x1000 == 0xB)) ||
              ((fi->ArgTypes[j] % 0x1000 == 0x4)&&
               (currentFunction->ArgTypes[j] % 0x1000 == 0xC)) ||
              ((fi->ArgTypes[j] % 0x1000 == 0xC)&&
               (currentFunction->ArgTypes[j] % 0x1000 == 0x4)) ||
              ((fi->ArgTypes[j] % 0x1000 == 0xC)&&
               (currentFunction->ArgTypes[j] % 0x1000 == 0x6)) ||
              ((fi->ArgTypes[j] % 0x1000 == 0x6)&&
               (currentFunction->ArgTypes[j] % 0x1000 == 0xC))))
          {
          match = 0;
          }
        else
          {
          if (fi->ArgTypes[j] % 0x1000 == 0x309 || fi->ArgTypes[j] % 0x1000 == 0x109)
            {
            if (strcmp(fi->ArgClasses[j],currentFunction->ArgClasses[j]))
              {
              match = 0;
              }
            }
          }
        }
      if ((fi->ReturnType != currentFunction->ReturnType) &&
          !(((fi->ReturnType % 0x1000 == 0x309)&&
             (currentFunction->ReturnType % 0x1000 == 0x109)) ||
            ((fi->ReturnType % 0x1000 == 0x109)&&
             (currentFunction->ReturnType % 0x1000 == 0x309)) ||
            ((fi->ReturnType % 0x1000 == 0x301)&&
             (currentFunction->ReturnType % 0x1000 == 0x307)) ||
            ((fi->ReturnType % 0x1000 == 0x307)&&
             (currentFunction->ReturnType % 0x1000 == 0x301)) ||
            ((fi->ReturnType % 0x1000 == 0x304)&&
             (currentFunction->ReturnType % 0x1000 == 0x306)) ||
            ((fi->ReturnType % 0x1000 == 0x306)&&
             (currentFunction->ReturnType % 0x1000 == 0x304)) ||
            ((fi->ReturnType % 0x1000 == 0x304)&&
             (currentFunction->ReturnType % 0x1000 == 0x30A)) ||
            ((fi->ReturnType % 0x1000 == 0x30A)&&
             (currentFunction->ReturnType % 0x1000 == 0x304)) ||
            ((fi->ReturnType % 0x1000 == 0x306)&&
             (currentFunction->ReturnType % 0x1000 == 0x30A)) ||
            ((fi->ReturnType % 0x1000 == 0x30A)&&
             (currentFunction->ReturnType % 0x1000 == 0x306)) ||
            ((fi->ReturnType % 0x1000 == 0x304)&&
             (currentFunction->ReturnType % 0x1000 == 0x30B)) ||
            ((fi->ReturnType % 0x1000 == 0x30B)&&
             (currentFunction->ReturnType % 0x1000 == 0x304)) ||
            ((fi->ReturnType % 0x1000 == 0x306)&&
             (currentFunction->ReturnType % 0x1000 == 0x30B)) ||
            ((fi->ReturnType % 0x1000 == 0x30B)&&
             (currentFunction->ReturnType % 0x1000 == 0x306)) ||
            ((fi->ReturnType % 0x1000 == 0x304)&&
             (currentFunction->ReturnType % 0x1000 == 0x30C)) ||
            ((fi->ReturnType % 0x1000 == 0x30C)&&
             (currentFunction->ReturnType % 0x1000 == 0x304)) ||
            ((fi->ReturnType % 0x1000 == 0x306)&&
             (currentFunction->ReturnType % 0x1000 == 0x30C)) ||
            ((fi->ReturnType % 0x1000 == 0x30C)&&
             (currentFunction->ReturnType % 0x1000 == 0x306)) ||
            ((fi->ReturnType % 0x1000 == 0x1)&&
             (currentFunction->ReturnType % 0x1000 == 0x7)) ||
            ((fi->ReturnType % 0x1000 == 0x7)&&
             (currentFunction->ReturnType % 0x1000 == 0x1)) ||
            ((fi->ReturnType % 0x1000 == 0x4)&&
             (currentFunction->ReturnType % 0x1000 == 0x6)) ||
            ((fi->ReturnType % 0x1000 == 0x6)&&
             (currentFunction->ReturnType % 0x1000 == 0x4)) ||
            ((fi->ReturnType % 0x1000 == 0xA)&&
             (currentFunction->ReturnType % 0x1000 == 0x6)) ||
            ((fi->ReturnType % 0x1000 == 0x6)&&
             (currentFunction->ReturnType % 0x1000 == 0xA)) ||
            ((fi->ReturnType % 0x1000 == 0x4)&&
             (currentFunction->ReturnType % 0x1000 == 0xA)) ||
            ((fi->ReturnType % 0x1000 == 0xA)&&
             (currentFunction->ReturnType % 0x1000 == 0x4)) ||
            ((fi->ReturnType % 0x1000 == 0xB)&&
             (currentFunction->ReturnType % 0x1000 == 0x6)) ||
            ((fi->ReturnType % 0x1000 == 0x6)&&
             (currentFunction->ReturnType % 0x1000 == 0xB)) ||
            ((fi->ReturnType % 0x1000 == 0x4)&&
             (currentFunction->ReturnType % 0x1000 == 0xB)) ||
            ((fi->ReturnType % 0x1000 == 0xB)&&
             (currentFunction->ReturnType % 0x1000 == 0x4)) ||
            ((fi->ReturnType % 0x1000 == 0xC)&&
             (currentFunction->ReturnType % 0x1000 == 0x6)) ||
            ((fi->ReturnType % 0x1000 == 0x6)&&
             (currentFunction->ReturnType % 0x1000 == 0xC)) ||
            ((fi->ReturnType % 0x1000 == 0x4)&&
             (currentFunction->ReturnType % 0x1000 == 0xC)) ||
            ((fi->ReturnType % 0x1000 == 0xC)&&
             (currentFunction->ReturnType % 0x1000 == 0x4))))
        
        {
        match = 0;
        }
      else
        {
        if (fi->ReturnType % 0x1000 == 0x309 || fi->ReturnType % 0x1000 == 0x109)
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
    if (currentFunction->ArgTypes[i] % 0x1000 == 0x9) args_ok = 0;
    if ((currentFunction->ArgTypes[i] % 0x10) == 0x8) args_ok = 0;
    if (((currentFunction->ArgTypes[i] % 0x1000)/0x100 != 0x3)&&
        (currentFunction->ArgTypes[i] % 0x1000 != 0x109)&&
        ((currentFunction->ArgTypes[i] % 0x1000)/0x100)) args_ok = 0;
    if (currentFunction->ArgTypes[i] % 0x1000 == 0x313) args_ok = 0;
    if (currentFunction->ArgTypes[i] % 0x1000 == 0x314) args_ok = 0;
    if (currentFunction->ArgTypes[i] % 0x1000 == 0x315) args_ok = 0;
    if (currentFunction->ArgTypes[i] % 0x1000 == 0x316) args_ok = 0;
    if (currentFunction->ArgTypes[i] % 0x1000 == 0x31A) args_ok = 0;
    if (currentFunction->ArgTypes[i] % 0x1000 == 0x31B) args_ok = 0;
    if (currentFunction->ArgTypes[i] % 0x1000 == 0x31C) args_ok = 0;
    }
  if ((currentFunction->ReturnType % 0x10) == 0x8) args_ok = 0;
  if (currentFunction->ReturnType % 0x1000 == 0x9) args_ok = 0;
  if (((currentFunction->ReturnType % 0x1000)/0x100 != 0x3)&&
      (currentFunction->ReturnType % 0x1000 != 0x109)&&
      ((currentFunction->ReturnType % 0x1000)/0x100)) args_ok = 0;


  /* eliminate unsigned short * usigned int * etc */
  if (currentFunction->ReturnType % 0x1000 == 0x314) args_ok = 0;
  if (currentFunction->ReturnType % 0x1000 == 0x315) args_ok = 0;
  if (currentFunction->ReturnType % 0x1000 == 0x316) args_ok = 0;
  if (currentFunction->ReturnType % 0x1000 == 0x31A) args_ok = 0;
  if (currentFunction->ReturnType % 0x1000 == 0x31B) args_ok = 0;
  if (currentFunction->ReturnType % 0x1000 == 0x31C) args_ok = 0;

  if (currentFunction->NumberOfArguments && 
      (currentFunction->ArgTypes[0] == 0x5000)
      &&(currentFunction->NumberOfArguments != 1)) args_ok = 0;

  /* make sure we have all the info we need for array arguments in */
  for (i = 0; i < currentFunction->NumberOfArguments; i++)
    {
    if (((currentFunction->ArgTypes[i] % 0x1000)/0x100 == 0x3)&&
        (currentFunction->ArgCounts[i] <= 0)&&
        (currentFunction->ArgTypes[i] % 0x1000 != 0x309)&&
        (currentFunction->ArgTypes[i] % 0x1000 != 0x303)) args_ok = 0;
    }

  /* if we need a return type hint make sure we have one */
  switch (currentFunction->ReturnType % 0x1000)
    {
    case 0x301: case 0x302: case 0x307:
    case 0x304: case 0x305: case 0x306:
    case 0x30A: case 0x30B: case 0x30C: case 0x30D: case 0x30E:
    case 0x313:
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
      
      
      switch (currentFunction->ReturnType % 0x1000)
          {
            case 0x2:
            fprintf(fp,"  op->%s(",currentFunction->Name);
          break;
            case 0x109:
            fprintf(fp,"  temp%i = &(op)->%s(",MAX_ARGS, currentFunction->Name);
          break;
          default:
             fprintf(fp,"  temp%i = (op)->%s(",MAX_ARGS, currentFunction->Name);
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
          else if (currentFunction->ArgTypes[i] == 0x5000)
            {
            fprintf(fp,"vtkJavaVoidFunc,(void *)temp%i",i);
            }
          else
            {
            fprintf(fp,"temp%i",i);
            }
          } /* for */
      fprintf(fp,");\n");
      if (currentFunction->NumberOfArguments == 1 && currentFunction->ArgTypes[0] == 0x5000)
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
