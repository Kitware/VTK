/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLinearContourLineInterpolator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkLinearContourLineInterpolator - Interpolates supplied nodes with line segments
// .SECTION Description
// The line interpolator interpolates supplied nodes (see InterpolateLine)
// with line segments. The finess of the curve may be controlled using
// SetMaximumCurveError and SetMaximumNumberOfLineSegments.
//
// .SECTION See Also
// vtkContourLineInterpolator

#ifndef __vtkLinearContourLineInterpolator_h
#define __vtkLinearContourLineInterpolator_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkContourLineInterpolator.h"

class VTKINTERACTIONWIDGETS_EXPORT vtkLinearContourLineInterpolator
                          : public vtkContourLineInterpolator
{
public:

  // Description:
  // Instantiate this class.
  static vtkLinearContourLineInterpolator *New();

  // Description:
  // Standard methods for instances of this class.
  vtkTypeMacro(vtkLinearContourLineInterpolator,vtkContourLineInterpolator);
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual int InterpolateLine( vtkRenderer *ren,
                               vtkContourRepresentation *rep,
                               int idx1, int idx2 );

protected:
  vtkLinearContourLineInterpolator();
  ~vtkLinearContourLineInterpolator();

private:
  vtkLinearContourLineInterpolator(const vtkLinearContourLineInterpolator&);  //Not implemented
  void operator=(const vtkLinearContourLineInterpolator&);  //Not implemented
};

#endif
