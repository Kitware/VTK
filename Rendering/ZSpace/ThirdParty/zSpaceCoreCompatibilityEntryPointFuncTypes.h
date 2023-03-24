//////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2022 zSpace, Inc.  All Rights Reserved.
//
//////////////////////////////////////////////////////////////////////////

#ifndef __ZSPACE_CORE_COMPATIBILITY_ENTRY_POINT_FUNC_TYPES_H__
#define __ZSPACE_CORE_COMPATIBILITY_ENTRY_POINT_FUNC_TYPES_H__

#if defined(_WIN32)
# include <dxgiformat.h>
#endif

#include "zSpaceTypes.h"

#include "zSpaceCoreCompatibilityPlatformDefines.h"
#include "zSpaceCoreCompatibilityTypes.h"


#if defined(_WIN32)
// Forward declare some Direct3D 11 structs so that the Direct3D 11 headers
// (which include many other things) do not need to be included here.
struct ID3D11Device;
struct ID3D11Texture2D;
#endif

#if defined(__cplusplus)
extern "C" {
#endif

// General API

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatInitializeFuncPtrType)(
        ZCCompatLogFuncFuncPtrType logFunc,
        void* logFuncUserData,
        ZCCompatContext* context);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatUpdateFuncPtrType)(
        ZCCompatContext context);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatShutDownFuncPtrType)(
        ZCCompatContext context);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatGetRuntimeVersionFuncPtrType)(
        ZCCompatContext context,
        ZSInt32* major,
        ZSInt32* minor,
        ZSInt32* patch);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatSetTrackingEnabledFuncPtrType)(
        ZCCompatContext context,
        ZSBool isEnabled);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatIsTrackingEnabledFuncPtrType)(
        ZCCompatContext context,
        ZSBool* isEnabled);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatGetStereoDisplayModeFuncPtrType)(
        ZCCompatContext context,
        ZCCompatStereoDisplayMode* stereoDisplayMode);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatSetApplicationWindowHandleFuncPtrType)(
        ZCCompatContext context,
        void* windowHandle);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatGetApplicationWindowHandleFuncPtrType)(
        ZCCompatContext context,
        void** windowHandle);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatSetStereoDisplayEnabledFuncPtrType)(
        ZCCompatContext context,
        ZSBool isEnabled);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatIsStereoDisplayEnabledFuncPtrType)(
        ZCCompatContext context,
        ZSBool* isEnabled);

// Display API

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatRefreshDisplaysFuncPtrType)(
        ZCCompatContext context);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatGetNumDisplaysFuncPtrType)(
        ZCCompatContext context,
        ZSInt32* numDisplays);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatGetNumDisplaysByTypeFuncPtrType)(
        ZCCompatContext context,
        ZCCompatDisplayType displayType,
        ZSInt32* numDisplays);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatGetDisplayFuncPtrType)(
        ZCCompatContext context,
        ZSInt32 x,
        ZSInt32 y,
        ZCCompatDisplay* display);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatGetDisplayByIndexFuncPtrType)(
        ZCCompatContext context,
        ZSInt32 index,
        ZCCompatDisplay* display);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatGetDisplayByTypeFuncPtrType)(
        ZCCompatContext context,
        ZCCompatDisplayType displayType,
        ZSInt32 index,
        ZCCompatDisplay* display);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatGetDisplayTypeFuncPtrType)(
        ZCCompatDisplay display,
        ZCCompatDisplayType* displayType);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatGetDisplayNumberFuncPtrType)(
        ZCCompatDisplay display,
        ZSInt32* number);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatGetDisplayAdapterIndexFuncPtrType)(
        ZCCompatDisplay display,
        ZSInt32* adapterIndex);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatGetDisplayAttributeStrFuncPtrType)(
        ZCCompatDisplay display,
        ZCCompatDisplayAttribute attribute,
        char* buffer,
        ZSInt32 bufferSize);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatGetDisplayAttributeStrSizeFuncPtrType)(
        ZCCompatDisplay display,
        ZCCompatDisplayAttribute attribute,
        ZSInt32* size);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatGetDisplaySizeFuncPtrType)(
        ZCCompatDisplay display,
        ZSFloat* width,
        ZSFloat* height);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatGetDisplayPositionFuncPtrType)(
        ZCCompatDisplay display,
        ZSInt32* x,
        ZSInt32* y);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatGetDisplayNativeResolutionFuncPtrType)(
        ZCCompatDisplay display,
        ZSInt32* x,
        ZSInt32* y);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatGetDisplayAngleFuncPtrType)(
        ZCCompatDisplay display,
        ZSFloat* x,
        ZSFloat* y,
        ZSFloat* z);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatGetDisplayVerticalRefreshRateFuncPtrType)(
        ZCCompatDisplay display,
        ZSFloat* refreshRate);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatIntersectDisplayFuncPtrType)(
        ZCCompatDisplay display,
        const ZCCompatTrackerPose* pose,
        ZCCompatDisplayIntersectionInfo* intersectionInfo);

// Viewport API

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatGetPrimaryViewportFuncPtrType)(
        ZCCompatContext context,
        ZCCompatViewport* viewport);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatCreateViewportFuncPtrType)(
        ZCCompatContext context,
        ZCCompatViewport* viewport);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatDestroyViewportFuncPtrType)(
        ZCCompatViewport viewport);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatSetViewportPositionFuncPtrType)(
        ZCCompatViewport viewport,
        ZSInt32 x,
        ZSInt32 y);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatGetViewportPositionFuncPtrType)(
        ZCCompatViewport viewport,
        ZSInt32* x,
        ZSInt32* y);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatSetViewportSizeFuncPtrType)(
        ZCCompatViewport viewport,
        ZSInt32 width,
        ZSInt32 height);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatGetViewportSizeFuncPtrType)(
        ZCCompatViewport viewport,
        ZSInt32* width,
        ZSInt32* height);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatGetCoordinateSpaceTransformFuncPtrType)(
        ZCCompatViewport viewport,
        ZCCompatCoordinateSpace a,
        ZCCompatCoordinateSpace b,
        ZSMatrix4* transform);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatTransformMatrixFuncPtrType)(
        ZCCompatViewport viewport,
        ZCCompatCoordinateSpace a,
        ZCCompatCoordinateSpace b,
        ZSMatrix4* matrix);

// Frustum API

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatGetFrustumFuncPtrType)(
        ZCCompatViewport viewport, ZCCompatFrustum* frustum);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatSetFrustumAttributeF32FuncPtrType)(
        ZCCompatFrustum frustum,
        ZCCompatFrustumAttribute attribute,
        ZSFloat value);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatGetFrustumAttributeF32FuncPtrType)(
        ZCCompatFrustum frustum,
        ZCCompatFrustumAttribute attribute,
        ZSFloat* value);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatSetFrustumAttributeBFuncPtrType)(
        ZCCompatFrustum frustum,
        ZCCompatFrustumAttribute attribute,
        ZSBool value);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatGetFrustumAttributeBFuncPtrType)(
        ZCCompatFrustum frustum,
        ZCCompatFrustumAttribute attribute,
        ZSBool* value);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatSetFrustumPortalModeFuncPtrType)(
        ZCCompatFrustum frustum,
        ZSInt32 portalModeFlags);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatGetFrustumPortalModeFuncPtrType)(
        ZCCompatFrustum frustum,
        ZSInt32* portalModeFlags);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatSetFrustumCameraOffsetFuncPtrType)(
        ZCCompatFrustum frustum,
        const ZSVector3* cameraOffset);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatGetFrustumCameraOffsetFuncPtrType)(
        ZCCompatFrustum frustum,
        ZSVector3* cameraOffset);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatSetFrustumTrackerSpaceEyePosesFuncPtrType)(
        ZCCompatFrustum frustum,
        const ZCCompatTrackerPose* leftEyePose,
        const ZCCompatTrackerPose* rightEyePose);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatGetFrustumTrackerSpaceEyePosesFuncPtrType)(
        ZCCompatFrustum frustum,
        ZCCompatTrackerPose* leftEyePose,
        ZCCompatTrackerPose* rightEyePose);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatGetFrustumViewMatrixFuncPtrType)(
        ZCCompatFrustum frustum,
        ZCCompatEye eye,
        ZSMatrix4* viewMatrix);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatGetFrustumProjectionMatrixFuncPtrType)(
        ZCCompatFrustum frustum,
        ZCCompatEye eye,
        ZSMatrix4* projectionMatrix);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatGetFrustumBoundsFuncPtrType)(
        ZCCompatFrustum frustum,
        ZCCompatEye eye,
        ZCCompatFrustumBounds* bounds);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatGetFrustumEyePositionFuncPtrType)(
        ZCCompatFrustum frustum,
        ZCCompatEye eye,
        ZCCompatCoordinateSpace coordinateSpace,
        ZSVector3* eyePosition);

// Target API

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatGetNumTargetsByTypeFuncPtrType)(
        ZCCompatContext context,
        ZCCompatTargetType targetType,
        ZSInt32* numTargets);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatGetTargetByTypeFuncPtrType)(
        ZCCompatContext context,
        ZCCompatTargetType targetType,
        ZSInt32 index,
        ZCCompatTarget* target);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatGetTargetNameFuncPtrType)(
        ZCCompatTarget target,
        char* buffer,
        ZSInt32 bufferSize);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatGetTargetNameSizeFuncPtrType)(
        ZCCompatTarget target,
        ZSInt32* size);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatSetTargetEnabledFuncPtrType)(
        ZCCompatTarget target,
        ZSBool isEnabled);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatIsTargetEnabledFuncPtrType)(
        ZCCompatTarget target,
        ZSBool* isEnabled);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatIsTargetVisibleFuncPtrType)(
        ZCCompatTarget target,
        ZSBool* isVisible);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatGetTargetPoseFuncPtrType)(
        ZCCompatTarget target,
        ZCCompatTrackerPose* pose);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatGetNumTargetButtonsFuncPtrType)(
        ZCCompatTarget target,
        ZSInt32* numButtons);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatIsTargetButtonPressedFuncPtrType)(
        ZCCompatTarget target,
        ZSInt32 buttonId,
        ZSBool* isButtonPressed);

// Target LED API

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatSetTargetLedEnabledFuncPtrType)(
        ZCCompatTarget target,
        ZSBool isLedEnabled);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatIsTargetLedEnabledFuncPtrType)(
        ZCCompatTarget target,
        ZSBool* isLedEnabled);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatSetTargetLedColorFuncPtrType)(
        ZCCompatTarget target,
        ZSFloat r,
        ZSFloat g,
        ZSFloat b);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatGetTargetLedColorFuncPtrType)(
        ZCCompatTarget target,
        ZSFloat* r,
        ZSFloat* g,
        ZSFloat* b);

// Target Vibration API

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatSetTargetVibrationEnabledFuncPtrType)(
        ZCCompatTarget target,
        ZSBool isVibrationEnabled);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatIsTargetVibrationEnabledFuncPtrType)(
        ZCCompatTarget target,
        ZSBool* isVibrationEnabled);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatIsTargetVibratingFuncPtrType)(
        ZCCompatTarget target,
        ZSBool* isVibrating);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatStartTargetVibrationFuncPtrType)(
        ZCCompatTarget target,
        ZSFloat onPeriod,
        ZSFloat offPeriod,
        ZSInt32 numTimes,
        ZSFloat intensity);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatStopTargetVibrationFuncPtrType)(
        ZCCompatTarget target);

// Target Tap API

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatIsTargetTapPressedFuncPtrType)(
        ZCCompatTarget target,
        ZSBool* isTapPressed);

// Mouse Emulation API

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatSetMouseEmulationEnabledFuncPtrType)(
        ZCCompatContext context,
        ZSBool isEnabled);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatIsMouseEmulationEnabledFuncPtrType)(
        ZCCompatContext context,
        ZSBool* isEnabled);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatSetMouseEmulationTargetFuncPtrType)(
        ZCCompatContext context,
        ZCCompatTarget target);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatGetMouseEmulationTargetFuncPtrType)(
        ZCCompatContext context,
        ZCCompatTarget* target);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatSetMouseEmulationMovementModeFuncPtrType)(
        ZCCompatContext context,
        ZCCompatMouseMovementMode movementMode);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatGetMouseEmulationMovementModeFuncPtrType)(
        ZCCompatContext context,
        ZCCompatMouseMovementMode* movementMode);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatSetMouseEmulationMaxDistanceFuncPtrType)(
        ZCCompatContext context,
        ZSFloat maxDistance);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatGetMouseEmulationMaxDistanceFuncPtrType)(
        ZCCompatContext context,
        ZSFloat* maxDistance);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatSetMouseEmulationButtonMappingFuncPtrType)(
        ZCCompatContext context,
        ZSInt32 buttonId,
        ZCCompatMouseButton mouseButton);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatGetMouseEmulationButtonMappingFuncPtrType)(
        ZCCompatContext context,
        ZSInt32 buttonId,
        ZCCompatMouseButton* mouseButton);

// Stereo Frame Display API

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatGetPerEyeImageResolutionFuncPtrType)(
        ZCCompatContext context,
        ZSInt32* width,
        ZSInt32* height);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatBeginFrameFuncPtrType)(
        ZCCompatContext context);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatEndFrameFuncPtrType)(
        ZCCompatContext context);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatIsAnyGraphicsBindingEnabledFuncPtrType)(
        ZCCompatContext context,
        ZSBool* isEnabled);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *
            ZCCompatEnableGraphicsBindingDirect3D11FuncPtrType)(
        ZCCompatContext context,
        ID3D11Device* d3d11Device);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatSubmitFrameDirect3D11FuncPtrType)(
        ZCCompatContext context,
        ID3D11Texture2D* leftEyeTexture,
        ID3D11Texture2D* rightEyeTexture,
        DXGI_FORMAT eyeTextureShaderResourceViewFormat,
        ZSBool isEyeTextureRowOrderFlipped);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR * ZCCompatEnableGraphicsBindingOpenGLFuncPtrType)(
        ZCCompatContext context);

typedef ZCCompatError
    (ZC_COMPAT_API_FUNC_PTR *ZCCompatSubmitFrameOpenGLFuncPtrType)(
        ZCCompatContext context,
        ZSUInt32 leftEyeTexture,
        ZSUInt32 rightEyeTexture,
        ZSBool isEyeTextureRowOrderFlipped);

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // __ZSPACE_CORE_COMPATIBILITY_ENTRY_POINT_FUNC_TYPES_H__
