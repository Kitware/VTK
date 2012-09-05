/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSliderRepresentation3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSliderRepresentation3D - provide the representation for a vtkSliderWidget with a 3D skin
// .SECTION Description
// This class is used to represent and render a vtkSliderWidget. To use this
// class, you must at a minimum specify the end points of the
// slider. Optional instance variable can be used to modify the appearance of
// the widget.
//

// .SECTION See Also
// vtkSliderWidget


#ifndef __vtkSliderRepresentation3D_h
#define __vtkSliderRepresentation3D_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkSliderRepresentation.h"
#include "vtkCoordinate.h" // For vtkViewportCoordinateMacro

class vtkActor;
class vtkPolyDataMapper;
class vtkSphereSource;
class vtkCellPicker;
class vtkProperty;
class vtkCylinderSource;
class vtkVectorText;
class vtkAssembly;
class vtkTransform;
class vtkTransformPolyDataFilter;
class vtkMatrix4x4;


class VTKINTERACTIONWIDGETS_EXPORT vtkSliderRepresentation3D : public vtkSliderRepresentation
{
public:
  // Description:
  // Instantiate the class.
  static vtkSliderRepresentation3D *New();

  // Description:
  // Standard methods for the class.
  vtkTypeMacro(vtkSliderRepresentation3D,vtkSliderRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Position the first end point of the slider. Note that this point is an
  // instance of vtkCoordinate, meaning that Point 1 can be specified in a
  // variety of coordinate systems, and can even be relative to another
  // point. To set the point, you'll want to get the Point1Coordinate and
  // then invoke the necessary methods to put it into the correct coordinate
  // system and set the correct initial value.
  vtkCoordinate *GetPoint1Coordinate();
  void SetPoint1InWorldCoordinates(double x, double y, double z);

  // Description:
  // Position the second end point of the slider. Note that this point is an
  // instance of vtkCoordinate, meaning that Point 1 can be specified in a
  // variety of coordinate systems, and can even be relative to another
  // point. To set the point, you'll want to get the Point2Coordinate and
  // then invoke the necessary methods to put it into the correct coordinate
  // system and set the correct initial value.
  vtkCoordinate *GetPoint2Coordinate();
  void SetPoint2InWorldCoordinates(double x, double y, double z);

  // Description:
  // Specify the title text for this widget. If the value is not set, or set
  // to the empty string "", then the title text is not displayed.
  virtual void SetTitleText(const char*);
  virtual const char* GetTitleText();

  // Description:
  // Specify whether to use a sphere or cylinder slider shape. By default, a
  // sphere shape is used.
  vtkSetClampMacro(SliderShape,int,SphereShape,CylinderShape);
  vtkGetMacro(SliderShape, int);
  void SetSliderShapeToSphere() { this->SetSliderShape(SphereShape); }
  void SetSliderShapeToCylinder() { this->SetSliderShape(CylinderShape); }

  // Description:
  // Set the rotation of the slider widget around the axis of the widget. This is
  // used to control which way the widget is initially oriented. (This is especially
  // important for the label and title.)
  vtkSetMacro(Rotation,double);
  vtkGetMacro(Rotation,double);

  // Description:
  // Get the slider properties. The properties of the slider when selected
  // and unselected can be manipulated.
  vtkGetObjectMacro(SliderProperty,vtkProperty);

  // Description:
  // Get the properties for the tube and end caps.
  vtkGetObjectMacro(TubeProperty,vtkProperty);
  vtkGetObjectMacro(CapProperty,vtkProperty);

  // Description:
  // Get the selection property. This property is used to modify the appearance of
  // selected objects (e.g., the slider).
  vtkGetObjectMacro(SelectedProperty,vtkProperty);

  // Description:
  // Methods to interface with the vtkSliderWidget.
  virtual void PlaceWidget(double bounds[6]);
  virtual void BuildRepresentation();
  virtual void StartWidgetInteraction(double eventPos[2]);
  virtual void WidgetInteraction(double newEventPos[2]);
  virtual void Highlight(int);

  // Decsription:
  // Methods supporting the rendering process.
  virtual double *GetBounds();
  virtual void GetActors(vtkPropCollection*);
  virtual void ReleaseGraphicsResources(vtkWindow*);
  virtual int RenderOpaqueGeometry(vtkViewport*);
  virtual int RenderTranslucentPolygonalGeometry(vtkViewport*);
  virtual int HasTranslucentPolygonalGeometry();

  // Description:
  // Override GetMTime to include point coordinates
  virtual unsigned long GetMTime();

protected:
  vtkSliderRepresentation3D();
  ~vtkSliderRepresentation3D();

  // Positioning the widget
  vtkCoordinate *Point1Coordinate;
  vtkCoordinate *Point2Coordinate;
  double        Length;

  // These are the slider end points taking into account the thickness
  // of the slider
  double        SP1[3];
  double        SP2[3];

  // More ivars controlling the appearance of the widget
  double Rotation;
  int    SliderShape;

  // Do the picking
  vtkCellPicker *Picker;

  // Register internal Pickers within PickingManager
  virtual void RegisterPickers();

  // Determine the parameter t along the slider
  virtual double ComputePickPosition(double eventPos[2]);

  // The widget consists of several actors, all grouped
  // together using an assembly. This makes it easier to
  // perform the final transformation into
  vtkAssembly *WidgetAssembly;

  // Cylinder used by other objects
  vtkCylinderSource          *CylinderSource;
  vtkTransformPolyDataFilter *Cylinder;

  // The tube
  vtkPolyDataMapper *TubeMapper;
  vtkActor          *TubeActor;
  vtkProperty       *TubeProperty;

  // The slider
  vtkSphereSource   *SliderSource;
  vtkPolyDataMapper *SliderMapper;
  vtkActor          *SliderActor;
  vtkProperty       *SliderProperty;
  vtkProperty       *SelectedProperty;

  // The left cap
  vtkPolyDataMapper *LeftCapMapper;
  vtkActor          *LeftCapActor;
  vtkProperty       *CapProperty;

  // The right cap
  vtkPolyDataMapper *RightCapMapper;
  vtkActor          *RightCapActor;

  // The text. There is an extra transform used to rotate
  // both the title and label
  vtkVectorText     *LabelText;
  vtkPolyDataMapper *LabelMapper;
  vtkActor          *LabelActor;

  vtkVectorText     *TitleText;
  vtkPolyDataMapper *TitleMapper;
  vtkActor          *TitleActor;

  // Transform used during slider motion
  vtkMatrix4x4 *Matrix;
  vtkTransform *Transform;

//BTX - manage the state of the widget
  enum _SliderShape {
    SphereShape,
    CylinderShape
  };

//ETX


private:
  vtkSliderRepresentation3D(const vtkSliderRepresentation3D&);  //Not implemented
  void operator=(const vtkSliderRepresentation3D&);  //Not implemented
};

#endif
