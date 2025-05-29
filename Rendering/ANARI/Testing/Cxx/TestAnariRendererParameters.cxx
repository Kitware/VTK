// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// This test verifies that lighting works as expected with ANARI.

// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**Description
 * This test checks the anari renderer parameter introspection code.
 */

#include "vtkAnariPass.h"
#include "vtkAnariRenderer.h"
#include "vtkLogger.h"
#include "vtkTesting.h"
#include <cstdlib>

int TestAnariRendererParameters(int argc, char* argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;
  vtkLogger::SetStderrVerbosity(vtkLogger::Verbosity::VERBOSITY_WARNING);
  bool useDebugDevice = false;

  for (int i = 0; i < argc; i++)
  {
    if (!strcmp(argv[i], "-trace"))
    {
      useDebugDevice = true;
      vtkLogger::SetStderrVerbosity(vtkLogger::Verbosity::VERBOSITY_INFO);
    }
  }
  vtkNew<vtkAnariPass> anariPass;
  auto* adev = anariPass->GetAnariDevice();
  if (useDebugDevice)
  {
    vtkNew<vtkTesting> testing;
    std::string traceDir = testing->GetTempDirectory();
    traceDir += "/anari-trace/";
    traceDir += "TestAnariRendererParameters";
    adev->SetAnariDebugConfig(traceDir.c_str(), "code");
  }
  auto* aren = anariPass->GetAnariRenderer();

  adev->SetupAnariDeviceFromLibrary("environment", "default", useDebugDevice);

  auto renParams = aren->GetRendererParameters();

  std::cout << "Number of parameters " << renParams.size() << std::endl;
  for (auto iter = renParams.cbegin(); iter != renParams.cend(); ++iter)
  {
    std::cout << "Parameter: " << iter->first << "\n\tType: " << iter->second
              << "\n\tDescription: " << aren->GetRendererParameterDescription((*iter))
              << "\n\tRequired: " << aren->IsRendererParameterRequired((*iter))
              << "\n\tDefault: " << aren->GetRendererParameterDefault((*iter))
              << "\n\tMinimum: " << aren->GetRendererParameterMinimum((*iter))
              << "\n\tMaximum: " << aren->GetRendererParameterMaximum((*iter))
              << "\n\tValue: " << aren->GetRendererParameterValue((*iter)) << std::endl;
  }

  int retVal = EXIT_SUCCESS;
  if (renParams.size() < 1)
  {
    retVal = EXIT_FAILURE;
  }
  return retVal;
}
