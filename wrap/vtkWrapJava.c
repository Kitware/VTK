/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWrapJava.c
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
  
  if (currentFunction->ArgTypes[i] == 303)
    {
    fprintf(fp,"jstring ");
    fprintf(fp,"id%i",i);
    return;
    }
  
  if ((currentFunction->ArgTypes[i] == 301)||(currentFunction->ArgTypes[i] == 307))
    {
    fprintf(fp,"jdoubleArray ");
    fprintf(fp,"id%i",i);
    return;
    }
  
  if ((currentFunction->ArgTypes[i] == 304)||(currentFunction->ArgTypes[i] == 306))
    {
    fprintf(fp,"jlongArray ");
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
    case 9:     fprintf(fp,"jref "); break;
    case 8: return;
    }
  
  fprintf(fp,"id%i",i);
}

/* when the cpp file doesn't have enough info use the hint file */
void use_hints(FILE *fp)
{
  /* use the hint */
  switch (currentFunction->ReturnType)
    {
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
      
    case 305: case 306: case 313: case 314: case 315: case 316:
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
      
    case 301: case 307:
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
      if ((i == MAX_ARGS)||(aType%10 == 9)||(aType == 303)) 
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
      (i != 10)&&
      (aType%10 != 9)&&
      (aType != 303))
    {
    fprintf(fp,"[%i]",aCount);
    fprintf(fp,";\n  void *tempArray");
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
    fprintf(fp,"  temp%i->uenv = env;\n",i);
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
      fprintf(fp,"  temp%i = (%s *)(vtkJavaGetPointerFromObject(env,id%i,\"%s\"));\n",i,currentFunction->ArgClasses[i],i,currentFunction->ArgClasses[i]);
      break;
    case 301:
    case 307:
      fprintf(fp,"  tempArray = (void *)(env->GetDoubleArrayElements(id%i,NULL));\n",i);
      for (j = 0; j < currentFunction->ArgCounts[i]; j++)
	{
	fprintf(fp,"  temp%i[%i] = ((jdouble *)tempArray)[%i];\n",i,j,j);
	}
      fprintf(fp,"  env->ReleaseDoubleArrayElements(id%i,(jdouble *)tempArray,0);\n",i);      
      break;
    case 304:
    case 306:
      fprintf(fp,"  tempArray = (void *)(env->GetLongArrayElements(id%i,NULL));\n",i);
      for (j = 0; j < currentFunction->ArgCounts[i]; j++)
	{
	fprintf(fp,"  temp%i[%i] = ((jlong *)tempArray)[%i];\n",i,j,j);
	}
      fprintf(fp,"  env->ReleaseLongArrayElements(id%i,(jlong *)tempArray,0);\n",i);      
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
    case 303: fprintf(fp,"  return vtkJavaMakeJavaString(env,temp%i);\n",
		      MAX_ARGS); 
    break;
    case 109:
    case 309:  
      {
      fprintf(fp,"  tempH = vtkJavaGetObjectFromPointer((void *)temp%i);\n",
	      MAX_ARGS);
      fprintf(fp,"  if (!tempH)\n    {\n");
      fprintf(fp,"    vtk_%s_NoCPP();\n",currentFunction->ReturnClass);
      fprintf(fp,"    tempH = env->NewObject(env->FindClass(\"vtk/%s\"),env->GetMethodID(env->FindClass(\"vtk/%s\"),\"<init>\",\"()V\"));\n",currentFunction->ReturnClass,currentFunction->ReturnClass);
      fprintf(fp,"    vtkJavaAddObjectToHash(env, tempH,(void *)temp%i,(void *)%s_Typecast,0);\n    }\n",MAX_ARGS, currentFunction->ReturnClass);
      fprintf(fp,"  return tempH;\n");
      break;
      }
      
    /* handle functions returning vectors */
    /* this is done by looking them up in a hint file */
    case 301: case 307:
    case 304: case 305: case 306:
      use_hints(fp);
      break;
    default: fprintf(fp,"  return temp%i;\n", MAX_ARGS); break;
    }
}

void handle_vtkobj_return(FILE *fp)
{
  fprintf(fp,"extern void *%s_Typecast(void *,char *);\n",
	  currentFunction->ReturnClass);
  fprintf(fp,"extern void vtk_%s_NoCPP();\n",currentFunction->ReturnClass);
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
	    !(((fi->ArgTypes[j] == 309)&&
	       (currentFunction->ArgTypes[j] == 109)) ||
	      ((fi->ArgTypes[j] == 109)&&
	       (currentFunction->ArgTypes[j] == 309))))
	  {
	  match = 0;
	  }
	else
	  {
	  if (fi->ArgTypes[j] == 309 || fi->ArgTypes[j] == 109)
	    {
	    if (strcmp(fi->ArgClasses[j],currentFunction->ArgClasses[j]))
	      {
	      match = 0;
	      }
	    }
	  }
	}
      if ((fi->ReturnType != currentFunction->ReturnType) &&
	  !(((fi->ReturnType == 309)&&(currentFunction->ReturnType == 109)) ||
	    ((fi->ReturnType == 109)&&(currentFunction->ReturnType == 309))))
	{
	match = 0;
	}
      else
	{
	if (fi->ReturnType == 309 || fi->ReturnType == 109)
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



void outputFunction(FILE *fp, FileInfo *data)
{
  int i;
  int args_ok = 1;
 
  /* some functions will not get wrapped no matter what else */
  if (currentFunction->IsPureVirtual ||
      currentFunction->IsOperator || 
      currentFunction->ArrayFailure ||
      !currentFunction->IsPublic) 
    {
    return;
    }
  
  /* check to see if we can handle the args */
  for (i = 0; i < currentFunction->NumberOfArguments; i++)
    {
    if (currentFunction->ArgTypes[i] == 9) args_ok = 0;
    if ((currentFunction->ArgTypes[i]%10) == 8) args_ok = 0;
    if (((currentFunction->ArgTypes[i]%1000)/100 != 3)&&
	(currentFunction->ArgTypes[i]%1000 != 109)&&
	((currentFunction->ArgTypes[i]%1000)/100)) args_ok = 0;
    if (currentFunction->ArgTypes[i] == 313) args_ok = 0;
    if (currentFunction->ArgTypes[i] == 314) args_ok = 0;
    if (currentFunction->ArgTypes[i] == 315) args_ok = 0;
    if (currentFunction->ArgTypes[i] == 316) args_ok = 0;
    }
  if ((currentFunction->ReturnType%10) == 8) args_ok = 0;
  if (currentFunction->ReturnType == 9) args_ok = 0;
  if (((currentFunction->ReturnType%1000)/100 != 3)&&
      (currentFunction->ReturnType%1000 != 109)&&
      ((currentFunction->ReturnType%1000)/100)) args_ok = 0;


  /* eliminate unsigned char * and unsigned short * */
  if (currentFunction->ReturnType == 313) args_ok = 0;
  if (currentFunction->ReturnType == 314) args_ok = 0;
  if (currentFunction->ReturnType == 315) args_ok = 0;
  if (currentFunction->ReturnType == 316) args_ok = 0;

  if (currentFunction->NumberOfArguments && 
      (currentFunction->ArgTypes[0] == 5000)
      &&(currentFunction->NumberOfArguments != 1)) args_ok = 0;

  /* make sure we have all the info we need for array arguments in */
  for (i = 0; i < currentFunction->NumberOfArguments; i++)
    {
    if (((currentFunction->ArgTypes[i]%1000)/100 == 3)&&
	(currentFunction->ArgCounts[i] <= 0)&&
	(currentFunction->ArgTypes[i] != 309)&&
	(currentFunction->ArgTypes[i] != 303)) args_ok = 0;
    }

  /* if we need a return type hint make sure we have one */
  switch (currentFunction->ReturnType%1000)
    {
    case 301: case 302: case 307:
    case 304: case 305: case 306:
      args_ok = currentFunction->HaveHint;
      break;
    }
  
  /* make sure it isn't a Delete or New function */
  if (!strcmp("Delete",currentFunction->Name) ||
      !strcmp("New",currentFunction->Name))
    {
    args_ok = 0;
    }
  
  if (currentFunction->IsPublic && args_ok && 
      strcmp(data->ClassName,currentFunction->Name) &&
      strcmp(data->ClassName, currentFunction->Name + 1))
    {
    /* make sure we haven't already done one of these */
    if (!DoneOne())
      {
      fprintf(fp,"\n");
      
      /* does this return a vtkObject if so must do special stuff */
      if ((currentFunction->ReturnType%1000 == 309)||(currentFunction->ReturnType%1000 == 109))
	{
	handle_vtkobj_return(fp);
	}
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
      
      fprintf(fp,"\n  op = (%s *)vtkJavaGetPointerFromObject(env,obj,\"%s\");\n",
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
	if (currentFunction->ArgTypes[i] == 109)
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
	}
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
      }
    }
}

/* print the parsed structures */
void vtkParseOutput(FILE *fp, FileInfo *data)
{
  int i;
  
  fprintf(fp,"// java wrapper for %s object\n//\n",data->ClassName);
  fprintf(fp,"#include \"%s.h\"\n",data->ClassName);
  fprintf(fp,"#include \"vtkJavaUtil.h\"\n\n");
  
  for (i = 0; i < data->NumberOfSuperClasses; i++)
    {
    fprintf(fp,"extern void *%s_Typecast(void *op,char *dType);\n",
	    data->SuperClasses[i]);
    }
  
  fprintf(fp,"\nvoid *%s_Typecast(void *me,char *dType)\n{\n",data->ClassName);
  fprintf(fp,"  if (!strcmp(\"%s\",dType))\n    {\n", data->ClassName);
  fprintf(fp,"    return me;\n    }\n  else\n    {\n");
  
  /* check our superclasses */
  for (i = 0; i < data->NumberOfSuperClasses; i++)
    {
    fprintf(fp,"    if (%s_Typecast(((void *)((%s *)me)),dType) != NULL)\n",
	    data->SuperClasses[i],data->SuperClasses[i]);
    fprintf(fp,"      {\n");
    fprintf(fp,"      return %s_Typecast(((void *)((%s *)me)),dType);\n      }\n",data->SuperClasses[i],data->SuperClasses[i]);
    
    }
  fprintf(fp,"    }\n  return NULL;\n}\n\n");

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
    fprintf(fp,"  op = (%s *)vtkJavaGetPointerFromObject(env,obj,\"%s\");\n",
	    data->ClassName,data->ClassName);
    fprintf(fp,"  if (vtkJavaShouldIDeleteObject(env,obj))\n");
    fprintf(fp,"    {\n    op->Delete();\n    }\n");
    
    fprintf(fp,"}\n");
    }
  if ((!data->IsAbstract)&&
      strcmp(data->ClassName,"vtkDataWriter") &&
      strcmp(data->ClassName,"vtkPointSet") &&
      strcmp(data->ClassName,"vtkDataSetSource"))
    {
    fprintf(fp,"static int vtk_%s_NoCreate = 0;\n",data->ClassName);
    fprintf(fp,"void vtk_%s_NoCPP()\n",data->ClassName);
    fprintf(fp,"{\n  vtk_%s_NoCreate = 1;\n}\n\n",data->ClassName);
    fprintf(fp,"\nextern \"C\" JNIEXPORT void JNICALL Java_vtk_%s_VTKInit(JNIEnv *env, jobject obj)\n",
	    data->ClassName);
    fprintf(fp,"{\n  if (!vtk_%s_NoCreate)\n",data->ClassName);
    fprintf(fp,"    {\n    %s *aNewOne = %s::New();\n",data->ClassName,
	    data->ClassName);
    fprintf(fp,"    vtkJavaAddObjectToHash(env,obj,(void *)aNewOne,(void *)%s_Typecast,1);\n",data->ClassName);
    fprintf(fp,"    }\n  vtk_%s_NoCreate = 0;\n}\n",data->ClassName);
    }
  else
    {
    if (data->NumberOfSuperClasses)
      {
      fprintf(fp,"extern void vtk_%s_NoCPP();\n",data->SuperClasses[0]);
      fprintf(fp,"void vtk_%s_NoCPP()\n",data->ClassName);
      fprintf(fp,"{\n  vtk_%s_NoCPP();\n}\n\n",data->SuperClasses[0]);
      }
    }
}

