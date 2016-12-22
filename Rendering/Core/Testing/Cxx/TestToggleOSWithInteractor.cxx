/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOSConeCxx.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test covers offscreen rendering.
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit

#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"

#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"

// test works on Windows, in the future we need to
// make sure it works for OSX and Linux/EGL
#ifdef WIN32
int TestToggleOSWithInteractor(int argc, char* argv[])
{
  // run through a couple cases

  vtkNew<vtkSphereSource> sphere;
  sphere->SetRadius( 10.0 );

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection( sphere->GetOutputPort() );

  vtkNew<vtkActor> actor;
  actor->SetMapper( mapper.Get() );

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor( actor.Get() );

  {
    vtkNew<vtkRenderWindow> renderWindow;
    renderWindow->AddRenderer( renderer.Get() );

    // 1) Try calling SupportsOpenGL to make sure that
    // doesn't crash
    renderWindow->SupportsOpenGL();

    vtkNew<vtkRenderWindowInteractor> interactor;
    interactor->SetRenderWindow( renderWindow.Get() );

    interactor->Initialize();

    // 2) try toggling offscreen rendering on and off
    renderWindow->OffScreenRenderingOn();
    renderWindow->Render();
    renderWindow->OffScreenRenderingOff();
    renderWindow->Render();
  }

  {
    // 3) try doing it again with a new window
    // but using existing old actor/rederer
    vtkNew<vtkRenderWindow> renderWindow;
    renderWindow->AddRenderer( renderer.Get() );

    vtkNew<vtkRenderWindowInteractor> interactor;
    interactor->SetRenderWindow( renderWindow.Get() );

    interactor->Initialize();

    renderWindow->OffScreenRenderingOn();
    renderWindow->Render();
    renderWindow->OffScreenRenderingOff();
    renderWindow->Render();

    // 4) try doing it again with offscreenbuffers
    renderWindow->SetUseOffScreenBuffers(true);
    renderWindow->OffScreenRenderingOn();
    renderWindow->Render();
    renderWindow->OffScreenRenderingOff();
    renderWindow->Render();
  }

  int retVal = 0;

  {
    // 5) try doing it again with a new everything
    vtkNew<vtkActor> actor2;
    actor2->SetMapper( mapper.Get() );
    actor2->GetProperty()->SetAmbient(1.0);
    actor2->GetProperty()->SetDiffuse(0.0);

    vtkNew<vtkRenderer> renderer2;
    renderer2->AddActor( actor2.Get() );

    vtkNew<vtkRenderWindow> renderWindow;
    renderWindow->AddRenderer( renderer2.Get() );

    vtkNew<vtkRenderWindowInteractor> interactor;
    interactor->SetRenderWindow( renderWindow.Get() );

    interactor->Initialize();

    renderWindow->OffScreenRenderingOn();
    renderWindow->SupportsOpenGL();
    renderWindow->Render();
    renderWindow->OffScreenRenderingOff();
    renderWindow->Render();

    retVal = vtkRegressionTestImage( renderWindow.Get() );
    if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
      interactor->Start();
    }

  }

  return !retVal;
#else
int TestToggleOSWithInteractor(int, char* [])
{
  return 0;
#endif
}
