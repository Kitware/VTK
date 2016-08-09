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
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPointSource.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#include <vtkUnstructuredGrid.h>

vtkSmartPointer<vtkUnstructuredGrid>
ConstructDelaunay3DSphere(vtkIdType numberOfPoints, bool sampleShellOnly)
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

  vtkMath::RandomSeed(0);

  const vtkIdType numberOfSourcePoints = 1.e4;
  const vtkIdType numberOfInputPoints = 1.e1;

  vtkSmartPointer<vtkUnstructuredGrid> sourceGrid =
    ConstructDelaunay3DSphere(numberOfSourcePoints, false);
  vtkSmartPointer<vtkUnstructuredGrid> inputGrid =
    ConstructDelaunay3DSphere(numberOfInputPoints, true);

  vtkNew<vtkBinCellDataFilter> binDataFilter;
  binDataFilter->SetInputData(inputGrid);
  binDataFilter->SetSourceData(sourceGrid);
  binDataFilter->SetComputeTolerance(false);
  binDataFilter->GenerateValues(3, .2, .8);
  binDataFilter->SetBinnedDataArrayName("BinnedData");
  binDataFilter->Update();

  vtkIdTypeArray* binnedData = vtkIdTypeArray::SafeDownCast(
    binDataFilter->GetOutput()->GetCellData()->GetArray("BinnedData"));

  if (!binnedData)
    {
    std::cerr<<"No binned data!"<<std::endl;
    return 1;
    }

  for (vtkIdType i=0; i<binnedData->GetNumberOfTuples(); i++)
    {
    std::cout<<"cell # "<<i<<std::endl;
    std::cout<<"[ < "<<binDataFilter->GetValue(0)<<" ]:\t\t"
             <<binnedData->GetTypedComponent(i,0)<<std::endl;
    for (vtkIdType j=1; j<binDataFilter->GetNumberOfBins(); j++)
      {
      std::cout << "[ " << binDataFilter->GetValue(j-1) << " - "
                << binDataFilter->GetValue(j) << " ]:\t"
                << binnedData->GetTypedComponent(i,j) << std::endl;
      }
    std::cout<<"[ > "<<binDataFilter->GetValue(binDataFilter->GetNumberOfBins())
             <<" ]:\t\t"<<binnedData->
      GetTypedComponent(i,binDataFilter->GetNumberOfBins())<<std::endl;
    std::cout << std::endl;
    }

  vtkIdType expectedBins[14][4] = {{0,0,291,290},
                                   {0,0,592,547},
                                   {0,0,276,104},
                                   {0,0,132,204},
                                   {0,0,69,576},
                                   {0,0,55,65},
                                   {0,69,1310,151},
                                   {0,0,1100,298},
                                   {0,0,0,138},
                                   {0,212,753,91},
                                   {281,3427,2660,165},
                                   {0,150,1715,241},
                                   {0,0,1725,452},
                                   {0,0,130,246}};

  if (binnedData->GetNumberOfTuples() != 14)
    {
    std::cerr<<"Number of cells has deviated from expected value "<<14<<std::endl;
    return 1;
    }

  if (binnedData->GetNumberOfComponents() != 4)
    {
    std::cerr<<"Number of bin values has deviated from expected value "<<4<<std::endl;
    return 1;
    }

  for (vtkIdType i=0; i<binnedData->GetNumberOfTuples(); i++)
    {
    for (vtkIdType j=0; j<binnedData->GetNumberOfComponents(); j++)
      {
      if (binnedData->GetTypedComponent(i,j) != expectedBins[i][j])
        {
        std::cerr<<"Bin value ("<<i<<","<<j<<") has deviated from expected value "<<expectedBins[i][j]<<std::endl;
        return 1;
        }
      }
    }

  return EXIT_SUCCESS;
}
