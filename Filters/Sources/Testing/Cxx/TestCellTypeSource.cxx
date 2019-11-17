/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCellTypeSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkCell.h>
#include <vtkCellData.h>
#include <vtkCellSizeFilter.h>
#include <vtkCellType.h>
#include <vtkCellTypeSource.h>
#include <vtkDataArray.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkUnstructuredGrid.h>

#include <cmath>

namespace
{
int CheckCells(int cellType, int blocksDimensions[3], int precision, int expectedNumberOfPoints,
  int expectedNumberOfCells, double* expectedSizeRange, double maxDistanceToCenter,
  double maxPolynomial)
{
  vtkNew<vtkCellTypeSource> cellSource;
  cellSource->SetBlocksDimensions(blocksDimensions);
  cellSource->SetOutputPrecision(precision);
  cellSource->SetCellType(cellType);
  cellSource->Update();
  vtkUnstructuredGrid* output = cellSource->GetOutput();
  if ((precision == vtkAlgorithm::SINGLE_PRECISION &&
        output->GetPoints()->GetDataType() != VTK_FLOAT) ||
    (precision == vtkAlgorithm::DOUBLE_PRECISION &&
      output->GetPoints()->GetDataType() != VTK_DOUBLE))
  {
    cerr << "Wrong points precision\n";
    return EXIT_FAILURE;
  }
  if (output->GetCellType(0) != cellType)
  {
    cerr << "Wrong cell type\n";
    return EXIT_FAILURE;
  }
  if (output->GetNumberOfPoints() != expectedNumberOfPoints)
  {
    cerr << "Expected " << expectedNumberOfPoints << " points but got "
         << output->GetNumberOfPoints() << endl;
    return EXIT_FAILURE;
  }
  if (output->GetNumberOfCells() != expectedNumberOfCells)
  {
    cerr << "Expected " << expectedNumberOfCells << " cells but got " << output->GetNumberOfCells()
         << endl;
    return EXIT_FAILURE;
  }

  // test the field output
  double fieldRange[2];
  output->GetPointData()->GetArray("DistanceToCenter")->GetRange(fieldRange);
  if (std::abs(fieldRange[1] - maxDistanceToCenter) > .0001)
  {
    cerr << "Expected DistanceToCenter max value of " << maxDistanceToCenter << " but got "
         << fieldRange[1] << endl;
    return EXIT_FAILURE;
  }

  output->GetPointData()->GetArray("Polynomial")->GetRange(fieldRange);
  if (std::abs(fieldRange[1] - maxPolynomial) > .0001)
  {
    cerr << "Expected Polynomial max value of " << maxPolynomial << " but got " << fieldRange[1]
         << endl;
    return EXIT_FAILURE;
  }

  if (expectedSizeRange)
  {
    vtkNew<vtkCellSizeFilter> cellSize;
    cellSize->SetInputConnection(cellSource->GetOutputPort());
    cellSize->ComputeVolumeOn();
    cellSize->Update();
    output = vtkUnstructuredGrid::SafeDownCast(cellSize->GetOutput());
    std::string arrayName;
    switch (output->GetCell(0)->GetCellDimension())
    {
      case 0:
        arrayName = "VertexCount";
        break;
      case 1:
        arrayName = "Length";
        break;
      case 2:
        arrayName = "Area";
        break;
      default:
        arrayName = "Volume";
    }
    double sizeRange[2];
    output->GetCellData()->GetArray(arrayName.c_str())->GetRange(sizeRange);
    if (std::abs(sizeRange[0] - expectedSizeRange[0]) > .0001 ||
      std::abs(sizeRange[1] - expectedSizeRange[1]) > .0001)
    {
      cerr << "Expected size range of " << expectedSizeRange[0] << " to " << expectedSizeRange[1]
           << " but got " << sizeRange[0] << " to " << sizeRange[1] << endl;
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
}

int TestCellTypeSource(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  int dims[3] = { 4, 5, 6 };
  double size[2] = { 1, 1 };
  // test 1D cells
  if (CheckCells(VTK_LINE, dims, vtkAlgorithm::SINGLE_PRECISION, dims[0] + 1, dims[0], size, 2.,
        5.) == EXIT_FAILURE)
  {
    cerr << "Error with VTK_LINE\n";
    return EXIT_FAILURE;
  }
  if (CheckCells(VTK_QUADRATIC_EDGE, dims, vtkAlgorithm::SINGLE_PRECISION, dims[0] * 2 + 1, dims[0],
        size, 2., 5.) == EXIT_FAILURE)
  {
    cerr << "Error with VTK_QUADRATIC_LINE\n";
    return EXIT_FAILURE;
  }
  if (CheckCells(VTK_CUBIC_LINE, dims, vtkAlgorithm::SINGLE_PRECISION, dims[0] * 3 + 1, dims[0],
        size, 2., 5.) == EXIT_FAILURE)
  {
    cerr << "Error with VTK_CUBIC_EDGE\n";
    return EXIT_FAILURE;
  }

  // test 2D cells
  size[0] = size[1] = .5;
  if (CheckCells(VTK_TRIANGLE, dims, vtkAlgorithm::DOUBLE_PRECISION, (dims[0] + 1) * (dims[1] + 1),
        dims[0] * dims[1] * 2, size, 3.2015621187164243, 10.) == EXIT_FAILURE)
  {
    cerr << "Error with VTK_TRIANGLE\n";
    return EXIT_FAILURE;
  }
  if (CheckCells(VTK_QUADRATIC_TRIANGLE, dims, vtkAlgorithm::DOUBLE_PRECISION,
        (dims[0] * 2 + 1) * (dims[1] * 2 + 1), dims[0] * dims[1] * 2, size, 3.2015621187164243,
        10.) == EXIT_FAILURE)
  {
    cerr << "Error with VTK_QUADRATIC_TRIANGLE\n";
    return EXIT_FAILURE;
  }
  size[0] = size[1] = 1;
  if (CheckCells(VTK_QUAD, dims, vtkAlgorithm::DOUBLE_PRECISION, (dims[0] + 1) * (dims[1] + 1),
        dims[0] * dims[1], size, 3.2015621187164243, 10.) == EXIT_FAILURE)
  {
    cerr << "Error with VTK_QUAD\n";
    return EXIT_FAILURE;
  }
  if (CheckCells(VTK_QUADRATIC_QUAD, dims, vtkAlgorithm::DOUBLE_PRECISION,
        (dims[0] * 2 + 1) * (dims[1] * 2 + 1) - dims[0] * dims[1], dims[0] * dims[1], size,
        3.2015621187164243, 10.) == EXIT_FAILURE)
  {
    cerr << "Error with VTK_QUADRATIC_QUAD\n";
    return EXIT_FAILURE;
  }

  // test 3D cells
  size[0] = size[1] = 1. / 12.;
  if (CheckCells(VTK_TETRA, dims, vtkAlgorithm::DOUBLE_PRECISION,
        (dims[0] + 1) * (dims[1] + 1) * (dims[2] + 1) + dims[0] * dims[1] * dims[2],
        dims[0] * dims[1] * dims[2] * 12, size, 4.387482193696061, 16.) == EXIT_FAILURE)
  {
    cerr << "Error with VTK_TETRA\n";
    return EXIT_FAILURE;
  }
  if (CheckCells(VTK_QUADRATIC_TETRA, dims, vtkAlgorithm::DOUBLE_PRECISION, 2247,
        dims[0] * dims[1] * dims[2] * 12, size, 4.387482193696061, 16.) == EXIT_FAILURE)
  {
    cerr << "Error with VTK_QUADRATIC_TETRA\n";
    return EXIT_FAILURE;
  }
  size[0] = size[1] = 1.;
  if (CheckCells(VTK_HEXAHEDRON, dims, vtkAlgorithm::DOUBLE_PRECISION,
        (dims[0] + 1) * (dims[1] + 1) * (dims[2] + 1), dims[0] * dims[1] * dims[2], size,
        4.387482193696061, 16.) == EXIT_FAILURE)
  {
    cerr << "Error with VTK_HEXAHEDRON\n";
    return EXIT_FAILURE;
  }
  if (CheckCells(VTK_QUADRATIC_HEXAHEDRON, dims, vtkAlgorithm::DOUBLE_PRECISION, 733,
        dims[0] * dims[1] * dims[2], size, 4.387482193696061, 16.) == EXIT_FAILURE)
  {
    cerr << "Error with VTK_QUADRATIC_HEXAHEDRON\n";
    return EXIT_FAILURE;
  }
  size[0] = size[1] = .5;
  if (CheckCells(VTK_WEDGE, dims, vtkAlgorithm::DOUBLE_PRECISION,
        (dims[0] + 1) * (dims[1] + 1) * (dims[2] + 1), dims[0] * dims[1] * dims[2] * 2, size,
        4.387482193696061, 16.) == EXIT_FAILURE)
  {
    cerr << "Error with VTK_WEDGE\n";
    return EXIT_FAILURE;
  }
  if (CheckCells(VTK_QUADRATIC_WEDGE, dims, vtkAlgorithm::DOUBLE_PRECISION,
        733 + dims[0] * dims[1] * (dims[2] + 1), dims[0] * dims[1] * dims[2] * 2, size,
        4.387482193696061, 16.) == EXIT_FAILURE)
  {
    cerr << "Error with VTK_QUADRATIC_WEDGE\n";
    return EXIT_FAILURE;
  }
  size[0] = size[1] = 1. / 6.;
  if (CheckCells(VTK_PYRAMID, dims, vtkAlgorithm::DOUBLE_PRECISION,
        (dims[0] + 1) * (dims[1] + 1) * (dims[2] + 1) + dims[0] * dims[1] * dims[2],
        dims[0] * dims[1] * dims[2] * 6, size, 4.387482193696061, 16.) == EXIT_FAILURE)
  {
    cerr << "Error with VTK_PYRAMID\n";
    return EXIT_FAILURE;
  }
  if (CheckCells(VTK_QUADRATIC_PYRAMID, dims, vtkAlgorithm::DOUBLE_PRECISION,
        733 + 9 * dims[0] * dims[1] * dims[2], dims[0] * dims[1] * dims[2] * 6, size,
        4.387482193696061, 16.) == EXIT_FAILURE)
  {
    cerr << "Error with VTK_QUADRATIC_PYRAMID\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
