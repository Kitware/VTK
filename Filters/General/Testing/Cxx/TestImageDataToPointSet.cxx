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

#include <vtkImageDataToPointSet.h>

#include <vtkImageData.h>
#include <vtkRTAnalyticSource.h>
#include <vtkStructuredGrid.h>

#include <vtkNew.h>

int TestImageDataToPointSet(int, char*[])
{
  vtkNew<vtkRTAnalyticSource> wavelet;
  wavelet->SetWholeExtent(-2, 2, -2, 2, -2, 2);
  wavelet->SetCenter(0, 0, 0);
  wavelet->SetMaximum(255);
  wavelet->SetStandardDeviation(.5);
  wavelet->SetXFreq(60);
  wavelet->SetYFreq(30);
  wavelet->SetZFreq(40);
  wavelet->SetXMag(10);
  wavelet->SetYMag(18);
  wavelet->SetZMag(5);
  wavelet->SetSubsampleRate(1);

  vtkNew<vtkImageDataToPointSet> image2points;
  image2points->SetInputConnection(wavelet->GetOutputPort());
  image2points->Update();

  vtkDataSet *inData = wavelet->GetOutput();
  vtkDataSet *outData = image2points->GetOutput();

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
