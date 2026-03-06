// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCellData.h"
#include "vtkCellSizeFilter.h"
#include "vtkCellType.h"
#include "vtkCellTypeSource.h"
#include "vtkCellTypeUtilities.h"
#include "vtkDoubleArray.h"
#include "vtkMathUtilities.h"
#include "vtkNew.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"

#include <set>

bool CheckVolume(vtkUnstructuredGrid* grid, int cellType, double expectedSize)
{
  vtkNew<vtkCellSizeFilter> filter;
  filter->SetInputData(grid);
  filter->ComputeSumOn();
  filter->Update();
  const int cellDim = vtkCellTypeUtilities::GetDimension(cellType);
  std::string sizeType;
  switch (cellDim)
  {
    case 1:
      sizeType = "Length";
      break;
    case 2:
      sizeType = "Area";
      break;
    default:
      sizeType = "Volume";
      break;
  }

  const double size =
    vtkDoubleArray::SafeDownCast(vtkUnstructuredGrid::SafeDownCast(filter->GetOutput())
                                   ->GetFieldData()
                                   ->GetArray(sizeType.c_str()))
      ->GetValue(0);

  if (!vtkMathUtilities::NearlyEqual(size, expectedSize, 1e-7))
  {
    vtkGenericWarningMacro("Wrong " << sizeType << " dimension for the cell source type "
                                    << vtkCellTypeUtilities::GetClassNameFromTypeId(cellType)
                                    << " supposed to be " << expectedSize << " whereas it is "
                                    << size);
    return false;
  }
  return true;
}

int CellSizeFilter2(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  const std::set<int> cellTypeSourceSupportedCellTypes = { VTK_LINE, VTK_QUADRATIC_EDGE,
    VTK_CUBIC_LINE, VTK_LAGRANGE_CURVE, VTK_BEZIER_CURVE, VTK_TRIANGLE, VTK_QUAD, VTK_POLYGON,
    VTK_PIXEL, VTK_QUADRATIC_TRIANGLE, VTK_QUADRATIC_QUAD, VTK_BIQUADRATIC_QUAD,
    VTK_LAGRANGE_TRIANGLE, VTK_LAGRANGE_QUADRILATERAL, VTK_BEZIER_TRIANGLE,
    VTK_BEZIER_QUADRILATERAL, VTK_TETRA, VTK_HEXAHEDRON, VTK_POLYHEDRON, VTK_VOXEL, VTK_WEDGE,
    VTK_PYRAMID, VTK_PENTAGONAL_PRISM, VTK_HEXAGONAL_PRISM, VTK_QUADRATIC_TETRA,
    VTK_QUADRATIC_HEXAHEDRON, VTK_TRIQUADRATIC_HEXAHEDRON, VTK_QUADRATIC_WEDGE,
    VTK_QUADRATIC_PYRAMID, VTK_TRIQUADRATIC_PYRAMID, VTK_LAGRANGE_TETRAHEDRON,
    VTK_LAGRANGE_HEXAHEDRON, VTK_LAGRANGE_WEDGE, VTK_BEZIER_TETRAHEDRON, VTK_BEZIER_HEXAHEDRON,
    VTK_BEZIER_WEDGE };

  // check volume with cell type source for all supported cell types.
  for (const auto cellType : cellTypeSourceSupportedCellTypes)
  {
    for (bool complete : { false, true })
    {
      vtkNew<vtkCellTypeSource> cellTypeSource;
      cellTypeSource->SetBlocksDimensions(1, 1, 1);
      cellTypeSource->SetCellOrder(2);
      cellTypeSource->SetCellType(cellType);
      cellTypeSource->SetCompleteQuadraticSimplicialElements(complete);
      cellTypeSource->Update();
      auto source = cellTypeSource->GetOutput();
      if (!CheckVolume(source, cellType, 1.))
      {
        return EXIT_FAILURE;
      }
    }
  }
  const std::set<int> pcoordsSupportedCellTypes = { VTK_LINE, VTK_TRIANGLE, VTK_QUAD, VTK_TETRA,
    VTK_HEXAHEDRON, VTK_WEDGE, VTK_PYRAMID, VTK_PENTAGONAL_PRISM, VTK_HEXAGONAL_PRISM,
    VTK_QUADRATIC_EDGE, VTK_QUADRATIC_TRIANGLE, VTK_QUADRATIC_QUAD, VTK_QUADRATIC_TETRA,
    VTK_QUADRATIC_HEXAHEDRON, VTK_QUADRATIC_WEDGE, VTK_QUADRATIC_PYRAMID, VTK_BIQUADRATIC_QUAD,
    VTK_TRIQUADRATIC_HEXAHEDRON, VTK_TRIQUADRATIC_PYRAMID, VTK_QUADRATIC_LINEAR_QUAD,
    VTK_QUADRATIC_LINEAR_WEDGE, VTK_BIQUADRATIC_QUADRATIC_WEDGE,
    VTK_BIQUADRATIC_QUADRATIC_HEXAHEDRON, VTK_BIQUADRATIC_TRIANGLE, VTK_CUBIC_LINE };

  // check volume by creating a mesh with parametric coordinates.
  vtkNew<vtkGenericCell> cell;
  for (const auto cellType : pcoordsSupportedCellTypes)
  {
    cell->SetCellType(cellType);
    // get parametric coordinates
    const double* pcoords = cell->GetParametricCoords();
    // skip cells that don't have parametric coordinates or their dimension is 0
    const int dim = cell->GetCellDimension();
    // create points
    vtkNew<vtkPoints> points;
    for (int i = 0; i < cell->GetNumberOfPoints(); ++i)
    {
      points->InsertNextPoint(pcoords + 3 * i);
    }
    // create cell point ids
    vtkNew<vtkIdList> pointIds;
    for (int i = 0; i < cell->GetNumberOfPoints(); ++i)
    {
      pointIds->InsertNextId(i);
    }
    // create unstructured grid with a single cell
    vtkNew<vtkUnstructuredGrid> grid;
    grid->SetPoints(points);
    grid->InsertNextCell(cellType, pointIds);
    // set expected size based on the cell type and dimension.
    double expectedSize;
    switch (dim)
    {
      case 1:
        switch (cellType)
        {
          case VTK_CUBIC_LINE:
            expectedSize = 2.0;
            break;
          default:
            expectedSize = 1.0;
            break;
        }
        break;
      case 2:
        switch (cellType)
        {
          case VTK_TRIANGLE:
          case VTK_QUADRATIC_TRIANGLE:
          case VTK_BIQUADRATIC_TRIANGLE:
            expectedSize = 0.5;
            break;
          default:
            expectedSize = 1.;
            break;
        }
        break;
      case 3:
      default:
        switch (cellType)
        {
          case VTK_TETRA:
          case VTK_QUADRATIC_TETRA:
            expectedSize = 1.0 / 6.0;
            break;
          case VTK_WEDGE:
          case VTK_QUADRATIC_WEDGE:
          case VTK_QUADRATIC_LINEAR_WEDGE:
          case VTK_BIQUADRATIC_QUADRATIC_WEDGE:
            expectedSize = 0.5;
            break;
          case VTK_PYRAMID:
            expectedSize = 1.0 / 3.0;
            break;
          case VTK_QUADRATIC_PYRAMID:
            expectedSize = 2.0 / 3.0; // the pcoords of the mid-edge points are skewed
            break;
          case VTK_TRIQUADRATIC_PYRAMID:
            expectedSize =
              1.0 / 6.0; // the top of the pyramid is at half the height of the linear pyramid
            break;
          case VTK_PENTAGONAL_PRISM:
            expectedSize = (5.0 / 2.0) * (0.5 * 0.5) * std::sin(2 * vtkMath::Pi() / 5.0);
            break;
          case VTK_HEXAGONAL_PRISM:
            expectedSize = 3.0 * (0.5 * 0.5) * std::sin(vtkMath::Pi() / 3.0);
            break;
          default:
            expectedSize = 1.0;
            break;
        }
        break;
    }
    if (!CheckVolume(grid, cellType, expectedSize))
    {
      return EXIT_FAILURE;
    }
  }
  return EXIT_SUCCESS;
}
