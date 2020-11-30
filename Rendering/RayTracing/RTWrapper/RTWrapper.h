#pragma once

#include "Backend.h"
#include "Types.h"
#include <set>

void rtwInit();
RTW::Backend *rtwSwitch(const char *name);
void rtwShutdown();

std::set<RTWBackendType> rtwGetAvailableBackends();

// --- Define-based mapping from OSPRay to RTWrapper ----
#define OSPObject RTWObject
#define OSPGroup RTWGroup
#define OSPRenderer RTWRenderer
#define OSPWorld RTWWorld
#define OSPInstance RTWInstance
#define OSPGeometricModel RTWGeometricModel
#define OSPData RTWData
#define OSPCamera RTWCamera
#define OSPLight RTWLight
#define OSPTexture2D RTWTexture2D
#define OSPTexture RTWTexture2D
#define OSPTextureFormat RTWTextureFormat
#define OSPMaterial RTWMaterial
#define OSPGeometry RTWGeometry
#define OSPFrameBuffer RTWFrameBuffer
#define OSPVolume RTWVolume
#define OSPVolumetricModel RTWVolumetricModel
#define OSPTransferFunction RTWTransferFunction
#define OSPDataType RTWDataType

#define OSP_FLOAT RTW_FLOAT
#define OSP_VEC2F RTW_VEC2F
#define OSP_VEC3F RTW_VEC3F
#define OSP_VEC4F RTW_VEC4F
#define OSP_BOX3F RTW_BOX3F

#define OSP_INT RTW_INT
#define OSP_VEC2I RTW_VEC2I
#define OSP_VEC3I RTW_VEC3I
#define OSP_VEC4I RTW_VEC4I
#define OSP_BOX3I RTW_BOX3I

#define OSP_UINT RTW_UINT
#define OSP_VEC2UI RTW_VEC2UI
#define OSP_VEC3UI RTW_VEC3UI
#define OSP_VEC4UI RTW_VEC4UI

#define OSP_UCHAR RTW_UCHAR
#define OSP_VEC2UC RTW_VEC2UC
#define OSP_VEC3UC RTW_VEC3UC
#define OSP_VEC4UC RTW_VEC4UC
#define OSP_USHORT RTW_USHORT

#define OSP_LONG RTW_LONG
#define OSP_ULONG RTW_ULONG

#define OSP_SHORT RTW_SHORT
#define OSP_USHORT RTW_USHORT

#define OSP_DOUBLE RTW_DOUBLE

#define OSP_OBJECT RTW_OBJECT
#define OSP_RAW RTW_RAW
// Object reference type.
#define OSP_DEVICE RTW_DEVICE
// Void pointer type.
#define OSP_VOID_PTR RTW_VOID_PTR
// Booleans, same size as RTW_INT.
#define OSP_BOOL RTW_BOOL
// object subtypes
#define OSP_CAMERA RTW_CAMERA
#define OSP_DATA RTW_DATA
#define OSP_FRAMEBUFFER RTW_FRAMEBUFFER
#define OSP_FUTURE RTW_FUTURE
#define OSP_GEOMETRIC_MODEL RTW_GEOMETRIC_MODEL
#define OSP_GEOMETRY RTW_GEOMETRY
#define OSP_GROUP RTW_GROUP
#define OSP_IMAGE_OPERATION RTW_IMAGE_OPERATION
#define OSP_INSTANCE RTW_INSTANCE
#define OSP_LIGHT RTW_LIGHT
#define OSP_MATERIAL RTW_MATERIAL
#define OSP_RENDERER RTW_RENDERER
#define OSP_TEXTURE RTW_TEXTURE
#define OSP_TRANSFER_FUNCTION RTW_TRANSFER_FUNCTION
#define OSP_VOLUME RTW_VOLUME
#define OSP_VOLUMETRIC_MODEL RTW_VOLUMETRIC_MODEL
#define OSP_WORLD RTW_WORLD
// Pointer to a C-style NULL-terminated character string.
#define OSP_STRING RTW_STRING
// Character scalar type.
#define OSP_CHAR RTW_CHAR
#define OSP_UNKNOWN RTW_UNKNOWN

#define OSP_TEXTURE_FILTER_NEAREST RTW_TEXTURE_FILTER_NEAREST
#define OSP_TEXTURE_FILTER_BILINEAR RTW_TEXTURE_FILTER_BILINEAR

#define OSP_TEXTURE_RGB8 RTW_TEXTURE_RGB8
#define OSP_TEXTURE_R32F RTW_TEXTURE_R32F
#define OSP_TEXTURE_RGB32F RTW_TEXTURE_RGB32F
#define OSP_TEXTURE_RGBA32F RTW_TEXTURE_RGBA32F
#define OSP_TEXTURE_R8 RTW_TEXTURE_R8
#define OSP_TEXTURE_RGB8 RTW_TEXTURE_RGB8
#define OSP_TEXTURE_RGBA8 RTW_TEXTURE_RGBA8
#define OSP_TEXTURE_L8 RTW_TEXTURE_L8
#define OSP_TEXTURE_LA8 RTW_TEXTURE_LA8
#define OSP_TEXTURE_SRGB RTW_TEXTURE_SRGB
#define OSP_TEXTURE_SRGBA RTW_TEXTURE_SRGBA

#define OSP_FB_RGBA32F RTW_FB_RGBA32F
#define OSP_FB_RGBA8 RTW_FB_RGBA8
#define OSP_FB_SRGBA RTW_FB_SRGBA
#define OSP_FB_COLOR RTW_FB_COLOR
#define OSP_FB_DEPTH RTW_FB_DEPTH
#define OSP_FB_ACCUM RTW_FB_ACCUM
#define OSP_FB_NORMAL RTW_FB_NORMAL
#define OSP_FB_ALBEDO RTW_FB_ALBEDO

#define OSP_TETRAHEDRON RTW_TETRAHEDRON
#define OSP_HEXAHEDRON RTW_HEXAHEDRON
#define OSP_WEDGE RTW_WEDGE
#define OSP_PYRAMID RTW_PYRAMID

#define OSP_ROUND RTW_ROUND
#define OSP_FLAT RTW_FLAT
#define OSP_RIBBON RTW_RIBBON
#define OSP_UNKNOWN_CURVE_TYPE RTW_UNKNOWN_CURVE_TYPE

#define OSP_LINEAR RTW_LINEAR
#define OSP_BEZIER RTW_BEZIER
#define OSP_BSPLINE RTW_BSPLINE
#define OSP_HERMITE RTW_HERMITE
#define OSP_CATMULL_ROM RTW_CATMULL_ROM
#define OSP_UNKNOWN_CURVE_BASIS RTW_UNKOWN_CURVE_BASIS

#define ospSetFloat backend->SetFloat
#define ospSetBool backend->SetBool
#define ospSetInt backend->SetInt
#define ospSetVec2i backend->SetVec2i
#define ospSetVec3i backend->SetVec3i
#define ospSetVec2f backend->SetVec2f
#define ospSetVec3f backend->SetVec3f
#define ospSetVec4f backend->SetVec4f
#define ospSetString backend->SetString
#define ospSetParam backend->SetParam
#define ospSetObject backend->SetObject
#define ospSetObjectAsData backend->SetObjectAsData

#define ospRemoveParam backend->RemoveParam

#define ospCommit backend->Commit
#define ospRelease backend->Release

#define ospNewData backend->NewData
#define ospNewCopyData1D backend->NewCopyData1D
#define ospNewCopyData2D backend->NewCopyData2D
#define ospNewCopyData3D backend->NewCopyData3D
#define ospNewSharedData1D backend->NewSharedData1D
#define ospNewSharedData2D backend->NewSharedData2D
#define ospNewSharedData3D backend->NewSharedData3D
#define ospNewGroup backend->NewGroup
#define ospNewCamera backend->NewCamera
#define ospNewLight backend->NewLight
#define ospNewTexture backend->NewTexture
#define ospNewMaterial backend->NewMaterial
#define ospNewTransferFunction backend->NewTransferFunction
#define ospNewVolume backend->NewVolume
#define ospNewGeometry backend->NewGeometry
#define ospNewModel backend->NewModel
#define ospNewGeometricModel backend->NewGeometricModel
#define ospNewVolumetricModel backend->NewVolumetricModel
#define ospNewWorld backend->NewWorld
#define ospNewInstance backend->NewInstance
#define ospNewFrameBuffer backend->NewFrameBuffer
#define ospNewRenderer backend->NewRenderer
#define ospFrameBufferClear backend->FrameBufferClear
#define ospRenderFrame backend->RenderFrame
#define ospMapFrameBuffer backend->MapFrameBuffer
#define ospUnmapFrameBuffer backend->UnmapFrameBuffer

#define ospAddGeometry backend->AddGeometry
#define ospAddVolume backend->AddVolume

#define ospcommon rtw
#define osp rtw
