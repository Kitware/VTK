/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDeflectNormals.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkActor.h"
#include "vtkDeflectNormals.h"
#include "vtkGeometryFilter.h"
#include "vtkGradientFilter.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkRTAnalyticSource.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

int TestDeflectNormals(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  vtkNew<vtkRTAnalyticSource> wavelet;
  wavelet->SetWholeExtent(-100, 100, -100, 100, 0, 0);

  vtkNew<vtkGradientFilter> gradient;
  gradient->SetInputConnection(wavelet->GetOutputPort());
  gradient->SetResultArrayName("Deflector");

  vtkNew<vtkGeometryFilter> surface;
  surface->SetInputConnection(gradient->GetOutputPort());

  // User normal deflector
  vtkNew<vtkDeflectNormals> deflect1;
  deflect1->SetInputConnection(surface->GetOutputPort());
  deflect1->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "Deflector");
  deflect1->SetScaleFactor(0.2);
  deflect1->UseUserNormalOn();
  deflect1->SetUserNormal(0.0, 0.0, 1.0);

  vtkNew<vtkPolyDataMapper> mapper1;
  mapper1->ScalarVisibilityOff();
  mapper1->SetInputConnection(deflect1->GetOutputPort());

  vtkNew<vtkActor> actor1;
  actor1->SetMapper(mapper1);

  vtkNew<vtkRenderer> renderer1;
  renderer1->SetViewport(0.0, 0.0, 0.5, 1.0);
  renderer1->AddActor(actor1);

  // Using point data normals
  vtkNew<vtkDeflectNormals> deflect2;
  deflect2->SetInputConnection(surface->GetOutputPort());
  deflect2->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "Deflector");
  deflect2->SetScaleFactor(0.8);
  deflect2->UseUserNormalOff();

  vtkNew<vtkPolyDataMapper> mapper2;
  mapper2->ScalarVisibilityOff();
  mapper2->SetInputConnection(deflect2->GetOutputPort());

  vtkNew<vtkActor> actor2;
  actor2->SetMapper(mapper2);

  vtkNew<vtkRenderer> renderer2;
  renderer2->SetViewport(0.5, 0.0, 1.0, 1.0);
  renderer2->AddActor(actor2);

  // render
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(600, 300);
  renWin->AddRenderer(renderer1);
  renWin->AddRenderer(renderer2);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  renWin->Render();
  iren->Start();

  return EXIT_SUCCESS;
}
