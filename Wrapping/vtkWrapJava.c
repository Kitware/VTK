/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWrapJava.c
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
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
extern FunctionInfo *currentFunction;
FileInfo *CurrentData;

void output_proto_vars(FILE *fp, int i)
{
  /* ignore void */
  if (((currentFunction->ArgTypes[i] % 10) == 2)&&
      (!((currentFunction->ArgTypes[i]%1000)/100)))
    {
    return;
    }
  
  if (currentFunction->ArgTypes[i] == 5000)
    {
    fprintf(fp,"jobject id0, jstring id1");
    return;
    }
  
  if (currentFunction->ArgTypes[i]%1000 == 303)
    {
    fprintf(fp,"jstring ");
    fprintf(fp,"id%i",i);
    return;
    }
  
  if ((currentFunction->ArgTypes[i]%1000 == 301)||(currentFunction->ArgTypes[i]%1000 == 307))
    {
    fprintf(fp,"jdoubleArray ");
    fprintf(fp,"id%i",i);
    return;
    }
  
  if ((currentFunction->ArgTypes[i]%1000 == 304)||(currentFunction->ArgTypes[i]%1000 == 306))
    {
    fprintf(fp,"jintArray ");
    fprintf(fp,"id%i",i);
    return;
    }


  switch (currentFunction->ArgTypes[i]%10)
    {
    case 1:   fprintf(fp,"jdouble "); break;
    case 7:   fprintf(fp,"jdouble "); break;
    case 4:   fprintf(fp,"jint "); break;
    case 5:   fprintf(fp,"jint "); break;
    case 6:   fprintf(fp,"jint "); break;
    case 2:     fprintf(fp,"void "); break;
    case 3:     fprintf(fp,"jchar "); break;
    case 9:     fprintf(fp,"jobject "); break;
    case 8: return;
    }
  
  fprintf(fp,"id%i",i);
}

/* when the cpp file doesn't have enough info use the hint file */
void use_hints(FILE *fp)
{
  /* use the hint */
  switch (currentFunction->ReturnType%1000)
    {
    case 313:
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
    case 301:
      fprintf(fp,"    return vtkJavaMakeJArrayOfDoubleFromFloat(env,temp%i,%i);\n",
	      MAX_ARGS, currentFunction->HintSize);
      break;
      
    case 307:  
      fprintf(fp,"    return vtkJavaMakeJArrayOfDoubleFromDouble(env,temp%i,%i);\n",
	      MAX_ARGS, currentFunction->HintSize);
      break;
      
    case 304: 
      fprintf(fp,"    return vtkJavaMakeJArrayOfIntFromInt(env,temp%i,%i);\n",
	      MAX_ARGS, currentFunction->HintSize);
      break;
      
    case 305: case 306: case 314: case 315: case 316:
      break;
    }
}

void return_result(FILE *fp)
{
  switch (currentFunction->ReturnType%1000)
    {
    case 1: fprintf(fp,"jdouble "); break;
    case 2: fprintf(fp,"void "); break;
    case 3: fprintf(fp,"jchar "); break;
    case 7: fprintf(fp,"jdouble "); break;
    case 4: case 5: case 6: case 13: case 14: case 15: case 16:
      fprintf(fp,"jint "); 
      break;
    case 303: fprintf(fp,"jstring "); break;
    case 109:
    case 309:  
      fprintf(fp,"jobject "); break;
      
    case 301: case 307: case 313:
    case 304: case 305: case 306:
      fprintf(fp,"jarray "); break;
    }
}


void output_temp(FILE *fp, int i, int aType, char *Id, int aCount)
{
  /* handle VAR FUNCTIONS */
  if (aType == 5000)
    {
    fprintf(fp,"  vtkJavaVoidFuncArg *temp%i = new vtkJavaVoidFuncArg;\n",i);
    return;
    }
  
  /* ignore void */
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
    fprintf(fp," unsigned ");
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
      if ((i == MAX_ARGS)||(aType%10 == 9)||(aType%1000 == 303)) 
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
      (i != MAX_ARGS)&&(aType%10 != 9)&&(aType%1000 != 303))
    {
    fprintf(fp,"[%i]",aCount);
    fprintf(fp,";\n  void *tempArray%i",i);
    }

  fprintf(fp,";\n");
  if ((i == MAX_ARGS) && ((aType%1000 == 309)||(aType%1000 == 109)))
    {
    fprintf(fp,"  jobject tempH;\n");
    }
}

void get_args(FILE *fp, int i)
{
  int j;
  
  /* handle VAR FUNCTIONS */
  if (currentFunction->ArgTypes[i] == 5000)
    {
    fprintf(fp,"  env->GetJavaVM(&(temp%i->vm));\n",i);
    fprintf(fp,"  temp%i->uobj = env->NewGlobalRef(id0);\n",i);
    fprintf(fp,"  temp%i->mid = env->GetMethodID(env->GetObjectClass(id0),vtkJavaUTFToChar(env,id1),\"()V\");\n",i);
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
    case 3:
      fprintf(fp,"  temp%i = (char)(0xff & id%i);\n",i,i);
      break;
    case 303:
      fprintf(fp,"  temp%i = vtkJavaUTFToChar(env,id%i);\n",i,i);
      break;
    case 109:
    case 309:
      fprintf(fp,"  temp%i = (%s *)(vtkJavaGetPointerFromObject(env,id%i,(char *) \"%s\"));\n",i,currentFunction->ArgClasses[i],i,currentFunction->ArgClasses[i]);
      break;
    case 301:
    case 307:
      fprintf(fp,"  tempArray%i = (void *)(env->GetDoubleArrayElements(id%i,NULL));\n",i,i);
      for (j = 0; j < currentFunction->ArgCounts[i]; j++)
	{
	fprintf(fp,"  temp%i[%i] = ((jdouble *)tempArray%i)[%i];\n",i,j,i,j);
	}
      fprintf(fp,"  env->ReleaseDoubleArrayElements(id%i,(jdouble *)tempArray%i,0);\n",i,i);      
      break;
    case 304:
    case 306:
      fprintf(fp,"  tempArray%i = (void *)(env->GetIntArrayElements(id%i,NULL));\n",i,i);
      for (j = 0; j < currentFunction->ArgCounts[i]; j++)
	{
	fprintf(fp,"  temp%i[%i] = ((jint *)tempArray%i)[%i];\n",i,j,i,j);
	}
      fprintf(fp,"  env->ReleaseIntArrayElements(id%i,(jint *)tempArray%i,0);\n",i,i);      
      break;
    case 2:    
    case 9: break;
    default: fprintf(fp,"  temp%i = id%i;\n",i,i); break;
    }
}


void do_return(FILE *fp)
{
  /* ignore void */
  if (((currentFunction->ReturnType % 10) == 2)&&(!((currentFunction->ReturnType%1000)/100)))
    {
    return;
    }

  switch (currentFunction->ReturnType%1000)
    {
    case 303: 
      {
      fprintf(fp,"  return vtkJavaMakeJavaString(env,temp%i);\n",
              MAX_ARGS); 
      }
    break;
    case 109:
    case 309:  
      {
      fprintf(fp,"  if (temp%i == NULL) { return NULL; }\n", MAX_ARGS);
      fprintf(fp,"  tempH = vtkJavaGetObjectFromPointer((void *)temp%i);\n", MAX_ARGS);
      fprintf(fp,"  if (!tempH)\n    {\n");
      fprintf(fp,"    tempH = vtkJavaCreateNewJavaStubForObject(env, (vtkObject *)temp%i);\n", MAX_ARGS);
      fprintf(fp,"    if (!tempH)\n      {\n");
      fprintf(fp,"      // clear the exception first\n");
      fprintf(fp,"      env->ExceptionClear();\n");
      fprintf(fp,"      // no java stub for this class exists? Use function return type\n");
      fprintf(fp,"      tempH = vtkJavaCreateNewJavaStub(env, \"vtk/%s\", (void *)temp%i);\n",
		  currentFunction->ReturnClass, MAX_ARGS);
      fprintf(fp,"      }\n");
      fprintf(fp,"    }\n");      
      fprintf(fp,"  return tempH;\n");
      break;
      }
      
    /* handle functions returning vectors */
    /* this is done by looking them up in a hint file */
    case 301: case 307: case 313:
    case 304: case 305: case 306:
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
	    !(((fi->ArgTypes[j]%1000 == 309)&&
	       (currentFunction->ArgTypes[j]%1000 == 109)) ||
	      ((fi->ArgTypes[j]%1000 == 109)&&
	       (currentFunction->ArgTypes[j]%1000 == 309)) ||
	      ((fi->ArgTypes[j]%1000 == 301)&&
	       (currentFunction->ArgTypes[j]%1000 == 307)) ||
	      ((fi->ArgTypes[j]%1000 == 307)&&
	       (currentFunction->ArgTypes[j]%1000 == 301)) ||
	      ((fi->ArgTypes[j]%1000 == 304)&&
	       (currentFunction->ArgTypes[j]%1000 == 306)) ||
	      ((fi->ArgTypes[j]%1000 == 306)&&
	       (currentFunction->ArgTypes[j]%1000 == 304)) ||
	      ((fi->ArgTypes[j]%1000 == 1)&&
	       (currentFunction->ArgTypes[j]%1000 == 7)) ||
	      ((fi->ArgTypes[j]%1000 == 7)&&
	       (currentFunction->ArgTypes[j]%1000 == 1)) ||
	      ((fi->ArgTypes[j]%1000 == 4)&&
	       (currentFunction->ArgTypes[j]%1000 == 6)) ||
	      ((fi->ArgTypes[j]%1000 == 6)&&
	       (currentFunction->ArgTypes[j]%1000 == 4))))
	  {
	  match = 0;
	  }
	else
	  {
	  if (fi->ArgTypes[j]%1000 == 309 || fi->ArgTypes[j]%1000 == 109)
	    {
	    if (strcmp(fi->ArgClasses[j],currentFunction->ArgClasses[j]))
	      {
	      match = 0;
	      }
	    }
	  }
	}
      if ((fi->ReturnType != currentFunction->ReturnType) &&
	  !(((fi->ReturnType%1000 == 309)&&
	     (currentFunction->ReturnType%1000 == 109)) ||
	    ((fi->ReturnType%1000 == 109)&&
	     (currentFunction->ReturnType%1000 == 309)) ||
	    ((fi->ReturnType%1000 == 301)&&
	     (currentFunction->ReturnType%1000 == 307)) ||
	    ((fi->ReturnType%1000 == 307)&&
	     (currentFunction->ReturnType%1000 == 301)) ||
	    ((fi->ReturnType%1000 == 304)&&
	     (currentFunction->ReturnType%1000 == 306)) ||
	    ((fi->ReturnType%1000 == 306)&&
	     (currentFunction->ReturnType%1000 == 304)) ||
	    ((fi->ReturnType%1000 == 1)&&
	     (currentFunction->ReturnType%1000 == 7)) ||
	    ((fi->ReturnType%1000 == 7)&&
	     (currentFunction->ReturnType%1000 == 1)) ||
	    ((fi->ReturnType%1000 == 4)&&
	     (currentFunction->ReturnType%1000 == 6)) ||
	    ((fi->ReturnType%1000 == 6)&&
	     (currentFunction->ReturnType%1000 == 4))))
	
	{
	match = 0;
	}
      else
	{
	if (fi->ReturnType%1000 == 309 || fi->ReturnType%1000 == 109)
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
    fprintf(fp,"  op = (%s *)vtkJavaGetPointerFromObject(env,obj,(char *) \"%s\");\n",
            data->ClassName, data->ClassName);
    fprintf(fp,"  jboolean isCopy;\n");
    fprintf(fp,"  jbyte *data = env->GetByteArrayElements(id0,&isCopy);\n");
    fprintf(fp,"  op->SetBinaryInputString((const char *)data,id1);\n");
    fprintf(fp,"  env->ReleaseByteArrayElements(id0,data,JNI_ABORT);\n");
    fprintf(fp,"}\n");
}


void outputFunction(FILE *fp, FileInfo *data)
{
  int i;
  int args_ok = 1;
  CurrentData = data;

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


  /* eliminate unsigned short * usigned int * etc */
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
	(currentFunction->ArgTypes[i]%1000 != 303)) args_ok = 0;
    }

  /* if we need a return type hint make sure we have one */
  switch (currentFunction->ReturnType%1000)
    {
    case 301: case 302: case 307:
    case 304: case 305: case 306: case 313:
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
          HandleDataReader(fp,data);
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
      
      fprintf(fp,"extern \"C\" JNIEXPORT ");
      return_result(fp);
      fprintf(fp," JNICALL Java_vtk_%s_%s_1%i(JNIEnv *env, jobject obj",
	      data->ClassName,currentFunction->Name, numberOfWrappedFunctions);
      
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
      
      fprintf(fp,"\n  op = (%s *)vtkJavaGetPointerFromObject(env,obj,(char *) \"%s\");\n",
	      data->ClassName,data->ClassName);
      
      
      switch (currentFunction->ReturnType%1000)
	  {
	    case 2:
	    fprintf(fp,"  op->%s(",currentFunction->Name);
	  break;
	    case 109:
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
	    if (currentFunction->ArgTypes[i]%1000 == 109)
		{
	      fprintf(fp,"*(temp%i)",i);
		}
	    else if (currentFunction->ArgTypes[i] == 5000)
		{
	      fprintf(fp,"vtkJavaVoidFunc,(void *)temp%i",i);
		}
	    else
		{
	      fprintf(fp,"temp%i",i);
		}
	  } /* for */
      fprintf(fp,");\n");
      if (currentFunction->NumberOfArguments == 1 && currentFunction->ArgTypes[0] == 5000)
	  {
	    fprintf(fp,"  op->%sArgDelete(vtkJavaVoidFuncArgDelete);\n",
		  currentFunction->Name);
	  }
      
      do_return(fp);
      fprintf(fp,"}\n");
      
      wrappedFunctions[numberOfWrappedFunctions] = currentFunction;
      numberOfWrappedFunctions++;
    } /* isDone() */
  } /* isAbstract */
}

/* print the parsed structures */
void vtkParseOutput(FILE *fp, FileInfo *data)
{
  int i;
  
  fprintf(fp,"// java wrapper for %s object\n//\n",data->ClassName);
  fprintf(fp,"#include \"vtkSystemIncludes.h\"\n");
  fprintf(fp,"#include \"%s.h\"\n",data->ClassName);
  fprintf(fp,"#include \"vtkJavaUtil.h\"\n\n");
  
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

  /* insert function handling code here */
  for (i = 0; i < data->NumberOfFunctions; i++)
    {
    currentFunction = data->Functions + i;
    outputFunction(fp, data);
    }

  if ((!data->NumberOfSuperClasses)&&(data->HasDelete))
    {
    fprintf(fp,"\nextern \"C\" JNIEXPORT void JNICALL Java_vtk_%s_VTKDelete(JNIEnv *env,jobject obj)\n",
	    data->ClassName);
    fprintf(fp,"{\n  %s *op;\n",data->ClassName);
    fprintf(fp,"  op = (%s *)vtkJavaGetPointerFromObject(env,obj,(char *) \"%s\");\n",
	    data->ClassName,data->ClassName);
    fprintf(fp,"  vtkJavaDeleteObject(env,obj);\n");
    fprintf(fp,"  op->Delete();\n");
    fprintf(fp,"}\n");
    }
  if (data->IsConcrete)
    {
    fprintf(fp,"\nextern \"C\" JNIEXPORT void JNICALL Java_vtk_%s_VTKInit(JNIEnv *env, jobject obj)",
	    data->ClassName);
    fprintf(fp,"\n{");
    fprintf(fp,"\n  %s *aNewOne = %s::New();",data->ClassName, data->ClassName);
    fprintf(fp,"\n  int id= vtkJavaRegisterNewObject(env,obj,(void *)aNewOne);");
    fprintf(fp,"\n  vtkJavaRegisterCastFunction(env,obj,id,(void *)%s_Typecast);", data->ClassName);
    fprintf(fp,"\n}\n");  
    } 

  fprintf(fp,"\nextern \"C\" JNIEXPORT void JNICALL Java_vtk_%s_VTKCastInit(JNIEnv *env, jobject obj)",
		data->ClassName);
  fprintf(fp,"\n{");
  fprintf(fp,"\n  int id= vtkJavaGetId(env,obj);");
  fprintf(fp,"\n  vtkJavaRegisterCastFunction(env,obj,id,(void *)%s_Typecast);", 
	    data->ClassName);
  fprintf(fp,"\n}\n");

  /* for vtkRenderWindow we want to add a special method to support */
  /* native AWT rendering */
  if (!strcmp("vtkRenderWindow",data->ClassName))
    {
    fprintf(fp,"\n#include \"vtkJavaAwt.h\"\n\n");
    }
  
  if (!strcmp("vtkObject",data->ClassName))
    {
    fprintf(fp,"\nextern \"C\" JNIEXPORT jstring JNICALL Java_vtk_vtkObject_Print(JNIEnv *env,jobject obj)\n");
    fprintf(fp,"{\n  vtkObject *op;\n");
    fprintf(fp,"  jstring tmp;\n\n");
    fprintf(fp,"  op = (vtkObject *)vtkJavaGetPointerFromObject(env,obj,(char *) \"vtkObject\");\n");
    
    fprintf(fp,"  ostrstream buf;\n");
    fprintf(fp,"  op->Print(buf);\n");
    fprintf(fp,"  buf.put('\\0');\n");  
	fprintf(fp,"  tmp = vtkJavaMakeJavaString(env,buf.str());\n");
    fprintf(fp,"  delete buf.str();\n");

    fprintf(fp,"  return tmp;\n");
    fprintf(fp,"}\n");

    fprintf(fp,"\nextern \"C\" JNIEXPORT jint JNICALL Java_vtk_vtkObject_AddObserver(JNIEnv *env,jobject obj, jstring id0, jobject id1, jstring id2)\n");
    fprintf(fp,"{\n  vtkObject *op;\n");

    fprintf(fp,"  vtkJavaCommand *cbc = new vtkJavaCommand;\n");
    fprintf(fp,"  cbc->AssignJavaVM(env);\n");
    fprintf(fp,"  cbc->SetGlobalRef(env->NewGlobalRef(id1));\n");
    fprintf(fp,"  cbc->SetMethodID(env->GetMethodID(env->GetObjectClass(id1),vtkJavaUTFToChar(env,id2),\"()V\"));\n");
    fprintf(fp,"  char    *temp0;\n");
    fprintf(fp,"  temp0 = vtkJavaUTFToChar(env,id0);\n");
    fprintf(fp,"  op = (vtkObject *)vtkJavaGetPointerFromObject(env,obj,(char *) \"vtkObject\");\n");
    fprintf(fp,"  unsigned long     temp20;\n");
    fprintf(fp,"  temp20 = op->AddObserver(temp0,cbc);\n");
    fprintf(fp,"  return temp20;\n}\n");
   }
}

