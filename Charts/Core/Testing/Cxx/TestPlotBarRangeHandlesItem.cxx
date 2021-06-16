#include <vtkAxis.h>
#include <vtkChartXY.h>
#include <vtkContextInteractorStyle.h>
#include <vtkContextScene.h>
#include <vtkIntArray.h>
#include <vtkInteractorEventRecorder.h>
#include <vtkNew.h>
#include <vtkPlotBar.h>
#include <vtkPlotBarRangeHandlesItem.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkTable.h>

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

int TestPlotBarRangeHandlesItem(int, char*[])
{
  vtkNew<vtkTable> table;

  vtkNew<vtkIntArray> arrMonth;
  arrMonth->SetName("Months");
  arrMonth->SetNumberOfComponents(1);
  arrMonth->SetNumberOfTuples(12);
  for (int i = 0; i < arrMonth->GetNumberOfTuples(); ++i)
  {
    arrMonth->SetValue(i, i);
  }
  table->AddColumn(arrMonth);

  constexpr int books[12] = { 5675, 5902, 6388, 5990, 5575, 7393, 9878, 8082, 6417, 5946, 5526,
    5166 };
  vtkNew<vtkIntArray> arrBooks;
  arrBooks->SetName("Books");
  arrBooks->SetNumberOfComponents(1);
  arrBooks->SetNumberOfTuples(12);
  for (int i = 0; i < arrBooks->GetNumberOfTuples(); ++i)
  {
    arrBooks->SetValue(i, books[i]);
  }
  table->AddColumn(arrBooks);

  // Setup scene/interactor
  vtkNew<vtkChartXY> chart;
  chart->GetAxis(vtkAxis::BOTTOM)->SetRange(-5, 15);
  chart->GetAxis(vtkAxis::LEFT)->SetRange(-5, 15);

  vtkNew<vtkContextScene> scene;
  scene->AddItem(chart);

  vtkNew<vtkContextInteractorStyle> interactorStyle;
  interactorStyle->SetScene(scene);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetInteractorStyle(interactorStyle);

  vtkNew<vtkInteractorEventRecorder> recorder;
  recorder->SetInteractor(iren);
  recorder->ReadFromInputStringOn();

  // Add bar plot and handles
  vtkPlotBar* barPlot = vtkPlotBar::SafeDownCast(chart->AddPlot(vtkChart::BAR));
  barPlot->SetInputData(table, "Months", "Books");
  chart->SetBarWidthFraction(1.0);

  vtkNew<vtkPlotBarRangeHandlesItem> rangeItem;
  rangeItem->SetPlotBar(barPlot);
  rangeItem->SetExtent(0, 12, 0, 1);

  chart->AddPlot(rangeItem);
  rangeItem->ComputeHandlesDrawRange();
  chart->RaisePlot(rangeItem);
  chart->Update(); // Force vtkChartXY to compute bars width.

  vtkNew<vtkRangeHandlesCallBack> cbk;
  rangeItem->AddObserver(vtkCommand::StartInteractionEvent, cbk);
  rangeItem->AddObserver(vtkCommand::InteractionEvent, cbk);
  rangeItem->AddObserver(vtkCommand::EndInteractionEvent, cbk);

  //
  // Check initialization
  //
  double range[2];
  rangeItem->GetHandlesRange(range);

  if (range[0] != 0 || range[1] != 12)
  {
    std::cerr << "Initialization: Unexpected range for vertical handle: [" << range[0] << ", "
              << range[1] << "]. Expecting: [0, 12]." << std::endl;
    return EXIT_FAILURE;
  }

  //
  // Moving left handle
  //
  const char leftEvents[] = "# StreamVersion 1\n"
                            "LeftButtonPressEvent 0 10 0 0 0 0 0\n"
                            "MouseMoveEvent 3 10 0 0 0 0 0\n"
                            "LeftButtonReleaseEvent 3 10 0 0 0 0 0\n";
  recorder->SetInputString(leftEvents);
  recorder->Play();

  if (cbk->EventSpy[vtkCommand::StartInteractionEvent] != 1 ||
    cbk->EventSpy[vtkCommand::InteractionEvent] != 1 ||
    cbk->EventSpy[vtkCommand::EndInteractionEvent] != 1)
  {
    std::cerr << "Move left handle: Wrong number of fired events : "
              << cbk->EventSpy[vtkCommand::StartInteractionEvent] << " "
              << cbk->EventSpy[vtkCommand::InteractionEvent] << " "
              << cbk->EventSpy[vtkCommand::EndInteractionEvent] << std::endl;
    return EXIT_FAILURE;
  }

  rangeItem->ComputeHandlesDrawRange();
  rangeItem->GetHandlesRange(range);

  // Expecting 2.5 = 3.0 - barWidth.
  if (fabs(range[0] - 2.5) > 1e-3 || range[1] != 12)
  {
    std::cerr << "Unexpected range for vertical handle: [" << range[0] << ", " << range[1]
              << "]. Expecting: [2.5, 12]." << std::endl;
    return EXIT_FAILURE;
  }

  //
  // Moving right handle
  //
  cbk->EventSpy.clear();
  const char rightEvents[] = "# StreamVersion 1\n"
                             "LeftButtonPressEvent 12 10 0 0 0 0 0\n"
                             "MouseMoveEvent 10 10 0 0 0 0 0\n"
                             "LeftButtonReleaseEvent 10 10 0 0 0 0 0\n";
  recorder->SetInputString(rightEvents);
  recorder->Play();

  if (cbk->EventSpy[vtkCommand::StartInteractionEvent] != 1 ||
    cbk->EventSpy[vtkCommand::InteractionEvent] != 1 ||
    cbk->EventSpy[vtkCommand::EndInteractionEvent] != 1)
  {
    std::cerr << "Move right handle: Wrong number of fired events : "
              << cbk->EventSpy[vtkCommand::StartInteractionEvent] << " "
              << cbk->EventSpy[vtkCommand::InteractionEvent] << " "
              << cbk->EventSpy[vtkCommand::EndInteractionEvent] << std::endl;
    return EXIT_FAILURE;
  }

  rangeItem->ComputeHandlesDrawRange();
  rangeItem->GetHandlesRange(range);

  // Expecting 10.5 = 10.0 + barWidth.
  if (fabs(range[0] - 2.5) > 1e-3 || fabs(range[1] - 10.5) > 1e-3)
  {
    std::cerr << "Unexpected range for vertical handle: [" << range[0] << ", " << range[1]
              << "]. Expecting: [2.5, 10.5]." << std::endl;
    return EXIT_FAILURE;
  }

  //
  // Moving horizontal right handle
  //
  barPlot->SetOrientation(vtkPlotBar::HORIZONTAL);
  rangeItem->SetHandleOrientationToHorizontal();
  rangeItem->SetExtent(0, 12, 0, 1);

  rangeItem->ComputeHandlesDrawRange();
  rangeItem->GetHandlesRange(range);

  // Initialization
  if (range[0] != 0 || range[1] != 12)
  {
    std::cerr << "Unexpected range in horizontal range handle: [" << range[0] << ", " << range[1]
              << "]. Expecting: [0, 12]." << std::endl;
    return EXIT_FAILURE;
  }

  cbk->EventSpy.clear();
  const char hRightEvents[] = "# StreamVersion 1\n"
                              "LeftButtonPressEvent 1 12 0 0 0 0 0\n"
                              "MouseMoveEvent 1 5 0 0 0 0 0\n"
                              "LeftButtonReleaseEvent 1 5 0 0 0 0 0\n";
  recorder->SetInputString(hRightEvents);
  recorder->Play();

  if (cbk->EventSpy[vtkCommand::StartInteractionEvent] != 1 ||
    cbk->EventSpy[vtkCommand::InteractionEvent] != 1 ||
    cbk->EventSpy[vtkCommand::EndInteractionEvent] != 1)
  {
    std::cerr << "Move horizontal handle: Wrong number of fired events : "
              << cbk->EventSpy[vtkCommand::StartInteractionEvent] << " "
              << cbk->EventSpy[vtkCommand::InteractionEvent] << " "
              << cbk->EventSpy[vtkCommand::EndInteractionEvent] << std::endl;
    return EXIT_FAILURE;
  }

  rangeItem->ComputeHandlesDrawRange();
  rangeItem->GetHandlesRange(range);

  // Expecting 5.5 = 5.0 + barWidth.
  if (range[0] != 0 || fabs(range[1] - 5.5) > 1e-3)
  {
    std::cerr << "Unexpected range for horizontal handle : [" << range[0] << ", " << range[1]
              << "]. Expecting : [0, 5.5]." << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
