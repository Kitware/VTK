/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkShrinkFilter.h
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
// .NAME vtkShrinkFilter - shrink cells composing an arbitrary data set
// .SECTION Description
// vtkShrinkFilter shrinks cells composing an arbitrary data set 
// towards their centroid. The centroid of a cell is computed as 
// the average position of the cell points. Shrinking results in 
// disconnecting the cells from one another. The output of this filter is
// of general dataset type vtkUnstructuredGrid.

// .SECTION Caveats
// It is possible to turn cells inside out or cause self intersection
// in special cases.

// .SECTION See Also
// vtkShrinkPolyData

#ifndef __vtkShrinkFilter_h
#define __vtkShrinkFilter_h

#include "vtkDataSetToUnstructuredGridFilter.h"

class VTK_GRAPHICS_EXPORT vtkShrinkFilter : public vtkDataSetToUnstructuredGridFilter
{
public:
  static vtkShrinkFilter *New();
  vtkTypeRevisionMacro(vtkShrinkFilter,vtkDataSetToUnstructuredGridFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the fraction of shrink for each cell.
  vtkSetClampMacro(ShrinkFactor,float,0.0,1.0);

  // Description:
  // Get the fraction of shrink for each cell.
  vtkGetMacro(ShrinkFactor,float);

protected:
  vtkShrinkFilter(float sf=0.5);
  ~vtkShrinkFilter() {};

  void Execute();
  float ShrinkFactor;
private:
  vtkShrinkFilter(const vtkShrinkFilter&);  // Not implemented.
  void operator=(const vtkShrinkFilter&);  // Not implemented.
};

#endif


