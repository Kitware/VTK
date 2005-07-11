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

// include stdmutex for borland
#ifdef __BORLANDC__
#include <rw/stdmutex.h>
#endif

#ifdef _INTEGRAL_MAX_BITS
#undef _INTEGRAL_MAX_BITS
#endif
#define _INTEGRAL_MAX_BITS 64

#include "vtkObject.h"
#include "vtkDebugLeaks.h"
#include "vtkWindows.h"

#ifdef _WIN32
HANDLE vtkGlobalMutex = NULL;
#define VTK_GET_MUTEX() WaitForSingleObject(vtkGlobalMutex,INFINITE)
#define VTK_RELEASE_MUTEX() ReleaseMutex(vtkGlobalMutex)
#include <mapiform.h>
#else

#ifdef VTK_USE_SPROC
// for SGI's
#include <abi_mutex.h>
abilock_t vtkGlobalMutex;
static void vtk_get_mutex() {
    static int inited = 0;
    if (!inited) {
        if (init_lock(&vtkGlobalMutex) < 0)
            perror("initializing mutex");
        inited = 1;
    }
    spin_lock(&vtkGlobalMutex);
}
static void vtk_release_mutex() {
    if (release_lock(&vtkGlobalMutex) < 0)
        perror("releasing mutex");
}
#define VTK_GET_MUTEX()  vtk_get_mutex()
#define VTK_RELEASE_MUTEX() vtk_release_mutex()
#elif defined(__FreeBSD__) || defined(__linux__) || defined(sgi) || defined(__APPLE_CC__)
#include <pthread.h>
pthread_mutex_t vtkGlobalMutex;
static void vtk_get_mutex() {
    static int inited = 0;
    if (!inited)
      {
      inited = 1;
      pthread_mutex_init ( &vtkGlobalMutex, NULL );
      }
    pthread_mutex_lock(&vtkGlobalMutex);
}
// #define VTK_GET_MUTEX()  pthread_mutex_lock(&vtkGlobalMutex)
#define VTK_GET_MUTEX()  vtk_get_mutex()
#define VTK_RELEASE_MUTEX() pthread_mutex_unlock(&vtkGlobalMutex)
#else
// for solaris
#include <thread.h>
#include <synch.h>
mutex_t vtkGlobalMutex;
#define VTK_GET_MUTEX()  mutex_lock(&vtkGlobalMutex)
#define VTK_RELEASE_MUTEX() mutex_unlock(&vtkGlobalMutex)
#endif

#endif

#include "vtkJavaUtil.h"

int vtkJavaIdCount = 1;

//#define VTKJAVADEBUG

class vtkHashNode 
{
public:
  vtkHashNode *next;
  void *key;
  void *value;
};


class vtkHashTable
{
public:
  vtkHashTable();
  vtkHashNode *(nodes[64]);
  void AddHashEntry(void *key,void *value);
  void *GetHashTableValue(void *key);
  void *GetHashTableValue(int key);
  void *InternalGetHashTableValue(int loc, void *key);
  void DeleteHashEntry(void *key);
};

vtkHashTable::vtkHashTable()
{
  int i;
  for (i = 0; i < 64; i++)
    {
    this->nodes[i] = NULL;
    }
}

vtkHashTable *vtkInstanceLookup = NULL;
vtkHashTable *vtkPointerLookup = NULL;
vtkHashTable *vtkTypecastLookup = NULL;

void vtkHashTable::AddHashEntry(void *key,void *value)
{
  vtkHashNode *pos;
  vtkHashNode *newpos;
  int loc;
  
  newpos = new vtkHashNode;
  newpos->key = key;
  newpos->value = value;
  newpos->next = NULL;
  
  loc = (unsigned long)((size_t)key & 0x03f0) / 16;
  
  pos = this->nodes[loc];
  if (!pos)
    {
    this->nodes[loc] = newpos;
    return;
    }
  while (pos->next)
    {
    pos = pos->next;
    }
  pos->next = newpos;
}

void *vtkHashTable::GetHashTableValue(int key)
{
  int loc = (unsigned long)(key & 0x03f0) / 16;
  return this->InternalGetHashTableValue(loc, (void *)(size_t)key);
}

void *vtkHashTable::GetHashTableValue(void *key)
{
  int loc = (unsigned long)((size_t)key & 0x03f0) / 16;
  return this->InternalGetHashTableValue(loc, key);
}

void *vtkHashTable::InternalGetHashTableValue(int loc, void *key)
{
  vtkHashNode *pos;
  pos = this->nodes[loc];

  if (!pos)
    {
    return NULL;
    }
  while ((pos)&&(pos->key != key))
    {
    pos = pos->next;
    }
  if (pos)
    {
    return pos->value;
    }
  return NULL;
}

void vtkHashTable::DeleteHashEntry(void *key)
{
  vtkHashNode *pos;
  vtkHashNode *prev = NULL;
  int loc = (unsigned long)((size_t)key & 0x03f0) / 16;
  
  pos = this->nodes[loc];

  while ((pos)&&(pos->key != key))
    {
    prev = pos;
    pos = pos->next;
    }
  if (pos)
    {
    // we found this object
    if (prev)
      {
      prev->next = pos->next;
      }
    else
      {
      this->nodes[loc] = pos->next;
      }
    delete pos;
    }
}

JNIEXPORT int vtkJavaGetId(JNIEnv *env,jobject obj)
{
  jfieldID id;
  int result;
    
  id = env->GetFieldID(env->GetObjectClass(obj),"vtkId","I");
  
  result = (int)env->GetIntField(obj,id);
  return result;
}

JNIEXPORT void vtkJavaSetId(JNIEnv *env,jobject obj, int newVal)
{
  jfieldID id;
  jint jNewVal = (jint)newVal;
    
  id = env->GetFieldID(env->GetObjectClass(obj),"vtkId","I");
  
  env->SetIntField(obj,id,jNewVal);
}

JNIEXPORT void vtkJavaRegisterCastFunction(JNIEnv *vtkNotUsed(env), 
                                           jobject vtkNotUsed(obj), 
                                           int id, void *tcFunc) 
{
  VTK_GET_MUTEX();
#ifdef VTKJAVADEBUG
  if (id == 0) {
    vtkGenericWarningMacro("RegisterCastFunction: Try to add a CastFuction to a unregistered function");
  }
#endif 
  vtkTypecastLookup->AddHashEntry((void *)(size_t)id,tcFunc);
  VTK_RELEASE_MUTEX();
}

// add an object to the hash table
JNIEXPORT int vtkJavaRegisterNewObject(JNIEnv *env, jobject obj, void *ptr)
{ 
  if (!vtkInstanceLookup) // first call ?
    {
    vtkInstanceLookup = new vtkHashTable();
    vtkPointerLookup = new vtkHashTable();
    vtkTypecastLookup = new vtkHashTable();

    // Java does not guarantee object destruction.  Do not produce an
    // error when leaks are detected.
    vtkDebugLeaks::SetExitError(0);

#ifdef _WIN32
    vtkGlobalMutex = CreateMutex(NULL, FALSE, NULL);
#endif
    }

VTK_GET_MUTEX();

#ifdef VTKJAVADEBUG
  vtkGenericWarningMacro("RegisterNewObject: Adding an object to hash ptr = " << ptr);
#endif  
  // lets make sure it isn't already there
  int id = 0;
  id = vtkJavaGetId(env,obj);
  if (id)
    {
#ifdef VTKJAVADEBUG
    vtkGenericWarningMacro("RegisterNewObject: Attempt to add an object to the hash when one already exists!!!");
#endif
    VTK_RELEASE_MUTEX();
    return id;
    }

  // get a unique id for this object
  // just use vtkJavaIdCount and then increment
  // to handle loop around make sure the id isn't currently in use
  while (vtkInstanceLookup->GetHashTableValue(vtkJavaIdCount))
    {
    vtkJavaIdCount++;
    if (vtkJavaIdCount > 268435456) vtkJavaIdCount = 1;
    }
  id = vtkJavaIdCount;
  vtkInstanceLookup->AddHashEntry((void *)(size_t)vtkJavaIdCount,ptr);

#ifdef JNI_VERSION_1_2
  vtkPointerLookup->AddHashEntry(ptr,(void *)env->NewWeakGlobalRef(obj));
#else
  vtkPointerLookup->AddHashEntry(ptr,(void *)env->NewGlobalRef(obj));
#endif
  vtkJavaSetId(env,obj,vtkJavaIdCount);
  
#ifdef VTKJAVADEBUG
  vtkGenericWarningMacro("RegisterNewObject: Added object to hash id= " << vtkJavaIdCount << " " << ptr);
#endif  
  vtkJavaIdCount++;
  VTK_RELEASE_MUTEX();
  return id;
}

// delete an object from the hash
// doesn't need a mutex because it is only called from within
// the above func which does have a mutex
JNIEXPORT void vtkJavaDeleteObjectFromHash(JNIEnv *env, int id)
{
  void *ptr;
  void *vptr;
  
  ptr = vtkInstanceLookup->GetHashTableValue(id);
  if (!ptr) 
    {
#ifdef VTKJAVADEBUG
    vtkGenericWarningMacro("DeleteObjectFromHash: Attempt to delete an object that doesn't exist!");
#endif  
    return;
    }
  vtkInstanceLookup->DeleteHashEntry((void *)(size_t)id);
  vtkTypecastLookup->DeleteHashEntry((void *)(size_t)id);
  vptr = vtkPointerLookup->GetHashTableValue(ptr);
#ifdef JNI_VERSION_1_2
  env->DeleteWeakGlobalRef((jweak)vptr);
#else
  env->DeleteGlobalRef((jobject)vptr);
#endif
  vtkPointerLookup->DeleteHashEntry(ptr);
}

// should we delete this object
JNIEXPORT void vtkJavaDeleteObject(JNIEnv *env,jobject obj)
{
  int id = vtkJavaGetId(env,obj);
  
  VTK_GET_MUTEX();

#ifdef VTKJAVADEBUG
  vtkGenericWarningMacro("DeleteObject: Deleting id = " << id);
#endif
  vtkJavaDeleteObjectFromHash(env, id);
  VTK_RELEASE_MUTEX();
}



JNIEXPORT jobject vtkJavaGetObjectFromPointer(void *ptr)
{
  jobject obj;

#ifdef VTKJAVADEBUG
  vtkGenericWarningMacro("GetObjectFromPointer: Checking into pointer " << ptr);
#endif  
  obj = (jobject)vtkPointerLookup->GetHashTableValue((jobject *)ptr);
#ifdef VTKJAVADEBUG
  vtkGenericWarningMacro("GetObjectFromPointer: Checking into pointer " << ptr << " obj = " << obj);
#endif  
  return obj;
}

JNIEXPORT void *vtkJavaGetPointerFromObject(JNIEnv *env, jobject obj, char *result_type)
{
  void *ptr;
  void *(*command)(void *,char *);
  int id;

  if (!obj)
    {
    return NULL;  
    }
  
  id = vtkJavaGetId(env,obj);
  VTK_GET_MUTEX();
  ptr = vtkInstanceLookup->GetHashTableValue(id);
  command = (void *(*)(void *,char *))vtkTypecastLookup->GetHashTableValue(id);
  VTK_RELEASE_MUTEX();

#ifdef VTKJAVADEBUG
  vtkGenericWarningMacro("GetPointerFromObject: Checking into id " << id << " ptr = " << ptr);
#endif  

  if (!ptr)
    {
    return NULL;
    }
  
  void* res= command(ptr,result_type);
  if (res)
    {
#ifdef VTKJAVADEBUG
    vtkGenericWarningMacro("GetPointerFromObject: Got id= " << id << " ptr= " << ptr << " " << result_type);
#endif  
    return res;
    }
  else
    {
    vtkGenericWarningMacro("GetPointerFromObject: vtk bad argument, type conversion failed.");
    return NULL;
    }
}

JNIEXPORT jobject vtkJavaCreateNewJavaStubForObject(JNIEnv *env, vtkObject* obj)
{
  char fullname[512];
  const char* classname= obj->GetClassName();
  fullname[0]= 'v';
  fullname[1]= 't';
  fullname[2]= 'k';
  fullname[3]= '/';      
  strcpy(&fullname[4], classname);
  obj->Register(obj);
  return vtkJavaCreateNewJavaStub(env, fullname, (void*)obj);
}

JNIEXPORT jobject vtkJavaCreateNewJavaStub(JNIEnv *env, const char* fullclassname, void* obj)
{
  jclass cl= env->FindClass(fullclassname);
  if (!cl) { return NULL; }
  
  jobject stub= env->NewObject(cl, env->GetMethodID(cl, "<init>","(I)V"), (int)0 );
  vtkJavaRegisterNewObject(env, stub, obj);
  env->CallVoidMethod(stub, env->GetMethodID(cl, "VTKCastInit", "()V"));
  return stub;
}


JNIEXPORT jarray vtkJavaMakeJArrayOfDoubleFromDouble(JNIEnv *env, double *ptr, int size)
{
  jdoubleArray ret;
  int i;
  jdouble *array;

  ret = env->NewDoubleArray(size);
  if (ret == 0)
    {
    // should throw an exception here
    return 0;
    }

  array = env->GetDoubleArrayElements(ret,NULL);

  // copy the data
  for (i = 0; i < size; i++)
    {
    array[i] = ptr[i];
    }
  
  env->ReleaseDoubleArrayElements(ret,array,0);
  return ret;
}

JNIEXPORT jarray vtkJavaMakeJArrayOfDoubleFromFloat(JNIEnv *env, float *ptr, int size)
{
  jdoubleArray ret;
  int i;
  jdouble *array;

  ret = env->NewDoubleArray(size);
  if (ret == 0)
    {
    // should throw an exception here
    return 0;
    }

  array = env->GetDoubleArrayElements(ret,NULL);

  // copy the data
  for (i = 0; i < size; i++)
    {
    array[i] = ptr[i];
    }
  
  env->ReleaseDoubleArrayElements(ret,array,0);
  return ret;
}

JNIEXPORT jarray vtkJavaMakeJArrayOfIntFromInt(JNIEnv *env, int *ptr, int size)
{
  jintArray ret;
  int i;
  jint *array;

  ret = env->NewIntArray(size);
  if (ret == 0)
    {
    // should throw an exception here
    return 0;
    }

  array = env->GetIntArrayElements(ret,NULL);

  // copy the data
  for (i = 0; i < size; i++)
    {
    array[i] = ptr[i];
    }
  
  env->ReleaseIntArrayElements(ret,array,0);
  return ret;
}

JNIEXPORT jarray vtkJavaMakeJArrayOfIntFromIdType(JNIEnv *env, vtkIdType *ptr, int size)
{
  jintArray ret;
  int i;
  jint *array;

  ret = env->NewIntArray(size);
  if (ret == 0)
    {
    // should throw an exception here
    return 0;
    }

  array = env->GetIntArrayElements(ret,NULL);

  // copy the data
  for (i = 0; i < size; i++)
    {
    array[i] = (int)ptr[i];
    }
  
  env->ReleaseIntArrayElements(ret,array,0);
  return ret;
}

#if defined(VTK_TYPE_USE_LONG_LONG)
JNIEXPORT jarray vtkJavaMakeJArrayOfIntFromLongLong(JNIEnv *env, long long *ptr, int size)
{
  jintArray ret;
  int i;
  jint *array;

  ret = env->NewIntArray(size);
  if (ret == 0)
    {
    // should throw an exception here
    return 0;
    }

  array = env->GetIntArrayElements(ret,NULL);

  // copy the data
  for (i = 0; i < size; i++)
    {
    array[i] = (int)ptr[i];
    }

  env->ReleaseIntArrayElements(ret,array,0);
  return ret;
}
#endif

#if defined(VTK_TYPE_USE___INT64)
JNIEXPORT jarray vtkJavaMakeJArrayOfIntFrom__Int64(JNIEnv *env, __int64 *ptr, int size)
{
  jintArray ret;
  int i;
  jint *array;

  ret = env->NewIntArray(size);
  if (ret == 0)
    {
    // should throw an exception here
    return 0;
    }

  array = env->GetIntArrayElements(ret,NULL);

  // copy the data
  for (i = 0; i < size; i++)
    {
    array[i] = (int)ptr[i];
    }

  env->ReleaseIntArrayElements(ret,array,0);
  return ret;
}
#endif

JNIEXPORT jarray vtkJavaMakeJArrayOfFloatFromFloat(JNIEnv *env, float *ptr, int size)
{
  jfloatArray ret;
  int i;
  jfloat *array;

  ret = env->NewFloatArray(size);
  if (ret == 0)
    {
    // should throw an exception here
    return 0;
    }

  array = env->GetFloatArrayElements(ret,NULL);

  // copy the data
  for (i = 0; i < size; i++)
    {
    array[i] = ptr[i];
    }
  
  env->ReleaseFloatArrayElements(ret,array,0);
  return ret;
}

JNIEXPORT jarray vtkJavaMakeJArrayOfShortFromShort(JNIEnv *env, short *ptr, int size)
{
  jshortArray ret;
  int i;
  jshort *array;

  ret = env->NewShortArray(size);
  if (ret == 0)
    {
    // should throw an exception here
    return 0;
    }

  array = env->GetShortArrayElements(ret,NULL);

  // copy the data
  for (i = 0; i < size; i++)
    {
    array[i] = ptr[i];
    }
  
  env->ReleaseShortArrayElements(ret,array,0);
  return ret;
}

JNIEXPORT jarray vtkJavaMakeJArrayOfByteFromUnsignedChar(JNIEnv *env, unsigned char *ptr, int size)
{
  jbyteArray ret;
  int i;
  jbyte *array;

  ret = env->NewByteArray(size);
  if (ret == 0)
    {
    // should throw an exception here
    return 0;
    }

  array = env->GetByteArrayElements(ret,NULL);

  // copy the data
  for (i = 0; i < size; i++)
    {
    array[i] = ptr[i];
    }
  
  env->ReleaseByteArrayElements(ret,array,0);
  return ret;
}

JNIEXPORT jarray vtkJavaMakeJArrayOfLongFromLong(JNIEnv *env, long *ptr, int size)
{
  cout.flush();
  jlongArray ret;
  int i;
  jlong *array;

  ret = env->NewLongArray(size);
  if (ret == 0)
    {
    // should throw an exception here
    return 0;
    }

  array = env->GetLongArrayElements(ret,NULL);

  // copy the data
  for (i = 0; i < size; i++)
    {
    array[i] = ptr[i];
    }
  
  env->ReleaseLongArrayElements(ret,array,0);
  return ret;
}

JNIEXPORT jarray vtkJavaMakeJArrayOfUnsignedLongFromUnsignedLong(JNIEnv *env,unsigned long *ptr,int size)
{
  cout.flush();
  jlongArray ret;
  int i;
  jlong *array;

  ret = env->NewLongArray(size);
  if (ret == 0)
    {
    // should throw an exception here
    return 0;
    }

  array = env->GetLongArrayElements(ret,NULL);

  // copy the data
  for (i = 0; i < size; i++)
    {
    array[i] = ptr[i];
    }
  
  env->ReleaseLongArrayElements(ret,array,0);
  return ret;
}

JNIEXPORT jarray vtkJavaMakeJArrayOfUnsignedShortFromUnsignedShort(JNIEnv *env,unsigned short *ptr,int size)
{
  cout.flush();
  jshortArray ret;
  int i;
  jshort *array;

  ret = env->NewShortArray(size);
  if (ret == 0)
    {
    // should throw an exception here
    return 0;
    }

  array = env->GetShortArrayElements(ret,NULL);

  // copy the data
  for (i = 0; i < size; i++)
    {
    array[i] = ptr[i];
    }
  
  env->ReleaseShortArrayElements(ret,array,0);
  return ret;
}

JNIEXPORT jarray vtkJavaMakeJArrayOfUnsignedCharFromUnsignedChar(JNIEnv *env,unsigned char *ptr,int size)
{
  cout.flush();
  jbyteArray ret;
  int i;
  jbyte *array;

  ret = env->NewByteArray(size);
  if (ret == 0)
    {
    // should throw an exception here
    return 0;
    }

  array = env->GetByteArrayElements(ret,NULL);

  // copy the data
  for (i = 0; i < size; i++)
    {
    array[i] = ptr[i];
    }
  
  env->ReleaseByteArrayElements(ret,array,0);
  return ret;
}

JNIEXPORT jarray vtkJavaMakeJArrayOfUnsignedIntFromUnsignedInt(JNIEnv *env,unsigned int *ptr,int size)
{
  cout.flush();
  jintArray ret;
  int i;
  jint *array;

  ret = env->NewIntArray(size);
  if (ret == 0)
    {
    // should throw an exception here
    return 0;
    }

  array = env->GetIntArrayElements(ret,NULL);

  // copy the data
  for (i = 0; i < size; i++)
    {
    array[i] = ptr[i];
    }
  
  env->ReleaseIntArrayElements(ret,array,0);
  return ret;
}

JNIEXPORT jarray vtkJavaMakeJArrayOfCharFromChar(JNIEnv *env, char *ptr, int size)
{
  cout.flush();
  jcharArray ret;
  int i;
  jchar *array;

  ret = env->NewCharArray(size);
  if (ret == 0)
    {
    // should throw an exception here
    return 0;
    }

  array = env->GetCharArrayElements(ret,NULL);

  // copy the data
  for (i = 0; i < size; i++)
    {
    array[i] = ptr[i];
    }
  
  env->ReleaseCharArrayElements(ret,array,0);
  return ret;
}

JNIEXPORT char *vtkJavaUTFToChar(JNIEnv *env, jstring in)
{
  char *result;
  const char *inBytes;
  int length, i;
  int resultLength = 1;
  
  length = env->GetStringUTFLength(in);
  inBytes = env->GetStringUTFChars(in,NULL);
  
  for (i = 0; i < length; i++)
    {
    if ((inBytes[i] & 0x80) == 0) resultLength++;
    }
  result = new char [resultLength];

  resultLength = 0; // the 0 versus 1 up above is on purpose
  for (i = 0; i < length; i++)
    {
    if ((inBytes[i] & 0x80) == 0)
      {
      result[resultLength] = inBytes[i];
      resultLength++;
      }
    }
  result[resultLength] = '\0';
  env->ReleaseStringUTFChars(in,inBytes);
  return result;
}

JNIEXPORT jstring vtkJavaMakeJavaString(JNIEnv *env, const char *in)
{
  if (!in) {
    return env->NewStringUTF("");
  } else {
    return env->NewStringUTF(in);
  }
}

//**jcp this is the callback inteface stub for Java. no user parms are passed
//since the callback must be a method of a class. We make the rash assumption
//that the <this> pointer will anchor any required other elements for the
//called functions. - edited by km
JNIEXPORT void vtkJavaVoidFunc(void* f) 
{  
  vtkJavaVoidFuncArg *iprm = (vtkJavaVoidFuncArg *)f;
  // make sure we have a valid method ID
  if (iprm->mid)
    {
    JNIEnv *e;
    // it should already be atached
#ifdef JNI_VERSION_1_2
    iprm->vm->AttachCurrentThread((void **)(&e),NULL);
#else
    iprm->vm->AttachCurrentThread((JNIEnv_**)(&e),NULL);
#endif
    e->CallVoidMethod(iprm->uobj,iprm->mid,NULL); 
    }
}

JNIEXPORT void vtkJavaVoidFuncArgDelete(void* arg) 
{
  vtkJavaVoidFuncArg *arg2;
  
  arg2 = (vtkJavaVoidFuncArg *)arg;
  
  JNIEnv *e;
  // it should already be atached
#ifdef JNI_VERSION_1_2
  arg2->vm->AttachCurrentThread((void **)(&e),NULL);
#else
  arg2->vm->AttachCurrentThread((JNIEnv_**)(&e),NULL);
#endif
  // free the structure
  e->DeleteGlobalRef(arg2->uobj);
  delete arg2;
}

jobject vtkJavaExportedGetObjectFromPointer(void *ptr)
{
        return vtkJavaGetObjectFromPointer(ptr);
}

void* vtkJavaExportedGetPointerFromObject(JNIEnv *env,jobject obj, 
                                          char *result_type)
{
        return vtkJavaGetPointerFromObject(env, obj, result_type);
}

vtkJavaCommand::vtkJavaCommand() 
{ 
  this->vm = NULL;
}

vtkJavaCommand::~vtkJavaCommand() 
{ 
  JNIEnv *e;
  // it should already be atached
#ifdef JNI_VERSION_1_2
  this->vm->AttachCurrentThread((void **)(&e),NULL);
#else
  this->vm->AttachCurrentThread((JNIEnv_**)(&e),NULL);
#endif
  // free the structure
  e->DeleteGlobalRef(this->uobj);
}

void vtkJavaCommand::Execute(vtkObject *, unsigned long, void *)
{
  // make sure we have a valid method ID
  if (this->mid)
    {
    JNIEnv *e;
    // it should already be atached
#ifdef JNI_VERSION_1_2
    this->vm->AttachCurrentThread((void **)(&e),NULL);
#else
    this->vm->AttachCurrentThread((JNIEnv_**)(&e),NULL);
#endif
    e->CallVoidMethod(this->uobj,this->mid,NULL); 
    }
}

