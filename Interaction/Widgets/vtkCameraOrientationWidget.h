// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCameraOrientationWidget
 * @brief   A widget to manipulate vtkCameraOrientationWidget.
 *
 *
 * This 3D widget creates and manages its own vtkCameraOrientationRepresentation.
 * To use this widget, make sure you call SetParentRenderer() and enable the widget.
 * The jump-to-axis-viewpoint feature is animated over 20 frames. See SetAnimatorTotalFrames()
 * Turn off animation with AnimateOff()
 *
 * @par Event Bindings:
 * By default, the widget responds to the following VTK events (i.e., it
 * watches the vtkRenderWindowInteractor for these events):
 * <pre>
 * LeftButtonPressEvent - select the appropriate handle
 * LeftButtonReleaseEvent - release the currently selected handle
 * If one of the six handles are selected:
 *   MouseMoveEvent - rotate (if left button) , else set hover representation for nearest handle.
 * </pre>
 * These input events are not forwarded to any other observers. This widget
 * eats up mouse events. (AbortFlagOn).
 *
 * @par Event Bindings:
 * Note that the event bindings described above can be changed using this
 * class's vtkWidgetEventTranslator, which translates VTK events
 * into the vtkCameraOrientationWidget's widget events:
 * <pre>
 *   vtkWidgetEvent::Select -- some part of the widget has been selected
 *   vtkWidgetEvent::EndSelect -- the selection process has completed
 *   vtkWidgetEvent::Move -- a request for motion has been invoked
 * </pre>
 *
 * @par Event Bindings:
 * This class, and the affiliated vtkCameraOrientationRepresentation,
 * are second generation VTK widgets.
 *
 * @sa
 * vtkCameraOrientationRepresentation
 */

#ifndef vtkCameraOrientationWidget_h
#define vtkCameraOrientationWidget_h

#include "vtkAbstractWidget.h"
#include "vtkInteractionWidgetsModule.h" // needed for export macro
#include "vtkWeakPointer.h"              // for weak pointer ivar
#include "vtkWrappingHints.h"            // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkCameraInterpolator;
class vtkCameraOrientationRepresentation;
class vtkRenderer;

class VTKINTERACTIONWIDGETS_EXPORT VTK_MARSHALAUTO vtkCameraOrientationWidget
  : public vtkAbstractWidget
{
public:
  static vtkCameraOrientationWidget* New();
  vtkTypeMacro(vtkCameraOrientationWidget, vtkAbstractWidget);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Enable jump-to-axis-view animation.
   * See AnimatorTotalFrames.
   */
  vtkSetMacro(Animate, bool);
  vtkGetMacro(Animate, bool);
  vtkBooleanMacro(Animate, bool);
  ///@}

  ///@{
  /**
   * Length of animation. (in frames)
   */
  vtkSetClampMacro(AnimatorTotalFrames, int, 2, VTK_INT_MAX);
  vtkGetMacro(AnimatorTotalFrames, int);
  ///@}

  /**
   * Specify an instance of vtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of vtkProp
   * so it can be added to the renderer independently of the widget.
   */
  void SetRepresentation(vtkCameraOrientationRepresentation* r);

  /**
   * Create a vtkCameraOrientationRepresentation.
   */
  void CreateDefaultRepresentation() override;

  /**
   * Fits the widget's renderer to a square viewport.
   */
  void SquareResize();

  /**
   * Override super class method for default renderer.
   * This widget adds the representation props into the default
   * renderer.
   */
  void SetDefaultRenderer(vtkRenderer* renderer) override;

  ///@{
  /**
   * This widget shows and manipulates the orientation of
   * the parent renderer's active camera.
   *
   * Note: The renderer must be part of a render window for
   * the widget to appear.
   */
  void SetParentRenderer(vtkRenderer* renderer);
  vtkRenderer* GetParentRenderer();
  ///@}

protected:
  vtkCameraOrientationWidget();
  ~vtkCameraOrientationWidget() override;

  // These methods handle events
  void ComputeWidgetState(int X, int Y, int modify = 0);
  static void SelectAction(vtkAbstractWidget*);
  static void EndSelectAction(vtkAbstractWidget*);
  static void MoveAction(vtkAbstractWidget*);

  // These control the representation and parent renderer's camera.
  void OrientParentCamera(double back[3], double up[3]);
  void OrientWidgetRepresentation();
  void InterpolateCamera(int t);

  void StartAnimation();
  void PlayAnimationSingleFrame(vtkObject* caller, unsigned long event, void* callData);
  void StopAnimation();

  // Manage the state of the widget
  enum class WidgetStateType : int
  {
    Inactive, // mouse is not over the widget, none of the handles are selected.
    Hot,      // mouse is over the widget but none of the handles are selected
    Active    // any one handle is selected, representation could be rotating.
  };
  WidgetStateType WidgetState = WidgetStateType::Inactive;

  vtkWeakPointer<vtkRenderer> ParentRenderer;

  // Store camera interpolations.
  vtkNew<vtkCameraInterpolator> CameraInterpolator;

  bool Animate = true;
  int AnimatorTotalFrames = 20;
  int AnimatorCurrentFrame = 0;
  int AnimationTimerId = -1;

  int ResizeObserverTag = -1;
  int ReorientObserverTag = -1;
  int AnimationTimerObserverTag = -1;

private:
  vtkCameraOrientationWidget(const vtkCameraOrientationWidget&) = delete;
  void operator=(const vtkCameraOrientationWidget&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
