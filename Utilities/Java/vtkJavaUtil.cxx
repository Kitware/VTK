// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#if defined(_MSC_VER)
#ifdef _INTEGRAL_MAX_BITS
#undef _INTEGRAL_MAX_BITS
#endif
#define _INTEGRAL_MAX_BITS 64
#endif

#include "vtkDebugLeaks.h"
#include "vtkObject.h"
#include "vtkWindows.h"

#include "vtkJavaUtil.h"

// Silence warning like
// "dereferencing type-punned pointer will break strict-aliasing rules"
// it happens because this kind of expression: (void **)(&e)
#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

VTK_ABI_NAMESPACE_BEGIN
JNIEXPORT jlong vtkJavaGetId(JNIEnv* env, jobject obj)
{
  jfieldID id;
  jlong result;

  id = env->GetFieldID(env->GetObjectClass(obj), "vtkId", "J");

  result = env->GetLongField(obj, id);
  return result;
}

JNIEXPORT void* vtkJavaGetPointerFromObject(JNIEnv* env, jobject obj)
{
  return obj ? (void*)(size_t)vtkJavaGetId(env, obj) : nullptr;
}

JNIEXPORT jbyteArray vtkJavaMakeJArrayOfByte(JNIEnv* env, const jbyte* ptr, int size)
{
  jbyteArray result = env->NewByteArray(size);
  if (result != nullptr)
  {
    env->SetByteArrayRegion(result, 0, size, ptr);
  }

  return result;
}

JNIEXPORT jshortArray vtkJavaMakeJArrayOfShort(JNIEnv* env, const jshort* ptr, int size)
{
  jshortArray result = env->NewShortArray(size);
  if (result != nullptr)
  {
    env->SetShortArrayRegion(result, 0, size, ptr);
  }

  return result;
}

JNIEXPORT jintArray vtkJavaMakeJArrayOfInt(JNIEnv* env, const jint* ptr, int size)
{
  jintArray result = env->NewIntArray(size);
  if (result != nullptr)
  {
    env->SetIntArrayRegion(result, 0, size, ptr);
  }

  return result;
}

JNIEXPORT jlongArray vtkJavaMakeJArrayOfLong(JNIEnv* env, const jlong* ptr, int size)
{
  jlongArray result = env->NewLongArray(size);
  if (result != nullptr)
  {
    env->SetLongArrayRegion(result, 0, size, ptr);
  }

  return result;
}

JNIEXPORT jbooleanArray vtkJavaMakeJArrayOfBoolean(JNIEnv* env, const jboolean* ptr, int size)
{
  jbooleanArray result = env->NewBooleanArray(size);
  if (result != nullptr)
  {
    env->SetBooleanArrayRegion(result, 0, size, ptr);
  }

  return result;
}

JNIEXPORT jdoubleArray vtkJavaMakeJArrayOfDouble(JNIEnv* env, const jdouble* ptr, int size)
{
  jdoubleArray result = env->NewDoubleArray(size);
  if (result != nullptr)
  {
    env->SetDoubleArrayRegion(result, 0, size, ptr);
  }

  return result;
}

JNIEXPORT jfloatArray vtkJavaMakeJArrayOfFloat(JNIEnv* env, const jfloat* ptr, int size)
{
  jfloatArray result = env->NewFloatArray(size);
  if (result != nullptr)
  {
    env->SetFloatArrayRegion(result, 0, size, ptr);
  }

  return result;
}

// http://java.sun.com/docs/books/jni/html/pitfalls.html#12400
static void JNU_ThrowByName(JNIEnv* env, const char* name, const char* msg)
{
  jclass cls = env->FindClass(name);
  /* if cls is nullptr, an exception has already been thrown */
  if (cls != nullptr)
  {
    env->ThrowNew(cls, msg);
  }
  /* free the local ref */
  env->DeleteLocalRef(cls);
}

JNIEXPORT char* vtkJavaUTF8ToChars(JNIEnv* env, jbyteArray bytes, jint length)
{
  char* result = new char[length + 1];
  if (result == nullptr)
  {
    JNU_ThrowByName(env, "java/lang/OutOfMemoryError", "in vtkJavaUTF8ToChar()");
  }
  else
  {
    env->GetByteArrayRegion(bytes, 0, length, (jbyte*)result);
    result[length] = '\0'; /* nullptr-terminate */
  }

  return result;
}

JNIEXPORT std::string vtkJavaUTF8ToString(JNIEnv* env, jbyteArray bytes, jint length)
{
  std::string result;
  char* cstring = vtkJavaUTF8ToChars(env, bytes, length);
  if (cstring != nullptr)
  {
    result.assign(cstring, length);
    delete[] cstring;
  }

  return result;
}

JNIEXPORT jbyteArray vtkJavaStringToUTF8(JNIEnv* env, const std::string& text)
{
  return vtkJavaCharsToUTF8(env, text.c_str(), text.length());
}

JNIEXPORT jbyteArray vtkJavaCharsToUTF8(JNIEnv* env, const char* chars, size_t length)
{
  return vtkJavaMakeJArrayOfByte(env, reinterpret_cast<const jbyte*>(chars), length);
}

//**jcp this is the callback interface stub for Java. no user parms are passed
// since the callback must be a method of a class. We make the rash assumption
// that the <this> pointer will anchor any required other elements for the
// called functions. - edited by km
JNIEXPORT void vtkJavaVoidFunc(void* f)
{
  vtkJavaVoidFuncArg* iprm = static_cast<vtkJavaVoidFuncArg*>(f);
  // make sure we have a valid method ID
  if (iprm->mid)
  {
    JNIEnv* e;
    // it should already be attached
#ifdef JNI_VERSION_1_2
    iprm->vm->AttachCurrentThread((void**)(&e), nullptr);
#else
    iprm->vm->AttachCurrentThread((JNIEnv_**)(&e), nullptr);
#endif
    e->CallVoidMethod(iprm->uobj, iprm->mid, nullptr);
  }
}

JNIEXPORT void vtkJavaVoidFuncArgDelete(void* arg)
{
  vtkJavaVoidFuncArg* arg2 = static_cast<vtkJavaVoidFuncArg*>(arg);

  JNIEnv* e;
  // it should already be attached
#ifdef JNI_VERSION_1_2
  arg2->vm->AttachCurrentThread((void**)(&e), nullptr);
#else
  arg2->vm->AttachCurrentThread((JNIEnv_**)(&e), nullptr);
#endif
  // free the structure
  e->DeleteGlobalRef(arg2->uobj);
  delete arg2;
}

vtkJavaCommand::vtkJavaCommand()
{
  this->vm = nullptr;
}

vtkJavaCommand::~vtkJavaCommand()
{
  JNIEnv* e;
  // it should already be attached
#ifdef JNI_VERSION_1_2
  this->vm->AttachCurrentThread((void**)(&e), nullptr);
#else
  this->vm->AttachCurrentThread((JNIEnv_**)(&e), nullptr);
#endif
  // free the structure
  e->DeleteGlobalRef(this->uobj);
}

void vtkJavaCommand::Execute(vtkObject*, unsigned long, void*)
{
  // make sure we have a valid method ID
  if (this->mid)
  {
    JNIEnv* e;
    // it should already be attached
#ifdef JNI_VERSION_1_2
    this->vm->AttachCurrentThread((void**)(&e), nullptr);
#else
    this->vm->AttachCurrentThread((JNIEnv_**)(&e), nullptr);
#endif
    e->CallVoidMethod(this->uobj, this->mid, nullptr);
  }
}
