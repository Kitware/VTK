/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSliderRepresentation2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSliderRepresentation2D
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

#ifndef vtkSliderRepresentation2D_h
#define vtkSliderRepresentation2D_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkSliderRepresentation.h"
#include "vtkCoordinate.h" // For vtkViewportCoordinateMacro

class vtkPoints;
class vtkCellArray;
class vtkPolyData;
class vtkPolyDataMapper2D;
class vtkActor2D;
class vtkCoordinate;
class vtkProperty2D;
class vtkPropCollection;
class vtkWindow;
class vtkViewport;
class vtkTransform;
class vtkTransformPolyDataFilter;
class vtkTextProperty;
class vtkTextMapper;
class vtkTextActor;


class VTKINTERACTIONWIDGETS_EXPORT vtkSliderRepresentation2D : public vtkSliderRepresentation
{
public:
  /**
   * Instantiate the class.
   */
  static vtkSliderRepresentation2D *New();

  //@{
  /**
   * Standard methods for the class.
   */
  vtkTypeMacro(vtkSliderRepresentation2D,vtkSliderRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  //@}

  /**
   * Position the first end point of the slider. Note that this point is an
   * instance of vtkCoordinate, meaning that Point 1 can be specified in a
   * variety of coordinate systems, and can even be relative to another
   * point. To set the point, you'll want to get the Point1Coordinate and
   * then invoke the necessary methods to put it into the correct coordinate
   * system and set the correct initial value.
   */
  vtkCoordinate *GetPoint1Coordinate();

  /**
   * Position the second end point of the slider. Note that this point is an
   * instance of vtkCoordinate, meaning that Point 1 can be specified in a
   * variety of coordinate systems, and can even be relative to another
   * point. To set the point, you'll want to get the Point2Coordinate and
   * then invoke the necessary methods to put it into the correct coordinate
   * system and set the correct initial value.
   */
  vtkCoordinate *GetPoint2Coordinate();

  //@{
  /**
   * Specify the label text for this widget. If the value is not set, or set
   * to the empty string "", then the label text is not displayed.
   */
  void SetTitleText(const char*) VTK_OVERRIDE;
  const char* GetTitleText() VTK_OVERRIDE;
  //@}

  //@{
  /**
   * Get the slider properties. The properties of the slider when selected
   * and unselected can be manipulated.
   */
  vtkGetObjectMacro(SliderProperty,vtkProperty2D);
  //@}

  //@{
  /**
   * Get the properties for the tube and end caps.
   */
  vtkGetObjectMacro(TubeProperty,vtkProperty2D);
  vtkGetObjectMacro(CapProperty,vtkProperty2D);
  //@}

  //@{
  /**
   * Get the selection property. This property is used to modify the appearance of
   * selected objects (e.g., the slider).
   */
  vtkGetObjectMacro(SelectedProperty,vtkProperty2D);
  //@}

  //@{
  /**
   * Set/Get the properties for the label and title text.
   */
  vtkGetObjectMacro(LabelProperty,vtkTextProperty);
  vtkGetObjectMacro(TitleProperty,vtkTextProperty);
  //@}

  //@{
  /**
   * Methods to interface with the vtkSliderWidget. The PlaceWidget() method
   * assumes that the parameter bounds[6] specifies the location in display space
   * where the widget should be placed.
   */
  void PlaceWidget(double bounds[6]) VTK_OVERRIDE;
  void BuildRepresentation() VTK_OVERRIDE;
  void StartWidgetInteraction(double eventPos[2]) VTK_OVERRIDE;
  void WidgetInteraction(double newEventPos[2]) VTK_OVERRIDE;
  void Highlight(int) VTK_OVERRIDE;
  //@}

  //@{
  /**
   * Methods supporting the rendering process.
   */
  void GetActors2D(vtkPropCollection*) VTK_OVERRIDE;
  void ReleaseGraphicsResources(vtkWindow*) VTK_OVERRIDE;
  int RenderOverlay(vtkViewport*) VTK_OVERRIDE;
  int RenderOpaqueGeometry(vtkViewport*) VTK_OVERRIDE;
  //@}

protected:
  vtkSliderRepresentation2D();
  ~vtkSliderRepresentation2D() VTK_OVERRIDE;

  // Positioning the widget
  vtkCoordinate *Point1Coordinate;
  vtkCoordinate *Point2Coordinate;

  // Determine the parameter t along the slider
  virtual double ComputePickPosition(double eventPos[2]);

  // Define the geometry. It is constructed in canaonical position
  // along the x-axis and then rotated into position.
  vtkTransform        *XForm;
  vtkPoints           *Points;

  vtkCellArray        *SliderCells;
  vtkPolyData         *Slider;
  vtkTransformPolyDataFilter *SliderXForm;
  vtkPolyDataMapper2D *SliderMapper;
  vtkActor2D          *SliderActor;
  vtkProperty2D       *SliderProperty;

  vtkCellArray        *TubeCells;
  vtkPolyData         *Tube;
  vtkTransformPolyDataFilter *TubeXForm;
  vtkPolyDataMapper2D *TubeMapper;
  vtkActor2D          *TubeActor;
  vtkProperty2D       *TubeProperty;

  vtkCellArray        *CapCells;
  vtkPolyData         *Cap;
  vtkTransformPolyDataFilter *CapXForm;
  vtkPolyDataMapper2D *CapMapper;
  vtkActor2D          *CapActor;
  vtkProperty2D       *CapProperty;

  vtkTextProperty     *LabelProperty;
  vtkTextMapper       *LabelMapper;
  vtkActor2D          *LabelActor;

  vtkTextProperty     *TitleProperty;
  vtkTextMapper       *TitleMapper;
  vtkActor2D          *TitleActor;

  vtkProperty2D       *SelectedProperty;

  // internal variables used for computation
  double X;

private:
  vtkSliderRepresentation2D(const vtkSliderRepresentation2D&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSliderRepresentation2D&) VTK_DELETE_FUNCTION;
};

#endif
