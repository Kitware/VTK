/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"

#include "vtkRenderWindowInteractor.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkPlaneSource.h"
#include "vtkElevationFilter.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkSuperquadricSource.h"

// If USE_FILTER is defined, glyph3D->PolyDataMapper is used instead of
// Glyph3DMapper.
//#define USE_FILTER

#ifdef USE_FILTER
# include "vtkGlyph3D.h"
#else
# include "vtkGlyph3DMapper.h"
#endif

#include "vtkPlane.h"

int TestGlyph3DMapper(int argc, char *argv[])
{
  int res=6;
  vtkPlaneSource *plane=vtkPlaneSource::New();
  plane->SetResolution(res,res);
  vtkElevationFilter *colors=vtkElevationFilter::New();
  colors->SetInputConnection(plane->GetOutputPort());
  plane->Delete();
  colors->SetLowPoint(-0.25,-0.25,-0.25);
  colors->SetHighPoint(0.25,0.25,0.25);
  vtkPolyDataMapper *planeMapper=vtkPolyDataMapper::New();
  planeMapper->SetInputConnection(colors->GetOutputPort());
  colors->Delete();

  vtkActor *planeActor=vtkActor::New();
  planeActor->SetMapper(planeMapper);
  planeMapper->Delete();
  planeActor->GetProperty()->SetRepresentationToWireframe();

// create simple poly data so we can apply glyph
  vtkSuperquadricSource *squad=vtkSuperquadricSource::New();

#ifdef USE_FILTER
  vtkGlyph3D *glypher=vtkGlyph3D::New();
#else
  vtkGlyph3DMapper *glypher=vtkGlyph3DMapper::New();
#endif
  glypher->SetInputConnection(colors->GetOutputPort());
  glypher->SetSourceConnection(squad->GetOutputPort());
  squad->Delete();

  // Useful code should you want to test clipping planes
  // with a glyph mapper, might shoudl just uncomment
  // this and add a new valid image
  // vtkPlane *cplane = vtkPlane::New();
  // cplane->SetNormal(-0.5,0.5,0);
  // cplane->SetOrigin(0.2,0,0);
  // glypher->AddClippingPlane(cplane);

#ifdef USE_FILTER
  vtkPolyDataMapper *glyphMapper=vtkPolyDataMapper::New();
  glyphMapper->SetInputConnection(glypher->GetOutputPort());
#endif

  vtkActor *glyphActor=vtkActor::New();
#ifdef USE_FILTER
  glyphActor->SetMapper(glyphMapper);
  glyphMapper->Delete();
#else
  glyphActor->SetMapper(glypher);
#endif
  glypher->Delete();

  //Create the rendering stuff

  vtkRenderer *ren=vtkRenderer::New();
  vtkRenderWindow *win=vtkRenderWindow::New();
  win->AddRenderer(ren);
  ren->Delete();
  vtkRenderWindowInteractor *iren=vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(win);
  win->Delete();

  ren->AddActor(planeActor);
  planeActor->Delete();
  ren->AddActor(glyphActor);
  glyphActor->Delete();
  ren->SetBackground(0.5,0.5,0.5);
  win->SetSize(450,450);
  win->Render();
  ren->GetActiveCamera()->Zoom(1.5);

  win->Render();

  int retVal = vtkRegressionTestImage(win);
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  iren->Delete();

  return !retVal;
}
