// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOpenXRRemotingRenderWindow
 * @brief   OpenXR remoting rendering window
 *
 *
 */

#ifndef vtkOpenXRRemotingRenderWindow_h
#define vtkOpenXRRemotingRenderWindow_h

#include "vtkOpenXRRenderWindow.h"
#include "vtkRenderingOpenXRRemotingModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGOPENXRREMOTING_EXPORT vtkOpenXRRemotingRenderWindow : public vtkOpenXRRenderWindow
{
public:
  static vtkOpenXRRemotingRenderWindow* New();
  vtkTypeMacro(vtkOpenXRRemotingRenderWindow, vtkOpenXRRenderWindow);

  /**
   * Set the OpenXR remoting IP address to connect to.
   */
  void SetRemotingIPAddress(const char* host);

  /**
   * Initialize helper window resources.
   */
  void Initialize() override;

  /**
   * Overridden to lock the opengl rendering while copying.
   */
  void CopyResultFrame() override;

  ///@{
  /**
   * Overridden to draw to the shared D3D texture
   */
  void StereoUpdate() override;
  void StereoMidpoint() override;
  void StereoRenderComplete() override;
  ///@}

protected:
  vtkOpenXRRemotingRenderWindow();
  ~vtkOpenXRRemotingRenderWindow() = default;

  void RenderOneEye(uint32_t eye) override;

private:
  vtkOpenXRRemotingRenderWindow(const vtkOpenXRRemotingRenderWindow&) = delete;
  void operator=(const vtkOpenXRRemotingRenderWindow&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkOpenXRRemotingRenderWindow.h
