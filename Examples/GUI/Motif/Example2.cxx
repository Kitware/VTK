/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Example2.cxx

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
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkXOpenGLRenderWindow.h"
#include "vtkXRenderWindowInteractor.h"

#include <Xm/PushB.h>
#include <Xm/Form.h>
void quit_cb(Widget,XtPointer,XtPointer);
main (int argc, char *argv[])
{
  // X window stuff
  XtAppContext app;
  Widget toplevel, form, toplevel2, vtk;
  Widget button;
  int depth;
  Visual *vis;
  Display *display;
  Colormap col;
  
  // VTK stuff
  vtkXOpenGLRenderWindow *renWin;
  vtkRenderer *ren1;
  vtkActor *sphereActor1, *spikeActor1;
  vtkSphereSource *sphere;
  vtkConeSource *cone;
  vtkGlyph3D *glyph;
  vtkPolyDataMapper *sphereMapper, *spikeMapper;
  vtkXRenderWindowInteractor *iren;
  
  renWin = vtkXOpenGLRenderWindow::New();
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
  glyph->SetSource(cone->GetOutput());
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
                               &argc,argv,NULL, NULL);


  // get the display connection and give it to the renderer
  display = XtDisplay(toplevel);
  renWin->SetDisplayId(display);
  depth = renWin->GetDesiredDepth();
  vis = renWin->GetDesiredVisual();
  col = renWin->GetDesiredColormap();
  
  toplevel2 = XtVaCreateWidget("top2",
       topLevelShellWidgetClass,toplevel,
                               XmNdepth, depth,
                               XmNvisual, vis,
                               XmNcolormap, col,
                               NULL);


  form     = XtVaCreateWidget("form",xmFormWidgetClass, toplevel2, NULL);
  vtk      = XtVaCreateManagedWidget("vtk",
     xmPrimitiveWidgetClass, form, 
                                     XmNwidth, 300, XmNheight, 300,
                                     XmNleftAttachment, XmATTACH_FORM,
                                     XmNrightAttachment, XmATTACH_FORM,
                                     XmNtopAttachment, XmATTACH_FORM,
                                     NULL);
  button    = XtVaCreateManagedWidget("Exit",
      xmPushButtonWidgetClass, form,
                                      XmNheight, 40,
                                      XmNbottomAttachment, XmATTACH_FORM,
                                      XmNtopAttachment, XmATTACH_WIDGET,
                                      XmNtopWidget, vtk,
                                      XmNleftAttachment, XmATTACH_FORM,
                                      XmNrightAttachment, XmATTACH_FORM,
                                      NULL);
  XtAddCallback(button,XmNactivateCallback,quit_cb,NULL);
  XtManageChild(form);
  XtRealizeWidget(toplevel2);
  XtMapWidget(toplevel2);
  
  // we typecast to an X specific interactor
  // Since we have decided to make this an X program
  iren = vtkXRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);
  iren->SetWidget(vtk);
  iren->Initialize(app);
  XtAppMainLoop(app);
}
/* quit when the arrow */
void quit_cb(Widget w,XtPointer client_data, XtPointer call_data)
{
  exit(0);
}
