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
#include "vtkTestUtilities.h"

int TestTecplotReader( int argc, char *argv[] )
{

  // we have to use a composite pipeline
  vtkCompositeDataPipeline* exec = vtkCompositeDataPipeline::New();

  // Read file name.
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/flow.tec");

  // Create the reader.
  vtkTecplotReader* reader = vtkTecplotReader::New();
  reader->SetFileName(fname);
  reader->SetDataArrayStatus("V",1);
  reader->Update();
  delete [] fname;

  vtkCompositeDataGeometryFilter* geom = vtkCompositeDataGeometryFilter::New();
  geom->SetExecutive(exec);
  geom->SetInputConnection(0,reader->GetOutputPort(0));
  geom->Update();

  vtkPolyData *data = geom->GetOutput();
  data->GetPointData()->SetScalars( data->GetPointData()->GetArray("V") );

  vtkPolyDataMapper* mapper = vtkPolyDataMapper::New();
  mapper->SetInput(data);
  mapper->ScalarVisibilityOn();
  mapper->SetColorModeToMapScalars();
  mapper->SetScalarRange(-0.3,0.3);

  // Create the actor.
  vtkActor* actor = vtkActor::New();
  actor->SetMapper(mapper);

  // Basic visualization.
  vtkRenderWindow* renWin = vtkRenderWindow::New();
  vtkRenderer* ren = vtkRenderer::New();
  renWin->AddRenderer(ren);
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  ren->AddActor(actor);
  ren->SetBackground(0.0,0.0,0.0);
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

  actor->Delete();
  mapper->Delete();
  geom->Delete();
  reader->Delete();
  renWin->Delete();
  ren->Delete();
  iren->Delete();
  exec->Delete();

  return !retVal;
}
