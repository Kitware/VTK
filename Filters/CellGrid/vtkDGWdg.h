// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDGWdg
 * @brief   Metadata for a discontinuous Galerkin wedge.
 *
 * Currently, only a linear shape is supported but this
 * may change to arbitrary order.
 */

#ifndef vtkDGWdg_h
#define vtkDGWdg_h

#include "vtkFiltersCellGridModule.h" // for export macro

#include "vtkDeRhamCell.h"
#include "vtkStringToken.h" // for vtkStringToken::Hash

VTK_ABI_NAMESPACE_BEGIN
class vtkCellGrid;
class vtkDataSetAttributes;
class vtkCellAttribute;

class VTKFILTERSCELLGRID_EXPORT vtkDGWdg : public vtkDeRhamCell
{
public:
  static vtkDGWdg* New();

  vtkTypeMacro(vtkDGWdg, vtkDeRhamCell);
  vtkInheritanceHierarchyOverrideMacro(vtkDGWdg);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /// Methods to query cell shape and reference parameterization.
  ///
  /// These methods do not return information about particular
  /// cells or sides specified by the cell-grid's arrays; only
  /// metadata about the cell.
  bool IsInside(const vtkVector3d& rst, double tolerance) override;
  Shape GetShape() const override { return Shape::Wedge; }
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
  static const std::array<std::array<double, 3>, 6> Parameters;
  static const std::array<std::vector<vtkIdType>, 21> Sides;
  static const std::array<std::vector<vtkIdType>, 21> SidesOfSides;
  static const std::array<int, Dimension + 3> SideOffsets;
  static const std::array<Shape, Dimension + 3> SideShapes;
  // Because wedges have sides of different types but the same dimension,
  // we store a table of the number of sides by dimension rather than computing
  // these values based on SideOffsets/SideShapes entries:
  static const std::array<int, Dimension + 1> SidesOfDimension;

protected:
  vtkDGWdg();
  ~vtkDGWdg() override;

private:
  vtkDGWdg(const vtkDGWdg&) = delete;
  void operator=(const vtkDGWdg&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
