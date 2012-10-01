/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQuantizePolyDataPoints.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkQuantizePolyDataPoints - quantizes x,y,z coordinates of points
// .SECTION Description
// vtkQuantizePolyDataPoints is a subclass of vtkCleanPolyData and
// inherits the functionality of vtkCleanPolyData with the addition that
// it quantizes the point coordinates before inserting into the point list.
// The user should set QFactor to a positive value (0.25 by default) and all
// {x,y,z} coordinates will be quantized to that grain size.
//
// A tolerance of zero is expected, though positive values may be used, the
// quantization will take place before the tolerance is applied.
//
// .SECTION Caveats
// Merging points can alter topology, including introducing non-manifold
// forms. Handling of degenerate cells is controlled by switches in
// vtkCleanPolyData.
//
// If you wish to operate on a set of coordinates that has no cells, you must
// add a vtkPolyVertex cell with all of the points to the PolyData
// (or use a vtkVertexGlyphFilter) before using the vtkCleanPolyData filter.
//
// .SECTION See Also
// vtkCleanPolyData

#ifndef __vtkQuantizePolyDataPoints_h
#define __vtkQuantizePolyDataPoints_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkCleanPolyData.h"

class VTKFILTERSGENERAL_EXPORT vtkQuantizePolyDataPoints : public vtkCleanPolyData
{
public:
  static vtkQuantizePolyDataPoints *New();
  vtkTypeMacro(vtkQuantizePolyDataPoints,vtkCleanPolyData);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify quantization grain size. Default is 0.25
  vtkSetClampMacro(QFactor,double,1E-5,VTK_LARGE_FLOAT);
  vtkGetMacro(QFactor,double);

  // Description:
  // Perform quantization on a point
  virtual void OperateOnPoint(double in[3], double out[3]);

  // Description:
  // Perform quantization on bounds
  virtual void OperateOnBounds(double in[6], double out[6]);

protected:
  vtkQuantizePolyDataPoints();
  ~vtkQuantizePolyDataPoints() {};

  double QFactor;
private:
  vtkQuantizePolyDataPoints(const vtkQuantizePolyDataPoints&);  // Not implemented.
  void operator=(const vtkQuantizePolyDataPoints&);  // Not implemented.
};

#endif


