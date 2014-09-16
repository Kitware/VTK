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
#include "vtkCellData.h"
#include "vtkDaxContour.h"
#include "vtkElevationFilter.h"
#include "vtkImageData.h"
#include "vtkImageMandelbrotSource.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"

namespace
{
template<typename T>
int RunVTKPipeline(T *t, int argc, char* argv[])
{
  vtkNew<vtkRenderer> ren;
  vtkNew<vtkRenderWindow> renWin;
  vtkNew<vtkRenderWindowInteractor> iren;

  renWin->AddRenderer(ren.GetPointer());
  iren->SetRenderWindow(renWin.GetPointer());

  vtkNew<vtkDaxContour> cubes;

  cubes->SetInputConnection(t->GetOutputPort());
  cubes->SetInputArrayToProcess(
        0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "Iterations");
  cubes->SetNumberOfContours(1);
  cubes->SetValue(0,50.5f);
  cubes->ComputeScalarsOn();

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(cubes->GetOutputPort());
  mapper->ScalarVisibilityOn();
  mapper->SetScalarModeToUsePointFieldData();
  mapper->SelectColorArray("Elevation");
  mapper->SetScalarRange(0.0, 1.0);

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



int TestDaxMarchingCubes(int argc, char* argv[])
{
  //create the sample grid
  vtkNew<vtkImageMandelbrotSource> src;
  src->SetWholeExtent(0,250,0,250,0,250);

  //create a secondary field for interpolation
  vtkNew<vtkElevationFilter> elevation;
  elevation->SetInputConnection(src->GetOutputPort());
  elevation->SetScalarRange(0.0, 1.0);
  elevation->SetLowPoint(-1.75, 0.0, 1.0);
  elevation->SetHighPoint(0.75, 0.0, 1.0);

  //run the pipeline
  return RunVTKPipeline(elevation.GetPointer(),argc,argv);
}
