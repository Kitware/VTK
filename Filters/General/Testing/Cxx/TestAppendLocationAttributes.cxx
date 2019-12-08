/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestAppendLocationAttributes.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <vtkAppendLocationAttributes.h>

#include <vtkCellCenters.h>
#include <vtkCellData.h>
#include <vtkCellTypeSource.h>
#include <vtkImageData.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkUnstructuredGrid.h>

#include <cstdlib>

int TestAppendLocationAttributes(int, char*[])
{
  // Reference dataset
  vtkNew<vtkCellTypeSource> cellTypeSource;
  cellTypeSource->SetBlocksDimensions(10, 10, 10);
  cellTypeSource->Update();
  vtkUnstructuredGrid* inputUG = cellTypeSource->GetOutput();

  // Create a vtkCellCenters object and use it to test the cell centers calculation in
  // vtkAppendLocationAttributes.
  vtkNew<vtkCellCenters> cellCenters;
  cellCenters->SetInputConnection(cellTypeSource->GetOutputPort());
  cellCenters->Update();

  vtkPointSet* cellCentersOutput = cellCenters->GetOutput();

  vtkNew<vtkAppendLocationAttributes> locationAttributes;
  locationAttributes->SetInputConnection(cellTypeSource->GetOutputPort());
  locationAttributes->Update();

  vtkPointSet* appendLocationOutput = vtkPointSet::SafeDownCast(locationAttributes->GetOutput());

  vtkIdType numCells = appendLocationOutput->GetNumberOfCells();
  vtkIdType numPoints = appendLocationOutput->GetNumberOfPoints();
  if (numCells != inputUG->GetNumberOfCells())
  {
    std::cerr << "Output number of cells is incorrect" << std::endl;
    return EXIT_FAILURE;
  }

  if (numPoints != inputUG->GetNumberOfPoints())
  {
    std::cerr << "Output number of points is incorrect" << std::endl;
    return EXIT_FAILURE;
  }

  vtkPoints* cellCenterPoints = cellCentersOutput->GetPoints();
  vtkDataArray* cellCentersArray = appendLocationOutput->GetCellData()->GetArray("CellCenters");
  vtkDataArray* pointLocationsArray =
    appendLocationOutput->GetPointData()->GetArray("PointLocations");

  for (vtkIdType i = 0; i < numCells; ++i)
  {
    double cellCenter[3];
    cellCenterPoints->GetPoint(i, cellCenter);

    double appendLocationCenter[3];
    cellCentersArray->GetTuple(i, appendLocationCenter);

    double dist2 = vtkMath::Distance2BetweenPoints(cellCenter, appendLocationCenter);
    if (dist2 > 1e-9)
    {
      std::cerr << "Cell center mismatch for cell " << i << std::endl;
      return EXIT_FAILURE;
    }
  }

  for (vtkIdType i = 0; i < numPoints; ++i)
  {
    double inputPoint[3];
    inputUG->GetPoints()->GetPoint(i, inputPoint);

    double appendLocationPoint[3];
    pointLocationsArray->GetTuple(i, appendLocationPoint);

    double dist2 = vtkMath::Distance2BetweenPoints(inputPoint, appendLocationPoint);
    if (dist2 > 1e-9)
    {
      std::cerr << "Point location mismatch for point " << i << std::endl;
      return EXIT_FAILURE;
    }
  }

  // Test with vtkImageData
  vtkNew<vtkImageData> image;
  image->SetDimensions(10, 10, 10);
  image->AllocateScalars(VTK_FLOAT, 1);

  locationAttributes->SetInputData(image);
  locationAttributes->Update();
  vtkImageData* imageWithLocations = locationAttributes->GetImageDataOutput();

  vtkPointData* imagePD = imageWithLocations->GetPointData();
  if (imagePD == nullptr)
  {
    std::cerr << "'PointLocations' array not added to vtkImageData point data" << std::endl;
    return EXIT_FAILURE;
  }

  vtkCellData* imageCD = imageWithLocations->GetCellData();
  if (imageCD == nullptr)
  {
    std::cerr << "'CellCenters' array not added to vtkImageData cell data" << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
