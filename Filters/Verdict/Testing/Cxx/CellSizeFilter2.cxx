// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCellData.h"
#include "vtkCellSizeFilter.h"
#include "vtkCellType.h"
#include "vtkCellTypeSource.h"
#include "vtkDoubleArray.h"
#include "vtkNew.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridReader.h"

#include <set>

int CellSizeFilter2(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{

  const std::set<int> SupportedCellTypes = { VTK_LINE, VTK_QUADRATIC_EDGE, VTK_CUBIC_LINE,
    VTK_LAGRANGE_CURVE, VTK_BEZIER_CURVE, VTK_TRIANGLE, VTK_QUAD, VTK_POLYGON, VTK_PIXEL,
    VTK_QUADRATIC_TRIANGLE, VTK_QUADRATIC_QUAD, VTK_BIQUADRATIC_QUAD, VTK_LAGRANGE_TRIANGLE,
    VTK_LAGRANGE_QUADRILATERAL, VTK_BEZIER_TRIANGLE, VTK_BEZIER_QUADRILATERAL, VTK_TETRA,
    VTK_HEXAHEDRON, VTK_POLYHEDRON, VTK_VOXEL, VTK_WEDGE, VTK_PYRAMID, VTK_PENTAGONAL_PRISM,
    VTK_HEXAGONAL_PRISM, VTK_QUADRATIC_TETRA, VTK_QUADRATIC_HEXAHEDRON, VTK_TRIQUADRATIC_HEXAHEDRON,
    VTK_QUADRATIC_WEDGE, VTK_QUADRATIC_PYRAMID, VTK_TRIQUADRATIC_PYRAMID, VTK_LAGRANGE_TETRAHEDRON,
    VTK_LAGRANGE_HEXAHEDRON, VTK_LAGRANGE_WEDGE, VTK_BEZIER_TETRAHEDRON, VTK_BEZIER_HEXAHEDRON,
    VTK_BEZIER_WEDGE };

  std::vector<bool> complete;
  for (const auto cellType : SupportedCellTypes)
  {

    if (cellType == VTK_TRIANGLE || cellType == VTK_LAGRANGE_TETRAHEDRON ||
      cellType == VTK_LAGRANGE_WEDGE)
    {
      complete = { false, true };
    }
    else
    {
      complete = { false };
    }
    for (size_t j = 0; j < complete.size(); j++)
    {
      vtkNew<vtkCellTypeSource> cellTypeSource;
      cellTypeSource->SetBlocksDimensions(1, 1, 1);
      cellTypeSource->SetCellOrder(2);
      cellTypeSource->SetCellType(cellType);
      cellTypeSource->SetCompleteQuadraticSimplicialElements(complete[j]);
      vtkNew<vtkCellSizeFilter> filter;
      filter->SetInputConnection(cellTypeSource->GetOutputPort());
      filter->ComputeSumOn();
      filter->Update();
      const int cellDim = vtkCellTypes::GetDimension(cellType);
      std::string sizeType = (cellDim == 1) ? "Length" : ((cellDim == 2) ? "Area" : "Volume");

      const double size =
        vtkDoubleArray::SafeDownCast(vtkUnstructuredGrid::SafeDownCast(filter->GetOutput())
                                       ->GetFieldData()
                                       ->GetArray(sizeType.c_str()))
          ->GetValue(0);

      if (fabs(size - 1.0) > .0001)
      {
        vtkGenericWarningMacro("Wrong " << sizeType << " dimension for the cell source type "
                                        << vtkCellTypes::GetClassNameFromTypeId(cellType)
                                        << " supposed to be 1.0 whereas it is " << size);
        return EXIT_FAILURE;
      }
    }
  }
  return EXIT_SUCCESS;
}
