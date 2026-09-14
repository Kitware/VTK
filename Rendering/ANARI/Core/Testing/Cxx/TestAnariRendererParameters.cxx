// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkLogger.h"
#include "vtkStringFormatter.h"

#include "vtkAnariRenderWindow.h"
#include "vtkAnariRenderer.h"
#include "vtkAnariTestUtilities.h"

#include <anari/frontend/anari_enums.h>

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

/**
 * This test checks the anari renderer parameter introspection code.
 */
int TestAnariRendererParameters(int argc, char* argv[])
{
  bool useDebugDevice = false;

  for (int i = 0; i < argc; i++)
  {
    if (!strcmp(argv[i], "--trace"))
    {
      useDebugDevice = true;
    }
  }

  vtkNew<vtkRenderWindow> renderWindow;
  vtkAnariTestUtilities::SetParameterDefaults(
    renderWindow, useDebugDevice, "TestAnariRendererParameters");
  // Calling render to initialize the ANARI device.
  renderWindow->Render();

  vtkAnariRenderer* anariRenderer =
    vtkAnariRenderWindow::SafeDownCast(renderWindow)->GetAnariRenderer();
  auto renParams = anariRenderer->GetRendererParameters();

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
    std::string parameterDescription = anariRenderer->GetRendererParameterDescription(*iter);
    ::PrintOptionalParameterInfo("Description", parameterType,
      parameterDescription.empty() ? nullptr : parameterDescription.c_str());
    vtkLogF(INFO, "\tRequired: %s",
      anariRenderer->IsRendererParameterRequired((*iter)) ? "true" : "false");

    ::PrintOptionalParameterInfo(
      "Default", parameterType, anariRenderer->GetRendererParameterValue(*iter));
    ::PrintOptionalParameterInfo(
      "Value", parameterType, anariRenderer->GetRendererParameterValue(*iter));
    if (parameterType == ANARI_INT16 || parameterType == ANARI_FLOAT16)
    {
      ::PrintOptionalParameterInfo(
        "Minimum", parameterType, anariRenderer->GetRendererParameterMinimum(*iter));
      ::PrintOptionalParameterInfo(
        "Maximum", parameterType, anariRenderer->GetRendererParameterMaximum(*iter));
    }
  }

  return retVal;
}
