/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPlotRangeHandlesItem.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkAxis.h"
#include "vtkChartXY.h"
#include "vtkContextInteractorStyle.h"
#include "vtkContextScene.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkNew.h"
#include "vtkPlotRangeHandlesItem.h"
#include "vtkRenderWindowInteractor.h"

#include <iostream>
#include <map>

//------------------------------------------------------------------------------
class vtkRangeHandlesCallBack : public vtkCommand
{
public:
  static vtkRangeHandlesCallBack* New() { return new vtkRangeHandlesCallBack; }

  void Execute(vtkObject* caller, unsigned long event, void* vtkNotUsed(callData)) override
  {
    vtkPlotRangeHandlesItem* self = vtkPlotRangeHandlesItem::SafeDownCast(caller);
    if (!self)
    {
      return;
    }
    if (event == vtkCommand::EndInteractionEvent)
    {
      self->GetHandlesRange(this->Range);
    }
    if (this->EventSpy.count(event) == 0)
    {
      this->EventSpy[event] = 0;
    }
    ++this->EventSpy[event];
    std::cout << "InvokedEvent: " << event << this->EventSpy[event] << std::endl;
  }
  std::map<unsigned long, int> EventSpy;
  double Range[2];
};

int TestPlotRangeHandlesItem(int, char*[])
{
  vtkNew<vtkChartXY> chart;
  chart->GetAxis(vtkAxis::BOTTOM)->SetRange(0, 50);
  chart->GetAxis(vtkAxis::LEFT)->SetRange(0, 50);

  // Vertical handles
  vtkNew<vtkPlotRangeHandlesItem> VRangeItem;
  VRangeItem->SetExtent(0, 10, 0, 30);
  VRangeItem->SynchronizeRangeHandlesOn();
  chart->AddPlot(VRangeItem);
  VRangeItem->ComputeHandlesDrawRange();

  vtkNew<vtkRangeHandlesCallBack> Vcbk;
  VRangeItem->AddObserver(vtkCommand::StartInteractionEvent, Vcbk);
  VRangeItem->AddObserver(vtkCommand::InteractionEvent, Vcbk);
  VRangeItem->AddObserver(vtkCommand::EndInteractionEvent, Vcbk);

  // Horizontal handles
  vtkNew<vtkPlotRangeHandlesItem> HRangeItem;
  HRangeItem->SetHandleOrientationToHorizontal();
  HRangeItem->SynchronizeRangeHandlesOn();
  HRangeItem->SetExtent(0, 20, 0, 10);
  chart->AddPlot(HRangeItem);
  HRangeItem->ComputeHandlesDrawRange();

  vtkNew<vtkRangeHandlesCallBack> Hcbk;
  HRangeItem->AddObserver(vtkCommand::StartInteractionEvent, Hcbk);
  HRangeItem->AddObserver(vtkCommand::InteractionEvent, Hcbk);
  HRangeItem->AddObserver(vtkCommand::EndInteractionEvent, Hcbk);

  vtkNew<vtkContextScene> scene;
  scene->AddItem(VRangeItem);
  scene->AddItem(HRangeItem);

  vtkNew<vtkContextInteractorStyle> interactorStyle;
  interactorStyle->SetScene(scene);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetInteractorStyle(interactorStyle);

  vtkNew<vtkInteractorEventRecorder> recorder;
  recorder->SetInteractor(iren);
  recorder->ReadFromInputStringOn();

  //
  // Initialization
  //
  double rangeV[2];
  VRangeItem->GetHandlesRange(rangeV);

  if (rangeV[0] != 0 || rangeV[1] != 10)
  {
    std::cerr << "Initialization: Unexepected range in vertical range handle : [" << rangeV[0]
              << ", " << rangeV[1] << "]. Expecting : [0, 10]." << std::endl;
    return EXIT_FAILURE;
  }

  double rangeH[2];
  HRangeItem->GetHandlesRange(rangeH);

  if (rangeH[0] != 0 || rangeH[1] != 20)
  {
    std::cerr << "Initialization: Unexepected range in horizontal range handle : [" << rangeH[0]
              << ", " << rangeH[1] << "]. Expecting : [0, 20]." << std::endl;
    return EXIT_FAILURE;
  }

  //
  // Moving vertical right handle
  //
  const char rightEvents[] = "# StreamVersion 1\n"
                             "LeftButtonPressEvent 10 2 0 0 0 0 0\n"
                             "MouseMoveEvent 20 2 0 0 0 0 0\n"
                             "LeftButtonReleaseEvent 20 2 0 0 0 0 0\n";
  recorder->SetInputString(rightEvents);
  recorder->Play();

  if (Vcbk->EventSpy[vtkCommand::StartInteractionEvent] != 1 ||
    Vcbk->EventSpy[vtkCommand::InteractionEvent] != 1 ||
    Vcbk->EventSpy[vtkCommand::EndInteractionEvent] != 1)
  {
    std::cerr << "Move right handle: Wrong number of fired events : "
              << Vcbk->EventSpy[vtkCommand::StartInteractionEvent] << " "
              << Vcbk->EventSpy[vtkCommand::InteractionEvent] << " "
              << Vcbk->EventSpy[vtkCommand::EndInteractionEvent] << std::endl;
    return EXIT_FAILURE;
  }

  VRangeItem->ComputeHandlesDrawRange();
  VRangeItem->GetHandlesRange(rangeV);

  if (rangeV[0] != 0 || rangeV[1] != 20.25)
  {
    std::cerr << "1. Unexepected range in vertical range handle : [" << rangeV[0] << ", "
              << rangeV[1] << "]. Expecting : [0, 20.25]." << std::endl;
    return EXIT_FAILURE;
  }

  //
  // Moving vertical left handle
  //
  Vcbk->EventSpy.clear();
  const char leftEvents[] = "# StreamVersion 1\n"
                            "LeftButtonPressEvent 0 2 0 0 0 0 0\n"
                            "MouseMoveEvent 10 2 0 0 0 0 0\n"
                            "LeftButtonReleaseEvent 10 2 0 0 0 0 0\n";
  recorder->SetInputString(leftEvents);
  recorder->Play();

  if (Vcbk->EventSpy[vtkCommand::StartInteractionEvent] != 1 ||
    Vcbk->EventSpy[vtkCommand::InteractionEvent] != 1 ||
    Vcbk->EventSpy[vtkCommand::EndInteractionEvent] != 1)
  {
    std::cerr << "Move left handle: Wrong number of fired events : "
              << Vcbk->EventSpy[vtkCommand::StartInteractionEvent] << " "
              << Vcbk->EventSpy[vtkCommand::InteractionEvent] << " "
              << Vcbk->EventSpy[vtkCommand::EndInteractionEvent] << std::endl;
    return EXIT_FAILURE;
  }

  VRangeItem->ComputeHandlesDrawRange();
  VRangeItem->GetHandlesRange(rangeV);

  if (rangeV[0] != 9.75 || rangeV[1] != 30)
  {
    std::cerr << "2. Unexepected range in vertical range handle : [" << rangeV[0] << ", "
              << rangeV[1] << "]. Expecting : [9.75, 30]." << std::endl;
    return EXIT_FAILURE;
  }

  //
  // Disable synchronization on vertical handles
  //
  VRangeItem->SynchronizeRangeHandlesOff();
  Vcbk->EventSpy.clear();
  const char leftEvents2[] = "# StreamVersion 1\n"
                             "LeftButtonPressEvent 10 2 0 0 0 0 0\n"
                             "MouseMoveEvent 20 2 0 0 0 0 0\n"
                             "LeftButtonReleaseEvent 20 2 0 0 0 0 0\n";
  recorder->SetInputString(leftEvents2);
  recorder->Play();

  if (Vcbk->EventSpy[vtkCommand::StartInteractionEvent] != 1 ||
    Vcbk->EventSpy[vtkCommand::InteractionEvent] != 1 ||
    Vcbk->EventSpy[vtkCommand::EndInteractionEvent] != 1)
  {
    std::cerr << "Move left handle: Wrong number of fired events : "
              << Vcbk->EventSpy[vtkCommand::StartInteractionEvent] << " "
              << Vcbk->EventSpy[vtkCommand::InteractionEvent] << " "
              << Vcbk->EventSpy[vtkCommand::EndInteractionEvent] << std::endl;
    return EXIT_FAILURE;
  }

  VRangeItem->ComputeHandlesDrawRange();
  VRangeItem->GetHandlesRange(rangeV);

  if (rangeV[0] != 19.75 || rangeV[1] != 30)
  {
    std::cerr << "3. Unexepected range in vertical range handle : [" << rangeV[0] << ", "
              << rangeV[1] << "]. Expecting : [19.75, 30]." << std::endl;
    return EXIT_FAILURE;
  }

  //
  // Move horizontal right handle (top handle)
  //
  const char topEvents[] = "# StreamVersion 1\n"
                           "LeftButtonPressEvent 2 20 0 0 0 0 0\n"
                           "MouseMoveEvent 2 30 0 0 0 0 0\n"
                           "LeftButtonReleaseEvent 2 30 0 0 0 0 0\n";
  recorder->SetInputString(topEvents);
  recorder->Play();

  if (Hcbk->EventSpy[vtkCommand::StartInteractionEvent] != 1 ||
    Hcbk->EventSpy[vtkCommand::InteractionEvent] != 1 ||
    Hcbk->EventSpy[vtkCommand::EndInteractionEvent] != 1)
  {
    std::cerr << "Move top handle: Wrong number of fired events : "
              << Hcbk->EventSpy[vtkCommand::StartInteractionEvent] << " "
              << Hcbk->EventSpy[vtkCommand::InteractionEvent] << " "
              << Hcbk->EventSpy[vtkCommand::EndInteractionEvent] << std::endl;
    return EXIT_FAILURE;
  }

  HRangeItem->ComputeHandlesDrawRange();
  HRangeItem->GetHandlesRange(rangeH);

  if (rangeH[0] != 0 || rangeH[1] != 30.25)
  {
    std::cerr << "4. Unexepected range in top range handle : [" << rangeH[0] << ", " << rangeH[1]
              << "]. Expecting : [0, 30.25]." << std::endl;
    return EXIT_FAILURE;
  }

  //
  // Move horizontal left handle (bottom handle)
  //
  Hcbk->EventSpy.clear();
  const char bottomEvents[] = "# StreamVersion 1\n"
                              "LeftButtonPressEvent 2 0 0 0 0 0 0\n"
                              "MouseMoveEvent 2 30 0 0 0 0 0\n"
                              "LeftButtonReleaseEvent 2 30 0 0 0 0 0\n";
  recorder->SetInputString(bottomEvents);
  recorder->Play();

  if (Hcbk->EventSpy[vtkCommand::StartInteractionEvent] != 1 ||
    Hcbk->EventSpy[vtkCommand::InteractionEvent] != 1 ||
    Hcbk->EventSpy[vtkCommand::EndInteractionEvent] != 1)
  {
    std::cerr << "Move bottom handle: Wrong number of fired events : "
              << Hcbk->EventSpy[vtkCommand::StartInteractionEvent] << " "
              << Hcbk->EventSpy[vtkCommand::InteractionEvent] << " "
              << Hcbk->EventSpy[vtkCommand::EndInteractionEvent] << std::endl;
    return EXIT_FAILURE;
  }

  HRangeItem->ComputeHandlesDrawRange();
  HRangeItem->GetHandlesRange(rangeH);

  if (rangeH[0] != 29.75 || rangeH[1] != 60)
  {
    std::cerr << "5. Unexepected range in vertical range handle : [" << rangeH[0] << ", "
              << rangeH[1] << "]. Expecting : [29.75, 60]." << std::endl;
    return EXIT_FAILURE;
  }

  //
  // Disable synchronization on horizontal handles
  //
  HRangeItem->SynchronizeRangeHandlesOff();
  Hcbk->EventSpy.clear();
  const char bottomEvents2[] = "# StreamVersion 1\n"
                               "LeftButtonPressEvent 2 30 0 0 0 0 0\n"
                               "MouseMoveEvent 2 20 0 0 0 0 0\n"
                               "LeftButtonReleaseEvent 2 20 0 0 0 0 0\n";
  recorder->SetInputString(bottomEvents2);
  recorder->Play();

  if (Hcbk->EventSpy[vtkCommand::StartInteractionEvent] != 1 ||
    Hcbk->EventSpy[vtkCommand::InteractionEvent] != 1 ||
    Hcbk->EventSpy[vtkCommand::EndInteractionEvent] != 1)
  {
    std::cerr << "Move bottom handle: Wrong number of fired events : "
              << Hcbk->EventSpy[vtkCommand::StartInteractionEvent] << " "
              << Hcbk->EventSpy[vtkCommand::InteractionEvent] << " "
              << Hcbk->EventSpy[vtkCommand::EndInteractionEvent] << std::endl;
    return EXIT_FAILURE;
  }

  HRangeItem->ComputeHandlesDrawRange();
  HRangeItem->GetHandlesRange(rangeH);

  if (rangeH[0] != 19.75 || rangeH[1] != 60)
  {
    std::cerr << "6. Unexepected range in vertical range handle : [" << rangeH[0] << ", "
              << rangeH[1] << "]. Expecting : [19.75, 60]." << std::endl;
    return EXIT_FAILURE;
  }

  //
  // Disable automatic height computation of vertical handles
  //
  VRangeItem->ExtentToAxisRangeOff();
  Vcbk->EventSpy.clear();
  const char leftEvents3[] = "# StreamVersion 1\n"
                             "LeftButtonPressEvent 20 10 0 0 0 0 0\n"
                             "MouseMoveEvent 10 10 0 0 0 0 0\n"
                             "LeftButtonReleaseEvent 10 10 0 0 0 0 0\n";
  recorder->SetInputString(leftEvents3);
  recorder->Play();

  if (Vcbk->EventSpy[vtkCommand::StartInteractionEvent] != 1 ||
    Vcbk->EventSpy[vtkCommand::InteractionEvent] != 1 ||
    Vcbk->EventSpy[vtkCommand::EndInteractionEvent] != 1)
  {
    std::cerr << "Move left handle: Wrong number of fired events : "
              << Vcbk->EventSpy[vtkCommand::StartInteractionEvent] << " "
              << Vcbk->EventSpy[vtkCommand::InteractionEvent] << " "
              << Vcbk->EventSpy[vtkCommand::EndInteractionEvent] << std::endl;
    return EXIT_FAILURE;
  }

  VRangeItem->ComputeHandlesDrawRange();
  VRangeItem->GetHandlesRange(rangeV);

  if (rangeV[0] != 9.75 || rangeV[1] != 30)
  {
    std::cerr << "3. Unexepected range in vertical range handle : [" << rangeV[0] << ", "
              << rangeV[1] << "]. Expecting : [9.75, 30]." << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
