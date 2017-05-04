/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCamera.h"
#include "vtkRenderer.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkActor.h"
#include "vtkCellArray.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkPLYReader.h"
#include "vtkNew.h"
#include "vtkProperty.h"
#include "vtkLightKit.h"
#include "vtkPolyDataNormals.h"
#include "vtkTimerLog.h"

#include "vtkRegressionTestImage.h"
#include "vtkTestUtilities.h"

#include "vtkRenderWindowInteractor.h"

#include "vtkOpenGLRenderWindow.h"


//----------------------------------------------------------------------------
int TestVBOPLYMapper(int argc, char *argv[])
{
  bool timeit = false;
  if (argc > 1 && argv[1] && !strcmp(argv[1], "-timeit"))
  {
    timeit = true;
  }

  vtkNew<vtkActor> actor;
  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkPolyDataMapper> mapper;
  renderer->SetBackground(0.0, 0.0, 0.0);
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(timeit ? 800 : 300, timeit ? 800 : 300);
  renderWindow->AddRenderer(renderer.Get());
  renderer->AddActor(actor.Get());
  vtkNew<vtkRenderWindowInteractor>  iren;
  iren->SetRenderWindow(renderWindow.Get());
  vtkNew<vtkLightKit> lightKit;
  lightKit->AddLightsToRenderer(renderer.Get());

  if (!renderWindow->SupportsOpenGL())
  {
    cerr << "The platform does not support OpenGL as required\n";
    cerr << vtkOpenGLRenderWindow::SafeDownCast(renderWindow.Get())->GetOpenGLSupportMessage();
    cerr << renderWindow->ReportCapabilities();
    return 1;
  }

  const char* fileName = vtkTestUtilities::ExpandDataFileName(argc, argv,
                                                               "Data/dragon.ply");
  vtkNew<vtkPLYReader> reader;
  reader->SetFileName(fileName);
  reader->Update();

  // vtkNew<vtkPolyDataNormals> norms;
  // norms->SetInputConnection(reader->GetOutputPort());
  // norms->Update();

  mapper->SetInputConnection(reader->GetOutputPort());
  //mapper->SetInputConnection(norms->GetOutputPort());
  actor->SetMapper(mapper.Get());
  actor->GetProperty()->SetAmbientColor(0.2, 0.2, 1.0);
  actor->GetProperty()->SetDiffuseColor(1.0, 0.65, 0.7);
  actor->GetProperty()->SetSpecularColor(1.0, 1.0, 1.0);
  actor->GetProperty()->SetSpecular(0.5);
  actor->GetProperty()->SetDiffuse(0.7);
  actor->GetProperty()->SetAmbient(0.5);
  actor->GetProperty()->SetSpecularPower(20.0);
  actor->GetProperty()->SetOpacity(1.0);
  //actor->GetProperty()->SetRepresentationToWireframe();

  renderWindow->SetMultiSamples(0);

  vtkNew<vtkTimerLog> timer;
  timer->StartTimer();
  renderWindow->Render();
  timer->StopTimer();
  double firstRender = timer->GetElapsedTime();
  cerr << "first render time: " << firstRender << endl;
  int major, minor;
  vtkOpenGLRenderWindow::SafeDownCast(renderWindow.Get())->GetOpenGLVersion(major,minor);
  cerr << "opengl version " << major << "." << minor << "\n";

  timer->StartTimer();
  int numRenders = timeit ? 600 : 8;
  for (int i = 0; i < numRenders; ++i)
  {
    renderer->GetActiveCamera()->Azimuth(80.0/numRenders);
    renderer->GetActiveCamera()->Elevation(80.0/numRenders);
    renderWindow->Render();
  }
  timer->StopTimer();
  double elapsed = timer->GetElapsedTime();
  cerr << "interactive render time: " << elapsed / numRenders << endl;
  unsigned int numTris = reader->GetOutput()->GetPolys()->GetNumberOfCells();
  cerr << "number of triangles: " <<  numTris << endl;
  cerr << "triangles per second: " <<  numTris*(numRenders/elapsed) << endl;


  renderer->GetActiveCamera()->SetPosition(0,0,1);
  renderer->GetActiveCamera()->SetFocalPoint(0,0,0);
  renderer->GetActiveCamera()->SetViewUp(0,1,0);
  renderer->ResetCamera();
  renderWindow->Render();
  renderWindow->Render();

  int retVal = vtkRegressionTestImage( renderWindow.Get() );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
