// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkOpenXRManagerRemoteConnection.h"

#include "vtkObjectFactory.h"
#include "vtkOpenXRManager.h"
#include "vtkResourceFileLocator.h"
#include "vtksys/SystemTools.hxx"

// Starting with Microsoft.Holographic.Remoting.OpenXr 2.9.3, the struct
// `XrRemotingPreferredGraphicsAdapterMSFT` is defined in `openxr_msft_holographic_remoting.h`
// and it requires the type LUID to be defined.
// Since LUID is defined in `windows.h` (see `OpenXR-SDK-Source/specification/registry/xr.xml`),
// we include the corresponding VTK header.
#include <vtkWindows.h> // Defines LUID used in XrRemotingPreferredGraphicsAdapterMSFT

#include <openxr_msft_holographic_remoting.h> // Defines XR_MSFT_holographic_remoting

#include "XrConnectionExtensions.h" // Provides holographic remoting extensions

#include <thread> // used to sleep after connection

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkOpenXRManagerRemoteConnection);

//------------------------------------------------------------------------------
bool vtkOpenXRManagerRemoteConnection::Initialize()
{
  // Get the path for the library
  std::string libPath =
    vtkResourceFileLocator::GetLibraryPathForSymbolWin32("vtkOpenXRManagerRemoteConnection");
  std::string libDir = vtksys::SystemTools::GetFilenamePath(libPath);

  // Get the path for the current executable
  std::string exePath = vtkResourceFileLocator::GetLibraryPathForSymbolWin32(nullptr);
  std::string exeDir = vtksys::SystemTools::GetFilenamePath(exePath);

  // Look for the RemotingXR.json file provided by the microsoft.holographic.remoting.openxr
  // package, in the system PATH, next to the vtkRenderingOpenXR library and also next to the
  // executable.
  // If found, set the XR_RUNTIME_JSON environment variable. It will be used by the OpenXR loader
  // to not use the system default OpenXR runtime but instead redirect to the Holographic Remoting
  // OpenXR runtime.
  std::string remotingXRPath =
    vtksys::SystemTools::FindFile("RemotingXR.json", { libDir, exeDir, this->RemotingXRDirectory });
  if (remotingXRPath.empty())
  {
    return false;
  }

  std::string oldValue;
  if (vtksys::SystemTools::GetEnv("XR_RUNTIME_JSON", oldValue))
  {
    this->OldXrRuntimeEnvValue = std::move(oldValue);
  }

  if (!vtksys::SystemTools::PutEnv("XR_RUNTIME_JSON=" + remotingXRPath))
  {
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkOpenXRManagerRemoteConnection::EndInitialize()
{
  if (!this->OldXrRuntimeEnvValue.empty())
  {
    if (!vtksys::SystemTools::PutEnv("XR_RUNTIME_JSON=" + this->OldXrRuntimeEnvValue))
    {
      return false;
    }
  }
  else if (!vtksys::SystemTools::UnPutEnv("XR_RUNTIME_JSON"))
  {
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkOpenXRManagerRemoteConnection::ConnectToRemote(XrInstance instance, XrSystemId id)
{
  if (this->IPAddress.empty())
  {
    vtkErrorMacro("Remoting IP address unspecified.");
    return false;
  }

  XrRemotingConnectionStateMSFT connectionState;
  xr::ConnectionExtensionDispatchTable extensions;
  extensions.PopulateDispatchTable(instance);
  extensions.xrRemotingGetConnectionStateMSFT(instance, id, &connectionState, nullptr);
  if (connectionState != XR_REMOTING_CONNECTION_STATE_DISCONNECTED_MSFT)
  {
    vtkErrorMacro("Error connecting to " << this->IPAddress << ": " << connectionState);
    return false;
  }

  // Apply remote context properties while disconnected.
  {
    XrRemotingRemoteContextPropertiesMSFT contextProperties;
    contextProperties = XrRemotingRemoteContextPropertiesMSFT{ static_cast<XrStructureType>(
      XR_TYPE_REMOTING_REMOTE_CONTEXT_PROPERTIES_MSFT) };
    contextProperties.enableAudio = false;
    contextProperties.maxBitrateKbps = 20000;
    contextProperties.videoCodec = XR_REMOTING_VIDEO_CODEC_ANY_MSFT;
    contextProperties.depthBufferStreamResolution =
      XR_REMOTING_DEPTH_BUFFER_STREAM_RESOLUTION_HALF_MSFT;

    extensions.xrRemotingSetContextPropertiesMSFT(instance, id, &contextProperties);
  }

  XrRemotingConnectInfoMSFT connectInfo{ static_cast<XrStructureType>(
    XR_TYPE_REMOTING_CONNECT_INFO_MSFT) };
  connectInfo.remoteHostName = this->IPAddress.c_str();
  connectInfo.remotePort = 8265;
  connectInfo.secureConnection = false;

  if (!vtkOpenXRManager::GetInstance().XrCheckOutput(vtkOpenXRManager::ErrorOutput,
        extensions.xrRemotingConnectMSFT(instance, id, &connectInfo), "Failed to connect"))
  {
    return false;
  }

  // Make sure the connection is established before the event loop gets started
  using namespace std::chrono_literals;
  std::this_thread::sleep_for(2500ms);

  return true;
}

//------------------------------------------------------------------------------
const char* vtkOpenXRManagerRemoteConnection::GetExtensionName()
{
  return XR_MSFT_HOLOGRAPHIC_REMOTING_EXTENSION_NAME;
}

//------------------------------------------------------------------------------
bool vtkOpenXRManagerRemoteConnection::HandleXrEvent(const XrEventDataBuffer& eventData)
{
  switch ((XrRemotingStructureType)eventData.type)
  {
    case XR_TYPE_REMOTING_EVENT_DATA_CONNECTED_MSFT:
    {
      vtkDebugMacro("Holographic Remoting: Connected.");
      return true;
    }
    case XR_TYPE_REMOTING_EVENT_DATA_DISCONNECTED_MSFT:
    {
      vtkDebugMacro("Holographic Remoting: Disconnected.");
      return true;
    }
    default:
    {
      break;
    }
  }

  return false;
}
VTK_ABI_NAMESPACE_END
