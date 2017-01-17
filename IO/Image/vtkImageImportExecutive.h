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
/**
 * @class   vtkImageImportExecutive
 *
 * vtkImageImportExecutive
*/

#ifndef vtkImageImportExecutive_h
#define vtkImageImportExecutive_h

#include "vtkIOImageModule.h" // For export macro
#include "vtkStreamingDemandDrivenPipeline.h"

class VTKIOIMAGE_EXPORT vtkImageImportExecutive :
  public vtkStreamingDemandDrivenPipeline
{
public:
  static vtkImageImportExecutive* New();
  vtkTypeMacro(vtkImageImportExecutive,
                       vtkStreamingDemandDrivenPipeline);

  /**
   * Override to implement some requests with callbacks.
   */
  int ProcessRequest(vtkInformation* request,
                             vtkInformationVector** inInfo,
                             vtkInformationVector* outInfo) VTK_OVERRIDE;

protected:
  vtkImageImportExecutive() {}
  ~vtkImageImportExecutive() VTK_OVERRIDE {}

private:
  vtkImageImportExecutive(const vtkImageImportExecutive&) VTK_DELETE_FUNCTION;
  void operator=(const vtkImageImportExecutive&) VTK_DELETE_FUNCTION;
};

#endif
