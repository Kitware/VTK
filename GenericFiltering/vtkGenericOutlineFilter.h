/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericOutlineFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkGenericOutlineFilter - create wireframe outline for arbitrary
// generic data set
// .SECTION Description
// vtkGenericOutlineFilter is a filter that generates a wireframe outline of
// any generic data set. The outline consists of the twelve edges of the
// generic dataset bounding box.

#ifndef __vtkGenericOutlineFilter_h
#define __vtkGenericOutlineFilter_h

#include "vtkGenericDataSetToPolyDataFilter.h"

class vtkOutlineSource;

class VTK_GENERIC_FILTERING_EXPORT vtkGenericOutlineFilter : public vtkGenericDataSetToPolyDataFilter
{
public:
  static vtkGenericOutlineFilter *New();
  vtkTypeRevisionMacro(vtkGenericOutlineFilter,vtkGenericDataSetToPolyDataFilter);

protected:
  vtkGenericOutlineFilter();
  ~vtkGenericOutlineFilter();

  vtkOutlineSource *OutlineSource;
  void Execute();
  void ExecuteInformation();
private:
  vtkGenericOutlineFilter(const vtkGenericOutlineFilter&);  // Not implemented.
  void operator=(const vtkGenericOutlineFilter&);  // Not implemented.
};

#endif
