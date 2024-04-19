// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkZSpaceCoreCompatibilitySDKManager
 * @brief   zSpace Core Compatibility SDK manager class.
 *
 * Class handling the interactions between the zSpace plugin
 * and the zSpace Core Compatibility SDK.
 * This class is private and should not be used directly. Please
 * use vtkZSpaceSDKManager instead.
 *
 * @see vtkZSpaceSDKManager
 */

#ifndef vtkZSpaceCoreCompatibilitySDKManager_h
#define vtkZSpaceCoreCompatibilitySDKManager_h

#include "vtkZSpaceSDKManager.h"

// Disable "anonymous struct/union" warning on zSpace compat headers
#pragma warning(disable : 4201)
#include "zSpaceCoreCompatibility.h" // zSpace header
#pragma warning(default : 4201)

#include <vector>    // for std::vector
#include <windows.h> // for HMODULE

VTK_ABI_NAMESPACE_BEGIN

/**
 * Structure holding the loaded zSpace Core Compatibility
 * API entry point function pointers
 */
struct zSpaceCoreCompatEntryPoints
{
  // Use the zSpace Core Compatibility API function name reflection macro to
  // auto-generate function pointer members for all zSpace Core Compatibility
  // API entry point functions.

#define ZC_COMPAT_SAMPLE_LOCAL_ENTRY_POINT_MEMBER(undecoratedFuncName)                             \
  ZCCompat##undecoratedFuncName##FuncPtrType zccompat##undecoratedFuncName;

  ZC_COMPAT_REFLECTION_LIST_UNDECORATED_FUNC_NAMES(ZC_COMPAT_SAMPLE_LOCAL_ENTRY_POINT_MEMBER)
#undef ZC_COMPAT_SAMPLE_LOCAL_ENTRY_POINT_MEMBER
};

class vtkRenderWindow;
class vtkMatrix4x4;

class vtkZSpaceCoreCompatibilitySDKManager : public vtkZSpaceSDKManager
{
public:
  static vtkZSpaceCoreCompatibilitySDKManager* New();
  vtkTypeMacro(vtkZSpaceCoreCompatibilitySDKManager, vtkZSpaceSDKManager);
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
   * Update the position of the stylus and head trackers.
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
   * Notify the zSpace SDK for the beginning/end of a frame.
   */
  void BeginFrame() override;
  void EndFrame() override;
  ///@}

  /**
   * Allow the zSpace Core Compatibility API to create its internal OpenGL resources
   * and prepare to accept eye textures and perform final rendering each frame.
   * Must be called right after the OpenGL context is created and made current.
   */
  void EnableGraphicsBinding() override;

  /**
   * Submit left / right eyes textures to the zSpace Core Compatibility API in order
   * to let it handle the final rendering into the mono back buffer.
   * Note 1: this will modify various parts of the OpenGL state so be sure to
   * save it before calling this method and restore it right after.
   * Note 2: the zSpace API will not swap the buffers after rendering in the backbuffer
   * so it should be done manually after calling this method.
   */
  void SubmitFrame(unsigned int leftText, unsigned int rightText) override;

  /**
   * Request from the zSpace Core Compatibility API the resolution needed to create
   * left / right eye textures. This can be different to the actual display resolution,
   * because of the scaling applied e.g. in case of high DPI screen.
   */
  void GetPerEyeImageResolution(signed int* width, signed int* height) override;

  /**
   * Return the actual stereo display mode, depending on zSpace hardware.
   * Possible values are:
   * - STEREO_DISPLAY_API for Inspire,
   * - QUAD_BUFFER_STEREO for other hardware.
   *
   * If the manager fails to retrieve it, the default returned value is QUAD_BUFFER_STEREO.
   */
  StereoDisplayMode GetStereoDisplayMode() override;

  ///@{
  /**
   * Set/Get if the "Stereo Display" is enabled.
   * When enabled, the zSpace Inspire hardware activates the autostereo lens
   * to display stereo content (i.e. send left/right image to left/right eye).
   *
   * If the manager fails to retrieve it, the default returned value is false.
   */
  void SetStereoDisplayEnabled(bool enabled) override;
  bool GetStereoDisplayEnabled() override;
  ///@}

  /**
   * Shutdown the zSpace SDK (clean its internal state).
   * Useful to re-initialize the zSpace SDK from a clean state.
   */
  void ShutDown() override;

  /**
   * Set the render window the manager makes viewport computations
   * from. Overridden to pass the related Windows window handle to
   * the SDK.
   */
  void SetRenderWindow(vtkRenderWindow*) override;

protected:
  vtkZSpaceCoreCompatibilitySDKManager();
  ~vtkZSpaceCoreCompatibilitySDKManager() override;

  ZCCompatContext ZSpaceContext = nullptr;
  ZCCompatDisplay DisplayHandle = nullptr;
  ZCCompatViewport ViewportHandle = nullptr;
  ZCCompatFrustum FrustumHandle = nullptr;
  ZCCompatTarget StylusHandle = nullptr;

  // Store the API functions entry points.
  zSpaceCoreCompatEntryPoints EntryPts;

  // Handle to the zSpace Core Compatibility API dynamic library (.dll).
  HMODULE zSpaceCoreCompatDllModuleHandle;

  // Handle to the current window
  HWND WidondowHandle;

  /**
   * Load the "zSpaceCoreCompatibility64.dll" shared library then load the
   * zSpace Core Compatibility API entry point functions (at runtime)
   */
  bool loadZspaceCoreCompatibilityEntryPoints(const char* zSpaceCoreCompatDllFilePath,
    HMODULE& zSpaceCoreCompatDllModuleHandle, zSpaceCoreCompatEntryPoints& entryPoints);

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

  /**
   * Set to true if zSpaceCoreCompatibility dynamic libraries are found upon
   * vtkZSpaceCoreCompatibilitySDKManager instantiation (see InitializeZSpace()).
   */
  bool Initialized = false;

private:
  vtkZSpaceCoreCompatibilitySDKManager(const vtkZSpaceCoreCompatibilitySDKManager&) = delete;
  void operator=(const vtkZSpaceCoreCompatibilitySDKManager&) = delete;
};

VTK_ABI_NAMESPACE_END

#endif
