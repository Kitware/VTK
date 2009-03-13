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
// .NAME vtkJavaProgrammableFilter -
//
// .SECTION Description
//
// .SECTION Caveats
//
// .SECTION Thanks
//

#ifndef __vtkJavaProgrammableFilter_h
#define __vtkJavaProgrammableFilter_h

#include "vtkAlgorithm.h"

class vtkJavaProgrammableFilterInternals;
class vtkJVMManager;

class VTK_GRAPHICS_EXPORT vtkJavaProgrammableFilter : public vtkAlgorithm
{
public:
  static vtkJavaProgrammableFilter* New();
  vtkTypeRevisionMacro(vtkJavaProgrammableFilter, vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  void SetParameter(const char* name, int value);
  void SetParameter(const char* name, double value);
  void SetParameter(const char* name, const char* value);
  int GetIntParameter(const char* name);
  double GetDoubleParameter(const char* name);
  const char* GetStringParameter(const char* name);
  
  vtkSetStringMacro(JavaClassName);
  vtkGetStringMacro(JavaClassName);
  
  vtkSetStringMacro(VTKJarPath);
  vtkGetStringMacro(VTKJarPath);
  
  vtkSetStringMacro(JavaClassPath);
  vtkGetStringMacro(JavaClassPath);
  
  vtkSetStringMacro(VTKBinaryPath);
  vtkGetStringMacro(VTKBinaryPath);
  
  bool Initialize();
  
  // Make these methods public so the Java algorithm can access them.
  void SetNumberOfInputPorts(int n)
  { this->Superclass::SetNumberOfInputPorts(n); }
  void SetNumberOfOutputPorts(int n)
  { this->Superclass::SetNumberOfOutputPorts(n); }
  
  // Description:
  // see vtkAlgorithm for details
  virtual int ProcessRequest(vtkInformation* request, 
                             vtkInformationVector** inputVector,
                             vtkInformationVector* outputVector);
  
protected:
  vtkJavaProgrammableFilter();
  ~vtkJavaProgrammableFilter();

  virtual int RequestData(
    vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*);
  
  virtual int RequestDataObject(
    vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*);
  
  virtual int RequestInformation(
    vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*);
  
  virtual int RequestUpdateExtent(
    vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*);
  
  virtual int FillInputPortInformation(int port, vtkInformation* info);
  virtual int FillOutputPortInformation(int port, vtkInformation* info);
  
  char* JavaClassName;
  char* VTKJarPath;
  char* JavaClassPath;
  char* VTKBinaryPath;
  vtkJVMManager* JVM;
  
  vtkJavaProgrammableFilterInternals* Internals;
  
private:
  vtkJavaProgrammableFilter(const vtkJavaProgrammableFilter&); // Not implemented
  void operator=(const vtkJavaProgrammableFilter&);   // Not implemented
};

#endif

