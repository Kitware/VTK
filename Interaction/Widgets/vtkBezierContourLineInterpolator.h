/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBezierContourLineInterpolator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkBezierContourLineInterpolator
 * @brief   Interpolates supplied nodes with bezier line segments
 *
 * The line interpolator interpolates supplied nodes (see InterpolateLine)
 * with Bezier line segments. The fitness of the curve may be controlled using
 * SetMaximumCurveError and SetMaximumNumberOfLineSegments.
 *
 * @sa
 * vtkContourLineInterpolator
*/

#ifndef vtkBezierContourLineInterpolator_h
#define vtkBezierContourLineInterpolator_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkContourLineInterpolator.h"

class VTKINTERACTIONWIDGETS_EXPORT vtkBezierContourLineInterpolator
                          : public vtkContourLineInterpolator
{
public:

  /**
   * Instantiate this class.
   */
  static vtkBezierContourLineInterpolator *New();

  //@{
  /**
   * Standard methods for instances of this class.
   */
  vtkTypeMacro(vtkBezierContourLineInterpolator, vtkContourLineInterpolator);
  void PrintSelf(ostream& os, vtkIndent indent);
  //@}

  virtual int InterpolateLine( vtkRenderer *ren,
                               vtkContourRepresentation *rep,
                               int idx1, int idx2 );

  //@{
  /**
   * The difference between a line segment connecting two points and the curve
   * connecting the same points. In the limit of the length of the curve
   * dx -> 0, the two values will be the same. The smaller this number, the
   * finer the bezier curve will be interpolated. Default is 0.005
   */
  vtkSetClampMacro(MaximumCurveError, double, 0.0, VTK_DOUBLE_MAX);
  vtkGetMacro(MaximumCurveError, double);
  //@}

  //@{
  /**
   * Maximum number of bezier line segments between two nodes. Larger values
   * create a finer interpolation. Default is 100.
   */
  vtkSetClampMacro(MaximumCurveLineSegments, int, 1, 1000);
  vtkGetMacro(MaximumCurveLineSegments, int);
  //@}

  /**
   * Span of the interpolator, i.e. the number of control points it's supposed
   * to interpolate given a node.

   * The first argument is the current nodeIndex.
   * i.e., you'd be trying to interpolate between nodes "nodeIndex" and
   * "nodeIndex-1", unless you're closing the contour, in which case you're
   * trying to interpolate "nodeIndex" and "Node=0". The node span is
   * returned in a vtkIntArray.

   * The node span is returned in a vtkIntArray. The node span returned by
   * this interpolator will be a 2-tuple with a span of 4.
   */
  virtual void GetSpan(int nodeIndex, vtkIntArray *nodeIndices,
                        vtkContourRepresentation *rep);

protected:
  vtkBezierContourLineInterpolator();
  ~vtkBezierContourLineInterpolator();

  void ComputeMidpoint(double p1[3], double p2[3], double mid[3])
  {
      mid[0] = (p1[0] + p2[0])/2;
      mid[1] = (p1[1] + p2[1])/2;
      mid[2] = (p1[2] + p2[2])/2;
  }

  double MaximumCurveError;
  int    MaximumCurveLineSegments;

private:
  vtkBezierContourLineInterpolator(const vtkBezierContourLineInterpolator&) VTK_DELETE_FUNCTION;
  void operator=(const vtkBezierContourLineInterpolator&) VTK_DELETE_FUNCTION;
};

#endif
