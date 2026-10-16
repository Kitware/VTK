// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// A browser canvas belongs to the host page; every other hardware window is VTK's own. The
// off-screen only path keys off that distinction, so both the predicate and the surface decision
// it drives are checked here.

#include "vtkHardwareWindow.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkRenderWindow.h"
#include "vtkWebGPURenderWindow.h"

#include <cstdlib>

int TestHardwareWindowOwnership(int, char*[])
{
  vtkNew<vtkHardwareWindow> hardwareWindow;
  const bool externallyOwned = hardwareWindow->IsA("vtkWebAssemblyHardwareWindow") != 0;
  if (hardwareWindow->IsExternallyOwned() != externallyOwned)
  {
    vtkLog(ERROR, << hardwareWindow->GetClassName() << "::IsExternallyOwned() returned "
                  << hardwareWindow->IsExternallyOwned() << ", expected " << externallyOwned);
    return EXIT_FAILURE;
  }

  vtkNew<vtkRenderWindow> renWin;
  if (vtkWebGPURenderWindow::SafeDownCast(renWin) == nullptr)
  {
    vtkLog(ERROR, << "This test needs the WebGPU rendering backend.");
    return EXIT_FAILURE;
  }
  renWin->SetWindowName(__func__);
  renWin->SetMultiSamples(0);
  renWin->SetSize(100, 100);
  renWin->SetShowWindow(false);
  renWin->SetUseOffScreenBuffers(true);
  renWin->Render();

  // An externally owned offscreen render window, such as one used in vtk-wasm, must have a
  // vtkHardwareWindow. A desktop offscreen render window that is not externally owned should not
  // have one.
  if ((renWin->GetHardwareWindow() != nullptr) != externallyOwned)
  {
    vtkLog(ERROR, << "Off-screen only render window " << (externallyOwned ? "has no" : "has a")
                  << " hardware window.");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
