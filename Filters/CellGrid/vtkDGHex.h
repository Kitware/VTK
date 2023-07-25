// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDGHex
 * @brief   Metadata for a discontinuous Galerkin hexahedron.
 *
 * Currently, only a linear shape is supported but this
 * may change to arbitrary order.
 */

#ifndef vtkDGHex_h
#define vtkDGHex_h

#include "vtkFiltersCellGridModule.h" // for export macro

#include "vtkDGCell.h"
#include "vtkStringToken.h" // for vtkStringToken::Hash

VTK_ABI_NAMESPACE_BEGIN
class vtkCellGrid;
class vtkDataSetAttributes;
class vtkCellAttribute;

class VTKFILTERSCELLGRID_EXPORT vtkDGHex : public vtkDGCell
{
public:
  static vtkDGHex* New();

  vtkTypeMacro(vtkDGHex, vtkDGCell);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkIdType GetNumberOfCells() override;

  Shape GetShape() const override { return Shape::Hexahedron; }
  int GetDimension() const override { return vtkDGHex::Dimension; }
  const std::array<double, 3>& GetCornerParameter(int corner) const override;
  int GetNumberOfSideTypes() const override;
  std::pair<int, int> GetSideRangeForType(int sideType) const override;
  int GetNumberOfSidesOfDimension(int dimension) const override;
  const std::vector<vtkIdType>& GetSideConnectivity(int side) const override;
  Shape GetSideShape(int side) const override;

  vtkTypeFloat32Array* GetReferencePoints() const override;
  vtkTypeInt32Array* GetSideConnectivity() const override;
  vtkTypeInt32Array* GetSideOffsetsAndShapes() const override;

  static constexpr int Dimension = 3;
  static const std::array<std::array<double, 3>, 8> Parameters;
  static const std::array<std::vector<vtkIdType>, 26> Sides;
  static const std::array<int, Dimension + 1> SideOffsets;
  static const std::array<Shape, Dimension + 1> SideShapes;

protected:
  vtkDGHex();
  ~vtkDGHex() override;

private:
  vtkDGHex(const vtkDGHex&) = delete;
  void operator=(const vtkDGHex&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
