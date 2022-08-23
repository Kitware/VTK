/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenXRRemotingRenderWindow.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
   * Initialize helper window ressources.
   */
  void Initialize() override;

  ///@{
  /**
   * Overriden to draw to the shared D3D texture
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

#endif
// VTK-HeaderTest-Exclude: vtkOpenXRRemotingRenderWindow.h
