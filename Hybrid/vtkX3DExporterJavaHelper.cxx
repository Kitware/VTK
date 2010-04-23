/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkX3DExporterJavaHelper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkX3DExporterJavaHelper.h"

#include "vtkObjectFactory.h"
#include <jni.h>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkX3DExporterJavaHelper);

//----------------------------------------------------------------------------
void vtkX3DExporterJavaHelper::SetFastInfosetJarLocation(const char* location)
{
  if ( vtkX3DExporterJavaHelper::FastInfosetJarLocation != 0 )
    {
    delete [] vtkX3DExporterJavaHelper::FastInfosetJarLocation;
    vtkX3DExporterJavaHelper::FastInfosetJarLocation = 0;
    }
  if ( location )
    {
    vtkX3DExporterJavaHelper::FastInfosetJarLocation = new char[ strlen(location) + 1 ];
    strcpy(vtkX3DExporterJavaHelper::FastInfosetJarLocation, location);
    }
}

char* vtkX3DExporterJavaHelper::FastInfosetJarLocation = 0;

//----------------------------------------------------------------------------
#define vtkX3DCheckJNIObject(str, className) \
  if ( !className ) \
    {\
    cerr << "Cannot find required " << str << ": " << #className << endl; \
    }

//----------------------------------------------------------------------------
class vtkX3DExporterJavaHelperInternal
{
public:
  vtkX3DExporterJavaHelperInternal() :
    JavaVirtualMachine(0), JavaEnvironment(0), X3DBinaryConverterClass_Write(0),
    X3DBinaryConverterClass_Close(0), X3DBinaryConverterObject(0)
    {
    }
  JavaVM* JavaVirtualMachine;
  JNIEnv* JavaEnvironment;
  jmethodID X3DBinaryConverterClass_Write;
  jmethodID X3DBinaryConverterClass_Close;
  jobject X3DBinaryConverterObject;

  jcharArray ConvertToJChar(JNIEnv* env, const char* message)
    {
    const size_t len = strlen(message);
    jcharArray array = env->NewCharArray(len);
    jchar *jarray = env->GetCharArrayElements(array,NULL);
    size_t cc;
    for ( cc = 0; cc < len; ++ cc )
      {
      jarray[cc] = message[cc];
      }
    env->ReleaseCharArrayElements(array,jarray,0);
    return array;
    }

  jobject CreateString(JNIEnv* env, const char* message)
    {
    jclass java_lang_String = env->FindClass("java/lang/String");
    jmethodID java_lang_String_Constructor_From_Array = env->GetMethodID(java_lang_String,
      "<init>", "([C)V");
    jcharArray inputFileNameArray = this->ConvertToJChar(env, message);
    jobject inputFileNameString = env->NewObject(
      java_lang_String, java_lang_String_Constructor_From_Array, inputFileNameArray);
    env->DeleteLocalRef(inputFileNameArray);
    return inputFileNameString;
    }
};

//----------------------------------------------------------------------------
vtkX3DExporterJavaHelper::vtkX3DExporterJavaHelper()
{
  this->Internal = new vtkX3DExporterJavaHelperInternal;

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
      delete this->Internal;
      return;
      }
    }
  if ( !this->Internal->JavaEnvironment )
    {
    JavaVMInitArgs args;
    JavaVMOption options[1];

    //status = JNI_GetDefaultJavaVMInitArgs(&vm_args);

    /* There is a new JNI_VERSION_1_4, but it doesn't add anything for the purposes of our example. */
    args.version = JNI_VERSION_1_2;
    args.nOptions = 1;
    vtkstd::string str;
    if ( vtkX3DExporterJavaHelper::FastInfosetJarLocation )
      {
      str = "-Djava.class.path=";
      str += vtkX3DExporterJavaHelper::FastInfosetJarLocation;
      }
    else
      {
      str = "-Djava.class.path=FastInfoset.jar";
      }
    char* classPath = new char[str.size()+1];
    strcpy(classPath, str.c_str());
    options[0].optionString = classPath;
    args.options = options;
    args.ignoreUnrecognized = JNI_FALSE;

    jint res = JNI_CreateJavaVM(&this->Internal->JavaVirtualMachine, (void **)&this->Internal->JavaEnvironment, &args);
    delete [] classPath;
    if ( res != 0 )
      {
      vtkErrorMacro("Cannot create VM: " << res);
      delete this->Internal;
      return;
      }
    }

  vtkX3DCheckJNIObject("VM", this->Internal->JavaEnvironment);
}

//----------------------------------------------------------------------------
vtkX3DExporterJavaHelper::~vtkX3DExporterJavaHelper()
{
  if ( this->Internal )
    {
    //this->Internal->JavaVirtualMachine->DestroyJavaVM();
    delete this->Internal;
    }
}

//----------------------------------------------------------------------------
int vtkX3DExporterJavaHelper::OpenFile(const char* fileName)
{
  if ( !this->Internal )
    {
    return 0;
    }
  jclass x3dConverterClass = this->Internal->JavaEnvironment->FindClass("vtkX3DBinaryConverter");
  vtkX3DCheckJNIObject("Class", x3dConverterClass);
  jmethodID constructorId = this->Internal->JavaEnvironment->GetMethodID(
    x3dConverterClass, "<init>", "(Ljava/lang/String;)V");
  vtkX3DCheckJNIObject("Constructor", constructorId);
  this->Internal->X3DBinaryConverterClass_Write = this->Internal->JavaEnvironment->GetMethodID(
    x3dConverterClass, "Write", "([B)V");
  vtkX3DCheckJNIObject("Write Method", this->Internal->X3DBinaryConverterClass_Write);
  this->Internal->X3DBinaryConverterClass_Close = this->Internal->JavaEnvironment->GetMethodID(
    x3dConverterClass, "Close", "()V");
  vtkX3DCheckJNIObject("Close Method", this->Internal->X3DBinaryConverterClass_Close);


  jobject outputFileName = this->Internal->CreateString(this->Internal->JavaEnvironment, fileName);
  vtkX3DCheckJNIObject("File Name String", outputFileName);
  this->Internal->X3DBinaryConverterObject = this->Internal->JavaEnvironment->NewObject(
    x3dConverterClass, constructorId, outputFileName);
  vtkX3DCheckJNIObject("X3D Converter Object", this->Internal->X3DBinaryConverterObject);
  return 1;
}

//----------------------------------------------------------------------------
int vtkX3DExporterJavaHelper::Write(const char* data, vtkIdType length)
{
  if ( !this->Internal )
    {
    return 0;
    }
  jbyteArray array = this->Internal->JavaEnvironment->NewByteArray(length);
  jbyte *jarray = this->Internal->JavaEnvironment->GetByteArrayElements(array,NULL);
  vtkIdType cc;
  for ( cc = 0; cc < length; ++ cc )
    {
    jarray[cc] = data[cc];
    }
  this->Internal->JavaEnvironment->ReleaseByteArrayElements(array,jarray,0);
  this->Internal->JavaEnvironment->CallVoidMethod(this->Internal->X3DBinaryConverterObject, this->Internal->X3DBinaryConverterClass_Write,
    array);
  return 1;
}

//----------------------------------------------------------------------------
int vtkX3DExporterJavaHelper::Close()
{
  if ( !this->Internal )
    {
    return 0;
    }
  this->Internal->JavaEnvironment->CallVoidMethod(this->Internal->X3DBinaryConverterObject, this->Internal->X3DBinaryConverterClass_Close);
  return 1;
}
