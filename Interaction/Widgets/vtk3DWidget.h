/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtk3DWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtk3DWidget - an abstract superclass for 3D widgets
// .SECTION Description
// vtk3DWidget is an abstract superclass for 3D interactor observers. These
// 3D widgets represent themselves in the scene, and have special callbacks
// associated with them that allows interactive manipulation of the widget.
// Inparticular, the difference between a vtk3DWidget and its abstract
// superclass vtkInteractorObserver is that vtk3DWidgets are "placed" in 3D
// space.  vtkInteractorObservers have no notion of where they are placed,
// and may not exist in 3D space at all.  3D widgets also provide auxiliary
// functions like producing a transformation, creating polydata (for seeding
// streamlines, probes, etc.) or creating implicit functions. See the
// concrete subclasses for particulars.
//
// Typically the widget is used by specifying a vtkProp3D or VTK dataset as
// input, and then invoking the "On" method to activate it. (You can also
// specify a bounding box to help position the widget.) Prior to invoking the
// On() method, the user may also wish to use the PlaceWidget() to initially
// position it. The 'i' (for "interactor") keypresses also can be used to
// turn the widgets on and off (methods exist to change the key value
// and enable keypress activiation).
//
// To support interactive manipulation of objects, this class (and
// subclasses) invoke the events StartInteractionEvent, InteractionEvent, and
// EndInteractionEvent.  These events are invoked when the vtk3DWidget enters
// a state where rapid response is desired: mouse motion, etc. The events can
// be used, for example, to set the desired update frame rate
// (StartInteractionEvent), operate on the vtkProp3D or other object
// (InteractionEvent), and set the desired frame rate back to normal values
// (EndInteractionEvent).
//
// Note that the Priority attribute inherited from vtkInteractorObserver has
// a new default value which is now 0.5 so that all 3D widgets have a higher
// priority than the usual interactor styles.
//
// .SECTION See Also
// vtkBoxWidget vtkPlaneWidget vtkLineWidget vtkPointWidget
// vtkSphereWidget vtkImplicitPlaneWidget

#ifndef __vtk3DWidget_h
#define __vtk3DWidget_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkInteractorObserver.h"

class vtk3DWidgetConnection;
class vtkAlgorithmOutput;
class vtkDataSet;
class vtkProp3D;

class VTKINTERACTIONWIDGETS_EXPORT vtk3DWidget : public vtkInteractorObserver
{
public:
  vtkTypeMacro(vtk3DWidget,vtkInteractorObserver);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This method is used to initially place the widget.  The placement of the
  // widget depends on whether a Prop3D or input dataset is provided. If one
  // of these two is provided, they will be used to obtain a bounding box,
  // around which the widget is placed. Otherwise, you can manually specify a
  // bounds with the PlaceWidget(bounds) method. Note: PlaceWidget(bounds)
  // is required by all subclasses; the other methods are provided as
  // convenience methods.
  virtual void PlaceWidget(double bounds[6]) = 0;
  virtual void PlaceWidget();
  virtual void PlaceWidget(double xmin, double xmax, double ymin, double ymax,
                           double zmin, double zmax);

  // Description:
  // Specify a vtkProp3D around which to place the widget. This
  // is not required, but if supplied, it is used to initially
  // position the widget.
  virtual void SetProp3D(vtkProp3D*);
  vtkGetObjectMacro(Prop3D,vtkProp3D);

  // Description:
  // Specify the input dataset. This is not required, but if supplied,
  // and no vtkProp3D is specified, it is used to initially position
  // the widget.
  virtual void SetInputData(vtkDataSet*);
  virtual void SetInputConnection(vtkAlgorithmOutput*);
  virtual vtkDataSet *GetInput();

  // Description:
  // Set/Get a factor representing the scaling of the widget upon placement
  // (via the PlaceWidget() method). Normally the widget is placed so that
  // it just fits within the bounding box defined in PlaceWidget(bounds).
  // The PlaceFactor will make the widget larger (PlaceFactor > 1) or smaller
  // (PlaceFactor < 1). By default, PlaceFactor is set to 0.5.
  vtkSetClampMacro(PlaceFactor,double,0.01,VTK_DOUBLE_MAX);
  vtkGetMacro(PlaceFactor,double);

  // Description:
  // Set/Get the factor that controls the size of the handles that
  // appear as part of the widget. These handles (like spheres, etc.)
  // are used to manipulate the widget, and are sized as a fraction of
  // the screen diagonal.
  vtkSetClampMacro(HandleSize,double,0.001,0.5);
  vtkGetMacro(HandleSize,double);

protected:
  vtk3DWidget();
  ~vtk3DWidget();

  // Used to position and scale the widget initially
  vtkProp3D *Prop3D;

  vtk3DWidgetConnection *ConnectionHolder;

  //has the widget ever been placed
  double PlaceFactor;
  int Placed;
  void AdjustBounds(double bounds[6], double newBounds[6], double center[3]);

  //control the size of handles (if there are any)
  double InitialBounds[6];
  double InitialLength;
  double HandleSize;
  double SizeHandles(double factor);
  virtual void SizeHandles() {}//subclass in turn invokes parent's SizeHandles()

  //used to track the depth of the last pick; also interacts with handle sizing
  int   ValidPick;
  double LastPickPosition[3];

  void UpdateInput();

private:
  vtk3DWidget(const vtk3DWidget&);  // Not implemented.
  void operator=(const vtk3DWidget&);  // Not implemented.

};

#endif
