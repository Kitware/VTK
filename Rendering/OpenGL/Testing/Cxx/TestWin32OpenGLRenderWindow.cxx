/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestWin32OpenGLRenderWindow.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkTestUtilities.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkActor.h"
#include "vtkConeSource.h"
#include "vtkWin32OpenGLRenderWindow.h"
#include <vtkConeSource.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkNew.h>

//This tests if the offscreen rendering works
static bool TestWin32OpenGLRenderWindowOffScreen(vtkWin32OpenGLRenderWindow* renderWindow)
{
  //Create a cone
  vtkNew<vtkConeSource> coneSource;
  coneSource->Update();

  //Create a mapper and actor
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(coneSource->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper.GetPointer());

  //Create a renderer, render window, and interactor
  vtkNew<vtkRenderer> renderer;
  renderWindow->AddRenderer(renderer.GetPointer());

  //Add the actors to the scene
  renderer->AddActor(actor.GetPointer());
  renderer->SetBackground(.3, .2, .1); // Background color dark red

  //Render and interact
  renderWindow->SetOffScreenRendering(1);
  renderWindow->SetSize(100,100);
  renderWindow->Render();
  return 1;
}

int TestWin32OpenGLRenderWindow(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  vtkWin32OpenGLRenderWindow* renderWindow(NULL);

  vtkNew<vtkRenderWindow> renderWindowBase;
  renderWindow = vtkWin32OpenGLRenderWindow::SafeDownCast(renderWindowBase.GetPointer());

  TestWin32OpenGLRenderWindowOffScreen(renderWindow);

  return EXIT_SUCCESS;
}
