/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestImplicitPlaneWidget.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImplicitPlaneWidget.h"
#include "vtkSphereSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkAppendPolyData.h"
#include "vtkClipPolyData.h"
#include "vtkConeSource.h"
#include "vtkGlyph3D.h"
#include "vtkActor.h"
#include "vtkLODActor.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkCommand.h"
#include "vtkPlane.h"
#include "vtkProperty.h"

// This does the actual work: updates the vtkPlane implicit function.
// This in turn causes the pipeline to update and clip the object.
// Callback for the interaction
class vtkMyCallback : public vtkCommand
{
public:
  static vtkMyCallback *New() 
    { return new vtkMyCallback; }
  virtual void Execute(vtkObject *caller, unsigned long, void*)
    {
      vtkImplicitPlaneWidget *planeWidget = 
        reinterpret_cast<vtkImplicitPlaneWidget*>(caller);
      planeWidget->GetPlane(this->Plane);
      this->Actor->VisibilityOn();
    }
  vtkMyCallback():Plane(0),Actor(0) {}
  vtkPlane *Plane;
  vtkActor *Actor;

};

int main( int argc, char *argv[] )
{
  // Create a mace out of filters.
  //
  vtkSphereSource *sphere = vtkSphereSource::New();
  vtkConeSource *cone = vtkConeSource::New();
  vtkGlyph3D *glyph = vtkGlyph3D::New();
  glyph->SetInput(sphere->GetOutput());
  glyph->SetSource(cone->GetOutput());
  glyph->SetVectorModeToUseNormal();
  glyph->SetScaleModeToScaleByVector();
  glyph->SetScaleFactor(0.25);

  // The sphere and spikes are appended into a single polydata. 
  // This just makes things simpler to manage.
  vtkAppendPolyData *apd = vtkAppendPolyData::New();
  apd->AddInput(glyph->GetOutput());
  apd->AddInput(sphere->GetOutput());

  vtkPolyDataMapper *maceMapper = vtkPolyDataMapper::New();
  maceMapper->SetInput(apd->GetOutput());

  vtkLODActor *maceActor = vtkLODActor::New();
  maceActor->SetMapper(maceMapper);
  maceActor->VisibilityOn();

  // This portion of the code clips the mace with the vtkPlanes 
  // implicit function. The clipped region is colored green.
  vtkPlane *plane = vtkPlane::New();
  vtkClipPolyData *clipper = vtkClipPolyData::New();
  clipper->SetInput (apd->GetOutput());
  clipper->SetClipFunction(plane);
  clipper->InsideOutOn();

  vtkPolyDataMapper *selectMapper = vtkPolyDataMapper::New();
  selectMapper->SetInput(clipper->GetOutput());

  vtkLODActor *selectActor = vtkLODActor::New();
  selectActor->SetMapper(selectMapper);
  selectActor->GetProperty()->SetColor(0,1,0);
  selectActor->VisibilityOff();
  selectActor->SetScale(1.01, 1.01, 1.01);

  // Create the RenderWindow, Renderer and both Actors
  //
  vtkRenderer *ren1 = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  renWin->AddRenderer(ren1);

  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  // The SetInteractor method is how 3D widgets are associated with the render
  // window interactor. Internally, SetInteractor sets up a bunch of callbacks
  // using the Command/Observer mechanism (AddObserver()).
  vtkMyCallback *myCallback = vtkMyCallback::New();
  myCallback->Plane = plane;
  myCallback->Actor = selectActor;

  vtkImplicitPlaneWidget *planeWidget = vtkImplicitPlaneWidget::New();
  planeWidget->SetInteractor(iren);
  planeWidget->SetPlaceFactor(1.25);
  planeWidget->SetInput (glyph->GetOutput());
  planeWidget->PlaceWidget();

  ren1->AddActor(maceActor);
  ren1->AddActor(selectActor);

  // Add the actors to the renderer, set the background and size
  //
  ren1->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(300, 300);

  // record events
  //vtkInteractorEventRecorder *recorder = vtkInteractorEventRecorder::New();
  //recorder->SetInteractor(iren);

  // render the image
  //
  iren->Initialize();
  iren->Start();

  sphere->Delete();
  cone->Delete();
  glyph->Delete();
  apd->Delete();
  maceMapper->Delete();
  maceActor->Delete();
  plane->Delete();
  clipper->Delete();
  selectMapper->Delete();
  selectActor->Delete();
  planeWidget->Delete();
  iren->Delete();
  renWin->Delete();
  ren1->Delete();
  myCallback->Delete();
  
  return 0;
}




