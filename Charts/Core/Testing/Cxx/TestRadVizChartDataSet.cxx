// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "TestParallelCoordinatesUtilities.h"
#include "vtkChartRadViz.h"
#include "vtkContextScene.h"
#include "vtkContextView.h"
#include "vtkDataSet.h"
#include "vtkNew.h"
#include "vtkPen.h"
#include "vtkPlotRadViz.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkStringScanner.h"
#include "vtkTesting.h"
#include "vtkXMLImageDataReader.h"

// Define your custom Command class
class ResetLens : public vtkCommand
{
public:
  static ResetLens* New() { return new ResetLens; }

  void SetPlot(vtkPlotRadViz* plot) { this->Plot = plot; }

  void SetView(vtkContextView* view) { this->View = view; }

  // This Execute method triggers when the event fires
  void Execute(vtkObject* caller, unsigned long eventId, void* callData) override
  {
    if (eventId == vtkCommand::KeyPressEvent)
    {
      // Safely cast the invoker to an interactor
      vtkRenderWindowInteractor* interactor = dynamic_cast<vtkRenderWindowInteractor*>(caller);

      if (interactor)
      {
        // Access member variables or your own functions here
        std::string key = interactor->GetKeySym();
        if (key == "r" || key == "R")
        {
          this->Plot->SetLensRadius(this->Plot->GetRadius());
          this->Plot->SetLensCenter(this->Plot->GetCenter());
          this->View->GetScene()->SetDirty(true);
        }
      }
    }
  }
  vtkPlotRadViz* Plot = nullptr;
  vtkContextView* View = nullptr;
};

//------------------------------------------------------------------------------
int TestRadVizChartDataSet(int argc, char** argv)
{
  int variant;
  VTK_FROM_CHARS_IF_ERROR_RETURN(argv[1], variant, EXIT_FAILURE);

  vtkNew<vtkTesting> testUtils;
  testUtils->AddArguments(argc, argv);
  std::string dataRoot = testUtils->GetDataRoot();

  // Set up a 2D scene, add an RadViz chart to it
  vtkNew<vtkContextView> view;
  view->GetRenderer()->SetBackground(1.0, 1.0, 1.0);
  view->GetRenderWindow()->SetSize(600, 350);
  vtkNew<vtkChartRadViz> chart;
  view->GetScene()->AddItem(chart);

  vtkNew<vtkXMLImageDataReader> reader;
  reader->SetFileName((dataRoot + "/Data/Ce_Co_Fe_Gd_small.vti").c_str());
  reader->Update();

  // Add a RadViz plot
  vtkPlotRadViz* plot = vtkPlotRadViz::SafeDownCast(chart->AddPlot(0));
  plot->GetPen()->SetOpacity(20);
  plot->SetLensVisibility(true);
  plot->GetPen()->SetWidth(6);
  if (variant)
  {
    plot->ColorWheelVisibilityOff();
    plot->ScalarVisibilityOn();
  }

  plot->SetInputData(vtkDataSet::SafeDownCast(reader->GetOutputDataObject(0)));

  chart->SetShowLegend(false);

  chart->SetTitle("Battery dataset (Ce, Co, Fe, Gd)");

  vtkNew<ResetLens> resetLens;
  resetLens->SetPlot(plot);
  resetLens->SetView(view);
  // Finally render the scene and compare the image to a reference image
  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->GetInteractor()->AddObserver(vtkCommand::KeyPressEvent, resetLens);
  view->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
