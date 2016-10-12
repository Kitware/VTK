/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContextInteractorStyle.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

#include "vtkViewsContext2DModule.h" // For export macro
#include "vtkInteractorStyle.h"
#include "vtkNew.h" // For ivars
#include "vtkWeakPointer.h" // For ivars

class vtkContextMouseEvent;
class vtkContextScene;

class VTKVIEWSCONTEXT2D_EXPORT vtkContextInteractorStyle : public vtkInteractorStyle
{
public:
  static vtkContextInteractorStyle *New();
  vtkTypeMacro(vtkContextInteractorStyle, vtkInteractorStyle);
  void PrintSelf(ostream& os, vtkIndent indent);

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
  virtual void OnMouseMove();

  /**
   * Called when the user clicks the mouse left button.
   * Default behavior forwards the event to the observed scene.
   */
  virtual void OnLeftButtonDown();

  /**
   * Called when the user releases the mouse left button.
   * Default behavior forwards the event to the observed scene.
   */
  virtual void OnLeftButtonUp();

  /**
   * Called when the user clicks the mouse middle button.
   * Default behavior forwards the event to the observed scene.
   */
  virtual void OnMiddleButtonDown();

  /**
   * Called when the user releases the mouse middle button.
   * Default behavior forwards the event to the observed scene.
   */
  virtual void OnMiddleButtonUp();

  /**
   * Called when the user clicks the mouse right button.
   * Default behavior forwards the event to the observed scene.
   */
  virtual void OnRightButtonDown();

  /**
   * Called when the user releases the mouse right button.
   * Default behavior forwards the event to the observed scene.
   */
  virtual void OnRightButtonUp();

  /**
   * Called when the user moves the mouse wheel forward.
   * Default behavior forwards the event to the observed scene.
   */
  virtual void OnMouseWheelForward();

  /**
   * Called when the user moves the mouse wheel backward.
   * Default behavior forwards the event to the observed scene.
   */
  virtual void OnMouseWheelBackward();

  /**
   * Place holder for future implementation.
   * Default behavior forwards the event to the observed scene.
   */
  virtual void OnSelection(unsigned int rect[5]);

  /**
   * Handle key presses.
   */
  virtual void OnChar();

  /**
   * Called when the user presses a key.
   */
  virtual void OnKeyPress();

  /**
   * Called when the user releases a key.
   */
  virtual void OnKeyRelease();

protected:
  vtkContextInteractorStyle();
  ~vtkContextInteractorStyle();

  static void ProcessSceneEvents(vtkObject* object, unsigned long event,
                                 void* clientdata, void* calldata);

  static void ProcessInteractorEvents(vtkObject* object, unsigned long event,
                                      void* clientdata, void* calldata);

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
  int                 ProcessingEvents;
  vtkMTimeType        LastSceneRepaintMTime;

  int                 TimerId;
  bool                TimerCallbackInitialized;

private:
  vtkContextInteractorStyle(const vtkContextInteractorStyle&) VTK_DELETE_FUNCTION;
  void operator=(const vtkContextInteractorStyle&) VTK_DELETE_FUNCTION;

  void ConstructMouseEvent(vtkContextMouseEvent &event, int button);
  bool ProcessMousePress(const vtkContextMouseEvent &event);
};

#endif
