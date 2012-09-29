/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestImageDAtaToUniformGrid.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME TestImageDAtaToUniformGrid.cxx --Test vtkImageDataToUniformGrid
//
// .SECTION Description
// Serial tests for converting an image data to a uniform grid with blanking.

// VTK includes
#include <vtkDataSet.h>
#include <vtkElevationFilter.h>
#include <vtkImageDataToUniformGrid.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkMultiBlockDataGroupFilter.h>
#include <vtkNew.h>
#include <vtkPointDataToCellData.h>
#include <vtkRTAnalyticSource.h>
#include <vtkSphereSource.h>
#include <vtkThreshold.h>
#include <vtkUnstructuredGrid.h>

// C++ includes
#include <iostream>

// returns 0 for success.
int TestSingleGridBlanking(bool pointBlanking, int expectedNumberOfCells)
{
  vtkNew<vtkRTAnalyticSource> source;
  vtkNew<vtkElevationFilter> elevation;
  elevation->SetInputConnection(source->GetOutputPort());
  elevation->SetLowPoint(-10, 0, 0);
  elevation->SetHighPoint(10, 0, 0);
  elevation->SetScalarRange(-3, 3);
  vtkNew<vtkPointDataToCellData> pointDataToCellData;
  pointDataToCellData->SetInputConnection(elevation->GetOutputPort());
  pointDataToCellData->PassPointDataOn();

  pointDataToCellData->Update();

  vtkNew<vtkImageDataToUniformGrid> imageDataToUniformGrid;
  imageDataToUniformGrid->SetInputConnection(pointDataToCellData->GetOutputPort());
  if(pointBlanking)
    {
    imageDataToUniformGrid->SetInputArrayToProcess(
      0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "Elevation");
    }
  else
    {
    imageDataToUniformGrid->SetInputArrayToProcess(
      0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS, "Elevation");
    }
  imageDataToUniformGrid->Update();
  vtkNew<vtkThreshold> threshold;
  threshold->SetInputArrayToProcess( 0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS,
                                     "RTData");
  threshold->ThresholdBetween(50, 150);
  threshold->SetInputConnection(imageDataToUniformGrid->GetOutputPort());
  threshold->Update();
  vtkUnstructuredGrid* outputGrid = threshold->GetOutput();

  if(outputGrid->GetNumberOfCells() == expectedNumberOfCells)
    {
    return 0;
    }
  vtkGenericWarningMacro("Expecting " << expectedNumberOfCells << " but getting "
                         << outputGrid->GetNumberOfCells());
  return 1;
}

// returns 0 for success. only tests point blanking
int TestMultiBlockBlanking(int expectedNumberOfCells)
{
  vtkNew<vtkRTAnalyticSource> source;
  vtkNew<vtkElevationFilter> elevation;
  elevation->SetInputConnection(source->GetOutputPort());
  elevation->SetLowPoint(-10, 0, 0);
  elevation->SetHighPoint(10, 0, 0);
  elevation->SetScalarRange(-3, 3);
  vtkNew<vtkPointDataToCellData> pointDataToCellData;
  pointDataToCellData->SetInputConnection(elevation->GetOutputPort());
  pointDataToCellData->PassPointDataOn();

  vtkNew<vtkSphereSource> sphereSource;
  vtkNew<vtkMultiBlockDataGroupFilter> groupFilter;
  groupFilter->SetInputConnection(pointDataToCellData->GetOutputPort());
  groupFilter->SetInputConnection(pointDataToCellData->GetOutputPort());
  groupFilter->AddInputConnection(sphereSource->GetOutputPort());

  vtkNew<vtkImageDataToUniformGrid> imageDataToUniformGrid;
  imageDataToUniformGrid->SetInputConnection(groupFilter->GetOutputPort());
  imageDataToUniformGrid->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "Elevation");
  imageDataToUniformGrid->Update();
  vtkMultiBlockDataSet* output =
    vtkMultiBlockDataSet::SafeDownCast(imageDataToUniformGrid->GetOutput());

  vtkNew<vtkThreshold> threshold;
  threshold->SetInputArrayToProcess( 0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS,
                                     "RTData");
  threshold->ThresholdBetween(50, 150);
  threshold->SetInputData(output->GetBlock(0));
  threshold->Update();
  vtkUnstructuredGrid* outputGrid = threshold->GetOutput();

  if(outputGrid->GetNumberOfCells() == expectedNumberOfCells)
    {
    return 0;
    }
  vtkGenericWarningMacro("Expecting " << expectedNumberOfCells << " but getting "
                         << outputGrid->GetNumberOfCells());
  return 1;
}



//------------------------------------------------------------------------------
// Program main
int TestImageDataToUniformGrid( int, char ** )
{
  int rc = TestSingleGridBlanking(true, 1638);
  rc += TestSingleGridBlanking(false, 1740);

  rc += TestMultiBlockBlanking(1638);

  return rc;
}
