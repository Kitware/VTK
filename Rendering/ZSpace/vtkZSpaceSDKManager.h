// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkZSpaceSDKManager
 * @brief   Abstract zSpace SDK manager class.
 *
 * Abstract class handling the interactions between the zSpace plugin
 * and the zSpace SDK. The implementation of virtual functions depend
 * of the version of the zSpace SDK currently used : "zSpace Core SDK"
 * (older) and zSpace Core Compatibility SDK (newer).
 *
 * This class encapsulates all the calls to the zSpace SDK :
 *  - Initializes the zSpace SDK via InitializeZSpace(). This method looks
 *    for a zSpace device and optional trackers.
 *  - Synchronizes the zSpace SDK and the plugin via the Update method.
 *    This method corresponds to these successive calls :
 *    - UpdateViewport : send the viewport-relative information to the
 *      zSpace SDK (position, interpupillary distance, near and far plane)
 *      in order to allow it to perform valid stereo frustum computations;
 *    - UpdateViewAndProjectionMatrix : retrieve the the view and projection
 *      matrix for each eye from the SDK (tied to the stereo frustum);
 *    - UpdateTrackers : retrieve the head pose and the trackers pose
 *      (such as the stylus) from the SDK;
 *    - UpdateButtonState : retrieve the state of the buttons of the stylus
 *      (Down, Pressed, Up or None) from the SDK.
 *
 * For button states, the states Down/Up are set by this class; whereas the states
 * Pressed/None should be set by the calling class when the state Down/Up has been
 * processed, to ensure that the same input won't be processed multiple times.
 */

#ifndef vtkZSpaceSDKManager_h
#define vtkZSpaceSDKManager_h

#include "vtkNew.h" // for vtkNew
#include "vtkObject.h"
#include "vtkRenderingZSpaceModule.h" // for export macro

#include <vector> // for std::vector

VTK_ABI_NAMESPACE_BEGIN

class vtkRenderWindow;
class vtkCamera;
class vtkMatrix4x4;
class vtkTransform;
class vtkPVZSpaceView;

class VTKRENDERINGZSPACE_EXPORT vtkZSpaceSDKManager : public vtkObject
{
public:
  vtkTypeMacro(vtkZSpaceSDKManager, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Return the singleton instance (with no reference counting)
   * of a vtkZSpaceCoreSDKManager or vtkZSpaceCoreCompatibilitySDKManager
   * depending on the current zSpaceSDK in use.
   */
  static vtkZSpaceSDKManager* GetInstance();

  /**
   * Initialize the zSpace SDK and check for zSpace devices :
   * the display, the stylus and the head trackers.
   */
  virtual void InitializeZSpace() = 0;

  /**
   * Update the zSpace viewport position and size based
   * on the position and size of the application window.
   */
  virtual void UpdateViewport() = 0;

  /**
   * Update the position of the stylus and head trackers.
   */
  virtual void UpdateTrackers() = 0;

  /**
   * Update the zSpace view and projection matrix for each eye.
   */
  virtual void UpdateViewAndProjectionMatrix() = 0;

  /**
   * Update the stylus buttons state.
   */
  virtual void UpdateButtonState() = 0;

  /**
   * Let zSpace compute the viewer scale, camera position and camera view up from the
   * input bounds.
   */
  virtual void CalculateFrustumFit(
    const double bounds[6], double position[3], double viewUp[3]) = 0;

  ///@{
  /**
   * Notify the zSpace SDK for the beginning/end of a frame
   * (vtkZSpaceCoreCompatibility only)
   */
  virtual void BeginFrame() = 0;
  virtual void EndFrame() = 0;
  ///@}

  /**
   * Shutdown the zSpace SDK (clean its internal state).
   * Useful to re-initialize the zSpace SDK from a clean state.
   */
  virtual void ShutDown(){};

  ///@{
  /**
   * These functions are overriden by vtkZSpaceCoreCompatibility (specific to zSpace Inspire).
   * @see vtkZSpaceCoreCompatibility
   */
  virtual void EnableGraphicsBinding(){};
  virtual void SubmitFrame(unsigned int vtkNotUsed(leftText), unsigned int vtkNotUsed(rightText)){};
  virtual void GetPerEyeImageResolution(int* vtkNotUsed(width), int* vtkNotUsed(height)){};
  virtual void SetStereoDisplayEnabled(bool vtkNotUsed(enabled)){};
  virtual bool GetStereoDisplayEnabled() { return false; };
  ///@}

  enum StereoDisplayMode
  {
    QUAD_BUFFER_STEREO = 0,
    STEREO_DISPLAY_API = 1
  };

  virtual StereoDisplayMode GetStereoDisplayMode() { return QUAD_BUFFER_STEREO; };

  ///@{
  /**
   * Set the render windwow the manager makes viewport computations
   * from.
   */
  virtual void SetRenderWindow(vtkRenderWindow* renderWindow);
  ///@}

  /**
   * Update the viewport, the trackers and the camera matrix
   * by calling the zSpace SDK.
   */
  void Update();

  /**
   * Get the x position of the upper left corner of the zSpace display
   * in the virtual desktop.
   */
  vtkGetMacro(WindowX, int);

  /**
   * Get the y position of the upper left corner of the zSpace display
   * in the virtual desktop.
   */
  vtkGetMacro(WindowY, int);

  /**
   * Get the x resolution in pixels of the zSpace display.
   */
  vtkGetMacro(WindowWidth, int);

  /**
   * Get the y resolution in pixels of the zSpace display.
   */
  vtkGetMacro(WindowHeight, int);

  /**
   * Get the number of stylus connected to the zSpace device.
   */
  vtkGetMacro(StylusTargets, int);

  /**
   * Get the number of glasses connected to the zSpace device.
   */
  vtkGetMacro(HeadTargets, int);

  /**
   * Get the number of secondary targets connected to the zSpace device.
   */
  vtkGetMacro(SecondaryTargets, int);

  ///@{
  /**
   * Get/Set the distance between the eyes in meters.
   */
  vtkGetMacro(InterPupillaryDistance, float);
  vtkSetClampMacro(InterPupillaryDistance, float, 0.f, 1.f);
  ///@}

  /**
   * Set the near and far plane.
   */
  void SetClippingRange(const float nearPlane, const float farPlane);

  /**
   * Get the viewer scale.
   */
  vtkGetMacro(ViewerScale, float);

  /**
   * Get the near plane.
   */
  vtkGetMacro(NearPlane, float);

  /**
   * Get the far plane.
   */
  vtkGetMacro(FarPlane, float);

  /**
   * Get the zSpace view matrix without stereo (eye set as EYE_CENTER)
   * in row major format (VTK format)
   */
  vtkGetObjectMacro(CenterEyeViewMatrix, vtkMatrix4x4);

  /**
   * Get the zSpace view matrix for the right or left eye
   * in row major format (VTK format)
   */
  vtkMatrix4x4* GetStereoViewMatrix(bool leftEye);

  /**
   * Get the zSpace projection matrix without stereo (eye set as EYE_CENTER)
   * in row major format (VTK format)
   */
  vtkGetObjectMacro(CenterEyeProjectionMatrix, vtkMatrix4x4);

  /**
   * Get the zSpace projection matrix for the right or left eye
   * in row major format (VTK format)
   */
  vtkMatrix4x4* GetStereoProjectionMatrix(bool leftEye);

  /**
   * Get the zSpace stylus matrix in world space.
   * The matrix is in column major format and can be
   * used by OpenGL.
   */
  vtkGetObjectMacro(StylusMatrixColMajor, vtkMatrix4x4);

  /**
   * Get the zSpace stylus matrix in world space.
   * The matrix is in row major format and can be
   * used by VTK.
   */
  vtkGetObjectMacro(StylusMatrixRowMajor, vtkMatrix4x4);

  /**
   * Get the zSpace stylus transform in world space.
   * The transform has for matrix StylusMatrixRowMajor
   */
  vtkGetObjectMacro(StylusTransformRowMajor, vtkTransform);

  enum ButtonIds
  {
    MiddleButton = 0,
    RightButton = 1,
    LeftButton = 2,
    NumberOfButtons = 3
  };

  enum ButtonState
  {
    Down = 0,
    Pressed = 1,
    Up = 2,
    None = 3,
    NumberOfStates = 4
  };

  ///@{
  /**
   * Get/Set the state of the left button of the stylus.
   */
  vtkGetMacro(LeftButtonState, int);
  vtkSetEnumMacro(LeftButtonState, ButtonState);
  ///@}

  ///@{
  /**
   * Get/Set the state of the middle button of the stylus.
   */
  vtkGetMacro(MiddleButtonState, int);
  vtkSetEnumMacro(MiddleButtonState, ButtonState);
  ///@}

  ///@{
  /**
   * Get/Set the state of the right button of the stylus.
   */
  vtkGetMacro(RightButtonState, int);
  vtkSetEnumMacro(RightButtonState, ButtonState);
  ///@}

protected:
  vtkZSpaceSDKManager();
  ~vtkZSpaceSDKManager() override;

  vtkRenderWindow* RenderWindow = nullptr;

  vtkNew<vtkMatrix4x4> CenterEyeViewMatrix;
  vtkNew<vtkMatrix4x4> LeftEyeViewMatrix;
  vtkNew<vtkMatrix4x4> RightEyeViewMatrix;
  vtkNew<vtkMatrix4x4> CenterEyeProjectionMatrix;
  vtkNew<vtkMatrix4x4> LeftEyeProjectionMatrix;
  vtkNew<vtkMatrix4x4> RightEyeProjectionMatrix;

  // In column major format, used by openGL
  vtkNew<vtkMatrix4x4> StylusMatrixColMajor;

  // In row major format, used by VTK
  vtkNew<vtkMatrix4x4> StylusMatrixRowMajor;
  vtkNew<vtkTransform> StylusTransformRowMajor;

  int WindowX = 0;
  int WindowY = 0;
  int WindowWidth = 0;
  int WindowHeight = 0;

  // Store the type for each detected display devices
  std::vector<std::string> Displays;
  // The number of stylus
  int StylusTargets = 0;
  // The number of glasses
  int HeadTargets = 0;
  // Additional targets
  int SecondaryTargets = 0;

  // Inter pupillary distance in meters
  float InterPupillaryDistance = 0.056f;
  float ViewerScale = 1.f;
  // Camera near plane
  float NearPlane = 0.0001f;
  // Camera far plane
  float FarPlane = 1000.f;

  // For interactions, store the state of each buttons
  ButtonState LeftButtonState = None;
  ButtonState MiddleButtonState = None;
  ButtonState RightButtonState = None;
  // Store buttons state to iterate over them
  ButtonState* ButtonsState[NumberOfButtons] = { &MiddleButtonState, &RightButtonState,
    &LeftButtonState };

private:
  vtkZSpaceSDKManager(const vtkZSpaceSDKManager&) = delete;
  void operator=(const vtkZSpaceSDKManager&) = delete;
};

VTK_ABI_NAMESPACE_END

#endif
