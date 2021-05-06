/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkJavaUtil.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

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

JNIEXPORT jarray vtkJavaMakeJArrayOfDoubleFromDouble(JNIEnv* env, const double* ptr, int size)
{
  jdoubleArray ret;
  int i;
  jdouble* array;

  ret = env->NewDoubleArray(size);
  if (ret == nullptr)
  {
    // should throw an exception here
    return nullptr;
  }

  array = env->GetDoubleArrayElements(ret, nullptr);

  // copy the data
  for (i = 0; i < size; i++)
  {
    array[i] = ptr[i];
  }

  env->ReleaseDoubleArrayElements(ret, array, 0);
  return ret;
}

JNIEXPORT jarray vtkJavaMakeJArrayOfIntFromInt(JNIEnv* env, const int* ptr, int size)
{
  jintArray ret;
  int i;
  jint* array;

  ret = env->NewIntArray(size);
  if (ret == nullptr)
  {
    // should throw an exception here
    return nullptr;
  }

  array = env->GetIntArrayElements(ret, nullptr);

  // copy the data
  for (i = 0; i < size; i++)
  {
    array[i] = ptr[i];
  }

  env->ReleaseIntArrayElements(ret, array, 0);
  return ret;
}

JNIEXPORT jarray vtkJavaMakeJArrayOfLongFromIdType(JNIEnv* env, const vtkIdType* ptr, int size)
{
  jlongArray ret;
  int i;
  jlong* array;

  ret = env->NewLongArray(size);
  if (ret == nullptr)
  {
    // should throw an exception here
    return nullptr;
  }

  array = env->GetLongArrayElements(ret, nullptr);

  // copy the data
  for (i = 0; i < size; i++)
  {
    array[i] = ptr[i];
  }

  env->ReleaseLongArrayElements(ret, array, 0);
  return ret;
}

JNIEXPORT jarray vtkJavaMakeJArrayOfLongFromLongLong(JNIEnv* env, const long long* ptr, int size)
{
  jlongArray ret;
  int i;
  jlong* array;

  ret = env->NewLongArray(size);
  if (ret == nullptr)
  {
    // should throw an exception here
    return nullptr;
  }

  array = env->GetLongArrayElements(ret, nullptr);

  // copy the data
  for (i = 0; i < size; i++)
  {
    array[i] = ptr[i];
  }

  env->ReleaseLongArrayElements(ret, array, 0);
  return ret;
}

JNIEXPORT jarray vtkJavaMakeJArrayOfLongFromUnsignedLongLong(
  JNIEnv* env, const unsigned long long* ptr, int size)
{
  jlongArray ret;
  int i;
  jlong* array;

  ret = env->NewLongArray(size);
  if (ret == nullptr)
  {
    // should throw an exception here
    return nullptr;
  }

  array = env->GetLongArrayElements(ret, nullptr);

  // copy the data
  for (i = 0; i < size; i++)
  {
    array[i] = ptr[i];
  }

  env->ReleaseLongArrayElements(ret, array, 0);
  return ret;
}

JNIEXPORT jarray vtkJavaMakeJArrayOfByteFromChar(JNIEnv* env, const char* ptr, int size)
{
  jbyteArray ret;
  int i;
  jbyte* array;

  ret = env->NewByteArray(size);
  if (ret == nullptr)
  {
    // should throw an exception here
    return nullptr;
  }

  array = env->GetByteArrayElements(ret, nullptr);

  // copy the data
  for (i = 0; i < size; i++)
  {
    array[i] = (int)ptr[i];
  }

  env->ReleaseByteArrayElements(ret, array, 0);
  return ret;
}

JNIEXPORT jarray vtkJavaMakeJArrayOfByteFromSignedChar(
  JNIEnv* env, const signed char* ptr, int size)
{
  jbyteArray ret;
  int i;
  jbyte* array;

  ret = env->NewByteArray(size);
  if (ret == nullptr)
  {
    // should throw an exception here
    return nullptr;
  }

  array = env->GetByteArrayElements(ret, nullptr);

  // copy the data
  for (i = 0; i < size; i++)
  {
    array[i] = (int)ptr[i];
  }

  env->ReleaseByteArrayElements(ret, array, 0);
  return ret;
}

JNIEXPORT jarray vtkJavaMakeJArrayOfFloatFromFloat(JNIEnv* env, const float* ptr, int size)
{
  jfloatArray ret;
  int i;
  jfloat* array;

  ret = env->NewFloatArray(size);
  if (ret == nullptr)
  {
    // should throw an exception here
    return nullptr;
  }

  array = env->GetFloatArrayElements(ret, nullptr);

  // copy the data
  for (i = 0; i < size; i++)
  {
    array[i] = ptr[i];
  }

  env->ReleaseFloatArrayElements(ret, array, 0);
  return ret;
}

JNIEXPORT jarray vtkJavaMakeJArrayOfShortFromShort(JNIEnv* env, const short* ptr, int size)
{
  jshortArray ret;
  int i;
  jshort* array;

  ret = env->NewShortArray(size);
  if (ret == nullptr)
  {
    // should throw an exception here
    return nullptr;
  }

  array = env->GetShortArrayElements(ret, nullptr);

  // copy the data
  for (i = 0; i < size; i++)
  {
    array[i] = ptr[i];
  }

  env->ReleaseShortArrayElements(ret, array, 0);
  return ret;
}

JNIEXPORT jarray vtkJavaMakeJArrayOfByteFromUnsignedChar(
  JNIEnv* env, const unsigned char* ptr, int size)
{
  jbyteArray ret;
  int i;
  jbyte* array;

  ret = env->NewByteArray(size);
  if (ret == nullptr)
  {
    // should throw an exception here
    return nullptr;
  }

  array = env->GetByteArrayElements(ret, nullptr);

  // copy the data
  for (i = 0; i < size; i++)
  {
    array[i] = ptr[i];
  }

  env->ReleaseByteArrayElements(ret, array, 0);
  return ret;
}

JNIEXPORT jarray vtkJavaMakeJArrayOfLongFromLong(JNIEnv* env, const long* ptr, int size)
{
  cout.flush();
  jlongArray ret;
  int i;
  jlong* array;

  ret = env->NewLongArray(size);
  if (ret == nullptr)
  {
    // should throw an exception here
    return nullptr;
  }

  array = env->GetLongArrayElements(ret, nullptr);

  // copy the data
  for (i = 0; i < size; i++)
  {
    array[i] = ptr[i];
  }

  env->ReleaseLongArrayElements(ret, array, 0);
  return ret;
}

JNIEXPORT jarray vtkJavaMakeJArrayOfLongFromUnsignedLong(
  JNIEnv* env, const unsigned long* ptr, int size)
{
  cout.flush();
  jlongArray ret;
  int i;
  jlong* array;

  ret = env->NewLongArray(size);
  if (ret == nullptr)
  {
    // should throw an exception here
    return nullptr;
  }

  array = env->GetLongArrayElements(ret, nullptr);

  // copy the data
  for (i = 0; i < size; i++)
  {
    array[i] = ptr[i];
  }

  env->ReleaseLongArrayElements(ret, array, 0);
  return ret;
}

JNIEXPORT jarray vtkJavaMakeJArrayOfShortFromUnsignedShort(
  JNIEnv* env, const unsigned short* ptr, int size)
{
  cout.flush();
  jshortArray ret;
  int i;
  jshort* array;

  ret = env->NewShortArray(size);
  if (ret == nullptr)
  {
    // should throw an exception here
    return nullptr;
  }

  array = env->GetShortArrayElements(ret, nullptr);

  // copy the data
  for (i = 0; i < size; i++)
  {
    array[i] = ptr[i];
  }

  env->ReleaseShortArrayElements(ret, array, 0);
  return ret;
}

JNIEXPORT jarray vtkJavaMakeJArrayOfIntFromUnsignedInt(
  JNIEnv* env, const unsigned int* ptr, int size)
{
  cout.flush();
  jintArray ret;
  int i;
  jint* array;

  ret = env->NewIntArray(size);
  if (ret == nullptr)
  {
    // should throw an exception here
    return nullptr;
  }

  array = env->GetIntArrayElements(ret, nullptr);

  // copy the data
  for (i = 0; i < size; i++)
  {
    array[i] = ptr[i];
  }

  env->ReleaseIntArrayElements(ret, array, 0);
  return ret;
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

JNIEXPORT char* vtkJavaUTF8ToChar(JNIEnv* env, jbyteArray bytes, jint length)
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
  char* cstring = vtkJavaUTF8ToChar(env, bytes, length);
  if (cstring != nullptr)
  {
    result.assign(cstring, length);
    delete[] cstring;
  }

  return result;
}

JNIEXPORT jbyteArray vtkJavaStringToUTF8(JNIEnv* env, const std::string& text)
{
  return vtkJavaCharToUTF8(env, text.c_str(), text.length());
}

JNIEXPORT jbyteArray vtkJavaCharToUTF8(JNIEnv* env, const char* chars, size_t length)
{
  jbyteArray result = env->NewByteArray(length);
  if (chars)
  {
    env->SetByteArrayRegion(result, 0, length, (jbyte*)chars);
  }

  return result;
}

//**jcp this is the callback interface stub for Java. no user parms are passed
// since the callback must be a method of a class. We make the rash assumption
// that the <this> pointer will anchor any required other elements for the
// called functions. - edited by km
JNIEXPORT void vtkJavaVoidFunc(void* f)
{
  vtkJavaVoidFuncArg* iprm = (vtkJavaVoidFuncArg*)f;
  // make sure we have a valid method ID
  if (iprm->mid)
  {
    JNIEnv* e;
    // it should already be atached
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
  vtkJavaVoidFuncArg* arg2;

  arg2 = (vtkJavaVoidFuncArg*)arg;

  JNIEnv* e;
  // it should already be atached
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
  // it should already be atached
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
    // it should already be atached
#ifdef JNI_VERSION_1_2
    this->vm->AttachCurrentThread((void**)(&e), nullptr);
#else
    this->vm->AttachCurrentThread((JNIEnv_**)(&e), nullptr);
#endif
    e->CallVoidMethod(this->uobj, this->mid, nullptr);
  }
}
