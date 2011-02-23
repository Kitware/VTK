/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkJavaUtil.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef __vtkJavaUtil_h
#define __vtkJavaUtil_h

#include "vtkSystemIncludes.h"
#include <jni.h>
#include "vtkCommand.h"
#include "vtkStdString.h"


extern JNIEXPORT jlong vtkJavaGetId(JNIEnv *env,jobject obj);

extern JNIEXPORT void *vtkJavaGetPointerFromObject(JNIEnv *env,jobject obj);
extern JNIEXPORT char *vtkJavaUTFToChar(JNIEnv *env, jstring in);
extern JNIEXPORT bool vtkJavaUTFToString(JNIEnv *env, jstring in, vtkStdString &out);
extern JNIEXPORT jstring vtkJavaMakeJavaString(JNIEnv *env, const char *in);

extern JNIEXPORT jarray vtkJavaMakeJArrayOfFloatFromFloat(JNIEnv *env,
             float *arr, int size);
extern JNIEXPORT jarray vtkJavaMakeJArrayOfDoubleFromFloat(JNIEnv *env,
             float *arr, int size);
extern JNIEXPORT jarray vtkJavaMakeJArrayOfDoubleFromDouble(JNIEnv *env, 
              double *arr, int size);
extern JNIEXPORT jarray vtkJavaMakeJArrayOfShortFromShort(JNIEnv *env, short *arr, int size);
extern JNIEXPORT jarray vtkJavaMakeJArrayOfIntFromInt(JNIEnv *env, int *arr, int size);
extern JNIEXPORT jarray vtkJavaMakeJArrayOfIntFromIdType(JNIEnv *env, vtkIdType *arr, int size);
#if defined(VTK_TYPE_USE_LONG_LONG)
extern JNIEXPORT jarray vtkJavaMakeJArrayOfIntFromLongLong(JNIEnv *env, long long *arr, int size);
#endif
#if defined(VTK_TYPE_USE___INT64)
extern JNIEXPORT jarray vtkJavaMakeJArrayOfIntFrom__Int64(JNIEnv *env, __int64 *arr, int size);
#endif
extern JNIEXPORT jarray vtkJavaMakeJArrayOfIntFromSignedChar(JNIEnv *env, signed char *arr, int size);
extern JNIEXPORT jarray vtkJavaMakeJArrayOfLongFromLong(JNIEnv *env, long *arr, int size);
extern JNIEXPORT jarray vtkJavaMakeJArrayOfByteFromUnsignedChar(JNIEnv *env, unsigned char *arr, int size);
extern JNIEXPORT jarray vtkJavaMakeJArrayOfByteFromChar(JNIEnv *env, char *arr, int size);
extern JNIEXPORT jarray vtkJavaMakeJArrayOfCharFromChar(JNIEnv *env, char *arr, int size);
extern JNIEXPORT jarray vtkJavaMakeJArrayOfUnsignedCharFromUnsignedChar(JNIEnv *env, unsigned char *arr, int size);
extern JNIEXPORT jarray vtkJavaMakeJArrayOfUnsignedIntFromUnsignedInt(JNIEnv *env, unsigned int *arr, int size);
extern JNIEXPORT jarray vtkJavaMakeJArrayOfUnsignedShortFromUnsignedShort(JNIEnv *env,unsigned short *ptr,int size);
extern JNIEXPORT jarray vtkJavaMakeJArrayOfUnsignedLongFromUnsignedLong(JNIEnv *env, unsigned long *arr, int size);

// this is the void pointer parameter passed to the vtk callback routines on
// behalf of the Java interface for callbacks.
struct vtkJavaVoidFuncArg 
{
  JavaVM *vm;
  jobject  uobj;
  jmethodID mid;
} ;

extern JNIEXPORT void vtkJavaVoidFunc(void *);
extern JNIEXPORT void vtkJavaVoidFuncArgDelete(void *);

class vtkJavaCommand : public vtkCommand
{
public:
  static vtkJavaCommand *New() { return new vtkJavaCommand; };

  void SetGlobalRef(jobject obj) { this->uobj = obj; };
  void SetMethodID(jmethodID id) { this->mid = id; };
  void AssignJavaVM(JNIEnv *env) { env->GetJavaVM(&(this->vm)); };
  
  void Execute(vtkObject *, unsigned long, void *);
  
  JavaVM *vm;
  jobject  uobj;
  jmethodID mid;
protected:
  vtkJavaCommand();
  ~vtkJavaCommand();
};

#endif
