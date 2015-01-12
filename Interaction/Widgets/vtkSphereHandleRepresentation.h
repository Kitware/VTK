/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSphereHandleRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSphereHandleRepresentation - A spherical rendition of point in 3D space
//
// .SECTION Description
// This class is a concrete implementation of vtkHandleRepresentation. It
// renders handles as spherical blobs in 3D space.
//
// .SECTION See Also
// vtkHandleRepresentation vtkHandleWidget vtkSphereSource


#ifndef vtkSphereHandleRepresentation_h
#define vtkSphereHandleRepresentation_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkHandleRepresentation.h"
#include "vtkSphereSource.h" // Needed for delegation to sphere

class vtkSphereSource;
class vtkProperty;
class vtkActor;
class vtkPolyDataMapper;
class vtkCellPicker;

class VTKINTERACTIONWIDGETS_EXPORT vtkSphereHandleRepresentation
                        : public vtkHandleRepresentation
{
public:
  // Description:
  // Instantiate this class.
  static vtkSphereHandleRepresentation *New();

  // Description:
  // Standard methods for instances of this class.
  vtkTypeMacro(vtkSphereHandleRepresentation,vtkHandleRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the position of the point in world and display coordinates. Note
  // that if the position is set outside of the bounding box, it will be
  // clamped to the boundary of the bounding box. This method overloads
  // the superclasses' SetWorldPosition() and SetDisplayPosition() in
  // order to set the focal point of the cursor properly.
  virtual void SetWorldPosition(double p[3]);
  virtual void SetDisplayPosition(double p[3]);

  // Description:
  // If translation mode is on, as the widget is moved the bounding box,
  // shadows, and cursor are all translated simultaneously as the point moves
  // (i.e., the left and middle mouse buttons act the same).  Otherwise, only
  // the cursor focal point moves, which is constrained by the bounds of the
  // point representation. (Note that the bounds can be scaled up using the
  // right mouse button.)
  vtkSetMacro(TranslationMode,int);
  vtkGetMacro(TranslationMode,int);
  vtkBooleanMacro(TranslationMode,int);

  void SetSphereRadius(double);
  double GetSphereRadius();

  // Description:
  // Set/Get the handle properties when unselected and selected.
  void SetProperty(vtkProperty*);
  void SetSelectedProperty(vtkProperty*);
  vtkGetObjectMacro(Property,vtkProperty);
  vtkGetObjectMacro(SelectedProperty,vtkProperty);

  // Description:
  // Set the "hot spot" size; i.e., the region around the focus, in which the
  // motion vector is used to control the constrained sliding action. Note the
  // size is specified as a fraction of the length of the diagonal of the
  // point widget's bounding box.
  vtkSetClampMacro(HotSpotSize,double,0.0,1.0);
  vtkGetMacro(HotSpotSize,double);

  // Description:
  // Overload the superclasses SetHandleSize() method to update internal
  // variables.
  virtual void SetHandleSize(double size);

  // Description:
  // Methods to make this class properly act like a vtkWidgetRepresentation.
  virtual double *GetBounds();
  virtual void BuildRepresentation();
  virtual void StartWidgetInteraction(double eventPos[2]);
  virtual void WidgetInteraction(double eventPos[2]);
  virtual int ComputeInteractionState(int X, int Y, int modify=0);
  virtual void PlaceWidget(double bounds[6]);

  // Description:
  // Methods to make this class behave as a vtkProp.
  virtual void ShallowCopy(vtkProp *prop);
  virtual void DeepCopy(vtkProp *prop);
  virtual void GetActors(vtkPropCollection *);
  virtual void ReleaseGraphicsResources(vtkWindow *);
  virtual int RenderOpaqueGeometry(vtkViewport *viewport);
  virtual int RenderTranslucentPolygonalGeometry(vtkViewport *viewport);
  virtual int HasTranslucentPolygonalGeometry();

  void Highlight(int highlight);
protected:
  vtkSphereHandleRepresentation();
  ~vtkSphereHandleRepresentation();

  // the cursor3D
  vtkActor          *Actor;
  vtkPolyDataMapper *Mapper;
  vtkSphereSource   *Sphere;
  // void Highlight(int highlight);

  // Do the picking
  vtkCellPicker *CursorPicker;
  double LastPickPosition[3];
  double LastEventPosition[2];

  // Register internal Pickers within PickingManager
  virtual void RegisterPickers();

  // Methods to manipulate the cursor
  int  ConstraintAxis;
  void Translate(double *p1, double *p2);
  void Scale(double *p1, double *p2, double eventPos[2]);
  void MoveFocus(double *p1, double *p2);
  void SizeBounds();

  // Properties used to control the appearance of selected objects and
  // the manipulator in general.
  vtkProperty *Property;
  vtkProperty *SelectedProperty;
  void         CreateDefaultProperties();

  // The size of the hot spot.
  double HotSpotSize;
  int    DetermineConstraintAxis(int constraint, double *x);
  int    WaitingForMotion;
  int    WaitCount;

  // Current handle sized (may reflect scaling)
  double CurrentHandleSize;

  // Control how translation works
  int TranslationMode;

private:
  vtkSphereHandleRepresentation(const vtkSphereHandleRepresentation&);  //Not implemented
  void operator=(const vtkSphereHandleRepresentation&);  //Not implemented
};

#endif
