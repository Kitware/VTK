// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOrientedGlyphFocalPlaneContourRepresentation
 * @brief   Contours constrained
 * to a focal plane.
 *
 *
 * This class is used to represent a contour drawn on the focal plane (usually
 * overlaid on top of an image or volume widget).
 * The class was written in order to be able to draw contours on a volume widget
 * and have the contours overlaid on the focal plane in order to do contour
 * segmentation.
 *
 * @sa
 * vtkOrientedGlyphContourRepresentation
 */

#ifndef vtkOrientedGlyphFocalPlaneContourRepresentation_h
#define vtkOrientedGlyphFocalPlaneContourRepresentation_h

#include "vtkFocalPlaneContourRepresentation.h"
#include "vtkInteractionWidgetsModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkProperty2D;
class vtkActor2D;
class vtkPolyDataMapper2D;
class vtkPolyData;
class vtkGlyph2D;
class vtkPoints;
class vtkPolyData;

class VTKINTERACTIONWIDGETS_EXPORT vtkOrientedGlyphFocalPlaneContourRepresentation
  : public vtkFocalPlaneContourRepresentation
{
public:
  /**
   * Instantiate this class.
   */
  static vtkOrientedGlyphFocalPlaneContourRepresentation* New();

  ///@{
  /**
   * Standard methods for instances of this class.
   */
  vtkTypeMacro(vtkOrientedGlyphFocalPlaneContourRepresentation, vtkFocalPlaneContourRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Specify the cursor shape. Keep in mind that the shape will be
   * aligned with the constraining plane by orienting it such that
   * the x axis of the geometry lies along the normal of the plane.
   */
  void SetCursorShape(vtkPolyData* cursorShape);
  vtkPolyData* GetCursorShape();
  ///@}

  ///@{
  /**
   * Specify the shape of the cursor (handle) when it is active.
   * This is the geometry that will be used when the mouse is
   * close to the handle or if the user is manipulating the handle.
   */
  void SetActiveCursorShape(vtkPolyData* activeShape);
  vtkPolyData* GetActiveCursorShape();
  ///@}

  ///@{
  /**
   * This is the property used when the handle is not active
   * (the mouse is not near the handle)
   */
  vtkGetObjectMacro(Property, vtkProperty2D);
  ///@}

  ///@{
  /**
   * This is the property used when the user is interacting
   * with the handle.
   */
  vtkGetObjectMacro(ActiveProperty, vtkProperty2D);
  ///@}

  ///@{
  /**
   * This is the property used by the lines.
   */
  vtkGetObjectMacro(LinesProperty, vtkProperty2D);
  ///@}

  ///@{
  /**
   * Subclasses of vtkOrientedGlyphFocalPlaneContourRepresentation must implement these methods.
   * These are the methods that the widget and its representation use to communicate with each
   * other.
   */
  void SetRenderer(vtkRenderer* ren) override;
  void BuildRepresentation() override;
  void StartWidgetInteraction(double eventPos[2]) override;
  void WidgetInteraction(double eventPos[2]) override;
  int ComputeInteractionState(int X, int Y, int modified = 0) override;
  ///@}

  ///@{
  /**
   * Methods to make this class behave as a vtkProp.
   */
  void GetActors2D(vtkPropCollection*) override;
  void ReleaseGraphicsResources(vtkWindow*) override;
  int RenderOverlay(vtkViewport* viewport) override;
  int RenderOpaqueGeometry(vtkViewport* viewport) override;
  int RenderTranslucentPolygonalGeometry(vtkViewport* viewport) override;
  vtkTypeBool HasTranslucentPolygonalGeometry() override;
  ///@}

  /**
   * Get the points in this contour as a vtkPolyData.
   */
  vtkPolyData* GetContourRepresentationAsPolyData() override;

  /**
   * Direction cosines of the plane on which the contour lies
   * on in world coordinates. This would be the same matrix that would be
   * set in vtkImageReslice or vtkImagePlaneWidget if there were a plane
   * passing through the contour points. The origin must be the origin of the
   * data under the contour.
   */
  vtkMatrix4x4* GetContourPlaneDirectionCosines(const double origin[3]);

protected:
  vtkOrientedGlyphFocalPlaneContourRepresentation();
  ~vtkOrientedGlyphFocalPlaneContourRepresentation() override;

  // Render the cursor
  vtkActor2D* Actor;
  vtkPolyDataMapper2D* Mapper;
  vtkGlyph2D* Glypher;
  vtkActor2D* ActiveActor;
  vtkPolyDataMapper2D* ActiveMapper;
  vtkGlyph2D* ActiveGlypher;
  vtkPolyData* CursorShape;
  vtkPolyData* ActiveCursorShape;
  vtkPolyData* FocalData;
  vtkPoints* FocalPoint;
  vtkPolyData* ActiveFocalData;
  vtkPoints* ActiveFocalPoint;

  // The polydata represents the contour in display coordinates.
  vtkPolyData* Lines;
  vtkPolyDataMapper2D* LinesMapper;
  vtkActor2D* LinesActor;

  // The polydata represents the contour in world coordinates. It is updated
  // (kept in sync with Lines) every time the GetContourRepresentationAsPolyData()
  // method is called.
  vtkPolyData* LinesWorldCoordinates;

  // Support picking
  double LastPickPosition[3];
  double LastEventPosition[2];

  // Methods to manipulate the cursor
  void Translate(double eventPos[2]);
  void Scale(double eventPos[2]);
  void ShiftContour(double eventPos[2]);
  void ScaleContour(double eventPos[2]);

  void ComputeCentroid(double* ioCentroid);

  // Properties used to control the appearance of selected objects and
  // the manipulator in general.
  vtkProperty2D* Property;
  vtkProperty2D* ActiveProperty;
  vtkProperty2D* LinesProperty;

  vtkMatrix4x4* ContourPlaneDirectionCosines;

  void CreateDefaultProperties();

  // Distance between where the mouse event happens and where the
  // widget is focused - maintain this distance during interaction.
  double InteractionOffset[2];

  void BuildLines() override;

private:
  vtkOrientedGlyphFocalPlaneContourRepresentation(
    const vtkOrientedGlyphFocalPlaneContourRepresentation&) = delete;
  void operator=(const vtkOrientedGlyphFocalPlaneContourRepresentation&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
