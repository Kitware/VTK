// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// VTK_DEPRECATED_IN_9_4_0()
#define VTK_DEPRECATION_LEVEL 0

#include "vtkGLTFReader.h"

int TestGLTFReaderGeometryNoBin(int argc, char* argv[])
{

  if (argc <= 1)
  {
    std::cout << "Usage: " << argv[0] << " <gltf file>" << std::endl;
    return EXIT_FAILURE;
  }

  vtkNew<vtkGLTFReader> reader;
  reader->SetFileName(argv[1]);

  // This test expects an error message, but we can't catch it via an error handler
  // because it's emitted by an internal object, so let's just deactivate the error logging.
  auto previousWarningLevel = vtkObject::GetGlobalWarningDisplay();
  vtkObject::SetGlobalWarningDisplay(0);

  reader->Update();

  vtkObject::SetGlobalWarningDisplay(previousWarningLevel);

  return EXIT_SUCCESS;
}
