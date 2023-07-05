// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkZSpaceCoreCompatibilitySDKManager.h"

#include "vtkBoundingBox.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkSmartPointer.h"
#include "vtkTransform.h"
#include "vtkVector.h"
#include "vtkVectorOperators.h"

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkZSpaceCoreCompatibilitySDKManager);

#define ZSPACE_CHECK_ERROR(fn, error)                                                              \
  if (error != ZC_COMPAT_ERROR_OK)                                                                 \
  {                                                                                                \
    std::string errorMessage = std::string("zSpace Core Compatibility API call \"") +              \
      std::string(#fn) +                                                                           \
      "\" failed with error "                                                                      \
      "code " +                                                                                    \
      std::to_string(error) + ".";                                                                 \
    vtkErrorMacro(<< "vtkZSpaceCoreCompatibilitySDKManager::" << #fn << " error : " << error);     \
  }

static const char* ZSPACE_CORE_COMPATIBILITY_DLL_FILE_PATH = "zSpaceCoreCompatibility"
#ifdef _WIN64
                                                             "64"
#else
                                                             "32"
#endif
  ;

//------------------------------------------------------------------------------
vtkZSpaceCoreCompatibilitySDKManager::vtkZSpaceCoreCompatibilitySDKManager()
{
  this->InitializeZSpace();
}

//------------------------------------------------------------------------------
vtkZSpaceCoreCompatibilitySDKManager::~vtkZSpaceCoreCompatibilitySDKManager()
{
  if (!this->Initialized)
  {
    return;
  }
  ZCCompatError error;

  error = this->EntryPts.zccompatShutDown(this->ZSpaceContext);
  ZSPACE_CHECK_ERROR(zccompatShutDown, error);
}

//------------------------------------------------------------------------------
bool vtkZSpaceCoreCompatibilitySDKManager::loadZspaceCoreCompatibilityEntryPoints(
  const char* zSpaceCoreCompatDllFilePath, HMODULE& dllModuleHandle,
  zSpaceCoreCompatEntryPoints& entryPoints)
{
  dllModuleHandle = LoadLibraryA(zSpaceCoreCompatDllFilePath);

  if (dllModuleHandle == nullptr)
  {
    // If the release variant of the zSpace Core Compatibility API DLL
    // could not be loaded, attempt to the debug variant instead.

    std::string zSpaceCoreCompatDllDebugFilePath(zSpaceCoreCompatDllFilePath);
    zSpaceCoreCompatDllDebugFilePath.append("_D");

    dllModuleHandle = LoadLibraryA(zSpaceCoreCompatDllDebugFilePath.c_str());

    if (dllModuleHandle == nullptr)
    {
      vtkErrorMacro(<< "Win32 Error : "
                    << "Failed to load zSpace Core Compatibility API DLL.");

      return false;
    }
  }

#define ZC_COMPAT_SAMPLE_LOCAL_LOAD_ENTRY_POINT(undecoratedFuncName)                               \
  {                                                                                                \
    void* entryPointProcAddress =                                                                  \
      GetProcAddress(dllModuleHandle, "zccompat" #undecoratedFuncName);                            \
                                                                                                   \
    if (entryPointProcAddress == nullptr)                                                          \
    {                                                                                              \
      vtkErrorMacro(<< "Win32 Error : "                                                            \
                    << "Failed to get zSpace Core Compatibility entry point "                      \
                    << "proc address for entry point "                                             \
                    << "\"zccompat" << #undecoratedFuncName << "\".");                             \
                                                                                                   \
      return false;                                                                                \
    }                                                                                              \
                                                                                                   \
    entryPoints.zccompat##undecoratedFuncName =                                                    \
      reinterpret_cast<ZCCompat##undecoratedFuncName##FuncPtrType>(entryPointProcAddress);         \
  }

  ZC_COMPAT_REFLECTION_LIST_UNDECORATED_FUNC_NAMES(ZC_COMPAT_SAMPLE_LOCAL_LOAD_ENTRY_POINT)

#undef ZC_COMPAT_SAMPLE_LOCAL_LOAD_ENTRY_POINT

  return true;
}

//------------------------------------------------------------------------------
void vtkZSpaceCoreCompatibilitySDKManager::InitializeZSpace()
{
  const bool didSucceed = loadZspaceCoreCompatibilityEntryPoints(
    ZSPACE_CORE_COMPATIBILITY_DLL_FILE_PATH, this->zSpaceCoreCompatDllModuleHandle, this->EntryPts);

  if (!didSucceed)
  {
    vtkErrorMacro("Unable to load the SDK functions entry points.");
    return;
  }

  this->Initialized = true;

  ZCCompatError error;

  // Initialize the zSpace SDK. This MUST be called before
  // calling any other zSpace API.
  error = this->EntryPts.zccompatInitialize(nullptr, nullptr, &this->ZSpaceContext);
  ZSPACE_CHECK_ERROR(zccompatInitialize, error);

  // Check the SDK version
  ZSInt32 major, minor, patch;
  error = this->EntryPts.zccompatGetRuntimeVersion(this->ZSpaceContext, &major, &minor, &patch);
  ZSPACE_CHECK_ERROR(zccompatGetRuntimeVersion, error);

  vtkDebugMacro(<< "zSpace SDK version: " << major << "." << minor << "." << patch);

  int numDisplays;
  error = this->EntryPts.zccompatGetNumDisplays(this->ZSpaceContext, &numDisplays);

  this->Displays.reserve(numDisplays);

  for (int i = 0; i < numDisplays; i++)
  {
    ZCCompatDisplay displayHandle;
    error = this->EntryPts.zccompatGetDisplayByIndex(this->ZSpaceContext, i, &displayHandle);
    ZSPACE_CHECK_ERROR(zccompatGetDisplayByIndex, error);

    ZCCompatDisplayType displayType;
    error = this->EntryPts.zccompatGetDisplayType(displayHandle, &displayType);
    ZSPACE_CHECK_ERROR(zccompatGetDisplayType, error);

    switch (displayType)
    {
      case ZC_COMPAT_DISPLAY_TYPE_GENERIC:
        this->Displays.push_back("Generic");
        break;
      case ZC_COMPAT_DISPLAY_TYPE_ZSPACE:
        this->Displays.push_back("ZSpace");
        break;
      default:
        this->Displays.push_back("Unknown");
        break;
    }
  }

  // Retrieve the zSpace primary viewport object and grab its associated frustum.
  // Note: The zSpace viewport is abstract and not an actual window/viewport
  // that is created and registered through the Windows OS. It manages
  // a zSpace stereo frustum, which is responsible for various stereoscopic
  // 3D calculations such as calculating the view and projection matrices for
  // each eye.
  error = this->EntryPts.zccompatGetPrimaryViewport(this->ZSpaceContext, &this->ViewportHandle);
  ZSPACE_CHECK_ERROR(zccompatCreateViewport, error);

  error = this->EntryPts.zccompatGetFrustum(this->ViewportHandle, &this->FrustumHandle);
  ZSPACE_CHECK_ERROR(zccompatGetFrustum, error);

  // Enable auto stereo.
  error = this->EntryPts.zccompatSetFrustumAttributeB(
    this->FrustumHandle, ZC_COMPAT_FRUSTUM_ATTRIBUTE_AUTO_STEREO_ENABLED, true);
  ZSPACE_CHECK_ERROR(zccompatSetFrustumAttributeB, error);
  error = this->EntryPts.zccompatSetFrustumAttributeF32(
    this->FrustumHandle, ZC_COMPAT_FRUSTUM_ATTRIBUTE_IPD, this->InterPupillaryDistance);
  ZSPACE_CHECK_ERROR(zccompatSetFrustumAttributeF32, error);
  error = this->EntryPts.zccompatSetFrustumAttributeF32(
    this->FrustumHandle, ZC_COMPAT_FRUSTUM_ATTRIBUTE_HEAD_SCALE, 1.f);
  ZSPACE_CHECK_ERROR(zccompatSetFrustumAttributeF32, error);

  // Disable the portal mode
  error = this->EntryPts.zccompatSetFrustumPortalMode(this->FrustumHandle, 0);
  ZSPACE_CHECK_ERROR(zccompatSetFrustumPortalMode, error);

  error = this->EntryPts.zccompatGetNumTargetsByType(
    this->ZSpaceContext, ZC_COMPAT_TARGET_TYPE_PRIMARY, &this->StylusTargets);
  ZSPACE_CHECK_ERROR(zccompatGetNumTargetsByType, error);

  error = this->EntryPts.zccompatGetNumTargetsByType(
    this->ZSpaceContext, ZC_COMPAT_TARGET_TYPE_HEAD, &this->HeadTargets);
  ZSPACE_CHECK_ERROR(zccompatGetNumTargetsByType, error);

  error = this->EntryPts.zccompatGetNumTargetsByType(
    this->ZSpaceContext, ZC_COMPAT_TARGET_TYPE_SECONDARY, &this->SecondaryTargets);
  ZSPACE_CHECK_ERROR(zccompatGetNumTargetsByType, error);

  // Grab a handle to the stylus target.
  error = this->EntryPts.zccompatGetTargetByType(
    this->ZSpaceContext, ZC_COMPAT_TARGET_TYPE_PRIMARY, 0, &this->StylusHandle);
  ZSPACE_CHECK_ERROR(zccompatGetTargetByType, error);

  // Find the zSpace display and set the window position
  // to be the top left corner of the zSpace display.
  error = this->EntryPts.zccompatGetDisplayByType(
    this->ZSpaceContext, ZC_COMPAT_DISPLAY_TYPE_ZSPACE, 0, &this->DisplayHandle);
  ZSPACE_CHECK_ERROR(zccompatGetDisplayByType, error);

  error =
    this->EntryPts.zccompatGetDisplayPosition(this->DisplayHandle, &this->WindowX, &this->WindowY);
  ZSPACE_CHECK_ERROR(zccompatGetDisplayPosition, error);

  error = this->EntryPts.zccompatGetDisplayNativeResolution(
    this->DisplayHandle, &this->WindowWidth, &this->WindowHeight);
  ZSPACE_CHECK_ERROR(zccompatGetDisplayNativeResolution, error);
}

//------------------------------------------------------------------------------
void vtkZSpaceCoreCompatibilitySDKManager::UpdateViewport()
{
  if (!this->Initialized)
  {
    return;
  }

  if (!this->RenderWindow)
  {
    vtkErrorMacro("No render window has been set to the zSpace SDK manager !");
    return;
  }

  int* position = this->RenderWindow->GetPosition();
  int* size = this->RenderWindow->GetSize();

  ZCCompatError error;

  error =
    this->EntryPts.zccompatSetViewportPosition(this->ViewportHandle, position[0], position[1]);
  ZSPACE_CHECK_ERROR(zccompatSetViewportPosition, error);
  error = this->EntryPts.zccompatSetViewportSize(this->ViewportHandle, size[0], size[1]);
  ZSPACE_CHECK_ERROR(zccompatSetViewportSize, error);

  // Update inter pupillary distance
  error = this->EntryPts.zccompatSetFrustumAttributeF32(
    this->FrustumHandle, ZC_COMPAT_FRUSTUM_ATTRIBUTE_IPD, this->InterPupillaryDistance);
  ZSPACE_CHECK_ERROR(zccompatSetFrustumAttributeF32, error);

  // Near and far plane
  error = this->EntryPts.zccompatSetFrustumAttributeF32(
    this->FrustumHandle, ZC_COMPAT_FRUSTUM_ATTRIBUTE_NEAR_CLIP, this->NearPlane);
  ZSPACE_CHECK_ERROR(zccompatSetFrustumAttributeF32, error);
  error = this->EntryPts.zccompatSetFrustumAttributeF32(
    this->FrustumHandle, ZC_COMPAT_FRUSTUM_ATTRIBUTE_FAR_CLIP, this->FarPlane);
  ZSPACE_CHECK_ERROR(zccompatSetFrustumAttributeF32, error);
}

//------------------------------------------------------------------------------
void vtkZSpaceCoreCompatibilitySDKManager::UpdateTrackers()
{
  if (!this->Initialized)
  {
    return;
  }

  ZCCompatError error;

  // Update the zSpace SDK. This updates both tracking information
  // as well as the head poses for any frustums that have been created.
  error = this->EntryPts.zccompatUpdate(this->ZSpaceContext);
  ZSPACE_CHECK_ERROR(zccompatUpdate, error);

  // Update the stylus matrix
  ZCCompatTrackerPose stylusPose;
  error = this->EntryPts.zccompatGetTargetPose(this->StylusHandle, &stylusPose);
  ZSPACE_CHECK_ERROR(zccompatGetTargetPose, error);
  error =
    this->EntryPts.zccompatTransformMatrix(this->ViewportHandle, ZC_COMPAT_COORDINATE_SPACE_TRACKER,
      ZC_COMPAT_COORDINATE_SPACE_CAMERA, &stylusPose.matrix); // Manual transfo added
  ZSPACE_CHECK_ERROR(zccompatTransformMatrix, error);

  this->ConvertZSpaceMatrixToVTKMatrix(stylusPose.matrix, this->StylusMatrixColMajor);

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
void vtkZSpaceCoreCompatibilitySDKManager::UpdateViewAndProjectionMatrix()
{
  if (!this->Initialized)
  {
    return;
  }

  ZCCompatError error;

  // Update the view matrix for each eye
  ZSMatrix4 zccompatViewMatrix;
  error = this->EntryPts.zccompatGetFrustumViewMatrix(
    this->FrustumHandle, ZC_COMPAT_EYE_CENTER, &zccompatViewMatrix);
  ZSPACE_CHECK_ERROR(zccompatGetFrustumViewMatrix, error);
  this->ConvertAndTransposeZSpaceMatrixToVTKMatrix(zccompatViewMatrix, this->CenterEyeViewMatrix);

  error = this->EntryPts.zccompatGetFrustumViewMatrix(
    this->FrustumHandle, ZC_COMPAT_EYE_LEFT, &zccompatViewMatrix);
  ZSPACE_CHECK_ERROR(zccompatGetFrustumViewMatrix, error);
  this->ConvertAndTransposeZSpaceMatrixToVTKMatrix(zccompatViewMatrix, this->LeftEyeViewMatrix);

  error = this->EntryPts.zccompatGetFrustumViewMatrix(
    this->FrustumHandle, ZC_COMPAT_EYE_RIGHT, &zccompatViewMatrix);
  ZSPACE_CHECK_ERROR(zccompatGetFrustumViewMatrix, error);
  this->ConvertAndTransposeZSpaceMatrixToVTKMatrix(zccompatViewMatrix, this->RightEyeViewMatrix);

  // Update the projection matrix for each eye
  ZSMatrix4 zccompatProjectionMatrix;
  error = this->EntryPts.zccompatGetFrustumProjectionMatrix(
    this->FrustumHandle, ZC_COMPAT_EYE_CENTER, &zccompatProjectionMatrix);
  ZSPACE_CHECK_ERROR(zccompatGetFrustumProjectionMatrix, error);
  this->ConvertAndTransposeZSpaceMatrixToVTKMatrix(
    zccompatProjectionMatrix, this->CenterEyeProjectionMatrix);

  error = this->EntryPts.zccompatGetFrustumProjectionMatrix(
    this->FrustumHandle, ZC_COMPAT_EYE_LEFT, &zccompatProjectionMatrix);
  ZSPACE_CHECK_ERROR(zccompatGetFrustumProjectionMatrix, error);
  this->ConvertAndTransposeZSpaceMatrixToVTKMatrix(
    zccompatProjectionMatrix, this->LeftEyeProjectionMatrix);

  error = this->EntryPts.zccompatGetFrustumProjectionMatrix(
    this->FrustumHandle, ZC_COMPAT_EYE_RIGHT, &zccompatProjectionMatrix);
  ZSPACE_CHECK_ERROR(zccompatGetFrustumProjectionMatrix, error);
  this->ConvertAndTransposeZSpaceMatrixToVTKMatrix(
    zccompatProjectionMatrix, this->RightEyeProjectionMatrix);
}

//------------------------------------------------------------------------------
void vtkZSpaceCoreCompatibilitySDKManager::UpdateButtonState()
{
  if (!this->Initialized)
  {
    return;
  }

  ZSBool isButtonPressed;

  for (int buttonId = vtkZSpaceSDKManager::MiddleButton;
       buttonId < vtkZSpaceSDKManager::NumberOfButtons; ++buttonId)
  {
    this->EntryPts.zccompatIsTargetButtonPressed(this->StylusHandle, buttonId, &isButtonPressed);

    ButtonState& buttonState = *this->ButtonsState[buttonId];
    buttonState = isButtonPressed
      ? buttonState != vtkZSpaceSDKManager::Pressed ? vtkZSpaceSDKManager::Down
                                                    : vtkZSpaceSDKManager::Pressed
      : buttonState != vtkZSpaceSDKManager::None ? vtkZSpaceSDKManager::Up
                                                 : vtkZSpaceSDKManager::None;
  }
}

//------------------------------------------------------------------------------
void vtkZSpaceCoreCompatibilitySDKManager::CalculateFrustumFit(
  const double bounds[6], double position[3], double viewUp[3])
{
  if (!this->Initialized)
  {
    return;
  }

  ZCCompatError error;

  vtkBoundingBox bBox;
  bBox.SetBounds(bounds);

  // Expand the bounding box a little bit to make sure the object is not clipped
  bBox.Scale(1.5, 1.5, 1.5);

  // Retrieve viewport size (pixels)
  ZSInt32 viewportResWidth = 0;
  ZSInt32 viewportResHeight = 0;
  error = this->EntryPts.zccompatGetViewportSize(
    this->ViewportHandle, &viewportResWidth, &viewportResHeight);
  ZSPACE_CHECK_ERROR(zccompatGetViewportSize, error);

  // Retrieve display size (meters)
  ZSFloat displayWidth = 0.0f;
  ZSFloat displayHeight = 0.0f;
  error = this->EntryPts.zccompatGetDisplaySize(this->DisplayHandle, &displayWidth, &displayHeight);
  ZSPACE_CHECK_ERROR(zccompatGetDisplaySize, error);

  // Retrieve display resolution
  ZSInt32 displayResWidth = 0;
  ZSInt32 displayResHeight = 0;
  error = this->EntryPts.zccompatGetDisplayNativeResolution(
    this->DisplayHandle, &displayResWidth, &displayResHeight);

  // Retrieve coupled zone maximum depth value for positive parallax
  ZSFloat ppMaxDepth = 0.0f;
  error = this->EntryPts.zccompatGetFrustumAttributeF32(
    this->FrustumHandle, ZC_COMPAT_FRUSTUM_ATTRIBUTE_UC_DEPTH, &ppMaxDepth);
  ZSPACE_CHECK_ERROR(zccompatGetFrustumAttributeF32, error);

  // Retrieve coupled zone maximum depth value for negative parallax
  ZSFloat npMaxDepth = 0.0f;
  error = this->EntryPts.zccompatGetFrustumAttributeF32(
    this->FrustumHandle, ZC_COMPAT_FRUSTUM_ATTRIBUTE_CC_DEPTH, &npMaxDepth);
  ZSPACE_CHECK_ERROR(zccompatGetFrustumAttributeF32, error);

  // Compute viewport size in meters
  float viewportWidth = (static_cast<float>(viewportResWidth) / displayResWidth) * displayWidth;
  float viewportHeight = (static_cast<float>(viewportResHeight) / displayResHeight) * displayHeight;

  double length[3] = { 0, 0, 0 };
  bBox.GetLengths(length);

  // Compute viewer scale as the maximum of widthScale, heightScale, and depthScale.
  double widthScale = length[0] / viewportWidth;
  double heightScale = length[1] / viewportHeight;
  double depthScale = length[2] / (npMaxDepth - ppMaxDepth);

  this->ViewerScale = std::max({ depthScale, widthScale, heightScale });

  // Get frustum's camera offset (distance to world center)
  ZSVector3 zsCameraOffset = { 0.0, 0.0, 0.0 };
  error = this->EntryPts.zccompatGetFrustumCameraOffset(this->FrustumHandle, &zsCameraOffset);
  ZSPACE_CHECK_ERROR(zccompatGetFrustumCameraOffset, error);

  // Compute new frustum's camera viewUp and position
  double center[3] = { 0.0, 0.0, 0.0 };
  bBox.GetCenter(center);

  vtkVector3d worldCenter = { center[0], center[1], center[2] };
  vtkVector3d cameraOffset = { zsCameraOffset.x, zsCameraOffset.y, zsCameraOffset.z };
  vtkVector3d cameraForward = (-cameraOffset).Normalized();
  vtkVector3d cameraRight = { 1.0f, 0.0f, 0.0f };
  vtkVector3d cameraUp = cameraRight.Cross(cameraForward);
  vtkVector3d cameraPosition =
    worldCenter - (cameraForward * cameraOffset.Norm() * this->ViewerScale);

  viewUp[0] = cameraUp.GetX();
  viewUp[1] = cameraUp.GetY();
  viewUp[2] = cameraUp.GetZ();

  position[0] = cameraPosition.GetX();
  position[1] = cameraPosition.GetY();
  position[2] = cameraPosition.GetZ();

  // Set the frustum's viewer scale with the value that was calculated.
  error = this->EntryPts.zccompatSetFrustumAttributeF32(
    this->FrustumHandle, ZC_COMPAT_FRUSTUM_ATTRIBUTE_VIEWER_SCALE, this->ViewerScale);
  ZSPACE_CHECK_ERROR(zccompatSetFrustumAttributeF32, error);
}

//------------------------------------------------------------------------------
void vtkZSpaceCoreCompatibilitySDKManager::BeginFrame()
{
  if (!this->Initialized)
  {
    return;
  }

  ZCCompatError error = this->EntryPts.zccompatBeginFrame(this->ZSpaceContext);
  ZSPACE_CHECK_ERROR(zccompatBeginFrame, error);
}

//------------------------------------------------------------------------------
void vtkZSpaceCoreCompatibilitySDKManager::EndFrame()
{
  if (!this->Initialized)
  {
    return;
  }

  ZCCompatError error = this->EntryPts.zccompatEndFrame(this->ZSpaceContext);
  ZSPACE_CHECK_ERROR(zccompatEndFrame, error);
}

//------------------------------------------------------------------------------
void vtkZSpaceCoreCompatibilitySDKManager::SetRenderWindow(vtkRenderWindow* renderWindow)
{
  if (!this->Initialized)
  {
    return;
  }
  // Give the application window handle to the zSpace Core Compatibilty API.
  HWND hWnd = static_cast<HWND>(renderWindow->GetGenericWindowId());
  ZCCompatError error =
    this->EntryPts.zccompatSetApplicationWindowHandle(this->ZSpaceContext, hWnd);
  ZSPACE_CHECK_ERROR(zccompatSetApplicationWindowHandle, error);

  this->Superclass::SetRenderWindow(renderWindow);
}

//------------------------------------------------------------------------------
void vtkZSpaceCoreCompatibilitySDKManager::ConvertAndTransposeZSpaceMatrixToVTKMatrix(
  ZSMatrix4 zSpaceMatrix, vtkMatrix4x4* vtkMatrix)
{
  for (int i = 0; i < 16; ++i)
  {
    vtkMatrix->SetElement(i % 4, i / 4, zSpaceMatrix.f[i]);
  }
}

//------------------------------------------------------------------------------
void vtkZSpaceCoreCompatibilitySDKManager::ConvertZSpaceMatrixToVTKMatrix(
  ZSMatrix4 zSpaceMatrix, vtkMatrix4x4* vtkMatrix)
{
  for (int i = 0; i < 16; ++i)
  {
    vtkMatrix->SetElement(i / 4, i % 4, zSpaceMatrix.f[i]);
  }
}

//------------------------------------------------------------------------------
void vtkZSpaceCoreCompatibilitySDKManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

VTK_ABI_NAMESPACE_END
