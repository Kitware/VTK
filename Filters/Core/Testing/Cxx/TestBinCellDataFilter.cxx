// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// This tests the vtkBinCellDataFilter class.

#include <vtkBinCellDataFilter.h>
#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkCellIterator.h>
#include <vtkCellLocator.h>
#include <vtkCleanPolyData.h>
#include <vtkDelaunay3D.h>
#include <vtkDoubleArray.h>
#include <vtkGenericCell.h>
#include <vtkIdTypeArray.h>
#include <vtkMersenneTwister.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkPointSource.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#include <vtkUnstructuredGrid.h>

#include <iostream>

vtkSmartPointer<vtkUnstructuredGrid> ReorderCells(vtkUnstructuredGrid* mesh, vtkIdType targetIds[])
{
  vtkNew<vtkCellArray> reorderedCells;
  vtkNew<vtkUnsignedCharArray> reorderedCellTypes;
  for (vtkIdType i = 0; i < mesh->GetNumberOfCells(); ++i)
  {
    reorderedCells->InsertNextCell(mesh->GetCell(targetIds[i])->GetPointIds());
    reorderedCellTypes->InsertNextValue(mesh->GetCellType(targetIds[i]));
  }
  vtkNew<vtkUnstructuredGrid> reorderedMesh;
  reorderedMesh->DeepCopy(mesh);
  reorderedMesh->SetCells(reorderedCellTypes, reorderedCells);
  reorderedMesh->Modified();
  return reorderedMesh;
}

vtkSmartPointer<vtkUnstructuredGrid> ConstructDelaunay3DSphere(
  vtkIdType numberOfPoints, vtkMersenneTwister* seq, bool sampleShellOnly)
{
  // This function constructs a tetrahedrally meshed sphere by first generating
  // <numberOfPoints> points randomly placed within a unit sphere, then removing
  // points that overlap within a tolerance, and finally constructing a delaunay
  // 3d tetrahedralization from the points. Additionally, cell data
  // corresponding to the cell center's distance from the origin are added to
  // this data. If <sampleShellOnly> is true, the original point sampling is
  // performed on the shell of the unit sphere.

  // Generate points within a unit sphere centered at the origin.
  vtkSmartPointer<vtkPointSource> source = vtkSmartPointer<vtkPointSource>::New();
  source->SetNumberOfPoints(numberOfPoints);
  source->SetCenter(0., 0., 0.);
  source->SetRadius(1.);
  source->SetDistributionToUniform();
  source->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);
  source->SetRandomSequence(seq);
  if (sampleShellOnly)
  {
    source->SetDistributionToShell();
  }

  // Clean the polydata. This will remove overlapping points that may be
  // present in the input data.
  vtkSmartPointer<vtkCleanPolyData> cleaner = vtkSmartPointer<vtkCleanPolyData>::New();
  cleaner->SetInputConnection(source->GetOutputPort());

  // Generate a tetrahedral mesh from the input points. By
  // default, the generated volume is the convex hull of the points.
  vtkSmartPointer<vtkDelaunay3D> delaunay3D = vtkSmartPointer<vtkDelaunay3D>::New();
  delaunay3D->SetInputConnection(cleaner->GetOutputPort());
  delaunay3D->Update();

  // Create cell data for use in binning.
  vtkUnstructuredGrid* ug = delaunay3D->GetOutput();
  vtkSmartPointer<vtkDoubleArray> radius = vtkSmartPointer<vtkDoubleArray>::New();
  radius->SetName("radius");
  radius->SetNumberOfComponents(1);
  radius->SetNumberOfTuples(ug->GetNumberOfCells());

  double weights[VTK_CELL_SIZE];
  double pcoords[3], coords[3];
  int subId;
  double r;
  vtkNew<vtkGenericCell> cell;
  vtkCellIterator* it = ug->NewCellIterator();
  for (it->InitTraversal(); !it->IsDoneWithTraversal(); it->GoToNextCell())
  {
    it->GetCell(cell);
    cell->GetParametricCenter(pcoords);
    cell->EvaluateLocation(subId, pcoords, coords, weights);

    r = std::sqrt(coords[0] * coords[0] + coords[1] * coords[1] + coords[2] * coords[2]);
    radius->SetTypedTuple(it->GetCellId(), &r);
  }
  it->Delete();

  ug->GetCellData()->SetScalars(radius);

  return delaunay3D->GetOutput();
}

int TestBinCellDataFilter(int, char*[])
{
  // This test constructs two 3d tetrahedral meshes of a unit sphere (a fine
  // source and a course input mesh) with cell data associated with the distance
  // of each cell to the origin. The cell data from the source mesh is then
  // binned within each cell of the input mesh, and the resulting binned values
  // are compared against precomputed expected values.

  vtkNew<vtkMersenneTwister> seq;
  seq->InitializeSequence(0, 0);

  constexpr vtkIdType numberOfSourcePoints = 1.e4;
  constexpr vtkIdType numberOfInputPoints = 1.e1;

  vtkSmartPointer<vtkUnstructuredGrid> sourceGrid =
    ConstructDelaunay3DSphere(numberOfSourcePoints, seq, false);
  vtkSmartPointer<vtkUnstructuredGrid> inputGrid =
    ConstructDelaunay3DSphere(numberOfInputPoints, seq, true);

  vtkNew<vtkCellLocator> locator;

  vtkNew<vtkBinCellDataFilter> binDataFilter;
  binDataFilter->SetSourceData(sourceGrid);
  binDataFilter->SetCellLocator(locator);
  binDataFilter->SetComputeTolerance(false);
  binDataFilter->GenerateValues(3, .2, .8);

  vtkIdType cellPermutation[10][18] = { { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
                                          17 },
    { 10, 6, 11, 14, 3, 13, 8, 0, 2, 7, 15, 9, 17, 16, 5, 4, 1, 12 },
    { 3, 9, 4, 8, 0, 15, 13, 5, 10, 6, 11, 16, 2, 12, 1, 7, 14, 17 },
    { 14, 2, 12, 13, 11, 17, 6, 3, 7, 4, 8, 5, 9, 1, 15, 0, 10, 16 },
    { 1, 4, 8, 6, 10, 9, 5, 12, 11, 3, 17, 0, 7, 13, 2, 14, 16, 15 },
    { 14, 17, 11, 9, 13, 4, 16, 8, 3, 10, 7, 0, 15, 1, 6, 2, 5, 12 },
    { 0, 5, 14, 11, 3, 6, 2, 17, 4, 16, 9, 1, 13, 10, 7, 15, 12, 8 },
    { 17, 2, 7, 11, 0, 1, 12, 8, 6, 15, 5, 16, 10, 9, 14, 3, 13, 4 },
    { 2, 9, 4, 8, 0, 15, 7, 17, 16, 3, 14, 13, 1, 12, 10, 5, 11, 6 },
    { 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 } };
  for (unsigned int permId = 0; permId < 2; ++permId)
  {
    vtkSmartPointer<vtkUnstructuredGrid> reorderedInputGrid =
      ReorderCells(inputGrid, cellPermutation[permId]);
    binDataFilter->SetInputData(reorderedInputGrid);
    binDataFilter->Update();

    vtkIdTypeArray* binnedData = vtkIdTypeArray::SafeDownCast(
      binDataFilter->GetOutput()->GetCellData()->GetArray("binned_radius"));

    if (!binnedData)
    {
      std::cerr << "No binned data!" << std::endl;
      return EXIT_FAILURE;
    }

    for (vtkIdType i = 0; i < binnedData->GetNumberOfTuples(); i++)
    {
      std::cout << "cell # " << i << std::endl;
      std::cout << "[ < " << binDataFilter->GetValue(0) << " ]:\t\t"
                << binnedData->GetTypedComponent(i, 0) << std::endl;
      for (vtkIdType j = 1; j < binDataFilter->GetNumberOfBins(); j++)
      {
        std::cout << "[ " << binDataFilter->GetValue(j - 1) << " - " << binDataFilter->GetValue(j)
                  << " ]:\t" << binnedData->GetTypedComponent(i, j) << std::endl;
      }
      std::cout << "[ > " << binDataFilter->GetValue(binDataFilter->GetNumberOfBins()) << " ]:\t\t"
                << binnedData->GetTypedComponent(i, binDataFilter->GetNumberOfBins()) << std::endl;
      std::cout << std::endl;
    }

    vtkIdType expectedBins[18][4] = { { 0, 145, 217, 20 }, { 0, 688, 2242, 185 },
      { 0, 0, 219, 253 }, { 0, 0, 883, 526 }, { 118, 1792, 1740, 167 }, { 0, 0, 115, 83 },
      { 0, 10, 940, 406 }, { 0, 0, 131, 52 }, { 0, 194, 580, 91 }, { 0, 153, 669, 158 },
      { 0, 26, 211, 18 }, { 0, 0, 2, 193 }, { 0, 20, 102, 42 }, { 13, 51, 41, 3 },
      { 0, 0, 1367, 294 }, { 428, 2240, 1636, 137 }, { 0, 184, 193, 27 }, { 0, 0, 30, 15 } };

    if (binnedData->GetNumberOfTuples() != 18)
    {
      std::cerr << "Number of cells (" << binnedData->GetNumberOfTuples()
                << ") has deviated from expected value " << 18 << std::endl;
      return EXIT_FAILURE;
    }

    if (binnedData->GetNumberOfComponents() != 4)
    {
      std::cerr << "Number of bin values has deviated from expected value " << 4 << std::endl;
      return EXIT_FAILURE;
    }

    for (vtkIdType i = 0; i < binnedData->GetNumberOfTuples(); i++)
    {
      vtkIdType iPerm = cellPermutation[permId][i];
      for (vtkIdType j = 0; j < binnedData->GetNumberOfComponents(); j++)
      {
        if (binnedData->GetTypedComponent(i, j) != expectedBins[iPerm][j])
        {
          std::cerr << "Bin value (" << i << "," << j
                    << ") = " << binnedData->GetTypedComponent(i, j)
                    << " has deviated from expected value " << expectedBins[iPerm][j] << std::endl;
          return EXIT_FAILURE;
        }
      }
    }
  }

  binDataFilter->SetInputData(inputGrid);
  binDataFilter->SetCellOverlapMethod(vtkBinCellDataFilter::CELL_POINTS);
  binDataFilter->Update();
  // The method vtkBinCellDataFilter::CELL_POINTS is dependent on the cells' order.
  // Thus, it is not stable to cell permutations.
  {
    vtkIdTypeArray* binnedData = vtkIdTypeArray::SafeDownCast(
      binDataFilter->GetOutput()->GetCellData()->GetArray("binned_radius"));

    if (!binnedData)
    {
      std::cerr << "No binned data!" << std::endl;
      return EXIT_FAILURE;
    }

    for (vtkIdType i = 0; i < binnedData->GetNumberOfTuples(); i++)
    {
      std::cout << "cell # " << i << std::endl;
      std::cout << "[ < " << binDataFilter->GetValue(0) << " ]:\t\t"
                << binnedData->GetTypedComponent(i, 0) << std::endl;
      for (vtkIdType j = 1; j < binDataFilter->GetNumberOfBins(); j++)
      {
        std::cout << "[ " << binDataFilter->GetValue(j - 1) << " - " << binDataFilter->GetValue(j)
                  << " ]:\t" << binnedData->GetTypedComponent(i, j) << std::endl;
      }
      std::cout << "[ > " << binDataFilter->GetValue(binDataFilter->GetNumberOfBins()) << " ]:\t\t"
                << binnedData->GetTypedComponent(i, binDataFilter->GetNumberOfBins()) << std::endl;
      std::cout << std::endl;
    }

    vtkIdType expectedBins[18][4] = { { 0, 179, 223, 63 }, { 0, 751, 2577, 445 },
      { 0, 0, 283, 473 }, { 0, 0, 1084, 985 }, { 150, 1810, 2072, 295 }, { 0, 0, 173, 136 },
      { 0, 5, 933, 704 }, { 0, 0, 138, 97 }, { 0, 259, 854, 194 }, { 0, 134, 603, 266 },
      { 0, 16, 107, 6 }, { 0, 0, 3, 236 }, { 0, 2, 20, 40 }, { 2, 15, 52, 0 }, { 0, 6, 1688, 571 },
      { 407, 2606, 1994, 156 }, { 0, 277, 284, 63 }, { 0, 0, 6, 12 } };

    if (binnedData->GetNumberOfTuples() != 18)
    {
      std::cerr << "Number of cells (" << binnedData->GetNumberOfTuples()
                << ") has deviated from expected value " << 18 << std::endl;
      return EXIT_FAILURE;
    }

    if (binnedData->GetNumberOfComponents() != 4)
    {
      std::cerr << "Number of bin values has deviated from expected value " << 4 << std::endl;
      return EXIT_FAILURE;
    }

    for (vtkIdType i = 0; i < binnedData->GetNumberOfTuples(); i++)
    {
      for (vtkIdType j = 0; j < binnedData->GetNumberOfComponents(); j++)
      {
        if (binnedData->GetTypedComponent(i, j) != expectedBins[i][j])
        {
          std::cerr << "Bin value (" << i << "," << j
                    << ") = " << binnedData->GetTypedComponent(i, j)
                    << " has deviated from expected value " << expectedBins[i][j] << std::endl;
          return EXIT_FAILURE;
        }
      }
    }
  }

  return EXIT_SUCCESS;
}
