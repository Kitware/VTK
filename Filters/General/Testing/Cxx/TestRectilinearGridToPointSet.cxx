/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCellDataToPointData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkRectilinearGridToPointSet.h>

#include <vtkDoubleArray.h>
#include <vtkMath.h>
#include <vtkRectilinearGrid.h>
#include <vtkStructuredGrid.h>

#include <vtkNew.h>
#include <vtkSmartPointer.h>

#include <time.h>

static vtkSmartPointer<vtkDataArray> MonotonicValues(vtkIdType numValues)
{
  vtkSmartPointer<vtkDoubleArray> values
      = vtkSmartPointer<vtkDoubleArray>::New();
  values->SetNumberOfComponents(1);
  values->SetNumberOfTuples(numValues);

  double v = vtkMath::Random();
  for (vtkIdType id = 0; id < numValues; id++)
  {
    values->SetValue(id, v);
    v += vtkMath::Random();
  }

  return values;
}

static vtkSmartPointer<vtkRectilinearGrid> MakeRectilinearGrid()
{
  vtkSmartPointer<vtkRectilinearGrid> grid
      = vtkSmartPointer<vtkRectilinearGrid>::New();

  int extent[6];
  for (int i = 0; i < 6; i += 2)
  {
    extent[i] = vtkMath::Round(vtkMath::Random(-10, 10));
    extent[i+1] = extent[i] + vtkMath::Round(vtkMath::Random(0, 10));
  }

  grid->SetExtent(extent);

  vtkSmartPointer<vtkDataArray> coord;

  coord = MonotonicValues(extent[1]-extent[0]+1);
  grid->SetXCoordinates(coord);

  coord = MonotonicValues(extent[3]-extent[2]+1);
  grid->SetYCoordinates(coord);

  coord = MonotonicValues(extent[5]-extent[4]+1);
  grid->SetZCoordinates(coord);

  return grid;
}

int TestRectilinearGridToPointSet(int, char*[])
{
  int seed = time(NULL);
  std::cout << "Seed: " << seed << std::endl;
  vtkMath::RandomSeed(seed);

  vtkSmartPointer<vtkRectilinearGrid> inData = MakeRectilinearGrid();

  vtkNew<vtkRectilinearGridToPointSet> rect2points;
  rect2points->SetInputData(inData);
  rect2points->Update();

  vtkDataSet *outData = rect2points->GetOutput();

  vtkIdType numPoints = inData->GetNumberOfPoints();
  if (numPoints != outData->GetNumberOfPoints())
  {
    std::cout << "Got wrong number of points: " << numPoints << " vs "
         << outData->GetNumberOfPoints() << std::endl;
    return EXIT_FAILURE;
  }

  vtkIdType numCells = inData->GetNumberOfCells();
  if (numCells != outData->GetNumberOfCells())
  {
    std::cout << "Got wrong number of cells: " << numCells << " vs "
         << outData->GetNumberOfCells() << std::endl;
    return EXIT_FAILURE;
  }

  for (vtkIdType pointId = 0; pointId < numPoints; pointId++)
  {
    double inPoint[3];
    double outPoint[3];

    inData->GetPoint(pointId, inPoint);
    outData->GetPoint(pointId, outPoint);

    if (   (inPoint[0] != outPoint[0])
        || (inPoint[1] != outPoint[1])
        || (inPoint[2] != outPoint[2]) )
    {
      std::cout << "Got mismatched point coordinates." << std::endl;
      std::cout << "Input: " << inPoint[0] << " " << inPoint[1] << " " << inPoint[2] << std::endl;
      std::cout << "Output: " << outPoint[0] << " " << outPoint[1] << " " << outPoint[2] << std::endl;
    }
  }

  return EXIT_SUCCESS;
}
