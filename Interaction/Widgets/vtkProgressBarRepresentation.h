// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkProgressBarRepresentation
 * @brief   represent a vtkProgressBarWidget
 *
 * This class is used to represent a vtkProgressBarWidget.
 *
 * @sa
 * vtkProgressBarWidget
 */

#ifndef vtkProgressBarRepresentation_h
#define vtkProgressBarRepresentation_h

#include "vtkBorderRepresentation.h"
#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkWrappingHints.h"            // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkActor2D;
class vtkPoints;
class vtkPolyData;
class vtkProperty2D;
class vtkUnsignedCharArray;

class VTKINTERACTIONWIDGETS_EXPORT VTK_MARSHALAUTO vtkProgressBarRepresentation
  : public vtkBorderRepresentation
{
public:
  /**
   * Instantiate this class.
   */
  static vtkProgressBarRepresentation* New();

  ///@{
  /**
   * Standard VTK class methods.
   */
  vtkTypeMacro(vtkProgressBarRepresentation, vtkBorderRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * By obtaining this property you can specify the properties of the
   * representation.
   */
  vtkGetObjectMacro(Property, vtkProperty2D);
  ///@}

  ///@{
  /**
   * Set/Get the progress rate of the progress bar, between 0 and 1
   * default is 0
   */
  vtkSetClampMacro(ProgressRate, double, 0, 1);
  vtkGetMacro(ProgressRate, double);
  ///@}

  ///@{
  /**
   * Set/Get the padding between the border and the progressbar.
   * The padding is expressed in percentage of the border box size
   * default is 0.017,0.1
   */
  vtkSetVector2Macro(Padding, double);
  vtkGetVector2Macro(Padding, double);
  ///@}

  ///@{
  /**
   * Set/Get frame visibility
   * default is on
   */
  vtkSetMacro(DrawFrame, bool);
  vtkGetMacro(DrawFrame, bool);
  vtkBooleanMacro(DrawFrame, bool);
  ///@}

  ///@{
  /**
   * Set/Get the progress bar color
   * Default is pure green
   */
  vtkSetVector3Macro(ProgressBarColor, double);
  vtkGetVector3Macro(ProgressBarColor, double);
  ///@}

  ///@{
  /**
   * Set/Get the background color
   * Default is white
   */
  vtkSetVector3Macro(BackgroundColor, double);
  vtkGetVector3Macro(BackgroundColor, double);
  ///@}

  ///@{
  /**
   * Set/Get background visibility
   * Default is off
   */
  vtkSetMacro(DrawBackground, bool);
  vtkGetMacro(DrawBackground, bool);
  vtkBooleanMacro(DrawBackground, bool);
  ///@}

  ///@{
  /**
   * Satisfy the superclasses' API.
   */
  void BuildRepresentation() override;
  ///@}

  ///@{
  /**
   * These methods are necessary to make this representation behave as
   * a vtkProp.
   */
  void GetActors2D(vtkPropCollection*) override;
  void ReleaseGraphicsResources(vtkWindow*) override;
  int RenderOverlay(vtkViewport*) override;
  int RenderOpaqueGeometry(vtkViewport*) override;
  int RenderTranslucentPolygonalGeometry(vtkViewport*) override;
  vtkTypeBool HasTranslucentPolygonalGeometry() override;
  ///@}

protected:
  vtkProgressBarRepresentation();
  ~vtkProgressBarRepresentation() override;

  double ProgressRate;
  double ProgressBarColor[3];
  double BackgroundColor[3];
  double Padding[2];
  bool DrawBackground;
  bool DrawFrame;

  vtkPoints* Points;
  vtkUnsignedCharArray* ProgressBarData;
  vtkProperty2D* Property;
  vtkActor2D* Actor;
  vtkActor2D* FrameActor;
  vtkActor2D* BackgroundActor;

private:
  vtkProgressBarRepresentation(const vtkProgressBarRepresentation&) = delete;
  void operator=(const vtkProgressBarRepresentation&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
