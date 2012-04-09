/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParallelopipedRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkParallelopipedRepresentation - Default representation for vtkParallelopipedWidget
// .SECTION Description
// This class provides the default geometrical representation for 
// vtkParallelopipedWidget. As a result of interactions of the widget, this
// representation can take on of the following shapes:
// <p>1) A parallelopiped. (8 handles, 6 faces)
// <p>2) Paralleopiped with a chair depression on any one handle. (A chair
// is a depression on one of the handles that carves inwards so as to allow 
// the user to visualize cuts in the volume). (14 handles, 9 faces).
//
// .SECTION See Also
// vtkParallelopipedWidget

#ifndef __vtkParallelopipedRepresentation_h
#define __vtkParallelopipedRepresentation_h

#include "vtkWidgetRepresentation.h"

class vtkActor;
class vtkPlane;
class vtkPoints;
class vtkPolyData;
class vtkPolyDataMapper;
class vtkProperty;
class vtkCellArray;
class vtkTransform;
class vtkHandleRepresentation;
class vtkClosedSurfacePointPlacer;
class vtkPlaneCollection;
class vtkParallelopipedTopology;

class VTK_WIDGETS_EXPORT vtkParallelopipedRepresentation 
                              : public vtkWidgetRepresentation
{
public:
  // Description:
  // Instantiate the class.
  static vtkParallelopipedRepresentation *New();

  // Description:
  // Standard methods for instances of this class.
  vtkTypeMacro(vtkParallelopipedRepresentation,vtkWidgetRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Methods to satisfy the superclass.
  virtual void GetActors(vtkPropCollection *pc);

  // Description:
  // Place the widget in the scene. You can use either of the two APIs : 
  // 1) PlaceWidget( double bounds[6] ) 
  //      Creates a cuboid conforming to the said bounds.
  // 2) PlaceWidget( double corners[8][3] )
  //      Creates a parallelopiped with corners specified. The order in 
  //      which corners are specified must obey the following rule:
  //      Corner 0 - 1 - 2 - 3 - 0  forms a face
  //      Corner 4 - 5 - 6 - 7 - 4  forms a face
  //      Corner 0 - 4 - 5 - 1 - 0  forms a face
  //      Corner 1 - 5 - 6 - 2 - 1  forms a face
  //      Corner 2 - 6 - 7 - 3 - 2  forms a face
  //      Corner 3 - 7 - 4 - 0 - 3  forms a face
  virtual void PlaceWidget(double corners[8][3]);
  virtual void PlaceWidget(double bounds[6]);
  
  // Description:
  // The interaction state may be set from a widget (e.g., PointWidget) 
  // or other object. This controls how the interaction with the
  // widget proceeds.
  vtkSetMacro(InteractionState,int);

  // Description:
  // Get the bounding planes of the object. The first 6 planes will
  // be bounding planes of the parallelopiped. If in chair mode, three 
  // additional planes will be present. The last three planes will be those
  // of the chair. The normals of all the planes will point into the object.
  void GetBoundingPlanes( vtkPlaneCollection *pc );

  // Description:
  // The parallelopiped polydata.
  void GetPolyData(vtkPolyData *pd);

  // Description:
  // The parallelopiped polydata.
  virtual double *GetBounds();

  // Description:
  // Set/Get the handle properties. 
  virtual void SetHandleProperty           (vtkProperty *);
  virtual void SetHoveredHandleProperty    (vtkProperty *);
  virtual void SetSelectedHandleProperty   (vtkProperty *);
  vtkGetObjectMacro(HandleProperty,         vtkProperty  );
  vtkGetObjectMacro(HoveredHandleProperty,  vtkProperty  );
  vtkGetObjectMacro(SelectedHandleProperty, vtkProperty  );
  
  void SetHandleRepresentation(vtkHandleRepresentation *handle);
  vtkHandleRepresentation* GetHandleRepresentation(int index);

  // Description:
  // Turns the visibility of the handles on/off. Sometimes they may get in
  // the way of visualization.
  void HandlesOn();
  void HandlesOff();
  
  // Description:
  // Get the face properties. When a face is being translated, the face gets
  // highlighted with the SelectedFaceProperty.
  vtkGetObjectMacro(FaceProperty,vtkProperty);
  vtkGetObjectMacro(SelectedFaceProperty,vtkProperty);
  
  // Description:
  // Get the outline properties. These are the properties with which the
  // parallelopiped wireframe is rendered.
  vtkGetObjectMacro(OutlineProperty,vtkProperty);
  vtkGetObjectMacro(SelectedOutlineProperty,vtkProperty);
  
  // Description:
  // This actually constructs the geometry of the widget from the various
  // data parameters.
  virtual void BuildRepresentation();
 
  // Description:
  // Methods required by vtkProp superclass.
  virtual void ReleaseGraphicsResources(vtkWindow *w);
  virtual int  RenderOverlay(vtkViewport *viewport);
  virtual int  RenderOpaqueGeometry(vtkViewport *viewport);

  // Description:
  // Given and x-y display coordinate, compute the interaction state of 
  // the widget.
  virtual int ComputeInteractionState(int X, int Y, int modify=0);

  //BTX - manage the state of the widget
  enum _InteractionState
  {
    Outside = 0,
    Inside,
    RequestResizeParallelopiped,
    RequestResizeParallelopipedAlongAnAxis,
    RequestChairMode,
    RequestTranslateParallelopiped,
    RequestScaleParallelopiped,
    RequestRotateParallelopiped,
    ResizingParallelopiped,
    ResizingParallelopipedAlongAnAxis,
    ChairMode,
    TranslatingParallelopiped,
    ScalingParallelopiped,
    RotatingParallelopiped
  };
  //ETX

  // Methods to manipulate the piped.
  virtual void Translate( double translation[3] );
  virtual void Translate( int X, int Y );
  virtual void Scale( int X, int Y );
  
  // Description:
  // Synchronize the parallelopiped handle positions with the 
  // Polygonal datastructure.
  virtual void PositionHandles();
  
  // Description:
  // Minimum thickness for the parallelopiped. User interactions cannot make
  // any individual axis of the parallopiped thinner than this value.
  // Default is 0.05 expressed as a fraction of the diagonal of the bounding
  // box used in the PlaceWidget() invocation.
  vtkSetMacro( MinimumThickness, double );
  vtkGetMacro( MinimumThickness, double );
  
protected:
  vtkParallelopipedRepresentation();
  ~vtkParallelopipedRepresentation();

  // Description:
  // Translate the nth PtId (0 <= n <= 15) by the specified amount. 
  void TranslatePoint( int n, const double motionVector[3] );
  
  // Description:
  // Set the highlight state of a handle. 
  // If handleIdx is -1, the property is applied to all handles.
  void SetHandleHighlight( int handleIdx, vtkProperty *property );

  // Description:
  // Highlight face defined by the supplied ptids with the specified property.
  void SetFaceHighlight( vtkCellArray * face, vtkProperty * );
  void HighlightAllFaces();
  void UnHighlightAllFaces();
  
  // Node can be a value within [0,7]. This will create a chair one one of
  // the handle corners. '0 < InitialChairDepth < 1' value dicates the starting
  // depth of the cavity.
  void UpdateChairAtNode( int node );
  
  // Removes any existing chairs.
  void RemoveExistingChairs();

  // Convenience method to get just the planes that define the parallelopiped.
  // If we aren't in chair mode, this will be the same as GetBoundingPlanes().
  // If we are in chair mode, this will be the first 6 planes from amongst 
  // those returned by "GetBoundingPlanes". 
  // All planes have their normals pointing inwards. 
  void GetParallelopipedBoundingPlanes( vtkPlaneCollection * pc );

  // Convenience method to edefine a plane passing through 3 points.
  void DefinePlane( vtkPlane *, double p[3][3]);

  // Convenience method to edefine a plane passing through 3 pointIds of the 
  // parallelopiped. The point Ids must like in the range [0,15], ie the
  // 15 points comprising the parallelopiped and the chair (also modelled
  // as a parallelopiped)
  void DefinePlane( vtkPlane *, vtkIdType, vtkIdType, vtkIdType);

  vtkActor                          * HexActor;
  vtkPolyDataMapper                 * HexMapper;
  vtkPolyData                       * HexPolyData;
  vtkPoints                         * Points;
  vtkActor                          * HexFaceActor;
  vtkPolyDataMapper                 * HexFaceMapper;
  vtkPolyData                       * HexFacePolyData;
  
  double                              LastEventPosition[2];
  
  // Cache the axis index used for face aligned resize.
  int                                 LastResizeAxisIdx;

  vtkHandleRepresentation           * HandleRepresentation;
  vtkHandleRepresentation          ** HandleRepresentations;
  int                                 CurrentHandleIdx;
  int                                 ChairHandleIdx;
  
  
  // When a chair is carved out for the first time, this is the initial 
  // depth of the chair
  double                              InitialChairDepth;
  
  vtkProperty                       * HandleProperty;
  vtkProperty                       * HoveredHandleProperty;
  vtkProperty                       * FaceProperty;
  vtkProperty                       * OutlineProperty;
  vtkProperty                       * SelectedHandleProperty;
  vtkProperty                       * SelectedFaceProperty;
  vtkProperty                       * SelectedOutlineProperty;
  vtkClosedSurfacePointPlacer       * ChairPointPlacer;
  vtkParallelopipedTopology         * Topology;
  double                              MinimumThickness;
  double                              AbsoluteMinimumThickness;

private:
  vtkParallelopipedRepresentation(const vtkParallelopipedRepresentation&);  //Not implemented
  void operator=(const vtkParallelopipedRepresentation&);  //Not implemented
};

#endif
