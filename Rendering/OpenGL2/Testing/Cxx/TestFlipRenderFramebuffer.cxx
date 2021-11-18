/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkActor.h"
#include "vtkAxesActor.h"
#include "vtkConeSource.h"
#include "vtkNew.h"
#include "vtkOpenGLPolyDataMapper.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"
#include "vtkTextActor.h"
#include "vtkTextProperty.h"

//------------------------------------------------------------------------------
int TestFlipRenderFramebuffer(int argc, char* argv[])
{
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(600, 600);
  vtkOpenGLRenderWindow::SafeDownCast(renderWindow)->FramebufferFlipYOn();

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renderWindow.Get());

  renderWindow->SetNumberOfLayers(2);

  vtkNew<vtkRenderer> renderer;
  renderWindow->AddRenderer(renderer);

  vtkNew<vtkRenderer> overlay;
  renderWindow->AddRenderer(overlay);
  overlay->SetLayer(1);
  overlay->SetViewport(0, 0, 0.4, 0.4);

  // cone
  vtkNew<vtkConeSource> source;
  source->SetDirection(0, 1, 0);

  vtkNew<vtkOpenGLPolyDataMapper> mapper;
  mapper->SetInputConnection(source->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  renderer->AddActor(actor);

  // text actor
  vtkNew<vtkTextActor> textActor;
  textActor->SetInput("FlipY Tests");
  textActor->GetTextProperty()->SetFontSize(30);
  renderer->AddActor(textActor);

  // axes actor overlay
  vtkNew<vtkAxesActor> axes;
  overlay->AddActor(axes);

  renderer->ResetCamera();
  renderWindow->Render();

  int retVal = vtkRegressionTestImage(renderWindow.Get());
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
