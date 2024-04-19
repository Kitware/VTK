// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPolyDataContourLineInterpolator
 * @brief   Contour interpolator for polygonal data
 *
 *
 * vtkPolyDataContourLineInterpolator is an abstract base class for contour
 * line interpolators that interpolate on polygonal data.
 *
 */

#ifndef vtkPolyDataContourLineInterpolator_h
#define vtkPolyDataContourLineInterpolator_h

#include "vtkContourLineInterpolator.h"
#include "vtkInteractionWidgetsModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkPolyData;
class vtkPolyDataCollection;

class VTKINTERACTIONWIDGETS_EXPORT vtkPolyDataContourLineInterpolator
  : public vtkContourLineInterpolator
{
public:
  ///@{
  /**
   * Standard methods for instances of this class.
   */
  vtkTypeMacro(vtkPolyDataContourLineInterpolator, vtkContourLineInterpolator);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * Subclasses that wish to interpolate a line segment must implement this.
   * For instance vtkBezierContourLineInterpolator adds nodes between idx1
   * and idx2, that allow the contour to adhere to a bezier curve.
   */
  int InterpolateLine(
    vtkRenderer* ren, vtkContourRepresentation* rep, int idx1, int idx2) override = 0;

  /**
   * The interpolator is given a chance to update the node.
   * vtkImageContourLineInterpolator updates the idx'th node in the contour,
   * so it automatically sticks to edges in the vicinity as the user
   * constructs the contour.
   * Returns 0 if the node (world position) is unchanged.
   */
  int UpdateNode(vtkRenderer*, vtkContourRepresentation*, double* vtkNotUsed(node),
    int vtkNotUsed(idx)) override = 0;

  ///@{
  /**
   * Be sure to add polydata on which you wish to place points to this list
   * or they will not be considered for placement.
   */
  vtkGetObjectMacro(Polys, vtkPolyDataCollection);
  ///@}

protected:
  vtkPolyDataContourLineInterpolator();
  ~vtkPolyDataContourLineInterpolator() override;

  vtkPolyDataCollection* Polys;

private:
  vtkPolyDataContourLineInterpolator(const vtkPolyDataContourLineInterpolator&) = delete;
  void operator=(const vtkPolyDataContourLineInterpolator&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
