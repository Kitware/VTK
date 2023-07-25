// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Description
// This test covers the use of box widget with the OSPRay rendering backend

#include <vtkAppendPolyData.h>
#include <vtkBoxRepresentation.h>
#include <vtkBoxWidget2.h>
#include <vtkCommand.h>
#include <vtkConeSource.h>
#include <vtkGlyph3D.h>
#include <vtkInteractorEventRecorder.h>
#include <vtkNew.h>
#include <vtkOSPRayPass.h>
#include <vtkOSPRayRendererNode.h>
#include <vtkPolyDataMapper.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSphereSource.h>
#include <vtkTestUtilities.h>
#include <vtkTesting.h>
#include <vtkTransform.h>

#include "vtkOSPRayTestInteractor.h"

// Callback for the interaction
class vtkOSPRayBWCallback2 : public vtkCommand
{
public:
  static vtkOSPRayBWCallback2* New() { return new vtkOSPRayBWCallback2; }
  void Execute(vtkObject* caller, unsigned long, void*) override
  {
    vtkBoxWidget2* boxWidget = reinterpret_cast<vtkBoxWidget2*>(caller);
    vtkBoxRepresentation* boxRep =
      reinterpret_cast<vtkBoxRepresentation*>(boxWidget->GetRepresentation());
    boxRep->GetTransform(this->Transform);
    this->Actor->SetUserTransform(this->Transform);
  }
  vtkOSPRayBWCallback2()
    : Transform(nullptr)
    , Actor(nullptr)
  {
  }
  vtkTransform* Transform;
  vtkActor* Actor;
};

const char TestOSPRayBoxWidgetEventLog2[] = "# StreamVersion 1.1\n"
                                            "EnterEvent 224 25 0 0 0 0\n"
                                            "MouseMoveEvent 237 147 0 0 0 0\n"
                                            "LeftButtonPressEvent 237 147 0 0 0 0\n"
                                            "MouseMoveEvent 237 146 0 0 0 0\n"
                                            "MouseMoveEvent 161 145 0 0 0 0\n"
                                            "LeftButtonReleaseEvent 161 145 0 0 0 0\n"
                                            "MouseMoveEvent 160 145 0 0 0 0\n"
                                            "MouseMoveEvent 113 233 0 0 0 0\n"
                                            "LeftButtonPressEvent 113 233 0 0 0 0\n"
                                            "MouseMoveEvent 113 232 0 0 0 0\n"
                                            "MouseMoveEvent 119 161 0 0 0 0\n"
                                            "LeftButtonReleaseEvent 119 161 0 0 0 0\n"
                                            "LeftButtonPressEvent 99 109 0 0 0 0\n"
                                            "MouseMoveEvent 100 109 0 0 0 0\n"
                                            "MouseMoveEvent 108 115 0 0 0 0\n"
                                            "MouseMoveEvent 125 130 0 0 0 0\n"
                                            "MouseMoveEvent 140 155 0 0 0 0\n"
                                            "MouseMoveEvent 154 179 0 0 0 0\n"
                                            "LeftButtonReleaseEvent 154 179 0 0 0 0\n"
                                            "LeftButtonPressEvent 125 85 0 0 0 0\n"
                                            "MouseMoveEvent 126 85 0 0 0 0\n"
                                            "MouseMoveEvent 179 36 0 0 0 0\n"
                                            "LeftButtonReleaseEvent 179 36 0 0 0 0\n"
                                            "MiddleButtonPressEvent 111 104 0 0 0 0\n"
                                            "MouseMoveEvent 110 105 0 0 0 0\n"
                                            "MouseMoveEvent 180 195 0 0 0 0\n"
                                            "MiddleButtonReleaseEvent 180 195 0 0 0 0\n"
                                            "LeftButtonPressEvent 119 117 0 0 0 0\n"
                                            "MouseMoveEvent 121 117 0 0 0 0\n"
                                            "MouseMoveEvent 175 157 0 0 0 0\n"
                                            "LeftButtonReleaseEvent 175 157 0 0 0 0\n"
                                            "MiddleButtonPressEvent 196 209 0 0 0 0\n"
                                            "MouseMoveEvent 187 201 0 0 0 0\n"
                                            "MouseMoveEvent 146 147 0 0 0 0\n"
                                            "MiddleButtonReleaseEvent 146 147 0 0 0 0\n"
                                            "RightButtonPressEvent 246 92 0 0 0 0\n"
                                            "MouseMoveEvent 247 96 0 0 0 0\n"
                                            "MouseMoveEvent 232 231 0 0 0 0\n"
                                            "RightButtonReleaseEvent 232 231 0 0 0 0\n"
                                            "";
int TestOSPRayBoxWidget2(int argc, char* argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  vtkNew<vtkRenderWindowInteractor> iren;
  vtkNew<vtkRenderWindow> renWin;
  iren->SetRenderWindow(renWin);
  vtkNew<vtkRenderer> renderer;
  renWin->AddRenderer(renderer);
  vtkOSPRayRendererNode::SetSamplesPerPixel(16, renderer);

  for (int i = 0; i < argc; ++i)
  {
    if (!strcmp(argv[i], "--OptiX"))
    {
      vtkOSPRayRendererNode::SetRendererType("optix pathtracer", renderer);
      break;
    }
  }

  vtkNew<vtkConeSource> cone;
  cone->SetResolution(6);
  vtkNew<vtkSphereSource> sphere;
  sphere->SetThetaResolution(8);
  sphere->SetPhiResolution(8);
  vtkNew<vtkGlyph3D> glyph;
  glyph->SetInputConnection(sphere->GetOutputPort());
  glyph->SetSourceConnection(cone->GetOutputPort());
  glyph->SetVectorModeToUseNormal();
  glyph->SetScaleModeToScaleByVector();
  glyph->SetScaleFactor(0.25);
  glyph->Update();

  vtkNew<vtkAppendPolyData> append;
  append->AddInputConnection(glyph->GetOutputPort());
  append->AddInputConnection(sphere->GetOutputPort());

  vtkNew<vtkPolyDataMapper> maceMapper;
  maceMapper->SetInputConnection(append->GetOutputPort());

  vtkNew<vtkActor> maceActor;
  maceActor->SetMapper(maceMapper);

  // Configure the box widget including callbacks
  vtkNew<vtkTransform> t;
  vtkNew<vtkOSPRayBWCallback2> myCallback;
  myCallback->Transform = t;
  myCallback->Actor = maceActor;

  vtkNew<vtkBoxRepresentation> boxRep;
  boxRep->SetPlaceFactor(1.25);
  boxRep->PlaceWidget(glyph->GetOutput()->GetBounds());

  vtkNew<vtkBoxWidget2> boxWidget;
  boxWidget->SetInteractor(iren);
  boxWidget->SetRepresentation(boxRep);
  boxWidget->AddObserver(vtkCommand::InteractionEvent, myCallback);
  boxWidget->SetPriority(1);

  renderer->AddActor(maceActor);
  renderer->SetBackground(0, 0, 0);
  renWin->SetSize(300, 300);
  vtkNew<vtkOSPRayPass> ospray;
  renderer->SetPass(ospray);

  vtkNew<vtkOSPRayTestInteractor> style;
  style->SetPipelineControlPoints(renderer, ospray, nullptr);
  iren->SetInteractorStyle(style);
  style->SetCurrentRenderer(renderer);

  boxRep->SetPlaceFactor(1.0);
  boxRep->HandlesOff();

  boxRep->SetPlaceFactor(1.25);
  boxRep->HandlesOn();
  boxWidget->On();

  // interact with data
  // render the image
  //
  renderer->ResetCamera();
  iren->Initialize();
  renWin->Render();

  renWin->Render();

  return vtkTesting::InteractorEventLoop(argc, argv, iren, TestOSPRayBoxWidgetEventLog2);
}
