#pragma once

#include <stdint.h>
#include <sys/types.h>

typedef enum : uint32_t
{
    RTW_NO_ERROR = 0,
    RTW_UNKNOWN_ERROR = 1,
    RTW_INVALID_ARGUMENT = 2,
    RTW_INVALID_OPERATION = 3,
    RTW_OUT_OF_MEMORY = 4,
    RTW_UNSUPPORTED_DEVICE = 5,
} RTWError;

typedef enum : uint32_t
{
    RTW_FB_RGBA8,
    RTW_FB_RGBA32F,
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

typedef enum : uint32_t
{
    RTW_DATA_SHARED_BUFFER = (1 << 0),
} RTWDataCreationFlags;

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
    RTW_TEXTURE_FORMAT_INVALID = 0xff,
} RTWTextureFormat;

typedef enum : uint32_t
{
    RTW_TEXTURE_SHARED_BUFFER = (1 << 0),
    RTW_TEXTURE_FILTER_NEAREST = (1 << 1)
} RTWTextureCreationFlags;

typedef enum : uint32_t
{
    RTW_OBJECT = 1000,
    RTW_UCHAR = 2500, RTW_UCHAR2, RTW_UCHAR3, RTW_UCHAR4,
    RTW_SHORT = 3000,
    RTW_USHORT = 3500,
    RTW_INT = 4000, RTW_INT2, RTW_INT3, RTW_INT4,
    RTW_FLOAT = 6000, RTW_FLOAT2, RTW_FLOAT3, RTW_FLOAT4, RTW_FLOAT3A,
    RTW_DOUBLE = 7000,
    RTW_UNKNOWN = 22222,
    RTW_RAW = 2500
} RTWDataType;

typedef enum : uint32_t
{
    RTW_BACKEND_OSPRAY = 1,
    RTW_BACKEND_VISRTX = 2
} RTWBackendType;

namespace rtw
{
    struct vec2f { float x, y; };
    struct vec2i { int x, y; };
    struct vec3i { int x, y, z; };
    struct vec3f { float x, y, z; };
    struct vec4f { float x, y, z, w; };
    struct box3i { vec3i lower, upper; };
    struct linear3f { vec3f vx, vy, vz; };
    struct affine3f { linear3f l; vec3f p; };
}

typedef struct RTWHandle
*RTWFrameBuffer,
*RTWRenderer,
*RTWCamera,
*RTWModel,
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
    //RTW_MDL,
} RTWFeature;
