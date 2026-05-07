// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// This tests the vtkBinCellDataFilter class.

#include <vtkBinCellDataFilter.h>
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
  binDataFilter->SetInputData(inputGrid);
  binDataFilter->SetSourceData(sourceGrid);
  binDataFilter->SetCellLocator(locator);
  binDataFilter->SetComputeTolerance(false);
  binDataFilter->GenerateValues(3, .2, .8);
  binDataFilter->Update();

  {
    vtkIdTypeArray* binnedData = vtkIdTypeArray::SafeDownCast(
      binDataFilter->GetOutput()->GetCellData()->GetArray("binned_radius"));

    if (!binnedData)
    {
      std::cerr << "No binned data!" << std::endl;
      return 1;
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

    vtkIdType expectedBins[18][4] = { { 0, 145, 220, 21 }, { 0, 692, 2253, 189 },
      { 0, 0, 214, 255 }, { 0, 0, 888, 539 }, { 118, 1805, 1764, 173 }, { 0, 0, 115, 85 },
      { 0, 9, 935, 416 }, { 0, 0, 123, 42 }, { 0, 196, 585, 92 }, { 0, 146, 663, 157 },
      { 0, 23, 210, 16 }, { 0, 0, 2, 193 }, { 0, 18, 97, 39 }, { 13, 46, 33, 3 },
      { 0, 0, 1368, 292 }, { 428, 2252, 1641, 136 }, { 0, 181, 192, 28 }, { 0, 0, 28, 13 } };

    if (binnedData->GetNumberOfTuples() != 18)
    {
      std::cerr << "Number of cells (" << binnedData->GetNumberOfTuples()
                << ") has deviated from expected value " << 18 << std::endl;
      return 1;
    }

    if (binnedData->GetNumberOfComponents() != 4)
    {
      std::cerr << "Number of bin values has deviated from expected value " << 4 << std::endl;
      return 1;
    }

    for (vtkIdType i = 0; i < binnedData->GetNumberOfTuples(); i++)
    {
      for (vtkIdType j = 0; j < binnedData->GetNumberOfComponents(); j++)
      {
        if (binnedData->GetTypedComponent(i, j) != expectedBins[i][j])
        {
          std::cerr << "Bin value (" << i << "," << j << ") has deviated from expected value "
                    << expectedBins[i][j] << std::endl;
          return 1;
        }
      }
    }
  }

  binDataFilter->SetCellOverlapMethod(vtkBinCellDataFilter::CELL_POINTS);
  binDataFilter->Update();

  {
    vtkIdTypeArray* binnedData = vtkIdTypeArray::SafeDownCast(
      binDataFilter->GetOutput()->GetCellData()->GetArray("binned_radius"));

    if (!binnedData)
    {
      std::cerr << "No binned data!" << std::endl;
      return 1;
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

    vtkIdType expectedBins[18][4] = { { 0, 179, 221, 63 }, { 0, 751, 2578, 458 },
      { 0, 0, 283, 458 }, { 0, 0, 1086, 983 }, { 150, 1810, 2077, 303 }, { 0, 0, 174, 150 },
      { 0, 5, 938, 718 }, { 0, 0, 121, 92 }, { 0, 259, 851, 180 }, { 0, 134, 603, 261 },
      { 0, 16, 105, 6 }, { 0, 0, 3, 236 }, { 0, 2, 20, 40 }, { 2, 15, 50, 0 }, { 0, 6, 1703, 584 },
      { 407, 2627, 2060, 168 }, { 0, 261, 242, 62 }, { 0, 0, 6, 12 } };

    if (binnedData->GetNumberOfTuples() != 18)
    {
      std::cerr << "Number of cells (" << binnedData->GetNumberOfTuples()
                << ") has deviated from expected value " << 18 << std::endl;
      return 1;
    }

    if (binnedData->GetNumberOfComponents() != 4)
    {
      std::cerr << "Number of bin values has deviated from expected value " << 4 << std::endl;
      return 1;
    }

    for (vtkIdType i = 0; i < binnedData->GetNumberOfTuples(); i++)
    {
      for (vtkIdType j = 0; j < binnedData->GetNumberOfComponents(); j++)
      {
        if (binnedData->GetTypedComponent(i, j) != expectedBins[i][j])
        {
          std::cerr << "Bin value (" << i << "," << j << ") has deviated from expected value "
                    << expectedBins[i][j] << std::endl;
          return 1;
        }
      }
    }
  }

  return EXIT_SUCCESS;
}
