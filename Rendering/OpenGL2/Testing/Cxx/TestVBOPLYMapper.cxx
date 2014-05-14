/*=========================================================================

  Program:   Visualization Toolkit

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
#include "vtkActor.h"
#include "vtkCellArray.h"
#include "vtkPointData.h"
#include "vtkVBOPolyDataMapper.h"
#include "vtkPLYReader.h"
#include "vtkNew.h"
#include "vtkProperty.h"
#include "vtkLightKit.h"
#include "vtkPolyDataNormals.h"
#include "vtkTimerLog.h"

#include "vtkRegressionTestImage.h"
#include "vtkTestUtilities.h"

//----------------------------------------------------------------------------
int TestVBOPLYMapper(int argc, char *argv[])
{
  vtkNew<vtkActor> actor;
  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkPolyDataMapper> mapper;
  renderer->SetBackground(0.0, 0.0, 0.0);
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(300, 300);
  renderWindow->AddRenderer(renderer.Get());
  renderer->AddActor(actor.Get());
  vtkNew<vtkLightKit> lightKit;
  lightKit->AddLightsToRenderer(renderer.Get());

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

  vtkNew<vtkRenderWindowInteractor> interactor;
  interactor->SetRenderWindow(renderWindow.Get());
  renderWindow->SetMultiSamples(0);
  interactor->Initialize();

  vtkNew<vtkTimerLog> timer;
  double time(0.0);
  for (int i = 0; i < 10; ++i)
    {
    timer->StartTimer();
    renderWindow->Render();
    timer->StopTimer();
    cout << "Rendering frame " << i << ": " << timer->GetElapsedTime() << endl;
    time += timer->GetElapsedTime();
    }
  cout << "Average time: " << time / 10.0 << endl;

  interactor->Start();

  delete [] fileName;

  return EXIT_SUCCESS;
}
