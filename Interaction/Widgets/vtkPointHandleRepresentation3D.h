/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointHandleRepresentation3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPointHandleRepresentation3D - represent the position of a point in 3D space
// .SECTION Description
// This class is used to represent a vtkHandleWidget. It represents a position
// in 3D world coordinates using a x-y-z cursor. The cursor can be configured to
// show a bounding box and/or shadows.

// .SECTION See Also
// vtkHandleRepresentation vtkHandleWidget vtkCursor3D


#ifndef vtkPointHandleRepresentation3D_h
#define vtkPointHandleRepresentation3D_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkHandleRepresentation.h"
#include "vtkCursor3D.h" // Needed for delegation to cursor3D

class vtkCursor3D;
class vtkProperty;
class vtkActor;
class vtkPolyDataMapper;
class vtkCellPicker;

class VTKINTERACTIONWIDGETS_EXPORT vtkPointHandleRepresentation3D : public vtkHandleRepresentation
{
public:
  // Description:
  // Instantiate this class.
  static vtkPointHandleRepresentation3D *New();

  // Description:
  // Standard methods for instances of this class.
  vtkTypeMacro(vtkPointHandleRepresentation3D,vtkHandleRepresentation);
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
  // Turn on/off the wireframe bounding box.
  void SetOutline(int o)
    {this->Cursor3D->SetOutline(o);}
  int GetOutline()
    {return this->Cursor3D->GetOutline();}
  void OutlineOn()
    {this->Cursor3D->OutlineOn();}
  void OutlineOff()
    {this->Cursor3D->OutlineOff();}

  // Description:
  // Turn on/off the wireframe x-shadows.
  void SetXShadows(int o)
    {this->Cursor3D->SetXShadows(o);}
  int GetXShadows()
    {return this->Cursor3D->GetXShadows();}
  void XShadowsOn()
    {this->Cursor3D->XShadowsOn();}
  void XShadowsOff()
    {this->Cursor3D->XShadowsOff();}

  // Description:
  // Turn on/off the wireframe y-shadows.
  void SetYShadows(int o)
    {this->Cursor3D->SetYShadows(o);}
  int GetYShadows()
    {return this->Cursor3D->GetYShadows();}
  void YShadowsOn()
    {this->Cursor3D->YShadowsOn();}
  void YShadowsOff()
    {this->Cursor3D->YShadowsOff();}

  // Description:
  // Turn on/off the wireframe z-shadows.
  void SetZShadows(int o)
    {this->Cursor3D->SetZShadows(o);}
  int GetZShadows()
    {return this->Cursor3D->GetZShadows();}
  void ZShadowsOn()
    {this->Cursor3D->ZShadowsOn();}
  void ZShadowsOff()
    {this->Cursor3D->ZShadowsOff();}

  // Description:
  // If translation mode is on, as the widget is moved the bounding box,
  // shadows, and cursor are all translated and sized simultaneously as the
  // point moves (i.e., the left and middle mouse buttons act the same). If
  // translation mode is off, the cursor does not scale itself (based on the
  // specified handle size), and the bounding box and shadows do not move or
  // size themselves as the cursor focal point moves, which is constrained by
  // the bounds of the point representation. (Note that the bounds can be
  // scaled up using the right mouse button, and the bounds can be manually
  // set with the SetBounds() method.)
  void SetTranslationMode(int mode);
  vtkGetMacro(TranslationMode,int);
  vtkBooleanMacro(TranslationMode,int);

  // Description:
  // Convenience methods to turn outline and shadows on and off.
  void AllOn()
    {
      this->OutlineOn();
      this->XShadowsOn();
      this->YShadowsOn();
      this->ZShadowsOn();
    }
  void AllOff()
    {
      this->OutlineOff();
      this->XShadowsOff();
      this->YShadowsOff();
      this->ZShadowsOff();
    }

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
  // Overload the superclasses SetHandleSize() method to update internal variables.
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

  // Description:
  // Turn on/off smooth motion of the handle. See the documentation of
  // MoveFocusRequest for details. By default, SmoothMotion is ON. However,
  // in certain applications the user may want to turn it off. For instance
  // when using certain specific PointPlacer's with the representation such
  // as the vtkCellCentersPointPlacer, which causes the representation to
  // snap to the center of cells, or using a vtkPolygonalSurfacePointPlacer
  // which constrains the widget to the surface of a mesh. In such cases,
  // inherent restrictions on handle placement might conflict with a request
  // for smooth motion of the handles.
  vtkSetMacro( SmoothMotion, int );
  vtkGetMacro( SmoothMotion, int );
  vtkBooleanMacro( SmoothMotion, int );

protected:
  vtkPointHandleRepresentation3D();
  ~vtkPointHandleRepresentation3D();

  // the cursor3D
  vtkActor          *Actor;
  vtkPolyDataMapper *Mapper;
  vtkCursor3D       *Cursor3D;


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

  // Given a motion vector defined by p1 --> p2 (p1 and p2 are in
  // world coordinates), the new display position of the handle center is
  // populated into requestedDisplayPos. This is again only a request for the
  // new display position. It is up to the point placer to deduce the
  // appropriate world co-ordinates that this display position will map into.
  // The placer may even disallow such a movement.
  // If "SmoothMotion" is OFF, the returned requestedDisplayPos is the same
  // as the event position, ie the location of the mouse cursor. If its OFF,
  // incremental offsets as described above are used to compute it.
  void MoveFocusRequest( double *p1, double *p2,
                         double eventPos[2], double requestedDisplayPos[3] );

  // Properties used to control the appearance of selected objects and
  // the manipulator in general.
  vtkProperty *Property;
  vtkProperty *SelectedProperty;
  void         CreateDefaultProperties();

  // The size of the hot spot.
  double HotSpotSize;
  int    DetermineConstraintAxis(int constraint, double *x, double *startPoint);
  int    WaitingForMotion;
  int    WaitCount;

  // Current handle sized (may reflect scaling)
  double CurrentHandleSize;

  // Control how translation works
  int TranslationMode;

  int SmoothMotion;

private:
  vtkPointHandleRepresentation3D(const vtkPointHandleRepresentation3D&);  //Not implemented
  void operator=(const vtkPointHandleRepresentation3D&);  //Not implemented
};

#endif
