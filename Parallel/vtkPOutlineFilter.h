/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPOutlineFilter.h
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
// .NAME vtkPOutlineFilter - create wireframe outline for arbitrary data set
// .SECTION Description
// vtkPOutlineFilter works like vtkOutlineFilter, but it looks for data
// partitions in other processes.  It assmues the fitler is operation
// in a data parallel pipeline.

#ifndef __vtkPOutlineFilter_h
#define __vtkPOutlineFilter_h

#include "vtkDataSetToPolyDataFilter.h"
class vtkOutlineSource;
class vtkMultiProcessController;

class VTK_PARALLEL_EXPORT vtkPOutlineFilter : public vtkDataSetToPolyDataFilter
{
public:
  static vtkPOutlineFilter *New();
  vtkTypeRevisionMacro(vtkPOutlineFilter,vtkDataSetToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set and get the controller.
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);

protected:
  vtkPOutlineFilter();
  ~vtkPOutlineFilter();

  vtkMultiProcessController* Controller;
  vtkOutlineSource *OutlineSource;
  void Execute();
  void ExecuteInformation();
private:
  vtkPOutlineFilter(const vtkPOutlineFilter&);  // Not implemented.
  void operator=(const vtkPOutlineFilter&);  // Not implemented.
};

#endif


