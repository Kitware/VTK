// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkMagnifierRepresentation
 * @brief   represent a vtkBorderWidget
 *
 * This class is used to represent and render a vtkMagnifierWidget.  To use
 * this class, you need to specify a renderer in which to place the
 * magnifier, and a magnification factor. Optionally, you can specify the
 * size of the magnifier window, whether it has a border, and the particular
 * actors to render.
 *
 * @sa
 * vtkMagnifierWidget
 */

#ifndef vtkMagnifierRepresentation_h
#define vtkMagnifierRepresentation_h

#include "vtkCoordinate.h"               //Because of the viewport coordinate macro
#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkWidgetRepresentation.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkPropCollection;
class vtkPoints;
class vtkPolyData;
class vtkPolyDataMapper2D;
class vtkActor2D;
class vtkProperty2D;

class VTKINTERACTIONWIDGETS_EXPORT VTK_MARSHALAUTO vtkMagnifierRepresentation
  : public vtkWidgetRepresentation
{
public:
  /**
   * Instantiate this class.
   */
  static vtkMagnifierRepresentation* New();

  ///@{
  /**
   * Define standard methods.
   */
  vtkTypeMacro(vtkMagnifierRepresentation, vtkWidgetRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Specify the renderer viewport in which to place the magnifier.
   */
  void SetRenderer(vtkRenderer* ren) override { this->Superclass::SetRenderer(ren); }
  ///@}

  ///@{
  /**
   * Methods to control the magnification factor. The magnification factor
   * is relative to the associated renderer's camera. The bump method
   * enables small increments of magnification to be applied. If the bump
   * is positive, it increases the magnification; if negative it decreases
   * the magnification.
   */
  vtkSetClampMacro(MagnificationFactor, double, 0.001, 1000.0);
  vtkGetMacro(MagnificationFactor, double);
  ///@}

  ///@{
  /**
   * Optionally specify and maintain the list of view props (e.g., actors,
   * volumes, etc).  By default, if nothing is specified, then the view props
   * from the associated renderer are used. Note, by using view props
   * different than that of the associated renderer, it is possible to create
   * special effects and/or remove props from what is shown in the magnifier.
   */
  VTK_MARSHALEXCLUDE(VTK_MARSHAL_EXCLUDE_REASON_IS_INTERNAL)
  void AddViewProp(vtkProp*);
  vtkPropCollection* GetViewProps() { return this->Props; }
  int HasViewProp(vtkProp*);
  VTK_MARSHALEXCLUDE(VTK_MARSHAL_EXCLUDE_REASON_IS_INTERNAL)
  void RemoveViewProp(vtkProp*);
  VTK_MARSHALEXCLUDE(VTK_MARSHAL_EXCLUDE_REASON_IS_INTERNAL)
  void RemoveAllViewProps();
  ///@{

  ///@{
  /**
   * Specify the size of the magnifier viewport in pixels.
   */
  vtkSetVector2Macro(Size, int);
  vtkGetVector2Macro(Size, int);
  ///@}

  ///@{
  /**
   * Optionally specify whether a border should be drawn on the outer edge of
   * the magnifier viewport. By default this is off.
   */
  vtkSetMacro(Border, bool);
  vtkGetMacro(Border, bool);
  vtkBooleanMacro(Border, bool);
  ///@}

  ///@{
  /**
   * Specify the properties of the border.
   */
  vtkGetObjectMacro(BorderProperty, vtkProperty2D);
  ///@}

  /**
   * Define the various states that the representation can be in.
   */
  enum InteractionStateType
  {
    Invisible = 0,
    Visible
  };

  ///@{
  /**
   * Subclasses should implement these methods. See the superclasses'
   * documentation for more information.
   */
  void BuildRepresentation() override;
  void WidgetInteraction(double eventPos[2]) override;
  ///@}

  /**
   * Specify the interaction state of the widget. This is generally performed
   * by the associated vtkMagnifierWidget. (It is necessary for the widget
   * to specify the interaction state in order to remove the internal
   * magnification renderer from the render window).
   */
  vtkSetClampMacro(InteractionState, int, Invisible, Visible);

  /**
   * Provide access to the magnification renderer. This is so
   * properties like background color can be set.
   */
  vtkRenderer* GetMagnificationRenderer() { return this->MagnificationRenderer; }

  ///@{
  /**
   * These methods are necessary to make this representation behave as
   * a vtkProp.
   */
  void ReleaseGraphicsResources(vtkWindow*) override;
  int RenderOverlay(vtkViewport*) override;
  int RenderOpaqueGeometry(vtkViewport*) override;
  int RenderTranslucentPolygonalGeometry(vtkViewport*) override;
  vtkTypeBool HasTranslucentPolygonalGeometry() override;
  ///@}

  /**
   * Return the MTime of this object. It takes into account MTimes
   * of the border's property.
   */
  vtkMTimeType GetMTime() override;

protected:
  vtkMagnifierRepresentation();
  ~vtkMagnifierRepresentation() override;

  // Ivars
  double MagnificationFactor;
  vtkPropCollection* Props;
  int Size[2];
  bool Border;
  vtkProperty2D* BorderProperty;

  // The internal magnification renderer and supporting classes
  vtkRenderer* MagnificationRenderer;
  vtkCoordinate* Coordinate;
  bool InsideRenderer;

  // Border representation.
  vtkPoints* BorderPoints;
  vtkPolyData* BorderPolyData;
  vtkPolyDataMapper2D* BorderMapper;
  vtkActor2D* BorderActor;

private:
  vtkMagnifierRepresentation(const vtkMagnifierRepresentation&) = delete;
  void operator=(const vtkMagnifierRepresentation&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
