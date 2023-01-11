/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDGTet.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

#include "vtkDGCell.h"
#include "vtkStringToken.h" // for vtkStringToken::Hash

VTK_ABI_NAMESPACE_BEGIN
class vtkCellGrid;
class vtkDataSetAttributes;
class vtkCellAttribute;

class VTKFILTERSCELLGRID_EXPORT vtkDGTet : public vtkDGCell
{
public:
  using CellTypeId = std::size_t;
  using DOFType = vtkStringToken;
  static vtkDGTet* New();

  vtkTypeMacro(vtkDGTet, vtkDGCell);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkIdType GetNumberOfCells() override;

  Shape GetShape() const override { return Shape::Tetrahedron; }
  int GetDimension() const override { return this->Dimension; }
  int GetNumberOfCorners() const override { return 4; }
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
  static const std::array<std::array<double, 3>, 4> Parameters;
  static const std::array<std::vector<vtkIdType>, 14> Sides;
  static const std::array<int, vtkDGTet::Dimension + 1> SideOffsets;
  static const std::array<Shape, Dimension + 1> SideShapes;

protected:
  vtkDGTet();
  ~vtkDGTet() override;

private:
  vtkDGTet(const vtkDGTet&) = delete;
  void operator=(const vtkDGTet&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
