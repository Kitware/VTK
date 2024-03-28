// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkTensorProbeRepresentation
 * @brief   Abstract class that serves as a representation for vtkTensorProbeWidget
 *
 * The class serves as an abstract geometrical representation for the
 * vtkTensorProbeWidget. It is left to the concrete implementation to render
 * the tensors as it desires. For instance,
 * vtkEllipsoidTensorProbeRepresentation renders the tensors as ellipsoids.
 *
 * @sa
 * vtkTensorProbeWidget
 */

#ifndef vtkTensorProbeRepresentation_h
#define vtkTensorProbeRepresentation_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkWidgetRepresentation.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkActor;
class vtkPolyData;
class vtkPolyDataMapper;
class vtkGenericCell;

class VTKINTERACTIONWIDGETS_EXPORT VTK_MARSHALAUTO vtkTensorProbeRepresentation
  : public vtkWidgetRepresentation
{
public:
  ///@{
  /**
   * Standard methods for instances of this class.
   */
  vtkTypeMacro(vtkTensorProbeRepresentation, vtkWidgetRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * See vtkWidgetRepresentation for details.
   */
  void BuildRepresentation() override;
  int RenderOpaqueGeometry(vtkViewport*) override;
  ///@}

  ///@{
  /**
   * Set the position of the Tensor probe.
   */
  vtkSetVector3Macro(ProbePosition, double);
  vtkGetVector3Macro(ProbePosition, double);
  vtkSetMacro(ProbeCellId, vtkIdType);
  vtkGetMacro(ProbeCellId, vtkIdType);
  ///@}

  /**
   * Set the trajectory that we are trying to probe tensors on
   */
  virtual void SetTrajectory(vtkPolyData*);

  /**
   * Set the probe position to a reasonable location on the trajectory.
   */
  void Initialize();

  /**
   * This method is invoked by the widget during user interaction.
   * Can we pick the tensor glyph at the current cursor pos
   */
  virtual int SelectProbe(int pos[2]) = 0;

  /**
   * INTERNAL - Do not use
   * This method is invoked by the widget during user interaction.
   * Move probe based on the position and the motion vector.
   */
  virtual int Move(double motionVector[2]);

  ///@{
  /**
   * See vtkProp for details.
   */
  void GetActors(vtkPropCollection*) override;
  void ReleaseGraphicsResources(vtkWindow*) override;
  ///@}

protected:
  vtkTensorProbeRepresentation();
  ~vtkTensorProbeRepresentation() override;

  void FindClosestPointOnPolyline(
    double displayPos[2], double closestWorldPos[3], vtkIdType& cellId, int maxSpeed = 10);

  vtkActor* TrajectoryActor;
  vtkPolyDataMapper* TrajectoryMapper;
  vtkPolyData* Trajectory;
  double ProbePosition[3];
  vtkIdType ProbeCellId;

private:
  vtkTensorProbeRepresentation(const vtkTensorProbeRepresentation&) = delete;
  void operator=(const vtkTensorProbeRepresentation&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
