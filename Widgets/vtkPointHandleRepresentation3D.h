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


#ifndef __vtkPointHandleRepresentation3D_h
#define __vtkPointHandleRepresentation3D_h

#include "vtkHandleRepresentation.h"
#include "vtkCursor3D.h"

class vtkCursor3D;
class vtkProperty;
class vtkActor;
class vtkPolyDataMapper;
class vtkCellPicker;

class VTK_WIDGETS_EXPORT vtkPointHandleRepresentation3D : public vtkHandleRepresentation
{
public:
  // Description:
  // Instantiate this class.
  static vtkPointHandleRepresentation3D *New();

  // Description:
  // Standard methods for instances of this class.
  vtkTypeRevisionMacro(vtkPointHandleRepresentation3D,vtkHandleRepresentation);
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
  // shadows, and cursor are all translated simultaneously as the point
  // moves.
  void SetTranslationMode(int mode)
    { this->Cursor3D->SetTranslationMode(mode); this->Cursor3D->Update(); }
  int GetTranslationMode()
    { return this->Cursor3D->GetTranslationMode(); }
  void TranslationModeOn()
    { this->SetTranslationMode(1); }
  void TranslationModeOff()
    { this->SetTranslationMode(0); }
  
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
  // Methods to make this class properly act like a vtkWidgetRepresentation.
  virtual void BuildRepresentation();
  virtual void StartWidgetInteraction(double eventPos[2]);
  virtual void WidgetInteraction(double eventPos[2]);
  virtual int ComputeInteractionState(int X, int Y, int modify=0);
  virtual void PlaceWidget(double bounds[6]);

  // Description:
  // Methods to make this class behave as a vtkProp.
  virtual void ShallowCopy(vtkProp *prop);
  virtual void GetActors(vtkPropCollection *);
  virtual void ReleaseGraphicsResources(vtkWindow *);
  virtual int RenderOpaqueGeometry(vtkViewport *viewport);
  virtual int RenderTranslucentGeometry(vtkViewport *viewport);
  
protected:
  vtkPointHandleRepresentation3D();
  ~vtkPointHandleRepresentation3D();

  // the cursor3D
  vtkActor          *Actor;
  vtkPolyDataMapper *Mapper;
  vtkCursor3D       *Cursor3D;
  void Highlight(int highlight);

  // Do the picking
  vtkCellPicker *CursorPicker;
  double LastPickPosition[3];
  double LastEventPosition[2];
  
  // Methods to manipulate the cursor
  int  ConstraintAxis;
  void Translate(double *p1, double *p2);
  void Scale(double *p1, double *p2, double eventPos[2]);
  void MoveFocus(double *p1, double *p2);
  int  TranslationMode;

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
  
private:
  vtkPointHandleRepresentation3D(const vtkPointHandleRepresentation3D&);  //Not implemented
  void operator=(const vtkPointHandleRepresentation3D&);  //Not implemented
};

#endif
