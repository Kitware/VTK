/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractiveArea.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkInteractiveArea
 * @brief   Implements zooming and panning in a vtkContextArea.
 *
 * Implements zooming and panning in a vtkContextArea.
 */

#ifndef vtkInteractiveArea_h
#define vtkInteractiveArea_h

#include "vtkChartsCoreModule.h" // For export macro
#include "vtkContextArea.h"
#include "vtkNew.h"              // For vtkNew


class vtkContextTransform;
class vtkRectd;

class VTKCHARTSCORE_EXPORT vtkInteractiveArea : public vtkContextArea
{
public:
  vtkTypeMacro(vtkInteractiveArea, vtkContextArea)

  static vtkInteractiveArea* New();
  void PrintSelf(ostream &os, vtkIndent indent) VTK_OVERRIDE;

  ///@{
  /**
   * \brief vtkAbstractContextItem API
   */
  bool Paint(vtkContext2D *painter) VTK_OVERRIDE;
  bool Hit(const vtkContextMouseEvent& mouse) VTK_OVERRIDE;
  bool MouseWheelEvent(const vtkContextMouseEvent& mouse, int delta) VTK_OVERRIDE;
  bool MouseMoveEvent(const vtkContextMouseEvent &mouse) VTK_OVERRIDE;
  bool MouseButtonPressEvent(const vtkContextMouseEvent &mouse) VTK_OVERRIDE;
  ///@}

protected:
  vtkInteractiveArea();
  ~vtkInteractiveArea();

  ///@{
  /**
   * \brief vtkContextArea API
   */
  void SetAxisRange(vtkRectd const& data) VTK_OVERRIDE;

private:
  /**
   * Re-scale axis when interacting.
   */
  void RecalculateTickSpacing(vtkAxis* axis, int const numClicks);

  /**
   * Re-computes the transformation expressing the current zoom, panning, etc.
   */
  void ComputeViewTransform() VTK_OVERRIDE;

  void ComputeZoom(vtkVector2d const& origin, vtkVector2d & scale,
    vtkVector2d& shift, vtkVector2d& factor);

  class MouseActions;
  MouseActions* Actions;

  vtkInteractiveArea(const vtkInteractiveArea &) VTK_DELETE_FUNCTION;
  void operator=(const vtkInteractiveArea &) VTK_DELETE_FUNCTION;
};

#endif //vtkInteractiveArea_h
