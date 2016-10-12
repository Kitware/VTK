/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWidgetRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkWidgetRepresentation
 * @brief   abstract class defines interface between the widget and widget representation classes
 *
 * This class is used to define the API for, and partially implement, a
 * representation for different types of widgets. Note that the widget
 * representation (i.e., subclasses of vtkWidgetRepresentation) are a type of
 * vtkProp; meaning that they can be associated with a vtkRenderer end
 * embedded in a scene like any other vtkActor. However,
 * vtkWidgetRepresentation also defines an API that enables it to be paired
 * with a subclass vtkAbstractWidget, meaning that it can be driven by a
 * widget, serving to represent the widget as the widget responds to
 * registered events.
 *
 * The API defined here should be regarded as a guideline for implementing
 * widgets and widget representations. Widget behavior is complex, as is the
 * way the representation responds to the registered widget events, so the API
 * may vary from widget to widget to reflect this complexity.
 *
 * @warning
 * The separation of the widget event handling and representation enables
 * users and developers to create new appearances for the widget. It also
 * facilitates parallel processing, where the client application handles
 * events, and remote representations of the widget are slaves to the
 * client (and do not handle events).
*/

#ifndef vtkWidgetRepresentation_h
#define vtkWidgetRepresentation_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkProp.h"
#include "vtkWeakPointer.h" // needed for vtkWeakPointer iVar.

class vtkAbstractPropPicker;
class vtkPickingManager;
class vtkRenderer;


class VTKINTERACTIONWIDGETS_EXPORT vtkWidgetRepresentation : public vtkProp
{
public:
  //@{
  /**
   * Standard methods for instances of this class.
   */
  vtkTypeMacro(vtkWidgetRepresentation,vtkProp);
  void PrintSelf(ostream& os, vtkIndent indent);
  //@}

  //@{
  /**
   * Enable/Disable the use of a manager to process the picking.
   * Enabled by default.
   */
  vtkBooleanMacro(PickingManaged, bool);
  vtkSetMacro(PickingManaged, bool);
  vtkGetMacro(PickingManaged, bool);
  //@}

  //@{
  /**
   * Subclasses of vtkWidgetRepresentation must implement these methods. This is
   * considered the minimum API for a widget representation.
   * <pre>
   * SetRenderer() - the renderer in which the representations draws itself.
   * Typically the renderer is set by the associated widget.
   * Use the widget's SetCurrentRenderer() method in most cases;
   * otherwise there is a risk of inconsistent behavior as events
   * and drawing may be performed in different viewports.
   * BuildRepresentation() - update the geometry of the widget based on its
   * current state.
   * </pre>
   * WARNING: The renderer is NOT reference counted by the representation,
   * in order to avoid reference loops.  Be sure that the representation
   * lifetime does not extend beyond the renderer lifetime.
   */
  virtual void SetRenderer(vtkRenderer *ren);
  virtual vtkRenderer* GetRenderer();
  virtual void BuildRepresentation() = 0;
  //@}

  /**
   * The following is a suggested API for widget representations. These methods
   * define the communication between the widget and its representation. These
   * methods are only suggestions because widgets take on so many different
   * forms that a universal API is not deemed practical. However, these methods
   * should be implemented when possible to insure that the VTK widget hierarchy
   * remains self-consistent.
   * <pre>
   * PlaceWidget() - given a bounding box (xmin,xmax,ymin,ymax,zmin,zmax), place
   * the widget inside of it. The current orientation of the widget
   * is preserved, only scaling and translation is performed.
   * StartWidgetInteraction() - generally corresponds to a initial event (e.g.,
   * mouse down) that starts the interaction process
   * with the widget.
   * WidgetInteraction() - invoked when an event causes the widget to change
   * appearance.
   * EndWidgetInteraction() - generally corresponds to a final event (e.g., mouse up)
   * and completes the interaction sequence.
   * ComputeInteractionState() - given (X,Y) display coordinates in a renderer, with a
   * possible flag that modifies the computation,
   * what is the state of the widget?
   * GetInteractionState() - return the current state of the widget. Note that the
   * value of "0" typically refers to "outside". The
   * interaction state is strictly a function of the
   * representation, and the widget/represent must agree
   * on what they mean.
   * Highlight() - turn on or off any highlights associated with the widget.
   * Highlights are generally turned on when the widget is selected.
   * </pre>
   * Note that subclasses may ignore some of these methods and implement their own
   * depending on the specifics of the widget.
   */
  virtual void PlaceWidget(double* vtkNotUsed(bounds[6])) {}
  virtual void StartWidgetInteraction(double eventPos[2]) { (void)eventPos; }
  virtual void WidgetInteraction(double newEventPos[2]) { (void)newEventPos; }
  virtual void EndWidgetInteraction(double newEventPos[2]) { (void)newEventPos; }
  virtual int ComputeInteractionState(int X, int Y, int modify=0);
  virtual int GetInteractionState()
    {return this->InteractionState;}
  virtual void Highlight(int vtkNotUsed(highlightOn)) {}

  //@{
  /**
   * Set/Get a factor representing the scaling of the widget upon placement
   * (via the PlaceWidget() method). Normally the widget is placed so that
   * it just fits within the bounding box defined in PlaceWidget(bounds).
   * The PlaceFactor will make the widget larger (PlaceFactor > 1) or smaller
   * (PlaceFactor < 1). By default, PlaceFactor is set to 0.5.
   */
  vtkSetClampMacro(PlaceFactor,double,0.01,VTK_DOUBLE_MAX);
  vtkGetMacro(PlaceFactor,double);
  //@}

  //@{
  /**
   * Set/Get the factor that controls the size of the handles that appear as
   * part of the widget (if any). These handles (like spheres, etc.)  are
   * used to manipulate the widget. The HandleSize data member allows you
   * to change the relative size of the handles. Note that while the handle
   * size is typically expressed in pixels, some subclasses may use a relative size
   * with respect to the viewport. (As a corollary, the value of this ivar is often
   * set by subclasses of this class during instance instantiation.)
   */
  vtkSetClampMacro(HandleSize,double,0.001,1000);
  vtkGetMacro(HandleSize,double);
  //@}

  //@{
  /**
   * Some subclasses use this data member to keep track of whether to render
   * or not (i.e., to minimize the total number of renders).
   */
  vtkGetMacro( NeedToRender, int );
  vtkSetClampMacro( NeedToRender, int, 0, 1 );
  vtkBooleanMacro( NeedToRender, int );
  //@}

  /**
   * Methods to make this class behave as a vtkProp. They are repeated here (from the
   * vtkProp superclass) as a reminder to the widget implementor. Failure to implement
   * these methods properly may result in the representation not appearing in the scene
   * (i.e., not implementing the Render() methods properly) or leaking graphics resources
   * (i.e., not implementing ReleaseGraphicsResources() properly).
   */
  virtual double *GetBounds() {return NULL;}
  virtual void ShallowCopy(vtkProp *prop);
  virtual void GetActors(vtkPropCollection *) {}
  virtual void GetActors2D(vtkPropCollection *) {}
  virtual void GetVolumes(vtkPropCollection *) {}
  virtual void ReleaseGraphicsResources(vtkWindow *) {}
  virtual int RenderOverlay(vtkViewport *vtkNotUsed(viewport)) {return 0;}
  virtual int RenderOpaqueGeometry(vtkViewport *vtkNotUsed(viewport)) {return 0;}
  virtual int RenderTranslucentPolygonalGeometry(vtkViewport *vtkNotUsed(viewport)) {return 0;}
  virtual int RenderVolumetricGeometry(vtkViewport *vtkNotUsed(viewport)) {return 0;}
  virtual int HasTranslucentPolygonalGeometry() { return 0; }

protected:
  vtkWidgetRepresentation();
  ~vtkWidgetRepresentation();

  // The renderer in which this widget is placed
  vtkWeakPointer<vtkRenderer> Renderer;

  // The state of this representation based on a recent event
  int InteractionState;

  // These are used to track the beginning of interaction with the representation
  // It's dimensioned [3] because some events re processed in 3D.
  double StartEventPosition[3];

  // Instance variable and members supporting suclasses
  double PlaceFactor; // Used to control how widget is placed around bounding box
  int    Placed; // Indicate whether widget has been placed
  void   AdjustBounds(double bounds[6], double newBounds[6], double center[3]);
  double InitialBounds[6]; //initial bounds on place widget (valid after PlaceWidget)
  double InitialLength; //initial length on place widget

  // Sizing handles is tricky because the procedure requires information
  // relative to the last pick, as well as a live renderer to perform
  // coordinate conversions. In some cases, a pick is never made so handle
  // sizing has to follow a different path. The following ivars help with
  // this process.
  int    ValidPick; //indicate when valid picks are made

  // This variable controls whether the picking is managed by the Picking
  // Manager or not. True by default.
  bool PickingManaged;

  /**
   * Register internal Pickers in the Picking Manager.
   * Must be reimplemented by concrete widget representations to register
   * their pickers.
   */
  virtual void RegisterPickers();

  /**
   * Unregister internal pickers from the Picking Manager.
   */
  virtual void UnRegisterPickers();

  /**
   * Update the pickers registered in the Picking Manager when pickers are
   * modified.
   */
  virtual void PickersModified();

  /**
   * Return the picking manager associated on the context on which the widget
   * representation currently belong.
   */
  vtkPickingManager* GetPickingManager();

  /**
   * Proceed to a pick, whether through the PickingManager if the picking is
   * managed or directly using the registered picker, and return the assembly
   * path.
   */
  vtkAssemblyPath* GetAssemblyPath(double X, double Y, double Z,
                                   vtkAbstractPropPicker* picker);


  // Members use to control handle size. The two methods return a "radius"
  // in world coordinates. Note that the HandleSize data member is used
  // internal to the SizeHandles__() methods.
  double HandleSize; //controlling relative size of widget handles
  double SizeHandlesRelativeToViewport(double factor, double pos[3]);
  double SizeHandlesInPixels(double factor,double pos[3]);

  // Try and reduce multiple renders
  int NeedToRender;

  // This is the time that the representation was built. This data member
  // can be used to reduce the time spent building the widget.
  vtkTimeStamp  BuildTime;

private:
  vtkWidgetRepresentation(const vtkWidgetRepresentation&) VTK_DELETE_FUNCTION;
  void operator=(const vtkWidgetRepresentation&) VTK_DELETE_FUNCTION;
};

#endif
