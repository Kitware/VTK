/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkJVMManager.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkJVMManager - Manages a java virtual machine.
// .SECTION Description

#ifndef __vtkJVMManager_h
#define __vtkJVMManager_h

#include "vtkObject.h"

#include <jni.h> // For JNI types

class vtkJVMManagerInternal;
class vtkStringArray;

//BTX
class jvariant
{
public:
  jvariant()           { this->Type = 0; }
  jvariant(jboolean z) { this->Value.z = z; this->Type = 'Z'; }
  jvariant(jbyte b)    { this->Value.b = b; this->Type = 'B'; }
  jvariant(jchar c)    { this->Value.c = c; this->Type = 'C'; }
  jvariant(jshort s)   { this->Value.s = s; this->Type = 'S'; }
  jvariant(jint i)     { this->Value.i = i; this->Type = 'I'; }
  jvariant(jlong j)    { this->Value.j = j; this->Type = 'J'; }
  jvariant(jfloat f)   { this->Value.f = f; this->Type = 'F'; }
  jvariant(jdouble d)  { this->Value.d = d; this->Type = 'D'; }
  jvariant(jobject l)  { this->Value.l = l; this->Type = 'L'; }
#if 1
  operator jboolean() { return (this->Type == 'Z') ? this->Value.z : 0; }
  operator jbyte()    { return (this->Type == 'B') ? this->Value.b : 0; }
  operator jchar()    { return (this->Type == 'C') ? this->Value.c : 0; }
  operator jshort()   { return (this->Type == 'S') ? this->Value.s : 0; }
  operator jint()     { return (this->Type == 'I') ? this->Value.i : 0; }
  operator jlong()    { return (this->Type == 'J') ? this->Value.j : 0; }
  operator jfloat()   { return (this->Type == 'F') ? this->Value.f : 0.0f; }
  operator jdouble()  { return (this->Type == 'D') ? this->Value.d : 0.0; }
  operator jobject()  { return (this->Type == 'L') ? this->Value.l : 0; }
  operator jstring()  { return (this->Type == 'L') ? reinterpret_cast<jstring>(this->Value.l) : 0; }
#else
  operator jboolean() { return this->Value.z; }
  operator jbyte()    { return this->Value.b; }
  operator jchar()    { return this->Value.c; }
  operator jshort()   { return this->Value.s; }
  operator jint()     { return this->Value.i; }
  operator jlong()    { return this->Value.j; }
  operator jfloat()   { return this->Value.f; }
  operator jdouble()  { return this->Value.d; }
  operator jobject()  { return this->Value.l; }
  operator jstring()  { return reinterpret_cast<jstring>(this->Value.l); }
#endif
  jvalue Value;
  char Type;
};
//ETX

class VTK_GRAPHICS_JAVA_EXPORT vtkJVMManager : public vtkObject
{
public:
  static vtkJVMManager *New();
  vtkTypeMacro(vtkJVMManager,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Add a path (or jar file) to the places Java will look for
  // classes. LoadJVM uses this list to set the java.class.path for
  // the virtual machine when it starts.
  static void AddClassPath(const char * path);
  
  // Description:
  // Remove all class paths.
  static void RemoveAllClassPaths();
  
  // Description:
  // Add a path to the places Java will look for shared libraries.
  // LoadJVM uses this list to set java.library.path for
  // the virtual machine when it starts.
  static void AddLibraryPath(const char * path);
  
  // Description:
  // Remove all class paths.
  static void RemoveAllLibraryPaths();
  
  // Description:
  // The maximum heap size of the virtual machine, in megabytes.
  vtkSetMacro(MaximumHeapSizeMB, unsigned int);
  vtkGetMacro(MaximumHeapSizeMB, unsigned int);
  
  // Description:
  // Start the Java Virtual Machine. This method must be called
  // after any AddClassPath calls, and before any method invocations.
  void CreateJVM();
  
  // Description:
  // Load an argument into the current argument list.
  // Use this method if you need to call a constructor or method with more than three
  // arguments.
  void AddArgument(jvariant arg);
  
  // Description:
  // Clear the argument list.
  // The argument list is automatically cleared after each call to
  // New... or Call... methods.
  void RemoveAllArguments();
  
  //BTX
  const char* GetStringCharacters(jstring str);
  void ReleaseStringCharacters(jstring str, const char* cstr);
  jstring NewString(const char* str);
  jobjectArray NewObjectArray(const char* name, int length);
  void SetObjectArrayElement(jobjectArray arr, int i, jobject obj);
  jobject GetObjectArrayElement(jobjectArray arr, int i);
  jobject NewObject(const char* name, const char* signature);
  jobject NewObject(const char* name, const char* signature, jvariant arg1);
  jobject NewObject(const char* name, const char* signature, jvariant arg1, jvariant arg2);
  jobject NewObject(const char* name, const char* signature, jvariant arg1, jvariant arg2, jvariant arg3);
  jvariant CallMethod(jobject obj, const char* name, const char* signature);
  jvariant CallMethod(jobject obj, const char* name, const char* signature, jvariant arg1);
  jvariant CallMethod(jobject obj, const char* name, const char* signature, jvariant arg1, jvariant arg2);
  jvariant CallMethod(jobject obj, const char* name, const char* signature, jvariant arg1, jvariant arg2, jvariant arg3);
  jvariant CallStaticMethod(const char* clazz, const char* name, const char* signature);
  jvariant CallStaticMethod(const char* clazz, const char* name, const char* signature, jvariant arg1);
  jvariant CallStaticMethod(const char* clazz, const char* name, const char* signature, jvariant arg1, jvariant arg2);
  jvariant CallStaticMethod(const char* clazz, const char* name, const char* signature, jvariant arg1, jvariant arg2, jvariant arg3);
  JNIEnv* GetEnvironment();
  //ETX
  
  bool ExceptionRaised();
  void DescribeException();
  
protected:
  vtkJVMManager();
  ~vtkJVMManager();

  //BTX
  vtkJVMManagerInternal* Internal;
  static vtkStringArray* ClassPaths;
  static vtkStringArray* LibraryPaths;
  unsigned int MaximumHeapSizeMB;
  //ETX

private:
  vtkJVMManager(const vtkJVMManager&); // Not implemented.
  void operator=(const vtkJVMManager&); // Not implemented.
};

#endif

