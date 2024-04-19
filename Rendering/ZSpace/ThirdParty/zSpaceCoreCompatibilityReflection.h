//////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2022 zSpace, Inc.  All Rights Reserved.
//
//////////////////////////////////////////////////////////////////////////

#ifndef __ZSPACE_CORE_COMPATIBILITY_REFLECTION_H__
#define __ZSPACE_CORE_COMPATIBILITY_REFLECTION_H__


#define ZC_COMPAT_REFLECTION_LIST_UNDECORATED_FUNC_NAMES(_) \
    _(Initialize) \
    _(Update) \
    _(ShutDown) \
    _(GetRuntimeVersion) \
    _(SetTrackingEnabled) \
    _(IsTrackingEnabled) \
    _(GetStereoDisplayMode) \
    _(SetApplicationWindowHandle) \
    _(GetApplicationWindowHandle) \
    _(SetStereoDisplayEnabled) \
    _(IsStereoDisplayEnabled) \
    _(RefreshDisplays) \
    _(GetNumDisplays) \
    _(GetNumDisplaysByType) \
    _(GetDisplay) \
    _(GetDisplayByIndex) \
    _(GetDisplayByType) \
    _(GetDisplayType) \
    _(GetDisplayNumber) \
    _(GetDisplayAdapterIndex) \
    _(GetDisplayAttributeStr) \
    _(GetDisplayAttributeStrSize) \
    _(GetDisplaySize) \
    _(GetDisplayPosition) \
    _(GetDisplayNativeResolution) \
    _(GetDisplayAngle) \
    _(GetDisplayVerticalRefreshRate) \
    _(IntersectDisplay) \
    _(GetPrimaryViewport) \
    _(CreateViewport) \
    _(DestroyViewport) \
    _(SetViewportPosition) \
    _(GetViewportPosition) \
    _(SetViewportSize) \
    _(GetViewportSize) \
    _(GetCoordinateSpaceTransform) \
    _(TransformMatrix) \
    _(GetFrustum) \
    _(SetFrustumAttributeF32) \
    _(GetFrustumAttributeF32) \
    _(SetFrustumAttributeB) \
    _(GetFrustumAttributeB) \
    _(SetFrustumPortalMode) \
    _(GetFrustumPortalMode) \
    _(SetFrustumCameraOffset) \
    _(GetFrustumCameraOffset) \
    _(SetFrustumTrackerSpaceEyePoses) \
    _(GetFrustumTrackerSpaceEyePoses) \
    _(GetFrustumViewMatrix) \
    _(GetFrustumProjectionMatrix) \
    _(GetFrustumBounds) \
    _(GetFrustumEyePosition) \
    _(GetNumTargetsByType) \
    _(GetTargetByType) \
    _(GetTargetName) \
    _(GetTargetNameSize) \
    _(SetTargetEnabled) \
    _(IsTargetEnabled) \
    _(IsTargetVisible) \
    _(GetTargetPose) \
    _(GetNumTargetButtons) \
    _(IsTargetButtonPressed) \
    _(SetTargetLedEnabled) \
    _(IsTargetLedEnabled) \
    _(SetTargetLedColor) \
    _(GetTargetLedColor) \
    _(SetTargetVibrationEnabled) \
    _(IsTargetVibrationEnabled) \
    _(IsTargetVibrating) \
    _(StartTargetVibration) \
    _(StopTargetVibration) \
    _(IsTargetTapPressed) \
    _(SetMouseEmulationEnabled) \
    _(IsMouseEmulationEnabled) \
    _(SetMouseEmulationTarget) \
    _(GetMouseEmulationTarget) \
    _(SetMouseEmulationMovementMode) \
    _(GetMouseEmulationMovementMode) \
    _(SetMouseEmulationMaxDistance) \
    _(GetMouseEmulationMaxDistance) \
    _(SetMouseEmulationButtonMapping) \
    _(GetMouseEmulationButtonMapping) \
    _(GetPerEyeImageResolution) \
    _(BeginFrame) \
    _(EndFrame) \
    _(IsAnyGraphicsBindingEnabled) \
    _(EnableGraphicsBindingDirect3D11) \
    _(SubmitFrameDirect3D11) \
    _(EnableGraphicsBindingOpenGL) \
    _(SubmitFrameOpenGL) \
    /**/

#endif // __ZSPACE_CORE_COMPATIBILITY_REFLECTION_H__
