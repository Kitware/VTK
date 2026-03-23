// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**Description
 * This test checks the anari renderer parameter introspection code.
 */

#include "vtkAnariPass.h"
#include "vtkAnariRenderer.h"
#include "vtkLogger.h"
#include "vtkStringFormatter.h"
#include "vtkTesting.h"

#include <anari/frontend/anari_enums.h>

#include <iostream>

namespace
{

/**
 * Use this function to print a parameter that may be non existent (aka parameterValue == nullptr).
 */
void PrintOptionalParameterInfo(
  const std::string& parameterName, int parameterType, const void* parameterValue)
{
  std::string parameterValueStr = "non existent";
  if (parameterValue)
  {
    switch (parameterType)
    {
      case ANARI_BOOL:
        parameterValueStr = *(static_cast<const int*>(parameterValue)) ? "true" : "false";
        break;
      case ANARI_INT16:
        parameterValueStr = vtk::to_string(*static_cast<const int*>(parameterValue));
        break;
      case ANARI_FLOAT16:
        parameterValueStr = vtk::to_string(*static_cast<const float*>(parameterValue));
        break;
      case ANARI_STRING:
        parameterValueStr = *static_cast<const char*>(parameterValue);
        break;
    }
  }

  vtkLogF(INFO, "\t%s: %s", parameterName.c_str(), parameterValueStr.c_str());
}

}

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

  adev->SetupAnariDeviceFromLibrary("environment", "default", useDebugDevice);

  auto* aren = anariPass->GetAnariRenderer();
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
    int parameterType = iter->second;

    vtkLogF(INFO, "----------------------------------------");
    vtkLogF(INFO, "Parameter: %s", iter->first.c_str());
    vtkLogF(INFO, "\tType: %d", iter->second);
    std::string parameterDescription = aren->GetRendererParameterDescription(*iter);
    ::PrintOptionalParameterInfo("Description", parameterType,
      parameterDescription.empty() ? nullptr : parameterDescription.c_str());
    vtkLogF(INFO, "\tRequired: %s", aren->IsRendererParameterRequired((*iter)) ? "true" : "false");

    ::PrintOptionalParameterInfo("Default", parameterType, aren->GetRendererParameterValue(*iter));
    ::PrintOptionalParameterInfo("Value", parameterType, aren->GetRendererParameterValue(*iter));
    if (parameterType == ANARI_INT16 || parameterType == ANARI_FLOAT16)
    {
      ::PrintOptionalParameterInfo(
        "Minimum", parameterType, aren->GetRendererParameterMinimum(*iter));
      ::PrintOptionalParameterInfo(
        "Maximum", parameterType, aren->GetRendererParameterMaximum(*iter));
    }
  }

  return retVal;
}
