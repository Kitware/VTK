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
#include "vtkDaxMarchingCubes.h"
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

  vtkNew<vtkDaxMarchingCubes> cubes;

  cubes->SetInputConnection(t->GetOutputPort());
  cubes->SetNumberOfContours(1);
  cubes->SetValue(0,50.5f);

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(cubes->GetOutputPort());

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
  src->Update(); //required so we can set the active scalars

  //set Iterations as the active scalar, otherwise we don't have an array
  //to contour on
  vtkImageData* data = vtkImageData::SafeDownCast(src->GetOutputDataObject(0));
  if(data->GetPointData()->HasArray("Iterations") == 0)
    {
    //vtkImageMandelbrotSource has changed and this test needs updating
    return (!vtkRegressionTester::FAILED); //yeah it is weird, but the right way
    }

  //setting active scalars
  data->GetPointData()->SetActiveScalars("Iterations");

  //run the pipeline
  return RunVTKPipeline(src.GetPointer(),argc,argv);
}
