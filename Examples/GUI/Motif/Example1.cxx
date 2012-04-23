/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Example1.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// include OS specific include file to mix in X code

#include "vtkActor.h"
#include "vtkConeSource.h"
#include "vtkGlyph3D.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkXRenderWindowInteractor.h"

#include <Xm/PushB.h>

void quit_cb(Widget,XtPointer,XtPointer);

int main (int argc, char *argv[])
{
  // X window stuff
  XtAppContext app;
  Widget toplevel, button;
  Display *display;

  // VTK stuff
  vtkRenderWindow *renWin;
  vtkRenderer *ren1;
  vtkActor *sphereActor1, *spikeActor1;
  vtkSphereSource *sphere;
  vtkConeSource *cone;
  vtkGlyph3D *glyph;
  vtkPolyDataMapper *sphereMapper, *spikeMapper;
  vtkXRenderWindowInteractor *iren;

  renWin = vtkRenderWindow::New();
  ren1 = vtkRenderer::New();
  renWin->AddRenderer(ren1);

  sphere = vtkSphereSource::New();
  sphereMapper = vtkPolyDataMapper::New();
  sphereMapper->SetInputConnection(sphere->GetOutputPort());
  sphereActor1 = vtkActor::New();
  sphereActor1->SetMapper(sphereMapper);
  cone = vtkConeSource::New();
  glyph = vtkGlyph3D::New();
  glyph->SetInputConnection(sphere->GetOutputPort());
  glyph->SetSourceConnection(cone->GetOutputPort());
  glyph->SetVectorModeToUseNormal();
  glyph->SetScaleModeToScaleByVector();
  glyph->SetScaleFactor(0.25);
  spikeMapper = vtkPolyDataMapper::New();
  spikeMapper->SetInputConnection(glyph->GetOutputPort());
  spikeActor1 = vtkActor::New();
  spikeActor1->SetMapper(spikeMapper);
  ren1->AddActor(sphereActor1);
  ren1->AddActor(spikeActor1);
  ren1->SetBackground(0.4,0.1,0.2);

  // do the xwindow ui stuff
  XtSetLanguageProc(NULL,NULL,NULL);
  toplevel = XtVaAppInitialize(&app,"Sample",NULL,0,
                               &argc,argv,NULL, static_cast<void *>(NULL));

  // get the display connection and give it to the renderer
  display = XtDisplay(toplevel);
  renWin->SetDisplayId(display);

  // We use an X specific interactor
  // since we have decided to make this an X program
  iren = vtkXRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);
  iren->Initialize(app);

  button = XtVaCreateManagedWidget("Exit",
                                   xmPushButtonWidgetClass,
                                   toplevel,
                                   XmNwidth, 50,
                                   XmNheight, 50, static_cast<void *>(NULL));

  XtRealizeWidget(toplevel);
  XtAddCallback(button,XmNactivateCallback,quit_cb,NULL);
  XtAppMainLoop(app);

  return 0;
}

// simple quit callback
void quit_cb(Widget vtkNotUsed(w),XtPointer vtkNotUsed(client_data),
             XtPointer vtkNotUsed(call_data))
{
  exit(0);
}
