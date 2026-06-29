// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
//
// This example tests the vtkSeedWidget

// First include the required header files for the VTK classes we are using.
#include "vtkActor.h"
#include "vtkCommand.h"
#include "vtkImageActor.h"
#include "vtkImageMapToColors.h"
#include "vtkImageMapper3D.h"
#include "vtkLookupTable.h"
#include "vtkPointHandleRepresentation2D.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty2D.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSeedRepresentation.h"
#include "vtkSeedWidget.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"
#include "vtkTesting.h"
#include "vtkVolume16Reader.h"

#include <iostream>

constexpr char TestSeedWidget2Log[] = "# StreamVersion 1\n"
                                      "EnterEvent 292 47 0 0 0 0 0\n"
                                      "MouseMoveEvent 64 211 0 0 0 0 0\n"
                                      "LeftButtonPressEvent 64 211 0 0 0 0 0\n"
                                      "RenderEvent 64 211 0 0 0 0 0\n"
                                      "LeftButtonReleaseEvent 64 211 0 0 0 0 0\n"
                                      "MouseMoveEvent 66 211 0 0 0 0 0\n"
                                      "RenderEvent 66 211 0 0 0 0 0\n"
                                      "MouseMoveEvent 242 219 0 0 0 0 0\n"
                                      "RenderEvent 242 219 0 0 0 0 0\n"
                                      "LeftButtonPressEvent 242 219 0 0 0 0 0\n"
                                      "RenderEvent 242 219 0 0 0 0 0\n"
                                      "LeftButtonReleaseEvent 242 219 0 0 0 0 0\n"
                                      "MouseMoveEvent 241 220 0 0 0 0 0\n"
                                      "RenderEvent 241 220 0 0 0 0 0\n"
                                      "MouseMoveEvent 217 134 0 0 0 0 0\n"
                                      "RenderEvent 217 134 0 0 0 0 0\n"
                                      "LeftButtonPressEvent 217 134 0 0 0 0 0\n"
                                      "RenderEvent 217 134 0 0 0 0 0\n"
                                      "LeftButtonReleaseEvent 217 134 0 0 0 0 0\n"
                                      "MouseMoveEvent 217 132 0 0 0 0 0\n"
                                      "RenderEvent 217 132 0 0 0 0 0\n"
                                      "MouseMoveEvent 192 82 0 0 0 0 0\n"
                                      "RenderEvent 192 82 0 0 0 0 0\n"
                                      "LeftButtonPressEvent 192 82 0 0 0 0 0\n"
                                      "RenderEvent 192 82 0 0 0 0 0\n"
                                      "LeftButtonReleaseEvent 192 82 0 0 0 0 0\n"
                                      "MouseMoveEvent 191 82 0 0 0 0 0\n"
                                      "RenderEvent 191 82 0 0 0 0 0\n"
                                      "MouseMoveEvent 92 103 0 0 0 0 0\n"
                                      "RenderEvent 92 103 0 0 0 0 0\n"
                                      "LeftButtonPressEvent 92 103 0 0 0 0 0\n"
                                      "RenderEvent 92 103 0 0 0 0 0\n"
                                      "LeftButtonReleaseEvent 92 103 0 0 0 0 0\n"
                                      "MouseMoveEvent 92 105 0 0 0 0 0\n"
                                      "RenderEvent 92 105 0 0 0 0 0\n"
                                      "MouseMoveEvent 92 107 0 0 0 0 0\n"
                                      "RenderEvent 66 210 0 0 0 0 0\n"
                                      "MouseMoveEvent 66 210 0 0 0 0 0\n"
                                      "RenderEvent 66 210 0 0 0 0 0\n"
                                      "LeftButtonPressEvent 66 210 0 0 0 0 0\n"
                                      "RenderEvent 66 210 0 0 0 0 0\n"
                                      "MouseMoveEvent 66 208 0 0 0 0 0\n"
                                      "RenderEvent 92 156 0 0 0 0 0\n"
                                      "MouseMoveEvent 92 156 0 0 0 0 0\n"
                                      "RenderEvent 92 156 0 0 0 0 0\n"
                                      "LeftButtonReleaseEvent 92 156 0 0 0 0 0\n"
                                      "RenderEvent 92 156 0 0 0 0 0\n"
                                      "MouseMoveEvent 94 158 0 0 0 0 0\n"
                                      "RenderEvent 240 221 0 0 0 0 0\n"
                                      "MouseMoveEvent 240 221 0 0 0 0 0\n"
                                      "RenderEvent 240 221 0 0 0 0 0\n"
                                      "LeftButtonPressEvent 240 221 0 0 0 0 0\n"
                                      "RenderEvent 240 221 0 0 0 0 0\n"
                                      "MouseMoveEvent 238 221 0 0 0 0 0\n"
                                      "RenderEvent 60 216 0 0 0 0 0\n"
                                      "MouseMoveEvent 60 216 0 0 0 0 0\n"
                                      "RenderEvent 60 216 0 0 0 0 0\n"
                                      "LeftButtonReleaseEvent 60 216 0 0 0 0 0\n"
                                      "RenderEvent 60 216 0 0 0 0 0\n"
                                      "MouseMoveEvent 62 213 0 0 0 0 0\n"
                                      "RenderEvent 188 81 0 0 0 0 0\n"
                                      "MouseMoveEvent 188 82 0 0 0 0 0\n"
                                      "RenderEvent 188 82 0 0 0 0 0\n"
                                      "LeftButtonPressEvent 188 82 0 0 0 0 0\n"
                                      "RenderEvent 188 82 0 0 0 0 0\n"
                                      "MouseMoveEvent 188 83 0 0 0 0 0\n"
                                      "RenderEvent 164 196 0 0 0 0 0\n"
                                      "MouseMoveEvent 164 197 0 0 0 0 0\n"
                                      "RenderEvent 164 197 0 0 0 0 0\n"
                                      "LeftButtonReleaseEvent 164 197 0 0 0 0 0\n"
                                      "RenderEvent 164 197 0 0 0 0 0\n"
                                      "MouseMoveEvent 164 195 0 0 0 0 0\n"
                                      "RenderEvent 91 108 0 0 0 0 0\n"
                                      "MouseMoveEvent 92 107 0 0 0 0 0\n"
                                      "RenderEvent 92 107 0 0 0 0 0\n"
                                      "LeftButtonPressEvent 92 107 0 0 0 0 0\n"
                                      "RenderEvent 92 107 0 0 0 0 0\n"
                                      "MouseMoveEvent 93 106 0 0 0 0 0\n"
                                      "RenderEvent 188 87 0 0 0 0 0\n"
                                      "MouseMoveEvent 189 87 0 0 0 0 0\n"
                                      "RenderEvent 189 87 0 0 0 0 0\n"
                                      "LeftButtonReleaseEvent 189 87 0 0 0 0 0\n"
                                      "RenderEvent 189 87 0 0 0 0 0\n"
                                      "MouseMoveEvent 190 86 0 0 0 0 0\n"
                                      "RenderEvent 288 85 0 0 0 0 0\n"
                                      "MouseMoveEvent 288 85 0 0 0 0 0\n"
                                      "RenderEvent 288 85 0 0 0 0 0\n"
                                      "LeaveEvent 300 80 0 0 0 0 0\n"
                                      "ExitEvent 300 80 0 0 0 0 0\n";

// This callback is responsible for setting the seed label.
class vtkSeedCallback2 : public vtkCommand
{
public:
  static vtkSeedCallback2* New() { return new vtkSeedCallback2; }
  void Execute(vtkObject*, unsigned long eid, void*) override
  {
    if (eid == vtkCommand::CursorChangedEvent)
    {
      std::cout << "cursor changed\n";
    }
    else
    {
      std::cout << "point placed\n";
    }
  }
};

// The actual test function
int TestSeedWidget2(int argc, char* argv[])
{
  // Create the RenderWindow, Renderer and both Actors
  //
  vtkSmartPointer<vtkRenderer> ren1 = vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkRenderWindow> renWin = vtkSmartPointer<vtkRenderWindow>::New();
  renWin->AddRenderer(ren1);

  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  // Create a test pipeline
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/headsq/quarter");

  // Start by creating a black/white lookup table.
  vtkSmartPointer<vtkLookupTable> bwLut = vtkSmartPointer<vtkLookupTable>::New();
  bwLut->SetTableRange(0, 2000);
  bwLut->SetSaturationRange(0, 0);
  bwLut->SetHueRange(0, 0);
  bwLut->SetValueRange(0, 1);
  bwLut->Build(); // effective built
  vtkSmartPointer<vtkVolume16Reader> v16 = vtkSmartPointer<vtkVolume16Reader>::New();
  v16->SetDataDimensions(64, 64);
  v16->SetDataByteOrderToLittleEndian();
  v16->SetFilePrefix(fname);
  v16->SetImageRange(1, 93);
  v16->SetDataSpacing(3.2, 3.2, 1.5);
  delete[] fname;
  vtkSmartPointer<vtkImageMapToColors> sagittalColors = vtkSmartPointer<vtkImageMapToColors>::New();
  sagittalColors->SetInputConnection(v16->GetOutputPort());
  sagittalColors->SetLookupTable(bwLut);
  vtkSmartPointer<vtkImageActor> sagittal = vtkSmartPointer<vtkImageActor>::New();
  sagittal->GetMapper()->SetInputConnection(sagittalColors->GetOutputPort());
  sagittal->SetDisplayExtent(32, 32, 0, 63, 0, 92);
  sagittal->RotateY(90);
  sagittal->RotateX(90);

  // Create the widget and its representation
  vtkSmartPointer<vtkPointHandleRepresentation2D> handle =
    vtkSmartPointer<vtkPointHandleRepresentation2D>::New();
  handle->GetProperty()->SetColor(1, 0, 0);
  vtkSmartPointer<vtkSeedRepresentation> rep = vtkSmartPointer<vtkSeedRepresentation>::New();
  rep->SetHandleRepresentation(handle);

  vtkSmartPointer<vtkSeedWidget> widget = vtkSmartPointer<vtkSeedWidget>::New();
  widget->SetInteractor(iren);
  widget->SetRepresentation(rep);

  vtkSmartPointer<vtkSeedCallback2> mcbk = vtkSmartPointer<vtkSeedCallback2>::New();
  widget->AddObserver(vtkCommand::PlacePointEvent, mcbk);
  widget->AddObserver(vtkCommand::CursorChangedEvent, mcbk);

  // Add the actors to the renderer, set the background and size
  //
  ren1->AddActor(sagittal);
  ren1->ResetCamera();
  ren1->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(300, 300);

  // render the image
  //
  iren->Initialize();
  renWin->Render();
  widget->On();

  return vtkTesting::InteractorEventLoop(argc, argv, iren, TestSeedWidget2Log);
}
