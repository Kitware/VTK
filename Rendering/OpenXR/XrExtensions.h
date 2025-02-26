// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Microsoft
// SPDX-License-Identifier: BSD-3-Clause AND Apache-2.0
/**
 * @file   XrExtensions.h
 *
 * @brief  Load OpenXR extensions common to all platforms and graphics backend.
 *
 * Provides the ExtensionDispatchTable struct to load extension function
 * pointers at runtime for the current XrInstance.
 *
 * File adapted from:
 * https://github.com/microsoft/MixedReality-HolographicRemoting-Samples/blob/f6b55479646bda3bffea58bb3e9c9d9c5e0ab177/remote_openxr/desktop/XrUtility/XrExtensions.h
 *
 * @sa
 * vtkOpenXr.h XrGraphicsExtensions.h XrConnectionExtensions.h
 */

#ifndef XrExtensions_h
#define XrExtensions_h

#if XR_KHR_visibility_mask
#define FOR_EACH_VISIBILITY_MASK_FUNCTION(_) _(xrGetVisibilityMaskKHR)
#else
#define FOR_EACH_VISIBILITY_MASK_FUNCTION(_)
#endif

#ifdef XR_MSFT_controller_model
#define FOR_EACH_CONTROLLER_MODEL_EXTENSION_FUNCTION(_)                                            \
  _(xrGetControllerModelKeyMSFT)                                                                   \
  _(xrLoadControllerModelMSFT)                                                                     \
  _(xrGetControllerModelPropertiesMSFT)                                                            \
  _(xrGetControllerModelStateMSFT)
#else
#define FOR_EACH_CONTROLLER_MODEL_EXTENSION_FUNCTION(_)
#endif

#if XR_MSFT_perception_anchor_interop && defined(XR_USE_PLATFORM_WIN32)
#define FOR_EACH_PERCEPTION_ANCHOR_INTEROP_FUNCTION(_)                                             \
  _(xrCreateSpatialAnchorFromPerceptionAnchorMSFT)                                                 \
  _(xrTryGetPerceptionAnchorFromSpatialAnchorMSFT)
#else
#define FOR_EACH_PERCEPTION_ANCHOR_INTEROP_FUNCTION(_)
#endif

#if XR_MSFT_spatial_anchor
#define FOR_EACH_SPATIAL_ANCHOR_FUNCTION(_)                                                        \
  _(xrCreateSpatialAnchorMSFT)                                                                     \
  _(xrCreateSpatialAnchorSpaceMSFT)                                                                \
  _(xrDestroySpatialAnchorMSFT)
#else
#define FOR_EACH_SPATIAL_ANCHOR_FUNCTION(_)
#endif

#if XR_EXT_hand_tracking
#define FOR_EACH_HAND_TRACKING_FUNCTION(_)                                                         \
  _(xrCreateHandTrackerEXT)                                                                        \
  _(xrDestroyHandTrackerEXT)                                                                       \
  _(xrLocateHandJointsEXT)
#else
#define FOR_EACH_HAND_TRACKING_FUNCTION(_)
#endif

#if XR_MSFT_hand_tracking_mesh
#define FOR_EACH_HAND_TRACKING_MESH_FUNCTION(_)                                                    \
  _(xrCreateHandMeshSpaceMSFT)                                                                     \
  _(xrUpdateHandMeshMSFT)
#else
#define FOR_EACH_HAND_TRACKING_MESH_FUNCTION(_)
#endif

#if XR_MSFT_spatial_graph_bridge
#define FOR_EACH_SPATIAL_GRAPH_BRIDGE_FUNCTION(_) _(xrCreateSpatialGraphNodeSpaceMSFT)
#else
#define FOR_EACH_SPATIAL_GRAPH_BRIDGE_FUNCTION(_)
#endif

#if XR_MSFT_scene_understanding
#define FOR_EACH_SCENE_UNDERSTANDING_FUNCTION(_)                                                   \
  _(xrCreateSceneObserverMSFT)                                                                     \
  _(xrDestroySceneObserverMSFT)                                                                    \
  _(xrCreateSceneMSFT)                                                                             \
  _(xrDestroySceneMSFT)                                                                            \
  _(xrComputeNewSceneMSFT)                                                                         \
  _(xrGetSceneComputeStateMSFT)                                                                    \
  _(xrGetSceneComponentsMSFT)                                                                      \
  _(xrLocateSceneComponentsMSFT)                                                                   \
  _(xrGetSceneMeshBuffersMSFT)                                                                     \
  _(xrEnumerateSceneComputeFeaturesMSFT)
#else
#define FOR_EACH_SCENE_UNDERSTANDING_FUNCTION(_)
#endif

#if XR_MSFT_scene_marker
#define FOR_EACH_SCENE_MARKER_FUNCTION(_)                                                          \
  _(xrGetSceneMarkerRawDataMSFT)                                                                   \
  _(xrGetSceneMarkerDecodedStringMSFT)
#else
#define FOR_EACH_SCENE_MARKER_FUNCTION(_)
#endif

#if XR_MSFT_scene_understanding_serialization_preview
#define FOR_EACH_SCENE_UNDERSTANDING_SERIALIZATION_FUNCTION(_)                                     \
  _(xrDeserializeSceneMSFT)                                                                        \
  _(xrGetSceneSerializedDataMSFT)

#else
#define FOR_EACH_SCENE_UNDERSTANDING_SERIALIZATION_FUNCTION(_)
#endif

#define FOR_EACH_EXTENSION_FUNCTION(_)                                                             \
  FOR_EACH_VISIBILITY_MASK_FUNCTION(_)                                                             \
  FOR_EACH_HAND_TRACKING_FUNCTION(_)                                                               \
  FOR_EACH_HAND_TRACKING_MESH_FUNCTION(_)                                                          \
  FOR_EACH_SPATIAL_GRAPH_BRIDGE_FUNCTION(_)                                                        \
  FOR_EACH_SPATIAL_ANCHOR_FUNCTION(_)                                                              \
  FOR_EACH_CONTROLLER_MODEL_EXTENSION_FUNCTION(_)                                                  \
  FOR_EACH_PERCEPTION_ANCHOR_INTEROP_FUNCTION(_)                                                   \
  FOR_EACH_SCENE_UNDERSTANDING_FUNCTION(_)                                                         \
  FOR_EACH_SCENE_UNDERSTANDING_SERIALIZATION_FUNCTION(_)                                           \
  FOR_EACH_SCENE_MARKER_FUNCTION(_)

#define GET_INSTANCE_PROC_ADDRESS(name)                                                            \
  (void)xrGetInstanceProcAddr(                                                                     \
    instance, #name, reinterpret_cast<PFN_xrVoidFunction*>(const_cast<PFN_##name*>(&name)));
#define DEFINE_PROC_MEMBER(name) PFN_##name name{ nullptr };

namespace xr
{
VTK_ABI_NAMESPACE_BEGIN
struct ExtensionDispatchTable
{
  FOR_EACH_EXTENSION_FUNCTION(DEFINE_PROC_MEMBER);

  ExtensionDispatchTable() = default;
  void PopulateDispatchTable(XrInstance instance)
  {
    FOR_EACH_EXTENSION_FUNCTION(GET_INSTANCE_PROC_ADDRESS);
  }
};
VTK_ABI_NAMESPACE_END
} // namespace xr

#undef DEFINE_PROC_MEMBER
#undef GET_INSTANCE_PROC_ADDRESS
#undef FOR_EACH_EXTENSION_FUNCTION

#endif
