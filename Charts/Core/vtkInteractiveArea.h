// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

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
#include "vtkNew.h"           // For vtkNew
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkContextTransform;
class vtkRectd;

class VTKCHARTSCORE_EXPORT VTK_MARSHALAUTO vtkInteractiveArea : public vtkContextArea
{
public:
  vtkTypeMacro(vtkInteractiveArea, vtkContextArea);

  static vtkInteractiveArea* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * \brief vtkAbstractContextItem API
   */
  bool Paint(vtkContext2D* painter) override;
  bool Hit(const vtkContextMouseEvent& mouse) override;
  bool MouseWheelEvent(const vtkContextMouseEvent& mouse, int delta) override;
  bool MouseMoveEvent(const vtkContextMouseEvent& mouse) override;
  bool MouseButtonPressEvent(const vtkContextMouseEvent& mouse) override;
  ///@}

protected:
  vtkInteractiveArea();
  ~vtkInteractiveArea() override;

  ///@{
  /**
   * \brief vtkContextArea API
   */
  void SetAxisRange(vtkRectd const& data) override;

private:
  /**
   * Re-scale axis when interacting.
   */
  void RecalculateTickSpacing(vtkAxis* axis, int numClicks);

  /**
   * Re-computes the transformation expressing the current zoom, panning, etc.
   */
  void ComputeViewTransform() override;

  void ComputeZoom(
    vtkVector2d const& origin, vtkVector2d& scale, vtkVector2d& shift, vtkVector2d& factor);

  class MouseActions;
  MouseActions* Actions;

  vtkInteractiveArea(const vtkInteractiveArea&) = delete;
  void operator=(const vtkInteractiveArea&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkInteractiveArea_h
