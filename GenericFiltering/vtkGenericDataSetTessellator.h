/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericDataSetTessellator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkGenericDataSetTessellator - 
// .SECTION Description
// vtkGenericDataSetTessellator is a filter that subdivise a generic
// vtkGenericDataSet into linear elements: tetras for 3D mesh and triangles
// for 2D mesh. The subdivision process only depend on the error metric.

// .SECTION See Also


#ifndef __vtkGenericDataSetTessellator_h
#define __vtkGenericDataSetTessellator_h

#include "vtkGenericDataSetToUnstructuredGridFilter.h"

class vtkPointLocator;

class VTK_GENERIC_FILTERING_EXPORT vtkGenericDataSetTessellator : public vtkGenericDataSetToUnstructuredGridFilter
{
public:
  vtkTypeRevisionMacro(vtkGenericDataSetTessellator,
                       vtkGenericDataSetToUnstructuredGridFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  static vtkGenericDataSetTessellator *New();

protected:
  vtkGenericDataSetTessellator();
  ~vtkGenericDataSetTessellator();

  void Execute();

private:
  vtkGenericDataSetTessellator(const vtkGenericDataSetTessellator&);  // Not implemented.
  void operator=(const vtkGenericDataSetTessellator&);  // Not implemented.
};

#endif
