// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDGTet
 * @brief   Metadata for a discontinuous Galerkin tetrahedron.
 *
 * Currently, only a linear shape is supported but this
 * may change to arbitrary order.
 */

#ifndef vtkDGTet_h
#define vtkDGTet_h

#include "vtkFiltersCellGridModule.h" // for export macro

#include "vtkDeRhamCell.h"
#include "vtkStringToken.h" // for vtkStringToken::Hash

VTK_ABI_NAMESPACE_BEGIN
class vtkCellGrid;
class vtkDataSetAttributes;
class vtkCellAttribute;

class VTKFILTERSCELLGRID_EXPORT vtkDGTet : public vtkDeRhamCell
{
public:
  static vtkDGTet* New();

  vtkTypeMacro(vtkDGTet, vtkDeRhamCell);
  vtkInheritanceHierarchyOverrideMacro(vtkDGTet);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /// Methods to query cell shape and reference parameterization.
  ///
  /// These methods do not return information about particular
  /// cells or sides specified by the cell-grid's arrays; only
  /// metadata about the cell.
  bool IsInside(const vtkVector3d& rst, double tolerance) override;
  Shape GetShape() const override { return Shape::Tetrahedron; }
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

  static constexpr int Dimension = 3;
  static const std::array<std::array<double, 3>, 4> Parameters;
  static const std::array<std::vector<vtkIdType>, 15> Sides;
  static const std::array<std::vector<vtkIdType>, 15> SidesOfSides;
  static const std::array<int, Dimension + 2> SideOffsets;
  static const std::array<Shape, Dimension + 2> SideShapes;

protected:
  vtkDGTet();
  ~vtkDGTet() override;

private:
  vtkDGTet(const vtkDGTet&) = delete;
  void operator=(const vtkDGTet&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
