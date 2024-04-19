// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkJavaUtil_h
#define vtkJavaUtil_h

#include "vtkCommand.h"
#include "vtkJavaModule.h"
#include "vtkSystemIncludes.h"
#include <jni.h>

#include <string>

extern VTKJAVA_EXPORT void* vtkJavaGetPointerFromObject(JNIEnv* env, jobject obj);
extern VTKJAVA_EXPORT char* vtkJavaUTF8ToChars(JNIEnv* env, jbyteArray bytes, jint length);
extern VTKJAVA_EXPORT std::string vtkJavaUTF8ToString(JNIEnv* env, jbyteArray bytes, jint length);
extern VTKJAVA_EXPORT jbyteArray vtkJavaCharsToUTF8(JNIEnv* env, const char* chars, size_t length);
extern VTKJAVA_EXPORT jbyteArray vtkJavaStringToUTF8(JNIEnv* env, const std::string& text);

extern VTKJAVA_EXPORT jbooleanArray vtkJavaMakeJArrayOfBoolean(
  JNIEnv* env, const jboolean* ptr, int size);
extern VTKJAVA_EXPORT jdoubleArray vtkJavaMakeJArrayOfDouble(
  JNIEnv* env, const jdouble* ptr, int size);
extern VTKJAVA_EXPORT jfloatArray vtkJavaMakeJArrayOfFloat(
  JNIEnv* env, const jfloat* ptr, int size);
extern VTKJAVA_EXPORT jbyteArray vtkJavaMakeJArrayOfByte(JNIEnv* env, const jbyte* ptr, int size);
extern VTKJAVA_EXPORT jshortArray vtkJavaMakeJArrayOfShort(
  JNIEnv* env, const jshort* ptr, int size);
extern VTKJAVA_EXPORT jintArray vtkJavaMakeJArrayOfInt(JNIEnv* env, const jint* ptr, int size);
extern VTKJAVA_EXPORT jlongArray vtkJavaMakeJArrayOfLong(JNIEnv* env, const jlong* ptr, int size);

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
