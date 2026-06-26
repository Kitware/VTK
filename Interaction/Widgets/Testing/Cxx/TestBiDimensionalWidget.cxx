// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
//
// This example tests the vtkBiDimensionalWidget.

// First include the required header files for the VTK classes we are using.
#include "vtkBiDimensionalRepresentation2D.h"
#include "vtkBiDimensionalWidget.h"
#include "vtkCommand.h"
#include "vtkImageActor.h"
#include "vtkImageData.h"
#include "vtkImageMapper3D.h"
#include "vtkImageShiftScale.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkInteractorStyleImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"
#include "vtkVolume16Reader.h"

#include <iostream>

constexpr char BiDimensionalWidgetEventLog[] = "# StreamVersion 1\n"
                                               "RenderEvent 0 0 0 0 0 0 0\n"
                                               "EnterEvent 287 155 0 0 0 0 0\n"
                                               "MouseMoveEvent 92 96 0 0 0 0 0\n"
                                               "LeftButtonPressEvent 92 96 0 0 0 0 0\n"
                                               "RenderEvent 92 96 0 0 0 0 0\n"
                                               "LeftButtonReleaseEvent 92 96 0 0 0 0 0\n"
                                               "MouseMoveEvent 92 96 0 0 0 0 0\n"
                                               "RenderEvent 92 96 0 0 0 0 0\n"
                                               "MouseMoveEvent 200 201 0 0 0 0 0\n"
                                               "RenderEvent 200 201 0 0 0 0 0\n"
                                               "LeftButtonPressEvent 200 201 0 0 0 0 0\n"
                                               "RenderEvent 200 201 0 0 0 0 0\n"
                                               "LeftButtonReleaseEvent 200 201 0 0 0 0 0\n"
                                               "MouseMoveEvent 200 201 0 0 0 0 0\n"
                                               "RenderEvent 92 204 0 0 0 0 0\n"
                                               "MouseMoveEvent 92 205 0 0 0 0 0\n"
                                               "RenderEvent 92 205 0 0 0 0 0\n"
                                               "LeftButtonPressEvent 92 205 0 0 0 0 0\n"
                                               "RenderEvent 92 205 0 0 0 0 0\n"
                                               "LeftButtonReleaseEvent 92 205 0 0 0 0 0\n"
                                               "MouseMoveEvent 92 205 0 0 0 0 0\n"
                                               "RenderEvent 92 205 0 0 0 0 0\n"
                                               "MouseMoveEvent 133 160 0 0 0 0 0\n"
                                               "RenderEvent 133 160 0 0 0 0 0\n"
                                               "LeftButtonPressEvent 133 160 0 0 0 0 0\n"
                                               "RenderEvent 133 160 0 0 0 0 0\n"
                                               "MouseMoveEvent 133 161 0 0 0 0 0\n"
                                               "RenderEvent 133 160 0 0 0 0 0\n"
                                               "MouseMoveEvent 118 159 0 0 0 0 0\n"
                                               "RenderEvent 118 159 0 0 0 0 0\n"
                                               "LeftButtonReleaseEvent 118 159 0 0 0 0 0\n"
                                               "RenderEvent 118 159 0 0 0 0 0\n"
                                               "MouseMoveEvent 118 159 0 0 0 0 0\n"
                                               "RenderEvent 161 160 0 0 0 0 0\n"
                                               "MouseMoveEvent 161 161 0 0 0 0 0\n"
                                               "RenderEvent 161 161 0 0 0 0 0\n"
                                               "LeftButtonPressEvent 161 161 0 0 0 0 0\n"
                                               "RenderEvent 161 161 0 0 0 0 0\n"
                                               "MouseMoveEvent 161 162 0 0 0 0 0\n"
                                               "RenderEvent 164 171 0 0 0 0 0\n"
                                               "MouseMoveEvent 165 169 0 0 0 0 0\n"
                                               "RenderEvent 165 169 0 0 0 0 0\n"
                                               "LeftButtonReleaseEvent 165 169 0 0 0 0 0\n"
                                               "RenderEvent 165 169 0 0 0 0 0\n"
                                               "MouseMoveEvent 165 169 0 0 0 0 0\n"
                                               "RenderEvent 199 200 0 0 0 0 0\n"
                                               "MouseMoveEvent 199 201 0 0 0 0 0\n"
                                               "RenderEvent 199 201 0 0 0 0 0\n"
                                               "LeftButtonPressEvent 199 201 0 0 0 0 0\n"
                                               "RenderEvent 199 201 0 0 0 0 0\n"
                                               "MouseMoveEvent 198 201 0 0 0 0 0\n"
                                               "RenderEvent 160 175 0 0 0 0 0\n"
                                               "MouseMoveEvent 160 174 0 0 0 0 0\n"
                                               "RenderEvent 160 174 0 0 0 0 0\n"
                                               "LeftButtonReleaseEvent 160 174 0 0 0 0 0\n"
                                               "RenderEvent 160 174 0 0 0 0 0\n"
                                               "MouseMoveEvent 160 174 0 0 0 0 0\n"
                                               "RenderEvent 87 96 0 0 0 0 0\n"
                                               "MouseMoveEvent 87 95 0 0 0 0 0\n"
                                               "RenderEvent 87 95 0 0 0 0 0\n"
                                               "LeftButtonPressEvent 87 95 0 0 0 0 0\n"
                                               "RenderEvent 87 95 0 0 0 0 0\n"
                                               "MouseMoveEvent 88 95 0 0 0 0 0\n"
                                               "RenderEvent 105 111 0 0 0 0 0\n"
                                               "MouseMoveEvent 106 111 0 0 0 0 0\n"
                                               "RenderEvent 106 111 0 0 0 0 0\n"
                                               "LeftButtonReleaseEvent 106 111 0 0 0 0 0\n"
                                               "RenderEvent 106 111 0 0 0 0 0\n"
                                               "MouseMoveEvent 106 111 0 0 0 0 0\n"
                                               "RenderEvent 130 140 0 0 0 0 0\n"
                                               "MouseMoveEvent 131 140 0 0 0 0 0\n"
                                               "RenderEvent 131 140 0 0 0 0 0\n"
                                               "LeftButtonPressEvent 131 140 0 0 0 0 0\n"
                                               "RenderEvent 131 140 0 0 0 0 0\n"
                                               "MouseMoveEvent 132 140 0 0 0 0 0\n"
                                               "RenderEvent 141 150 0 0 0 0 0\n"
                                               "MouseMoveEvent 141 151 0 0 0 0 0\n"
                                               "RenderEvent 141 151 0 0 0 0 0\n"
                                               "LeftButtonReleaseEvent 141 151 0 0 0 0 0\n"
                                               "RenderEvent 141 151 0 0 0 0 0\n"
                                               "MouseMoveEvent 141 151 0 0 0 0 0\n"
                                               "MouseMoveEvent 62 122 0 0 0 0 0\n"
                                               "RenderEvent 62 122 0 0 0 0 0\n";

// This does the actual work: updates the probe.
// Callback for the interaction
class vtkBiDimensionalCallback : public vtkCommand
{
public:
  static vtkBiDimensionalCallback* New() { return new vtkBiDimensionalCallback; }
  void Execute(vtkObject*, unsigned long, void*) override
  {
    std::cout << "End interaction event\n" << std::flush;
  }
  vtkBiDimensionalCallback() = default;
};

int TestBiDimensionalWidget(int argc, char* argv[])
{
  // Create the pipeline
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/headsq/quarter");

  vtkSmartPointer<vtkVolume16Reader> v16 = vtkSmartPointer<vtkVolume16Reader>::New();
  v16->SetDataDimensions(64, 64);
  v16->SetDataByteOrderToLittleEndian();
  v16->SetImageRange(1, 93);
  v16->SetDataSpacing(3.2, 3.2, 1.5);
  v16->SetFilePrefix(fname);
  v16->ReleaseDataFlagOn();
  v16->SetDataMask(0x7fff);
  v16->Update();
  delete[] fname;

  double range[2];
  v16->GetOutput()->GetScalarRange(range);

  vtkSmartPointer<vtkImageShiftScale> shifter = vtkSmartPointer<vtkImageShiftScale>::New();
  shifter->SetShift(-1.0 * range[0]);
  shifter->SetScale(255.0 / (range[1] - range[0]));
  shifter->SetOutputScalarTypeToUnsignedChar();
  shifter->SetInputConnection(v16->GetOutputPort());
  shifter->ReleaseDataFlagOff();
  shifter->Update();

  vtkSmartPointer<vtkImageActor> imageActor = vtkSmartPointer<vtkImageActor>::New();
  imageActor->GetMapper()->SetInputConnection(shifter->GetOutputPort());
  imageActor->VisibilityOn();
  imageActor->SetDisplayExtent(0, 63, 0, 63, 46, 46);
  imageActor->InterpolateOn();

  double bounds[6];
  imageActor->GetBounds(bounds);

  // Create the RenderWindow, Renderer and both Actors
  //
  vtkSmartPointer<vtkRenderer> ren1 = vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkRenderWindow> renWin = vtkSmartPointer<vtkRenderWindow>::New();
  renWin->AddRenderer(ren1);

  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  vtkSmartPointer<vtkInteractorStyleImage> style = vtkSmartPointer<vtkInteractorStyleImage>::New();
  iren->SetInteractorStyle(style);

  // VTK widgets consist of two parts: the widget part that handles event processing;
  // and the widget representation that defines how the widget appears in the scene
  // (i.e., matters pertaining to geometry).
  vtkSmartPointer<vtkBiDimensionalRepresentation2D> rep =
    vtkSmartPointer<vtkBiDimensionalRepresentation2D>::New();

  vtkSmartPointer<vtkBiDimensionalWidget> widget = vtkSmartPointer<vtkBiDimensionalWidget>::New();
  widget->SetInteractor(iren);
  widget->SetRepresentation(rep);

  vtkSmartPointer<vtkBiDimensionalCallback> callback =
    vtkSmartPointer<vtkBiDimensionalCallback>::New();
  widget->AddObserver(vtkCommand::EndInteractionEvent, callback);

  // Add the actors to the renderer, set the background and size
  ren1->AddActor(imageActor);
  ren1->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(300, 300);

  // record events
  vtkSmartPointer<vtkInteractorEventRecorder> recorder =
    vtkSmartPointer<vtkInteractorEventRecorder>::New();
  recorder->SetInteractor(iren);

#ifdef RECORD
  recorder->SetFileName("record.log");
  recorder->On();
  recorder->Record();
#else
  recorder->ReadFromInputStringOn();
  recorder->SetInputString(BiDimensionalWidgetEventLog);
#endif

  // render the image
  //
  iren->Initialize();
  renWin->Render();
  widget->On();

#ifndef RECORD
  recorder->Play();
  recorder->Off();
#endif

  iren->Start();

  return EXIT_SUCCESS;
}
