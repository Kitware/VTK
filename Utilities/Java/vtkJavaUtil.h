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

#ifndef vtkJavaUtil_h
#define vtkJavaUtil_h

#include "vtkCommand.h"
#include "vtkJavaModule.h"
#include "vtkSystemIncludes.h"
#include <jni.h>

#include <string>

extern VTKJAVA_EXPORT jlong q(JNIEnv* env, jobject obj);

extern VTKJAVA_EXPORT void* vtkJavaGetPointerFromObject(JNIEnv* env, jobject obj);
extern VTKJAVA_EXPORT char* vtkJavaUTFToChar(JNIEnv* env, jstring in);
extern VTKJAVA_EXPORT bool vtkJavaUTFToString(JNIEnv* env, jstring in, std::string& out);
extern VTKJAVA_EXPORT jstring vtkJavaMakeJavaString(JNIEnv* env, const char* in);

extern VTKJAVA_EXPORT jarray vtkJavaMakeJArrayOfFloatFromFloat(
  JNIEnv* env, const float* arr, int size);
extern VTKJAVA_EXPORT jarray vtkJavaMakeJArrayOfDoubleFromFloat(
  JNIEnv* env, const float* arr, int size);
extern VTKJAVA_EXPORT jarray vtkJavaMakeJArrayOfDoubleFromDouble(
  JNIEnv* env, const double* arr, int size);
extern VTKJAVA_EXPORT jarray vtkJavaMakeJArrayOfShortFromShort(
  JNIEnv* env, const short* arr, int size);
extern VTKJAVA_EXPORT jarray vtkJavaMakeJArrayOfIntFromInt(JNIEnv* env, const int* arr, int size);
extern VTKJAVA_EXPORT jarray vtkJavaMakeJArrayOfIntFromIdType(
  JNIEnv* env, const vtkIdType* arr, int size);
extern VTKJAVA_EXPORT jarray vtkJavaMakeJArrayOfIntFromLongLong(
  JNIEnv* env, const long long* arr, int size);
extern VTKJAVA_EXPORT jarray vtkJavaMakeJArrayOfIntFromSignedChar(
  JNIEnv* env, const signed char* arr, int size);
extern VTKJAVA_EXPORT jarray vtkJavaMakeJArrayOfLongFromLong(
  JNIEnv* env, const long* arr, int size);
extern VTKJAVA_EXPORT jarray vtkJavaMakeJArrayOfByteFromUnsignedChar(
  JNIEnv* env, const unsigned char* arr, int size);
extern VTKJAVA_EXPORT jarray vtkJavaMakeJArrayOfByteFromChar(
  JNIEnv* env, const char* arr, int size);
extern VTKJAVA_EXPORT jarray vtkJavaMakeJArrayOfCharFromChar(
  JNIEnv* env, const char* arr, int size);
extern VTKJAVA_EXPORT jarray vtkJavaMakeJArrayOfUnsignedCharFromUnsignedChar(
  JNIEnv* env, const unsigned char* arr, int size);
extern VTKJAVA_EXPORT jarray vtkJavaMakeJArrayOfUnsignedIntFromUnsignedInt(
  JNIEnv* env, const unsigned int* arr, int size);
extern VTKJAVA_EXPORT jarray vtkJavaMakeJArrayOfUnsignedShortFromUnsignedShort(
  JNIEnv* env, const unsigned short* ptr, int size);
extern VTKJAVA_EXPORT jarray vtkJavaMakeJArrayOfUnsignedLongFromUnsignedLong(
  JNIEnv* env, const unsigned long* arr, int size);

// this is the void pointer parameter passed to the vtk callback routines on
// behalf of the Java interface for callbacks.
struct vtkJavaVoidFuncArg
{
  JavaVM* vm;
  jobject uobj;
  jmethodID mid;
};

extern VTKJAVA_EXPORT void vtkJavaVoidFunc(void*);
extern VTKJAVA_EXPORT void vtkJavaVoidFuncArgDelete(void*);

class VTKJAVA_EXPORT vtkJavaCommand : public vtkCommand
{
public:
  static vtkJavaCommand* New() { return new vtkJavaCommand; }

  void SetGlobalRef(jobject obj) { this->uobj = obj; }
  void SetMethodID(jmethodID id) { this->mid = id; }
  void AssignJavaVM(JNIEnv* env) { env->GetJavaVM(&(this->vm)); }

  void Execute(vtkObject*, unsigned long, void*);

  JavaVM* vm;
  jobject uobj;
  jmethodID mid;

protected:
  vtkJavaCommand();
  ~vtkJavaCommand();
};

#endif
// VTK-HeaderTest-Exclude: vtkJavaUtil.h
