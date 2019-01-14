/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestUnstructuredGridQuadricDecimation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <vtkSmartPointer.h>
#include <vtkCleanPolyData.h>
#include <vtkDelaunay3D.h>
#include <vtkDoubleArray.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPointSource.h>
#include <vtkPolyData.h>
#include <vtkUnstructuredGrid.h>
#include <vtkUnstructuredGridQuadricDecimation.h>

int TestUnstructuredGridQuadricDecimation(int, char *[])
{
  // This test constructs a tetrahedrally meshed sphere by first generating
  // <numberOfOriginalPoints> points randomly placed within a unit sphere, then
  // removing points that overlap within a tolerance, and finally constructing a
  // delaunay 3d tetrahedralization from the points. Additionally, point data
  // corresponding to the points distance from the origin are added to this
  // data. The resulting tetrahedral mesh is then decimated <numberOfTests>
  // times, each time with a target reduction facter <targetReduction[test]>.
  // The number of remaining tetrahedra is then compared to the original number
  // of tetrahedra and compared against the target reduction factor. If the
  // difference is greater than <absTolerance>, the test fails. Otherwise, the
  // test passes.

  // # of points to generate the original tetrahedral mesh
  const vtkIdType numberOfOriginalPoints = 1.e4;

  // # of decimation tests to perform
  const vtkIdType numberOfTests = 4;

  // target reduction values for each test
  const double targetReduction[numberOfTests] = {.1,.3,.5,.7};

  // absolute tolerance between the expected and received tetrahedron reduction
  // to determine whether the decimation successfully executed
  const double absTolerance = 1.e-1;

  // Generate points within a unit sphere centered at the origin.
  vtkSmartPointer<vtkPointSource> source =
    vtkSmartPointer<vtkPointSource>::New();
  source->SetNumberOfPoints(numberOfOriginalPoints);
  source->SetCenter(0.,0.,0.);
  source->SetRadius(1.);
  source->SetDistributionToUniform();
  source->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);

  // Clean the polydata. This will remove overlapping points that may be
  // present in the input data.
  vtkSmartPointer<vtkCleanPolyData> cleaner =
    vtkSmartPointer<vtkCleanPolyData>::New();
  cleaner->SetInputConnection(source->GetOutputPort());
  cleaner->Update();

  // Create point data for use in decimation (the point data acts as a fourth
  // dimension in a Euclidean metric for determining the "nearness" of points).
  vtkPolyData* pd = cleaner->GetOutput();
  vtkPoints* points = pd->GetPoints();
  vtkSmartPointer<vtkDoubleArray> radius =
    vtkSmartPointer<vtkDoubleArray>::New();
  radius->SetName("radius");
  radius->SetNumberOfComponents(1);
  radius->SetNumberOfTuples(points->GetNumberOfPoints());
  double xyz[3];
  double r;
  for (vtkIdType i=0; i<points->GetNumberOfPoints(); i++)
  {
    points->GetPoint(i,xyz);
    r = std::sqrt(xyz[0]*xyz[0] + xyz[1]*xyz[1] + xyz[2]*xyz[2]);
    radius->SetTypedTuple(i,&r);
  }
  pd->GetPointData()->SetScalars(radius);

  // Generate a tetrahedral mesh from the input points. By
  // default, the generated volume is the convex hull of the points.
  vtkSmartPointer<vtkDelaunay3D> delaunay3D =
    vtkSmartPointer<vtkDelaunay3D>::New();
  delaunay3D->SetInputData(pd);
  delaunay3D->Update();

  const vtkIdType numberOfOriginalTetras =
    delaunay3D->GetOutput()->GetNumberOfCells();

  for (vtkIdType test = 0; test < numberOfTests; test++)
  {
    // Decimate the tetrahedral mesh.
    vtkSmartPointer<vtkUnstructuredGridQuadricDecimation> decimate =
      vtkSmartPointer<vtkUnstructuredGridQuadricDecimation>::New();
    decimate->SetInputConnection(delaunay3D->GetOutputPort());
    decimate->SetScalarsName("radius");
    decimate->SetTargetReduction(targetReduction[test]);
    decimate->Update();

    // Compare the resultant decimation fraction with the expected fraction.
    double fraction =
      (1. - static_cast<double>(decimate->GetOutput()
                                ->GetNumberOfCells()) / numberOfOriginalTetras);

    std::cout<<"Test # "<<test<<std::endl;
    std::cout<<"number of original tetras: "<<numberOfOriginalTetras<<std::endl;
    std::cout<<"number of tetras after decimation: "<<decimate->GetOutput()
             ->GetNumberOfCells()<<std::endl;
    std::cout<<"fraction: "<<fraction<<std::endl;
    std::cout<<"expected fraction: "<<targetReduction[test]<<std::endl;
    if (std::fabs(fraction - targetReduction[test]) > absTolerance)
    {
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
