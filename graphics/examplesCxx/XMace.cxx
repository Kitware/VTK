#include "vtk.h"

// include OS specific include file to mix in X code
#include "vtkXRenderWindowInteractor.h"
#include <Xm/ArrowB.h>
#include <Xm/Form.h>

void quit_cb(Widget,XtPointer,XtPointer);

main (int argc, char *argv[])
{
  // X window stuff
  XtAppContext app;
  Widget toplevel, form;
  Widget button[4];

  vtkRenderer *vtkRenderer *ren1 = vtkRenderer::New();
  vtkRenderWindow *vtkRenderWindow *renWin = vtkRenderWindow::New();
    renWin->AddRenderer(ren1);

  vtkSphereSource *sphere = vtkSphereSource::New();
    sphere->SetThetaResolution(8); sphere->SetPhiResolution(8);
  vtkPolyDataMapper *sphereMapper = vtkPolyDataMapper::New();
    sphereMapper->SetInput(sphere->GetOutput());
  vtkActor *sphereActor1 = vtkActor::New();
    sphereActor1->SetMapper(sphereMapper);

  vtkConeSource *cone = vtkConeSource::New();
    cone->SetResolution(6);

  vtkGlyph3D *glyph = vtkGlyph3D::New();
    glyph->SetInput(sphere->GetOutput());
    glyph->SetSource(cone->GetOutput());
    glyph->SetVectorModeToUseNormal();
    glyph->SetScaleModeToScaleByVector();
    glyph->SetScaleFactor(0.25);

  vtkPolyDataMapper *spikeMapper = vtkPolyDataMapper::New();
    spikeMapper->SetInput(glyph->GetOutput());

  vtkActor *spikeActor1 = vtkActor::New();
    spikeActor1->SetMapper(spikeMapper);

  ren1->AddActor(sphereActor1);
  ren1->AddActor(spikeActor1);
  ren1->SetBackground(0.4,0.1,0.2);

  // do the xwindow ui stuff
  XtSetLanguageProc(NULL,NULL,NULL);
  toplevel = XtVaAppInitialize(&app,"Prog6",NULL,0,&argc,argv,NULL,NULL);
  form     = XtVaCreateWidget("form",xmFormWidgetClass, toplevel, NULL);

  button[0] = XtVaCreateManagedWidget("arrow1",xmArrowButtonWidgetClass, form,
				      XmNarrowDirection, XmARROW_LEFT,
				      XmNwidth, 50,
				      XmNheight, 50,
				      XmNbottomAttachment, XmATTACH_FORM,
				      XmNtopAttachment, XmATTACH_FORM,
				      XmNleftAttachment, XmATTACH_POSITION,
				      XmNleftPosition, 0,
				      XmNrightAttachment, XmATTACH_POSITION,
				      XmNrightPosition, 25,
				      NULL);

  XtAddCallback(button[0],XmNactivateCallback,quit_cb,NULL);

  XtManageChild(form);
  XtRealizeWidget(toplevel);

  // we typecast to an X specific interactor
  // Since we have specifically decided to make this 
  // an X windows program
  vtkXRenderWindowInteractor *iren = 
            (vtkXRenderWindowInteractor *)vtkRenderWindowInteractor::New();
            iren->SetRenderWindow(renWin);
  iren->Initialize(app);
  renWin->Render();

  XtAppMainLoop(app);

  // Clean up
  ren1->Delete();
  renWin->Delete();
  sphere->Delete();
  sphereMapper->Delete();
  sphereActor1->Delete();
  cone->Delete();
  glyph->Delete();
  spikeMapper->Delete();
  spikeActor1->Delete();
  vtkXRenderWindowInteractor->Delete();
}

/* quit when the arrow */
void quit_cb(Widget w,XtPointer client_data, XtPointer call_data)
{
  exit(0);
}

