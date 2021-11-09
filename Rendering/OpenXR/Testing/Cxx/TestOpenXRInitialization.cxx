/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkActor.h"
#include "vtkCullerCollection.h"
#include "vtkLight.h"
#include "vtkNew.h"
#include "vtkOpenGLPolyDataMapper.h"
#include "vtkOpenGLVertexBufferObject.h"
#include "vtkOpenXRCamera.h"
#include "vtkOpenXRRenderWindow.h"
#include "vtkOpenXRRenderWindowInteractor.h"
#include "vtkOpenXRRenderer.h"
#include "vtkPLYReader.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"
#include "vtkTransform.h"
#include "vtkTransformPolyDataFilter.h"

#include <memory>

//------------------------------------------------------------------------------
int TestOpenXRInitialization(int argc, char* argv[])
{
  vtkNew<vtkOpenXRRenderer> renderer;
  renderer->SetShowFloor(true);

  vtkNew<vtkOpenXRRenderWindow> renderWindow;
  vtkNew<vtkOpenXRRenderWindowInteractor> iren;
  vtkNew<vtkOpenXRCamera> cam;

  renderer->SetBackground(0.2, 0.3, 0.4);
  renderWindow->AddRenderer(renderer);
  vtkNew<vtkActor> actor;
  renderer->AddActor(actor);
  iren->SetRenderWindow(renderWindow);
  renderer->SetActiveCamera(cam);

  renderer->RemoveCuller(renderer->GetCullers()->GetLastItem());

  vtkNew<vtkLight> light;
  light->SetLightTypeToSceneLight();
  light->SetPosition(1.0, 1.0, 1.0);
  renderer->AddLight(light);

  const char* fileName = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/dragon.ply");
  vtkNew<vtkPLYReader> reader;
  reader->SetFileName(fileName);
  delete[] fileName;

  vtkNew<vtkTransform> trans;
  trans->Translate(20.0, 0.0, 0.0);
  trans->Scale(0.001, 0.001, 0.001);
  vtkNew<vtkTransformPolyDataFilter> tf;
  tf->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);
  tf->SetTransform(trans);
  tf->SetInputConnection(reader->GetOutputPort());

  vtkNew<vtkOpenGLPolyDataMapper> mapper;
  mapper->SetInputConnection(tf->GetOutputPort());
  mapper->SetVBOShiftScaleMethod(vtkOpenGLVertexBufferObject::AUTO_SHIFT_SCALE);
  actor->SetMapper(mapper);
  actor->GetProperty()->SetAmbientColor(0.2, 0.2, 1.0);
  actor->GetProperty()->SetDiffuseColor(1.0, 0.65, 0.7);
  actor->GetProperty()->SetSpecularColor(1.0, 1.0, 1.0);
  actor->GetProperty()->SetSpecular(0.5);
  actor->GetProperty()->SetDiffuse(0.7);
  actor->GetProperty()->SetAmbient(0.5);
  actor->GetProperty()->SetSpecularPower(20.0);
  actor->GetProperty()->SetOpacity(1.0);

  renderer->ResetCamera();

  iren->Start();

  return 0;
}
