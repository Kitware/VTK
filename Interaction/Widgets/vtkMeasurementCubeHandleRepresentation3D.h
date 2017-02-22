/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMeasurementCubeHandleRepresentation3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkMeasurementCubeHandleRepresentation3D
 * @brief   represent a unit cube for measuring/comparing to data.
 *
 * @sa
 * vtkPolygonalHandleRepresentation3D vtkHandleRepresentation vtkHandleWidget
*/

#ifndef vtkMeasurementCubeHandleRepresentation3D_h
#define vtkMeasurementCubeHandleRepresentation3D_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkHandleRepresentation.h"

class vtkProperty;
class vtkPolyDataMapper;
class vtkCellPicker;
class vtkTransformPolyDataFilter;
class vtkMatrixToLinearTransform;
class vtkMatrix4x4;
class vtkPolyData;
class vtkAbstractTransform;
class vtkActor;
class vtkFollower;
class vtkBillboardTextActor3D;

class VTKINTERACTIONWIDGETS_EXPORT vtkMeasurementCubeHandleRepresentation3D
                           : public vtkHandleRepresentation
{
public:

  /**
   * Instantiate this class.
   */
  static vtkMeasurementCubeHandleRepresentation3D *New();

  //@{
  /**
   * Standard methods for instances of this class.
   */
  vtkTypeMacro(vtkMeasurementCubeHandleRepresentation3D,
               vtkHandleRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  //@}

  //@{
  /**
   * Set the position of the point in world and display coordinates.
   */
  void SetWorldPosition(double p[3]) VTK_OVERRIDE;
  void SetDisplayPosition(double p[3]) VTK_OVERRIDE;
  //@}

  //@{
  /**
   * Get the handle polydata.
   */
  vtkPolyData * GetHandle();
  //@}

  //@{
  /**
   * Set/Get the handle properties when unselected and selected.
   */
  void SetProperty(vtkProperty*);
  void SetSelectedProperty(vtkProperty*);
  vtkGetObjectMacro(Property,vtkProperty);
  vtkGetObjectMacro(SelectedProperty,vtkProperty);
  //@}

  /**
   * Get the transform used to transform the generic handle polydata before
   * placing it in the render window
   */
  virtual vtkAbstractTransform * GetTransform();

  //@{
  /**
   * Methods to make this class properly act like a vtkWidgetRepresentation.
   */
  void BuildRepresentation() VTK_OVERRIDE;
  void StartWidgetInteraction(double eventPos[2]) VTK_OVERRIDE;
  void WidgetInteraction(double eventPos[2]) VTK_OVERRIDE;
  int ComputeInteractionState(int X, int Y, int modify=0) VTK_OVERRIDE;
  //@}

  //@{
  /**
   * Methods to make this class behave as a vtkProp.
   */
  void ShallowCopy(vtkProp *prop) VTK_OVERRIDE;
  void DeepCopy(vtkProp *prop) VTK_OVERRIDE;
  void GetActors(vtkPropCollection *) VTK_OVERRIDE;
  void ReleaseGraphicsResources(vtkWindow *) VTK_OVERRIDE;
  int RenderOpaqueGeometry(vtkViewport *viewport) VTK_OVERRIDE;
  int RenderTranslucentPolygonalGeometry(vtkViewport *viewport) VTK_OVERRIDE;
  int HasTranslucentPolygonalGeometry() VTK_OVERRIDE;
  double *GetBounds() VTK_OVERRIDE;
  //@}

  //@{
  /**
   * A label may be associated with the cube. The string can be set via
   * SetLabelText. The visibility of the label can be turned on / off.
   */
  vtkSetMacro( LabelVisibility, int );
  vtkGetMacro( LabelVisibility, int );
  vtkBooleanMacro( LabelVisibility, int );
  vtkSetMacro( SelectedLabelVisibility, int );
  vtkGetMacro( SelectedLabelVisibility, int );
  vtkBooleanMacro( SelectedLabelVisibility, int );

  virtual void SetLabelTextInput( const char * label );
  virtual char * GetLabelTextInput();
  //@}

  //@{
  /**
   * Get the label text actor
   */
  vtkGetObjectMacro( LabelText, vtkBillboardTextActor3D );
  //@}

  //@{
  /**
   * Toggle the visibility of the handle on and off
   */
  vtkSetMacro( HandleVisibility, int );
  vtkGetMacro( HandleVisibility, int );
  vtkBooleanMacro( HandleVisibility, int );
  //@}

  //@{
  /**
   * Toggle highlighting (used when the cube is selected).
   */
  void Highlight(int highlight) VTK_OVERRIDE;
  //@}

  //@{
  /**
   * Turn on/off smooth motion of the handle. See the documentation of
   * MoveFocusRequest for details. By default, SmoothMotion is ON. However,
   * in certain applications the user may want to turn it off. For instance
   * when using certain specific PointPlacer's with the representation such
   * as the vtkCellCentersPointPlacer, which causes the representation to
   * snap to the center of cells. In such cases, inherent restrictions on
   * handle placement might conflict with a request for smooth motion of the
   * handles.
   */
  vtkSetMacro( SmoothMotion, int );
  vtkGetMacro( SmoothMotion, int );
  vtkBooleanMacro( SmoothMotion, int );
  //@}

  //@{
  /**
   * Set the length of a side of the cube (default is 1).
   */
  void SetSideLength(double);
  vtkGetMacro( SideLength, double );
  //@}

  //@{
  /**
   * Turn on/off adaptive scaling for the cube.
   */
  vtkSetMacro( AdaptiveScaling, int );
  vtkGetMacro( AdaptiveScaling, int );
  vtkBooleanMacro( AdaptiveScaling, int );
  //@}

  //@{
  /**
   * Set/Get the rescaling increment for the cube. This value is applied to
   * each dimension, so volume scaling = std::pow(RescaleFactor, 3).
   */
  vtkSetClampMacro( RescaleFactor, double, 1., VTK_DOUBLE_MAX );
  vtkGetMacro( RescaleFactor, double );
  //@}

  //@{
  /**
   * Set the min/max cube representational area relative to the render window
   * area. If adaptive scaling is on and the cube's image is outside of these
   * bounds, the cube is adaptively scaled. The max and min relative cube sizes
   * are clamped between 1. and 1.e-6, and MaxRelativeubeSize must be more than
   * <RescaleFactor> greater than MinRelativeCubeScreenArea.
   */
  void SetMinRelativeCubeScreenArea(double);
  vtkGetMacro( MinRelativeCubeScreenArea, double );
  void SetMaxRelativeCubeScreenArea(double);
  vtkGetMacro( MaxRelativeCubeScreenArea, double );
  //@}

  //@{
  /**
   * Set the label for the unit of length of a side of the cube.
   */
  vtkSetStringMacro(LengthUnit);
  vtkGetStringMacro(LengthUnit);
  //@}

protected:
  vtkMeasurementCubeHandleRepresentation3D();
  ~vtkMeasurementCubeHandleRepresentation3D() VTK_OVERRIDE;

  vtkActor                   * Actor;
  vtkPolyDataMapper          * Mapper;
  vtkTransformPolyDataFilter * HandleTransformFilter;
  vtkMatrixToLinearTransform * HandleTransform;
  vtkMatrix4x4               * HandleTransformMatrix;
  vtkCellPicker              * HandlePicker;
  double                       LastPickPosition[3];
  double                       LastEventPosition[2];
  vtkProperty                * Property;
  vtkProperty                * SelectedProperty;
  int                          WaitingForMotion;
  int                          WaitCount;
  int                          HandleVisibility;
  double                       Offset[3];
  int                          AdaptiveScaling;
  double                       RescaleFactor;
  double                       MinRelativeCubeScreenArea;
  double                       MaxRelativeCubeScreenArea;
  double                       SideLength;
  char                       * LengthUnit;

  // Register internal Pickers within PickingManager
  void RegisterPickers() VTK_OVERRIDE;

  // Methods to manipulate the cursor
  virtual void Translate(double *p1, double *p2);
  virtual void Scale(double *p1, double *p2, double eventPos[2]);
  virtual void MoveFocus(double *p1, double *p2);

  void CreateDefaultProperties();

  /**
   * If adaptive scaling is enabled, rescale the cube so that its
   * representational area in the display window falls between
   * <MinRelativeCubeScreenArea> and <MaxRelativeCubeScreenArea>.
   */
  void ScaleIfNecessary(vtkViewport*);

  /**
  * Given a motion vector defined by p1 --> p2 (p1 and p2 are in
  * world coordinates), the new display position of the handle center is
  * populated into requestedDisplayPos. This is again only a request for the
  * new display position. It is up to the point placer to deduce the
  * appropriate world co-ordinates that this display position will map into.
  * The placer may even disallow such a movement.
  * If "SmoothMotion" is OFF, the returned requestedDisplayPos is the same
  * as the event position, ie the location of the mouse cursor. If its OFF,
  * incremental offsets as described above are used to compute it.
   */
  void MoveFocusRequest( double *p1, double *p2,
                         double eventPos[2], double requestedDisplayPos[3] );

  /**
   * The handle may be scaled uniformly in all three dimensions using this
   * API. The handle can also be scaled interactively using the right
   * mouse button.
   */
  virtual void SetUniformScale( double scale );

  /**
   * Update the actor position. Different subclasses handle this differently.
   * For instance vtkPolygonalHandleRepresentation3D updates the handle
   * transformation and sets this on the handle.
   * vtkOrientedPolygonalHandleRepresentation3D, which uses a vtkFollower to
   * keep the handle geometry facinig the camera handles this differently. This
   * is an opportunity for subclasses to update the actor's position etc each
   * time the handle is rendered.
   */
  virtual void UpdateHandle();

  /**
   * Opportunity to update the label position and text during each render.
   */
  virtual void UpdateLabel();

  // Handle the label.
  int LabelVisibility;
  int SelectedLabelVisibility;
  vtkBillboardTextActor3D *LabelText;
  bool LabelAnnotationTextScaleInitialized;
  int SmoothMotion;

private:
  vtkMeasurementCubeHandleRepresentation3D(
    const vtkMeasurementCubeHandleRepresentation3D&) VTK_DELETE_FUNCTION;
  void operator=(
    const vtkMeasurementCubeHandleRepresentation3D&) VTK_DELETE_FUNCTION;
};

#endif
