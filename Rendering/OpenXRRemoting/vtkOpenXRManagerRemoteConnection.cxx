/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenXRManagerRemoteConnection.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenXRManagerRemoteConnection.h"

#include "vtkObjectFactory.h"
#include "vtkOpenXRManager.h"
#include "vtkWindows.h" // For Win32 API used in Initialize

#include <openxr_msft_holographic_remoting.h> // Defines XR_MSFT_holographic_remoting

#include "XrConnectionExtensions.h" // Provides holographic remoting extensions

#include <thread> // used to sleep after connection

vtkStandardNewMacro(vtkOpenXRManagerRemoteConnection);

//------------------------------------------------------------------------------
bool vtkOpenXRManagerRemoteConnection::Initialize()
{
  std::wstring filename;
  filename.resize(MAX_PATH);
  if (!SearchPathW(nullptr, L"RemotingXR", L".json", MAX_PATH, &filename[0], nullptr))
  {
    vtkErrorMacro("Could not find RemotingXR.json.");
    return false;
  }

  if (GetFileAttributesW(filename.data()) != INVALID_FILE_ATTRIBUTES)
  {
    // This environment variable is used by the OpenXR loader to not use the system default OpenXR
    // runtime but instead redirect to the Holographic Remoting OpenXR runtime
    SetEnvironmentVariableW(L"XR_RUNTIME_JSON", filename.data());
    return true;
  }

  return false;
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

  if (!vtkOpenXRManager::GetInstance().XrCheckError(
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
