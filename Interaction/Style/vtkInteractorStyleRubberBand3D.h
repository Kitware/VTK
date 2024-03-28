// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkInteractorStyleRubberBand3D
 * @brief   A rubber band interactor for a 3D view
 *
 *
 * vtkInteractorStyleRubberBand3D manages interaction in a 3D view.
 * The style also allows draws a rubber band using the left button.
 * All camera changes invoke StartInteractionEvent when the button
 * is pressed, InteractionEvent when the mouse (or wheel) is moved,
 * and EndInteractionEvent when the button is released.  The bindings
 * are as follows:
 * Left mouse - Select (invokes a SelectionChangedEvent).
 * Right mouse - Rotate.
 * Shift + right mouse - Zoom.
 * Middle mouse - Pan.
 * Scroll wheel - Zoom.
 */

#ifndef vtkInteractorStyleRubberBand3D_h
#define vtkInteractorStyleRubberBand3D_h

#include "vtkInteractionStyleModule.h" // For export macro
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkUnsignedCharArray;

class VTKINTERACTIONSTYLE_EXPORT VTK_MARSHALAUTO vtkInteractorStyleRubberBand3D
  : public vtkInteractorStyleTrackballCamera
{
public:
  static vtkInteractorStyleRubberBand3D* New();
  vtkTypeMacro(vtkInteractorStyleRubberBand3D, vtkInteractorStyleTrackballCamera);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void OnLeftButtonDown() override;
  void OnLeftButtonUp() override;
  void OnMiddleButtonDown() override;
  void OnMiddleButtonUp() override;
  void OnRightButtonDown() override;
  void OnRightButtonUp() override;
  void OnMouseMove() override;
  void OnMouseWheelForward() override;
  void OnMouseWheelBackward() override;

  ///@{
  /**
   * Whether to invoke a render when the mouse moves.
   */
  vtkSetMacro(RenderOnMouseMove, bool);
  vtkGetMacro(RenderOnMouseMove, bool);
  vtkBooleanMacro(RenderOnMouseMove, bool);
  ///@}

  /**
   * Selection types
   */
  enum
  {
    SELECT_NORMAL = 0,
    SELECT_UNION = 1
  };

  ///@{
  /**
   * Current interaction state
   */
  vtkGetMacro(Interaction, int);
  ///@}

  enum
  {
    NONE,
    PANNING,
    ZOOMING,
    ROTATING,
    SELECTING
  };

  ///@{
  /**
   * Access to the start and end positions (display coordinates) of the rubber
   * band pick area. This is a convenience method for the wrapped languages
   * since the event callData is lost when using those wrappings.
   */
  vtkGetVector2Macro(StartPosition, int);
  vtkGetVector2Macro(EndPosition, int);
  ///@}

protected:
  vtkInteractorStyleRubberBand3D();
  ~vtkInteractorStyleRubberBand3D() override;

  // The interaction mode
  int Interaction;

  // Draws the selection rubber band
  void RedrawRubberBand();

  // The end position of the selection
  int StartPosition[2];

  // The start position of the selection
  int EndPosition[2];

  // The pixel array for the rubber band
  vtkUnsignedCharArray* PixelArray;

  // Whether to trigger a render when the mouse moves
  bool RenderOnMouseMove;

private:
  vtkInteractorStyleRubberBand3D(const vtkInteractorStyleRubberBand3D&) = delete;
  void operator=(const vtkInteractorStyleRubberBand3D&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
