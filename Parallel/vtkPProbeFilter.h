/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPProbeFilter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPProbeFilter - probe dataset in parallel
// .SECTION Description

#ifndef __vtkPProbeFilter_h
#define __vtkPProbeFilter_h

#include "vtkProbeFilter.h"

class vtkMultiProcessController;

class VTK_PARALLEL_EXPORT vtkPProbeFilter : public vtkProbeFilter
{
public:
  vtkTypeRevisionMacro(vtkPProbeFilter,vtkProbeFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  static vtkPProbeFilter *New();

  // Description:
  // Set and get the controller.
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);

protected:
  vtkPProbeFilter();
  ~vtkPProbeFilter();

  // Usual data generation method
  virtual void ExecuteData(vtkDataObject*);
  virtual void ExecuteInformation();
  virtual void ComputeInputUpdateExtents(vtkDataObject *output);

  vtkMultiProcessController* Controller;

private:
  vtkPProbeFilter(const vtkPProbeFilter&);  // Not implemented.
  void operator=(const vtkPProbeFilter&);  // Not implemented.
};

#endif
