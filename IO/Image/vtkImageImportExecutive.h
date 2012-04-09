/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageImportExecutive.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageImportExecutive -
// .SECTION Description
// vtkImageImportExecutive

#ifndef __vtkImageImportExecutive_h
#define __vtkImageImportExecutive_h

#include "vtkStreamingDemandDrivenPipeline.h"

class VTK_IMAGING_EXPORT vtkImageImportExecutive : 
  public vtkStreamingDemandDrivenPipeline
{
public:
  static vtkImageImportExecutive* New();
  vtkTypeMacro(vtkImageImportExecutive,
                       vtkStreamingDemandDrivenPipeline);

  // Description:
  // Override to implement some requests with callbacks.
  virtual int ProcessRequest(vtkInformation* request,
                             vtkInformationVector** inInfo,
                             vtkInformationVector* outInfo);

protected:
  vtkImageImportExecutive() {};
  ~vtkImageImportExecutive() {};
  
private:
  vtkImageImportExecutive(const vtkImageImportExecutive&);  // Not implemented.
  void operator=(const vtkImageImportExecutive&);  // Not implemented.
};

#endif
