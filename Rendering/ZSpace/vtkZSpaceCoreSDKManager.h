// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkZSpaceCoreSDKManager
 * @brief   zSpace Core SDK manager class.
 *
 * Class handling the interactions between the zSpace plugin
 * and the zSpace Core SDK.
 * This class is private and should not be used directly. Please
 * use vtkZSpaceSDKManager instead.
 *
 * @see vtkZSpaceSDKManager
 */

#ifndef vtkZSpaceCoreSDKManager_h
#define vtkZSpaceCoreSDKManager_h

#include "vtkZSpaceSDKManager.h"

#include <vector>   // for std::vector
#include <zspace.h> // zspace header

VTK_ABI_NAMESPACE_BEGIN

class vtkRenderWindow;
class vtkMatrix4x4;

class vtkZSpaceCoreSDKManager : public vtkZSpaceSDKManager
{
public:
  static vtkZSpaceCoreSDKManager* New();
  vtkTypeMacro(vtkZSpaceCoreSDKManager, vtkZSpaceSDKManager);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Initialize the zSpace SDK and check for zSpace devices :
   * the display, the stylus and the head trackers.
   */
  void InitializeZSpace() override;

  /**
   * Update the zSpace viewport position and size based
   * on the position and size of the application window.
   */
  void UpdateViewport() override;

  /**
   * Update the position of the stylus and head trakers.
   */
  void UpdateTrackers() override;

  /**
   * Update the zSpace view and projection matrix for each eye.
   */
  void UpdateViewAndProjectionMatrix() override;

  /**
   * Update the stylus buttons state.
   */
  void UpdateButtonState() override;

  /**
   * Let zSpace compute the viewer scale, camera position and camera view up from the
   * input bounds.
   */
  void CalculateFrustumFit(const double bounds[6], double position[3], double viewUp[3]) override;

  ///@{
  /**
   * Notify the zSpace SDK for the begining of a frame.
   * There is no "end frame" notification to make in the
   * case of the zSpace Core SDK.
   */
  void BeginFrame() override;
  void EndFrame() override{};
  ///@}

protected:
  vtkZSpaceCoreSDKManager();
  ~vtkZSpaceCoreSDKManager() override;

  ZCContext ZSpaceContext = nullptr;
  ZCHandle DisplayHandle = nullptr;
  ZCHandle BufferHandle = nullptr;
  ZCHandle ViewportHandle = nullptr;
  ZCHandle FrustumHandle = nullptr;
  ZCHandle StylusHandle = nullptr;

  /**
   * zSpace stores matrix in column-major format (as OpenGL). The matrix
   * needs to be transposed to be used by VTK.
   */
  void ConvertAndTransposeZSpaceMatrixToVTKMatrix(ZSMatrix4 zSpaceMatrix, vtkMatrix4x4* vtkMatrix);

  /**
   * zSpace stores matrix in column-major format (as OpenGL). The matrix
   * needs to be transposed to be used by VTK.
   */
  void ConvertZSpaceMatrixToVTKMatrix(ZSMatrix4 zSpaceMatrix, vtkMatrix4x4* vtkMatrix);

private:
  vtkZSpaceCoreSDKManager(const vtkZSpaceCoreSDKManager&) = delete;
  void operator=(const vtkZSpaceCoreSDKManager&) = delete;
};

VTK_ABI_NAMESPACE_END

#endif
