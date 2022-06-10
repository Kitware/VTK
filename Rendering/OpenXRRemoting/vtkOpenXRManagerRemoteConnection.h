/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenXRManagerRemoteConnection.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
  bool ConnectToRemote(XrInstance instance, XrSystemId id) override;

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
};

VTK_ABI_NAMESPACE_END
#endif
