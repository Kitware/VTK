/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkJVMManager.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkJVMManager.h"

#include "vtkObjectFactory.h"
#include "vtkStringArray.h"

#include <jni.h>
#include <vtksys/stl/vector>
#include <vtksys/ios/sstream>

#if 0
//----------------------------------------------------------------------------
#define vtkJVMManagerCheckMacro(str, obj)                                     \
  if ( !obj )                                                                 \
    {                                                                         \
    vtkErrorMacro( << "Cannot find required " << str << ": " << #obj );       \
    return;                                                                   \
    }
#endif

//----------------------------------------------------------------------------
#define vtkJVMManagerCheckMacro(obj, str, retval)                             \
  if ( !obj )                                                                 \
    {                                                                         \
    vtkErrorMacro( << "Cannot find required \"" << str << "\" " << #obj );    \
    return retval;                                                            \
    }                                                                         \
  if ( this->ExceptionRaised() )                                              \
    {                                                                         \
    this->DescribeException();                                                \
    return retval;                                                            \
    }

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkJVMManager);

//----------------------------------------------------------------------------
class vtkJVMManagerInternal
{
public:
  vtkJVMManagerInternal() :
    JavaVirtualMachine(0), JavaEnvironment(0)
    {
    }
  JavaVM* JavaVirtualMachine;
  JNIEnv* JavaEnvironment;
  vtksys_stl::vector<jvalue> Arguments;
};

vtkStringArray* vtkJVMManager::ClassPaths = 0;
vtkStringArray* vtkJVMManager::LibraryPaths = 0;

//----------------------------------------------------------------------------
vtkJVMManager::vtkJVMManager()
{
  this->Internal = new vtkJVMManagerInternal;
  this->MaximumHeapSizeMB = 256;
}

//----------------------------------------------------------------------------
vtkJVMManager::~vtkJVMManager()
{
  if ( this->Internal )
    {
    delete this->Internal;
    }
}

//----------------------------------------------------------------------------
JNIEnv* vtkJVMManager::GetEnvironment()
{
  return this->Internal->JavaEnvironment;
}

//----------------------------------------------------------------------------
void vtkJVMManager::AddClassPath(const char* path)
{
  if (!ClassPaths)
    {
    ClassPaths = vtkStringArray::New();
    }
  ClassPaths->InsertNextValue(path);
}

//----------------------------------------------------------------------------
void vtkJVMManager::RemoveAllClassPaths()
{
  if (ClassPaths)
    {
    ClassPaths->Delete();
    ClassPaths = 0;
    }
}

//----------------------------------------------------------------------------
void vtkJVMManager::AddLibraryPath(const char* path)
{
  if (!LibraryPaths)
    {
    LibraryPaths = vtkStringArray::New();
    }
  LibraryPaths->InsertNextValue(path);
}

//----------------------------------------------------------------------------
void vtkJVMManager::RemoveAllLibraryPaths()
{
  if (LibraryPaths)
    {
    LibraryPaths->Delete();
    LibraryPaths = 0;
    }
}

//----------------------------------------------------------------------------
void vtkJVMManager::CreateJVM()
{
  JavaVM* VMs;
  jsize numVMs = 0;
  jint status = JNI_GetCreatedJavaVMs(&VMs, 10, &numVMs);
  if ( numVMs > 0 )
    {
    this->Internal->JavaVirtualMachine = VMs;
    status = this->Internal->JavaVirtualMachine->GetEnv(
      (void**)&this->Internal->JavaEnvironment, JNI_VERSION_1_2);
    if ( status != 0 )
      {
      vtkErrorMacro("Cannot attach to the Java Virtual Machine");
      return;
      }
    }
  if ( !this->Internal->JavaEnvironment )
    {
    JavaVMInitArgs args;

    args.version = JNI_VERSION_1_4;
    args.ignoreUnrecognized = JNI_FALSE;
    args.nOptions = 1;
    JavaVMOption options[3];
    vtksys_ios::ostringstream oss;
    oss << "-Xmx" << this->MaximumHeapSizeMB << "M" << ends;
    vtkStdString memStr = oss.str();
    options[0].optionString = const_cast<char*>(memStr.c_str());
    vtkStdString classStr;
    if (this->ClassPaths && this->ClassPaths->GetNumberOfTuples() > 0)
      {
      args.nOptions++;
      classStr += "-Djava.class.path=";
      vtkIdType numClassPaths = this->ClassPaths->GetNumberOfTuples();
      for (vtkIdType i = 0; i < numClassPaths; ++i)
        {
        if (i > 0)
          {
          classStr += ":";
          }
        classStr += this->ClassPaths->GetValue(i);
        }
      options[args.nOptions-1].optionString = const_cast<char*>(classStr.c_str());
      }
    vtkStdString libraryStr;
    if (this->LibraryPaths && this->LibraryPaths->GetNumberOfTuples() > 0)
      {
      args.nOptions++;
      libraryStr += "-Djava.library.path=";
      vtkIdType numLibraryPaths = this->LibraryPaths->GetNumberOfTuples();
      for (vtkIdType i = 0; i < numLibraryPaths; ++i)
        {
        if (i > 0)
          {
          libraryStr += ":";
          }
        libraryStr += this->LibraryPaths->GetValue(i);
        }
      options[args.nOptions-1].optionString = const_cast<char*>(libraryStr.c_str());
      }
    args.options = options;
    jint res = JNI_CreateJavaVM(
      &this->Internal->JavaVirtualMachine,
      (void **)&this->Internal->JavaEnvironment,
      &args);
    if ( res != 0 )
      {
      vtkErrorMacro("Cannot create VM: " << res);
      return;
      }
    }

  vtkJVMManagerCheckMacro(this->Internal->JavaEnvironment, "VM", );
}

//----------------------------------------------------------------------------
const char* vtkJVMManager::GetStringCharacters(jstring str)
{
  if (!this->Internal->JavaEnvironment)
    {
    return 0;
    }
  return this->Internal->JavaEnvironment->GetStringUTFChars(str, 0);
}

//----------------------------------------------------------------------------
void vtkJVMManager::ReleaseStringCharacters(jstring str, const char* cstr)
{
  if (!this->Internal->JavaEnvironment)
    {
    return;
    }
  this->Internal->JavaEnvironment->ReleaseStringUTFChars(str, cstr);
}

//----------------------------------------------------------------------------
jstring vtkJVMManager::NewString(const char* str)
{
  if (!this->Internal->JavaEnvironment)
    {
    return 0;
    }
  return this->Internal->JavaEnvironment->NewStringUTF(str);
}

//----------------------------------------------------------------------------
jobjectArray vtkJVMManager::NewObjectArray(const char* name, int length)
{
  if (!this->Internal->JavaEnvironment)
    {
    return 0;
    }
  jclass cls = this->Internal->JavaEnvironment->FindClass(name);
  vtkJVMManagerCheckMacro(cls, name, 0);
  jobjectArray arr = this->Internal->JavaEnvironment->NewObjectArray(length, cls, 0);
  return arr;
}

//----------------------------------------------------------------------------
void vtkJVMManager::SetObjectArrayElement(jobjectArray arr, int i, jobject obj)
{
  if (!this->Internal->JavaEnvironment)
    {
    return;
    }
  this->Internal->JavaEnvironment->SetObjectArrayElement(arr, i, obj);
}

//----------------------------------------------------------------------------
jobject vtkJVMManager::GetObjectArrayElement(jobjectArray arr, int i)
{
  if (!this->Internal->JavaEnvironment)
    {
    return 0;
    }
  return this->Internal->JavaEnvironment->GetObjectArrayElement(arr, i);
}

//----------------------------------------------------------------------------
jobject vtkJVMManager::NewObject(const char* name, const char* signature,
  jvariant arg1)
{
  this->RemoveAllArguments();
  this->AddArgument(arg1);
  return this->NewObject(name, signature);
}

//----------------------------------------------------------------------------
jobject vtkJVMManager::NewObject(const char* name, const char* signature,
  jvariant arg1, jvariant arg2)
{
  this->RemoveAllArguments();
  this->AddArgument(arg1);
  this->AddArgument(arg2);
  return this->NewObject(name, signature);
}

//----------------------------------------------------------------------------
jobject vtkJVMManager::NewObject(const char* name, const char* signature,
  jvariant arg1, jvariant arg2, jvariant arg3)
{
  this->RemoveAllArguments();
  this->AddArgument(arg1);
  this->AddArgument(arg2);
  this->AddArgument(arg3);
  return this->NewObject(name, signature);
}

//----------------------------------------------------------------------------
jobject vtkJVMManager::NewObject(const char* name, const char* signature)
{
  if (!this->Internal->JavaEnvironment)
    {
    return 0;
    }
  jclass cls = this->Internal->JavaEnvironment->FindClass(name);
  vtkJVMManagerCheckMacro(cls, name, 0);
  jmethodID constructor = this->Internal->JavaEnvironment->GetMethodID(
    cls, "<init>", signature);
  vtkJVMManagerCheckMacro(constructor, signature, 0);
  jvalue* args = 0;
  if (this->Internal->Arguments.size() > 0)
    {
    args = &this->Internal->Arguments[0];
    }
  jobject obj = this->Internal->JavaEnvironment->NewObjectA(
    cls, constructor, args);
  vtkJVMManagerCheckMacro(obj, name, 0);
  this->RemoveAllArguments();
  return obj;
}

//----------------------------------------------------------------------------
jvariant vtkJVMManager::CallMethod(jobject obj, const char* name, const char* signature,
  jvariant arg1)
{
  this->RemoveAllArguments();
  this->AddArgument(arg1);
  return this->CallMethod(obj, name, signature);
}

//----------------------------------------------------------------------------
jvariant vtkJVMManager::CallMethod(jobject obj, const char* name, const char* signature,
  jvariant arg1, jvariant arg2)
{
  this->RemoveAllArguments();
  this->AddArgument(arg1);
  this->AddArgument(arg2);
  return this->CallMethod(obj, name, signature);
}

//----------------------------------------------------------------------------
jvariant vtkJVMManager::CallMethod(jobject obj, const char* name, const char* signature,
  jvariant arg1, jvariant arg2, jvariant arg3)
{
  this->RemoveAllArguments();
  this->AddArgument(arg1);
  this->AddArgument(arg2);
  this->AddArgument(arg3);
  return this->CallMethod(obj, name, signature);
}

//----------------------------------------------------------------------------
jvariant vtkJVMManager::CallMethod(jobject obj, const char* name, const char* signature)
{
  jvariant ret;
  if (!this->Internal->JavaEnvironment)
    {
    return ret;
    }
  jclass cls = this->Internal->JavaEnvironment->GetObjectClass(obj);
  vtkJVMManagerCheckMacro(cls, "Class from object", ret);
  jmethodID method = this->Internal->JavaEnvironment->GetMethodID(
    cls, name, signature);
  vtkStdString methodString(name);
  methodString += signature;
  jvalue* args = 0;
  if (this->Internal->Arguments.size() > 0)
    {
    args = &this->Internal->Arguments[0];
    }
  vtkJVMManagerCheckMacro(method, methodString.c_str(), ret);
  if (signature[strlen(signature)-2] != ')')
    {
    ret.Type = 'L';
    ret.Value.l = this->Internal->JavaEnvironment->CallObjectMethodA(
      obj, method, args);
    }
  else
    {
    char retType = signature[strlen(signature)-1];
    ret.Type = retType;
    switch(retType)
      {
      case 'V':
        this->Internal->JavaEnvironment->CallVoidMethodA(
          obj, method, args);
        break;
      case 'Z':
        ret.Value.z = this->Internal->JavaEnvironment->CallBooleanMethodA(
          obj, method, args);
        break;
      case 'B':
        ret.Value.b = this->Internal->JavaEnvironment->CallByteMethodA(
          obj, method, args);
        break;
      case 'C':
        ret.Value.c = this->Internal->JavaEnvironment->CallCharMethodA(
          obj, method, args);
        break;
      case 'S':
        ret.Value.s = this->Internal->JavaEnvironment->CallShortMethodA(
          obj, method, args);
        break;
      case 'I':
        ret.Value.i = this->Internal->JavaEnvironment->CallIntMethodA(
          obj, method, args);
        break;
      case 'J':
        ret.Value.j = this->Internal->JavaEnvironment->CallLongMethodA(
          obj, method, args);
        break;
      case 'F':
        ret.Value.f = this->Internal->JavaEnvironment->CallFloatMethodA(
          obj, method, args);
        break;
      case 'D':
        ret.Value.d = this->Internal->JavaEnvironment->CallDoubleMethodA(
          obj, method, args);
        break;
      }
    }
  this->RemoveAllArguments();
  return ret;
}

//----------------------------------------------------------------------------
jvariant vtkJVMManager::CallStaticMethod(const char* cname, const char* name, const char* signature,
  jvariant arg1)
{
  this->RemoveAllArguments();
  this->AddArgument(arg1);
  return this->CallStaticMethod(cname, name, signature);
}

//----------------------------------------------------------------------------
jvariant vtkJVMManager::CallStaticMethod(const char* cname, const char* name, const char* signature,
  jvariant arg1, jvariant arg2)
{
  this->RemoveAllArguments();
  this->AddArgument(arg1);
  this->AddArgument(arg2);
  return this->CallStaticMethod(cname, name, signature);
}

//----------------------------------------------------------------------------
jvariant vtkJVMManager::CallStaticMethod(const char* cname, const char* name, const char* signature,
  jvariant arg1, jvariant arg2, jvariant arg3)
{
  this->RemoveAllArguments();
  this->AddArgument(arg1);
  this->AddArgument(arg2);
  this->AddArgument(arg3);
  return this->CallStaticMethod(cname, name, signature);
}

//----------------------------------------------------------------------------
jvariant vtkJVMManager::CallStaticMethod(const char* cname, const char* name, const char* signature)
{
  jvariant ret;
  if (!this->Internal->JavaEnvironment)
    {
    return ret;
    }
  jclass cls = this->Internal->JavaEnvironment->FindClass(cname);
  vtkJVMManagerCheckMacro(cls, cname, ret);
  jmethodID method = this->Internal->JavaEnvironment->GetStaticMethodID(
    cls, name, signature);
  vtkStdString methodString(cname);
  methodString += "::";
  methodString += name;
  methodString += signature;
  vtkJVMManagerCheckMacro(method, methodString.c_str(), ret);
  jvalue* args = 0;
  if (this->Internal->Arguments.size() > 0)
    {
    args = &this->Internal->Arguments[0];
    }
  if (signature[strlen(signature)-2] != ')')
    {
    ret.Type = 'L';
    ret.Value.l = this->Internal->JavaEnvironment->CallStaticObjectMethodA(
      cls, method, args);
    }
  else
    {
    char retType = signature[strlen(signature)-1];
    ret.Type = retType;
    switch(retType)
      {
      case 'V':
        this->Internal->JavaEnvironment->CallStaticVoidMethodA(
          cls, method, args);
        break;
      case 'Z':
        ret.Value.z = this->Internal->JavaEnvironment->CallStaticBooleanMethodA(
          cls, method, args);
        break;
      case 'B':
        ret.Value.b = this->Internal->JavaEnvironment->CallStaticByteMethodA(
          cls, method, args);
        break;
      case 'C':
        ret.Value.c = this->Internal->JavaEnvironment->CallStaticCharMethodA(
          cls, method, args);
        break;
      case 'S':
        ret.Value.s = this->Internal->JavaEnvironment->CallStaticShortMethodA(
          cls, method, args);
        break;
      case 'I':
        ret.Value.i = this->Internal->JavaEnvironment->CallStaticIntMethodA(
          cls, method, args);
        break;
      case 'J':
        ret.Value.j = this->Internal->JavaEnvironment->CallStaticLongMethodA(
          cls, method, args);
        break;
      case 'F':
        ret.Value.f = this->Internal->JavaEnvironment->CallStaticFloatMethodA(
          cls, method, args);
        break;
      case 'D':
        ret.Value.d = this->Internal->JavaEnvironment->CallStaticDoubleMethodA(
          cls, method, args);
        break;
      }
    }
  this->RemoveAllArguments();
  return ret;
}

//----------------------------------------------------------------------------
void vtkJVMManager::AddArgument(jvariant arg)
{
  this->Internal->Arguments.push_back(arg.Value);
}

//----------------------------------------------------------------------------
void vtkJVMManager::RemoveAllArguments()
{
  this->Internal->Arguments.clear();
}

//----------------------------------------------------------------------------
bool vtkJVMManager::ExceptionRaised()
{
return (this->Internal->JavaEnvironment->ExceptionCheck() ? 1 : 0);
}

//----------------------------------------------------------------------------
void vtkJVMManager::DescribeException()
{
  this->Internal->JavaEnvironment->ExceptionDescribe();
}

//----------------------------------------------------------------------------
void vtkJVMManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "MaximumHeapSizeMB: " << this->MaximumHeapSizeMB << endl;
}

