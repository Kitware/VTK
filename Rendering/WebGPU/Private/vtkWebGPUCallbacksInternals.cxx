// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "Private/vtkWebGPUCallbacksInternals.h"
#include "vtkLogger.h"
#include "vtkObject.h"

#include <sstream>

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
void vtkWebGPUCallbacksInternals::DeviceLostCallback(const WGPUDevice* vtkNotUsed(device),
  WGPUDeviceLostReason reason, char const* message, void* userdata)
{
  std::string reasonStr;
  switch (reason)
  {
    case WGPUDeviceLostReason_Destroyed:
      reasonStr = "Destroyed";
      break;
    case WGPUDeviceLostReason_InstanceDropped:
      reasonStr = "InstanceDropped";
      break;
    case WGPUDeviceLostReason_FailedCreation:
      reasonStr = "FailedCreation";
      break;
    default:
      reasonStr = "Unknown";
  }

  if (userdata)
  {
    vtkWarningWithObjectMacro(reinterpret_cast<vtkObject*>(userdata),
      << "WebGPU device lost: \"" << message << "\" with reason \"" << reasonStr << "\"");
  }
  else
  {
    vtkLogF(WARNING, "WebGPU device lost: \"%s\" with reason \"%s\"", message, reasonStr.c_str());
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUCallbacksInternals::UncapturedErrorCallback(
  WGPUErrorType type, char const* message, void* userdata)
{
  vtkWebGPUCallbacksInternals::PrintWGPUError(type, message, userdata);
}

//------------------------------------------------------------------------------
void vtkWebGPUCallbacksInternals::PrintWGPUError(
  WGPUErrorType type, const char* message, void* userdata)
{
  std::string typeStr;
  switch (type)
  {
    case WGPUErrorType_Validation:
      typeStr = "Validation";
      break;

    case WGPUErrorType_OutOfMemory:
      typeStr = "Out of memory";
      break;

    case WGPUErrorType_Unknown:
      typeStr = "Unknown";
      break;

    case WGPUErrorType_DeviceLost:
      typeStr = "Device lost";
      break;

    default:
      typeStr = "Unknown";
  }

  std::stringstream logString;
  logString << "Uncaptured device error: type " << typeStr;

  if (message)
  {
    logString << " with message: \"" << message << "\"";
  }

  if (userdata)
  {
    vtkErrorWithObjectMacro(reinterpret_cast<vtkObject*>(userdata), << logString.str());
  }
  else
  {
    vtkLogF(ERROR, "%s", logString.str().c_str());
  }
}

VTK_ABI_NAMESPACE_END
