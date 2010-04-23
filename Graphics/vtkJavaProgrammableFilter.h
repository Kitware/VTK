/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkJavaProgrammableFilter.h

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
//
// .NAME vtkJavaProgrammableFilter - Execute an algorithm defined in a Java class.
//
// .SECTION Description
// vtkJavaProgrammableFilter executes code from a Java class. The Java class must
// be a subclass of vtk.Algorithm, which is defined in the VTK jar file built
// when VTK_WRAP_JAVA is on. The subclass of vtk.Algorithm works just as if it
// was a vtkAlgorithm subclass, with the following exceptions:
// * Inside the Java class, you have access to the "real" algorithm which is
//   an attribute called VTKAlgorithm. Use this to set parameters, get filter
//   input and output, etc.
// * You initialize parameters and set the number of input/output ports in the
//   method named "initialize" instead of the constructor.
// * Parameters are set using SetParameter(name, value) and are retrieved in
//   Java by GetXParameter(name), where X is Int, Double, or String.
//
// The default algorithm will take one input and produce an output. You must
// minimally override
//
// bool requestData(vtkInformation, vtkInformationVector[], vtkInformationVector)
//
// which performs the same function as vtkAlgorithm::RequestData().
// See VTK/Wrapping/Java/vtk/SampleAlgorithm.java as an example Algorithm subclass.
//
// To use this filter, the application must first call the static methods
// vtkJVMManager::AddLibraryPath() and vtkJVMManager::AddClassPath() with
// the paths to the VTK libraries and vtk.jar, respectively.
// Then, on this filter you must call SetJavaClassPath() with the location
// of the .jar file or .class files which contain your vtk.Algorithm
// subclass. Also use SetJavaClassName() to set the name of the vtk.Algorithm
// subclass. See VTK/Graphics/Testing/Cxx/TestJavaProgrammableFilter.cxx for
// an example of how to set up the programmable filter.

#ifndef __vtkJavaProgrammableFilter_h
#define __vtkJavaProgrammableFilter_h

#include "vtkAlgorithm.h"

class vtkJavaProgrammableFilterInternals;
class vtkJVMManager;

class VTK_GRAPHICS_JAVA_EXPORT vtkJavaProgrammableFilter : public vtkAlgorithm
{
public:
  static vtkJavaProgrammableFilter* New();
  vtkTypeMacro(vtkJavaProgrammableFilter, vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Sets and retrieves parameters for the algorithm as name/value pairs.
  // This API allows the Java code to access arbitrary parameters.
  void SetParameter(const char* name, int value);
  void SetParameter(const char* name, double value);
  void SetParameter(const char* name, const char* value);
  int GetIntParameter(const char* name);
  double GetDoubleParameter(const char* name);
  const char* GetStringParameter(const char* name);
  
  // Description:
  // The name of the vtk.Algorithm subclass. The class name should
  // be fully qualified, and should contain a "/" for package separators.
  // (e.g. com/kitware/mypackage/MyAlgorithm).
  virtual void SetJavaClassName(const char* name);
  virtual const char* GetJavaClassName()
    { return this->JavaClassNameInternal; }
  
  // Description:
  // The class path (.jar file or path to .class files) containing
  // the vtk.Algorithm subclass. This string may contain multiple
  // paths separated by ":".
  virtual void SetJavaClassPath(const char* path);
  virtual const char* GetJavaClassPath()
    { return this->JavaClassPathInternal; }
  
  // Description:
  // Make an instance of the Java vtk.Algorithm subclass.
  // This should not be called until the class name and class path
  // are set. It is automatically called once the two parameters
  // are initially set.
  bool Initialize();
  
  // Description:
  // We need to make these methods public so the Java algorithm can access them
  // to set the number of inputs and outputs for the algorithm.
  void SetNumberOfInputPorts(int n)
  { this->Superclass::SetNumberOfInputPorts(n); }
  void SetNumberOfOutputPorts(int n)
  { this->Superclass::SetNumberOfOutputPorts(n); }
  
  // Description:
  // Delegates requests to several protected functions.
  // Each function in turn calls an equivalent function in the
  // Java class.
  virtual int ProcessRequest(vtkInformation* request, 
                             vtkInformationVector** inputVector,
                             vtkInformationVector* outputVector);
  
protected:
  vtkJavaProgrammableFilter();
  ~vtkJavaProgrammableFilter();

  // Description:
  // Calls requestData() on the Java object.
  virtual int RequestData(
    vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*);
  
  // Description:
  // Calls requestDataObject() on the Java object.
  virtual int RequestDataObject(
    vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*);
  
  // Description:
  // Calls requestInformation() on the Java object.
  virtual int RequestInformation(
    vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*);
  
  // Description:
  // Calls requestUpdateExtent() on the Java object.
  virtual int RequestUpdateExtent(
    vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*);
  
  // Description:
  // Calls fillInputPortInformation() on the Java object.
  virtual int FillInputPortInformation(int port, vtkInformation* info);

  // Description:
  // Calls fillOutputPortInformation() on the Java object.
  virtual int FillOutputPortInformation(int port, vtkInformation* info);
  
  vtkSetStringMacro(JavaClassNameInternal);
  char* JavaClassNameInternal;
  vtkSetStringMacro(JavaClassPathInternal);
  char* JavaClassPathInternal;
  vtkJVMManager* JVM;
  
  vtkJavaProgrammableFilterInternals* Internals;
  
private:
  vtkJavaProgrammableFilter(const vtkJavaProgrammableFilter&); // Not implemented
  void operator=(const vtkJavaProgrammableFilter&);   // Not implemented
};

#endif

