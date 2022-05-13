#include <vtkCallbackCommand.h>
#include <vtkChartParallelCoordinates.h>
#include <vtkContextActor.h>
#include <vtkContextKeyEvent.h>
#include <vtkContextMouseEvent.h>
#include <vtkContextView.h>
#include <vtkDoubleArray.h>
#include <vtkIdTypeArray.h>
#include <vtkInteractorEventRecorder.h>
#include <vtkNew.h>
#include <vtkPlotParallelCoordinates.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkTable.h>

static const int expectedValues[2] = { 62, 105 };

static const char* eventLog = "# StreamVersion 1.1\n"
                              "ExposeEvent 0 399 0 0 0 0\n"
                              "RenderEvent 0 399 0 0 0 0\n"
                              "TimerEvent 0 399 0 0 0 0\n"
                              "RenderEvent 0 399 0 0 0 0\n"
                              "EnterEvent 599 141 0 0 0 0\n"
                              "MouseMoveEvent 599 141 0 0 0 0\n"
                              "MouseMoveEvent 220 91 0 0 0 0\n"
                              "KeyPressEvent 220 91 0 0 1 Control_L\n"
                              "CharEvent 220 91 0 0 1 Control_L\n"
                              "KeyPressEvent 220 91 2 0 1 Shift_L\n"
                              "CharEvent 220 91 2 0 1 Shift_L\n"
                              "LeftButtonPressEvent 220 91 3 0 0 Shift_L\n"
                              "TimerEvent 220 91 3 0 0 Shift_L\n"
                              "RenderEvent 220 91 3 0 0 Shift_L\n"
                              "MouseMoveEvent 220 96 3 0 0 Shift_L\n"
                              "MouseMoveEvent 219 115 3 0 0 Shift_L\n"
                              "TimerEvent 219 115 3 0 0 Shift_L\n"
                              "RenderEvent 219 115 3 0 0 Shift_L\n"
                              "MouseMoveEvent 219 118 3 0 0 Shift_L\n"
                              "MouseMoveEvent 219 127 3 0 0 Shift_L\n"
                              "TimerEvent 219 127 3 0 0 Shift_L\n"
                              "RenderEvent 219 127 3 0 0 Shift_L\n"
                              "MouseMoveEvent 219 128 3 0 0 Shift_L\n"
                              "MouseMoveEvent 219 130 3 0 0 Shift_L\n"
                              "TimerEvent 219 130 3 0 0 Shift_L\n"
                              "RenderEvent 219 130 3 0 0 Shift_L\n"
                              "MouseMoveEvent 219 130 3 0 0 Shift_L\n"
                              "MouseMoveEvent 219 131 3 0 0 Shift_L\n"
                              "TimerEvent 219 131 3 0 0 Shift_L\n"
                              "RenderEvent 219 131 3 0 0 Shift_L\n"
                              "MouseMoveEvent 219 132 3 0 0 Shift_L\n"
                              "MouseMoveEvent 219 139 3 0 0 Shift_L\n"
                              "TimerEvent 219 139 3 0 0 Shift_L\n"
                              "RenderEvent 219 139 3 0 0 Shift_L\n"
                              "MouseMoveEvent 219 140 3 0 0 Shift_L\n"
                              "MouseMoveEvent 219 143 3 0 0 Shift_L\n"
                              "TimerEvent 219 143 3 0 0 Shift_L\n"
                              "RenderEvent 219 143 3 0 0 Shift_L\n"
                              "MouseMoveEvent 219 144 3 0 0 Shift_L\n"
                              "MouseMoveEvent 219 147 3 0 0 Shift_L\n"
                              "TimerEvent 219 147 3 0 0 Shift_L\n"
                              "RenderEvent 219 147 3 0 0 Shift_L\n"
                              "MouseMoveEvent 219 149 3 0 0 Shift_L\n"
                              "MouseMoveEvent 219 152 3 0 0 Shift_L\n"
                              "TimerEvent 219 152 3 0 0 Shift_L\n"
                              "RenderEvent 219 152 3 0 0 Shift_L\n"
                              "MouseMoveEvent 219 153 3 0 0 Shift_L\n"
                              "MouseMoveEvent 219 155 3 0 0 Shift_L\n"
                              "MouseMoveEvent 219 160 3 0 0 Shift_L\n"
                              "TimerEvent 219 160 3 0 0 Shift_L\n"
                              "RenderEvent 219 160 3 0 0 Shift_L\n"
                              "LeftButtonReleaseEvent 219 160 3 0 0 Shift_L\n"
                              "TimerEvent 219 160 3 0 0 Shift_L\n"
                              "RenderEvent 219 160 3 0 0 Shift_L\n"
                              "MouseMoveEvent 220 160 3 0 0 Shift_L\n"
                              "MouseMoveEvent 221 159 3 0 0 Shift_L\n"
                              "MouseMoveEvent 410 86 3 0 0 Shift_L\n"
                              "KeyReleaseEvent 410 86 3 0 1 Control_L\n"
                              "MouseMoveEvent 408 85 1 0 0 Control_L\n"
                              "KeyReleaseEvent 408 85 1 0 1 Shift_L\n"
                              "MouseMoveEvent 405 83 0 0 0 Shift_L\n"
                              "MouseMoveEvent 403 83 0 0 0 Shift_L\n"
                              "MouseMoveEvent 384 82 0 0 0 Shift_L\n"
                              "KeyPressEvent 384 82 0 0 1 Control_L\n"
                              "CharEvent 384 82 0 0 1 Control_L\n"
                              "KeyPressEvent 384 82 2 0 1 Shift_L\n"
                              "CharEvent 384 82 2 0 1 Shift_L\n"
                              "MouseMoveEvent 383 82 3 0 0 Shift_L\n"
                              "MouseMoveEvent 379 80 3 0 0 Shift_L\n"
                              "LeftButtonPressEvent 379 80 3 0 0 Shift_L\n"
                              "TimerEvent 379 80 3 0 0 Shift_L\n"
                              "RenderEvent 379 80 3 0 0 Shift_L\n"
                              "MouseMoveEvent 379 83 3 0 0 Shift_L\n"
                              "MouseMoveEvent 379 95 3 0 0 Shift_L\n"
                              "TimerEvent 379 95 3 0 0 Shift_L\n"
                              "RenderEvent 379 95 3 0 0 Shift_L\n"
                              "MouseMoveEvent 379 97 3 0 0 Shift_L\n"
                              "MouseMoveEvent 379 106 3 0 0 Shift_L\n"
                              "TimerEvent 379 106 3 0 0 Shift_L\n"
                              "RenderEvent 379 106 3 0 0 Shift_L\n"
                              "MouseMoveEvent 379 107 3 0 0 Shift_L\n"
                              "MouseMoveEvent 379 115 3 0 0 Shift_L\n"
                              "TimerEvent 379 115 3 0 0 Shift_L\n"
                              "RenderEvent 379 115 3 0 0 Shift_L\n"
                              "MouseMoveEvent 379 116 3 0 0 Shift_L\n"
                              "TimerEvent 379 116 3 0 0 Shift_L\n"
                              "RenderEvent 379 116 3 0 0 Shift_L\n"
                              "LeftButtonReleaseEvent 379 116 3 0 0 Shift_L\n"
                              "TimerEvent 379 116 3 0 0 Shift_L\n"
                              "RenderEvent 379 116 3 0 0 Shift_L\n"
                              "MouseMoveEvent 380 122 3 0 0 Shift_L\n"
                              "KeyReleaseEvent 380 122 3 0 1 Control_L\n"
                              "MouseMoveEvent 382 129 1 0 0 Control_L\n"
                              "MouseMoveEvent 388 143 1 0 0 Control_L\n"
                              "KeyReleaseEvent 388 143 1 0 1 Shift_L\n"
                              "MouseMoveEvent 390 156 0 0 0 Shift_L\n"
                              "MouseMoveEvent 393 164 0 0 0 Shift_L\n"
                              "MouseMoveEvent 381 317 0 0 0 Shift_L\n"
                              "KeyPressEvent 381 317 0 0 1 Control_L\n"
                              "CharEvent 381 317 0 0 1 Control_L\n"
                              "MouseMoveEvent 380 317 2 0 0 Control_L\n"
                              "MouseMoveEvent 379 317 2 0 0 Control_L\n"
                              "KeyPressEvent 379 317 2 0 1 Shift_L\n"
                              "CharEvent 379 317 2 0 1 Shift_L\n"
                              "LeftButtonPressEvent 379 317 3 0 0 Shift_L\n"
                              "TimerEvent 379 317 3 0 0 Shift_L\n"
                              "RenderEvent 379 317 3 0 0 Shift_L\n"
                              "MouseMoveEvent 379 318 3 0 0 Shift_L\n"
                              "MouseMoveEvent 379 329 3 0 0 Shift_L\n"
                              "TimerEvent 379 329 3 0 0 Shift_L\n"
                              "RenderEvent 379 329 3 0 0 Shift_L\n"
                              "MouseMoveEvent 379 331 3 0 0 Shift_L\n"
                              "MouseMoveEvent 379 341 3 0 0 Shift_L\n"
                              "TimerEvent 379 341 3 0 0 Shift_L\n"
                              "RenderEvent 379 341 3 0 0 Shift_L\n"
                              "MouseMoveEvent 379 343 3 0 0 Shift_L\n"
                              "MouseMoveEvent 379 348 3 0 0 Shift_L\n"
                              "TimerEvent 379 348 3 0 0 Shift_L\n"
                              "RenderEvent 379 348 3 0 0 Shift_L\n"
                              "MouseMoveEvent 379 349 3 0 0 Shift_L\n"
                              "MouseMoveEvent 379 351 3 0 0 Shift_L\n"
                              "TimerEvent 379 351 3 0 0 Shift_L\n"
                              "RenderEvent 379 351 3 0 0 Shift_L\n"
                              "LeftButtonReleaseEvent 379 351 3 0 0 Shift_L\n"
                              "TimerEvent 379 351 3 0 0 Shift_L\n"
                              "RenderEvent 379 351 3 0 0 Shift_L\n"
                              "MouseMoveEvent 379 351 3 0 0 Shift_L\n"
                              "MouseMoveEvent 379 350 3 0 0 Shift_L\n"
                              "MouseMoveEvent 378 207 3 0 0 Shift_L\n"
                              "KeyReleaseEvent 378 207 3 0 1 Control_L\n"
                              "MouseMoveEvent 376 206 1 0 0 Control_L\n"
                              "MouseMoveEvent 373 202 1 0 0 Control_L\n"
                              "MouseMoveEvent 371 200 1 0 0 Control_L\n"
                              "KeyReleaseEvent 371 200 1 0 1 Shift_L\n"
                              "MouseMoveEvent 366 196 0 0 0 Shift_L\n"
                              "MouseMoveEvent 218 99 0 0 0 Shift_L\n"
                              "KeyPressEvent 218 99 0 0 1 Shift_L\n"
                              "CharEvent 218 99 0 0 1 Shift_L\n"
                              "KeyPressEvent 218 99 1 0 1 Control_L\n"
                              "CharEvent 218 99 1 0 1 Control_L\n"
                              "MouseMoveEvent 218 101 3 0 0 Control_L\n"
                              "MouseMoveEvent 220 130 3 0 0 Control_L\n"
                              "LeftButtonPressEvent 220 130 3 0 0 Control_L\n"
                              "TimerEvent 220 130 3 0 0 Control_L\n"
                              "RenderEvent 220 130 3 0 0 Control_L\n"
                              "MouseMoveEvent 220 129 3 0 0 Control_L\n"
                              "MouseMoveEvent 220 126 3 0 0 Control_L\n"
                              "TimerEvent 220 126 3 0 0 Control_L\n"
                              "RenderEvent 220 126 3 0 0 Control_L\n"
                              "MouseMoveEvent 220 124 3 0 0 Control_L\n"
                              "MouseMoveEvent 220 122 3 0 0 Control_L\n"
                              "TimerEvent 220 122 3 0 0 Control_L\n"
                              "RenderEvent 220 122 3 0 0 Control_L\n"
                              "MouseMoveEvent 220 121 3 0 0 Control_L\n"
                              "TimerEvent 220 121 3 0 0 Control_L\n"
                              "RenderEvent 220 121 3 0 0 Control_L\n"
                              "MouseMoveEvent 220 121 3 0 0 Control_L\n"
                              "MouseMoveEvent 220 120 3 0 0 Control_L\n"
                              "TimerEvent 220 120 3 0 0 Control_L\n"
                              "RenderEvent 220 120 3 0 0 Control_L\n"
                              "MouseMoveEvent 220 119 3 0 0 Control_L\n"
                              "MouseMoveEvent 220 117 3 0 0 Control_L\n"
                              "TimerEvent 220 117 3 0 0 Control_L\n"
                              "RenderEvent 220 117 3 0 0 Control_L\n"
                              "MouseMoveEvent 220 116 3 0 0 Control_L\n"
                              "MouseMoveEvent 220 111 3 0 0 Control_L\n"
                              "TimerEvent 220 111 3 0 0 Control_L\n"
                              "RenderEvent 220 111 3 0 0 Control_L\n"
                              "MouseMoveEvent 220 109 3 0 0 Control_L\n"
                              "MouseMoveEvent 220 105 3 0 0 Control_L\n"
                              "TimerEvent 220 105 3 0 0 Control_L\n"
                              "RenderEvent 220 105 3 0 0 Control_L\n"
                              "MouseMoveEvent 220 104 3 0 0 Control_L\n"
                              "MouseMoveEvent 220 103 3 0 0 Control_L\n"
                              "MouseMoveEvent 220 102 3 0 0 Control_L\n"
                              "MouseMoveEvent 220 101 3 0 0 Control_L\n"
                              "TimerEvent 220 101 3 0 0 Control_L\n"
                              "RenderEvent 220 101 3 0 0 Control_L\n"
                              "MouseMoveEvent 220 100 3 0 0 Control_L\n"
                              "MouseMoveEvent 220 99 3 0 0 Control_L\n"
                              "MouseMoveEvent 220 98 3 0 0 Control_L\n"
                              "TimerEvent 220 98 3 0 0 Control_L\n"
                              "RenderEvent 220 98 3 0 0 Control_L\n"
                              "LeftButtonReleaseEvent 220 98 3 0 0 Control_L\n"
                              "TimerEvent 220 98 3 0 0 Control_L\n"
                              "RenderEvent 220 98 3 0 0 Control_L\n"
                              "MouseMoveEvent 222 100 3 0 0 Control_L\n"
                              "MouseMoveEvent 225 102 3 0 0 Control_L\n"
                              "MouseMoveEvent 226 103 3 0 0 Control_L\n"
                              "MouseMoveEvent 229 103 3 0 0 Control_L\n"
                              "MouseMoveEvent 231 104 3 0 0 Control_L\n"
                              "MouseMoveEvent 232 105 3 0 0 Control_L\n"
                              "KeyReleaseEvent 232 105 3 0 1 Control_L\n"
                              "KeyReleaseEvent 232 105 1 0 1 Shift_L\n"
                              "MouseMoveEvent 232 104 0 0 0 Shift_L\n"
                              "MouseMoveEvent 217 84 0 0 0 Shift_L\n"
                              "KeyPressEvent 217 84 0 0 1 Control_L\n"
                              "CharEvent 217 84 0 0 1 Control_L\n"
                              "KeyPressEvent 217 84 2 0 1 Shift_L\n"
                              "CharEvent 217 84 2 0 1 Shift_L\n"
                              "LeftButtonPressEvent 217 84 3 0 0 Shift_L\n"
                              "TimerEvent 217 84 3 0 0 Shift_L\n"
                              "RenderEvent 217 84 3 0 0 Shift_L\n"
                              "MouseMoveEvent 217 85 3 0 0 Shift_L\n"
                              "MouseMoveEvent 217 86 3 0 0 Shift_L\n"
                              "TimerEvent 217 86 3 0 0 Shift_L\n"
                              "RenderEvent 217 86 3 0 0 Shift_L\n"
                              "MouseMoveEvent 217 87 3 0 0 Shift_L\n"
                              "MouseMoveEvent 217 89 3 0 0 Shift_L\n"
                              "MouseMoveEvent 217 90 3 0 0 Shift_L\n"
                              "MouseMoveEvent 217 91 3 0 0 Shift_L\n"
                              "TimerEvent 217 91 3 0 0 Shift_L\n"
                              "RenderEvent 217 91 3 0 0 Shift_L\n"
                              "MouseMoveEvent 217 91 3 0 0 Shift_L\n"
                              "MouseMoveEvent 217 92 3 0 0 Shift_L\n"
                              "TimerEvent 217 92 3 0 0 Shift_L\n"
                              "RenderEvent 217 92 3 0 0 Shift_L\n"
                              "MouseMoveEvent 217 93 3 0 0 Shift_L\n"
                              "MouseMoveEvent 217 94 3 0 0 Shift_L\n"
                              "MouseMoveEvent 217 95 3 0 0 Shift_L\n"
                              "TimerEvent 217 95 3 0 0 Shift_L\n"
                              "RenderEvent 217 95 3 0 0 Shift_L\n"
                              "MouseMoveEvent 217 96 3 0 0 Shift_L\n"
                              "MouseMoveEvent 217 97 3 0 0 Shift_L\n"
                              "MouseMoveEvent 217 98 3 0 0 Shift_L\n"
                              "TimerEvent 217 98 3 0 0 Shift_L\n"
                              "RenderEvent 217 98 3 0 0 Shift_L\n"
                              "MouseMoveEvent 217 98 3 0 0 Shift_L\n"
                              "TimerEvent 217 98 3 0 0 Shift_L\n"
                              "RenderEvent 217 98 3 0 0 Shift_L\n"
                              "MouseMoveEvent 217 98 3 0 0 Shift_L\n"
                              "MouseMoveEvent 217 99 3 0 0 Shift_L\n"
                              "TimerEvent 217 99 3 0 0 Shift_L\n"
                              "RenderEvent 217 99 3 0 0 Shift_L\n"
                              "MouseMoveEvent 217 100 3 0 0 Shift_L\n"
                              "MouseMoveEvent 217 100 3 0 0 Shift_L\n"
                              "TimerEvent 217 100 3 0 0 Shift_L\n"
                              "RenderEvent 217 100 3 0 0 Shift_L\n"
                              "LeftButtonReleaseEvent 217 100 3 0 0 Shift_L\n"
                              "TimerEvent 217 100 3 0 0 Shift_L\n"
                              "RenderEvent 217 100 3 0 0 Shift_L\n"
                              "MouseMoveEvent 218 100 3 0 0 Shift_L\n"
                              "MouseMoveEvent 218 99 3 0 0 Shift_L\n"
                              "MouseMoveEvent 226 91 3 0 0 Shift_L\n"
                              "MouseMoveEvent 226 90 3 0 0 Shift_L\n"
                              "KeyReleaseEvent 226 90 3 0 1 Control_L\n"
                              "KeyReleaseEvent 226 90 1 0 1 Shift_L\n"
                              "MouseMoveEvent 225 91 0 0 0 Shift_L\n"
                              "KeyPressEvent 225 91 0 0 1 Alt_L\n"
                              "CharEvent 225 91 0 0 1 Alt_L\n"
                              "KeyPressEvent 225 91 4 0 1 Control_L\n"
                              "CharEvent 225 91 4 0 1 Control_L\n"
                              "MouseMoveEvent 225 91 6 0 0 Control_L\n"
                              "MouseMoveEvent 224 91 6 0 0 Control_L\n"
                              "MouseMoveEvent 219 79 6 0 0 Control_L\n"
                              "LeftButtonPressEvent 219 79 6 0 0 Control_L\n"
                              "TimerEvent 219 79 6 0 0 Control_L\n"
                              "RenderEvent 219 79 6 0 0 Control_L\n"
                              "MouseMoveEvent 219 80 6 0 0 Control_L\n"
                              "MouseMoveEvent 219 85 6 0 0 Control_L\n"
                              "TimerEvent 219 85 6 0 0 Control_L\n"
                              "RenderEvent 219 85 6 0 0 Control_L\n"
                              "MouseMoveEvent 219 86 6 0 0 Control_L\n"
                              "MouseMoveEvent 219 90 6 0 0 Control_L\n"
                              "MouseMoveEvent 219 92 6 0 0 Control_L\n"
                              "TimerEvent 219 92 6 0 0 Control_L\n"
                              "RenderEvent 219 92 6 0 0 Control_L\n"
                              "MouseMoveEvent 219 93 6 0 0 Control_L\n"
                              "MouseMoveEvent 219 96 6 0 0 Control_L\n"
                              "TimerEvent 219 96 6 0 0 Control_L\n"
                              "RenderEvent 219 96 6 0 0 Control_L\n"
                              "MouseMoveEvent 219 96 6 0 0 Control_L\n"
                              "TimerEvent 219 96 6 0 0 Control_L\n"
                              "RenderEvent 219 96 6 0 0 Control_L\n"
                              "MouseMoveEvent 219 97 6 0 0 Control_L\n"
                              "MouseMoveEvent 219 97 6 0 0 Control_L\n"
                              "MouseMoveEvent 219 98 6 0 0 Control_L\n"
                              "TimerEvent 219 98 6 0 0 Control_L\n"
                              "RenderEvent 219 98 6 0 0 Control_L\n"
                              "MouseMoveEvent 219 99 6 0 0 Control_L\n"
                              "MouseMoveEvent 219 100 6 0 0 Control_L\n"
                              "MouseMoveEvent 219 101 6 0 0 Control_L\n"
                              "MouseMoveEvent 219 102 6 0 0 Control_L\n"
                              "TimerEvent 219 102 6 0 0 Control_L\n"
                              "RenderEvent 219 102 6 0 0 Control_L\n"
                              "MouseMoveEvent 219 103 6 0 0 Control_L\n"
                              "MouseMoveEvent 219 103 6 0 0 Control_L\n"
                              "TimerEvent 219 103 6 0 0 Control_L\n"
                              "RenderEvent 219 103 6 0 0 Control_L\n"
                              "LeftButtonReleaseEvent 219 103 6 0 0 Control_L\n"
                              "TimerEvent 219 103 6 0 0 Control_L\n"
                              "RenderEvent 219 103 6 0 0 Control_L\n"
                              "MouseMoveEvent 221 102 6 0 0 Control_L\n"
                              "MouseMoveEvent 237 97 6 0 0 Control_L\n"
                              "KeyReleaseEvent 237 97 6 0 1 Alt_L\n"
                              "KeyReleaseEvent 237 97 2 0 1 Control_L\n"
                              "MouseMoveEvent 242 103 0 0 0 Control_L\n"
                              "MouseMoveEvent 571 305 0 0 0 Control_L\n"
                              "LeaveEvent 602 305 0 0 0 Control_L\n";

namespace
{
void UpdateSelectionMode(vtkObject* caller, unsigned long vtkNotUsed(eventId), void* clientData,
  void* vtkNotUsed(callData))
{
  const auto iren = static_cast<vtkRenderWindowInteractor*>(caller);
  auto chart = static_cast<vtkChartParallelCoordinates*>(clientData);

  if (iren->GetControlKey() != 0)
  {
    chart->SetSelectionMode(vtkContextScene::SELECTION_ADDITION);
  }

  if (iren->GetShiftKey() != 0)
  {
    chart->SetSelectionMode(vtkContextScene::SELECTION_SUBTRACTION);
  }

  if (iren->GetAltKey() != 0)
  {
    chart->SetSelectionMode(vtkContextScene::SELECTION_TOGGLE);
  }
}
}

int TestParallelCoordinatesSelection(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  // Set up a 2D scene, add an parallel coordinate chart to it
  vtkNew<vtkContextView> view;
  view->GetRenderWindow()->SetWindowName("TestParallelCoordinateSelection");
  view->GetRenderWindow()->SetSize(600, 400);
  view->GetRenderWindow()->SetMultiSamples(0);
  vtkNew<vtkChartParallelCoordinates> chart;
  view->GetScene()->AddItem(chart);

  // Create a table with some points in it...
  vtkNew<vtkTable> table;
  vtkNew<vtkDoubleArray> arrX;
  arrX->SetName("x");
  table->AddColumn(arrX);
  vtkNew<vtkDoubleArray> arrC;
  arrC->SetName("cosine");
  table->AddColumn(arrC);
  vtkNew<vtkDoubleArray> arrS;
  arrS->SetName("sine");
  table->AddColumn(arrS);
  vtkNew<vtkDoubleArray> arrS2;
  arrS2->SetName("tangent");
  table->AddColumn(arrS2);

  int numPoints = 200;
  float inc = 7.5 / (numPoints - 1);
  table->SetNumberOfRows(numPoints);
  for (int i = 0; i < numPoints; ++i)
  {
    table->SetValue(i, 0, i * inc);
    table->SetValue(i, 1, cos(i * inc));
    table->SetValue(i, 2, sin(i * inc));
    table->SetValue(i, 3, tan(i * inc) + 0.5);
  }
  chart->GetPlot(0)->SetInputData(table);

  // Link some key events to switch between each selection mode.
  vtkNew<vtkCallbackCommand> keypressCallback;
  keypressCallback->SetCallback(::UpdateSelectionMode);
  keypressCallback->SetClientData(chart);
  view->GetInteractor()->AddObserver(vtkCommand::KeyPressEvent, keypressCallback);

  // record events
  vtkSmartPointer<vtkInteractorEventRecorder> recorder =
    vtkSmartPointer<vtkInteractorEventRecorder>::New();
  recorder->SetInteractor(view->GetInteractor());
  view->GetInteractor()->Initialize();
  recorder->ReadFromInputStringOn();
  recorder->SetInputString(eventLog);
  recorder->Play();

  if (chart->GetNumberOfPlots() != 1)
  {
    std::cerr << "Wrong number of plot. Expected 1 but got " << chart->GetNumberOfPlots()
              << std::endl;
    return EXIT_FAILURE;
  }

  auto ids = chart->GetPlot(0)->GetSelection();
  if (!ids)
  {
    std::cerr << "Selection shouldn't be null" << std::endl;
    return EXIT_FAILURE;
  }

  if (ids->GetSize() != 2)
  {
    std::cerr << "Wrong number of id selection. Expected to have 2 ids but got " << ids->GetSize()
              << std::endl;
    return EXIT_FAILURE;
  }

  for (int i = 0; i < ids->GetSize(); i++)
  {
    if (ids->GetValue(i) != expectedValues[i])
    {
      std::cerr << "Wrong id values in the current selection. Expected to have "
                << expectedValues[i] << " id but got " << ids->GetValue(i) << std::endl;
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
