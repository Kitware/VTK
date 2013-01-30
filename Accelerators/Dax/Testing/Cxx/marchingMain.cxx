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
#include <vtkImageData.h>
#include <vtkDaxMarchingCubes.h>
#include <vtkUnstructuredGrid.h>
#include <vtkXMLImageDataReader.h>
#include <vtkImageMandelbrotSource.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkRegressionTestImage.h>

namespace
{
template<typename T>
void RunVTKPipeline(T *t)
{
    vtkNew<vtkRenderer> ren;
    vtkNew<vtkRenderWindow> renWin;
    vtkNew<vtkRenderWindowInteractor> iren;

    renWin->AddRenderer(ren.GetPointer());
    iren->SetRenderWindow(renWin.GetPointer());

    vtkNew<vtkDaxMarchingCubes> cubes;

    cubes->SetInputConnection(t->GetOutputPort());
    cubes->SetNumberOfContours(1);
    cubes->SetValue(0,10);

    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputConnection(cubes->GetOutputPort());

    vtkNew<vtkActor> actor;
    actor->SetMapper(mapper.GetPointer());

    ren->AddActor(actor.GetPointer());
    ren->ResetCamera();
    renWin->Render();

// int retVal = vtkRegressionTestImage(renWin.GetPointer());

// if(retVal == vtkRegressionTester::DO_INTERACTOR)
//   {
    iren->Start();
// }
}

} // Anonymous namespace



int marchingMain(int argc, char* argv[])
{
  //create the sample grid
  vtkImageMandelbrotSource *src = vtkImageMandelbrotSource::New();
  src->SetWholeExtent(0,40,0,40,0,40);

  //run the pipeline
  RunVTKPipeline(src);
  return 0;
}
