// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOpenXRManagerRemoteConnection

 * @brief   OpenXR remoting connection strategy
 *
 * @sa
 * vtkOpenXRManager
 */

#ifndef vtkOpenXRManagerRemoteConnection_h
#define vtkOpenXRManagerRemoteConnection_h

#include "vtkOpenXRManagerConnection.h"
#include "vtkRenderingOpenXRRemotingModule.h" // For export macro

#include "vtkOpenXR.h" // For XrInstance/XrSystemId

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGOPENXRREMOTING_EXPORT vtkOpenXRManagerRemoteConnection
  : public vtkOpenXRManagerConnection
{
public:
  static vtkOpenXRManagerRemoteConnection* New();
  vtkTypeMacro(vtkOpenXRManagerRemoteConnection, vtkOpenXRManagerConnection);

  bool Initialize() override;
  bool EndInitialize() override;
  bool ConnectToRemote(XrInstance instance, XrSystemId id) override;

  ///@{
  /**
   * Set/Get a custom path to look for the RemotingXR.json file provided by the
   * microsoft.holographic.remoting.openxr package.
   * This file sets the XR_RUNTIME_JSON environment variable used by the OpenXR loader to not use
   * the system default OpenXR runtime but instead redirect to the Holographic Remoting runtime.
   * Default is empty. Calling this method after initialization has no effect.
   */
  vtkGetMacro(RemotingXRDirectory, std::string);
  vtkSetMacro(RemotingXRDirectory, std::string);
  ///@}

  /**
   * Enable the OpenXR Remoting extension if supported.
   */
  const char* GetExtensionName() override;

  /**
   * Handle connection/deconnection events
   */
  bool HandleXrEvent(const XrEventDataBuffer& eventData) override;

protected:
  vtkOpenXRManagerRemoteConnection() = default;
  ~vtkOpenXRManagerRemoteConnection() = default;

private:
  vtkOpenXRManagerRemoteConnection(const vtkOpenXRManagerRemoteConnection&) = delete;
  void operator=(const vtkOpenXRManagerRemoteConnection&) = delete;

  std::string OldXrRuntimeEnvValue;
  std::string RemotingXRDirectory;
};

VTK_ABI_NAMESPACE_END
#endif
