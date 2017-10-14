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
/**
 * @class   vtkLinearContourLineInterpolator
 * @brief   Interpolates supplied nodes with line segments
 *
 * The line interpolator interpolates supplied nodes (see InterpolateLine)
 * with line segments. The finess of the curve may be controlled using
 * SetMaximumCurveError and SetMaximumNumberOfLineSegments.
 *
 * @sa
 * vtkContourLineInterpolator
*/

#ifndef vtkLinearContourLineInterpolator_h
#define vtkLinearContourLineInterpolator_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkContourLineInterpolator.h"

class VTKINTERACTIONWIDGETS_EXPORT vtkLinearContourLineInterpolator
                          : public vtkContourLineInterpolator
{
public:

  /**
   * Instantiate this class.
   */
  static vtkLinearContourLineInterpolator *New();

  //@{
  /**
   * Standard methods for instances of this class.
   */
  vtkTypeMacro(vtkLinearContourLineInterpolator,vtkContourLineInterpolator);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  //@}

  int InterpolateLine( vtkRenderer *ren,
                               vtkContourRepresentation *rep,
                               int idx1, int idx2 ) override;

protected:
  vtkLinearContourLineInterpolator();
  ~vtkLinearContourLineInterpolator() override;

private:
  vtkLinearContourLineInterpolator(const vtkLinearContourLineInterpolator&) = delete;
  void operator=(const vtkLinearContourLineInterpolator&) = delete;
};

#endif
