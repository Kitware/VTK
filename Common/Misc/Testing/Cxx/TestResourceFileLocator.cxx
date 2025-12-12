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

  vtkNew<vtkResourceFileLocator> locator;
  locator->SetLogVerbosity(vtkLogger::VERBOSITY_INFO);

  // ------------------------------------------------------------
  // Test: new unified API
  // ------------------------------------------------------------
  auto pathFromAddress =
    vtkResourceFileLocator::GetLibraryPathForAddress(reinterpret_cast<const void*>(&GetVTKVersion));

  if (pathFromAddress.empty())
  {
    std::cerr << "FAILED to locate 'Testing/Temporary' dir." << std::endl;
    return EXIT_FAILURE;
  }

  std::string libDir = vtksys::SystemTools::GetFilenamePath(pathFromAddress);
  if (!vtksys::SystemTools::FileIsDirectory(libDir))
  {
    std::cerr << "FAILED: Library directory from GetLibraryPathForAddress() does not exist: "
              << libDir << "\n";
    return EXIT_FAILURE;
  }

  // ------------------------------------------------------------
  // Test: legacy macro API (kept for compatibility)
  // ------------------------------------------------------------
  auto pathFromMacro = vtkGetLibraryPathForSymbol(GetVTKVersion);
  if (pathFromMacro.empty())
  {
    std::cerr << "FAILED: vtkGetLibraryPathForSymbol() macro returned empty path.\n";
    return EXIT_FAILURE;
  }

  // ------------------------------------------------------------
  // Test: Locate(anchor, landmark)
  // Look for directory that should exist in build tree
  // ------------------------------------------------------------
  auto located1 = locator->Locate(libDir, "Testing/Temporary");
  if (located1.empty())
  {
    std::cerr << "FAILED: Locate() did not find Testing/Temporary relative to library dir.\n";
    return EXIT_FAILURE;
  }

  // verify the directory is real
  if (!vtksys::SystemTools::FileIsDirectory(located1))
  {
    std::cerr << "FAILED: Located path is not a directory: " << located1 << "\n";
    return EXIT_FAILURE;
  }

  // ------------------------------------------------------------
  // Test Locate(anchor, prefixes, landmark)
  // ------------------------------------------------------------
  std::vector<std::string> prefixes = { "Testing", "Testing/Temporary" };
  auto located2 = locator->Locate(libDir, prefixes, "Temporary");
  if (located2.empty())
  {
    std::cerr << "FAILED: Locate(prefixes) did not find expected directory.\n";
    return EXIT_FAILURE;
  }

  // ------------------------------------------------------------
  // Test defaultDir fallback
  // ------------------------------------------------------------
  auto fallback = locator->Locate("/nonexistent/path", "nothing", "/fallback/dir");
  if (fallback != "/fallback/dir")
  {
    std::cerr << "FAILED: defaultDir fallback did not activate.\n";
    return EXIT_FAILURE;
  }

  // ------------------------------------------------------------
  // Test GetCurrentExecutablePath()
  // ------------------------------------------------------------
  auto exePath = vtkResourceFileLocator::GetCurrentExecutablePath();
  if (exePath.empty())
  {
    std::cerr << "FAILED: GetCurrentExecutablePath() returned empty path.\n";
    return EXIT_FAILURE;
  }

  std::string exeDir = vtksys::SystemTools::GetFilenamePath(exePath);
  if (!vtksys::SystemTools::FileIsDirectory(exeDir))
  {
    std::cerr << "FAILED: Executable directory does not exist: " << exeDir << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
