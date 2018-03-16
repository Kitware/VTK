/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorObserver.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkInteractorObserver
 * @brief   an abstract superclass for classes observing events invoked by vtkRenderWindowInteractor
 *
 * vtkInteractorObserver is an abstract superclass for subclasses that observe
 * events invoked by vtkRenderWindowInteractor. These subclasses are
 * typically things like 3D widgets; objects that interact with actors
 * in the scene, or interactively probe the scene for information.
 *
 * vtkInteractorObserver defines the method SetInteractor() and enables and
 * disables the processing of events by the vtkInteractorObserver. Use the
 * methods EnabledOn() or SetEnabled(1) to turn on the interactor observer,
 * and the methods EnabledOff() or SetEnabled(0) to turn off the interactor.
 * Initial value is 0.
 *
 * To support interactive manipulation of objects, this class (and
 * subclasses) invoke the events StartInteractionEvent, InteractionEvent, and
 * EndInteractionEvent.  These events are invoked when the
 * vtkInteractorObserver enters a state where rapid response is desired:
 * mouse motion, etc. The events can be used, for example, to set the desired
 * update frame rate (StartInteractionEvent), operate on data or update a
 * pipeline (InteractionEvent), and set the desired frame rate back to normal
 * values (EndInteractionEvent). Two other events, EnableEvent and
 * DisableEvent, are invoked when the interactor observer is enabled or
 * disabled.
 *
 * @sa
 * vtk3DWidget vtkBoxWidget vtkLineWidget
*/

#ifndef vtkInteractorObserver_h
#define vtkInteractorObserver_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkObject.h"

class vtkAbstractPropPicker;
class vtkAssemblyPath;
class vtkRenderWindowInteractor;
class vtkRenderer;
class vtkCallbackCommand;
class vtkObserverMediator;
class vtkPickingManager;

class VTKRENDERINGCORE_EXPORT vtkInteractorObserver : public vtkObject
{
public:
  vtkTypeMacro(vtkInteractorObserver,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Methods for turning the interactor observer on and off, and determining
   * its state. All subclasses must provide the SetEnabled() method.
   * Enabling a vtkInteractorObserver has the side effect of adding
   * observers; disabling it removes the observers. Prior to enabling the
   * vtkInteractorObserver you must set the render window interactor (via
   * SetInteractor()). Initial value is 0.
   */
  virtual void SetEnabled(int) {}
  int GetEnabled() {return this->Enabled;}
  void EnabledOn() {this->SetEnabled(1);}
  void EnabledOff() {this->SetEnabled(0);}
  void On() {this->SetEnabled(1);}
  void Off() {this->SetEnabled(0);}

  //@{
  /**
   * This method is used to associate the widget with the render window
   * interactor.  Observers of the appropriate events invoked in the render
   * window interactor are set up as a result of this method invocation.
   * The SetInteractor() method must be invoked prior to enabling the
   * vtkInteractorObserver.
   * It automatically registers available pickers to the Picking Manager.
   */
  virtual void SetInteractor(vtkRenderWindowInteractor* iren);
  vtkGetObjectMacro(Interactor, vtkRenderWindowInteractor);
  //@}

  //@{
  /**
   * Set/Get the priority at which events are processed. This is used when
   * multiple interactor observers are used simultaneously. The default value
   * is 0.0 (lowest priority.) Note that when multiple interactor observer
   * have the same priority, then the last observer added will process the
   * event first. (Note: once the SetInteractor() method has been called,
   * changing the priority does not effect event processing. You will have
   * to SetInteractor(NULL), change priority, and then SetInteractor(iren)
   * to have the priority take effect.)
   */
  vtkSetClampMacro(Priority,float,0.0f,1.0f);
  vtkGetMacro(Priority,float);
  //@}

  //@{
  /**
   * Enable/Disable the use of a manager to process the picking.
   * Enabled by default.
   */
  vtkBooleanMacro(PickingManaged, bool);
  virtual void SetPickingManaged(bool managed);
  vtkGetMacro(PickingManaged, bool);
  //@}

  //@{
  /**
   * Enable/Disable of the use of a keypress to turn on and off the
   * interactor observer. (By default, the keypress is 'i' for "interactor
   * observer".)  Set the KeyPressActivationValue to change which key
   * activates the widget.)
   */
  vtkSetMacro(KeyPressActivation,vtkTypeBool);
  vtkGetMacro(KeyPressActivation,vtkTypeBool);
  vtkBooleanMacro(KeyPressActivation,vtkTypeBool);
  //@}

  //@{
  /**
   * Specify which key press value to use to activate the interactor observer
   * (if key press activation is enabled). By default, the key press
   * activation value is 'i'. Note: once the SetInteractor() method is
   * invoked, changing the key press activation value will not affect the key
   * press until SetInteractor(NULL)/SetInteractor(iren) is called.
   */
  vtkSetMacro(KeyPressActivationValue,char);
  vtkGetMacro(KeyPressActivationValue,char);
  //@}

  //@{
  /**
   * Set/Get the default renderer to use when activating the interactor
   * observer. Normally when the widget is activated (SetEnabled(1) or when
   * keypress activation takes place), the renderer over which the mouse
   * pointer is positioned is used. Alternatively, you can specify the
   * renderer to bind the interactor to when the interactor observer is
   * activated.
   */
  vtkGetObjectMacro(DefaultRenderer,vtkRenderer);
  virtual void SetDefaultRenderer(vtkRenderer*);
  //@}

  //@{
  /**
   * Set/Get the current renderer. Normally when the widget is activated
   * (SetEnabled(1) or when keypress activation takes place), the renderer
   * over which the mouse pointer is positioned is used and assigned to
   * this Ivar. Alternatively, you might want to set the CurrentRenderer
   * explicitly. This is especially true with multiple viewports (renderers).
   * WARNING: note that if the DefaultRenderer Ivar is set (see above),
   * it will always override the parameter passed to SetCurrentRenderer,
   * unless it is NULL.
   * (i.e., SetCurrentRenderer(foo) = SetCurrentRenderer(DefaultRenderer).
   */
  vtkGetObjectMacro(CurrentRenderer,vtkRenderer);
  virtual void SetCurrentRenderer(vtkRenderer*);
  //@}

  /**
   * Sets up the keypress-i event.
   */
  virtual void OnChar();

  //@{
  /**
   * Convenience methods for outside classes. Make sure that the
   * parameter "ren" is not-null.
   */
  static void ComputeDisplayToWorld(vtkRenderer *ren, double x, double y,
                                    double z, double worldPt[4]);
  static void ComputeWorldToDisplay(vtkRenderer *ren, double x, double y,
                                    double z, double displayPt[3]);
  //@}

  //@{
  /**
   * These methods enable an interactor observer to exclusively grab all
   * events invoked by its associated vtkRenderWindowInteractor. (This method
   * is typically used by widgets to grab events once an event sequence
   * begins.) The GrabFocus() signature takes up to two vtkCommands
   * corresponding to mouse events and keypress events. (These two commands
   * are separated so that the widget can listen for its activation keypress,
   * as well as listening for DeleteEvents, without actually having to process
   * mouse events.)
   */
  void GrabFocus(vtkCommand *mouseEvents, vtkCommand *keypressEvents=nullptr);
  void ReleaseFocus();
  //@}

protected:
  vtkInteractorObserver();
  ~vtkInteractorObserver() override;

  //@{
  /**
   * Utility routines used to start and end interaction.
   * For example, it switches the display update rate. It does not invoke
   * the corresponding events.
   */
  virtual void StartInteraction();
  virtual void EndInteraction();
  //@}

  /**
   * Handles the char widget activation event. Also handles the delete event.
   */
  static void ProcessEvents(vtkObject* object,
                            unsigned long event,
                            void* clientdata,
                            void* calldata);

  //@{
  /**
   * Helper method for subclasses.
   */
  void ComputeDisplayToWorld(double x, double y, double z,
                             double worldPt[4]);
  void ComputeWorldToDisplay(double x, double y, double z,
                             double displayPt[3]);
  //@}

  // The state of the widget, whether on or off (observing events or not)
  int Enabled;

  // Used to process events
  vtkCallbackCommand* EventCallbackCommand; //subclasses use one
  vtkCallbackCommand* KeyPressCallbackCommand; //listens to key activation

  // Priority at which events are processed
  float Priority;

  // This variable controls whether the picking is managed by the Picking
  // Manager process or not. True by default.
  bool PickingManaged;

  /**
   * Register internal Pickers in the Picking Manager.
   * Must be reimplemented by concrete widgets to register
   * their pickers.
   */
  virtual void RegisterPickers();

  /**
   * Unregister internal pickers from the Picking Manager.
   */
  void UnRegisterPickers();

  /**
   * Return the picking manager associated on the context on which the
   * observer currently belong.
   */
  vtkPickingManager* GetPickingManager();

  /**
   * Proceed to a pick, whether through the PickingManager if the picking is
   * managed or directly using the picker, and return the assembly path.
   */
  vtkAssemblyPath* GetAssemblyPath(double X, double Y, double Z,
                                   vtkAbstractPropPicker* picker);

  // Keypress activation controls
  vtkTypeBool KeyPressActivation;
  char KeyPressActivationValue;

  // Used to associate observers with the interactor
  vtkRenderWindowInteractor *Interactor;

  // Internal ivars for processing events
  vtkRenderer *CurrentRenderer;
  vtkRenderer *DefaultRenderer;

  unsigned long CharObserverTag;
  unsigned long DeleteObserverTag;

  // The mediator used to request resources from the interactor.
  vtkObserverMediator *ObserverMediator;
  int RequestCursorShape(int requestedShape);

private:
  vtkInteractorObserver(const vtkInteractorObserver&) = delete;
  void operator=(const vtkInteractorObserver&) = delete;

};

#endif
