/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkReaderExecutive.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkReaderExecutive
 * @brief   Executive that works with vtkReaderAlgorithm and subclasses.
 *
 * @deprecated VTK 9.1.0. This is no longer needed. vtkReaderAlgorithm can now
 * work with standard executive and hence this can be removed. Follows docs are
 * no longer relevant and left for historical reasons.
 *
 * vtkReaderExecutive is an executive that supports simplified API readers
 * that are written by subclassing from the vtkReaderAlgorithm hierarchy.
 * Currently, its main functionality is to call the basic reader API instead
 * if the standard ProcessRequest() method that other algorithms use.
 * In time, this is likely to add functionality such as caching. See
 * vtkReaderAlgorithm for the API.
 *
 * Note that this executive assumes that the reader has one output port.
 */

#ifndef vtkReaderExecutive_h
#define vtkReaderExecutive_h

#include "vtkCommonExecutionModelModule.h" // For export macro
#include "vtkDeprecation.h"                // for deprecation macros
#include "vtkStreamingDemandDrivenPipeline.h"

VTK_DEPRECATED_IN_9_1_0("no longer needed")
class VTKCOMMONEXECUTIONMODEL_EXPORT vtkReaderExecutive : public vtkStreamingDemandDrivenPipeline
{
public:
  static vtkReaderExecutive* New();
  vtkTypeMacro(vtkReaderExecutive, vtkStreamingDemandDrivenPipeline);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkReaderExecutive();
  ~vtkReaderExecutive() override;

private:
  vtkReaderExecutive(const vtkReaderExecutive&) = delete;
  void operator=(const vtkReaderExecutive&) = delete;
};

#endif
