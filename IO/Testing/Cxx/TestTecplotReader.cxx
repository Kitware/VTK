/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTecPlotReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Test of vtkTecplotReader
// .SECTION Description
//

#include "vtkTecplotReader.h"
#include "vtkDebugLeaks.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkCompositeDataGeometryFilter.h"
#include "vtkPolyDataMapper.h"
#include "vtkPointData.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRegressionTestImage.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"


int TestTecplotReader( int argc, char *argv[] )
{
  // we have to use a composite pipeline
  vtkCompositeDataPipeline* exec = vtkCompositeDataPipeline::New();
  vtkCompositeDataPipeline* exec2 = vtkCompositeDataPipeline::New();

  // Basic visualization.
  vtkRenderWindow* renWin = vtkRenderWindow::New();
  vtkRenderer* ren = vtkRenderer::New();
  renWin->AddRenderer(ren);
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);


  // Create the reader.
  char* fname1 = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/flow.tec");
  vtkSmartPointer<vtkTecplotReader> reader =
    vtkSmartPointer<vtkTecplotReader>::New();
  reader->SetFileName(fname1);
  reader->SetDataArrayStatus("V",1); //both files have a property named V
  reader->Update();
  delete [] fname1;

  vtkSmartPointer<vtkCompositeDataGeometryFilter> geom =
    vtkSmartPointer<vtkCompositeDataGeometryFilter>::New();
  geom->SetExecutive(exec);
  geom->SetInputConnection(0,reader->GetOutputPort(0));
  geom->Update();

  vtkPolyData *data = geom->GetOutput();
  data->GetPointData()->SetScalars( data->GetPointData()->GetArray("V") );

  vtkSmartPointer<vtkPolyDataMapper> mapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInput(data);
  mapper->ScalarVisibilityOn();
  mapper->SetColorModeToMapScalars();
  mapper->SetScalarRange(-0.3,0.3);

  vtkSmartPointer<vtkActor> actor =
    vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);

  char* fname2 = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/cellcentered.tec");
  vtkSmartPointer<vtkTecplotReader> reader2 =
  vtkSmartPointer<vtkTecplotReader>::New();
  reader2->SetFileName(fname2);
  reader2->SetDataArrayStatus("V",1); //both files have a property named V
  reader2->Update();
  delete [] fname2;

  vtkSmartPointer<vtkCompositeDataGeometryFilter> geom2 =
  vtkSmartPointer<vtkCompositeDataGeometryFilter>::New();
  geom2->SetExecutive(exec2);
  geom2->SetInputConnection(0,reader2->GetOutputPort(0));
  geom2->Update();

  vtkPolyData *data2 = geom2->GetOutput();
  data2->GetPointData()->SetScalars( data2->GetPointData()->GetArray("V") );

  vtkSmartPointer<vtkPolyDataMapper> mapper2 =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper2->SetInput(data2);
  mapper2->ScalarVisibilityOn();
  mapper2->SetColorModeToMapScalars();
  mapper2->SetScalarRange(-0.3,0.3);

  vtkSmartPointer<vtkActor> actor2 =
    vtkSmartPointer<vtkActor>::New();
  actor2->SetMapper(mapper2);

  ren->SetBackground(0.0,0.0,0.0);
  ren->AddActor(actor);
  ren->AddActor(actor2);
  renWin->SetSize(300,300);
  vtkCamera *cam=ren->GetActiveCamera();
  ren->ResetCamera();
  cam->Azimuth(180);

  // interact with data
  renWin->Render();

  int retVal = vtkRegressionTestImage( renWin );

  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  renWin->Delete();
  ren->Delete();
  iren->Delete();
  exec->Delete();
  exec2->Delete();
  return !retVal;
}
