// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkContextInteractorStyle
 * @brief   An interactor for chart views.
 *
 *
 * It observes the user events (mouse events) and propagates them
 * to the scene. If the scene doesn't eat the event, it is propagated
 * to the interactor style superclass.
 */

#ifndef vtkContextInteractorStyle_h
#define vtkContextInteractorStyle_h

#include "vtkInteractorStyle.h"
#include "vtkNew.h"                  // For ivars
#include "vtkViewsContext2DModule.h" // For export macro
#include "vtkWeakPointer.h"          // For ivars
#include "vtkWrappingHints.h"        // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkContextMouseEvent;
class vtkContextScene;

class VTKVIEWSCONTEXT2D_EXPORT VTK_MARSHALAUTO vtkContextInteractorStyle : public vtkInteractorStyle
{
public:
  static vtkContextInteractorStyle* New();
  vtkTypeMacro(vtkContextInteractorStyle, vtkInteractorStyle);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Set the scene to forward user events to.
   * Refresh the view when the scene is dirty and no event is being processed.
   * The scene is observed (vtkCommand::ModifiedEvent) and a refresh on the
   * view is called appropriately: scene is dirty and no event is being
   * processed.
   */
  void SetScene(vtkContextScene* scene);

  /**
   * Return the observed scene.
   */
  vtkContextScene* GetScene();

  /**
   * Called when the scene is modified. Refresh the scene if needed.
   */
  virtual void OnSceneModified();

  /**
   * Called when the user moves the mouse
   * Default behavior forwards the event to the observed scene.
   */
  void OnMouseMove() override;

  /**
   * Called when the user clicks the mouse left button.
   * Default behavior forwards the event to the observed scene.
   */
  void OnLeftButtonDown() override;

  /**
   * Called when the user releases the mouse left button.
   * Default behavior forwards the event to the observed scene.
   */
  void OnLeftButtonUp() override;

  /**
   * Called when the user double-clicks the mouse left button.
   * Default behavior forwards the event to the observed scene.
   */
  void OnLeftButtonDoubleClick() override;

  /**
   * Called when the user clicks the mouse middle button.
   * Default behavior forwards the event to the observed scene.
   */
  void OnMiddleButtonDown() override;

  /**
   * Called when the user releases the mouse middle button.
   * Default behavior forwards the event to the observed scene.
   */
  void OnMiddleButtonUp() override;

  /**
   * Called when the user double-clicks the mouse middle button.
   * Default behavior forwards the event to the observed scene.
   */
  void OnMiddleButtonDoubleClick() override;

  /**
   * Called when the user clicks the mouse right button.
   * Default behavior forwards the event to the observed scene.
   */
  void OnRightButtonDown() override;

  /**
   * Called when the user releases the mouse right button.
   * Default behavior forwards the event to the observed scene.
   */
  void OnRightButtonUp() override;

  /**
   * Called when the user double-clicks the mouse right button.
   * Default behavior forwards the event to the observed scene.
   */
  void OnRightButtonDoubleClick() override;

  /**
   * Called when the user moves the mouse wheel forward.
   * Default behavior forwards the event to the observed scene.
   */
  void OnMouseWheelForward() override;

  /**
   * Called when the user moves the mouse wheel backward.
   * Default behavior forwards the event to the observed scene.
   */
  void OnMouseWheelBackward() override;

  /**
   * Place holder for future implementation.
   * Default behavior forwards the event to the observed scene.
   */
  virtual void OnSelection(unsigned int rect[5]);

  /**
   * Handle key presses.
   */
  void OnChar() override;

  /**
   * Called when the user presses a key.
   */
  void OnKeyPress() override;

  /**
   * Called when the user releases a key.
   */
  void OnKeyRelease() override;

protected:
  vtkContextInteractorStyle();
  ~vtkContextInteractorStyle() override;

  static void ProcessSceneEvents(
    vtkObject* object, unsigned long event, void* clientdata, void* calldata);

  static void ProcessInteractorEvents(
    vtkObject* object, unsigned long event, void* clientdata, void* calldata);

  virtual void RenderNow();

  /**
   * Inform the interactor style that an event is being processed.
   * That way is knows to not refresh the view (the view will eventually be
   * refreshed at the end.
   */
  void BeginProcessingEvent();

  /**
   * Inform the interactor style that an event is finished to be processed.
   * If no other event is being processed it check if the scene needs to be
   * rendered (scene is dirty)
   */
  void EndProcessingEvent();

  vtkWeakPointer<vtkContextScene> Scene;
  vtkNew<vtkCallbackCommand> SceneCallbackCommand;
  vtkNew<vtkCallbackCommand> InteractorCallbackCommand;
  int ProcessingEvents;
  vtkMTimeType LastSceneRepaintMTime;

  int SceneTimerId;
  bool TimerCallbackInitialized;

private:
  vtkContextInteractorStyle(const vtkContextInteractorStyle&) = delete;
  void operator=(const vtkContextInteractorStyle&) = delete;

  void ConstructMouseEvent(vtkContextMouseEvent& event, int button);
  bool ProcessMousePress(const vtkContextMouseEvent& event);
};

VTK_ABI_NAMESPACE_END
#endif
