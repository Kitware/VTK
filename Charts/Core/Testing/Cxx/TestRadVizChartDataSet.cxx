// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "TestParallelCoordinatesUtilities.h"
#include "vtkChartRadViz.h"
#include "vtkContextScene.h"
#include "vtkContextView.h"
#include "vtkDataSet.h"
#include "vtkNew.h"
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
  void Execute(vtkObject* caller, unsigned long eventId, void* vtkNotUsed(callData)) override
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
          this->Plot->SetSceneLensRadius(this->Plot->GetSceneRadius());
          this->Plot->SetSceneLensCenter(this->Plot->GetSceneCenter());
          this->View->GetScene()->SetDirty(true);
        }
      }
    }
  }
  vtkPlotRadViz* Plot = nullptr;
  vtkContextView* View = nullptr;
};

enum TestType
{
  COLOR_WHEEL_VISIBLE,
  SCALAR_COLOR,
  HIDE_COLUMN
};
//------------------------------------------------------------------------------
int TestRadVizChartDataSet(int argc, char* argv[])
{
  int testType;
  VTK_FROM_CHARS_IF_ERROR_RETURN(argv[1], testType, EXIT_FAILURE);

  vtkNew<vtkTesting> testUtils;
  testUtils->AddArguments(argc, argv);
  std::string dataRoot = testUtils->GetDataRoot();

  // Set up a 2D scene, add an RadViz chart to it
  vtkNew<vtkContextView> view;
  view->GetRenderer()->SetBackground(1.0, 1.0, 1.0);
  view->GetRenderWindow()->SetSize(600, 350);
  vtkNew<vtkChartRadViz> chart;
  view->GetScene()->AddItem(chart);

  // Downsampled MIEC (mixed ionic-electronic conductor) dataset from:
  // Ge, M., Huang, X., Yan, H. et al. Three-dimensional imaging of grain
  // boundaries via quantitative fluorescence X-ray tomography analysis.
  // Commun Mater 3, 37 (2022). https://doi.org/10.1038/s43246-022-00259-x
  vtkNew<vtkXMLImageDataReader> reader;
  reader->SetFileName(
    (dataRoot + "/Data/Ce_Co_Fe_Gd_small_10.1038_s43246-022-00259-x.vti").c_str());
  reader->Update();

  // Add a RadViz plot
  vtkPlotRadViz* plot = vtkPlotRadViz::SafeDownCast(chart->AddPlot(0));
  plot->SetLensVisibility(true);

  // The input must be set before configuring column visibility, since
  // hiding a column needs to know the full column list of the current
  // input to keep every other column visible.
  plot->SetInputDataSet(vtkDataSet::SafeDownCast(reader->GetOutputDataObject(0)));

  if (testType == SCALAR_COLOR)
  {
    plot->ColorWheelVisibilityOff();
    plot->ScalarVisibilityOn();
  }
  if (testType == HIDE_COLUMN)
  {
    plot->ColorWheelVisibilityOff();
    plot->ScalarVisibilityOn();
    plot->SetColumnVisibility("Co", false);
  }

  chart->SetShowLegend(false);

  chart->SetTitle("MIEC dataset (Ce, Co, Fe, Gd)");

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
