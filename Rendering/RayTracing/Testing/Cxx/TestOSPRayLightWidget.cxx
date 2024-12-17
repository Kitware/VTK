// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**Description
 * Test the use of light widget with a ray traced backend
 */

#include "vtkTesting.h"

#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkInteractorEventRecorder.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkLight.h>
#include <vtkLightRepresentation.h>
#include <vtkLightWidget.h>
#include <vtkNew.h>
#include <vtkOSPRayLightNode.h>
#include <vtkOSPRayPass.h>
#include <vtkOSPRayRendererNode.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSphereSource.h>

#include "vtkOSPRayTestInteractor.h"

// Callback for the interaction
class vtkOSPRayLWCallback : public vtkCommand
{
public:
  static vtkOSPRayLWCallback* New() { return new vtkOSPRayLWCallback; }
  void Execute(vtkObject* caller, unsigned long, void*) override
  {
    vtkLightWidget* lw = reinterpret_cast<vtkLightWidget*>(caller);
    if (!lw)
    {
      return;
    }
    vtkLight* l = this->Light;
    if (lw == this->LightWidget2)
    {
      l = this->Light2;
    }

    vtkLightRepresentation* lr = lw->GetLightRepresentation();
    if (l != nullptr)
    {
      l->SetPosition(lr->GetLightPosition());
      l->SetFocalPoint(lr->GetFocalPoint());
      l->SetConeAngle(lr->GetConeAngle());
    }
  }
  vtkOSPRayLWCallback()
    : LightWidget2(nullptr)
    , Light(nullptr)
    , Light2(nullptr)
  {
  }
  vtkLightWidget* LightWidget2;
  vtkLight* Light;
  vtkLight* Light2;
};

int TestOSPRayLightWidget(int argc, char* argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;
  bool useOSP = true;
  for (int i = 0; i < argc; i++)
  {
    if (!strcmp(argv[i], "-GL"))
    {
      cerr << "GL" << endl;
      useOSP = false;
    }
  }

  // Create a renderer, render window and interactor
  vtkNew<vtkRenderer> ren;
  ren->SetBackground(0.1, 0.2, 0.4);
  ren->SetAutomaticLightCreation(false);
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(300, 300);
  renWin->AddRenderer(ren);

  vtkNew<vtkSphereSource> sphere;
  sphere->SetThetaResolution(25);
  sphere->SetPhiResolution(25);
  sphere->SetRadius(10);
  vtkNew<vtkPolyDataMapper> sphereMapper;
  sphereMapper->SetInputConnection(sphere->GetOutputPort());
  vtkNew<vtkActor> sphereActor;
  sphereActor->SetMapper(sphereMapper);
  // sphereActor->GetProperty()->SetOpacity(0.6);
  ren->AddActor(sphereActor);

  vtkOSPRayLightNode::SetLightScale(3);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  vtkNew<vtkLight> light;
  light->SetPosition(-10, 20, 10);
  light->SetFocalPoint(0, 0, 0);
  light->SetPositional(true);
  light->SetIntensity(40);
  light->SetConeAngle(20);
  ren->AddLight(light);
  vtkNew<vtkLightRepresentation> lightRep;
  lightRep->SetLightPosition(light->GetPosition());
  lightRep->SetConeAngle(light->GetConeAngle());
  lightRep->SetPositional(true);
  lightRep->GetProperty()->SetLineWidth(3);

  vtkNew<vtkLightWidget> lightWidget;
  lightWidget->SetInteractor(iren);
  lightWidget->SetRepresentation(lightRep);
  lightWidget->On();

  double color[3] = { 1.0, 1.0, 0.0 };
  vtkNew<vtkLight> light2;
  light2->SetPosition(10, -20, -10);
  light2->SetFocalPoint(0, 0, 0);
  light2->SetColor(color);
  ren->AddLight(light2);
  vtkNew<vtkLightRepresentation> lightRep2;
  lightRep2->SetLightPosition(light2->GetPosition());
  lightRep2->SetLightColor(color);
  lightRep2->GetProperty()->SetLineWidth(3);

  vtkNew<vtkLightWidget> lightWidget2;
  lightWidget2->SetInteractor(iren);
  lightWidget2->SetRepresentation(lightRep2);
  lightWidget2->On();

  vtkNew<vtkOSPRayLWCallback> cb;
  cb->Light = light;
  cb->Light2 = light2;
  cb->LightWidget2 = lightWidget2;
  lightWidget->AddObserver(vtkCommand::InteractionEvent, cb);
  lightWidget2->AddObserver(vtkCommand::InteractionEvent, cb);

  renWin->Render();
  ren->ResetCamera();
  renWin->Render();

  if (useOSP)
  {
    vtkNew<vtkOSPRayPass> ospray;
    ren->SetPass(ospray);

    vtkNew<vtkOSPRayTestInteractor> style;
    style->SetPipelineControlPoints(ren, ospray, nullptr);
    iren->SetInteractorStyle(style);
    style->SetCurrentRenderer(ren);
    vtkOSPRayRendererNode::SetRendererType("OSPRay raycaster", ren);
    vtkOSPRayRendererNode::SetMaxFrames(0, ren);
    vtkOSPRayRendererNode::SetSamplesPerPixel(20, ren);
  }

  ren->ResetCamera();
  iren->Initialize();
  renWin->Render();

  return vtkTesting::InteractorEventLoop(argc, argv, iren);
}
