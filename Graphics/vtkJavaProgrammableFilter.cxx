/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkJavaProgrammableFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkJavaProgrammableFilter.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkJVMManager.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"
#include "vtkTable.h"

#include <jni.h>
#include <vtksys/SystemTools.hxx>
#include <vtksys/stl/map>
#include <vtksys/stl/vector>

class vtkJavaProgrammableFilterInternals
{
public:
  jobject JavaAlgorithm;
  vtksys_stl::map<vtkStdString, vtkVariant> Parameters;
};

vtkStandardNewMacro(vtkJavaProgrammableFilter);
//---------------------------------------------------------------------------
vtkJavaProgrammableFilter::vtkJavaProgrammableFilter()
{
  this->Internals = new vtkJavaProgrammableFilterInternals();
  this->JVM = vtkJVMManager::New();
  this->JavaClassNameInternal = 0;
  this->JavaClassPathInternal = 0;
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//---------------------------------------------------------------------------
vtkJavaProgrammableFilter::~vtkJavaProgrammableFilter()
{
  delete this->Internals;
  this->SetJavaClassNameInternal(0);
  this->SetJavaClassPathInternal(0);
  if (this->JVM)
    {
    this->JVM->Delete();
    }
}

//---------------------------------------------------------------------------
void vtkJavaProgrammableFilter::SetJavaClassName(const char* name)
{
  this->SetJavaClassNameInternal(name);
  if (this->JavaClassNameInternal && this->JavaClassPathInternal)
    {
    this->Initialize();
    }
}

//---------------------------------------------------------------------------
void vtkJavaProgrammableFilter::SetJavaClassPath(const char* name)
{
  this->SetJavaClassPathInternal(name);
  if (this->JavaClassNameInternal && this->JavaClassPathInternal)
    {
    this->Initialize();
    }
}

//---------------------------------------------------------------------------
void vtkJavaProgrammableFilter::SetParameter(const char* name, int value)
{
  this->Internals->Parameters[name] = vtkVariant(value);
  this->Modified();
}

//---------------------------------------------------------------------------
void vtkJavaProgrammableFilter::SetParameter(const char* name, double value)
{
  this->Internals->Parameters[name] = vtkVariant(value);
  this->Modified();
}

//---------------------------------------------------------------------------
void vtkJavaProgrammableFilter::SetParameter(const char* name, const char* value)
{
  this->Internals->Parameters[name] = vtkVariant(value);
  this->Modified();
}

//---------------------------------------------------------------------------
int vtkJavaProgrammableFilter::GetIntParameter(const char* name)
{
  return this->Internals->Parameters[name].ToInt();
}

//---------------------------------------------------------------------------
double vtkJavaProgrammableFilter::GetDoubleParameter(const char* name)
{
  return this->Internals->Parameters[name].ToDouble();
}

//---------------------------------------------------------------------------
const char* vtkJavaProgrammableFilter::GetStringParameter(const char* name)
{
  return this->Internals->Parameters[name].ToString().c_str();
}

//---------------------------------------------------------------------------
bool vtkJavaProgrammableFilter::Initialize()
{
  this->JVM->CreateJVM();
  
  // Load the new class path
  vtksys_stl::vector<vtksys::String> paths = vtksys::SystemTools::SplitString(this->JavaClassPathInternal, ':');
  for (unsigned int i = 0; i < paths.size(); ++i)
    {
    jstring str = this->JVM->NewString(paths[i].c_str());
    this->JVM->CallStaticMethod("vtk/DynamicClassLoader", "addFile", "(Ljava/lang/String;)V", str);
    }
  
  this->Internals->JavaAlgorithm = this->JVM->NewObject(this->JavaClassNameInternal, "()V");
  jobject javathis = this->JVM->NewObject("vtk/vtkJavaProgrammableFilter", "(J)V", reinterpret_cast<jlong>(this));
  this->JVM->CallMethod(this->Internals->JavaAlgorithm, "initialize",
      "(Lvtk/vtkJavaProgrammableFilter;)V", javathis);
  
  return true;
}

//---------------------------------------------------------------------------
int vtkJavaProgrammableFilter::FillInputPortInformation(int port, vtkInformation* info)
{
  if (!this->Internals->JavaAlgorithm)
    {
    return 1;
    }
  jobject javainfo = this->JVM->NewObject("vtk/vtkInformation", "(J)V", reinterpret_cast<jlong>(info));
  jboolean b = this->JVM->CallMethod(this->Internals->JavaAlgorithm, "fillInputPortInformation", "(ILvtk/vtkInformation;)Z", static_cast<jint>(port), javainfo);
  return b ? 1 : 0;
}

//----------------------------------------------------------------------------
int vtkJavaProgrammableFilter::FillOutputPortInformation(
  int port, vtkInformation* info)
{
  if (!this->Internals->JavaAlgorithm)
    {
    return 1;
    }
  jobject javainfo = this->JVM->NewObject("vtk/vtkInformation", "(J)V", reinterpret_cast<jlong>(info));
  jboolean b = this->JVM->CallMethod(this->Internals->JavaAlgorithm, "fillOutputPortInformation", "(ILvtk/vtkInformation;)Z", static_cast<jint>(port), javainfo);
  return b ? 1 : 0;
}

//----------------------------------------------------------------------------
int vtkJavaProgrammableFilter::ProcessRequest(
  vtkInformation* request, 
  vtkInformationVector** inputVector, 
  vtkInformationVector* outputVector)
{
  // generate the data
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
    {
    return this->RequestData(request, inputVector, outputVector);
    }

  // create the output
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
    {
    return this->RequestDataObject(request, inputVector, outputVector);
    }

  // execute information
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
    {
    return this->RequestInformation(request, inputVector, outputVector);
    }

  // set update extent
 if(request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
    {
    return this->RequestUpdateExtent(request, inputVector, outputVector);
    }
 
  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//---------------------------------------------------------------------------
int vtkJavaProgrammableFilter::RequestData(
  vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  if (!this->Internals->JavaAlgorithm)
    {
    return 1;
    }
  jobject javaRequest = this->JVM->NewObject("vtk/vtkInformation",
      "(J)V", reinterpret_cast<jlong>(request));
  int numInputs = this->GetNumberOfInputPorts();
  jobjectArray javaInputVector = this->JVM->NewObjectArray("vtk/vtkInformationVector", numInputs);
  for (int i = 0; i < numInputs; ++i)
    {
    jobject javaCurInputVec = this->JVM->NewObject("vtk/vtkInformationVector",
        "(J)V", reinterpret_cast<jlong>(inputVector[i]));
    this->JVM->SetObjectArrayElement(javaInputVector, i, javaCurInputVec);
    }
  jobject javaOutputVector = this->JVM->NewObject("vtk/vtkInformationVector",
      "(J)V", reinterpret_cast<jlong>(outputVector));
  jboolean b = this->JVM->CallMethod(this->Internals->JavaAlgorithm, "requestData",
      "(Lvtk/vtkInformation;[Lvtk/vtkInformationVector;Lvtk/vtkInformationVector;)Z",
      javaRequest, javaInputVector, javaOutputVector);
  return b ? 1 : 0;
}

//---------------------------------------------------------------------------
int vtkJavaProgrammableFilter::RequestDataObject(
  vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  if (!this->Internals->JavaAlgorithm)
    {
    return 1;
    }
  jobject javaRequest = this->JVM->NewObject("vtk/vtkInformation",
      "(J)V", reinterpret_cast<jlong>(request));
  int numInputs = this->GetNumberOfInputPorts();
  jobjectArray javaInputVector = this->JVM->NewObjectArray("vtk/vtkInformationVector", numInputs);
  for (int i = 0; i < numInputs; ++i)
    {
    jobject javaCurInputVec = this->JVM->NewObject("vtk/vtkInformationVector",
        "(J)V", reinterpret_cast<jlong>(inputVector[i]));
    this->JVM->SetObjectArrayElement(javaInputVector, i, javaCurInputVec);
    }
  jobject javaOutputVector = this->JVM->NewObject("vtk/vtkInformationVector",
      "(J)V", reinterpret_cast<jlong>(outputVector));
  jboolean b = this->JVM->CallMethod(this->Internals->JavaAlgorithm, "requestDataObject",
      "(Lvtk/vtkInformation;[Lvtk/vtkInformationVector;Lvtk/vtkInformationVector;)Z",
      javaRequest, javaInputVector, javaOutputVector);
  return b ? 1 : 0;
}

//---------------------------------------------------------------------------
int vtkJavaProgrammableFilter::RequestInformation(
  vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  if (!this->Internals->JavaAlgorithm)
    {
    return 1;
    }
  jobject javaRequest = this->JVM->NewObject("vtk/vtkInformation",
      "(J)V", reinterpret_cast<jlong>(request));
  int numInputs = this->GetNumberOfInputPorts();
  jobjectArray javaInputVector = this->JVM->NewObjectArray("vtk/vtkInformationVector", numInputs);
  for (int i = 0; i < numInputs; ++i)
    {
    jobject javaCurInputVec = this->JVM->NewObject("vtk/vtkInformationVector",
        "(J)V", reinterpret_cast<jlong>(inputVector[i]));
    this->JVM->SetObjectArrayElement(javaInputVector, i, javaCurInputVec);
    }
  jobject javaOutputVector = this->JVM->NewObject("vtk/vtkInformationVector",
      "(J)V", reinterpret_cast<jlong>(outputVector));
  jboolean b = this->JVM->CallMethod(this->Internals->JavaAlgorithm, "requestInformation",
      "(Lvtk/vtkInformation;[Lvtk/vtkInformationVector;Lvtk/vtkInformationVector;)Z",
      javaRequest, javaInputVector, javaOutputVector);
  return b ? 1 : 0;
}

//---------------------------------------------------------------------------
int vtkJavaProgrammableFilter::RequestUpdateExtent(
  vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  if (!this->Internals->JavaAlgorithm)
    {
    return 1;
    }
  jobject javaRequest = this->JVM->NewObject("vtk/vtkInformation",
      "(J)V", reinterpret_cast<jlong>(request));
  int numInputs = this->GetNumberOfInputPorts();
  jobjectArray javaInputVector = this->JVM->NewObjectArray("vtk/vtkInformationVector", numInputs);
  for (int i = 0; i < numInputs; ++i)
    {
    jobject javaCurInputVec = this->JVM->NewObject("vtk/vtkInformationVector",
        "(J)V", reinterpret_cast<jlong>(inputVector[i]));
    this->JVM->SetObjectArrayElement(javaInputVector, i, javaCurInputVec);
    }
  jobject javaOutputVector = this->JVM->NewObject("vtk/vtkInformationVector",
      "(J)V", reinterpret_cast<jlong>(outputVector));
  jboolean b = this->JVM->CallMethod(this->Internals->JavaAlgorithm, "requestUpdateExtent",
      "(Lvtk/vtkInformation;[Lvtk/vtkInformationVector;Lvtk/vtkInformationVector;)Z",
      javaRequest, javaInputVector, javaOutputVector);
  return b ? 1 : 0;
}

//---------------------------------------------------------------------------
void vtkJavaProgrammableFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "JavaClassName: " << (this->JavaClassNameInternal ? this->JavaClassNameInternal : "(none)") << endl;
  os << indent << "JavaClassPath: " << (this->JavaClassPathInternal ? this->JavaClassPathInternal : "(none)") << endl;
}
