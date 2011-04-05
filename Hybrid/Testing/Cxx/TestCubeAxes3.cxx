/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCubeAxes3.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// This example illustrates how one may explicitly specify the range of each
// axes that's used to define the prop, while displaying data with a different
// set of bounds (unlike cubeAxes2.tcl). This example allows you to separate
// the notion of extent of the axes in physical space (bounds) and the extent
// of the values it represents. In other words, you can have the ticks and
// labels show a different range.

#include "vtkBYUReader.h"
#include "vtkCamera.h"
#include "vtkCubeAxesActor.h"
#include "vtkLight.h"
#include "vtkLODActor.h"
#include "vtkNew.h"
#include "vtkOutlineFilter.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyDataNormals.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"



//----------------------------------------------------------------------------
int TestCubeAxes3( int argc, char * argv [] )
{
  vtkNew<vtkBYUReader> fohe;
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/teapot.g");
  fohe->SetGeometryFileName(fname);
  delete [] fname;

  vtkNew<vtkPolyDataNormals> normals;
  normals->SetInputConnection(fohe->GetOutputPort());

  vtkNew<vtkPolyDataMapper> foheMapper;
  foheMapper->SetInputConnection(normals->GetOutputPort());

  vtkNew<vtkLODActor> foheActor;
  foheActor->SetMapper(foheMapper.GetPointer());

  vtkNew<vtkOutlineFilter> outline;
  outline->SetInputConnection(normals->GetOutputPort());

  vtkNew<vtkPolyDataMapper> mapOutline;
  mapOutline->SetInputConnection(outline->GetOutputPort());

  vtkNew<vtkActor> outlineActor;
  outlineActor->SetMapper(mapOutline.GetPointer());
  outlineActor->GetProperty()->SetColor(0.0 ,0.0 ,0.0);

  vtkNew<vtkCamera> camera;
  camera->SetClippingRange(1.60187, 20.0842);
  camera->SetFocalPoint(0.21406, 1.5, 0.0);
  camera->SetPosition(11.63, 6.32, 5.77);
  camera->SetViewUp(0.180325, 0.549245, -0.815974);

  vtkNew<vtkLight> light;
  light->SetFocalPoint(0.21406, 1.5, 0.0);
  light->SetPosition(8.3761, 4.94858, 4.12505);

  vtkNew<vtkRenderer> ren2;
  ren2->SetActiveCamera(camera.GetPointer());
  ren2->AddLight(light.GetPointer());

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  renWin->AddRenderer(ren2.GetPointer());
  renWin->SetWindowName("VTK - Cube Axes custom range");
  renWin->SetSize(600, 600);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin.GetPointer());

  ren2->AddViewProp(foheActor.GetPointer());
  ren2->AddViewProp(outlineActor.GetPointer());
  ren2->SetBackground(0.1, 0.2, 0.4);

  normals->Update();

  vtkNew<vtkCubeAxesActor> axes2;
  axes2->SetBounds(normals->GetOutput()->GetBounds());
  axes2->SetXAxisRange(20, 300);
  axes2->SetYAxisRange(-0.01, 0.01);
  axes2->SetCamera(ren2->GetActiveCamera());
  axes2->SetXLabelFormat("%6.1f");
  axes2->SetYLabelFormat("%6.1f");
  axes2->SetZLabelFormat("%6.1f");
  axes2->SetFlyModeToClosestTriad();

  axes2->DrawXGridlinesOn();
  axes2->DrawYGridlinesOn();
  axes2->DrawZGridlinesOn();

  ren2->AddViewProp(axes2.GetPointer());
  renWin->Render();

  int retVal = vtkRegressionTestImage( renWin.GetPointer() );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  return !retVal;
}
