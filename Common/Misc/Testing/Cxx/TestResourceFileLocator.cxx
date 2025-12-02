// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkResourceFileLocator.h"
#include "vtkVersion.h"

#include <vtksys/SystemTools.hxx>

#include <iostream>

int TestResourceFileLocator(int, char*[])
{
  auto vtklib = vtkGetLibraryPathForSymbol(GetVTKVersion);
  if (vtklib.empty())
  {
    std::cerr << "FAILED to locate `GetVTKVersion`." << std::endl;
    return EXIT_FAILURE;
  }
  const std::string vtkdir = vtksys::SystemTools::GetFilenamePath(vtklib);

  vtkNew<vtkResourceFileLocator> locator;
  locator->SetLogVerbosity(vtkLogger::VERBOSITY_INFO);
  auto path = locator->Locate(vtkdir, "Testing/Temporary");
  if (path.empty())
  {
    std::cerr << "FAILED to locate 'Testing/Temporary' dir." << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
