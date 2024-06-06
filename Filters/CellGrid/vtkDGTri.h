// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDGTri
 * @brief   Metadata for a discontinuous Galerkin triangle.
 *
 * Currently, only a linear shape is supported but this
 * may change to arbitrary order.
 */

#ifndef vtkDGTri_h
#define vtkDGTri_h

#include "vtkFiltersCellGridModule.h" // for export macro

#include "vtkDeRhamCell.h"
#include "vtkStringToken.h" // for vtkStringToken::Hash

VTK_ABI_NAMESPACE_BEGIN
class vtkCellGrid;
class vtkDataSetAttributes;
class vtkCellAttribute;

class VTKFILTERSCELLGRID_EXPORT vtkDGTri : public vtkDeRhamCell
{
public:
  static vtkDGTri* New();

  vtkTypeMacro(vtkDGTri, vtkDeRhamCell);
  vtkInheritanceHierarchyOverrideMacro(vtkDGTri);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /// Methods to query cell shape and reference parameterization.
  ///
  /// These methods do not return information about particular
  /// cells or sides specified by the cell-grid's arrays; only
  /// metadata about the cell.
  bool IsInside(const vtkVector3d& rst, double tolerance) override;
  Shape GetShape() const override { return Shape::Triangle; }
  int GetDimension() const override { return Dimension; }
  const std::array<double, 3>& GetCornerParameter(int corner) const override;
  int GetNumberOfSideTypes() const override;
  std::pair<int, int> GetSideRangeForType(int sideType) const override;
  int GetNumberOfSidesOfDimension(int dimension) const override;
  const std::vector<vtkIdType>& GetSideConnectivity(int side) const override;
  const std::vector<vtkIdType>& GetSidesOfSide(int side) const override;
  Shape GetSideShape(int side) const override;
  ///@}

  vtkTypeFloat32Array* GetReferencePoints() const override;
  vtkTypeInt32Array* GetSideConnectivity() const override;
  vtkTypeInt32Array* GetSideOffsetsAndShapes() const override;

  static constexpr int Dimension = 2;
  static const std::array<std::array<double, 3>, 3> Parameters;
  static const std::array<std::vector<vtkIdType>, 7> Sides;
  static const std::array<std::vector<vtkIdType>, 7> SidesOfSides;
  static const std::array<int, Dimension + 2> SideOffsets;
  static const std::array<Shape, Dimension + 2> SideShapes;

protected:
  vtkDGTri();
  ~vtkDGTri() override;

private:
  vtkDGTri(const vtkDGTri&) = delete;
  void operator=(const vtkDGTri&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
