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
                                             "MouseMoveEvent 128 96 0 0 0 0\n"
                                             "MouseMoveEvent 135 103 0 0 0 0\n"
                                             "MouseMoveEvent 135 105 0 0 0 0\n"
                                             "LeftButtonPressEvent 135 105 0 0 0 0\n"
                                             "MouseMoveEvent 135 105 0 0 0 0\n"
                                             "MouseMoveEvent 135 107 0 0 0 0\n"
                                             "MouseMoveEvent 135 109 0 0 0 0\n"
                                             "MouseMoveEvent 135 110 0 0 0 0\n"
                                             "MouseMoveEvent 135 113 0 0 0 0\n"
                                             "MouseMoveEvent 135 114 0 0 0 0\n"
                                             "MouseMoveEvent 135 116 0 0 0 0\n"
                                             "MouseMoveEvent 134 118 0 0 0 0\n"
                                             "MouseMoveEvent 134 120 0 0 0 0\n"
                                             "MouseMoveEvent 134 121 0 0 0 0\n"
                                             "MouseMoveEvent 134 124 0 0 0 0\n"
                                             "MouseMoveEvent 134 125 0 0 0 0\n"
                                             "MouseMoveEvent 133 126 0 0 0 0\n"
                                             "MouseMoveEvent 133 128 0 0 0 0\n"
                                             "MouseMoveEvent 133 129 0 0 0 0\n"
                                             "MouseMoveEvent 133 131 0 0 0 0\n"
                                             "MouseMoveEvent 133 133 0 0 0 0\n"
                                             "MouseMoveEvent 133 134 0 0 0 0\n"
                                             "MouseMoveEvent 133 135 0 0 0 0\n"
                                             "MouseMoveEvent 132 136 0 0 0 0\n"
                                             "MouseMoveEvent 132 138 0 0 0 0\n"
                                             "MouseMoveEvent 132 139 0 0 0 0\n"
                                             "MouseMoveEvent 132 143 0 0 0 0\n"
                                             "MouseMoveEvent 131 145 0 0 0 0\n"
                                             "MouseMoveEvent 131 146 0 0 0 0\n"
                                             "MouseMoveEvent 131 148 0 0 0 0\n"
                                             "MouseMoveEvent 131 149 0 0 0 0\n"
                                             "MouseMoveEvent 131 150 0 0 0 0\n"
                                             "MouseMoveEvent 131 151 0 0 0 0\n"
                                             "MouseMoveEvent 130 151 0 0 0 0\n"
                                             "MouseMoveEvent 129 153 0 0 0 0\n"
                                             "MouseMoveEvent 128 154 0 0 0 0\n"
                                             "MouseMoveEvent 127 155 0 0 0 0\n"
                                             "MouseMoveEvent 125 157 0 0 0 0\n"
                                             "MouseMoveEvent 123 159 0 0 0 0\n"
                                             "MouseMoveEvent 122 160 0 0 0 0\n"
                                             "MouseMoveEvent 120 161 0 0 0 0\n"
                                             "MouseMoveEvent 119 161 0 0 0 0\n"
                                             "MouseMoveEvent 118 162 0 0 0 0\n"
                                             "MouseMoveEvent 117 163 0 0 0 0\n"
                                             "MouseMoveEvent 116 163 0 0 0 0\n"
                                             "MouseMoveEvent 115 163 0 0 0 0\n"
                                             "MouseMoveEvent 114 163 0 0 0 0\n"
                                             "MouseMoveEvent 113 163 0 0 0 0\n"
                                             "MouseMoveEvent 112 162 0 0 0 0\n"
                                             "MouseMoveEvent 111 162 0 0 0 0\n"
                                             "MouseMoveEvent 110 161 0 0 0 0\n"
                                             "LeftButtonReleaseEvent 110 161 0 0 0 0\n"
                                             "MouseMoveEvent 110 161 0 0 0 0\n"
                                             "MouseMoveEvent 110 160 0 0 0 0\n"
                                             "MouseMoveEvent 110 158 0 0 0 0\n"
                                             "MouseMoveEvent 110 155 0 0 0 0\n"
                                             "MouseMoveEvent 110 154 0 0 0 0\n"
                                             "MouseMoveEvent 110 149 0 0 0 0\n"
                                             "MouseMoveEvent 110 144 0 0 0 0\n"
                                             "MouseMoveEvent 110 141 0 0 0 0\n"
                                             "MouseMoveEvent 111 138 0 0 0 0\n"
                                             "MouseMoveEvent 112 135 0 0 0 0\n"
                                             "MouseMoveEvent 114 132 0 0 0 0\n"
                                             "MouseMoveEvent 115 131 0 0 0 0\n"
                                             "RightButtonPressEvent 115 131 0 0 0 0\n"
                                             "MouseMoveEvent 115 132 0 0 0 0\n"
                                             "MouseMoveEvent 115 135 0 0 0 0\n"
                                             "MouseMoveEvent 114 137 0 0 0 0\n"
                                             "MouseMoveEvent 113 141 0 0 0 0\n"
                                             "MouseMoveEvent 112 146 0 0 0 0\n"
                                             "MouseMoveEvent 110 149 0 0 0 0\n"
                                             "MouseMoveEvent 109 152 0 0 0 0\n"
                                             "MouseMoveEvent 108 155 0 0 0 0\n"
                                             "MouseMoveEvent 108 159 0 0 0 0\n"
                                             "MouseMoveEvent 108 162 0 0 0 0\n"
                                             "MouseMoveEvent 108 167 0 0 0 0\n"
                                             "MouseMoveEvent 108 171 0 0 0 0\n"
                                             "MouseMoveEvent 108 175 0 0 0 0\n"
                                             "MouseMoveEvent 108 177 0 0 0 0\n"
                                             "MouseMoveEvent 108 179 0 0 0 0\n"
                                             "MouseMoveEvent 109 183 0 0 0 0\n"
                                             "MouseMoveEvent 110 184 0 0 0 0\n"
                                             "MouseMoveEvent 110 187 0 0 0 0\n"
                                             "MouseMoveEvent 110 198 0 0 0 0\n"
                                             "MouseMoveEvent 110 208 0 0 0 0\n"
                                             "MouseMoveEvent 110 217 0 0 0 0\n"
                                             "MouseMoveEvent 110 222 0 0 0 0\n"
                                             "MouseMoveEvent 110 228 0 0 0 0\n"
                                             "MouseMoveEvent 110 233 0 0 0 0\n"
                                             "MouseMoveEvent 110 235 0 0 0 0\n"
                                             "MouseMoveEvent 110 240 0 0 0 0\n"
                                             "MouseMoveEvent 110 242 0 0 0 0\n"
                                             "MouseMoveEvent 110 244 0 0 0 0\n"
                                             "MouseMoveEvent 110 245 0 0 0 0\n"
                                             "MouseMoveEvent 110 250 0 0 0 0\n"
                                             "MouseMoveEvent 110 253 0 0 0 0\n"
                                             "RightButtonReleaseEvent 110 253 0 0 0 0\n"
                                             "MouseMoveEvent 112 253 0 0 0 0\n"
                                             "MouseMoveEvent 117 246 0 0 0 0\n"
                                             "MouseMoveEvent 124 234 0 0 0 0\n"
                                             "MouseMoveEvent 126 229 0 0 0 0\n"
                                             "MouseMoveEvent 132 218 0 0 0 0\n"
                                             "MouseMoveEvent 139 206 0 0 0 0\n"
                                             "MouseMoveEvent 143 200 0 0 0 0\n"
                                             "MouseMoveEvent 144 199 0 0 0 0\n"
                                             "MouseMoveEvent 146 197 0 0 0 0\n"
                                             "MouseMoveEvent 147 197 0 0 0 0\n"
                                             "MouseMoveEvent 149 196 0 0 0 0\n"
                                             "MouseMoveEvent 154 195 0 0 0 0\n"
                                             "MouseMoveEvent 164 191 0 0 0 0\n"
                                             "MouseMoveEvent 165 190 0 0 0 0\n"
                                             "MouseMoveEvent 167 188 0 0 0 0\n"
                                             "MouseMoveEvent 167 187 0 0 0 0\n"
                                             "MouseMoveEvent 169 183 0 0 0 0\n"
                                             "MouseMoveEvent 169 181 0 0 0 0\n"
                                             "MouseMoveEvent 169 177 0 0 0 0\n"
                                             "MouseMoveEvent 169 170 0 0 0 0\n"
                                             "MouseMoveEvent 169 165 0 0 0 0\n"
                                             "MouseMoveEvent 169 162 0 0 0 0\n"
                                             "MouseMoveEvent 167 155 0 0 0 0\n"
                                             "MouseMoveEvent 161 147 0 0 0 0\n"
                                             "MouseMoveEvent 149 134 0 0 0 0\n"
                                             "MouseMoveEvent 145 132 0 0 0 0\n"
                                             "MouseMoveEvent 143 130 0 0 0 0\n"
                                             "MouseMoveEvent 138 127 0 0 0 0\n"
                                             "MouseMoveEvent 137 126 0 0 0 0\n"
                                             "MouseMoveEvent 135 122 0 0 0 0\n"
                                             "MouseMoveEvent 133 119 0 0 0 0\n"
                                             "MouseMoveEvent 132 116 0 0 0 0\n"
                                             "MouseMoveEvent 131 115 0 0 0 0\n"
                                             "MouseMoveEvent 130 114 0 0 0 0\n"
                                             "MouseMoveEvent 129 113 0 0 0 0\n"
                                             "MouseMoveEvent 128 111 0 0 0 0\n"
                                             "MouseMoveEvent 126 110 0 0 0 0\n"
                                             "MouseMoveEvent 124 109 0 0 0 0\n"
                                             "MouseMoveEvent 121 107 0 0 0 0\n"
                                             "MouseMoveEvent 119 106 0 0 0 0\n"
                                             "MouseMoveEvent 109 103 0 0 0 0\n"
                                             "MouseMoveEvent 107 101 0 0 0 0\n"
                                             "MouseMoveEvent 104 100 0 0 0 0\n"
                                             "MouseMoveEvent 103 100 0 0 0 0\n"
                                             "MouseMoveEvent 102 99 0 0 0 0\n"
                                             "MouseMoveEvent 100 97 0 0 0 0\n"
                                             "MouseMoveEvent 98 96 0 0 0 0\n"
                                             "MouseMoveEvent 96 94 0 0 0 0\n"
                                             "MouseMoveEvent 94 90 0 0 0 0\n"
                                             "MouseMoveEvent 93 86 0 0 0 0\n"
                                             "MouseMoveEvent 93 85 0 0 0 0\n"
                                             "MouseMoveEvent 93 82 0 0 0 0\n"
                                             "MouseMoveEvent 93 81 0 0 0 0\n"
                                             "MouseMoveEvent 96 79 0 0 0 0\n"
                                             "MouseMoveEvent 101 77 0 0 0 0\n"
                                             "MouseMoveEvent 105 77 0 0 0 0\n"
                                             "MouseMoveEvent 108 77 0 0 0 0\n"
                                             "MouseMoveEvent 111 77 0 0 0 0\n"
                                             "MouseMoveEvent 126 77 0 0 0 0\n"
                                             "MouseMoveEvent 134 77 0 0 0 0\n"
                                             "MouseMoveEvent 141 77 0 0 0 0\n"
                                             "MouseMoveEvent 142 78 0 0 0 0\n"
                                             "MouseMoveEvent 143 79 0 0 0 0\n"
                                             "MouseMoveEvent 144 80 0 0 0 0\n"
                                             "MouseMoveEvent 144 81 0 0 0 0\n"
                                             "MouseMoveEvent 144 82 0 0 0 0\n"
                                             "MouseMoveEvent 144 83 0 0 0 0\n"
                                             "MouseMoveEvent 144 86 0 0 0 0\n"
                                             "MouseMoveEvent 145 88 0 0 0 0\n"
                                             "MouseMoveEvent 145 89 0 0 0 0\n"
                                             "MouseMoveEvent 145 91 0 0 0 0\n"
                                             "MouseMoveEvent 147 94 0 0 0 0\n"
                                             "MouseMoveEvent 147 96 0 0 0 0\n"
                                             "MouseMoveEvent 147 97 0 0 0 0\n"
                                             "MouseMoveEvent 148 99 0 0 0 0\n"
                                             "MouseMoveEvent 148 104 0 0 0 0\n"
                                             "MouseMoveEvent 148 106 0 0 0 0\n"
                                             "MouseMoveEvent 149 108 0 0 0 0\n"
                                             "MouseMoveEvent 149 112 0 0 0 0\n"
                                             "MouseMoveEvent 149 118 0 0 0 0\n"
                                             "MouseMoveEvent 149 122 0 0 0 0\n"
                                             "MouseMoveEvent 149 134 0 0 0 0\n"
                                             "MouseMoveEvent 149 139 0 0 0 0\n"
                                             "MouseMoveEvent 149 142 0 0 0 0\n"
                                             "MouseMoveEvent 149 146 0 0 0 0\n"
                                             "MouseMoveEvent 149 151 0 0 0 0\n"
                                             "MouseMoveEvent 149 155 0 0 0 0\n"
                                             "MouseMoveEvent 147 165 0 0 0 0\n"
                                             "MouseMoveEvent 147 171 0 0 0 0\n"
                                             "MouseMoveEvent 147 172 0 0 0 0\n"
                                             "RightButtonPressEvent 147 172 0 0 0 0\n"
                                             "MouseMoveEvent 147 171 0 0 0 0\n"
                                             "MouseMoveEvent 148 165 0 0 0 0\n"
                                             "MouseMoveEvent 153 157 0 0 0 0\n"
                                             "MouseMoveEvent 161 145 0 0 0 0\n"
                                             "MouseMoveEvent 168 135 0 0 0 0\n"
                                             "MouseMoveEvent 173 128 0 0 0 0\n"
                                             "MouseMoveEvent 177 122 0 0 0 0\n"
                                             "MouseMoveEvent 179 120 0 0 0 0\n"
                                             "MouseMoveEvent 182 117 0 0 0 0\n"
                                             "MouseMoveEvent 183 116 0 0 0 0\n"
                                             "MouseMoveEvent 185 114 0 0 0 0\n"
                                             "MouseMoveEvent 192 109 0 0 0 0\n"
                                             "MouseMoveEvent 194 107 0 0 0 0\n"
                                             "MouseMoveEvent 203 101 0 0 0 0\n"
                                             "MouseMoveEvent 209 98 0 0 0 0\n"
                                             "MouseMoveEvent 211 96 0 0 0 0\n"
                                             "MouseMoveEvent 216 94 0 0 0 0\n"
                                             "MouseMoveEvent 218 93 0 0 0 0\n"
                                             "MouseMoveEvent 219 92 0 0 0 0\n"
                                             "RightButtonReleaseEvent 219 92 0 0 0 0\n"
                                             "MouseMoveEvent 219 93 0 0 0 0\n"
                                             "MouseMoveEvent 219 93 0 0 0 0\n"
                                             "MouseMoveEvent 219 94 0 0 0 0\n"
                                             "MouseMoveEvent 219 96 0 0 0 0\n"
                                             "MouseMoveEvent 219 98 0 0 0 0\n"
                                             "MouseMoveEvent 219 99 0 0 0 0\n"
                                             "MouseMoveEvent 219 101 0 0 0 0\n"
                                             "MouseMoveEvent 220 103 0 0 0 0\n"
                                             "MouseMoveEvent 220 104 0 0 0 0\n"
                                             "MouseMoveEvent 220 107 0 0 0 0\n"
                                             "MouseMoveEvent 220 109 0 0 0 0\n"
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
