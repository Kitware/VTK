// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Tests QVTKWebGPUWidget HiDPI and custom device pixel ratio support.

#include "QVTKWebGPUWidget.h"

#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkWebGPURenderWindow.h"
#include "vtkWebGPURenderer.h"

#include <QApplication>
#include <QEventLoop>
#include <QTimer>

#include <cmath>

namespace
{
void ProcessEventsAndWait(int msec)
{
  QEventLoop loop;
  QTimer::singleShot(msec, &loop, SLOT(quit()));
  loop.exec();
}
}

int TestQVTKWebGPUWidgetHiDPI(int argc, char* argv[])
{
  QApplication app(argc, argv);

  // Create the QVTKWebGPUWidget
  QVTKWebGPUWidget widget;
  widget.setWindowTitle("TestQVTKWebGPUWidgetHiDPI");

  vtkWebGPURenderWindow* renWin = widget.renderWindow();
  if (!renWin)
  {
    vtkLog(ERROR, "Failed to get render window from QVTKWebGPUWidget");
    return EXIT_FAILURE;
  }

  vtkNew<vtkWebGPURenderer> ren;
  ren->SetBackground(0.1, 0.2, 0.4);
  renWin->AddRenderer(ren);

  const int widgetWidth = 200;
  const int widgetHeight = 200;

  widget.resize(widgetWidth, widgetHeight);
  widget.show();
  ProcessEventsAndWait(500);

  // Test 1: Verify default HiDPI is enabled
  if (!widget.enableHiDPI())
  {
    vtkLog(ERROR, "HiDPI should be enabled by default");
    return EXIT_FAILURE;
  }

  // Test 2: Verify default unscaled DPI
  if (widget.unscaledDPI() != 72)
  {
    vtkLog(ERROR, "Expected default unscaledDPI to be 72, got " << widget.unscaledDPI());
    return EXIT_FAILURE;
  }

  // Test 3: Verify default custom device pixel ratio (0 means use Qt's value)
  if (widget.customDevicePixelRatio() != 0.0)
  {
    vtkLog(ERROR,
      "Expected default customDevicePixelRatio to be 0.0, got " << widget.customDevicePixelRatio());
    return EXIT_FAILURE;
  }

  // Test 4: Verify effectiveDevicePixelRatio uses Qt's ratio when custom is 0
  double qtDpr = widget.devicePixelRatioF();
  double effectiveDpr = widget.effectiveDevicePixelRatio();
  if (std::fabs(effectiveDpr - qtDpr) > 1e-6)
  {
    vtkLog(ERROR,
      "effectiveDevicePixelRatio (" << effectiveDpr << ") should match Qt's devicePixelRatioF ("
                                    << qtDpr << ") when customDevicePixelRatio is 0");
    return EXIT_FAILURE;
  }

  // Test 5: Set custom device pixel ratio and verify it is used
  const double customDpr = 2.0;
  widget.setCustomDevicePixelRatio(customDpr);
  ProcessEventsAndWait(100);

  if (std::fabs(widget.effectiveDevicePixelRatio() - customDpr) > 1e-6)
  {
    vtkLog(ERROR,
      "effectiveDevicePixelRatio should be " << customDpr << " after setting custom DPR, got "
                                             << widget.effectiveDevicePixelRatio());
    return EXIT_FAILURE;
  }

  // Verify render window size is scaled by the custom DPR
  const int* windowSize = renWin->GetSize();
  const int expectedWidth = static_cast<int>(widgetWidth * customDpr);
  const int expectedHeight = static_cast<int>(widgetHeight * customDpr);
  if (windowSize[0] != expectedWidth || windowSize[1] != expectedHeight)
  {
    vtkLog(ERROR,
      "Expected render window size " << expectedWidth << "x" << expectedHeight
                                     << " with custom DPR, got " << windowSize[0] << "x"
                                     << windowSize[1]);
    return EXIT_FAILURE;
  }

  // Test 6: Disable HiDPI and verify DPI is unscaled
  widget.setEnableHiDPI(false);
  ProcessEventsAndWait(100);

  if (widget.enableHiDPI())
  {
    vtkLog(ERROR, "HiDPI should be disabled after setEnableHiDPI(false)");
    return EXIT_FAILURE;
  }

  int expectedDpi = widget.unscaledDPI();
  int actualDpi = renWin->GetDPI();
  if (actualDpi != expectedDpi)
  {
    vtkLog(ERROR, "When HiDPI is disabled, DPI should be " << expectedDpi << ", got " << actualDpi);
    return EXIT_FAILURE;
  }

  // Test 7: Re-enable HiDPI and verify DPI is scaled
  widget.setEnableHiDPI(true);
  ProcessEventsAndWait(100);

  int scaledDpi = static_cast<int>(widget.unscaledDPI() * widget.effectiveDevicePixelRatio());
  actualDpi = renWin->GetDPI();
  if (actualDpi != scaledDpi)
  {
    vtkLog(ERROR,
      "When HiDPI is enabled with custom DPR, DPI should be " << scaledDpi << ", got "
                                                              << actualDpi);
    return EXIT_FAILURE;
  }

  // Test 8: Reset custom DPR to 0 and verify fallback to Qt's ratio
  widget.setCustomDevicePixelRatio(0.0);
  ProcessEventsAndWait(100);

  if (std::fabs(widget.effectiveDevicePixelRatio() - qtDpr) > 1e-6)
  {
    vtkLog(
      ERROR, "effectiveDevicePixelRatio should fall back to Qt's ratio after resetting custom DPR");
    return EXIT_FAILURE;
  }

  // Test 9: Set custom unscaled DPI
  const int customDpi = 96;
  widget.setUnscaledDPI(customDpi);
  ProcessEventsAndWait(100);

  if (widget.unscaledDPI() != customDpi)
  {
    vtkLog(ERROR, "Expected unscaledDPI to be " << customDpi << ", got " << widget.unscaledDPI());
    return EXIT_FAILURE;
  }

  vtkLog(INFO, "All HiDPI tests passed.");
  return EXIT_SUCCESS;
}
