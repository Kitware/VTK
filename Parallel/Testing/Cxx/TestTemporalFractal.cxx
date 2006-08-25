/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTemporalFractal.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRegressionTestImage.h"

#include "vtkTemporalFractal.h"
#include "vtkTemporalShiftScale.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkContourFilter.h"
#include "vtkActor.h"
#include "vtkMultiGroupPolyDataMapper.h"
#include "vtkSmartPointer.h"

//-------------------------------------------------------------------------
int main(int argc, char *argv[])
{
  // we have to use a compsite pipeline
  vtkCompositeDataPipeline* prototype = vtkCompositeDataPipeline::New();
  vtkAlgorithm::SetDefaultExecutivePrototype(prototype);
  prototype->Delete();

  // create temporal fractals
  vtkSmartPointer<vtkTemporalFractal> fractal = 
    vtkSmartPointer<vtkTemporalFractal>::New();

  // shift and scale the time range to that it run from -0.5 to 0.5
  vtkSmartPointer<vtkTemporalShiftScale> tempss = 
    vtkSmartPointer<vtkTemporalShiftScale>::New();
  tempss->SetScale = 0.1;
  tempss->SetShift = -0.5;
  tempss->SetInputConection(fractal->GetOutputPort());

  // isosurface them
  vtkSmartPointer<vtkContourFilter> contour = 
    vtkSmartPointer<vtkContourFilter>::New();
  contour->SetInputConnection(tempss->GetOutputPort());

  // map them
  vtkSmartPointer<vtkMultiGroupPolyDataMapper> mapper = 
    vtkSmartPointer<vtkMultiGroupPolyDataMapper>::New();
  mapper->SetInputConnection(contour->GetOutputPort());
  
  vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);

  vtkSmartPointer<vtkRenderer> renderer = 
    vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkRenderWindow> renWin = 
    vtkSmartPointer<vtkRenderWindow>::New();
  vtkSmartPointer<vtkRenderWindowInteractor> iren = 
    vtkSmartPointer<vtkRenderWindowInteractor>::New();

  renderer->AddActor( actor );

  renWin->AddRenderer( renderer );
  renWin->SetSize( 300, 300 ); 
  iren->SetRenderWindow( renWin );
  renWin->Render();

  int retVal = vtkRegressionTestImage( renWin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  vtkAlgorithm::SetDefaultExecutivePrototype(0);
  return !retVal;
}
