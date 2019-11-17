/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQWidgetRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkQWidgetRepresentation
 * @brief   a class defining the representation for a vtkQWidgetWidget
 *
 * This class renders a QWidget as a simple vtkPlaneSource with a vtkTexture
 * that contains a vtkQWidgetTexture which imports the OpenGL texture handle
 * from Qt into the vtk scene. Qt and VTK may need to be useing the same
 * graphics context.
 */

#ifndef vtkQWidgetRepresentation_h
#define vtkQWidgetRepresentation_h

#include "vtkGUISupportQtModule.h" // For export macro
#include "vtkWidgetRepresentation.h"

class QWidget;
class vtkActor;
class vtkCellPicker;
class vtkOpenGLTexture;
class vtkPlaneSource;
class vtkPolyDataAlgorithm;
class vtkPolyDataMapper;
class vtkQWidgetTexture;

class VTKGUISUPPORTQT_EXPORT vtkQWidgetRepresentation : public vtkWidgetRepresentation
{
public:
  /**
   * Instantiate the class.
   */
  static vtkQWidgetRepresentation* New();

  //@{
  /**
   * Standard methods for the class.
   */
  vtkTypeMacro(vtkQWidgetRepresentation, vtkWidgetRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  //@}

  /**
   * Satisfies superclass API.  This returns a pointer to the underlying
   * PolyData (which represents the plane).
   */
  vtkPolyDataAlgorithm* GetPolyDataAlgorithm();

  /**
   * Satisfies the superclass API.  This will change the state of the widget
   * to match changes that have been made to the underlying PolyDataSource
   */
  void UpdatePlacement(void);

  //@{
  /**
   * Methods to interface with the vtkImplicitPlaneWidget2.
   */
  void PlaceWidget(double bounds[6]) override;
  void BuildRepresentation() override;
  int ComputeComplexInteractionState(vtkRenderWindowInteractor* iren, vtkAbstractWidget* widget,
    unsigned long event, void* calldata, int modify = 0) override;
  //@}

  //@{
  /**
   * Methods supporting the rendering process.
   */
  double* GetBounds() VTK_SIZEHINT(6) override;
  void GetActors(vtkPropCollection* pc) override;
  void ReleaseGraphicsResources(vtkWindow*) override;
  int RenderOpaqueGeometry(vtkViewport*) override;
  int RenderTranslucentPolygonalGeometry(vtkViewport*) override;
  vtkTypeBool HasTranslucentPolygonalGeometry() override;
  //@}

  // Manage the state of the widget
  enum _InteractionState
  {
    Outside = 0,
    Inside
  };

  //@{
  /**
   * The interaction state may be set from a widget (e.g.,
   * vtkQWidgetWidget) or other object. This controls how the
   * interaction with the widget proceeds. Normally this method is used as
   * part of a handshaking process with the widget: First
   * ComputeInteractionState() is invoked that returns a state based on
   * geometric considerations (i.e., cursor near a widget feature), then
   * based on events, the widget may modify this further.
   */
  vtkSetClampMacro(InteractionState, int, Outside, Inside);
  //@}

  /**
   * Set the QWidget this representation will render
   */
  void SetWidget(QWidget* w);

  /**
   * Get the QWidgetTexture used by the representation
   */
  vtkGetObjectMacro(QWidgetTexture, vtkQWidgetTexture);

  /**
   * Get the vtkPlaneSouce used by this representation. This can be useful
   * to set the Origin, Point1, Point2 of the plane source directly.
   */
  vtkGetObjectMacro(PlaneSource, vtkPlaneSource);

  /**
   * Get the widget coordinates as computed in the last call to
   * ComputeComplexInteractionState.
   */
  vtkGetVector2Macro(WidgetCoordinates, int);

protected:
  vtkQWidgetRepresentation();
  ~vtkQWidgetRepresentation() override;

  int WidgetCoordinates[2];

  vtkPlaneSource* PlaneSource;
  vtkPolyDataMapper* PlaneMapper;
  vtkActor* PlaneActor;
  vtkOpenGLTexture* PlaneTexture;
  vtkQWidgetTexture* QWidgetTexture;

  vtkCellPicker* Picker;

  // Register internal Pickers within PickingManager
  void RegisterPickers() override;

private:
  vtkQWidgetRepresentation(const vtkQWidgetRepresentation&) = delete;
  void operator=(const vtkQWidgetRepresentation&) = delete;
};

#endif
