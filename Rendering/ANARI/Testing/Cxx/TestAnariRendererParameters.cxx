// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**Description
 * This test checks the anari renderer parameter introspection code.
 */

#include "vtkAnariPass.h"
#include "vtkAnariRenderer.h"
#include "vtkLogger.h"
#include "vtkTesting.h"
#include <anari/frontend/anari_enums.h>

#include <iostream>

int TestAnariRendererParameters(int argc, char* argv[])
{
  std::cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << std::endl;
  vtkLogger::SetStderrVerbosity(vtkLogger::Verbosity::VERBOSITY_INFO);
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

  // Ensure that we use the helide implementation for this test.
  adev->SetupAnariDeviceFromLibrary("helide", "default", useDebugDevice);

  // Uncomment the following line to test a different implementation provided by ANARI_LIBRARY
  // environment variable.
  // adev->SetupAnariDeviceFromLibrary("environment", "default", useDebugDevice);

  auto renParams = aren->GetRendererParameters();

  int retVal = EXIT_SUCCESS;
  if (renParams.empty())
  {
    vtkLogF(ERROR, "No renderer parameters found. Is the Anari device set up correctly?");
    return EXIT_FAILURE;
  }
  vtkLogF(INFO, "Found %zu renderer parameters.", renParams.size());
  for (auto iter = renParams.cbegin(); iter != renParams.cend(); ++iter)
  {
    vtkLogF(INFO, "----------------------------------------");
    vtkLogF(INFO, "Parameter: %s", iter->first.c_str());
    vtkLogF(INFO, "\tType: %d", iter->second);
    vtkLogF(INFO, "\tDescription: %s", aren->GetRendererParameterDescription((*iter)).c_str());
    vtkLogF(INFO, "\tRequired: %s", aren->IsRendererParameterRequired((*iter)) ? "true" : "false");
    switch (iter->second)
    {
      case ANARI_BOOL:
        vtkLogF(INFO, "\tDefault: %s",
          (*(static_cast<const int*>(aren->GetRendererParameterDefault((*iter))))) ? "true"
                                                                                   : "false");
        vtkLogF(INFO, "\tValue: %s",
          (*(static_cast<const int*>(aren->GetRendererParameterValue((*iter))))) ? "true"
                                                                                 : "false");
        break;
      case ANARI_INT16:
        vtkLogF(INFO, "\tDefault: %d",
          *(static_cast<const int*>(aren->GetRendererParameterDefault((*iter)))));
        vtkLogF(INFO, "\tValue: %d",
          *(static_cast<const int*>(aren->GetRendererParameterValue((*iter)))));
        vtkLogF(INFO, "\tMinimum: %d",
          *(static_cast<const int*>(aren->GetRendererParameterMinimum((*iter)))));
        vtkLogF(INFO, "\tMaximum: %d",
          *(static_cast<const int*>(aren->GetRendererParameterMaximum((*iter)))));
        break;
      case ANARI_FLOAT16:
        vtkLogF(INFO, "\tDefault: %f",
          *(static_cast<const float*>(aren->GetRendererParameterDefault((*iter)))));
        vtkLogF(INFO, "\tValue: %f",
          *(static_cast<const float*>(aren->GetRendererParameterValue((*iter)))));
        vtkLogF(INFO, "\tMinimum: %f",
          *(static_cast<const float*>(aren->GetRendererParameterMinimum((*iter)))));
        vtkLogF(INFO, "\tMaximum: %f",
          *(static_cast<const float*>(aren->GetRendererParameterMaximum((*iter)))));
        break;
      case ANARI_STRING:
        vtkLogF(INFO, "\tDefault: %s",
          static_cast<const char*>(aren->GetRendererParameterDefault((*iter))));
        vtkLogF(
          INFO, "\tValue: %s", static_cast<const char*>(aren->GetRendererParameterValue((*iter))));
        break;
      default:
        vtkLogF(INFO, "\tNot printing default/value/minimum/maximum for this type.");
        break;
    }
  }

  return retVal;
}
