/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOutlineFilter.h
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
// .NAME vtkOutlineFilter - create wireframe outline for arbitrary data set
// .SECTION Description
// vtkOutlineFilter is a filter that generates a wireframe outline of any 
// data set. The outline consists of the twelve edges of the dataset 
// bounding box.

#ifndef __vtkOutlineFilter_h
#define __vtkOutlineFilter_h

#include "vtkDataSetToPolyDataFilter.h"
class vtkOutlineSource;

class VTK_GRAPHICS_EXPORT vtkOutlineFilter : public vtkDataSetToPolyDataFilter
{
public:
  static vtkOutlineFilter *New();
  vtkTypeRevisionMacro(vtkOutlineFilter,vtkDataSetToPolyDataFilter);

protected:
  vtkOutlineFilter();
  ~vtkOutlineFilter();

  vtkOutlineSource *OutlineSource;
  void Execute();
  void ExecuteInformation();
private:
  vtkOutlineFilter(const vtkOutlineFilter&);  // Not implemented.
  void operator=(const vtkOutlineFilter&);  // Not implemented.
};

#endif


