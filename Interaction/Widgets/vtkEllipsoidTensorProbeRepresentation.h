// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkEllipsoidTensorProbeRepresentation
 * @brief   A concrete implementation of vtkTensorProbeRepresentation that renders tensors as
 * ellipoids.
 *
 * vtkEllipsoidTensorProbeRepresentation is a concrete implementation of
 * vtkTensorProbeRepresentation. It renders tensors as ellipsoids. Locations
 * between two points when probed have the tensors linearly interpolated
 * from the neighboring locations on the polyline.
 *
 * @sa
 * vtkTensorProbeWidget
 */

#ifndef vtkEllipsoidTensorProbeRepresentation_h
#define vtkEllipsoidTensorProbeRepresentation_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkTensorProbeRepresentation.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkCellPicker;
class vtkTensorGlyph;
class vtkPolyDataNormals;

class VTKINTERACTIONWIDGETS_EXPORT vtkEllipsoidTensorProbeRepresentation
  : public vtkTensorProbeRepresentation
{
public:
  static vtkEllipsoidTensorProbeRepresentation* New();

  ///@{
  /**
   * Standard methods for instances of this class.
   */
  vtkTypeMacro(vtkEllipsoidTensorProbeRepresentation, vtkTensorProbeRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  void BuildRepresentation() override;
  int RenderOpaqueGeometry(vtkViewport*) override;

  /**
   * Can we pick the tensor glyph at the current cursor pos
   */
  int SelectProbe(int pos[2]) override;

  ///@{
  /**
   * See vtkProp for details.
   */
  void GetActors(vtkPropCollection*) override;
  void ReleaseGraphicsResources(vtkWindow*) override;
  ///@}

  /*
   * Register internal Pickers within PickingManager
   */
  void RegisterPickers() override;

protected:
  vtkEllipsoidTensorProbeRepresentation();
  ~vtkEllipsoidTensorProbeRepresentation() override;

  // Get the interpolated tensor at the current position
  void EvaluateTensor(double t[9]);

  vtkActor* EllipsoidActor;
  vtkPolyDataMapper* EllipsoidMapper;
  vtkPolyData* TensorSource;
  vtkTensorGlyph* TensorGlypher;
  vtkCellPicker* CellPicker;
  vtkPolyDataNormals* PolyDataNormals;

private:
  vtkEllipsoidTensorProbeRepresentation(const vtkEllipsoidTensorProbeRepresentation&) = delete;
  void operator=(const vtkEllipsoidTensorProbeRepresentation&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
