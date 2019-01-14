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
/**
 * @class   vtkSliderRepresentation3D
 * @brief   provide the representation for a vtkSliderWidget with a 3D skin
 *
 * This class is used to represent and render a vtkSliderWidget. To use this
 * class, you must at a minimum specify the end points of the
 * slider. Optional instance variable can be used to modify the appearance of
 * the widget.
 *
 *
 * @sa
 * vtkSliderWidget
*/

#ifndef vtkSliderRepresentation3D_h
#define vtkSliderRepresentation3D_h

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
  /**
   * Instantiate the class.
   */
  static vtkSliderRepresentation3D *New();

  //@{
  /**
   * Standard methods for the class.
   */
  vtkTypeMacro(vtkSliderRepresentation3D,vtkSliderRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  //@}

  //@{
  /**
   * Position the first end point of the slider. Note that this point is an
   * instance of vtkCoordinate, meaning that Point 1 can be specified in a
   * variety of coordinate systems, and can even be relative to another
   * point. To set the point, you'll want to get the Point1Coordinate and
   * then invoke the necessary methods to put it into the correct coordinate
   * system and set the correct initial value.
   */
  vtkCoordinate *GetPoint1Coordinate();
  void SetPoint1InWorldCoordinates(double x, double y, double z);
  //@}

  //@{
  /**
   * Position the second end point of the slider. Note that this point is an
   * instance of vtkCoordinate, meaning that Point 1 can be specified in a
   * variety of coordinate systems, and can even be relative to another
   * point. To set the point, you'll want to get the Point2Coordinate and
   * then invoke the necessary methods to put it into the correct coordinate
   * system and set the correct initial value.
   */
  vtkCoordinate *GetPoint2Coordinate();
  void SetPoint2InWorldCoordinates(double x, double y, double z);
  //@}

  //@{
  /**
   * Specify the title text for this widget. If the value is not set, or set
   * to the empty string "", then the title text is not displayed.
   */
  void SetTitleText(const char*) override;
  const char* GetTitleText() override;
  //@}

  //@{
  /**
   * Specify whether to use a sphere or cylinder slider shape. By default, a
   * sphere shape is used.
   */
  vtkSetClampMacro(SliderShape,int,SphereShape,CylinderShape);
  vtkGetMacro(SliderShape, int);
  void SetSliderShapeToSphere() { this->SetSliderShape(SphereShape); }
  void SetSliderShapeToCylinder() { this->SetSliderShape(CylinderShape); }
  //@}

  //@{
  /**
   * Set the rotation of the slider widget around the axis of the widget. This is
   * used to control which way the widget is initially oriented. (This is especially
   * important for the label and title.)
   */
  vtkSetMacro(Rotation,double);
  vtkGetMacro(Rotation,double);
  //@}

  //@{
  /**
   * Get the slider properties. The properties of the slider when selected
   * and unselected can be manipulated.
   */
  vtkGetObjectMacro(SliderProperty,vtkProperty);
  //@}

  //@{
  /**
   * Get the properties for the tube and end caps.
   */
  vtkGetObjectMacro(TubeProperty,vtkProperty);
  vtkGetObjectMacro(CapProperty,vtkProperty);
  //@}

  //@{
  /**
   * Get the selection property. This property is used to modify the appearance of
   * selected objects (e.g., the slider).
   */
  vtkGetObjectMacro(SelectedProperty,vtkProperty);
  //@}

  //@{
  /**
   * Methods to interface with the vtkSliderWidget.
   */
  void PlaceWidget(double bounds[6]) override;
  void BuildRepresentation() override;
  void StartWidgetInteraction(double eventPos[2]) override;
  void WidgetInteraction(double newEventPos[2]) override;
  void Highlight(int) override;
  //@}

  //@{
  /**
   * Methods supporting the rendering process.
   */
  double *GetBounds() VTK_SIZEHINT(6) override;
  void GetActors(vtkPropCollection*) override;
  void ReleaseGraphicsResources(vtkWindow*) override;
  int RenderOpaqueGeometry(vtkViewport*) override;
  int RenderTranslucentPolygonalGeometry(vtkViewport*) override;
  vtkTypeBool HasTranslucentPolygonalGeometry() override;
  //@}

  /**
   * Override GetMTime to include point coordinates
   */
  vtkMTimeType GetMTime() override;

  /*
  * Register internal Pickers within PickingManager
  */
  void RegisterPickers() override;

protected:
  vtkSliderRepresentation3D();
  ~vtkSliderRepresentation3D() override;

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

  // Manage the state of the widget
  enum _SliderShape {
    SphereShape,
    CylinderShape
  };



private:
  vtkSliderRepresentation3D(const vtkSliderRepresentation3D&) = delete;
  void operator=(const vtkSliderRepresentation3D&) = delete;
};

#endif
