// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "TestParallelCoordinatesUtilities.h"
#include "vtkChartRadViz.h"
#include "vtkContextScene.h"
#include "vtkContextView.h"
#include "vtkDataSet.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkNew.h"
#include "vtkPen.h"
#include "vtkPlotRadViz.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkStringScanner.h"
#include "vtkTesting.h"
#include "vtkXMLImageDataReader.h"

static const char* eventLogVertices = "# StreamVersion 1.2\n"
                                      "ExposeEvent 0 349 0 0 0 0 0\n"
                                      "TimerEvent 0 349 0 0 0 0 0\n"
                                      "EnterEvent 596 293 0 0 0 0 0\n"
                                      "MouseMoveEvent 301 313 0 0 0 0 0\n"
                                      "LeftButtonPressEvent 301 313 0 0 0 0 0\n"
                                      "MouseMoveEvent 300 313 0 0 0 0 0\n"
                                      "TimerEvent 290 313 0 0 0 0 0\n"
                                      "MouseMoveEvent 287 313 0 0 0 0 0\n"
                                      "MouseMoveEvent 278 313 0 0 0 0 0\n"
                                      "MouseMoveEvent 276 313 0 0 0 0 0\n"
                                      "TimerEvent 276 313 0 0 0 0 0\n"
                                      "MouseMoveEvent 275 313 0 0 0 0 0\n"
                                      "MouseMoveEvent 275 312 0 0 0 0 0\n"
                                      "MouseMoveEvent 274 312 0 0 0 0 0\n"
                                      "MouseMoveEvent 274 311 0 0 0 0 0\n"
                                      "MouseMoveEvent 273 311 0 0 0 0 0\n"
                                      "TimerEvent 273 311 0 0 0 0 0\n"
                                      "MouseMoveEvent 272 311 0 0 0 0 0\n"
                                      "MouseMoveEvent 271 310 0 0 0 0 0\n"
                                      "MouseMoveEvent 270 310 0 0 0 0 0\n"
                                      "TimerEvent 270 310 0 0 0 0 0\n"
                                      "MouseMoveEvent 267 308 0 0 0 0 0\n"
                                      "TimerEvent 267 308 0 0 0 0 0\n"
                                      "LeftButtonReleaseEvent 267 308 0 0 0 0 0\n"
                                      "MouseMoveEvent 269 307 0 0 0 0 0\n"
                                      "MouseMoveEvent 445 200 0 0 0 0 0\n"
                                      "LeftButtonPressEvent 445 200 0 0 0 0 0\n"
                                      "MouseMoveEvent 444 200 0 0 0 0 0\n"
                                      "MouseMoveEvent 441 213 0 0 0 0 0\n"
                                      "MouseMoveEvent 441 214 0 0 0 0 0\n"
                                      "MouseMoveEvent 440 216 0 0 0 0 0\n"
                                      "TimerEvent 440 216 0 0 0 0 0\n"
                                      "MouseMoveEvent 439 218 0 0 0 0 0\n"
                                      "MouseMoveEvent 439 219 0 0 0 0 0\n"
                                      "TimerEvent 439 219 0 0 0 0 0\n"
                                      "MouseMoveEvent 439 220 0 0 0 0 0\n"
                                      "MouseMoveEvent 439 223 0 0 0 0 0\n"
                                      "MouseMoveEvent 439 225 0 0 0 0 0\n"
                                      "TimerEvent 439 225 0 0 0 0 0\n"
                                      "MouseMoveEvent 433 256 0 0 0 0 0\n"
                                      "LeftButtonReleaseEvent 433 256 0 0 0 0 0\n"
                                      "TimerEvent 433 256 0 0 0 0 0\n"
                                      "MouseMoveEvent 435 254 0 0 0 0 0\n"
                                      "MouseMoveEvent 385 46 0 0 0 0 0\n"
                                      "LeftButtonPressEvent 385 46 0 0 0 0 0\n"
                                      "MouseMoveEvent 386 46 0 0 0 0 0\n"
                                      "TimerEvent 394 48 0 0 0 0 0\n"
                                      "MouseMoveEvent 401 54 0 0 0 0 0\n"
                                      "MouseMoveEvent 432 85 0 0 0 0 0\n"
                                      "TimerEvent 432 85 0 0 0 0 0\n"
                                      "MouseMoveEvent 440 97 0 0 0 0 0\n"
                                      "TimerEvent 479 166 0 0 0 0 0\n"
                                      "MouseMoveEvent 490 221 0 0 0 0 0\n"
                                      "TimerEvent 486 307 0 0 0 0 0\n"
                                      "MouseMoveEvent 468 368 0 0 0 0 0\n"
                                      "TimerEvent 468 368 0 0 0 0 0\n"
                                      "MouseMoveEvent 468 370 0 0 0 0 0\n"
                                      "TimerEvent 467 370 0 0 0 0 0\n"
                                      "MouseMoveEvent 467 371 0 0 0 0 0\n"
                                      "TimerEvent 467 371 0 0 0 0 0\n"
                                      "LeftButtonReleaseEvent 467 371 0 0 0 0 0\n"
                                      "LeaveEvent 467 371 0 0 0 0 0\n";

static const char* eventLogLens = "# StreamVersion 1.2\n"
                                  "ExposeEvent 0 349 0 0 0 0 0\n"
                                  "TimerEvent 0 349 0 0 0 0 0\n"
                                  "EnterEvent 598 302 0 0 0 0 0\n"
                                  "MouseMoveEvent 393 281 0 0 0 0 0\n"
                                  "LeftButtonPressEvent 393 281 0 0 0 0 0\n"
                                  "MouseMoveEvent 393 280 0 0 0 0 0\n"
                                  "TimerEvent 393 279 0 0 0 0 0\n"
                                  "MouseMoveEvent 381 267 0 0 0 0 0\n"
                                  "TimerEvent 381 266 0 0 0 0 0\n"
                                  "MouseMoveEvent 378 259 0 0 0 0 0\n"
                                  "TimerEvent 378 259 0 0 0 0 0\n"
                                  "MouseMoveEvent 376 255 0 0 0 0 0\n"
                                  "TimerEvent 374 253 0 0 0 0 0\n"
                                  "MouseMoveEvent 372 251 0 0 0 0 0\n"
                                  "TimerEvent 372 248 0 0 0 0 0\n"
                                  "MouseMoveEvent 371 247 0 0 0 0 0\n"
                                  "TimerEvent 371 247 0 0 0 0 0\n"
                                  "MouseMoveEvent 371 246 0 0 0 0 0\n"
                                  "TimerEvent 371 246 0 0 0 0 0\n"
                                  "MouseMoveEvent 369 238 0 0 0 0 0\n"
                                  "TimerEvent 368 235 0 0 0 0 0\n"
                                  "MouseMoveEvent 367 232 0 0 0 0 0\n"
                                  "TimerEvent 366 231 0 0 0 0 0\n"
                                  "MouseMoveEvent 366 229 0 0 0 0 0\n"
                                  "TimerEvent 366 229 0 0 0 0 0\n"
                                  "MouseMoveEvent 364 223 0 0 0 0 0\n"
                                  "TimerEvent 364 223 0 0 0 0 0\n"
                                  "MouseMoveEvent 364 221 0 0 0 0 0\n"
                                  "TimerEvent 364 221 0 0 0 0 0\n"
                                  "LeftButtonReleaseEvent 364 221 0 0 0 0 0\n"
                                  "MouseMoveEvent 363 220 0 0 0 0 0\n"
                                  "LeftButtonPressEvent 315 185 0 0 0 0 0\n"
                                  "MouseMoveEvent 315 186 0 0 0 0 0\n"
                                  "TimerEvent 316 188 0 0 0 0 0\n"
                                  "MouseMoveEvent 321 194 0 0 0 0 0\n"
                                  "TimerEvent 325 197 0 0 0 0 0\n"
                                  "MouseMoveEvent 328 198 0 0 0 0 0\n"
                                  "TimerEvent 331 199 0 0 0 0 0\n"
                                  "MouseMoveEvent 334 200 0 0 0 0 0\n"
                                  "TimerEvent 337 202 0 0 0 0 0\n"
                                  "MouseMoveEvent 343 206 0 0 0 0 0\n"
                                  "TimerEvent 346 208 0 0 0 0 0\n"
                                  "MouseMoveEvent 350 209 0 0 0 0 0\n"
                                  "TimerEvent 352 210 0 0 0 0 0\n"
                                  "MouseMoveEvent 355 213 0 0 0 0 0\n"
                                  "TimerEvent 355 213 0 0 0 0 0\n"
                                  "MouseMoveEvent 358 214 0 0 0 0 0\n"
                                  "TimerEvent 359 217 0 0 0 0 0\n"
                                  "MouseMoveEvent 360 217 0 0 0 0 0\n"
                                  "TimerEvent 363 217 0 0 0 0 0\n"
                                  "MouseMoveEvent 364 219 0 0 0 0 0\n"
                                  "TimerEvent 364 219 0 0 0 0 0\n"
                                  "MouseMoveEvent 364 220 0 0 0 0 0\n"
                                  "TimerEvent 366 223 0 0 0 0 0\n"
                                  "MouseMoveEvent 367 225 0 0 0 0 0\n"
                                  "TimerEvent 367 225 0 0 0 0 0\n"
                                  "MouseMoveEvent 367 226 0 0 0 0 0\n"
                                  "TimerEvent 367 226 0 0 0 0 0\n"
                                  "MouseMoveEvent 367 228 0 0 0 0 0\n"
                                  "TimerEvent 367 228 0 0 0 0 0\n"
                                  "MouseMoveEvent 367 229 0 0 0 0 0\n"
                                  "MouseMoveEvent 367 230 0 0 0 0 0\n"
                                  "TimerEvent 367 230 0 0 0 0 0\n"
                                  "LeftButtonReleaseEvent 367 230 0 0 0 0 0\n"
                                  "MouseMoveEvent 369 230 0 0 0 0 0\n";

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
int TestRadVizChartInteraction(int argc, char** argv)
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
  plot->SetLensVisibility(true);
  plot->GetPen()->SetOpacity(20);
  plot->GetPen()->SetWidth(6);
  plot->ColorWheelVisibilityOff();
  plot->ScalarVisibilityOn();
  plot->SetOutsideLensAlphaCap(0.01);

  plot->SetInputData(vtkDataSet::SafeDownCast(reader->GetOutputDataObject(0)));

  chart->SetShowLegend(false);

  chart->SetTitle("Battery dataset (Ce, Co, Fe, Gd)");

  vtkNew<ResetLens> resetLens;
  resetLens->SetPlot(plot);
  resetLens->SetView(view);
  // Finally render the scene and compare the image to a reference image
  view->GetRenderWindow()->SetMultiSamples(0);
  vtkSmartPointer<vtkInteractorEventRecorder> recorder =
    vtkSmartPointer<vtkInteractorEventRecorder>::New();
  view->GetInteractor()->Initialize();
  recorder->SetInteractor(view->GetInteractor());

// #define RECORD_TESTING
#ifdef RECORD_TESTING
  recorder->SetFileName("record.txt");
  recorder->SetEnabled(true);
  recorder->Record();
  view->GetInteractor()->Start();
#else
  recorder->ReadFromInputStringOn();
  if (variant)
  {
    recorder->SetInputString(eventLogLens);
  }
  else
  {
    recorder->SetInputString(eventLogVertices);
  }
  recorder->Play();
#endif

  view->GetInteractor()->AddObserver(vtkCommand::KeyPressEvent, resetLens);
  view->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
