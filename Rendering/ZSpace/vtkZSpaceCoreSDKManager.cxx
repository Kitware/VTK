// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkZSpaceCoreSDKManager.h"

#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkSmartPointer.h"
#include "vtkTransform.h"

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkZSpaceCoreSDKManager);

#define ZSPACE_CHECK_ERROR(fn, error)                                                              \
  if (error != ZC_ERROR_OK)                                                                        \
  {                                                                                                \
    char errorString[256];                                                                         \
    zcGetErrorString(error, errorString, sizeof(errorString));                                     \
    vtkErrorMacro(<< "vtkZSpaceCoreSDKManager::" << #fn << " error : " << errorString);            \
  }

//------------------------------------------------------------------------------
vtkZSpaceCoreSDKManager::vtkZSpaceCoreSDKManager()
{
  this->InitializeZSpace();
}

//------------------------------------------------------------------------------
vtkZSpaceCoreSDKManager::~vtkZSpaceCoreSDKManager()
{
  ZCError error;
  error = zcDestroyStereoBuffer(this->BufferHandle);
  ZSPACE_CHECK_ERROR(zcDestroyStereoBuffer, error);
  error = zcDestroyViewport(this->ViewportHandle);
  ZSPACE_CHECK_ERROR(zcDestroyViewport, error);
  error = zcShutDown(this->ZSpaceContext);
  ZSPACE_CHECK_ERROR(zcShutDown, error);
}

//------------------------------------------------------------------------------
void vtkZSpaceCoreSDKManager::InitializeZSpace()
{
  ZCError error;

  // Initialize the zSpace SDK. This MUST be called before
  // calling any other zSpace API.
  error = zcInitialize(&this->ZSpaceContext);
  ZSPACE_CHECK_ERROR(zcInitialize, error);

  // Check the SDK version
  ZSInt32 major, minor, patch;
  error = zcGetRuntimeVersion(this->ZSpaceContext, &major, &minor, &patch);
  ZSPACE_CHECK_ERROR(zcGetRuntimeVersion, error);

  vtkDebugMacro(<< "zSpace SDK version: " << major << "." << minor << "." << patch);

  int numDisplays;
  error = zcGetNumDisplays(this->ZSpaceContext, &numDisplays);

  this->Displays.reserve(numDisplays);

  for (int i = 0; i < numDisplays; i++)
  {
    ZCHandle displayHandle;
    error = zcGetDisplayByIndex(this->ZSpaceContext, i, &displayHandle);
    ZSPACE_CHECK_ERROR(zcGetDisplayByIndex, error);

    ZCDisplayType displayType;
    error = zcGetDisplayType(displayHandle, &displayType);
    ZSPACE_CHECK_ERROR(zcGetDisplayType, error);

    switch (displayType)
    {
      case ZC_DISPLAY_TYPE_GENERIC:
        this->Displays.push_back("Generic");
        break;
      case ZC_DISPLAY_TYPE_ZSPACE:
        this->Displays.push_back("ZSpace");
        break;
      default:
        this->Displays.push_back("Unknown");
        break;
    }
  }

  // Create a stereo buffer to handle L/R detection.
  error =
    zcCreateStereoBuffer(this->ZSpaceContext, ZC_RENDERER_QUAD_BUFFER_GL, 0, &this->BufferHandle);
  ZSPACE_CHECK_ERROR(zcCreateStereoBuffer, error);

  // Create a zSpace viewport object and grab its associated frustum.
  // Note: The zSpace viewport is abstract and not an actual window/viewport
  // that is created and registered through the Windows OS. It manages
  // a zSpace stereo frustum, which is responsible for various stereoscopic
  // 3D calculations such as calculating the view and projection matrices for
  // each eye.
  error = zcCreateViewport(this->ZSpaceContext, &this->ViewportHandle);
  ZSPACE_CHECK_ERROR(zcCreateViewport, error);

  error = zcGetFrustum(this->ViewportHandle, &this->FrustumHandle);
  ZSPACE_CHECK_ERROR(zcGetFrustum, error);

  // Enable auto stereo.
  error =
    zcSetFrustumAttributeB(this->FrustumHandle, ZC_FRUSTUM_ATTRIBUTE_AUTO_STEREO_ENABLED, true);
  ZSPACE_CHECK_ERROR(zcSetFrustumAttributeB, error);
  error = zcSetFrustumAttributeF32(
    this->FrustumHandle, ZC_FRUSTUM_ATTRIBUTE_IPD, this->InterPupillaryDistance);
  ZSPACE_CHECK_ERROR(zcSetFrustumAttributeF32, error);
  error = zcSetFrustumAttributeF32(this->FrustumHandle, ZC_FRUSTUM_ATTRIBUTE_HEAD_SCALE, 1.f);
  ZSPACE_CHECK_ERROR(zcSetFrustumAttributeF32, error);

  error = zcSetFrustumPortalMode(this->FrustumHandle, ZC_PORTAL_MODE_NONE);
  ZSPACE_CHECK_ERROR(zcSetFrustumPortalMode, error);

  error = zcGetNumTargetsByType(this->ZSpaceContext, ZC_TARGET_TYPE_PRIMARY, &this->StylusTargets);
  ZSPACE_CHECK_ERROR(zcGetNumTargetsByType, error);

  error = zcGetNumTargetsByType(this->ZSpaceContext, ZC_TARGET_TYPE_HEAD, &this->HeadTargets);
  ZSPACE_CHECK_ERROR(zcGetNumTargetsByType, error);

  error =
    zcGetNumTargetsByType(this->ZSpaceContext, ZC_TARGET_TYPE_SECONDARY, &this->SecondaryTargets);
  ZSPACE_CHECK_ERROR(zcGetNumTargetsByType, error);

  // Grab a handle to the stylus target.
  error = zcGetTargetByType(this->ZSpaceContext, ZC_TARGET_TYPE_PRIMARY, 0, &this->StylusHandle);
  ZSPACE_CHECK_ERROR(zcGetTargetByType, error);

  // Find the zSpace display and set the window position
  // to be the top left corner of the zSpace display.
  error = zcGetDisplayByType(this->ZSpaceContext, ZC_DISPLAY_TYPE_ZSPACE, 0, &this->DisplayHandle);
  ZSPACE_CHECK_ERROR(zcGetDisplayByType, error);

  error = zcGetDisplayPosition(this->DisplayHandle, &this->WindowX, &this->WindowY);
  ZSPACE_CHECK_ERROR(zcGetDisplayPosition, error);

  error =
    zcGetDisplayNativeResolution(this->DisplayHandle, &this->WindowWidth, &this->WindowHeight);
  ZSPACE_CHECK_ERROR(zcGetDisplayNativeResolution, error);
}

//------------------------------------------------------------------------------
void vtkZSpaceCoreSDKManager::UpdateViewport()
{
  ZCError error;

  if (!this->RenderWindow)
  {
    vtkErrorMacro("No render window has been set to the zSpace SDK manager !");
    return;
  }

  int* position = this->RenderWindow->GetPosition();
  int* size = this->RenderWindow->GetSize();

  error = zcSetViewportPosition(this->ViewportHandle, position[0], position[1]);
  ZSPACE_CHECK_ERROR(zcSetViewportPosition, error);
  error = zcSetViewportSize(this->ViewportHandle, size[0], size[1]);
  ZSPACE_CHECK_ERROR(zcSetViewportSize, error);

  // Update inter pupillary distance
  error = zcSetFrustumAttributeF32(
    this->FrustumHandle, ZC_FRUSTUM_ATTRIBUTE_IPD, this->InterPupillaryDistance);
  ZSPACE_CHECK_ERROR(zcSetFrustumAttributeF32, error);

  // Near and far plane
  error =
    zcSetFrustumAttributeF32(this->FrustumHandle, ZC_FRUSTUM_ATTRIBUTE_NEAR_CLIP, this->NearPlane);
  ZSPACE_CHECK_ERROR(zcSetFrustumAttributeF32, error);
  error =
    zcSetFrustumAttributeF32(this->FrustumHandle, ZC_FRUSTUM_ATTRIBUTE_FAR_CLIP, this->FarPlane);
  ZSPACE_CHECK_ERROR(zcSetFrustumAttributeF32, error);
}

//------------------------------------------------------------------------------
void vtkZSpaceCoreSDKManager::UpdateTrackers()
{
  ZCError error;

  // Update the zSpace SDK. This updates both tracking information
  // as well as the head poses for any frustums that have been created.
  error = zcUpdate(this->ZSpaceContext);
  ZSPACE_CHECK_ERROR(zcUpdate, error);

  // Update the stylus matrix
  ZCTrackerPose stylusPose;
  error = zcGetTargetTransformedPose(
    this->StylusHandle, this->ViewportHandle, ZC_COORDINATE_SPACE_CAMERA, &stylusPose);
  ZSPACE_CHECK_ERROR(zcGetTargetTransformedPose, error);

  vtkZSpaceCoreSDKManager::ConvertZSpaceMatrixToVTKMatrix(
    stylusPose.matrix, this->StylusMatrixColMajor);

  // The stylus direction is the normalized negative Z axis of the pose
  this->StylusMatrixColMajor->SetElement(2, 0, -this->StylusMatrixColMajor->GetElement(2, 0));
  this->StylusMatrixColMajor->SetElement(2, 1, -this->StylusMatrixColMajor->GetElement(2, 1));
  this->StylusMatrixColMajor->SetElement(2, 2, -this->StylusMatrixColMajor->GetElement(2, 2));

  vtkNew<vtkMatrix4x4> invView;
  vtkMatrix4x4::Invert(this->CenterEyeViewMatrix.GetPointer(), invView);

  // Convert from camera space to world space
  vtkMatrix4x4::Multiply4x4(invView, this->StylusMatrixColMajor, this->StylusMatrixColMajor);

  // Transpose the matrix to be used by VTK
  vtkMatrix4x4::Transpose(this->StylusMatrixColMajor, this->StylusMatrixRowMajor);

  this->StylusTransformRowMajor->SetMatrix(this->StylusMatrixRowMajor);
}

//------------------------------------------------------------------------------
void vtkZSpaceCoreSDKManager::UpdateViewAndProjectionMatrix()
{
  ZCError error;

  // Update the view matrix for each eye
  ZSMatrix4 zcViewMatrix;
  error = zcGetFrustumViewMatrix(this->FrustumHandle, ZC_EYE_CENTER, &zcViewMatrix);
  ZSPACE_CHECK_ERROR(zcGetFrustumViewMatrix, error);
  vtkZSpaceCoreSDKManager::ConvertAndTransposeZSpaceMatrixToVTKMatrix(
    zcViewMatrix, this->CenterEyeViewMatrix);

  error = zcGetFrustumViewMatrix(this->FrustumHandle, ZC_EYE_LEFT, &zcViewMatrix);
  ZSPACE_CHECK_ERROR(zcGetFrustumViewMatrix, error);
  vtkZSpaceCoreSDKManager::ConvertAndTransposeZSpaceMatrixToVTKMatrix(
    zcViewMatrix, this->LeftEyeViewMatrix);

  error = zcGetFrustumViewMatrix(this->FrustumHandle, ZC_EYE_RIGHT, &zcViewMatrix);
  ZSPACE_CHECK_ERROR(zcGetFrustumViewMatrix, error);
  vtkZSpaceCoreSDKManager::ConvertAndTransposeZSpaceMatrixToVTKMatrix(
    zcViewMatrix, this->RightEyeViewMatrix);

  // Update the projection matrix for each eye
  ZSMatrix4 zcProjectionMatrix;
  error = zcGetFrustumProjectionMatrix(this->FrustumHandle, ZC_EYE_CENTER, &zcProjectionMatrix);
  ZSPACE_CHECK_ERROR(zcGetFrustumProjectionMatrix, error);
  vtkZSpaceCoreSDKManager::ConvertAndTransposeZSpaceMatrixToVTKMatrix(
    zcProjectionMatrix, this->CenterEyeProjectionMatrix);

  error = zcGetFrustumProjectionMatrix(this->FrustumHandle, ZC_EYE_LEFT, &zcProjectionMatrix);
  ZSPACE_CHECK_ERROR(zcGetFrustumProjectionMatrix, error);
  vtkZSpaceCoreSDKManager::ConvertAndTransposeZSpaceMatrixToVTKMatrix(
    zcProjectionMatrix, this->LeftEyeProjectionMatrix);

  error = zcGetFrustumProjectionMatrix(this->FrustumHandle, ZC_EYE_RIGHT, &zcProjectionMatrix);
  ZSPACE_CHECK_ERROR(zcGetFrustumProjectionMatrix, error);
  vtkZSpaceCoreSDKManager::ConvertAndTransposeZSpaceMatrixToVTKMatrix(
    zcProjectionMatrix, this->RightEyeProjectionMatrix);
}

//------------------------------------------------------------------------------
void vtkZSpaceCoreSDKManager::UpdateButtonState()
{
  ZSBool isButtonPressed;

  for (int buttonId = vtkZSpaceSDKManager::MiddleButton;
       buttonId < vtkZSpaceSDKManager::NumberOfButtons; ++buttonId)
  {
    ZCError error = zcIsTargetButtonPressed(this->StylusHandle, buttonId, &isButtonPressed);
    ZSPACE_CHECK_ERROR(zcIsTargetButtonPressed, error);

    ButtonState& buttonState = *this->ButtonsState[buttonId];
    buttonState = isButtonPressed
      ? buttonState != vtkZSpaceSDKManager::Pressed ? vtkZSpaceSDKManager::Down
                                                    : vtkZSpaceSDKManager::Pressed
      : buttonState != vtkZSpaceSDKManager::None ? vtkZSpaceSDKManager::Up
                                                 : vtkZSpaceSDKManager::None;
  }
}

//------------------------------------------------------------------------------
void vtkZSpaceCoreSDKManager::CalculateFrustumFit(
  const double bounds[6], double position[3], double viewUp[3])
{
  // Expand bounds a little bit to make sure object is not clipped
  const double w1 = bounds[1] - bounds[0];
  const double w2 = bounds[3] - bounds[2];
  const double w3 = bounds[5] - bounds[4];

  ZCBoundingBox zcBbox;
  zcBbox.lower.x = bounds[0] - w1 / 4.0;
  zcBbox.lower.y = bounds[2] - w2 / 4.0;
  zcBbox.lower.z = bounds[4] - w3 / 4.0;

  zcBbox.upper.x = bounds[1] + w1 / 4.0;
  zcBbox.upper.y = bounds[3] + w2 / 4.0;
  zcBbox.upper.z = bounds[5] + w3 / 4.0;

  ZSMatrix4 zcLookAtMatrix;
  ZSFloat zcViewerScale;

  // Calculate the appropriate viewer scale and camera lookat matrix
  // such that content in the above bounding box will take up the entire
  // viewport without being clipped.
  ZCError error =
    zcCalculateFrustumFit(this->FrustumHandle, &zcBbox, &zcViewerScale, &zcLookAtMatrix);
  ZSPACE_CHECK_ERROR(zcCalculateFrustumFit, error);

  // Set the frustum's viewer scale with the value that was calculated
  // by zcCalculateFrustumFit().
  error =
    zcSetFrustumAttributeF32(this->FrustumHandle, ZC_FRUSTUM_ATTRIBUTE_VIEWER_SCALE, zcViewerScale);
  ZSPACE_CHECK_ERROR(zcSetFrustumAttributeF32, error);

  this->ViewerScale = static_cast<float>(zcViewerScale);

  position[0] = -zcLookAtMatrix.m03;
  position[1] = -zcLookAtMatrix.m13;
  position[2] = -zcLookAtMatrix.m23;

  viewUp[0] = zcLookAtMatrix.m01;
  viewUp[1] = zcLookAtMatrix.m11;
  viewUp[2] = zcLookAtMatrix.m21;
}

//------------------------------------------------------------------------------
void vtkZSpaceCoreSDKManager::BeginFrame()
{
  ZCError error = zcBeginStereoBufferFrame(this->BufferHandle);
  ZSPACE_CHECK_ERROR(zcBeginStereoBufferFrame, error);
}

//------------------------------------------------------------------------------
void vtkZSpaceCoreSDKManager::ConvertAndTransposeZSpaceMatrixToVTKMatrix(
  ZSMatrix4 zSpaceMatrix, vtkMatrix4x4* vtkMatrix)
{
  for (int i = 0; i < 16; ++i)
  {
    vtkMatrix->SetElement(i % 4, i / 4, zSpaceMatrix.f[i]);
  }
}

//------------------------------------------------------------------------------
void vtkZSpaceCoreSDKManager::ConvertZSpaceMatrixToVTKMatrix(
  ZSMatrix4 zSpaceMatrix, vtkMatrix4x4* vtkMatrix)
{
  for (int i = 0; i < 16; ++i)
  {
    vtkMatrix->SetElement(i / 4, i % 4, zSpaceMatrix.f[i]);
  }
}

//------------------------------------------------------------------------------
void vtkZSpaceCoreSDKManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

VTK_ABI_NAMESPACE_END
