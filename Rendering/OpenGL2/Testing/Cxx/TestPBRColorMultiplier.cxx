/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPBRColorMultiplier.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCubeSource.h"
#include "vtkImageData.h"
#include "vtkNew.h"
#include "vtkPBRIrradianceTexture.h"
#include "vtkPBRLUTTexture.h"
#include "vtkPBRPrefilterTexture.h"
#include "vtkPNGReader.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"
#include "vtkTexture.h"

//------------------------------------------------------------------------------
int TestPBRColorMultiplier(int argc, char* argv[])
{
  vtkNew<vtkRenderer> renderer;

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(600, 600);
  renWin->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  vtkNew<vtkCubeSource> cube;

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(cube->GetOutputPort());

  vtkNew<vtkPNGReader> albedoReader;
  char* colname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/vtk_Base_Color.png");
  albedoReader->SetFileName(colname);
  delete[] colname;

  vtkNew<vtkTexture> albedo;
  albedo->UseSRGBColorSpaceOn();
  albedo->InterpolateOn();
  albedo->SetInputConnection(albedoReader->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->SetOrientation(0.0, 25.0, 0.0);
  actor->SetMapper(mapper);
  actor->GetProperty()->SetInterpolationToPBR();

  actor->GetProperty()->SetColor(1.0, 1.0, 0.0);
  actor->GetProperty()->SetOpacity(0.5);

  actor->GetProperty()->SetBaseColorTexture(albedo);

  renderer->AddActor(actor);

  renWin->Render();

  renderer->GetActiveCamera()->Zoom(1.5);
  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
