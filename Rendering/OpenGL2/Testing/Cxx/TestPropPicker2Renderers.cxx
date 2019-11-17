/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestImageTracerWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkActor.h>
#include <vtkAssemblyPath.h>
#include <vtkCamera.h>
#include <vtkCubeSource.h>
#include <vtkInteractorEventRecorder.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkPolyDataNormals.h>
#include <vtkPolyDataReader.h>
#include <vtkPropPicker.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkRendererCollection.h>
#include <vtkSphereSource.h>

bool corner = true;
// bool corner = false;

double sphereColor[3] = { 0.73, 0.33, 0.83 };
double sphereColorPicked[3] = { 1.0, 1., 0.0 };
double sphereColor2[3] = { 0.33, 0.73, 0.83 };

// Handle mouse events
class MouseInteractorStyle2 : public vtkInteractorStyleTrackballCamera
{
public:
  static MouseInteractorStyle2* New();
  vtkTypeMacro(MouseInteractorStyle2, vtkInteractorStyleTrackballCamera);

  void OnLeftButtonDown() override
  {
    int* clickPos = this->GetInteractor()->GetEventPosition();

    vtkRenderWindow* renwin = this->GetInteractor()->GetRenderWindow();
    vtkRenderer* aren = this->GetInteractor()->FindPokedRenderer(clickPos[0], clickPos[1]);

    vtkNew<vtkPropPicker> picker2;
    if (0 != picker2->Pick(clickPos[0], clickPos[1], 0, aren))
    {
      vtkAssemblyPath* path = picker2->GetPath();
      vtkProp* prop = path->GetFirstNode()->GetViewProp();
      vtkActor* actor = vtkActor::SafeDownCast(prop);
      actor->GetProperty()->SetColor(sphereColorPicked);
    }
    else
      renwin->SetCurrentCursor(VTK_CURSOR_DEFAULT);

    renwin->Render();
  }

private:
};

vtkStandardNewMacro(MouseInteractorStyle2);

void InitRepresentation(vtkRenderer* renderer)
{
  // Sphere
  vtkNew<vtkSphereSource> sphereSource;
  sphereSource->SetPhiResolution(24);
  sphereSource->SetThetaResolution(24);
  sphereSource->SetRadius(1.75);
  sphereSource->Update();

  vtkNew<vtkActor> sphere;
  vtkNew<vtkPolyDataMapper> sphereM;
  sphereM->SetInputConnection(sphereSource->GetOutputPort());
  sphereM->Update();
  sphere->SetMapper(sphereM);
  sphere->GetProperty()->BackfaceCullingOff();
  sphere->GetProperty()->SetColor(sphereColor);
  sphere->SetPosition(0, 0, 2);
  renderer->AddActor(sphere);
}

const char PropPickerEventLog[] = "# StreamVersion 1.1\n"
                                  "LeftButtonPressEvent 160 150 0 0 0 0\n"
                                  "LeftButtonReleaseEvent 160 150 0 0 0 0\n";

int TestPropPicker2Renderers(int, char*[])
{
  vtkNew<vtkRenderer> renderer0;
  renderer0->SetUseDepthPeeling(1);
  renderer0->SetMaximumNumberOfPeels(8);
  renderer0->LightFollowCameraOn();
  renderer0->TwoSidedLightingOn();
  renderer0->SetOcclusionRatio(0.0);

  renderer0->GetActiveCamera()->SetParallelProjection(1);

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetAlphaBitPlanes(1);
  renWin->SetMultiSamples(0);
  renWin->AddRenderer(renderer0);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);
  iren->LightFollowCameraOff();

  // Set the custom stype to use for interaction.
  vtkNew<MouseInteractorStyle2> istyle;

  iren->SetInteractorStyle(istyle);

  if (corner) // corner
  {
    vtkNew<vtkRenderer> renderer1;
    renderer1->SetViewport(0, 0, 0.1, 0.1);
    renWin->AddRenderer(renderer1);

    vtkNew<vtkSphereSource> sphereSource;
    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputConnection(sphereSource->GetOutputPort());
    mapper->Update();

    vtkNew<vtkActor> actor;
    actor->PickableOff();
    actor->SetMapper(mapper);
    renderer1->AddActor(actor);
  }

  {
    vtkNew<vtkCubeSource> reader;
    reader->SetXLength(80);
    reader->SetYLength(50);
    reader->SetZLength(1);
    reader->Update();

    vtkNew<vtkPolyDataNormals> norm;
    norm->SetInputConnection(reader->GetOutputPort());
    norm->ComputePointNormalsOn();
    norm->SplittingOff();
    norm->Update();

    vtkNew<vtkPolyDataMapper> mapper;
    mapper->ScalarVisibilityOff();
    mapper->SetResolveCoincidentTopologyToPolygonOffset();
    mapper->SetInputConnection(norm->GetOutputPort());
    mapper->Update();

    vtkNew<vtkActor> actor;
    actor->SetMapper(mapper);
    actor->GetProperty()->BackfaceCullingOff();
    actor->GetProperty()->SetColor(0.93, 0.5, 0.5);

    {
      renderer0->AddActor(actor);

      InitRepresentation(renderer0);

      renderer0->ResetCameraClippingRange();
      renderer0->ResetCamera();

      istyle->SetDefaultRenderer(renderer0);
    }

    actor->PickableOff();
  }
  renWin->SetSize(300, 300);

  vtkNew<vtkInteractorEventRecorder> recorder;
  recorder->SetInteractor(iren);
  recorder->ReadFromInputStringOn();
  recorder->SetInputString(PropPickerEventLog);

  renWin->Render();
  recorder->Play();
  // Remove the observers so we can go interactive. Without this the "-I"
  // testing option fails.
  recorder->Off();

  iren->Start();

  return EXIT_SUCCESS;
}
