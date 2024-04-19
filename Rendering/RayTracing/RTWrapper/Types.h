// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#pragma once

#include "vtkRenderingRayTracingModule.h"
#include <stdint.h>
#include <sys/types.h>

VTK_ABI_NAMESPACE_BEGIN
typedef enum : uint32_t
{
    RTW_NO_ERROR = 0,
    RTW_UNKNOWN_ERROR = 1,
    RTW_INVALID_ARGUMENT = 2,
    RTW_INVALID_OPERATION = 3,
    RTW_OUT_OF_MEMORY = 4,
    RTW_UNSUPPORTED_DEVICE = 5,
    RTW_VERSION_MISMATCH = 6,
} RTWError;

typedef enum : uint32_t
{
  RTW_FB_NONE,    //< framebuffer will not be mapped by application
  RTW_FB_RGBA8,   //< one dword per pixel: rgb+alpha, each one byte
  RTW_FB_SRGBA,   //< one dword per pixel: rgb (in sRGB space) + alpha, each one byte
  RTW_FB_RGBA32F, //< one float4 per pixel: rgb+alpha, each one float
} RTWFrameBufferFormat;

typedef enum : uint32_t
{
    RTW_FB_COLOR = (1 << 0),
    RTW_FB_DEPTH = (1 << 1),
    RTW_FB_ACCUM = (1 << 2),
    RTW_FB_VARIANCE = (1 << 3),
    RTW_FB_NORMAL = (1 << 4),
    RTW_FB_ALBEDO = (1 << 5),
} RTWFrameBufferChannel;

// OSPRay events which can be waited on via ospWait()
typedef enum : uint32_t
{
  RTW_NONE_FINISHED = 0,
  RTW_WORLD_RENDERED = 10,
  RTW_WORLD_COMMITTED = 20,
  RTW_FRAME_FINISHED = 30,
  RTW_TASK_FINISHED = 100000
} RTWSyncEvent;

// OSPRay cell types definition for unstructured volumes, values are set to match VTK
typedef enum : uint8_t
{
  RTW_TETRAHEDRON = 10,
  RTW_HEXAHEDRON = 12,
  RTW_WEDGE = 13,
  RTW_PYRAMID = 14,
  RTW_UNKNOWN_CELL_TYPE = 255
} RTWUnstructuredCellType;

// OSPRay PerspectiveCamera stereo image modes
typedef enum : uint8_t
{
  RTW_STEREO_NONE,
  RTW_STEREO_LEFT,
  RTW_STEREO_RIGHT,
  RTW_STEREO_SIDE_BY_SIDE,
  RTW_STEREO_UNKNOWN = 255
} RTWStereoMode;

// OSPRay Curves geometry types
typedef enum : uint8_t
{
  RTW_ROUND,
  RTW_FLAT,
  RTW_RIBBON,
  RTW_UNKNOWN_CURVE_TYPE = 255
} RTWCurveType;

// OSPRay Curves geometry bases
typedef enum : uint8_t
{
  RTW_LINEAR,
  RTW_BEZIER,
  RTW_BSPLINE,
  RTW_HERMITE,
  RTW_CATMULL_ROM,
  RTW_UNKNOWN_CURVE_BASIS = 255
} RTWCurveBasis;

// AMR Volume rendering methods
typedef enum : uint8_t
{
  RTW_AMR_CURRENT,
  RTW_AMR_FINEST,
  RTW_AMR_OCTANT
} RTWAMRMethod;

typedef enum : uint32_t
{
    RTW_TEXTURE_RGBA8,
    RTW_TEXTURE_SRGBA,
    RTW_TEXTURE_RGBA32F,
    RTW_TEXTURE_RGB8,
    RTW_TEXTURE_SRGB,
    RTW_TEXTURE_RGB32F,
    RTW_TEXTURE_R8,
    RTW_TEXTURE_R32F,
    RTW_TEXTURE_L8,
    RTW_TEXTURE_RA8,
    RTW_TEXTURE_LA8,
    RTW_TEXTURE_RGBA16,
    RTW_TEXTURE_RGB16,
    RTW_TEXTURE_RA16,
    RTW_TEXTURE_R16,
    RTW_TEXTURE_FORMAT_INVALID = 255,
} RTWTextureFormat;

typedef enum :uint32_t
{
    RTW_TEXTURE_FILTER_BILINEAR = 0,
    RTW_TEXTURE_FILTER_NEAREST
} RTWTextureFilter;

typedef enum : uint32_t
{
  // Object reference type.
  RTW_DEVICE = 100,

  // Void pointer type.
  RTW_VOID_PTR = 200,

  // Booleans, same size as RTW_INT.
  RTW_BOOL = 250,

  // highest bit to represent objects/handles
  RTW_OBJECT = 0x8000000,

  // object subtypes
  RTW_DATA = 0x8000000 + 100,
  RTW_CAMERA,
  RTW_FRAMEBUFFER,
  RTW_FUTURE,
  RTW_GEOMETRIC_MODEL,
  RTW_GEOMETRY,
  RTW_GROUP,
  RTW_IMAGE_OPERATION,
  RTW_INSTANCE,
  RTW_LIGHT,
  RTW_MATERIAL,
  RTW_RENDERER,
  RTW_TEXTURE,
  RTW_TRANSFER_FUNCTION,
  RTW_VOLUME,
  RTW_VOLUMETRIC_MODEL,
  RTW_WORLD,

  // Pointer to a C-style NULL-terminated character string.
  RTW_STRING = 1500,

  // Character scalar type.
  RTW_CHAR = 2000,

  // Unsigned character scalar and vector types.
  RTW_UCHAR = 2500, RTW_VEC2UC, RTW_VEC3UC, RTW_VEC4UC,
  RTW_BYTE = 2500, //XXX RTW_UCHAR, ISPC issue #1246
  RTW_RAW = 2500,  //XXX RTW_UCHAR, ISPC issue #1246

  // Signed 16-bit integer scalar.
  RTW_SHORT = 3000,

  // Unsigned 16-bit integer scalar.
  RTW_USHORT = 3500,

  // Signed 32-bit integer scalar and vector types.
  RTW_INT = 4000, RTW_VEC2I, RTW_VEC3I, RTW_VEC4I,

  // Unsigned 32-bit integer scalar and vector types.
  RTW_UINT = 4500, RTW_VEC2UI, RTW_VEC3UI, RTW_VEC4UI,

  // Signed 64-bit integer scalar and vector types.
  RTW_LONG = 5000, RTW_VEC2L, RTW_VEC3L, RTW_VEC4L,

  // Unsigned 64-bit integer scalar and vector types.
  RTW_ULONG = 5550, RTW_VEC2UL, RTW_VEC3UL, RTW_VEC4UL,

  // Single precision floating point scalar and vector types.
  RTW_FLOAT = 6000, RTW_VEC2F, RTW_VEC3F, RTW_VEC4F,

  // Double precision floating point scalar type.
  RTW_DOUBLE = 7000,

  // Signed 32-bit integer N-dimensional box types
  RTW_BOX1I = 8000, RTW_BOX2I, RTW_BOX3I, RTW_BOX4I,

  // Single precision floating point N-dimensional box types
  RTW_BOX1F = 10000, RTW_BOX2F, RTW_BOX3F, RTW_BOX4F,

  // Transformation types
  RTW_LINEAR2F = 12000, RTW_LINEAR3F, RTW_AFFINE2F, RTW_AFFINE3F,

  // Guard value.
  RTW_UNKNOWN = 9999999
} RTWDataType;

typedef enum : uint32_t
{
    RTW_BACKEND_OSPRAY = 1,
    RTW_BACKEND_VISRTX = 2
} RTWBackendType;
VTK_ABI_NAMESPACE_END

namespace rtw
{
VTK_ABI_NAMESPACE_BEGIN
    struct vec2f { float x, y; };
    struct vec2i { int x, y; };
    struct vec3i { int x, y, z; };
    struct vec3ui { unsigned int x, y, z; };
    struct vec3f { float x, y, z; };
    struct vec4f { float x, y, z, w; };
    struct box3i { vec3i lower, upper; };
    struct box3f { vec3f lower, upper; };
    struct linear3f { vec3f vx, vy, vz; };
    struct affine3f { linear3f l; vec3f p; };
VTK_ABI_NAMESPACE_END
}

VTK_ABI_NAMESPACE_BEGIN

typedef struct RTWHandle
*RTWFrameBuffer,
*RTWRenderer,
*RTWCamera,
*RTWGroup,
*RTWInstance,
*RTWGeometricModel,
*RTWVolumetricModel,
*RTWWorld,
*RTWData,
*RTWGeometry,
*RTWMaterial,
*RTWLight,
*RTWVolume,
*RTWTransferFunction,
*RTWTexture,
*RTWObject;

typedef RTWTexture RTWTexture2D;

typedef enum : uint32_t
{
    RTW_DEPTH_NORMALIZATION = 0,
    RTW_OPENGL_INTEROP = 1,
    RTW_ANIMATED_PARAMETERIZATION = 2,
    RTW_INSTANCING = 3,
    RTW_DENOISER = 4,
    RTW_DEPTH_COMPOSITING = 5,
} RTWFeature;
VTK_ABI_NAMESPACE_END
