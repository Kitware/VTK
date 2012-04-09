/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLinearSubdivisionFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkLinearSubdivisionFilter - generate a subdivision surface using the Linear Scheme
// .SECTION Description
// vtkLinearSubdivisionFilter is a filter that generates output by
// subdividing its input polydata. Each subdivision iteration create 4
// new triangles for each triangle in the polydata.

// .SECTION Thanks
// This work was supported by PHS Research Grant No. 1 P41 RR13218-01
// from the National Center for Research Resources.

// .SECTION See Also
// vtkInterpolatingSubdivisionFilter vtkButterflySubdivisionFilter

#ifndef __vtkLinearSubdivisionFilter_h
#define __vtkLinearSubdivisionFilter_h

#include "vtkInterpolatingSubdivisionFilter.h"

class vtkIntArray;
class vtkPointData;
class vtkPoints;
class vtkPolyData;

class VTK_GRAPHICS_EXPORT vtkLinearSubdivisionFilter : public vtkInterpolatingSubdivisionFilter
{
public:
  // Description:
  // Construct object with NumberOfSubdivisions set to 1.
  static vtkLinearSubdivisionFilter *New();
  vtkTypeMacro(vtkLinearSubdivisionFilter,vtkInterpolatingSubdivisionFilter);

protected:
  vtkLinearSubdivisionFilter () {};
  ~vtkLinearSubdivisionFilter () {};

  void GenerateSubdivisionPoints (vtkPolyData *inputDS, 
                                  vtkIntArray *edgeData, 
                                  vtkPoints *outputPts, 
                                  vtkPointData *outputPD);

private:
  vtkLinearSubdivisionFilter(const vtkLinearSubdivisionFilter&);  // Not implemented.
  void operator=(const vtkLinearSubdivisionFilter&);  // Not implemented.
};

#endif


