/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOSPRayPointCloudWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Description
// This test covers the use of point cloud widget with the OSPRay rendering backend

#include <vtkCommand.h>
#include <vtkNew.h>
#include <vtkOSPRayPass.h>
#include <vtkOSPRayRendererNode.h>
#include <vtkPointCloudRepresentation.h>
#include <vtkPointCloudWidget.h>
#include <vtkPointSource.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkTestUtilities.h>
#include <vtkTesting.h>

#include "vtkOSPRayTestInteractor.h"

// Callback for the interaction
class vtkOSPRayPCCallback : public vtkCommand
{
public:
  static vtkOSPRayPCCallback* New() { return new vtkOSPRayPCCallback; }
  void Execute(vtkObject* caller, unsigned long eid, void*) override
  {
    vtkPointCloudWidget* pcWidget = reinterpret_cast<vtkPointCloudWidget*>(caller);
    vtkPointCloudRepresentation* pcRep =
      reinterpret_cast<vtkPointCloudRepresentation*>(pcWidget->GetRepresentation());
    switch (eid)
    {
      case vtkCommand::PickEvent:
      {
        std::cout << "Point Id {0}: " << pcRep->GetPointId() << std::endl;
        break;
      }
      case vtkCommand::WidgetActivateEvent:
      {
        vtkIdType pId = pcRep->GetPointId();
        std::cout << "Selected Point Id {0}: " << pId << std::endl;
        double* pt = this->Source->GetOutput()->GetPoints()->GetPoint(pId);
        std::cout << "Point Coordinates {0}: " << pt[0] << ", " << pt[1] << ", " << pt[2]
                  << std::endl;
        break;
      }
      default:
      {
        break;
      }
    }
  }
  vtkOSPRayPCCallback()
    : Source(nullptr)
  {
  }
  vtkPointSource* Source;
};

const char TestOSPRayPointCloudWidgetLog[] = "# StreamVersion 1.1\n"
                                             "ExposeEvent 0 299 0 0 0 0\n"
                                             "MouseMoveEvent 117 91 0 0 0 0\n"
                                             "MouseMoveEvent 135 105 0 0 0 0\n"
                                             "LeftButtonPressEvent 135 105 0 0 0 0\n"
                                             "MouseMoveEvent 135 105 0 0 0 0\n"
                                             "MouseMoveEvent 110 161 0 0 0 0\n"
                                             "LeftButtonReleaseEvent 110 161 0 0 0 0\n"
                                             "MouseMoveEvent 110 161 0 0 0 0\n"
                                             "MouseMoveEvent 115 131 0 0 0 0\n"
                                             "RightButtonPressEvent 115 131 0 0 0 0\n"
                                             "MouseMoveEvent 115 132 0 0 0 0\n"
                                             "MouseMoveEvent 110 253 0 0 0 0\n"
                                             "RightButtonReleaseEvent 110 253 0 0 0 0\n"
                                             "MouseMoveEvent 112 253 0 0 0 0\n"
                                             "MouseMoveEvent 147 172 0 0 0 0\n"
                                             "RightButtonPressEvent 147 172 0 0 0 0\n"
                                             "MouseMoveEvent 147 171 0 0 0 0\n"
                                             "MouseMoveEvent 219 92 0 0 0 0\n"
                                             "RightButtonReleaseEvent 219 92 0 0 0 0\n"
                                             "MouseMoveEvent 219 93 0 0 0 0\n"
                                             "MouseMoveEvent 218 112 0 0 0 0\n"
                                             "";

int TestOSPRayPointCloudWidget(int argc, char* argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  // Control the size of the test
  int npts = 10000;

  // Create the RenderWindow, Renderer and both Actors
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

  // Create a point source
  vtkNew<vtkPointSource> pc;
  pc->SetNumberOfPoints(npts);
  pc->SetCenter(5, 10, 20);
  pc->SetRadius(7.5);
  pc->Update();

  // Conveniently the representation creates an actor/mapper
  // to render the point cloud.
  vtkNew<vtkPointCloudRepresentation> rep;
  rep->SetPlaceFactor(1.0);
  rep->PlacePointCloud(pc->GetOutput());
  rep->SetPickingModeToSoftware();

  // Configure the box widget including callbacks
  vtkNew<vtkOSPRayPCCallback> myCallback;
  myCallback->Source = pc;

  vtkNew<vtkPointCloudWidget> pcWidget;
  pcWidget->SetInteractor(iren);
  pcWidget->SetRepresentation(rep);
  pcWidget->AddObserver(vtkCommand::PickEvent, myCallback);
  pcWidget->AddObserver(vtkCommand::WidgetActivateEvent, myCallback);
  pcWidget->On();

  renderer->SetBackground(0, 0, 0);
  renWin->SetSize(300, 300);
  vtkNew<vtkOSPRayPass> ospray;
  renderer->SetPass(ospray);

  vtkNew<vtkOSPRayTestInteractor> style;
  style->SetPipelineControlPoints(renderer, ospray, nullptr);
  iren->SetInteractorStyle(style);
  style->SetCurrentRenderer(renderer);

  // interact with data
  // render the image
  //
  renderer->ResetCamera();
  iren->Initialize();
  renWin->Render();

  return vtkTesting::InteractorEventLoop(argc, argv, iren, TestOSPRayPointCloudWidgetLog);
}
