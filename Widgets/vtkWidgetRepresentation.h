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
// .NAME vtkWidgetRepresentation - abstract class defines widget and widget representation interface
// .SECTION Description
// This class is used to define the API for, and partially implement, a
// representation for different types of widgets. The vtkAbstracWidget
// handles events and cursor definitions; the vtkWidgetRepresentation is
// responsible for the geometric representation, and modifies its appearance
// based on certain types of events. The widget representation is also a type
// of vtkProp, as such it can be placed in the scene and rendered.

// .SECTION Caveats
// The separation of the widget event handling and representation enables
// users and developers to create new appearances for the widget. It also
// facilitates parallel processing, where the client application handles
// events, and remote representations of the widget are slaves to the 
// client (and do not handle events).


#ifndef __vtkWidgetRepresentation_h
#define __vtkWidgetRepresentation_h

#include "vtkProp.h"

class vtkRenderer;


class VTK_WIDGETS_EXPORT vtkWidgetRepresentation : public vtkProp
{
public:
  // Description:
  // Standard methods for instances of this class.
  vtkTypeRevisionMacro(vtkWidgetRepresentation,vtkProp);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Subclasses of vtkWidgetRepresentation must implement these methods. This is
  // considered the minimum API for a widget representation.
  // SetRenderer() - the renderer in which the widget is to appear must be set.
  // BuildRepresentation() - update the geometry of the widget based on its
  //                         current state.
  // </pre>
  virtual void SetRenderer(vtkRenderer *ren);
  vtkGetObjectMacro(Renderer,vtkRenderer);
  virtual void BuildRepresentation() = 0;

  // Description:
  // The following is a suggested API for widget representations. These methods
  // define the communication between the widget and its representation. These
  // methods are only suggestions because widgets take on so many different
  // forms that a universal API is not deemed practical. However, these methods
  // should be implemented when possible to insure that the VTK widget hierarchy
  // remains self-consistent.
  // <pre>
  // PlaceWidget() - given a bounding box (xmin,xmax,ymin,ymax,zmin,zmax), place 
  //                 the widget inside of it. The current orientation of the widget 
  //                 is preserved, only scaling and translation is performed.
  // StartWidgetInteraction() - generally corresponds to a initial event (e.g.,
  //                            mouse down) that starts the interaction process
  //                            with the widget.
  // WidgetInteraction() - invoked when an event causes the widget to change 
  //                       appearance.
  // EndWidgetInteraction() - generally corresponds to a final event (e.g., mouse up)
  //                          and completes the interaction sequence.
  // ComputeInteractionState() - given (X,Y) display coordinates in a renderer, with a
  //                             possible flag that modifies the computation,
  //                             what is the state of the widget?
  // GetInteractionState() - return the current state of the widget. Note that the
  //                         value of "0" typically refers to "outside". The 
  //                         interaction state is strictly a function of the
  //                         representation, and the widget/represent must agree
  //                         on what they mean.
  // Highlight() - turn on or off any highlights associated with the widget.
  //               Highlights are generally turned on when the widget is selected.
  // </pre>
  // Note that subclasses may ignore some of these methods and implement their own
  // depending on the specifics of the widget.
  virtual void PlaceWidget(double* vtkNotUsed(bounds[6])) {}
  virtual void StartWidgetInteraction(double* vtkNotUsed(eventPos[2])) {}
  virtual void WidgetInteraction(double* vtkNotUsed(newEventPos[2])) {}
  virtual void EndWidgetInteraction(double* vtkNotUsed(newEventPos[2])) {}
  virtual int ComputeInteractionState(int X, int Y, int modify=0);
  virtual int GetInteractionState()
    {return this->InteractionState;}
  virtual void Highlight(int vtkNotUsed(highlightOn)) {}

  // Description:
  // Set/Get a factor representing the scaling of the widget upon placement
  // (via the PlaceWidget() method). Normally the widget is placed so that
  // it just fits within the bounding box defined in PlaceWidget(bounds).
  // The PlaceFactor will make the widget larger (PlaceFactor > 1) or smaller
  // (PlaceFactor < 1). By default, PlaceFactor is set to 0.5.
  vtkSetClampMacro(PlaceFactor,double,0.01,VTK_FLOAT_MAX);
  vtkGetMacro(PlaceFactor,double);

  // Description:
  // Set/Get the factor that controls the size of the handles that appear as
  // part of the widget (if any). These handles (like spheres, etc.)  are
  // used to manipulate the widget, and are sized as a fraction of the screen
  // diagonal.
  vtkSetClampMacro(HandleSize,double,0.001,0.5);
  vtkGetMacro(HandleSize,double);

  vtkGetMacro( NeedToRender, int );
  vtkSetClampMacro( NeedToRender, int, 0, 1 );
  vtkBooleanMacro( NeedToRender, int );
  
  // Description:
  // Methods to make this class behave as a vtkProp. They are repeated here (from the
  // vtkProp superclass) as a reminder to the widget implementor.
  virtual void ShallowCopy(vtkProp *prop);
  virtual void GetActors(vtkPropCollection *) {}
  virtual void GetActors2D(vtkPropCollection *) {}
  virtual void GetVolumes(vtkPropCollection *) {}
  virtual void ReleaseGraphicsResources(vtkWindow *) {}
  virtual int RenderOverlay(vtkViewport *vtkNotUsed(viewport)) {return 0;}
  virtual int RenderOpaqueGeometry(vtkViewport *vtkNotUsed(viewport)) {return 0;}
  virtual int RenderTranslucentGeometry(vtkViewport *vtkNotUsed(viewport)) {return 0;}
  
protected:
  vtkWidgetRepresentation();
  ~vtkWidgetRepresentation();

  // The renderer in which this widget is placed
  vtkRenderer *Renderer;

  // The state of this representation based on a recent event
  int InteractionState;

  // These are used to track the beginning of interaction with the representation
  // It's dimensioned [3] because some events re processed in 3D
  double StartEventPosition[3];

  // Instance variable supporting suclasses
  double PlaceFactor;
  int Placed; 
  void AdjustBounds(double bounds[6], double newBounds[6], double center[3]);
  
  // Convenience members to support widget representations
  double InitialBounds[6];
  double InitialLength;
  double HandleSize;
  
  int NeedToRender;
  
  // This is the time that the representation was built.
  vtkTimeStamp  BuildTime;

private:
  vtkWidgetRepresentation(const vtkWidgetRepresentation&);  //Not implemented
  void operator=(const vtkWidgetRepresentation&);  //Not implemented
};

#endif
