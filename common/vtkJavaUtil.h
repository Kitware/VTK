/*=========================================================================

  Program:   Java Wrapper for VTK
  Module:    vtkJavaUtil.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file's contents may be copied, reproduced or altered in any way 
without the express written consent of the author.

Copyright (c) Ken Martin 1995

=========================================================================*/

#include <string.h>
#include <jni.h>

extern void vtkJavaAddObjectToHash(JNIEnv *env,jobject obj,void *anInstance,
				   void *tcFunc, int);
extern void *vtkJavaGetPointerFromObject(JNIEnv *env,jobject obj, 
					 char *result_type);
extern void vtkJavaDeleteObjectFromHash(JNIEnv *env, int id);
extern jobject vtkJavaGetObjectFromPointer(void *ptr);
extern int  vtkJavaShouldIDeleteObject(JNIEnv *env,jobject obj);
extern char *vtkJavaUTFToChar(JNIEnv *env, jstring in);
extern jstring vtkJavaMakeJavaString(JNIEnv *env, const char *in);

extern jarray vtkJavaMakeJArrayOfDoubleFromFloat(JNIEnv *env,
						 float *arr, int size);
extern jarray vtkJavaMakeJArrayOfDoubleFromDouble(JNIEnv *env, 
						  double *arr, int size);
extern jarray vtkJavaMakeJArrayOfIntFromInt(JNIEnv *env, int *arr, int size);


