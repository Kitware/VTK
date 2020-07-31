#include <iostream>
#include <map>

#include <vtkChartXY.h>
#include <vtkContextScene.h>
#include <vtkContextInteractorStyle.h>
#include <vtkContextView.h>
#include <vtkFloatArray.h>
#include <vtkInteractorEventRecorder.h>
#include <vtkNew.h>
#include <vtkPlotRangeHandlesItem.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkTable.h>

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
  // Create a table with some points in it
  vtkNew<vtkTable> table;

  vtkNew<vtkFloatArray> arrX;
  arrX->SetName("X Axis");
  table->AddColumn(arrX);

  vtkNew<vtkFloatArray> arrC;
  arrC->SetName("Cosine");
  table->AddColumn(arrC);

  // Fill in the table with some example values
  int numPoints = 69;
  float inc = 10 / (numPoints - 1);
  table->SetNumberOfRows(numPoints);
  for (int i = 1; i <= numPoints; ++i)
  {
    table->SetValue(i - 1, 0, i);
    table->SetValue(i - 1, 1, i);
  }

  vtkNew<vtkChartXY> chart;

  vtkPlot* line = chart->AddPlot(vtkChart::LINE);
  line->SetInputData(table, 0, 1);
  line->SetColor(0, 255, 0, 255);
  line->SetWidth(1.0);

  // Define vertical handles

  vtkNew<vtkPlotRangeHandlesItem> VRangeItem;
  VRangeItem->SetExtent(0, 10, 0, 30);
  VRangeItem->SynchronizeRangeHandlesOn();
  chart->AddPlot(VRangeItem);
  
  vtkNew<vtkRangeHandlesCallBack> Vcbk;
  VRangeItem->AddObserver(vtkCommand::StartInteractionEvent, Vcbk);
  VRangeItem->AddObserver(vtkCommand::InteractionEvent, Vcbk);
  VRangeItem->AddObserver(vtkCommand::EndInteractionEvent, Vcbk);

  // Define horizontal handles

  vtkNew<vtkPlotRangeHandlesItem> HRangeItem;
  HRangeItem->SetHandleOrientationToHorizontal();
  HRangeItem->SynchronizeRangeHandlesOn();
  HRangeItem->SetExtent(0, 20, 0, 10);
  chart->AddPlot(HRangeItem);

  vtkNew<vtkRangeHandlesCallBack> Hcbk;
  HRangeItem->AddObserver(vtkCommand::StartInteractionEvent, Hcbk);
  HRangeItem->AddObserver(vtkCommand::InteractionEvent, Hcbk);
  HRangeItem->AddObserver(vtkCommand::EndInteractionEvent, Hcbk);


  // Setup scene/interactor

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
  // Check initialization
  //

  double rangeV[2];
  VRangeItem->GetHandlesRange(rangeV);

  if (rangeV[0] != 0 || rangeV[1] != 10)
  {
    std::cerr << "Initialization: Unexepected range in vertical range handle : [" << rangeV[0] << ", " << rangeV[1]
              << "]. Expecting : [0, 10]." << std::endl;
    return EXIT_FAILURE;
  }

  double rangeH[2];
  HRangeItem->GetHandlesRange(rangeH);

  if (rangeH[0] != 0 || rangeH[1] != 20)
  {
    std::cerr << "Initialization: Unexepected range in horizontal range handle : [" << rangeH[0] << ", "
              << rangeH[1]
              << "]. Expecting : [0, 20]." << std::endl;
    return EXIT_FAILURE;
  }

  //
  // Check when moving vertical right handle
  //

  const char rightEvents[] = "# StreamVersion 1\n"
                             "LeftButtonPressEvent 10 10 0 0 0 0 0\n"
                            "MouseMoveEvent 20 10 0 0 0 0 0\n"
                            "LeftButtonReleaseEvent 20 10 0 0 0 0 0\n";
  recorder->SetInputString(rightEvents);
  recorder->Play();

  if (Vcbk->EventSpy[vtkCommand::StartInteractionEvent] != 1 ||
    Vcbk->EventSpy[vtkCommand::InteractionEvent] != 1 ||
    Vcbk->EventSpy[vtkCommand::EndInteractionEvent] != 1)
  {
    std::cerr << "Move right handle1: Wrong number of fired events : "
              << Vcbk->EventSpy[vtkCommand::StartInteractionEvent] << " "
              << Vcbk->EventSpy[vtkCommand::InteractionEvent] << " "
              << Vcbk->EventSpy[vtkCommand::EndInteractionEvent] << std::endl;
    return EXIT_FAILURE;
  }

  VRangeItem->GetHandlesRange(rangeV);
  if (rangeV[0] != 0 || rangeV[1] != 20)
  {
    std::cerr << "Unexepected range in vertical range handle : [" << rangeV[0]
              << ", " << rangeV[1] << "]. Expecting : [0, 20]." << std::endl;
    return EXIT_FAILURE;
  }

  //
  // Check when moving vertical left handle
  //

  Vcbk->EventSpy.clear();
  const char leftEvents[] = "# StreamVersion 1\n"
                            "LeftButtonPressEvent 0 10 0 0 0 0 0\n"
                            "MouseMoveEvent 10 10 0 0 0 0 0\n"
                            "LeftButtonReleaseEvent 10 10 0 0 0 0 0\n";
  recorder->SetInputString(leftEvents);
  recorder->Play();

  if (Vcbk->EventSpy[vtkCommand::StartInteractionEvent] != 1 ||
    Vcbk->EventSpy[vtkCommand::InteractionEvent] != 1 ||
    Vcbk->EventSpy[vtkCommand::EndInteractionEvent] != 1)
  {
    std::cerr << "Move left handle2: Wrong number of fired events : "
              << Vcbk->EventSpy[vtkCommand::StartInteractionEvent] << " "
              << Vcbk->EventSpy[vtkCommand::InteractionEvent] << " "
              << Vcbk->EventSpy[vtkCommand::EndInteractionEvent] << std::endl;
    return EXIT_FAILURE;
  }

  VRangeItem->GetHandlesRange(rangeV);
  if (rangeV[0] != 10 || rangeV[1] != 30)
  {
    std::cerr << "Unexepected range in vertical range handle : [" << rangeV[0]
              << ", " << rangeV[1] << "]. Expecting : [10, 30]." << std::endl;
    return EXIT_FAILURE;
  }

  //
  // Check when disable synchronization on vertical handles
  //

  VRangeItem->SynchronizeRangeHandlesOff();
  Vcbk->EventSpy.clear();
  const char leftEvents2[] = "# StreamVersion 1\n"
                            "LeftButtonPressEvent 10 10 0 0 0 0 0\n"
                            "MouseMoveEvent 20 10 0 0 0 0 0\n"
                            "LeftButtonReleaseEvent 20 10 0 0 0 0 0\n";
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

  VRangeItem->GetHandlesRange(rangeV);
  if (rangeV[0] != 20 || rangeV[1] != 30)
  {
    std::cerr << "Unexepected range in vertical range handle : [" << rangeV[0] << ", " << rangeV[1]
              << "]. Expecting : [20, 30]." << std::endl;
    return EXIT_FAILURE;
  }

  //
  // Check when moving horizontal right handle (top handle)
  //

  const char topEvents[] = "# StreamVersion 1\n"
                             "LeftButtonPressEvent 10 20 0 0 0 0 0\n"
                             "MouseMoveEvent 10 30 0 0 0 0 0\n"
                             "LeftButtonReleaseEvent 10 30 0 0 0 0 0\n";
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

  HRangeItem->GetHandlesRange(rangeH);
  if (rangeH[0] != 0 || rangeH[1] != 30)
  {
    std::cerr << "Unexepected range in top range handle : [" << rangeH[0] << ", " << rangeH[1]
              << "]. Expecting : [0, 30]." << std::endl;
    return EXIT_FAILURE;
  }

  //
  // Check when moving horizontal left handle (bottom handle)
  //
  Hcbk->EventSpy.clear();
  const char bottomEvents[] = "# StreamVersion 1\n"
                           "LeftButtonPressEvent 10 0 0 0 0 0 0\n"
                           "MouseMoveEvent 10 30 0 0 0 0 0\n"
                           "LeftButtonReleaseEvent 10 30 0 0 0 0 0\n";
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

  HRangeItem->GetHandlesRange(rangeH);
  if (rangeH[0] != 30 || rangeH[1] != 60)
  {
    std::cerr << "Unexepected range in vertical range handle : [" << rangeH[0] << ", " << rangeH[1]
              << "]. Expecting : [30, 60]." << std::endl;
    return EXIT_FAILURE;
  }

  //
  // Check when disable synchronization on horizontal handles
  //
  HRangeItem->SynchronizeRangeHandlesOff();
  Hcbk->EventSpy.clear();
  const char bottomEvents2[] = "# StreamVersion 1\n"
                              "LeftButtonPressEvent 10 30 0 0 0 0 0\n"
                              "MouseMoveEvent 10 20 0 0 0 0 0\n"
                              "LeftButtonReleaseEvent 10 20 0 0 0 0 0\n";
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

  HRangeItem->GetHandlesRange(rangeH);
  if (rangeH[0] != 20 || rangeH[1] != 60)
  {
    std::cerr << "Unexepected range in vertical range handle : [" << rangeH[0] << ", " << rangeH[1]
              << "]. Expecting : [20, 60]." << std::endl;
    return EXIT_FAILURE;
  }

  // HRangeItem->GetYAxis()->SetLogScale(true);
  // line->GetYAxis()->SetLogScale(true);

  return EXIT_SUCCESS;
}