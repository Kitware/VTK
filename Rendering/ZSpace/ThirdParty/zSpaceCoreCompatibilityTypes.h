//////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2022 zSpace, Inc.  All Rights Reserved.
//
//////////////////////////////////////////////////////////////////////////

#ifndef __ZSPACE_CORE_COMPATIBILITY_TYPES_H__
#define __ZSPACE_CORE_COMPATIBILITY_TYPES_H__

#include "zSpaceTypes.h"


#if defined(__cplusplus)
extern "C" {
#endif

//////////////////////////////////////////////////////////////////////////
// Handle Types
//////////////////////////////////////////////////////////////////////////

struct ZCCompatContextOpaque;
typedef struct ZCCompatContextOpaque* ZCCompatContext;

struct ZCCompatDisplayOpaque;
typedef struct ZCCompatDisplayOpaque* ZCCompatDisplay;

struct ZCCompatViewportOpaque;
typedef struct ZCCompatViewportOpaque* ZCCompatViewport;

struct ZCCompatFrustumOpaque;
typedef struct ZCCompatFrustumOpaque* ZCCompatFrustum;

struct ZCCompatTargetOpaque;
typedef struct ZCCompatTargetOpaque* ZCCompatTarget;


//////////////////////////////////////////////////////////////////////////
// Enums
/////////////////////////////////////////////////////////////////////////

// Defines the error codes returned by all Core SDK functions.
typedef enum ZCCompatError
{
    ZC_COMPAT_ERROR_OK = 0,
    ZC_COMPAT_ERROR_NOT_IMPLEMENTED = 1,
    ZC_COMPAT_ERROR_NOT_INITIALIZED = 2,
    ZC_COMPAT_ERROR_ALREADY_INITIALIZED = 3,
    ZC_COMPAT_ERROR_INVALID_PARAMETER = 4,
    ZC_COMPAT_ERROR_INVALID_CONTEXT = 5,
    ZC_COMPAT_ERROR_INVALID_HANDLE = 6,
    ZC_COMPAT_ERROR_RUNTIME_INCOMPATIBLE = 7,
    ZC_COMPAT_ERROR_RUNTIME_NOT_FOUND = 8,
    ZC_COMPAT_ERROR_SYMBOL_NOT_FOUND = 9,
    ZC_COMPAT_ERROR_DISPLAY_NOT_FOUND = 10,
    ZC_COMPAT_ERROR_DEVICE_NOT_FOUND = 11,
    ZC_COMPAT_ERROR_TARGET_NOT_FOUND = 12,
    ZC_COMPAT_ERROR_CAPABILITY_NOT_FOUND = 13,
    ZC_COMPAT_ERROR_BUFFER_TOO_SMALL = 14,
    ZC_COMPAT_ERROR_SYNC_FAILED = 15,
    ZC_COMPAT_ERROR_OPERATION_FAILED = 16,
    ZC_COMPAT_ERROR_INVALID_ATTRIBUTE = 17,
} ZCCompatError;


// Defines the log level values passed to logging functions.
typedef enum ZCCompatLogLevel
{
    ZC_COMPAT_LOG_LEVEL_ERROR = 0,
    ZC_COMPAT_LOG_LEVEL_WARNING = 1,
    ZC_COMPAT_LOG_LEVEL_INFO = 2,
    ZC_COMPAT_LOG_LEVEL_DEBUG = 3,
} ZCCompatLogLevel;


// Defines the types of displays for the Display APIs.
typedef enum ZCCompatDisplayType
{
    ZC_COMPAT_DISPLAY_TYPE_UNKNOWN = -1,
    ZC_COMPAT_DISPLAY_TYPE_GENERIC = 0,
    ZC_COMPAT_DISPLAY_TYPE_ZSPACE = 1
} ZCCompatDisplayType;


// Defines the attributes that you can query for the display using
// zccompatGetDisplayAttributeStr().
typedef enum ZCCompatDisplayAttribute
{
    // The graphics adapter name.
    ZC_COMPAT_DISPLAY_ATTRIBUTE_ADAPTER_NAME = 0,
    // The graphics adapter context string.
    ZC_COMPAT_DISPLAY_ATTRIBUTE_ADAPTER_STRING = 1,
    // The entire ID string of the graphics adapter.
    ZC_COMPAT_DISPLAY_ATTRIBUTE_ADAPTER_ID = 2,
    // The vendor ID of the graphics adapter.
    ZC_COMPAT_DISPLAY_ATTRIBUTE_ADAPTER_VENDOR_ID = 3,
    // The device ID of the graphics adapter.
    ZC_COMPAT_DISPLAY_ATTRIBUTE_ADAPTER_DEVICE_ID = 4,
    // Reserved.
    ZC_COMPAT_DISPLAY_ATTRIBUTE_ADAPTER_KEY = 5,
    // The monitor name.
    ZC_COMPAT_DISPLAY_ATTRIBUTE_MONITOR_NAME = 6,
    // The monitor context string.
    ZC_COMPAT_DISPLAY_ATTRIBUTE_MONITOR_STRING = 7,
    // The entire ID string of the monitor.
    ZC_COMPAT_DISPLAY_ATTRIBUTE_MONITOR_ID = 8,
    // The vendor ID of the monitor.
    ZC_COMPAT_DISPLAY_ATTRIBUTE_MONITOR_VENDOR_ID = 9,
    // The device ID of the monitor.
    ZC_COMPAT_DISPLAY_ATTRIBUTE_MONITOR_DEVICE_ID = 10,
    // Reserved.
    ZC_COMPAT_DISPLAY_ATTRIBUTE_MONITOR_KEY = 11,
    // The display's manufacturer name.
    ZC_COMPAT_DISPLAY_ATTRIBUTE_MANUFACTURER_NAME = 12,
    // The display's product code.
    ZC_COMPAT_DISPLAY_ATTRIBUTE_PRODUCT_CODE = 13,
    // The display's serial number.
    ZC_COMPAT_DISPLAY_ATTRIBUTE_SERIAL_NUMBER = 14,
    // The display's video interface.
    ZC_COMPAT_DISPLAY_ATTRIBUTE_VIDEO_INTERFACE = 15,
    // The display's hardware model (currently only supported for zSpace
    // displays).
    ZC_COMPAT_DISPLAY_ATTRIBUTE_MODEL = 16,
} ZCCompatDisplayAttribute;


// Defines the eyes for the Stereo Frustum API.  This enum is also used by the
// Stereo Buffer API.
typedef enum ZCCompatEye
{
    ZC_COMPAT_EYE_LEFT = 0,
    ZC_COMPAT_EYE_RIGHT = 1,
    ZC_COMPAT_EYE_CENTER = 2
} ZCCompatEye;


// Defines the coordinate spaces used by the zSpace Core SDK.  This enum is used
// by both the Coordinate Space API and the Stereo Frustum API.
typedef enum ZCCompatCoordinateSpace
{
    ZC_COMPAT_COORDINATE_SPACE_TRACKER = 0,
    ZC_COMPAT_COORDINATE_SPACE_DISPLAY = 1,
    ZC_COMPAT_COORDINATE_SPACE_VIEWPORT = 2,
    ZC_COMPAT_COORDINATE_SPACE_CAMERA = 3
} ZCCompatCoordinateSpace;


// Defines the attributes that you can set and query for the StereoFrustum.
// These attributes are important for comfortable viewing of stereoscopic 3D.
typedef enum ZCCompatFrustumAttribute
{
    // Float Attributes

    // The physical separation, or inter-pupillary distance, between the eyes
    // in meters.
    //
    // An IPD of 0 will effectively disable stereo since the eyes are assumed
    // to be at the same location.  (Default: 0.060)
    ZC_COMPAT_FRUSTUM_ATTRIBUTE_IPD = 0x00000000,

    // Viewer scale adjusts the display and head tracking for larger and
    // smaller scenes.  (Default: 1)
    //
    // Use larger values for scenes with large models and smaller values for
    // smaller models.
    ZC_COMPAT_FRUSTUM_ATTRIBUTE_VIEWER_SCALE = 0x00000001,

    // Uniform scale factor applied to the frustum's incoming head pose.
    // (Default: 1)
    ZC_COMPAT_FRUSTUM_ATTRIBUTE_HEAD_SCALE = 0x00000003,

    // Near clipping plane for the frustum in meters.  (Default: 0.1)
    ZC_COMPAT_FRUSTUM_ATTRIBUTE_NEAR_CLIP = 0x00000004,

    // Far clipping plane for the frustum in meters.  (Default: 1000)
    ZC_COMPAT_FRUSTUM_ATTRIBUTE_FAR_CLIP = 0x00000005,

    // Distance between the bridge of the glasses and the bridge of the nose in
    // meters.  (Default: 0.01)
    ZC_COMPAT_FRUSTUM_ATTRIBUTE_GLASSES_OFFSET = 0x00000006,

    // Maximum pixel disparity for crossed images (negative parallax) in the
    // coupled zone.  (Default: -100)
    //
    // The coupled zone refers to the area where our eyes can both comfortably
    // converge and focus on an object.
    ZC_COMPAT_FRUSTUM_ATTRIBUTE_CC_LIMIT = 0x00000007,

    // Maximum pixel disparity for uncrossed images (positive parallax) in the
    // coupled zone.  (Default: 100)
    ZC_COMPAT_FRUSTUM_ATTRIBUTE_UC_LIMIT = 0x00000008,

    // Maximum pixel disparity for crossed images (negative parallax) in the
    // uncoupled zone.  (Default: -200)
    ZC_COMPAT_FRUSTUM_ATTRIBUTE_CU_LIMIT = 0x00000009,

    // Maximum pixel disparity for uncrossed images (positive parallax) in the
    // uncoupled zone.  (Default: 250)
    ZC_COMPAT_FRUSTUM_ATTRIBUTE_UU_LIMIT = 0x0000000A,

    // Maximum depth in meters for negative parallax in the coupled zone.
    // (Default: 0.13)
    ZC_COMPAT_FRUSTUM_ATTRIBUTE_CC_DEPTH = 0x0000000B,

    // Maximum depth in meters for positive parallax in the coupled zone.
    // (Default: -0.30)
    ZC_COMPAT_FRUSTUM_ATTRIBUTE_UC_DEPTH = 0x0000000C,

    // Display angle in degrees about the X axis.  (Default: 30.0)
    //
    // Is only used when ZC_PORTAL_MODE_ANGLE is not enabled on the frustum.
    ZC_COMPAT_FRUSTUM_ATTRIBUTE_DISPLAY_ANGLE_X = 0x0000000D,

    // Display angle in degrees about the Y axis.  (Default: 0.0)
    //
    // Is only used when ZC_PORTAL_MODE_ANGLE is not enabled on the frustum.
    ZC_COMPAT_FRUSTUM_ATTRIBUTE_DISPLAY_ANGLE_Y = 0x0000000E,

    // Display angle in degrees about the Z axis.  (Default: 0.0)
    //
    // Is only used when ZC_PORTAL_MODE_ANGLE is not enabled on the frustum.
    ZC_COMPAT_FRUSTUM_ATTRIBUTE_DISPLAY_ANGLE_Z = 0x0000000F,

    // The delay in seconds before the automatic transition from stereo to
    // mono begins.  (Default: 5.0)
    ZC_COMPAT_FRUSTUM_ATTRIBUTE_AUTO_STEREO_DELAY = 0x00000010,

    // The duration in seconds of the automatic transition from stereo to
    // mono.  (Default: 1.0)
    ZC_COMPAT_FRUSTUM_ATTRIBUTE_AUTO_STEREO_DURATION = 0x00000011,


    // Boolean Attributes

    // Flag controlling whether the automatic transition from stereo to mono
    // is enabled.  (Default: true)
    ZC_COMPAT_FRUSTUM_ATTRIBUTE_AUTO_STEREO_ENABLED = 0x00010000,

} ZCCompatFrustumAttribute;


// Defines the types of tracker targets.
typedef enum ZCCompatTargetType
{
    // The tracker target corresponding to the user's head.
    ZC_COMPAT_TARGET_TYPE_HEAD = 0,
    // The tracker target corresponding to the user's primary hand.
    ZC_COMPAT_TARGET_TYPE_PRIMARY = 1,
    // The tracker target corresponding to the user's secondary hand.
    // (Reserved for future use.)
    ZC_COMPAT_TARGET_TYPE_SECONDARY = 2,

    // The tracker target corresponding to the user's left eye.
    ZC_COMPAT_TARGET_TYPE_EYE_LEFT = 1000,
    // The tracker target corresponding to the user's right eye.
    ZC_COMPAT_TARGET_TYPE_EYE_RIGHT = 1001,
    // The tracker target that is halfway between the user's left and righ
    // eyes.
    ZC_COMPAT_TARGET_TYPE_EYE_CENTER = 1002,
} ZCCompatTargetType;


// Defines mouse buttons to be used when mapping a tracker target's buttons to
// a mouse.
typedef enum ZCCompatMouseButton
{
    ZC_COMPAT_MOUSE_BUTTON_UNKNOWN = -1,
    ZC_COMPAT_MOUSE_BUTTON_LEFT = 0,
    ZC_COMPAT_MOUSE_BUTTON_RIGHT = 1,
    ZC_COMPAT_MOUSE_BUTTON_CENTER = 2
} ZCCompatMouseButton;


// Determines how the stylus and mouse control the cursor when both are used.
typedef enum ZCCompatMouseMovementMode
{
    // The stylus uses absolute positions.  In this mode, the mouse and stylus
    // can fight for control of the cursor if both are in use.
    //
    // This is the default mode.
    ZC_COMPAT_MOUSE_MOVEMENT_MODE_ABSOLUTE = 0,

    // The stylus applies delta positions to the mouse cursor's current
    // position.
    //
    // Movements by the mouse and stylus are compounded without fighting.
    ZC_COMPAT_MOUSE_MOVEMENT_MODE_RELATIVE = 1
} ZCCompatMouseMovementMode;


// Defines the possible modes for display stereo content.
typedef enum ZCCompatStereoDisplayMode
{
    ZC_COMPAT_STEREO_DISPLAY_MODE_QUAD_BUFFER_STEREO = 0,
    ZC_COMPAT_STEREO_DISPLAY_MODE_STEREO_DISPLAY_API = 1,
} ZCCompatStereoDisplayMode;


//////////////////////////////////////////////////////////////////////////
// Function Pointer Types
//////////////////////////////////////////////////////////////////////////

typedef void(*ZCCompatLogFuncFuncPtrType)(
    void* userData,
    ZCCompatLogLevel level,
    const char* file,
    ZSInt32 line,
    const char* function,
    const char* message);


//////////////////////////////////////////////////////////////////////////
// Compound Types
//////////////////////////////////////////////////////////////////////////

// Ensure 8 byte packing.
#pragma pack(push, 8)

// Struct representing display intersection information.
typedef struct ZCCompatDisplayIntersectionInfo
{
    // Whether or not the display was intersected.
    ZSBool hit;
    // The x pixel coordinate on the virtual desktop.
    ZSInt32 x;
    // The y pixel coordinate on the virtual desktop.
    ZSInt32 y;
    // The normalized absolute x pixel coordinate on the virtual desktop.
    ZSInt32 nx;
    // The normalized absolute y pixel coordinate on the virtual desktop.
    ZSInt32 ny;
    // The distance from origin of the raycast to the point of intersection on
    // the display in meters.
    ZSFloat distance;
} ZCCompatDisplayIntersectionInfo;


// Union representing frustum bounds.
typedef union ZCCompatFrustumBounds
{
    ZSFloat f[6];
    struct
    {
        ZSFloat left;
        ZSFloat right;
        ZSFloat bottom;
        ZSFloat top;
        ZSFloat nearClip;
        ZSFloat farClip;
    };
} ZCCompatFrustumBounds;


// Struct representing tracker pose information.
//
// This structure is used by the Tracker Target, Display, and Stereo Frustum
// APIs.
typedef struct ZCCompatTrackerPose
{
    // The time that the pose was captured (represented in seconds since last
    // system reboot).
    ZSDouble timestamp;
    // The tracker-space position and orientation in 4x4 matrix format.
    ZSMatrix4 matrix;
} ZCCompatTrackerPose;

#pragma pack(pop)

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif // __ZSPACE_CORE_COMPATIBILITY_TYPES_H__
