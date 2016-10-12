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
#include "vtkSphereSource.h"
#include "vtkPolyDataReader.h"
#include "vtkInteractorStyleSwitch.h"
#include "vtkArrayCalculator.h"
#include "vtkPointData.h"

// If USE_FILTER is defined, glyph3D->PolyDataMapper is used instead of
// Glyph3DMapper.
//#define USE_FILTER

#ifdef USE_FILTER
# include "vtkGlyph3D.h"
#else
# include "vtkGlyph3DMapper.h"
#endif

int TestGlyph3DMapperMasking(int argc, char *argv[])
{
  int res=30;
  vtkPlaneSource *plane=vtkPlaneSource::New();
  plane->SetResolution(res,res);
  vtkElevationFilter *colors=vtkElevationFilter::New();
  colors->SetInputConnection(plane->GetOutputPort());
  colors->SetLowPoint(-0.25,-0.25,-0.25);
  colors->SetHighPoint(0.25,0.25,0.25);
  vtkPolyDataMapper *planeMapper=vtkPolyDataMapper::New();
  planeMapper->SetInputConnection(colors->GetOutputPort());
  colors->Delete();

  vtkArrayCalculator *calc=vtkArrayCalculator::New();
  calc->SetInputConnection(colors->GetOutputPort());
  calc->SetResultArrayName("mask");
  calc->SetResultArrayType(VTK_BIT);
  calc->AddScalarArrayName("Elevation");
  calc->SetFunction("Elevation>0.2 & Elevation<0.4");
  calc->Update();

  calc->GetOutput()->GetPointData()->GetArray("mask");
  calc->GetOutput()->GetPointData()->SetActiveScalars("Elevation");

  vtkActor *planeActor=vtkActor::New();
  planeActor->SetMapper(planeMapper);
  planeMapper->Delete();
  planeActor->GetProperty()->SetRepresentationToWireframe();

// create simple poly data so we can apply glyph
  // vtkSuperquadricSource *squad=vtkSuperquadricSource::New();
   vtkSphereSource *squad=vtkSphereSource::New();
   squad->SetPhiResolution(45);
   squad->SetThetaResolution(45);

#ifdef USE_FILTER
  vtkGlyph3D *glypher=vtkGlyph3D::New();
  glypher->SetInputConnection(colors->GetOutputPort());
#else
  vtkGlyph3DMapper *glypher=vtkGlyph3DMapper::New();
  glypher->SetNestedDisplayLists(0);
  glypher->SetMasking(1);
  glypher->SetMaskArray("mask");
  glypher->SetInputConnection(calc->GetOutputPort());
  //glypher->SetInputConnection(colors->GetOutputPort());
  calc->Delete();
#endif
  //  glypher->SetScaleModeToDataScalingOn();
  glypher->SetScaleFactor(0.1);

  //glypher->SetInputConnection(plane->GetOutputPort());
  glypher->SetSourceConnection(squad->GetOutputPort());
  squad->Delete();
  plane->Delete();

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
  win->SetMultiSamples(0); // make sure regression images are the same on all platforms
  win->AddRenderer(ren);
  ren->Delete();
  vtkRenderWindowInteractor *iren=vtkRenderWindowInteractor::New();
  vtkInteractorStyleSwitch::SafeDownCast(iren->GetInteractorStyle())->SetCurrentStyleToTrackballCamera();

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
