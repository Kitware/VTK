/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestBinCellDataFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This tests the vtkBinCellDataFilter class.

#include <vtkBinCellDataFilter.h>
#include <vtkCellData.h>
#include <vtkCellIterator.h>
#include <vtkCleanPolyData.h>
#include <vtkDelaunay3D.h>
#include <vtkDoubleArray.h>
#include <vtkGenericCell.h>
#include <vtkIdTypeArray.h>
#include <vtkMersenneTwister.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPointSource.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#include <vtkUnstructuredGrid.h>

vtkSmartPointer<vtkUnstructuredGrid>
ConstructDelaunay3DSphere(vtkIdType numberOfPoints, vtkMersenneTwister* seq,
                          bool sampleShellOnly)
{
  // This function constructs a tetrahedrally meshed sphere by first generating
  // <numberOfPoints> points randomly placed within a unit sphere, then removing
  // points that overlap within a tolerance, and finally constructing a delaunay
  // 3d tetrahedralization from the points. Additionally, cell data
  // corresponding to the cell center's distance from the origin are added to
  // this data. If <sampleShellOnly> is true, the original point sampling is
  // performed on the shell of the unit sphere.

  // Generate points within a unit sphere centered at the origin.
  vtkSmartPointer<vtkPointSource> source =
    vtkSmartPointer<vtkPointSource>::New();
  source->SetNumberOfPoints(numberOfPoints);
  source->SetCenter(0.,0.,0.);
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
  vtkSmartPointer<vtkCleanPolyData> cleaner =
    vtkSmartPointer<vtkCleanPolyData>::New();
  cleaner->SetInputConnection(source->GetOutputPort());

  // Generate a tetrahedral mesh from the input points. By
  // default, the generated volume is the convex hull of the points.
  vtkSmartPointer<vtkDelaunay3D> delaunay3D =
    vtkSmartPointer<vtkDelaunay3D>::New();
  delaunay3D->SetInputConnection(cleaner->GetOutputPort());
  delaunay3D->Update();

  // Create cell data for use in binning.
  vtkUnstructuredGrid* ug = delaunay3D->GetOutput();
  vtkSmartPointer<vtkDoubleArray> radius =
    vtkSmartPointer<vtkDoubleArray>::New();
  radius->SetName("radius");
  radius->SetNumberOfComponents(1);
  radius->SetNumberOfTuples(ug->GetNumberOfCells());

  double weights[VTK_CELL_SIZE];
  double pcoords[3], coords[3];
  int subId;
  double r;
  vtkNew<vtkGenericCell> cell;
  vtkCellIterator *it = ug->NewCellIterator();
  for (it->InitTraversal(); !it->IsDoneWithTraversal(); it->GoToNextCell())
  {
    it->GetCell(cell.GetPointer());
    cell->GetParametricCenter(pcoords);
    cell->EvaluateLocation(subId, pcoords, coords, weights);

    r = std::sqrt(coords[0]*coords[0] + coords[1]*coords[1] +
                  coords[2]*coords[2]);
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

  const vtkIdType numberOfSourcePoints = 1.e4;
  const vtkIdType numberOfInputPoints = 1.e1;

  vtkSmartPointer<vtkUnstructuredGrid> sourceGrid =
    ConstructDelaunay3DSphere(numberOfSourcePoints, seq.GetPointer(), false);
  vtkSmartPointer<vtkUnstructuredGrid> inputGrid =
    ConstructDelaunay3DSphere(numberOfInputPoints, seq.GetPointer(), true);

  vtkNew<vtkBinCellDataFilter> binDataFilter;
  binDataFilter->SetInputData(inputGrid);
  binDataFilter->SetSourceData(sourceGrid);
  binDataFilter->SetComputeTolerance(false);
  binDataFilter->GenerateValues(3, .2, .8);
  binDataFilter->Update();

  {
  vtkIdTypeArray* binnedData = vtkIdTypeArray::SafeDownCast(
    binDataFilter->GetOutput()->GetCellData()->GetArray("binned_radius"));

  if (!binnedData)
  {
    cerr << "No binned data!" << endl;
    return 1;
  }

  for (vtkIdType i = 0; i < binnedData->GetNumberOfTuples(); i++)
  {
    cout << "cell # "<<i << endl;
    cout << "[ < "<<binDataFilter->GetValue(0) << " ]:\t\t"
         << binnedData->GetTypedComponent(i,0) << endl;
    for (vtkIdType j = 1; j < binDataFilter->GetNumberOfBins(); j++)
    {
      cout << "[ " << binDataFilter->GetValue(j-1) << " - "
                << binDataFilter->GetValue(j) << " ]:\t"
                << binnedData->GetTypedComponent(i,j) << endl;
    }
    cout << "[ > "<<binDataFilter->GetValue(binDataFilter->GetNumberOfBins())
         << " ]:\t\t" << binnedData->
      GetTypedComponent(i,binDataFilter->GetNumberOfBins()) << endl;
    cout << endl;
  }

  vtkIdType expectedBins[17][4] = {{0,0,0,62},
                                   {0,0,7,134},
                                   {60,937,1471,152},
                                   {0,0,392,526},
                                   {99,373,352,216},
                                   {6,1262,2054,316},
                                   {0,36,74,4},
                                   {33,358,357,46},
                                   {302,1682,2064,362},
                                   {26,60,58,41},
                                   {162,444,620,186},
                                   {0,316,752,187},
                                   {0,279,300,23},
                                   {0,838,1428,152},
                                   {0,30,53,9},
                                   {0,381,706,151},
                                   {0,122,117,6}};

  if (binnedData->GetNumberOfTuples() != 17)
  {
    cerr << "Number of cells has deviated from expected value " << 17 << endl;
    return 1;
  }

  if (binnedData->GetNumberOfComponents() != 4)
  {
    cerr << "Number of bin values has deviated from expected value "<< 4
         << endl;
    return 1;
  }

  for (vtkIdType i = 0; i < binnedData->GetNumberOfTuples(); i++)
  {
    for (vtkIdType j = 0; j < binnedData->GetNumberOfComponents(); j++)
    {
      if (binnedData->GetTypedComponent(i,j) != expectedBins[i][j])
      {
        cerr << "Bin value (" << i << "," << j
             << ") has deviated from expected value " << expectedBins[i][j]
             << endl;
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
    cerr << "No binned data!" << endl;
    return 1;
  }

  for (vtkIdType i = 0; i < binnedData->GetNumberOfTuples(); i++)
  {
    cout << "cell # "<< i << endl;
    cout << "[ < " << binDataFilter->GetValue(0) << " ]:\t\t"
             <<binnedData->GetTypedComponent(i,0) << endl;
    for (vtkIdType j = 1; j < binDataFilter->GetNumberOfBins(); j++)
    {
      cout << "[ " << binDataFilter->GetValue(j-1) << " - "
           << binDataFilter->GetValue(j) << " ]:\t"
           << binnedData->GetTypedComponent(i,j) << endl;
    }
    cout << "[ > " << binDataFilter->GetValue(binDataFilter->GetNumberOfBins())
         << " ]:\t\t" << binnedData->
      GetTypedComponent(i,binDataFilter->GetNumberOfBins()) << endl;
    cout << endl;
  }

  vtkIdType expectedBins[17][4] = {{0,0,0,165},
                                   {0,0,6,282},
                                   {69,1096,1979,278},
                                   {0,0,382,839},
                                   {121,406,345,326},
                                   {6,1352,2675,667},
                                   {0,14,130,0},
                                   {6,314,335,46},
                                   {331,1858,2425,713},
                                   {22,0,32,27},
                                   {132,359,538,188},
                                   {0,313,857,307},
                                   {0,405,264,24},
                                   {1,861,1880,270},
                                   {0,31,77,1},
                                   {0,406,1094,348},
                                   {0,106,204,11}};

  if (binnedData->GetNumberOfTuples() != 17)
  {
    cerr << "Number of cells has deviated from expected value " << 17 << endl;
    return 1;
  }

  if (binnedData->GetNumberOfComponents() != 4)
  {
    cerr << "Number of bin values has deviated from expected value " << 4
         << endl;
    return 1;
  }

  for (vtkIdType i = 0; i < binnedData->GetNumberOfTuples(); i++)
  {
    for (vtkIdType j = 0; j < binnedData->GetNumberOfComponents(); j++)
    {
      if (binnedData->GetTypedComponent(i,j) != expectedBins[i][j])
      {
        cerr << "Bin value (" << i << "," << j
             << ") has deviated from expected value "
             << expectedBins[i][j] << endl;
        return 1;
      }
    }
  }
  }

  return EXIT_SUCCESS;
}
