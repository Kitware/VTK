// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPointHandleSource
 * @brief   handle source used to represent points.
 *
 * vtkPointHandleSource is deriving vtkHandleSource interface.
 * This handle represents a point with its shape being a sphere.
 * Its center and radius can be modified.
 * If the point is also parametered by any direction, it is then
 * represented as a cone pointing in this direction.
 */

#ifndef vtkPointHandleSource_h
#define vtkPointHandleSource_h

#include "vtkFiltersSourcesModule.h" // For export macro
#include "vtkHandleSource.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkConeSource;
class vtkSphereSource;

class VTKFILTERSSOURCES_EXPORT vtkPointHandleSource : public vtkHandleSource
{
public:
  static vtkPointHandleSource* New();
  vtkTypeMacro(vtkPointHandleSource, vtkHandleSource);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/Get the position of the handle.
   * The default position is (0,0,0).
   */
  using vtkHandleSource::SetPosition;
  void SetPosition(double xPos, double yPos, double zPos) override;
  using vtkHandleSource::GetPosition;
  double* GetPosition() override;
  ///@}

  ///@{
  /**
   * Set/Get the direction of the handle.
   * The direction is used in case the handle is
   * represented as a cone (when this->Directional = true).
   * The default direction is (1,0,0).
   */
  using vtkHandleSource::SetDirection;
  void SetDirection(double xDir, double yDir, double zDir) override;
  using vtkHandleSource::GetDirection;
  double* GetDirection() override;
  ///@}

  vtkPointHandleSource(const vtkPointHandleSource&) = delete;
  void operator=(const vtkPointHandleSource&) = delete;

protected:
  vtkPointHandleSource() = default;
  ~vtkPointHandleSource() override = default;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  void RecomputeSphere();
  void RecomputeCone();

  double Position[3] = { 0, 0, 0 };
  double Direction[3] = { 1, 0, 0 };

  vtkNew<vtkSphereSource> PositionSphere;
  vtkNew<vtkConeSource> PositionCone;
};

VTK_ABI_NAMESPACE_END
#endif
