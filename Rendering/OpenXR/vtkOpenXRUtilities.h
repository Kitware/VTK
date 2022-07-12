/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenXRUtilities.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOpenXRUtilities
 //----------------------------------------------------------------------------
 * @brief   Header file that contains utility functions for OpenXR
 *
 * This class contains inline functions to create matrices from OpenXR pose
 * And other functions to convert OpenXR structures to string.
 *
 * vtkOpenXRUtilities
 */

#ifndef vtkOpenXRUtilities_h
#define vtkOpenXRUtilities_h

#include "vtkMatrix4x4.h"
#include "vtkOpenXR.h"

class vtkOpenXRUtilities : public vtkObject
{
public:
  static vtkOpenXRUtilities* New();
  vtkTypeMacro(vtkOpenXRUtilities, vtkObject);

  ///@{
  /**
   * Given a field of view (left, right, up, down angles) and near/far
   * clipping builds, return a projection matrix in result.
   */
  static void CreateProjectionFov(
    vtkMatrix4x4* result, const XrFovf fov, const float nearZ, const float farZ);
  ///@}

  ///@{
  /**
   * Given an XR pose (orientation quaternion + position vector), set the provided
   * matrix from it.
   */
  static void SetMatrixFromXrPose(vtkMatrix4x4* result, const XrPosef& xrPose);
  ///@}

  ///@{
  /**
   * Given a quaternion, return elements of equivalent matrix as an array.
   */
  static void CreateFromQuaternion(double result[16], const XrQuaternionf& quat);
  ///@}

  ///@{
  /**
   * Return string representation of given XrActionType.
   */
  static const char* GetActionTypeAsString(const XrActionType& actionType);
  ///@}

  ///@{
  /**
   * Return string representation of given XrViewConfigurationType.
   */
  static const char* GetViewConfigurationTypeAsString(
    const XrViewConfigurationType& viewConfigType);
  ///@}

  ///@{
  /**
   * Return string representation of given XrStructureType.
   */
  static const char* GetStructureTypeAsString(const XrStructureType& structureType);
  ///@}

  static const XrPosef& GetIdentityPose();

protected:
  vtkOpenXRUtilities() = default;
  ~vtkOpenXRUtilities() override = default;

private:
  vtkOpenXRUtilities(const vtkOpenXRUtilities&) = delete;
  void operator=(const vtkOpenXRUtilities&) = delete;
};

//----------------------------------------------------------------------------
inline const XrPosef& vtkOpenXRUtilities::GetIdentityPose()
{
  static const XrPosef pose = {
    { 0.0, 0.0, 0.0, 1.0 }, // .orientation
    { 0.0, 0.0, 0.0 }       // .position
  };
  return pose;
}

//----------------------------------------------------------------------------
inline void vtkOpenXRUtilities::CreateProjectionFov(
  vtkMatrix4x4* result, const XrFovf fov, const float nearZ, const float farZ)
{
  const float tanAngleLeft = tanf(fov.angleLeft);
  const float tanAngleRight = tanf(fov.angleRight);

  const float tanAngleDown = tanf(fov.angleDown);
  const float tanAngleUp = tanf(fov.angleUp);

  const float tanAngleWidth = tanAngleRight - tanAngleLeft;

  // Clip space with
  // positive Y up (OpenGL / D3D / Metal).
  const float tanAngleHeight = (tanAngleUp - tanAngleDown);

  double matrixArray[16] = { 0 };
  if (farZ <= nearZ)
  {
    // place the far plane at infinity
    matrixArray[0] = 2 / tanAngleWidth;
    matrixArray[4] = 0;
    matrixArray[8] = (tanAngleRight + tanAngleLeft) / tanAngleWidth;
    matrixArray[12] = 0;

    matrixArray[1] = 0;
    matrixArray[5] = 2 / tanAngleHeight;
    matrixArray[9] = (tanAngleUp + tanAngleDown) / tanAngleHeight;
    matrixArray[13] = 0;

    matrixArray[2] = 0;
    matrixArray[6] = 0;
    matrixArray[10] = -1;
    matrixArray[14] = -(nearZ + nearZ);

    matrixArray[3] = 0;
    matrixArray[7] = 0;
    matrixArray[11] = -1;
    matrixArray[15] = 0;
  }
  else
  {
    // normal projection
    matrixArray[0] = 2 / tanAngleWidth;
    matrixArray[4] = 0;
    matrixArray[8] = (tanAngleRight + tanAngleLeft) / tanAngleWidth;
    matrixArray[12] = 0;

    matrixArray[1] = 0;
    matrixArray[5] = 2 / tanAngleHeight;
    matrixArray[9] = (tanAngleUp + tanAngleDown) / tanAngleHeight;
    matrixArray[13] = 0;

    matrixArray[2] = 0;
    matrixArray[6] = 0;
    matrixArray[10] = -(farZ + nearZ) / (farZ - nearZ);
    matrixArray[14] = -2 * farZ * nearZ / (farZ - nearZ);

    matrixArray[3] = 0;
    matrixArray[7] = 0;
    matrixArray[11] = -1;
    matrixArray[15] = 0;
  }

  // Set the array to the result vtkMatrix
  result->DeepCopy(matrixArray);
  result->Transpose();
}

//----------------------------------------------------------------------------
// transpose of vtk standard
inline void vtkOpenXRUtilities::CreateFromQuaternion(double result[16], const XrQuaternionf& quat)
{
  const double x2 = quat.x + quat.x;
  const double y2 = quat.y + quat.y;
  const double z2 = quat.z + quat.z;

  const double xx2 = quat.x * x2;
  const double yy2 = quat.y * y2;
  const double zz2 = quat.z * z2;

  const double yz2 = quat.y * z2;
  const double wx2 = quat.w * x2;
  const double xy2 = quat.x * y2;
  const double wz2 = quat.w * z2;
  const double xz2 = quat.x * z2;
  const double wy2 = quat.w * y2;

  result[0] = 1.0 - yy2 - zz2;
  result[4] = xy2 + wz2;
  result[8] = xz2 - wy2;
  result[12] = 0.0;

  result[1] = xy2 - wz2;
  result[5] = 1.0 - xx2 - zz2;
  result[9] = yz2 + wx2;
  result[13] = 0.0;

  result[2] = xz2 + wy2;
  result[6] = yz2 - wx2;
  result[10] = 1.0 - xx2 - yy2;
  result[14] = 0.0;

  result[3] = 0.0;
  result[7] = 0.0;
  result[11] = 0.0;
  result[15] = 1.0;
}

//----------------------------------------------------------------------------
// transpose of VTK standard
inline void vtkOpenXRUtilities::SetMatrixFromXrPose(vtkMatrix4x4* result, const XrPosef& xrPose)
{
  const XrQuaternionf& xrQuat = xrPose.orientation;
  const XrVector3f& xrPos = xrPose.position;

  double* elems = result->GetData();
  vtkOpenXRUtilities::CreateFromQuaternion(elems, xrQuat);

  // Add the translation
  elems[3] = xrPos.x;
  elems[7] = xrPos.y;
  elems[11] = xrPos.z;

  result->Modified();
}

//----------------------------------------------------------------------------
inline const char* vtkOpenXRUtilities::GetActionTypeAsString(const XrActionType& actionType)
{
  switch (actionType)
  {
    case XR_ACTION_TYPE_BOOLEAN_INPUT:
      return "XR_ACTION_TYPE_BOOLEAN_INPUT";
    case XR_ACTION_TYPE_FLOAT_INPUT:
      return "XR_ACTION_TYPE_FLOAT_INPUT";
    case XR_ACTION_TYPE_VECTOR2F_INPUT:
      return "XR_ACTION_TYPE_VECTOR2F_INPUT";
    case XR_ACTION_TYPE_POSE_INPUT:
      return "XR_ACTION_TYPE_POSE_INPUT";
    case XR_ACTION_TYPE_VIBRATION_OUTPUT:
      return "XR_ACTION_TYPE_VIBRATION_OUTPUT";
    default:
      return "UNRECOGNIZED_ACTION_TYPE";
  }
}

//----------------------------------------------------------------------------
inline const char* vtkOpenXRUtilities::GetViewConfigurationTypeAsString(
  const XrViewConfigurationType& viewConfigType)
{
  switch (viewConfigType)
  {
    case XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO:
      return "XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO";
    case XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO:
      return "XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO";
    case XR_VIEW_CONFIGURATION_TYPE_PRIMARY_QUAD_VARJO:
      return "XR_VIEW_CONFIGURATION_TYPE_PRIMARY_QUAD_VARJO";
    case XR_VIEW_CONFIGURATION_TYPE_SECONDARY_MONO_FIRST_PERSON_OBSERVER_MSFT:
      return "XR_VIEW_CONFIGURATION_TYPE_SECONDARY_MONO_FIRST_PERSON_OBSERVER_MSFT";
    default:
      return "UNRECOGNIZED_VIEW_CONFIGURATION_TYPE";
  }
}

//----------------------------------------------------------------------------
inline const char* vtkOpenXRUtilities::GetStructureTypeAsString(
  const XrStructureType& structureType)
{
  switch (structureType)
  {
    case XR_TYPE_API_LAYER_PROPERTIES:
      return "XR_TYPE_API_LAYER_PROPERTIES";
    case XR_TYPE_EXTENSION_PROPERTIES:
      return "XR_TYPE_EXTENSION_PROPERTIES";
    case XR_TYPE_INSTANCE_CREATE_INFO:
      return "XR_TYPE_INSTANCE_CREATE_INFO";
    case XR_TYPE_SYSTEM_GET_INFO:
      return "XR_TYPE_SYSTEM_GET_INFO";
    case XR_TYPE_SYSTEM_PROPERTIES:
      return "XR_TYPE_SYSTEM_PROPERTIES";
    case XR_TYPE_VIEW_LOCATE_INFO:
      return "XR_TYPE_VIEW_LOCATE_INFO";
    case XR_TYPE_VIEW:
      return "XR_TYPE_VIEW";
    case XR_TYPE_SESSION_CREATE_INFO:
      return "XR_TYPE_SESSION_CREATE_INFO";
    case XR_TYPE_SWAPCHAIN_CREATE_INFO:
      return "XR_TYPE_SWAPCHAIN_CREATE_INFO";
    case XR_TYPE_SESSION_BEGIN_INFO:
      return "XR_TYPE_SESSION_BEGIN_INFO";
    case XR_TYPE_VIEW_STATE:
      return "XR_TYPE_VIEW_STATE";
    case XR_TYPE_FRAME_END_INFO:
      return "XR_TYPE_FRAME_END_INFO";
    case XR_TYPE_HAPTIC_VIBRATION:
      return "XR_TYPE_HAPTIC_VIBRATION";
    case XR_TYPE_EVENT_DATA_BUFFER:
      return "XR_TYPE_EVENT_DATA_BUFFER";
    case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING:
      return "XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING";
    case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED:
      return "XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED";
    case XR_TYPE_ACTION_STATE_BOOLEAN:
      return "XR_TYPE_ACTION_STATE_BOOLEAN";
    case XR_TYPE_ACTION_STATE_FLOAT:
      return "XR_TYPE_ACTION_STATE_FLOAT";
    case XR_TYPE_ACTION_STATE_VECTOR2F:
      return "XR_TYPE_ACTION_STATE_VECTOR2F";
    case XR_TYPE_ACTION_STATE_POSE:
      return "XR_TYPE_ACTION_STATE_POSE";
    case XR_TYPE_ACTION_SET_CREATE_INFO:
      return "XR_TYPE_ACTION_SET_CREATE_INFO";
    case XR_TYPE_ACTION_CREATE_INFO:
      return "XR_TYPE_ACTION_CREATE_INFO";
    case XR_TYPE_INSTANCE_PROPERTIES:
      return "XR_TYPE_INSTANCE_PROPERTIES";
    case XR_TYPE_FRAME_WAIT_INFO:
      return "XR_TYPE_FRAME_WAIT_INFO";
    case XR_TYPE_COMPOSITION_LAYER_PROJECTION:
      return "XR_TYPE_COMPOSITION_LAYER_PROJECTION";
    case XR_TYPE_COMPOSITION_LAYER_QUAD:
      return "XR_TYPE_COMPOSITION_LAYER_QUAD";
    case XR_TYPE_REFERENCE_SPACE_CREATE_INFO:
      return "XR_TYPE_REFERENCE_SPACE_CREATE_INFO";
    case XR_TYPE_ACTION_SPACE_CREATE_INFO:
      return "XR_TYPE_ACTION_SPACE_CREATE_INFO";
    case XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING:
      return "XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING";
    case XR_TYPE_VIEW_CONFIGURATION_VIEW:
      return "XR_TYPE_VIEW_CONFIGURATION_VIEW";
    case XR_TYPE_SPACE_LOCATION:
      return "XR_TYPE_SPACE_LOCATION";
    case XR_TYPE_SPACE_VELOCITY:
      return "XR_TYPE_SPACE_VELOCITY";
    case XR_TYPE_FRAME_STATE:
      return "XR_TYPE_FRAME_STATE";
    case XR_TYPE_VIEW_CONFIGURATION_PROPERTIES:
      return "XR_TYPE_VIEW_CONFIGURATION_PROPERTIES";
    case XR_TYPE_FRAME_BEGIN_INFO:
      return "XR_TYPE_FRAME_BEGIN_INFO";
    case XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW:
      return "XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW";
    case XR_TYPE_EVENT_DATA_EVENTS_LOST:
      return "XR_TYPE_EVENT_DATA_EVENTS_LOST";
    case XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING:
      return "XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING";
    case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED:
      return "XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED";
    case XR_TYPE_INTERACTION_PROFILE_STATE:
      return "XR_TYPE_INTERACTION_PROFILE_STATE";
    case XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO:
      return "XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO";
    case XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO:
      return "XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO";
    case XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO:
      return "XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO";
    case XR_TYPE_ACTION_STATE_GET_INFO:
      return "XR_TYPE_ACTION_STATE_GET_INFO";
    case XR_TYPE_HAPTIC_ACTION_INFO:
      return "XR_TYPE_HAPTIC_ACTION_INFO";
    case XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO:
      return "XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO";
    case XR_TYPE_ACTIONS_SYNC_INFO:
      return "XR_TYPE_ACTIONS_SYNC_INFO";
    case XR_TYPE_BOUND_SOURCES_FOR_ACTION_ENUMERATE_INFO:
      return "XR_TYPE_BOUND_SOURCES_FOR_ACTION_ENUMERATE_INFO";
    case XR_TYPE_INPUT_SOURCE_LOCALIZED_NAME_GET_INFO:
      return "XR_TYPE_INPUT_SOURCE_LOCALIZED_NAME_GET_INFO";
    case XR_TYPE_COMPOSITION_LAYER_CUBE_KHR:
      return "XR_TYPE_COMPOSITION_LAYER_CUBE_KHR";
    case XR_TYPE_INSTANCE_CREATE_INFO_ANDROID_KHR:
      return "XR_TYPE_INSTANCE_CREATE_INFO_ANDROID_KHR";
    case XR_TYPE_COMPOSITION_LAYER_DEPTH_INFO_KHR:
      return "XR_TYPE_COMPOSITION_LAYER_DEPTH_INFO_KHR";
    case XR_TYPE_VULKAN_SWAPCHAIN_FORMAT_LIST_CREATE_INFO_KHR:
      return "XR_TYPE_VULKAN_SWAPCHAIN_FORMAT_LIST_CREATE_INFO_KHR";
    case XR_TYPE_EVENT_DATA_PERF_SETTINGS_EXT:
      return "XR_TYPE_EVENT_DATA_PERF_SETTINGS_EXT";
    case XR_TYPE_COMPOSITION_LAYER_CYLINDER_KHR:
      return "XR_TYPE_COMPOSITION_LAYER_CYLINDER_KHR";
    case XR_TYPE_COMPOSITION_LAYER_EQUIRECT_KHR:
      return "XR_TYPE_COMPOSITION_LAYER_EQUIRECT_KHR";
    case XR_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT:
      return "XR_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT";
    case XR_TYPE_DEBUG_UTILS_MESSENGER_CALLBACK_DATA_EXT:
      return "XR_TYPE_DEBUG_UTILS_MESSENGER_CALLBACK_DATA_EXT";
    case XR_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT:
      return "XR_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT";
    case XR_TYPE_DEBUG_UTILS_LABEL_EXT:
      return "XR_TYPE_DEBUG_UTILS_LABEL_EXT";
    case XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR:
      return "XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR";
    case XR_TYPE_GRAPHICS_BINDING_OPENGL_XLIB_KHR:
      return "XR_TYPE_GRAPHICS_BINDING_OPENGL_XLIB_KHR";
    case XR_TYPE_GRAPHICS_BINDING_OPENGL_XCB_KHR:
      return "XR_TYPE_GRAPHICS_BINDING_OPENGL_XCB_KHR";
    case XR_TYPE_GRAPHICS_BINDING_OPENGL_WAYLAND_KHR:
      return "XR_TYPE_GRAPHICS_BINDING_OPENGL_WAYLAND_KHR";
    case XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR:
      return "XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR";
    case XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR:
      return "XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR";
    case XR_TYPE_GRAPHICS_BINDING_OPENGL_ES_ANDROID_KHR:
      return "XR_TYPE_GRAPHICS_BINDING_OPENGL_ES_ANDROID_KHR";
    case XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR:
      return "XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR";
    case XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_ES_KHR:
      return "XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_ES_KHR";
    case XR_TYPE_GRAPHICS_BINDING_VULKAN_KHR:
      return "XR_TYPE_GRAPHICS_BINDING_VULKAN_KHR";
    case XR_TYPE_SWAPCHAIN_IMAGE_VULKAN_KHR:
      return "XR_TYPE_SWAPCHAIN_IMAGE_VULKAN_KHR";
    case XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN_KHR:
      return "XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN_KHR";
    case XR_TYPE_GRAPHICS_BINDING_D3D11_KHR:
      return "XR_TYPE_GRAPHICS_BINDING_D3D11_KHR";
    case XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR:
      return "XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR";
    case XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR:
      return "XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR";
    case XR_TYPE_GRAPHICS_BINDING_D3D12_KHR:
      return "XR_TYPE_GRAPHICS_BINDING_D3D12_KHR";
    case XR_TYPE_SWAPCHAIN_IMAGE_D3D12_KHR:
      return "XR_TYPE_SWAPCHAIN_IMAGE_D3D12_KHR";
    case XR_TYPE_GRAPHICS_REQUIREMENTS_D3D12_KHR:
      return "XR_TYPE_GRAPHICS_REQUIREMENTS_D3D12_KHR";
    case XR_TYPE_SYSTEM_EYE_GAZE_INTERACTION_PROPERTIES_EXT:
      return "XR_TYPE_SYSTEM_EYE_GAZE_INTERACTION_PROPERTIES_EXT";
    case XR_TYPE_EYE_GAZE_SAMPLE_TIME_EXT:
      return "XR_TYPE_EYE_GAZE_SAMPLE_TIME_EXT";
    case XR_TYPE_VISIBILITY_MASK_KHR:
      return "XR_TYPE_VISIBILITY_MASK_KHR";
    case XR_TYPE_EVENT_DATA_VISIBILITY_MASK_CHANGED_KHR:
      return "XR_TYPE_EVENT_DATA_VISIBILITY_MASK_CHANGED_KHR";
    case XR_TYPE_SESSION_CREATE_INFO_OVERLAY_EXTX:
      return "XR_TYPE_SESSION_CREATE_INFO_OVERLAY_EXTX";
    case XR_TYPE_EVENT_DATA_MAIN_SESSION_VISIBILITY_CHANGED_EXTX:
      return "XR_TYPE_EVENT_DATA_MAIN_SESSION_VISIBILITY_CHANGED_EXTX";
    case XR_TYPE_COMPOSITION_LAYER_COLOR_SCALE_BIAS_KHR:
      return "XR_TYPE_COMPOSITION_LAYER_COLOR_SCALE_BIAS_KHR";
    case XR_TYPE_SPATIAL_ANCHOR_CREATE_INFO_MSFT:
      return "XR_TYPE_SPATIAL_ANCHOR_CREATE_INFO_MSFT";
    case XR_TYPE_SPATIAL_ANCHOR_SPACE_CREATE_INFO_MSFT:
      return "XR_TYPE_SPATIAL_ANCHOR_SPACE_CREATE_INFO_MSFT";
    case XR_TYPE_VIEW_CONFIGURATION_DEPTH_RANGE_EXT:
      return "XR_TYPE_VIEW_CONFIGURATION_DEPTH_RANGE_EXT";
    case XR_TYPE_GRAPHICS_BINDING_EGL_MNDX:
      return "XR_TYPE_GRAPHICS_BINDING_EGL_MNDX";
    case XR_TYPE_SPATIAL_GRAPH_NODE_SPACE_CREATE_INFO_MSFT:
      return "XR_TYPE_SPATIAL_GRAPH_NODE_SPACE_CREATE_INFO_MSFT";
    case XR_TYPE_SYSTEM_HAND_TRACKING_PROPERTIES_EXT:
      return "XR_TYPE_SYSTEM_HAND_TRACKING_PROPERTIES_EXT";
    case XR_TYPE_HAND_TRACKER_CREATE_INFO_EXT:
      return "XR_TYPE_HAND_TRACKER_CREATE_INFO_EXT";
    case XR_TYPE_HAND_JOINTS_LOCATE_INFO_EXT:
      return "XR_TYPE_HAND_JOINTS_LOCATE_INFO_EXT";
    case XR_TYPE_HAND_JOINT_LOCATIONS_EXT:
      return "XR_TYPE_HAND_JOINT_LOCATIONS_EXT";
    case XR_TYPE_HAND_JOINT_VELOCITIES_EXT:
      return "XR_TYPE_HAND_JOINT_VELOCITIES_EXT";
    case XR_TYPE_SYSTEM_HAND_TRACKING_MESH_PROPERTIES_MSFT:
      return "XR_TYPE_SYSTEM_HAND_TRACKING_MESH_PROPERTIES_MSFT";
    case XR_TYPE_HAND_MESH_SPACE_CREATE_INFO_MSFT:
      return "XR_TYPE_HAND_MESH_SPACE_CREATE_INFO_MSFT";
    case XR_TYPE_HAND_MESH_UPDATE_INFO_MSFT:
      return "XR_TYPE_HAND_MESH_UPDATE_INFO_MSFT";
    case XR_TYPE_HAND_MESH_MSFT:
      return "XR_TYPE_HAND_MESH_MSFT";
    case XR_TYPE_HAND_POSE_TYPE_INFO_MSFT:
      return "XR_TYPE_HAND_POSE_TYPE_INFO_MSFT";
    case XR_TYPE_SECONDARY_VIEW_CONFIGURATION_SESSION_BEGIN_INFO_MSFT:
      return "XR_TYPE_SECONDARY_VIEW_CONFIGURATION_SESSION_BEGIN_INFO_MSFT";
    case XR_TYPE_SECONDARY_VIEW_CONFIGURATION_STATE_MSFT:
      return "XR_TYPE_SECONDARY_VIEW_CONFIGURATION_STATE_MSFT";
    case XR_TYPE_SECONDARY_VIEW_CONFIGURATION_FRAME_STATE_MSFT:
      return "XR_TYPE_SECONDARY_VIEW_CONFIGURATION_FRAME_STATE_MSFT";
    case XR_TYPE_SECONDARY_VIEW_CONFIGURATION_FRAME_END_INFO_MSFT:
      return "XR_TYPE_SECONDARY_VIEW_CONFIGURATION_FRAME_END_INFO_MSFT";
    case XR_TYPE_SECONDARY_VIEW_CONFIGURATION_LAYER_INFO_MSFT:
      return "XR_TYPE_SECONDARY_VIEW_CONFIGURATION_LAYER_INFO_MSFT";
    case XR_TYPE_SECONDARY_VIEW_CONFIGURATION_SWAPCHAIN_CREATE_INFO_MSFT:
      return "XR_TYPE_SECONDARY_VIEW_CONFIGURATION_SWAPCHAIN_CREATE_INFO_MSFT";
    case XR_TYPE_CONTROLLER_MODEL_KEY_STATE_MSFT:
      return "XR_TYPE_CONTROLLER_MODEL_KEY_STATE_MSFT";
    case XR_TYPE_CONTROLLER_MODEL_NODE_PROPERTIES_MSFT:
      return "XR_TYPE_CONTROLLER_MODEL_NODE_PROPERTIES_MSFT";
    case XR_TYPE_CONTROLLER_MODEL_PROPERTIES_MSFT:
      return "XR_TYPE_CONTROLLER_MODEL_PROPERTIES_MSFT";
    case XR_TYPE_CONTROLLER_MODEL_NODE_STATE_MSFT:
      return "XR_TYPE_CONTROLLER_MODEL_NODE_STATE_MSFT";
    case XR_TYPE_CONTROLLER_MODEL_STATE_MSFT:
      return "XR_TYPE_CONTROLLER_MODEL_STATE_MSFT";
    case XR_TYPE_VIEW_CONFIGURATION_VIEW_FOV_EPIC:
      return "XR_TYPE_VIEW_CONFIGURATION_VIEW_FOV_EPIC";
    case XR_TYPE_HOLOGRAPHIC_WINDOW_ATTACHMENT_MSFT:
      return "XR_TYPE_HOLOGRAPHIC_WINDOW_ATTACHMENT_MSFT";
    case XR_TYPE_ANDROID_SURFACE_SWAPCHAIN_CREATE_INFO_FB:
      return "XR_TYPE_ANDROID_SURFACE_SWAPCHAIN_CREATE_INFO_FB";
    case XR_TYPE_INTERACTION_PROFILE_ANALOG_THRESHOLD_VALVE:
      return "XR_TYPE_INTERACTION_PROFILE_ANALOG_THRESHOLD_VALVE";
    case XR_TYPE_LOADER_INIT_INFO_ANDROID_KHR:
      return "XR_TYPE_LOADER_INIT_INFO_ANDROID_KHR";
    case XR_TYPE_VULKAN_INSTANCE_CREATE_INFO_KHR:
      return "XR_TYPE_VULKAN_INSTANCE_CREATE_INFO_KHR";
    case XR_TYPE_VULKAN_DEVICE_CREATE_INFO_KHR:
      return "XR_TYPE_VULKAN_DEVICE_CREATE_INFO_KHR";
    case XR_TYPE_VULKAN_GRAPHICS_DEVICE_GET_INFO_KHR:
      return "XR_TYPE_VULKAN_GRAPHICS_DEVICE_GET_INFO_KHR";
    case XR_TYPE_COMPOSITION_LAYER_EQUIRECT2_KHR:
      return "XR_TYPE_COMPOSITION_LAYER_EQUIRECT2_KHR";
    case XR_TYPE_EVENT_DATA_DISPLAY_REFRESH_RATE_CHANGED_FB:
      return "XR_TYPE_EVENT_DATA_DISPLAY_REFRESH_RATE_CHANGED_FB";
    case XR_TYPE_SYSTEM_COLOR_SPACE_PROPERTIES_FB:
      return "XR_TYPE_SYSTEM_COLOR_SPACE_PROPERTIES_FB";
    case XR_TYPE_BINDING_MODIFICATIONS_KHR:
      return "XR_TYPE_BINDING_MODIFICATIONS_KHR";
    default:
      return "UNRECOGNIZED_XR_TYPE";
  }
}

#endif
// VTK-HeaderTest-Exclude: vtkOpenXRUtilities.h
