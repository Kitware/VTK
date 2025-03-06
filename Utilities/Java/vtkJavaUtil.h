// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkJavaUtil_h
#define vtkJavaUtil_h

#include "vtkCommand.h"
#include "vtkJavaModule.h"
#include "vtkSystemIncludes.h"
#include <jni.h>

#include <string>

extern JNIEXPORT void* vtkJavaGetPointerFromObject(JNIEnv* env, jobject obj);
extern JNIEXPORT char* vtkJavaUTF8ToChars(JNIEnv* env, jbyteArray bytes, jint length);
extern JNIEXPORT std::string vtkJavaUTF8ToString(JNIEnv* env, jbyteArray bytes, jint length);
extern JNIEXPORT jbyteArray vtkJavaCharsToUTF8(JNIEnv* env, const char* chars, size_t length);
extern JNIEXPORT jbyteArray vtkJavaStringToUTF8(JNIEnv* env, const std::string& text);

extern JNIEXPORT jbooleanArray vtkJavaMakeJArrayOfBoolean(
  JNIEnv* env, const jboolean* ptr, int size);
extern JNIEXPORT jdoubleArray vtkJavaMakeJArrayOfDouble(JNIEnv* env, const jdouble* ptr, int size);
extern JNIEXPORT jfloatArray vtkJavaMakeJArrayOfFloat(JNIEnv* env, const jfloat* ptr, int size);
extern JNIEXPORT jbyteArray vtkJavaMakeJArrayOfByte(JNIEnv* env, const jbyte* ptr, int size);
extern JNIEXPORT jshortArray vtkJavaMakeJArrayOfShort(JNIEnv* env, const jshort* ptr, int size);
extern JNIEXPORT jintArray vtkJavaMakeJArrayOfInt(JNIEnv* env, const jint* ptr, int size);
extern JNIEXPORT jlongArray vtkJavaMakeJArrayOfLong(JNIEnv* env, const jlong* ptr, int size);

// this is the void pointer parameter passed to the vtk callback routines on
// behalf of the Java interface for callbacks.
struct vtkJavaVoidFuncArg
{
  JavaVM* vm;
  jobject uobj;
  jmethodID mid;
};

extern JNIEXPORT void vtkJavaVoidFunc(void*);
extern JNIEXPORT void vtkJavaVoidFuncArgDelete(void*);

class JNIEXPORT vtkJavaCommand : public vtkCommand
{
public:
  static vtkJavaCommand* New() { return new vtkJavaCommand; }

  void SetGlobalRef(jobject obj) { this->uobj = obj; }
  void SetMethodID(jmethodID id) { this->mid = id; }
  void AssignJavaVM(JNIEnv* env) { env->GetJavaVM(&(this->vm)); }

  void Execute(vtkObject*, unsigned long, void*) override;

  JavaVM* vm;
  jobject uobj;
  jmethodID mid;

protected:
  vtkJavaCommand();
  ~vtkJavaCommand() override;
};

#endif
// VTK-HeaderTest-Exclude: vtkJavaUtil.h
