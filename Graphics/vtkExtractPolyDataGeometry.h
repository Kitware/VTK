/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractPolyDataGeometry.h
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
// .NAME vtkExtractPolyDataGeometry - extract vtkPolyData cells that lies either entirely inside or outside of a specified implicit function

// .SECTION Description
// vtkExtractPolyDataGeometry extracts from its input vtkPolyData all cells
// that are either completely inside or outside of a specified implicit
// function. This filter is specialized to vtkPolyData. On output the 
// filter generates vtkPolyData.
//
// To use this filter you must specify an implicit function. You must also
// specify whethter to extract cells lying inside or outside of the implicit 
// function. (The inside of an implicit function is the negative values 
// region.) An option exists to extract cells that are neither inside or
// outside (i.e., boundary).
//
// A more general version of this filter is available for arbitrary
// vtkDataSet input (see vtkExtractGeometry).

// .SECTION See Also
// vtkExtractGeometry vtkClipPolyData

#ifndef __vtkExtractPolyDataGeometry_h
#define __vtkExtractPolyDataGeometry_h

#include "vtkPolyDataToPolyDataFilter.h"
#include "vtkImplicitFunction.h"

class VTK_GRAPHICS_EXPORT vtkExtractPolyDataGeometry : public vtkPolyDataToPolyDataFilter
{
public:
  vtkTypeRevisionMacro(vtkExtractPolyDataGeometry,vtkPolyDataToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct object with ExtractInside turned on.
  static vtkExtractPolyDataGeometry *New();

  // Description:
  // Return the MTime taking into account changes to the implicit function
  unsigned long GetMTime();

  // Description:
  // Specify the implicit function for inside/outside checks.
  vtkSetObjectMacro(ImplicitFunction,vtkImplicitFunction);
  vtkGetObjectMacro(ImplicitFunction,vtkImplicitFunction);

  // Description:
  // Boolean controls whether to extract cells that are inside of implicit 
  // function (ExtractInside == 1) or outside of implicit function 
  // (ExtractInside == 0).
  vtkSetMacro(ExtractInside,int);
  vtkGetMacro(ExtractInside,int);
  vtkBooleanMacro(ExtractInside,int);

  // Description:
  // Boolean controls whether to extract cells that are partially inside.
  // By default, ExtractBoundaryCells is off.
  vtkSetMacro(ExtractBoundaryCells,int);
  vtkGetMacro(ExtractBoundaryCells,int);
  vtkBooleanMacro(ExtractBoundaryCells,int);

protected:
  vtkExtractPolyDataGeometry(vtkImplicitFunction *f=NULL);
  ~vtkExtractPolyDataGeometry();

  // Usual data generation method
  void Execute();

  vtkImplicitFunction *ImplicitFunction;
  int ExtractInside;
  int ExtractBoundaryCells;
private:
  vtkExtractPolyDataGeometry(const vtkExtractPolyDataGeometry&);  // Not implemented.
  void operator=(const vtkExtractPolyDataGeometry&);  // Not implemented.
};

#endif


