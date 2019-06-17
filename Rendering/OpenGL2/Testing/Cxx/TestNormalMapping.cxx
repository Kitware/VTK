/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestNormalMapping.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test covers the normal mapping feature
// Texture credits:
// Julian Herzog, CC BY 4.0 (https://creativecommons.org/licenses/by/4.0/)
// The image has been cropped and resized

#include "vtkActor.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkLight.h"
#include "vtkNew.h"
#include "vtkOpenGLPolyDataMapper.h"
#include "vtkPNGReader.h"
#include "vtkPlaneSource.h"
#include "vtkPolyDataTangents.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"
#include "vtkTexture.h"
#include "vtkTriangleFilter.h"

//----------------------------------------------------------------------------
int TestNormalMapping(int argc, char* argv[])
{
  vtkNew<vtkRenderer> renderer;
  renderer->AutomaticLightCreationOff();

  vtkNew<vtkLight> light;
  light->SetPosition(0.5, 0.5, 1.0);
  light->SetFocalPoint(0.0, 0.0, 0.0);

  renderer->AddLight(light);

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(600, 600);
  renWin->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  vtkNew<vtkPlaneSource> plane;

  vtkNew<vtkTriangleFilter> triangulation;
  triangulation->SetInputConnection(plane->GetOutputPort());

  vtkNew<vtkPolyDataTangents> tangents;
  tangents->SetInputConnection(triangulation->GetOutputPort());

  vtkNew<vtkPNGReader> png;
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/normalMapping.png");
  png->SetFileName(fname);
  delete[] fname;

  vtkNew<vtkTexture> texture;
  texture->SetInputConnection(png->GetOutputPort());

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(tangents->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  actor->GetProperty()->SetNormalTexture(texture);
  renderer->AddActor(actor);

  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
