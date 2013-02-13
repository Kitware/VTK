//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2012 Sandia Corporation.
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//=============================================================================

#include <vtkNew.h>
#include <vtkFloatArray.h>
#include <vtkImageData.h>
#include <vtkMath.h>
#include <vtkThreshold.h>
#include <vtkPointData.h>
#include <vtkSmartPointer.h>
#include <vtkTrivialProducer.h>
#include <vtkUnstructuredGrid.h>
#include <vtkXMLDataSetWriter.h>
#include <vtkDaxThreshold.h>

namespace
{
  void fillElevationArray(vtkFloatArray* elven, vtkImageData* grid)
  {
    elven->SetName("Elevation");
    const vtkIdType size = grid->GetNumberOfPoints();
    elven->SetNumberOfValues(size);
    double pos[3]={0,0,0};
    for(vtkIdType i=0; i < size; ++i)
      {
      grid->GetPoint(i,pos);
      elven->SetValue(i,sqrt(vtkMath::Dot(pos,pos)));
      }
  }

  void RunVTKPipeline(vtkImageData* grid, int dim)
  {
    std::cout << "Running pipeline 1: Elevation -> Threshold" << std::endl;

    //compute an elevation array
    vtkSmartPointer<vtkFloatArray> elevationPoints = vtkSmartPointer<vtkFloatArray>::New();
    fillElevationArray(elevationPoints, grid);
    grid->GetPointData()->AddArray(elevationPoints);

    vtkNew<vtkTrivialProducer> producer;
    producer->SetOutput(grid);
    producer->Update();

    vtkNew<vtkDaxThreshold> threshold;
    threshold->SetInputConnection(producer->GetOutputPort());
    threshold->SetPointsDataTypeToFloat();
    threshold->AllScalarsOn();
    threshold->ThresholdBetween(0,100);
    threshold->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS,"Elevation");
    threshold->Update();
  }

} // Anonymous namespace

int thresholdMain(int argc, char* argv[])
{
  //create the sample grid
  vtkNew<vtkImageData> grid;
  int dim = 128;
  grid->SetOrigin(0.0, 0.0, 0.0);
  grid->SetSpacing(1.0, 1.0, 1.0);
  grid->SetExtent(0, dim-1,0, dim-1,0, dim-1);

  //run the pipeline
  RunVTKPipeline(grid.GetPointer(),dim);
  return EXIT_SUCCESS;
}
