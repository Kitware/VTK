// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "TestParallelCoordinatesUtilities.h"
#include "vtkChartRadViz.h"
#include "vtkContextScene.h"
#include "vtkContextView.h"
#include "vtkDataSet.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkNew.h"
#include "vtkPlotRadViz.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkStringScanner.h"
#include "vtkTesting.h"
#include "vtkXMLImageDataReader.h"

static const char* eventLogVertices = "# StreamVersion 1.3\n"
                                      "ExposeEvent 0 349 0 0 0 0 0\n"
                                      "EnterEvent 524 4 0 0 0 0 0\n"
                                      "MouseMoveEvent 439 160 0 0 0 0 0\n"
                                      "TimerEvent 439 160 0 0 0 0 0\n"
                                      "LeftButtonPressEvent 439 160 0 0 0 0 0\n"
                                      "MouseMoveEvent 440 160 0 0 0 0 0\n"
                                      "MouseMoveEvent 442 166 0 0 0 0 0\n"
                                      "MouseMoveEvent 442 170 0 0 0 0 0\n"
                                      "MouseMoveEvent 443 181 0 0 0 0 0\n"
                                      "MouseMoveEvent 443 185 0 0 0 0 0\n"
                                      "TimerEvent 443 185 0 0 0 0 0\n"
                                      "MouseMoveEvent 443 195 0 0 0 0 0\n"
                                      "MouseMoveEvent 443 203 0 0 0 0 0\n"
                                      "MouseMoveEvent 443 207 0 0 0 0 0\n"
                                      "MouseMoveEvent 442 213 0 0 0 0 0\n"
                                      "MouseMoveEvent 349 296 0 0 0 0 0\n"
                                      "LeftButtonReleaseEvent 349 296 0 0 0 0 0\n"
                                      "TimerEvent 349 296 0 0 0 0 0\n"
                                      "MouseMoveEvent 346 292 0 0 0 0 0\n"
                                      "LeftButtonPressEvent 346 292 0 0 0 0 0\n"
                                      "MouseMoveEvent 346 293 0 0 0 0 0\n"
                                      "MouseMoveEvent 342 293 0 0 0 0 0\n"
                                      "MouseMoveEvent 323 293 0 0 0 0 0\n"
                                      "MouseMoveEvent 313 293 0 0 0 0 0\n"
                                      "MouseMoveEvent 306 292 0 0 0 0 0\n"
                                      "TimerEvent 306 292 0 0 0 0 0\n"
                                      "MouseMoveEvent 290 290 0 0 0 0 0\n"
                                      "MouseMoveEvent 273 287 0 0 0 0 0\n"
                                      "MouseMoveEvent 255 282 0 0 0 0 0\n"
                                      "MouseMoveEvent 250 279 0 0 0 0 0\n"
                                      "MouseMoveEvent 235 268 0 0 0 0 0\n"
                                      "TimerEvent 235 268 0 0 0 0 0\n"
                                      "MouseMoveEvent 226 259 0 0 0 0 0\n"
                                      "MouseMoveEvent 224 256 0 0 0 0 0\n"
                                      "MouseMoveEvent 219 252 0 0 0 0 0\n"
                                      "MouseMoveEvent 170 43 0 0 0 0 0\n"
                                      "LeftButtonReleaseEvent 170 43 0 0 0 0 0\n"
                                      "TimerEvent 170 43 0 0 0 0 0\n"
                                      "MouseMoveEvent 198 67 0 0 0 0 0\n"
                                      "LeftButtonPressEvent 198 67 0 0 0 0 0\n"
                                      "MouseMoveEvent 198 67 0 0 0 0 0\n"
                                      "MouseMoveEvent 206 60 0 0 0 0 0\n"
                                      "MouseMoveEvent 221 47 0 0 0 0 0\n"
                                      "MouseMoveEvent 228 39 0 0 0 0 0\n"
                                      "MouseMoveEvent 241 34 0 0 0 0 0\n"
                                      "TimerEvent 241 34 0 0 0 0 0\n"
                                      "MouseMoveEvent 261 36 0 0 0 0 0\n"
                                      "MouseMoveEvent 297 41 0 0 0 0 0\n"
                                      "MouseMoveEvent 326 46 0 0 0 0 0\n"
                                      "MouseMoveEvent 359 52 0 0 0 0 0\n"
                                      "MouseMoveEvent 381 56 0 0 0 0 0\n"
                                      "TimerEvent 381 56 0 0 0 0 0\n"
                                      "MouseMoveEvent 400 61 0 0 0 0 0\n"
                                      "MouseMoveEvent 415 69 0 0 0 0 0\n"
                                      "MouseMoveEvent 424 83 0 0 0 0 0\n"
                                      "MouseMoveEvent 430 103 0 0 0 0 0\n"
                                      "MouseMoveEvent 433 124 0 0 0 0 0\n"
                                      "TimerEvent 433 124 0 0 0 0 0\n"
                                      "MouseMoveEvent 434 147 0 0 0 0 0\n"
                                      "MouseMoveEvent 435 161 0 0 0 0 0\n"
                                      "MouseMoveEvent 435 178 0 0 0 0 0\n"
                                      "MouseMoveEvent 435 191 0 0 0 0 0\n"
                                      "MouseMoveEvent 433 208 0 0 0 0 0\n"
                                      "TimerEvent 433 208 0 0 0 0 0\n"
                                      "MouseMoveEvent 430 221 0 0 0 0 0\n"
                                      "MouseMoveEvent 421 234 0 0 0 0 0\n"
                                      "MouseMoveEvent 406 247 0 0 0 0 0\n"
                                      "MouseMoveEvent 397 253 0 0 0 0 0\n"
                                      "MouseMoveEvent 386 257 0 0 0 0 0\n"
                                      "TimerEvent 386 257 0 0 0 0 0\n"
                                      "MouseMoveEvent 369 266 0 0 0 0 0\n"
                                      "MouseMoveEvent 353 275 0 0 0 0 0\n"
                                      "MouseMoveEvent 344 282 0 0 0 0 0\n"
                                      "MouseMoveEvent 365 282 0 0 0 0 0\n"
                                      "MouseMoveEvent 393 273 0 0 0 0 0\n"
                                      "TimerEvent 393 273 0 0 0 0 0\n"
                                      "MouseMoveEvent 416 259 0 0 0 0 0\n"
                                      "MouseMoveEvent 431 231 0 0 0 0 0\n"
                                      "MouseMoveEvent 430 178 0 0 0 0 0\n"
                                      "MouseMoveEvent 421 146 0 0 0 0 0\n"
                                      "MouseMoveEvent 418 130 0 0 0 0 0\n"
                                      "TimerEvent 418 130 0 0 0 0 0\n"
                                      "MouseMoveEvent 414 111 0 0 0 0 0\n"
                                      "MouseMoveEvent 411 83 0 0 0 0 0\n"
                                      "MouseMoveEvent 403 59 0 0 0 0 0\n"
                                      "MouseMoveEvent 399 53 0 0 0 0 0\n"
                                      "MouseMoveEvent 419 88 0 0 0 0 0\n"
                                      "TimerEvent 419 88 0 0 0 0 0\n"
                                      "MouseMoveEvent 434 129 0 0 0 0 0\n"
                                      "MouseMoveEvent 436 159 0 0 0 0 0\n"
                                      "MouseMoveEvent 431 185 0 0 0 0 0\n"
                                      "MouseMoveEvent 422 207 0 0 0 0 0\n"
                                      "MouseMoveEvent 410 228 0 0 0 0 0\n"
                                      "TimerEvent 410 228 0 0 0 0 0\n"
                                      "MouseMoveEvent 253 270 0 0 0 0 0\n"
                                      "LeftButtonReleaseEvent 253 270 0 0 0 0 0\n"
                                      "TimerEvent 253 270 0 0 0 0 0\n"
                                      "MouseMoveEvent 518 230 0 0 0 0 0\n"
                                      "TimerEvent 518 230 0 0 0 0 0\n";

static const char* eventLogLens = "# StreamVersion 1.3\n"
                                  "ExposeEvent 0 349 0 0 0 0 0\n"
                                  "TimerEvent 0 349 0 0 0 0 0\n"
                                  "EnterEvent 461 4 0 0 0 0 0\n"
                                  "MouseMoveEvent 461 4 0 0 0 0 0\n"
                                  "MouseMoveEvent 457 9 0 0 0 0 0\n"
                                  "MouseMoveEvent 450 14 0 0 0 0 0\n"
                                  "MouseMoveEvent 446 14 0 0 0 0 0\n"
                                  "MouseMoveEvent 446 16 0 0 0 0 0\n"
                                  "MouseMoveEvent 446 16 0 0 0 0 0\n"
                                  "MouseMoveEvent 449 21 0 0 0 0 0\n"
                                  "MouseMoveEvent 450 36 0 0 0 0 0\n"
                                  "MouseMoveEvent 446 43 0 0 0 0 0\n"
                                  "MouseMoveEvent 437 60 0 0 0 0 0\n"
                                  "MouseMoveEvent 429 93 0 0 0 0 0\n"
                                  "MouseMoveEvent 427 129 0 0 0 0 0\n"
                                  "MouseMoveEvent 424 162 0 0 0 0 0\n"
                                  "MouseMoveEvent 418 169 0 0 0 0 0\n"
                                  "MouseMoveEvent 414 169 0 0 0 0 0\n"
                                  "MouseMoveEvent 410 168 0 0 0 0 0\n"
                                  "MouseMoveEvent 401 174 0 0 0 0 0\n"
                                  "MouseMoveEvent 394 198 0 0 0 0 0\n"
                                  "MouseMoveEvent 388 220 0 0 0 0 0\n"
                                  "MouseMoveEvent 383 242 0 0 0 0 0\n"
                                  "MouseMoveEvent 379 264 0 0 0 0 0\n"
                                  "MouseMoveEvent 378 273 0 0 0 0 0\n"
                                  "MouseMoveEvent 377 274 0 0 0 0 0\n"
                                  "MouseMoveEvent 379 272 0 0 0 0 0\n"
                                  "MouseMoveEvent 383 272 0 0 0 0 0\n"
                                  "LeftButtonPressEvent 383 272 0 0 0 0 0\n"
                                  "MouseMoveEvent 383 272 0 0 0 0 0\n"
                                  "TimerEvent 383 272 0 0 0 0 0\n"
                                  "TimerEvent 382 271 0 0 0 0 0\n"
                                  "MouseMoveEvent 377 268 0 0 0 0 0\n"
                                  "MouseMoveEvent 374 265 0 0 0 0 0\n"
                                  "MouseMoveEvent 372 262 0 0 0 0 0\n"
                                  "MouseMoveEvent 370 261 0 0 0 0 0\n"
                                  "MouseMoveEvent 368 259 0 0 0 0 0\n"
                                  "MouseMoveEvent 366 255 0 0 0 0 0\n"
                                  "MouseMoveEvent 363 252 0 0 0 0 0\n"
                                  "MouseMoveEvent 361 249 0 0 0 0 0\n"
                                  "MouseMoveEvent 359 247 0 0 0 0 0\n"
                                  "MouseMoveEvent 357 243 0 0 0 0 0\n"
                                  "MouseMoveEvent 355 241 0 0 0 0 0\n"
                                  "TimerEvent 354 239 0 0 0 0 0\n"
                                  "TimerEvent 352 236 0 0 0 0 0\n"
                                  "TimerEvent 350 233 0 0 0 0 0\n"
                                  "MouseMoveEvent 349 229 0 0 0 0 0\n"
                                  "MouseMoveEvent 348 226 0 0 0 0 0\n"
                                  "TimerEvent 347 224 0 0 0 0 0\n"
                                  "TimerEvent 346 221 0 0 0 0 0\n"
                                  "MouseMoveEvent 343 218 0 0 0 0 0\n"
                                  "MouseMoveEvent 342 215 0 0 0 0 0\n"
                                  "MouseMoveEvent 341 212 0 0 0 0 0\n"
                                  "MouseMoveEvent 340 210 0 0 0 0 0\n"
                                  "MouseMoveEvent 339 209 0 0 0 0 0\n"
                                  "MouseMoveEvent 338 207 0 0 0 0 0\n"
                                  "TimerEvent 338 206 0 0 0 0 0\n"
                                  "TimerEvent 338 204 0 0 0 0 0\n"
                                  "LeftButtonReleaseEvent 338 204 0 0 0 0 0\n"
                                  "MouseMoveEvent 337 204 0 0 0 0 0\n"
                                  "MouseMoveEvent 334 200 0 0 0 0 0\n"
                                  "MouseMoveEvent 328 194 0 0 0 0 0\n"
                                  "MouseMoveEvent 323 188 0 0 0 0 0\n"
                                  "MouseMoveEvent 320 187 0 0 0 0 0\n"
                                  "MouseMoveEvent 318 185 0 0 0 0 0\n"
                                  "MouseMoveEvent 316 182 0 0 0 0 0\n"
                                  "MouseMoveEvent 314 180 0 0 0 0 0\n"
                                  "MouseMoveEvent 310 178 0 0 0 0 0\n"
                                  "LeftButtonPressEvent 310 178 0 0 0 0 0\n"
                                  "MouseMoveEvent 310 178 0 0 0 0 0\n"
                                  "TimerEvent 310 178 0 0 0 0 0\n"
                                  "MouseMoveEvent 311 177 0 0 0 0 0\n"
                                  "MouseMoveEvent 314 176 0 0 0 0 0\n"
                                  "MouseMoveEvent 316 174 0 0 0 0 0\n"
                                  "MouseMoveEvent 320 171 0 0 0 0 0\n"
                                  "MouseMoveEvent 324 169 0 0 0 0 0\n"
                                  "MouseMoveEvent 328 164 0 0 0 0 0\n"
                                  "MouseMoveEvent 330 161 0 0 0 0 0\n"
                                  "MouseMoveEvent 332 159 0 0 0 0 0\n"
                                  "MouseMoveEvent 335 154 0 0 0 0 0\n"
                                  "MouseMoveEvent 337 150 0 0 0 0 0\n"
                                  "TimerEvent 339 148 0 0 0 0 0\n"
                                  "MouseMoveEvent 341 144 0 0 0 0 0\n"
                                  "MouseMoveEvent 343 140 0 0 0 0 0\n"
                                  "MouseMoveEvent 346 134 0 0 0 0 0\n"
                                  "MouseMoveEvent 348 131 0 0 0 0 0\n"
                                  "MouseMoveEvent 348 128 0 0 0 0 0\n"
                                  "MouseMoveEvent 349 125 0 0 0 0 0\n"
                                  "MouseMoveEvent 352 120 0 0 0 0 0\n"
                                  "MouseMoveEvent 353 115 0 0 0 0 0\n"
                                  "MouseMoveEvent 355 112 0 0 0 0 0\n"
                                  "MouseMoveEvent 357 109 0 0 0 0 0\n"
                                  "TimerEvent 359 107 0 0 0 0 0\n"
                                  "TimerEvent 360 105 0 0 0 0 0\n"
                                  "MouseMoveEvent 363 102 0 0 0 0 0\n"
                                  "MouseMoveEvent 364 101 0 0 0 0 0\n"
                                  "TimerEvent 366 101 0 0 0 0 0\n"
                                  "TimerEvent 366 100 0 0 0 0 0\n"
                                  "LeftButtonReleaseEvent 366 100 0 0 0 0 0\n";

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
int TestRadVizChartInteraction(int argc, char* argv[])
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
  plot->ColorWheelVisibilityOff();
  plot->ScalarVisibilityOn();
  plot->SetOutsideLensAlpha(0.01);

  plot->SetInputDataSet(vtkDataSet::SafeDownCast(reader->GetOutputDataObject(0)));

  chart->SetShowLegend(false);

  chart->SetTitle("MIEC dataset (Ce, Co, Fe, Gd)");

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
