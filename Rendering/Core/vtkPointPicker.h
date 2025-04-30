// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPointPicker
 * @brief   select a point by shooting a ray into a graphics window
 *
 *
 * vtkPointPicker is used to select a point by shooting a ray into a graphics
 * window and intersecting with actor's defining geometry - specifically its
 * points. Beside returning coordinates, actor, and mapper, vtkPointPicker
 * returns the id of the point projecting closest onto the ray (within the
 * specified tolerance).  Ties are broken (i.e., multiple points all
 * projecting within the tolerance along the pick ray) by choosing the point
 * closest to the ray origin (i.e., closest to the eye).
 *
 *
 * @sa
 * vtkPicker vtkCellPicker
 */

#ifndef vtkPointPicker_h
#define vtkPointPicker_h

#include "vtkPicker.h"
#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkWrappingHints.h"       // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkDataSet;

class VTKRENDERINGCORE_EXPORT VTK_MARSHALAUTO vtkPointPicker : public vtkPicker
{
public:
  ///@{
  /**
   * Standard methods for instantiation, type information, and printing.
   */
  static vtkPointPicker* New();
  vtkTypeMacro(vtkPointPicker, vtkPicker);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Get the id of the picked point. If PointId = -1, nothing was picked.
   */
  vtkGetMacro(PointId, vtkIdType);
  ///@}

  ///@{
  /**
   * Specify whether the point search should be based on cell points or
   * directly on the point list.
   */
  vtkSetMacro(UseCells, vtkTypeBool);
  vtkGetMacro(UseCells, vtkTypeBool);
  vtkBooleanMacro(UseCells, vtkTypeBool);
  ///@}

protected:
  vtkPointPicker();
  ~vtkPointPicker() override = default;

  vtkIdType PointId;    // picked point
  vtkTypeBool UseCells; // Use cell points vs. points directly

  double IntersectWithLine(const double p1[3], const double p2[3], double tol,
    vtkAssemblyPath* path, vtkProp3D* p, vtkAbstractMapper3D* m) override;
  void Initialize() override;

  vtkIdType IntersectDataSetWithLine(const double p1[3], double ray[3], double rayFactor,
    double tol, vtkDataSet* dataSet, double& tMin, double minXYZ[3]);

private:
  vtkPointPicker(const vtkPointPicker&) = delete;
  void operator=(const vtkPointPicker&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
