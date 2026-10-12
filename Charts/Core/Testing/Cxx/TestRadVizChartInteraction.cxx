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
                                  "EnterEvent 599 225 0 0 0 0 0\n"
                                  "MouseMoveEvent 430 222 0 0 0 0 0\n"
                                  "TimerEvent 430 222 0 0 0 0 0\n"
                                  "LeftButtonPressEvent 430 222 0 0 0 0 0\n"
                                  "MouseMoveEvent 430 221 0 0 0 0 0\n"
                                  "MouseMoveEvent 430 220 0 0 0 0 0\n"
                                  "MouseMoveEvent 430 219 0 0 0 0 0\n"
                                  "MouseMoveEvent 427 218 0 0 0 0 0\n"
                                  "MouseMoveEvent 426 216 0 0 0 0 0\n"
                                  "TimerEvent 426 216 0 0 0 0 0\n"
                                  "MouseMoveEvent 423 215 0 0 0 0 0\n"
                                  "MouseMoveEvent 416 212 0 0 0 0 0\n"
                                  "MouseMoveEvent 412 210 0 0 0 0 0\n"
                                  "MouseMoveEvent 410 209 0 0 0 0 0\n"
                                  "MouseMoveEvent 407 207 0 0 0 0 0\n"
                                  "TimerEvent 407 207 0 0 0 0 0\n"
                                  "MouseMoveEvent 403 204 0 0 0 0 0\n"
                                  "MouseMoveEvent 401 203 0 0 0 0 0\n"
                                  "MouseMoveEvent 399 201 0 0 0 0 0\n"
                                  "MouseMoveEvent 397 200 0 0 0 0 0\n"
                                  "MouseMoveEvent 397 199 0 0 0 0 0\n"
                                  "TimerEvent 397 199 0 0 0 0 0\n"
                                  "MouseMoveEvent 396 198 0 0 0 0 0\n"
                                  "MouseMoveEvent 393 195 0 0 0 0 0\n"
                                  "MouseMoveEvent 392 194 0 0 0 0 0\n"
                                  "MouseMoveEvent 391 194 0 0 0 0 0\n"
                                  "MouseMoveEvent 390 194 0 0 0 0 0\n"
                                  "TimerEvent 390 194 0 0 0 0 0\n"
                                  "MouseMoveEvent 389 193 0 0 0 0 0\n"
                                  "MouseMoveEvent 388 191 0 0 0 0 0\n"
                                  "MouseMoveEvent 388 191 0 0 0 0 0\n"
                                  "MouseMoveEvent 387 189 0 0 0 0 0\n"
                                  "MouseMoveEvent 386 189 0 0 0 0 0\n"
                                  "TimerEvent 386 189 0 0 0 0 0\n"
                                  "MouseMoveEvent 385 187 0 0 0 0 0\n"
                                  "MouseMoveEvent 384 186 0 0 0 0 0\n"
                                  "MouseMoveEvent 383 185 0 0 0 0 0\n"
                                  "MouseMoveEvent 382 182 0 0 0 0 0\n"
                                  "MouseMoveEvent 381 181 0 0 0 0 0\n"
                                  "TimerEvent 381 181 0 0 0 0 0\n"
                                  "MouseMoveEvent 381 181 0 0 0 0 0\n"
                                  "MouseMoveEvent 381 179 0 0 0 0 0\n"
                                  "MouseMoveEvent 380 178 0 0 0 0 0\n"
                                  "MouseMoveEvent 379 176 0 0 0 0 0\n"
                                  "MouseMoveEvent 378 175 0 0 0 0 0\n"
                                  "TimerEvent 378 175 0 0 0 0 0\n"
                                  "MouseMoveEvent 377 173 0 0 0 0 0\n"
                                  "LeftButtonReleaseEvent 377 173 0 0 0 0 0\n"
                                  "TimerEvent 377 173 0 0 0 0 0\n"
                                  "MouseMoveEvent 339 169 0 0 0 0 0\n"
                                  "LeftButtonPressEvent 339 169 0 0 0 0 0\n"
                                  "MouseMoveEvent 339 170 0 0 0 0 0\n"
                                  "MouseMoveEvent 339 171 0 0 0 0 0\n"
                                  "MouseMoveEvent 345 175 0 0 0 0 0\n"
                                  "MouseMoveEvent 348 176 0 0 0 0 0\n"
                                  "MouseMoveEvent 350 177 0 0 0 0 0\n"
                                  "TimerEvent 350 177 0 0 0 0 0\n"
                                  "MouseMoveEvent 354 181 0 0 0 0 0\n"
                                  "MouseMoveEvent 356 182 0 0 0 0 0\n"
                                  "MouseMoveEvent 357 183 0 0 0 0 0\n"
                                  "MouseMoveEvent 362 186 0 0 0 0 0\n"
                                  "MouseMoveEvent 362 187 0 0 0 0 0\n"
                                  "TimerEvent 362 187 0 0 0 0 0\n"
                                  "MouseMoveEvent 363 188 0 0 0 0 0\n"
                                  "MouseMoveEvent 364 189 0 0 0 0 0\n"
                                  "MouseMoveEvent 367 192 0 0 0 0 0\n"
                                  "MouseMoveEvent 368 193 0 0 0 0 0\n"
                                  "MouseMoveEvent 370 195 0 0 0 0 0\n"
                                  "TimerEvent 370 195 0 0 0 0 0\n"
                                  "MouseMoveEvent 370 196 0 0 0 0 0\n"
                                  "MouseMoveEvent 372 199 0 0 0 0 0\n"
                                  "MouseMoveEvent 373 200 0 0 0 0 0\n"
                                  "MouseMoveEvent 373 203 0 0 0 0 0\n"
                                  "MouseMoveEvent 374 204 0 0 0 0 0\n"
                                  "TimerEvent 374 204 0 0 0 0 0\n"
                                  "MouseMoveEvent 375 208 0 0 0 0 0\n"
                                  "MouseMoveEvent 375 210 0 0 0 0 0\n"
                                  "MouseMoveEvent 375 213 0 0 0 0 0\n"
                                  "MouseMoveEvent 375 215 0 0 0 0 0\n"
                                  "MouseMoveEvent 376 217 0 0 0 0 0\n"
                                  "TimerEvent 376 217 0 0 0 0 0\n"
                                  "MouseMoveEvent 376 220 0 0 0 0 0\n"
                                  "MouseMoveEvent 377 222 0 0 0 0 0\n"
                                  "MouseMoveEvent 377 223 0 0 0 0 0\n"
                                  "MouseMoveEvent 378 225 0 0 0 0 0\n"
                                  "MouseMoveEvent 379 227 0 0 0 0 0\n"
                                  "TimerEvent 379 227 0 0 0 0 0\n"
                                  "MouseMoveEvent 381 230 0 0 0 0 0\n"
                                  "MouseMoveEvent 382 232 0 0 0 0 0\n"
                                  "MouseMoveEvent 382 234 0 0 0 0 0\n"
                                  "MouseMoveEvent 384 235 0 0 0 0 0\n"
                                  "MouseMoveEvent 385 236 0 0 0 0 0\n"
                                  "TimerEvent 385 236 0 0 0 0 0\n"
                                  "MouseMoveEvent 386 236 0 0 0 0 0\n"
                                  "LeftButtonReleaseEvent 386 236 0 0 0 0 0\n"
                                  "TimerEvent 386 236 0 0 0 0 0\n"
                                  "MouseMoveEvent 524 249 0 0 0 0 0\n"
                                  "TimerEvent 524 249 0 0 0 0 0\n";

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
