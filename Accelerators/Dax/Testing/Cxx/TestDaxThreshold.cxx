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

#include "vtkActor.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkDaxThreshold.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkThreshold.h"
#include "vtkTrivialProducer.h"

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

  int RunVTKPipeline(vtkImageData* grid, int argc, char* argv[])
  {
    vtkNew<vtkRenderer> ren;
    vtkNew<vtkRenderWindow> renWin;
    vtkNew<vtkRenderWindowInteractor> iren;

    renWin->AddRenderer(ren.GetPointer());
    iren->SetRenderWindow(renWin.GetPointer());

    //compute an elevation array
    vtkNew<vtkFloatArray> elevationPoints;
    fillElevationArray(elevationPoints.GetPointer(), grid);
    grid->GetPointData()->AddArray(elevationPoints.GetPointer());

    vtkNew<vtkTrivialProducer> producer;
    producer->SetOutput(grid);

    vtkNew<vtkDaxThreshold> threshold;
    threshold->SetInputConnection(producer->GetOutputPort());
    threshold->SetPointsDataTypeToFloat();
    threshold->AllScalarsOn();
    threshold->ThresholdBetween(0,100);
    threshold->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS,"Elevation");

    vtkNew<vtkDataSetSurfaceFilter> surface;
    surface->SetInputConnection(threshold->GetOutputPort());

    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputConnection(surface->GetOutputPort());

    vtkNew<vtkActor> actor;
    actor->SetMapper(mapper.GetPointer());

    ren->AddActor(actor.GetPointer());
    ren->ResetCamera();
    renWin->Render();

    int retVal = vtkRegressionTestImage(renWin.GetPointer());
    if(retVal == vtkRegressionTester::DO_INTERACTOR)
      {
      iren->Start();
      retVal = vtkRegressionTester::PASSED;
      }
    return (!retVal);
  }

} // Anonymous namespace

int TestDaxThreshold(int argc, char* argv[])
{
  //create the sample grid
  vtkNew<vtkImageData> grid;
  int dim = 128;
  grid->SetOrigin(0.0, 0.0, 0.0);
  grid->SetSpacing(1.0, 1.0, 1.0);
  grid->SetExtent(0, dim-1,0, dim-1,0, dim-1);

  //run the pipeline
  return RunVTKPipeline(grid.GetPointer(), argc, argv);
}
