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
#define OSPRenderer RTWRenderer
#define OSPModel RTWModel
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
#define OSPTransferFunction RTWTransferFunction
#define OSPDataType RTWDataType

#define OSP_FLOAT RTW_FLOAT
#define OSP_FLOAT2 RTW_FLOAT2
#define OSP_FLOAT3 RTW_FLOAT3
#define OSP_FLOAT4 RTW_FLOAT4
#define OSP_INT RTW_INT
#define OSP_INT2 RTW_INT2
#define OSP_INT3 RTW_INT3
#define OSP_INT4 RTW_INT4
#define OSP_UCHAR RTW_UCHAR
#define OSP_USHORT RTW_USHORT
#define OSP_SHORT RTW_SHORT
#define OSP_DOUBLE RTW_DOUBLE
#define OSP_OBJECT RTW_OBJECT
#define OSP_RAW RTW_RAW
#define OSP_UNKNOWN RTW_UNKNOWN

#define OSP_DATA_SHARED_BUFFER RTW_DATA_SHARED_BUFFER
#define OSP_TEXTURE_FILTER_NEAREST RTW_TEXTURE_FILTER_NEAREST
#define OSP_TEXTURE_SHARED_BUFFER RTW_TEXTURE_SHARED_BUFFER

#define OSP_TEXTURE_RGB8 RTW_TEXTURE_RGB8
#define OSP_TEXTURE_R32F RTW_TEXTURE_R32F
#define OSP_TEXTURE_RGB32F RTW_TEXTURE_RGB32F
#define OSP_TEXTURE_RGBA32F RTW_TEXTURE_RGBA32F
#define OSP_TEXTURE_R8 RTW_TEXTURE_R8
#define OSP_TEXTURE_RGB8 RTW_TEXTURE_RGB8
#define OSP_TEXTURE_RGBA8 RTW_TEXTURE_RGBA8

#define OSP_FB_RGBA32F RTW_FB_RGBA32F
#define OSP_FB_RGBA8 RTW_FB_RGBA8
#define OSP_FB_COLOR RTW_FB_COLOR
#define OSP_FB_DEPTH RTW_FB_DEPTH
#define OSP_FB_ACCUM RTW_FB_ACCUM
#define OSP_FB_NORMAL RTW_FB_NORMAL
#define OSP_FB_ALBEDO RTW_FB_ALBEDO

#define ospSet1f backend->Set1f
#define ospSetf backend->Setf
#define ospSet1i backend->Set1i
#define ospSet2i backend->Set2i
#define ospSet3i backend->Set3i
#define ospSet2f backend->Set2f
#define ospSetVec2f backend->SetVec2f
#define ospSet3f backend->Set3f
#define ospSet3fv backend->Set3fv
#define ospSet4f backend->Set4f
#define ospSetString backend->SetString
#define ospSetData backend->SetData
#define ospSetObject backend->SetObject
#define ospSetMaterial backend->SetMaterial
#define ospSetRegion backend->SetRegion

#define ospRemoveParam backend->RemoveParam

#define ospCommit backend->Commit
#define ospRelease backend->Release

#define ospNewData backend->NewData
#define ospNewCamera backend->NewCamera
#define ospNewLight3 backend->NewLight3
#define ospNewTexture backend->NewTexture
#define ospNewMaterial2 backend->NewMaterial2
#define ospNewTransferFunction backend->NewTransferFunction
#define ospNewVolume backend->NewVolume
#define ospNewGeometry backend->NewGeometry
#define ospNewModel backend->NewModel
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
