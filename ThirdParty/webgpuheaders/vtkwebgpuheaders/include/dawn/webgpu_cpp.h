// Copyright 2017 The Dawn & Tint Authors
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


#ifdef __EMSCRIPTEN__
#error "Do not include this header. Use the headers provided by Emdawnwebgpu instead."
#endif

#ifndef WEBGPU_CPP_H_
#define WEBGPU_CPP_H_

#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <functional>
#include <span>
#include <string_view>
#include <type_traits>
#include <utility>

#include "webgpu/webgpu.h"
#include "webgpu/webgpu_cpp_chained_struct.h"
#include "webgpu/webgpu_enum_class_bitmasks.h"  // IWYU pragma: export

namespace wgpu {

inline constexpr uint32_t kArrayLayerCountUndefined = WGPU_ARRAY_LAYER_COUNT_UNDEFINED;
inline constexpr uint32_t kCopyStrideUndefined = WGPU_COPY_STRIDE_UNDEFINED;
inline constexpr float kDepthClearValueUndefined = std::numeric_limits<float>::quiet_NaN();
inline constexpr uint32_t kDepthSliceUndefined = WGPU_DEPTH_SLICE_UNDEFINED;
inline constexpr uint32_t kInvalidBinding = WGPU_INVALID_BINDING;
inline constexpr uint32_t kLimitU32Undefined = WGPU_LIMIT_U32_UNDEFINED;
inline constexpr uint64_t kLimitU64Undefined = WGPU_LIMIT_U64_UNDEFINED;
inline constexpr uint32_t kMipLevelCountUndefined = WGPU_MIP_LEVEL_COUNT_UNDEFINED;
inline constexpr uint32_t kQuerySetIndexUndefined = WGPU_QUERY_SET_INDEX_UNDEFINED;
inline constexpr size_t kStrlen = WGPU_STRLEN;
inline constexpr size_t kWholeMapSize = WGPU_WHOLE_MAP_SIZE;
inline constexpr uint64_t kWholeSize = WGPU_WHOLE_SIZE;

enum class AdapterType : uint32_t {
    DiscreteGPU = WGPUAdapterType_DiscreteGPU,
    IntegratedGPU = WGPUAdapterType_IntegratedGPU,
    CPU = WGPUAdapterType_CPU,
    Unknown = WGPUAdapterType_Unknown,
};
static_assert(sizeof(AdapterType) == sizeof(WGPUAdapterType), "sizeof mismatch for AdapterType");
static_assert(alignof(AdapterType) == alignof(WGPUAdapterType), "alignof mismatch for AdapterType");

enum class AddressMode : uint32_t {
    Undefined = WGPUAddressMode_Undefined,
    ClampToEdge = WGPUAddressMode_ClampToEdge,
    Repeat = WGPUAddressMode_Repeat,
    MirrorRepeat = WGPUAddressMode_MirrorRepeat,
};
static_assert(sizeof(AddressMode) == sizeof(WGPUAddressMode), "sizeof mismatch for AddressMode");
static_assert(alignof(AddressMode) == alignof(WGPUAddressMode), "alignof mismatch for AddressMode");

enum class AlphaMode : uint32_t {
    Opaque = WGPUAlphaMode_Opaque,
    Premultiplied = WGPUAlphaMode_Premultiplied,
    Unpremultiplied = WGPUAlphaMode_Unpremultiplied,
};
static_assert(sizeof(AlphaMode) == sizeof(WGPUAlphaMode), "sizeof mismatch for AlphaMode");
static_assert(alignof(AlphaMode) == alignof(WGPUAlphaMode), "alignof mismatch for AlphaMode");

enum class BackendType : uint32_t {
    Undefined = WGPUBackendType_Undefined,
    Null = WGPUBackendType_Null,
    WebGPU = WGPUBackendType_WebGPU,
    D3D11 = WGPUBackendType_D3D11,
    D3D12 = WGPUBackendType_D3D12,
    Metal = WGPUBackendType_Metal,
    Vulkan = WGPUBackendType_Vulkan,
    OpenGL = WGPUBackendType_OpenGL,
    OpenGLES = WGPUBackendType_OpenGLES,
};
static_assert(sizeof(BackendType) == sizeof(WGPUBackendType), "sizeof mismatch for BackendType");
static_assert(alignof(BackendType) == alignof(WGPUBackendType), "alignof mismatch for BackendType");

enum class BlendFactor : uint32_t {
    Undefined = WGPUBlendFactor_Undefined,
    Zero = WGPUBlendFactor_Zero,
    One = WGPUBlendFactor_One,
    Src = WGPUBlendFactor_Src,
    OneMinusSrc = WGPUBlendFactor_OneMinusSrc,
    SrcAlpha = WGPUBlendFactor_SrcAlpha,
    OneMinusSrcAlpha = WGPUBlendFactor_OneMinusSrcAlpha,
    Dst = WGPUBlendFactor_Dst,
    OneMinusDst = WGPUBlendFactor_OneMinusDst,
    DstAlpha = WGPUBlendFactor_DstAlpha,
    OneMinusDstAlpha = WGPUBlendFactor_OneMinusDstAlpha,
    SrcAlphaSaturated = WGPUBlendFactor_SrcAlphaSaturated,
    Constant = WGPUBlendFactor_Constant,
    OneMinusConstant = WGPUBlendFactor_OneMinusConstant,
    Src1 = WGPUBlendFactor_Src1,
    OneMinusSrc1 = WGPUBlendFactor_OneMinusSrc1,
    Src1Alpha = WGPUBlendFactor_Src1Alpha,
    OneMinusSrc1Alpha = WGPUBlendFactor_OneMinusSrc1Alpha,
};
static_assert(sizeof(BlendFactor) == sizeof(WGPUBlendFactor), "sizeof mismatch for BlendFactor");
static_assert(alignof(BlendFactor) == alignof(WGPUBlendFactor), "alignof mismatch for BlendFactor");

enum class BlendOperation : uint32_t {
    Undefined = WGPUBlendOperation_Undefined,
    Add = WGPUBlendOperation_Add,
    Subtract = WGPUBlendOperation_Subtract,
    ReverseSubtract = WGPUBlendOperation_ReverseSubtract,
    Min = WGPUBlendOperation_Min,
    Max = WGPUBlendOperation_Max,
};
static_assert(sizeof(BlendOperation) == sizeof(WGPUBlendOperation), "sizeof mismatch for BlendOperation");
static_assert(alignof(BlendOperation) == alignof(WGPUBlendOperation), "alignof mismatch for BlendOperation");

enum class BufferBindingType : uint32_t {
    BindingNotUsed = WGPUBufferBindingType_BindingNotUsed,
    Undefined = WGPUBufferBindingType_Undefined,
    Uniform = WGPUBufferBindingType_Uniform,
    Storage = WGPUBufferBindingType_Storage,
    ReadOnlyStorage = WGPUBufferBindingType_ReadOnlyStorage,
};
static_assert(sizeof(BufferBindingType) == sizeof(WGPUBufferBindingType), "sizeof mismatch for BufferBindingType");
static_assert(alignof(BufferBindingType) == alignof(WGPUBufferBindingType), "alignof mismatch for BufferBindingType");

enum class BufferMapState : uint32_t {
    Unmapped = WGPUBufferMapState_Unmapped,
    Pending = WGPUBufferMapState_Pending,
    Mapped = WGPUBufferMapState_Mapped,
};
static_assert(sizeof(BufferMapState) == sizeof(WGPUBufferMapState), "sizeof mismatch for BufferMapState");
static_assert(alignof(BufferMapState) == alignof(WGPUBufferMapState), "alignof mismatch for BufferMapState");

enum class CallbackMode : uint32_t {
    WaitAnyOnly = WGPUCallbackMode_WaitAnyOnly,
    AllowProcessEvents = WGPUCallbackMode_AllowProcessEvents,
    AllowSpontaneous = WGPUCallbackMode_AllowSpontaneous,
};
static_assert(sizeof(CallbackMode) == sizeof(WGPUCallbackMode), "sizeof mismatch for CallbackMode");
static_assert(alignof(CallbackMode) == alignof(WGPUCallbackMode), "alignof mismatch for CallbackMode");

enum class ColorSpacePrimariesDawn : uint32_t {
    SRGB = WGPUColorSpacePrimariesDawn_SRGB,
    Rec709 = WGPUColorSpacePrimariesDawn_Rec709,
    Rec601 = WGPUColorSpacePrimariesDawn_Rec601,
    Rec2020 = WGPUColorSpacePrimariesDawn_Rec2020,
    DisplayP3 = WGPUColorSpacePrimariesDawn_DisplayP3,
};
static_assert(sizeof(ColorSpacePrimariesDawn) == sizeof(WGPUColorSpacePrimariesDawn), "sizeof mismatch for ColorSpacePrimariesDawn");
static_assert(alignof(ColorSpacePrimariesDawn) == alignof(WGPUColorSpacePrimariesDawn), "alignof mismatch for ColorSpacePrimariesDawn");

enum class ColorSpaceTransferDawn : uint32_t {
    Identity = WGPUColorSpaceTransferDawn_Identity,
    SRGB = WGPUColorSpaceTransferDawn_SRGB,
    DisplayP3 = WGPUColorSpaceTransferDawn_DisplayP3,
    SMPTE_170M = WGPUColorSpaceTransferDawn_SMPTE_170M,
    HLG = WGPUColorSpaceTransferDawn_HLG,
    PQ = WGPUColorSpaceTransferDawn_PQ,
};
static_assert(sizeof(ColorSpaceTransferDawn) == sizeof(WGPUColorSpaceTransferDawn), "sizeof mismatch for ColorSpaceTransferDawn");
static_assert(alignof(ColorSpaceTransferDawn) == alignof(WGPUColorSpaceTransferDawn), "alignof mismatch for ColorSpaceTransferDawn");

enum class ColorSpaceYCbCrMatrixDawn : uint32_t {
    Identity = WGPUColorSpaceYCbCrMatrixDawn_Identity,
    Rec601 = WGPUColorSpaceYCbCrMatrixDawn_Rec601,
    Rec709 = WGPUColorSpaceYCbCrMatrixDawn_Rec709,
    Rec2020 = WGPUColorSpaceYCbCrMatrixDawn_Rec2020,
};
static_assert(sizeof(ColorSpaceYCbCrMatrixDawn) == sizeof(WGPUColorSpaceYCbCrMatrixDawn), "sizeof mismatch for ColorSpaceYCbCrMatrixDawn");
static_assert(alignof(ColorSpaceYCbCrMatrixDawn) == alignof(WGPUColorSpaceYCbCrMatrixDawn), "alignof mismatch for ColorSpaceYCbCrMatrixDawn");

enum class ColorSpaceYCbCrRangeDawn : uint32_t {
    Identity = WGPUColorSpaceYCbCrRangeDawn_Identity,
    Narrow = WGPUColorSpaceYCbCrRangeDawn_Narrow,
    Full = WGPUColorSpaceYCbCrRangeDawn_Full,
};
static_assert(sizeof(ColorSpaceYCbCrRangeDawn) == sizeof(WGPUColorSpaceYCbCrRangeDawn), "sizeof mismatch for ColorSpaceYCbCrRangeDawn");
static_assert(alignof(ColorSpaceYCbCrRangeDawn) == alignof(WGPUColorSpaceYCbCrRangeDawn), "alignof mismatch for ColorSpaceYCbCrRangeDawn");

enum class CompareFunction : uint32_t {
    Undefined = WGPUCompareFunction_Undefined,
    Never = WGPUCompareFunction_Never,
    Less = WGPUCompareFunction_Less,
    Equal = WGPUCompareFunction_Equal,
    LessEqual = WGPUCompareFunction_LessEqual,
    Greater = WGPUCompareFunction_Greater,
    NotEqual = WGPUCompareFunction_NotEqual,
    GreaterEqual = WGPUCompareFunction_GreaterEqual,
    Always = WGPUCompareFunction_Always,
};
static_assert(sizeof(CompareFunction) == sizeof(WGPUCompareFunction), "sizeof mismatch for CompareFunction");
static_assert(alignof(CompareFunction) == alignof(WGPUCompareFunction), "alignof mismatch for CompareFunction");

enum class CompilationInfoRequestStatus : uint32_t {
    Success = WGPUCompilationInfoRequestStatus_Success,
    CallbackCancelled = WGPUCompilationInfoRequestStatus_CallbackCancelled,
};
static_assert(sizeof(CompilationInfoRequestStatus) == sizeof(WGPUCompilationInfoRequestStatus), "sizeof mismatch for CompilationInfoRequestStatus");
static_assert(alignof(CompilationInfoRequestStatus) == alignof(WGPUCompilationInfoRequestStatus), "alignof mismatch for CompilationInfoRequestStatus");

enum class CompilationMessageType : uint32_t {
    Error = WGPUCompilationMessageType_Error,
    Warning = WGPUCompilationMessageType_Warning,
    Info = WGPUCompilationMessageType_Info,
};
static_assert(sizeof(CompilationMessageType) == sizeof(WGPUCompilationMessageType), "sizeof mismatch for CompilationMessageType");
static_assert(alignof(CompilationMessageType) == alignof(WGPUCompilationMessageType), "alignof mismatch for CompilationMessageType");

enum class ComponentSwizzle : uint32_t {
    Undefined = WGPUComponentSwizzle_Undefined,
    Zero = WGPUComponentSwizzle_Zero,
    One = WGPUComponentSwizzle_One,
    R = WGPUComponentSwizzle_R,
    G = WGPUComponentSwizzle_G,
    B = WGPUComponentSwizzle_B,
    A = WGPUComponentSwizzle_A,
};
static_assert(sizeof(ComponentSwizzle) == sizeof(WGPUComponentSwizzle), "sizeof mismatch for ComponentSwizzle");
static_assert(alignof(ComponentSwizzle) == alignof(WGPUComponentSwizzle), "alignof mismatch for ComponentSwizzle");

enum class CompositeAlphaMode : uint32_t {
    Auto = WGPUCompositeAlphaMode_Auto,
    Opaque = WGPUCompositeAlphaMode_Opaque,
    Premultiplied = WGPUCompositeAlphaMode_Premultiplied,
    Unpremultiplied = WGPUCompositeAlphaMode_Unpremultiplied,
    Inherit = WGPUCompositeAlphaMode_Inherit,
};
static_assert(sizeof(CompositeAlphaMode) == sizeof(WGPUCompositeAlphaMode), "sizeof mismatch for CompositeAlphaMode");
static_assert(alignof(CompositeAlphaMode) == alignof(WGPUCompositeAlphaMode), "alignof mismatch for CompositeAlphaMode");

enum class CreatePipelineAsyncStatus : uint32_t {
    Success = WGPUCreatePipelineAsyncStatus_Success,
    CallbackCancelled = WGPUCreatePipelineAsyncStatus_CallbackCancelled,
    ValidationError = WGPUCreatePipelineAsyncStatus_ValidationError,
    InternalError = WGPUCreatePipelineAsyncStatus_InternalError,
};
static_assert(sizeof(CreatePipelineAsyncStatus) == sizeof(WGPUCreatePipelineAsyncStatus), "sizeof mismatch for CreatePipelineAsyncStatus");
static_assert(alignof(CreatePipelineAsyncStatus) == alignof(WGPUCreatePipelineAsyncStatus), "alignof mismatch for CreatePipelineAsyncStatus");

enum class CullMode : uint32_t {
    Undefined = WGPUCullMode_Undefined,
    None = WGPUCullMode_None,
    Front = WGPUCullMode_Front,
    Back = WGPUCullMode_Back,
};
static_assert(sizeof(CullMode) == sizeof(WGPUCullMode), "sizeof mismatch for CullMode");
static_assert(alignof(CullMode) == alignof(WGPUCullMode), "alignof mismatch for CullMode");

enum class DeviceLostReason : uint32_t {
    Unknown = WGPUDeviceLostReason_Unknown,
    Destroyed = WGPUDeviceLostReason_Destroyed,
    CallbackCancelled = WGPUDeviceLostReason_CallbackCancelled,
    FailedCreation = WGPUDeviceLostReason_FailedCreation,
};
static_assert(sizeof(DeviceLostReason) == sizeof(WGPUDeviceLostReason), "sizeof mismatch for DeviceLostReason");
static_assert(alignof(DeviceLostReason) == alignof(WGPUDeviceLostReason), "alignof mismatch for DeviceLostReason");

enum class ErrorFilter : uint32_t {
    Validation = WGPUErrorFilter_Validation,
    OutOfMemory = WGPUErrorFilter_OutOfMemory,
    Internal = WGPUErrorFilter_Internal,
};
static_assert(sizeof(ErrorFilter) == sizeof(WGPUErrorFilter), "sizeof mismatch for ErrorFilter");
static_assert(alignof(ErrorFilter) == alignof(WGPUErrorFilter), "alignof mismatch for ErrorFilter");

enum class ErrorType : uint32_t {
    NoError = WGPUErrorType_NoError,
    Validation = WGPUErrorType_Validation,
    OutOfMemory = WGPUErrorType_OutOfMemory,
    Internal = WGPUErrorType_Internal,
    Unknown = WGPUErrorType_Unknown,
};
static_assert(sizeof(ErrorType) == sizeof(WGPUErrorType), "sizeof mismatch for ErrorType");
static_assert(alignof(ErrorType) == alignof(WGPUErrorType), "alignof mismatch for ErrorType");

enum class ExternalTextureRotation : uint32_t {
    Rotate0Degrees = WGPUExternalTextureRotation_Rotate0Degrees,
    Rotate90Degrees = WGPUExternalTextureRotation_Rotate90Degrees,
    Rotate180Degrees = WGPUExternalTextureRotation_Rotate180Degrees,
    Rotate270Degrees = WGPUExternalTextureRotation_Rotate270Degrees,
};
static_assert(sizeof(ExternalTextureRotation) == sizeof(WGPUExternalTextureRotation), "sizeof mismatch for ExternalTextureRotation");
static_assert(alignof(ExternalTextureRotation) == alignof(WGPUExternalTextureRotation), "alignof mismatch for ExternalTextureRotation");

enum class FeatureLevel : uint32_t {
    Undefined = WGPUFeatureLevel_Undefined,
    Compatibility = WGPUFeatureLevel_Compatibility,
    Core = WGPUFeatureLevel_Core,
};
static_assert(sizeof(FeatureLevel) == sizeof(WGPUFeatureLevel), "sizeof mismatch for FeatureLevel");
static_assert(alignof(FeatureLevel) == alignof(WGPUFeatureLevel), "alignof mismatch for FeatureLevel");

enum class FeatureName : uint32_t {
    CoreFeaturesAndLimits = WGPUFeatureName_CoreFeaturesAndLimits,
    DepthClipControl = WGPUFeatureName_DepthClipControl,
    Depth32FloatStencil8 = WGPUFeatureName_Depth32FloatStencil8,
    TextureCompressionBC = WGPUFeatureName_TextureCompressionBC,
    TextureCompressionBCSliced3D = WGPUFeatureName_TextureCompressionBCSliced3D,
    TextureCompressionETC2 = WGPUFeatureName_TextureCompressionETC2,
    TextureCompressionASTC = WGPUFeatureName_TextureCompressionASTC,
    TextureCompressionASTCSliced3D = WGPUFeatureName_TextureCompressionASTCSliced3D,
    TimestampQuery = WGPUFeatureName_TimestampQuery,
    IndirectFirstInstance = WGPUFeatureName_IndirectFirstInstance,
    ShaderF16 = WGPUFeatureName_ShaderF16,
    RG11B10UfloatRenderable = WGPUFeatureName_RG11B10UfloatRenderable,
    BGRA8UnormStorage = WGPUFeatureName_BGRA8UnormStorage,
    Float32Filterable = WGPUFeatureName_Float32Filterable,
    Float32Blendable = WGPUFeatureName_Float32Blendable,
    ClipDistances = WGPUFeatureName_ClipDistances,
    DualSourceBlending = WGPUFeatureName_DualSourceBlending,
    Subgroups = WGPUFeatureName_Subgroups,
    TextureFormatsTier1 = WGPUFeatureName_TextureFormatsTier1,
    TextureFormatsTier2 = WGPUFeatureName_TextureFormatsTier2,
    PrimitiveIndex = WGPUFeatureName_PrimitiveIndex,
    TextureComponentSwizzle = WGPUFeatureName_TextureComponentSwizzle,
    SubgroupSizeControl = WGPUFeatureName_SubgroupSizeControl,
    DawnInternalUsages = WGPUFeatureName_DawnInternalUsages,
    DawnMultiPlanarFormats = WGPUFeatureName_DawnMultiPlanarFormats,
    DawnNative = WGPUFeatureName_DawnNative,
    ChromiumExperimentalTimestampQueryInsidePasses = WGPUFeatureName_ChromiumExperimentalTimestampQueryInsidePasses,
    ImplicitDeviceSynchronization = WGPUFeatureName_ImplicitDeviceSynchronization,
    TransientAttachments = WGPUFeatureName_TransientAttachments,
    MSAARenderToSingleSampled = WGPUFeatureName_MSAARenderToSingleSampled,
    D3D11MultithreadProtected = WGPUFeatureName_D3D11MultithreadProtected,
    ANGLETextureSharing = WGPUFeatureName_ANGLETextureSharing,
    PixelLocalStorageCoherent = WGPUFeatureName_PixelLocalStorageCoherent,
    PixelLocalStorageNonCoherent = WGPUFeatureName_PixelLocalStorageNonCoherent,
    Unorm16TextureFormats = WGPUFeatureName_Unorm16TextureFormats,
    MultiPlanarFormatExtendedUsages = WGPUFeatureName_MultiPlanarFormatExtendedUsages,
    MultiPlanarFormatP010 = WGPUFeatureName_MultiPlanarFormatP010,
    HostMappedPointer = WGPUFeatureName_HostMappedPointer,
    MultiPlanarRenderTargets = WGPUFeatureName_MultiPlanarRenderTargets,
    MultiPlanarFormatNv12a = WGPUFeatureName_MultiPlanarFormatNv12a,
    FramebufferFetch = WGPUFeatureName_FramebufferFetch,
    BufferMapExtendedUsages = WGPUFeatureName_BufferMapExtendedUsages,
    AdapterPropertiesMemoryHeaps = WGPUFeatureName_AdapterPropertiesMemoryHeaps,
    AdapterPropertiesD3D = WGPUFeatureName_AdapterPropertiesD3D,
    AdapterPropertiesVk = WGPUFeatureName_AdapterPropertiesVk,
    DawnFormatCapabilities = WGPUFeatureName_DawnFormatCapabilities,
    DawnDrmFormatCapabilities = WGPUFeatureName_DawnDrmFormatCapabilities,
    MultiPlanarFormatNv16 = WGPUFeatureName_MultiPlanarFormatNv16,
    MultiPlanarFormatNv24 = WGPUFeatureName_MultiPlanarFormatNv24,
    MultiPlanarFormatP210 = WGPUFeatureName_MultiPlanarFormatP210,
    MultiPlanarFormatP410 = WGPUFeatureName_MultiPlanarFormatP410,
    SharedTextureMemoryVkDedicatedAllocation = WGPUFeatureName_SharedTextureMemoryVkDedicatedAllocation,
    SharedTextureMemoryAHardwareBuffer = WGPUFeatureName_SharedTextureMemoryAHardwareBuffer,
    SharedTextureMemoryDmaBuf = WGPUFeatureName_SharedTextureMemoryDmaBuf,
    SharedTextureMemoryOpaqueFD = WGPUFeatureName_SharedTextureMemoryOpaqueFD,
    SharedTextureMemoryZirconHandle = WGPUFeatureName_SharedTextureMemoryZirconHandle,
    SharedTextureMemoryDXGISharedHandle = WGPUFeatureName_SharedTextureMemoryDXGISharedHandle,
    SharedTextureMemoryD3D11Texture2D = WGPUFeatureName_SharedTextureMemoryD3D11Texture2D,
    SharedTextureMemoryIOSurface = WGPUFeatureName_SharedTextureMemoryIOSurface,
    SharedTextureMemoryEGLImage = WGPUFeatureName_SharedTextureMemoryEGLImage,
    SharedFenceVkSemaphoreOpaqueFD = WGPUFeatureName_SharedFenceVkSemaphoreOpaqueFD,
    SharedFenceSyncFD = WGPUFeatureName_SharedFenceSyncFD,
    SharedFenceVkSemaphoreZirconHandle = WGPUFeatureName_SharedFenceVkSemaphoreZirconHandle,
    SharedFenceDXGISharedHandle = WGPUFeatureName_SharedFenceDXGISharedHandle,
    SharedFenceMTLSharedEvent = WGPUFeatureName_SharedFenceMTLSharedEvent,
    SharedBufferMemoryD3D12Resource = WGPUFeatureName_SharedBufferMemoryD3D12Resource,
    StaticSamplers = WGPUFeatureName_StaticSamplers,
    YCbCrVulkanSamplers = WGPUFeatureName_YCbCrVulkanSamplers,
    ShaderModuleCompilationOptions = WGPUFeatureName_ShaderModuleCompilationOptions,
    DawnLoadResolveTexture = WGPUFeatureName_DawnLoadResolveTexture,
    DawnPartialLoadResolveTexture = WGPUFeatureName_DawnPartialLoadResolveTexture,
    MultiDrawIndirect = WGPUFeatureName_MultiDrawIndirect,
    DawnTexelCopyBufferRowAlignment = WGPUFeatureName_DawnTexelCopyBufferRowAlignment,
    FlexibleTextureViews = WGPUFeatureName_FlexibleTextureViews,
    ChromiumExperimentalSubgroupMatrix = WGPUFeatureName_ChromiumExperimentalSubgroupMatrix,
    SharedFenceEGLSync = WGPUFeatureName_SharedFenceEGLSync,
    DawnDeviceAllocatorControl = WGPUFeatureName_DawnDeviceAllocatorControl,
    AdapterPropertiesWGPU = WGPUFeatureName_AdapterPropertiesWGPU,
    SharedBufferMemoryFromWindowsHandle = WGPUFeatureName_SharedBufferMemoryFromWindowsHandle,
    SharedTextureMemoryD3D12Resource = WGPUFeatureName_SharedTextureMemoryD3D12Resource,
    ChromiumExperimentalSamplingResourceTable = WGPUFeatureName_ChromiumExperimentalSamplingResourceTable,
    AtomicVec2uMinMax = WGPUFeatureName_AtomicVec2uMinMax,
    Unorm16FormatsForExternalTexture = WGPUFeatureName_Unorm16FormatsForExternalTexture,
    OpaqueYCbCrAndroidForExternalTexture = WGPUFeatureName_OpaqueYCbCrAndroidForExternalTexture,
    Unorm16Filterable = WGPUFeatureName_Unorm16Filterable,
    RenderPassRenderArea = WGPUFeatureName_RenderPassRenderArea,
    AdapterPropertiesDrm = WGPUFeatureName_AdapterPropertiesDrm,
    TextureCompressionUnaligned = WGPUFeatureName_TextureCompressionUnaligned,
};
static_assert(sizeof(FeatureName) == sizeof(WGPUFeatureName), "sizeof mismatch for FeatureName");
static_assert(alignof(FeatureName) == alignof(WGPUFeatureName), "alignof mismatch for FeatureName");

enum class FilterMode : uint32_t {
    Undefined = WGPUFilterMode_Undefined,
    Nearest = WGPUFilterMode_Nearest,
    Linear = WGPUFilterMode_Linear,
};
static_assert(sizeof(FilterMode) == sizeof(WGPUFilterMode), "sizeof mismatch for FilterMode");
static_assert(alignof(FilterMode) == alignof(WGPUFilterMode), "alignof mismatch for FilterMode");

enum class FrontFace : uint32_t {
    Undefined = WGPUFrontFace_Undefined,
    CCW = WGPUFrontFace_CCW,
    CW = WGPUFrontFace_CW,
};
static_assert(sizeof(FrontFace) == sizeof(WGPUFrontFace), "sizeof mismatch for FrontFace");
static_assert(alignof(FrontFace) == alignof(WGPUFrontFace), "alignof mismatch for FrontFace");

enum class IndexFormat : uint32_t {
    Undefined = WGPUIndexFormat_Undefined,
    Uint16 = WGPUIndexFormat_Uint16,
    Uint32 = WGPUIndexFormat_Uint32,
};
static_assert(sizeof(IndexFormat) == sizeof(WGPUIndexFormat), "sizeof mismatch for IndexFormat");
static_assert(alignof(IndexFormat) == alignof(WGPUIndexFormat), "alignof mismatch for IndexFormat");

enum class InstanceFeatureName : uint32_t {
    TimedWaitAny = WGPUInstanceFeatureName_TimedWaitAny,
    ShaderSourceSPIRV = WGPUInstanceFeatureName_ShaderSourceSPIRV,
    MultipleDevicesPerAdapter = WGPUInstanceFeatureName_MultipleDevicesPerAdapter,
};
static_assert(sizeof(InstanceFeatureName) == sizeof(WGPUInstanceFeatureName), "sizeof mismatch for InstanceFeatureName");
static_assert(alignof(InstanceFeatureName) == alignof(WGPUInstanceFeatureName), "alignof mismatch for InstanceFeatureName");

enum class LoadOp : uint32_t {
    Undefined = WGPULoadOp_Undefined,
    Load = WGPULoadOp_Load,
    Clear = WGPULoadOp_Clear,
    ExpandResolveTexture = WGPULoadOp_ExpandResolveTexture,
};
static_assert(sizeof(LoadOp) == sizeof(WGPULoadOp), "sizeof mismatch for LoadOp");
static_assert(alignof(LoadOp) == alignof(WGPULoadOp), "alignof mismatch for LoadOp");

enum class LoggingType : uint32_t {
    Verbose = WGPULoggingType_Verbose,
    Info = WGPULoggingType_Info,
    Warning = WGPULoggingType_Warning,
    Error = WGPULoggingType_Error,
};
static_assert(sizeof(LoggingType) == sizeof(WGPULoggingType), "sizeof mismatch for LoggingType");
static_assert(alignof(LoggingType) == alignof(WGPULoggingType), "alignof mismatch for LoggingType");

enum class MapAsyncStatus : uint32_t {
    Success = WGPUMapAsyncStatus_Success,
    CallbackCancelled = WGPUMapAsyncStatus_CallbackCancelled,
    Error = WGPUMapAsyncStatus_Error,
    Aborted = WGPUMapAsyncStatus_Aborted,
};
static_assert(sizeof(MapAsyncStatus) == sizeof(WGPUMapAsyncStatus), "sizeof mismatch for MapAsyncStatus");
static_assert(alignof(MapAsyncStatus) == alignof(WGPUMapAsyncStatus), "alignof mismatch for MapAsyncStatus");

enum class MipmapFilterMode : uint32_t {
    Undefined = WGPUMipmapFilterMode_Undefined,
    Nearest = WGPUMipmapFilterMode_Nearest,
    Linear = WGPUMipmapFilterMode_Linear,
};
static_assert(sizeof(MipmapFilterMode) == sizeof(WGPUMipmapFilterMode), "sizeof mismatch for MipmapFilterMode");
static_assert(alignof(MipmapFilterMode) == alignof(WGPUMipmapFilterMode), "alignof mismatch for MipmapFilterMode");

enum class PopErrorScopeStatus : uint32_t {
    Success = WGPUPopErrorScopeStatus_Success,
    CallbackCancelled = WGPUPopErrorScopeStatus_CallbackCancelled,
    Error = WGPUPopErrorScopeStatus_Error,
};
static_assert(sizeof(PopErrorScopeStatus) == sizeof(WGPUPopErrorScopeStatus), "sizeof mismatch for PopErrorScopeStatus");
static_assert(alignof(PopErrorScopeStatus) == alignof(WGPUPopErrorScopeStatus), "alignof mismatch for PopErrorScopeStatus");

enum class PowerPreference : uint32_t {
    Undefined = WGPUPowerPreference_Undefined,
    LowPower = WGPUPowerPreference_LowPower,
    HighPerformance = WGPUPowerPreference_HighPerformance,
};
static_assert(sizeof(PowerPreference) == sizeof(WGPUPowerPreference), "sizeof mismatch for PowerPreference");
static_assert(alignof(PowerPreference) == alignof(WGPUPowerPreference), "alignof mismatch for PowerPreference");

enum class PredefinedColorSpace : uint32_t {
    SRGB = WGPUPredefinedColorSpace_SRGB,
    DisplayP3 = WGPUPredefinedColorSpace_DisplayP3,
    SRGBLinear = WGPUPredefinedColorSpace_SRGBLinear,
    DisplayP3Linear = WGPUPredefinedColorSpace_DisplayP3Linear,
    Rec2020Linear = WGPUPredefinedColorSpace_Rec2020Linear,
};
static_assert(sizeof(PredefinedColorSpace) == sizeof(WGPUPredefinedColorSpace), "sizeof mismatch for PredefinedColorSpace");
static_assert(alignof(PredefinedColorSpace) == alignof(WGPUPredefinedColorSpace), "alignof mismatch for PredefinedColorSpace");

enum class PresentMode : uint32_t {
    Undefined = WGPUPresentMode_Undefined,
    Fifo = WGPUPresentMode_Fifo,
    FifoRelaxed = WGPUPresentMode_FifoRelaxed,
    Immediate = WGPUPresentMode_Immediate,
    Mailbox = WGPUPresentMode_Mailbox,
};
static_assert(sizeof(PresentMode) == sizeof(WGPUPresentMode), "sizeof mismatch for PresentMode");
static_assert(alignof(PresentMode) == alignof(WGPUPresentMode), "alignof mismatch for PresentMode");

enum class PrimitiveTopology : uint32_t {
    Undefined = WGPUPrimitiveTopology_Undefined,
    PointList = WGPUPrimitiveTopology_PointList,
    LineList = WGPUPrimitiveTopology_LineList,
    LineStrip = WGPUPrimitiveTopology_LineStrip,
    TriangleList = WGPUPrimitiveTopology_TriangleList,
    TriangleStrip = WGPUPrimitiveTopology_TriangleStrip,
};
static_assert(sizeof(PrimitiveTopology) == sizeof(WGPUPrimitiveTopology), "sizeof mismatch for PrimitiveTopology");
static_assert(alignof(PrimitiveTopology) == alignof(WGPUPrimitiveTopology), "alignof mismatch for PrimitiveTopology");

enum class QueryType : uint32_t {
    Occlusion = WGPUQueryType_Occlusion,
    Timestamp = WGPUQueryType_Timestamp,
};
static_assert(sizeof(QueryType) == sizeof(WGPUQueryType), "sizeof mismatch for QueryType");
static_assert(alignof(QueryType) == alignof(WGPUQueryType), "alignof mismatch for QueryType");

enum class QueueWorkDoneStatus : uint32_t {
    Success = WGPUQueueWorkDoneStatus_Success,
    CallbackCancelled = WGPUQueueWorkDoneStatus_CallbackCancelled,
    Error = WGPUQueueWorkDoneStatus_Error,
};
static_assert(sizeof(QueueWorkDoneStatus) == sizeof(WGPUQueueWorkDoneStatus), "sizeof mismatch for QueueWorkDoneStatus");
static_assert(alignof(QueueWorkDoneStatus) == alignof(WGPUQueueWorkDoneStatus), "alignof mismatch for QueueWorkDoneStatus");

enum class RequestAdapterStatus : uint32_t {
    Success = WGPURequestAdapterStatus_Success,
    CallbackCancelled = WGPURequestAdapterStatus_CallbackCancelled,
    Unavailable = WGPURequestAdapterStatus_Unavailable,
    Error = WGPURequestAdapterStatus_Error,
};
static_assert(sizeof(RequestAdapterStatus) == sizeof(WGPURequestAdapterStatus), "sizeof mismatch for RequestAdapterStatus");
static_assert(alignof(RequestAdapterStatus) == alignof(WGPURequestAdapterStatus), "alignof mismatch for RequestAdapterStatus");

enum class RequestDeviceStatus : uint32_t {
    Success = WGPURequestDeviceStatus_Success,
    CallbackCancelled = WGPURequestDeviceStatus_CallbackCancelled,
    Error = WGPURequestDeviceStatus_Error,
};
static_assert(sizeof(RequestDeviceStatus) == sizeof(WGPURequestDeviceStatus), "sizeof mismatch for RequestDeviceStatus");
static_assert(alignof(RequestDeviceStatus) == alignof(WGPURequestDeviceStatus), "alignof mismatch for RequestDeviceStatus");

enum class SamplerBindingType : uint32_t {
    BindingNotUsed = WGPUSamplerBindingType_BindingNotUsed,
    Undefined = WGPUSamplerBindingType_Undefined,
    Filtering = WGPUSamplerBindingType_Filtering,
    NonFiltering = WGPUSamplerBindingType_NonFiltering,
    Comparison = WGPUSamplerBindingType_Comparison,
};
static_assert(sizeof(SamplerBindingType) == sizeof(WGPUSamplerBindingType), "sizeof mismatch for SamplerBindingType");
static_assert(alignof(SamplerBindingType) == alignof(WGPUSamplerBindingType), "alignof mismatch for SamplerBindingType");

enum class SharedFenceType : uint32_t {
    VkSemaphoreOpaqueFD = WGPUSharedFenceType_VkSemaphoreOpaqueFD,
    SyncFD = WGPUSharedFenceType_SyncFD,
    VkSemaphoreZirconHandle = WGPUSharedFenceType_VkSemaphoreZirconHandle,
    DXGISharedHandle = WGPUSharedFenceType_DXGISharedHandle,
    MTLSharedEvent = WGPUSharedFenceType_MTLSharedEvent,
    EGLSync = WGPUSharedFenceType_EGLSync,
};
static_assert(sizeof(SharedFenceType) == sizeof(WGPUSharedFenceType), "sizeof mismatch for SharedFenceType");
static_assert(alignof(SharedFenceType) == alignof(WGPUSharedFenceType), "alignof mismatch for SharedFenceType");

enum class Status : uint32_t {
    Success = WGPUStatus_Success,
    Error = WGPUStatus_Error,
};
static_assert(sizeof(Status) == sizeof(WGPUStatus), "sizeof mismatch for Status");
static_assert(alignof(Status) == alignof(WGPUStatus), "alignof mismatch for Status");

enum class StencilOperation : uint32_t {
    Undefined = WGPUStencilOperation_Undefined,
    Keep = WGPUStencilOperation_Keep,
    Zero = WGPUStencilOperation_Zero,
    Replace = WGPUStencilOperation_Replace,
    Invert = WGPUStencilOperation_Invert,
    IncrementClamp = WGPUStencilOperation_IncrementClamp,
    DecrementClamp = WGPUStencilOperation_DecrementClamp,
    IncrementWrap = WGPUStencilOperation_IncrementWrap,
    DecrementWrap = WGPUStencilOperation_DecrementWrap,
};
static_assert(sizeof(StencilOperation) == sizeof(WGPUStencilOperation), "sizeof mismatch for StencilOperation");
static_assert(alignof(StencilOperation) == alignof(WGPUStencilOperation), "alignof mismatch for StencilOperation");

enum class StorageTextureAccess : uint32_t {
    BindingNotUsed = WGPUStorageTextureAccess_BindingNotUsed,
    Undefined = WGPUStorageTextureAccess_Undefined,
    WriteOnly = WGPUStorageTextureAccess_WriteOnly,
    ReadOnly = WGPUStorageTextureAccess_ReadOnly,
    ReadWrite = WGPUStorageTextureAccess_ReadWrite,
};
static_assert(sizeof(StorageTextureAccess) == sizeof(WGPUStorageTextureAccess), "sizeof mismatch for StorageTextureAccess");
static_assert(alignof(StorageTextureAccess) == alignof(WGPUStorageTextureAccess), "alignof mismatch for StorageTextureAccess");

enum class StoreOp : uint32_t {
    Undefined = WGPUStoreOp_Undefined,
    Store = WGPUStoreOp_Store,
    Discard = WGPUStoreOp_Discard,
};
static_assert(sizeof(StoreOp) == sizeof(WGPUStoreOp), "sizeof mismatch for StoreOp");
static_assert(alignof(StoreOp) == alignof(WGPUStoreOp), "alignof mismatch for StoreOp");

enum class SType : uint32_t {
    ShaderSourceSPIRV = WGPUSType_ShaderSourceSPIRV,
    ShaderSourceWGSL = WGPUSType_ShaderSourceWGSL,
    RenderPassMaxDrawCount = WGPUSType_RenderPassMaxDrawCount,
    SurfaceSourceMetalLayer = WGPUSType_SurfaceSourceMetalLayer,
    SurfaceSourceWindowsHWND = WGPUSType_SurfaceSourceWindowsHWND,
    SurfaceSourceXlibWindow = WGPUSType_SurfaceSourceXlibWindow,
    SurfaceSourceWaylandSurface = WGPUSType_SurfaceSourceWaylandSurface,
    SurfaceSourceAndroidNativeWindow = WGPUSType_SurfaceSourceAndroidNativeWindow,
    SurfaceSourceXCBWindow = WGPUSType_SurfaceSourceXCBWindow,
    SurfaceColorManagement = WGPUSType_SurfaceColorManagement,
    RequestAdapterWebXROptions = WGPUSType_RequestAdapterWebXROptions,
    TextureComponentSwizzleDescriptor = WGPUSType_TextureComponentSwizzleDescriptor,
    ExternalTextureBindingLayout = WGPUSType_ExternalTextureBindingLayout,
    ExternalTextureBindingEntry = WGPUSType_ExternalTextureBindingEntry,
    CompatibilityModeLimits = WGPUSType_CompatibilityModeLimits,
    TextureBindingViewDimension = WGPUSType_TextureBindingViewDimension,
    EmscriptenSurfaceSourceCanvasHTMLSelector = WGPUSType_EmscriptenSurfaceSourceCanvasHTMLSelector,
    SurfaceDescriptorFromWindowsCoreWindow = WGPUSType_SurfaceDescriptorFromWindowsCoreWindow,
    SurfaceDescriptorFromWindowsUWPSwapChainPanel = WGPUSType_SurfaceDescriptorFromWindowsUWPSwapChainPanel,
    DawnTextureInternalUsageDescriptor = WGPUSType_DawnTextureInternalUsageDescriptor,
    DawnEncoderInternalUsageDescriptor = WGPUSType_DawnEncoderInternalUsageDescriptor,
    DawnInstanceDescriptor = WGPUSType_DawnInstanceDescriptor,
    DawnCacheDeviceDescriptor = WGPUSType_DawnCacheDeviceDescriptor,
    DawnAdapterPropertiesPowerPreference = WGPUSType_DawnAdapterPropertiesPowerPreference,
    DawnBufferDescriptorErrorInfoFromWireClient = WGPUSType_DawnBufferDescriptorErrorInfoFromWireClient,
    DawnTogglesDescriptor = WGPUSType_DawnTogglesDescriptor,
    DawnShaderModuleSPIRVOptionsDescriptor = WGPUSType_DawnShaderModuleSPIRVOptionsDescriptor,
    RequestAdapterOptionsLUID = WGPUSType_RequestAdapterOptionsLUID,
    RequestAdapterOptionsGetGLProc = WGPUSType_RequestAdapterOptionsGetGLProc,
    RequestAdapterOptionsD3D11Device = WGPUSType_RequestAdapterOptionsD3D11Device,
    DawnRenderPassSampleCount = WGPUSType_DawnRenderPassSampleCount,
    RenderPassPixelLocalStorage = WGPUSType_RenderPassPixelLocalStorage,
    PipelineLayoutPixelLocalStorage = WGPUSType_PipelineLayoutPixelLocalStorage,
    BufferHostMappedPointer = WGPUSType_BufferHostMappedPointer,
    AdapterPropertiesMemoryHeaps = WGPUSType_AdapterPropertiesMemoryHeaps,
    AdapterPropertiesD3D = WGPUSType_AdapterPropertiesD3D,
    AdapterPropertiesVk = WGPUSType_AdapterPropertiesVk,
    DawnWireWGSLControl = WGPUSType_DawnWireWGSLControl,
    DawnWGSLBlocklist = WGPUSType_DawnWGSLBlocklist,
    DawnDrmFormatCapabilities = WGPUSType_DawnDrmFormatCapabilities,
    ShaderModuleCompilationOptions = WGPUSType_ShaderModuleCompilationOptions,
    ColorTargetStateExpandResolveTextureDawn = WGPUSType_ColorTargetStateExpandResolveTextureDawn,
    RenderPassRenderAreaRect = WGPUSType_RenderPassRenderAreaRect,
    SharedTextureMemoryVkDedicatedAllocationDescriptor = WGPUSType_SharedTextureMemoryVkDedicatedAllocationDescriptor,
    SharedTextureMemoryAHardwareBufferDescriptor = WGPUSType_SharedTextureMemoryAHardwareBufferDescriptor,
    SharedTextureMemoryDmaBufDescriptor = WGPUSType_SharedTextureMemoryDmaBufDescriptor,
    SharedTextureMemoryOpaqueFDDescriptor = WGPUSType_SharedTextureMemoryOpaqueFDDescriptor,
    SharedTextureMemoryZirconHandleDescriptor = WGPUSType_SharedTextureMemoryZirconHandleDescriptor,
    SharedTextureMemoryDXGISharedHandleDescriptor = WGPUSType_SharedTextureMemoryDXGISharedHandleDescriptor,
    SharedTextureMemoryD3D11Texture2DDescriptor = WGPUSType_SharedTextureMemoryD3D11Texture2DDescriptor,
    SharedTextureMemoryIOSurfaceDescriptor = WGPUSType_SharedTextureMemoryIOSurfaceDescriptor,
    SharedTextureMemoryEGLImageDescriptor = WGPUSType_SharedTextureMemoryEGLImageDescriptor,
    SharedTextureMemoryInitializedBeginState = WGPUSType_SharedTextureMemoryInitializedBeginState,
    SharedTextureMemoryInitializedEndState = WGPUSType_SharedTextureMemoryInitializedEndState,
    SharedTextureMemoryVkImageLayoutBeginState = WGPUSType_SharedTextureMemoryVkImageLayoutBeginState,
    SharedTextureMemoryVkImageLayoutEndState = WGPUSType_SharedTextureMemoryVkImageLayoutEndState,
    SharedTextureMemoryD3DSwapchainBeginState = WGPUSType_SharedTextureMemoryD3DSwapchainBeginState,
    SharedFenceVkSemaphoreOpaqueFDDescriptor = WGPUSType_SharedFenceVkSemaphoreOpaqueFDDescriptor,
    SharedFenceVkSemaphoreOpaqueFDExportInfo = WGPUSType_SharedFenceVkSemaphoreOpaqueFDExportInfo,
    SharedFenceSyncFDDescriptor = WGPUSType_SharedFenceSyncFDDescriptor,
    SharedFenceSyncFDExportInfo = WGPUSType_SharedFenceSyncFDExportInfo,
    SharedFenceVkSemaphoreZirconHandleDescriptor = WGPUSType_SharedFenceVkSemaphoreZirconHandleDescriptor,
    SharedFenceVkSemaphoreZirconHandleExportInfo = WGPUSType_SharedFenceVkSemaphoreZirconHandleExportInfo,
    SharedFenceDXGISharedHandleDescriptor = WGPUSType_SharedFenceDXGISharedHandleDescriptor,
    SharedFenceDXGISharedHandleExportInfo = WGPUSType_SharedFenceDXGISharedHandleExportInfo,
    SharedFenceMTLSharedEventDescriptor = WGPUSType_SharedFenceMTLSharedEventDescriptor,
    SharedFenceMTLSharedEventExportInfo = WGPUSType_SharedFenceMTLSharedEventExportInfo,
    SharedBufferMemoryD3D12ResourceDescriptor = WGPUSType_SharedBufferMemoryD3D12ResourceDescriptor,
    StaticSamplerBindingLayout = WGPUSType_StaticSamplerBindingLayout,
    YCbCrVkDescriptor = WGPUSType_YCbCrVkDescriptor,
    SharedTextureMemoryAHardwareBufferProperties = WGPUSType_SharedTextureMemoryAHardwareBufferProperties,
    AHardwareBufferProperties = WGPUSType_AHardwareBufferProperties,
    DawnTexelCopyBufferRowAlignmentLimits = WGPUSType_DawnTexelCopyBufferRowAlignmentLimits,
    AdapterPropertiesSubgroupMatrixConfigs = WGPUSType_AdapterPropertiesSubgroupMatrixConfigs,
    SharedFenceEGLSyncDescriptor = WGPUSType_SharedFenceEGLSyncDescriptor,
    SharedFenceEGLSyncExportInfo = WGPUSType_SharedFenceEGLSyncExportInfo,
    DawnInjectedInvalidSType = WGPUSType_DawnInjectedInvalidSType,
    DawnCompilationMessageUtf16 = WGPUSType_DawnCompilationMessageUtf16,
    DawnFakeBufferOOMForTesting = WGPUSType_DawnFakeBufferOOMForTesting,
    SurfaceDescriptorFromWindowsWinUISwapChainPanel = WGPUSType_SurfaceDescriptorFromWindowsWinUISwapChainPanel,
    DawnDeviceAllocatorControl = WGPUSType_DawnDeviceAllocatorControl,
    DawnHostMappedPointerLimits = WGPUSType_DawnHostMappedPointerLimits,
    RenderPassDescriptorResolveRect = WGPUSType_RenderPassDescriptorResolveRect,
    RequestAdapterWebGPUBackendOptions = WGPUSType_RequestAdapterWebGPUBackendOptions,
    DawnFakeDeviceInitializeErrorForTesting = WGPUSType_DawnFakeDeviceInitializeErrorForTesting,
    SharedTextureMemoryD3D11BeginState = WGPUSType_SharedTextureMemoryD3D11BeginState,
    DawnConsumeAdapterDescriptor = WGPUSType_DawnConsumeAdapterDescriptor,
    TexelBufferBindingEntry = WGPUSType_TexelBufferBindingEntry,
    TexelBufferBindingLayout = WGPUSType_TexelBufferBindingLayout,
    SharedTextureMemoryMetalEndAccessState = WGPUSType_SharedTextureMemoryMetalEndAccessState,
    AdapterPropertiesWGPU = WGPUSType_AdapterPropertiesWGPU,
    SharedBufferMemoryFromWindowsHandleDescriptor = WGPUSType_SharedBufferMemoryFromWindowsHandleDescriptor,
    SharedTextureMemoryD3D12ResourceDescriptor = WGPUSType_SharedTextureMemoryD3D12ResourceDescriptor,
    RequestAdapterOptionsAngleVirtualizationGroup = WGPUSType_RequestAdapterOptionsAngleVirtualizationGroup,
    PipelineLayoutResourceTable = WGPUSType_PipelineLayoutResourceTable,
    AdapterPropertiesDrm = WGPUSType_AdapterPropertiesDrm,
};
static_assert(sizeof(SType) == sizeof(WGPUSType), "sizeof mismatch for SType");
static_assert(alignof(SType) == alignof(WGPUSType), "alignof mismatch for SType");

enum class SubgroupMatrixComponentType : uint32_t {
    F32 = WGPUSubgroupMatrixComponentType_F32,
    F16 = WGPUSubgroupMatrixComponentType_F16,
    U32 = WGPUSubgroupMatrixComponentType_U32,
    I32 = WGPUSubgroupMatrixComponentType_I32,
    U8 = WGPUSubgroupMatrixComponentType_U8,
    I8 = WGPUSubgroupMatrixComponentType_I8,
};
static_assert(sizeof(SubgroupMatrixComponentType) == sizeof(WGPUSubgroupMatrixComponentType), "sizeof mismatch for SubgroupMatrixComponentType");
static_assert(alignof(SubgroupMatrixComponentType) == alignof(WGPUSubgroupMatrixComponentType), "alignof mismatch for SubgroupMatrixComponentType");

enum class SurfaceGetCurrentTextureStatus : uint32_t {
    SuccessOptimal = WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal,
    SuccessSuboptimal = WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal,
    Timeout = WGPUSurfaceGetCurrentTextureStatus_Timeout,
    Outdated = WGPUSurfaceGetCurrentTextureStatus_Outdated,
    Lost = WGPUSurfaceGetCurrentTextureStatus_Lost,
    Error = WGPUSurfaceGetCurrentTextureStatus_Error,
};
static_assert(sizeof(SurfaceGetCurrentTextureStatus) == sizeof(WGPUSurfaceGetCurrentTextureStatus), "sizeof mismatch for SurfaceGetCurrentTextureStatus");
static_assert(alignof(SurfaceGetCurrentTextureStatus) == alignof(WGPUSurfaceGetCurrentTextureStatus), "alignof mismatch for SurfaceGetCurrentTextureStatus");

enum class TexelBufferAccess : uint32_t {
    Undefined = WGPUTexelBufferAccess_Undefined,
    ReadOnly = WGPUTexelBufferAccess_ReadOnly,
    ReadWrite = WGPUTexelBufferAccess_ReadWrite,
};
static_assert(sizeof(TexelBufferAccess) == sizeof(WGPUTexelBufferAccess), "sizeof mismatch for TexelBufferAccess");
static_assert(alignof(TexelBufferAccess) == alignof(WGPUTexelBufferAccess), "alignof mismatch for TexelBufferAccess");

enum class TextureAspect : uint32_t {
    Undefined = WGPUTextureAspect_Undefined,
    All = WGPUTextureAspect_All,
    StencilOnly = WGPUTextureAspect_StencilOnly,
    DepthOnly = WGPUTextureAspect_DepthOnly,
    Plane0Only = WGPUTextureAspect_Plane0Only,
    Plane1Only = WGPUTextureAspect_Plane1Only,
    Plane2Only = WGPUTextureAspect_Plane2Only,
};
static_assert(sizeof(TextureAspect) == sizeof(WGPUTextureAspect), "sizeof mismatch for TextureAspect");
static_assert(alignof(TextureAspect) == alignof(WGPUTextureAspect), "alignof mismatch for TextureAspect");

enum class TextureDimension : uint32_t {
    Undefined = WGPUTextureDimension_Undefined,
    e1D = WGPUTextureDimension_1D,
    e2D = WGPUTextureDimension_2D,
    e3D = WGPUTextureDimension_3D,
};
static_assert(sizeof(TextureDimension) == sizeof(WGPUTextureDimension), "sizeof mismatch for TextureDimension");
static_assert(alignof(TextureDimension) == alignof(WGPUTextureDimension), "alignof mismatch for TextureDimension");

enum class TextureFormat : uint32_t {
    Undefined = WGPUTextureFormat_Undefined,
    R8Unorm = WGPUTextureFormat_R8Unorm,
    R8Snorm = WGPUTextureFormat_R8Snorm,
    R8Uint = WGPUTextureFormat_R8Uint,
    R8Sint = WGPUTextureFormat_R8Sint,
    R16Unorm = WGPUTextureFormat_R16Unorm,
    R16Snorm = WGPUTextureFormat_R16Snorm,
    R16Uint = WGPUTextureFormat_R16Uint,
    R16Sint = WGPUTextureFormat_R16Sint,
    R16Float = WGPUTextureFormat_R16Float,
    RG8Unorm = WGPUTextureFormat_RG8Unorm,
    RG8Snorm = WGPUTextureFormat_RG8Snorm,
    RG8Uint = WGPUTextureFormat_RG8Uint,
    RG8Sint = WGPUTextureFormat_RG8Sint,
    R32Float = WGPUTextureFormat_R32Float,
    R32Uint = WGPUTextureFormat_R32Uint,
    R32Sint = WGPUTextureFormat_R32Sint,
    RG16Unorm = WGPUTextureFormat_RG16Unorm,
    RG16Snorm = WGPUTextureFormat_RG16Snorm,
    RG16Uint = WGPUTextureFormat_RG16Uint,
    RG16Sint = WGPUTextureFormat_RG16Sint,
    RG16Float = WGPUTextureFormat_RG16Float,
    RGBA8Unorm = WGPUTextureFormat_RGBA8Unorm,
    RGBA8UnormSrgb = WGPUTextureFormat_RGBA8UnormSrgb,
    RGBA8Snorm = WGPUTextureFormat_RGBA8Snorm,
    RGBA8Uint = WGPUTextureFormat_RGBA8Uint,
    RGBA8Sint = WGPUTextureFormat_RGBA8Sint,
    BGRA8Unorm = WGPUTextureFormat_BGRA8Unorm,
    BGRA8UnormSrgb = WGPUTextureFormat_BGRA8UnormSrgb,
    RGB10A2Uint = WGPUTextureFormat_RGB10A2Uint,
    RGB10A2Unorm = WGPUTextureFormat_RGB10A2Unorm,
    RG11B10Ufloat = WGPUTextureFormat_RG11B10Ufloat,
    RGB9E5Ufloat = WGPUTextureFormat_RGB9E5Ufloat,
    RG32Float = WGPUTextureFormat_RG32Float,
    RG32Uint = WGPUTextureFormat_RG32Uint,
    RG32Sint = WGPUTextureFormat_RG32Sint,
    RGBA16Unorm = WGPUTextureFormat_RGBA16Unorm,
    RGBA16Snorm = WGPUTextureFormat_RGBA16Snorm,
    RGBA16Uint = WGPUTextureFormat_RGBA16Uint,
    RGBA16Sint = WGPUTextureFormat_RGBA16Sint,
    RGBA16Float = WGPUTextureFormat_RGBA16Float,
    RGBA32Float = WGPUTextureFormat_RGBA32Float,
    RGBA32Uint = WGPUTextureFormat_RGBA32Uint,
    RGBA32Sint = WGPUTextureFormat_RGBA32Sint,
    Stencil8 = WGPUTextureFormat_Stencil8,
    Depth16Unorm = WGPUTextureFormat_Depth16Unorm,
    Depth24Plus = WGPUTextureFormat_Depth24Plus,
    Depth24PlusStencil8 = WGPUTextureFormat_Depth24PlusStencil8,
    Depth32Float = WGPUTextureFormat_Depth32Float,
    Depth32FloatStencil8 = WGPUTextureFormat_Depth32FloatStencil8,
    BC1RGBAUnorm = WGPUTextureFormat_BC1RGBAUnorm,
    BC1RGBAUnormSrgb = WGPUTextureFormat_BC1RGBAUnormSrgb,
    BC2RGBAUnorm = WGPUTextureFormat_BC2RGBAUnorm,
    BC2RGBAUnormSrgb = WGPUTextureFormat_BC2RGBAUnormSrgb,
    BC3RGBAUnorm = WGPUTextureFormat_BC3RGBAUnorm,
    BC3RGBAUnormSrgb = WGPUTextureFormat_BC3RGBAUnormSrgb,
    BC4RUnorm = WGPUTextureFormat_BC4RUnorm,
    BC4RSnorm = WGPUTextureFormat_BC4RSnorm,
    BC5RGUnorm = WGPUTextureFormat_BC5RGUnorm,
    BC5RGSnorm = WGPUTextureFormat_BC5RGSnorm,
    BC6HRGBUfloat = WGPUTextureFormat_BC6HRGBUfloat,
    BC6HRGBFloat = WGPUTextureFormat_BC6HRGBFloat,
    BC7RGBAUnorm = WGPUTextureFormat_BC7RGBAUnorm,
    BC7RGBAUnormSrgb = WGPUTextureFormat_BC7RGBAUnormSrgb,
    ETC2RGB8Unorm = WGPUTextureFormat_ETC2RGB8Unorm,
    ETC2RGB8UnormSrgb = WGPUTextureFormat_ETC2RGB8UnormSrgb,
    ETC2RGB8A1Unorm = WGPUTextureFormat_ETC2RGB8A1Unorm,
    ETC2RGB8A1UnormSrgb = WGPUTextureFormat_ETC2RGB8A1UnormSrgb,
    ETC2RGBA8Unorm = WGPUTextureFormat_ETC2RGBA8Unorm,
    ETC2RGBA8UnormSrgb = WGPUTextureFormat_ETC2RGBA8UnormSrgb,
    EACR11Unorm = WGPUTextureFormat_EACR11Unorm,
    EACR11Snorm = WGPUTextureFormat_EACR11Snorm,
    EACRG11Unorm = WGPUTextureFormat_EACRG11Unorm,
    EACRG11Snorm = WGPUTextureFormat_EACRG11Snorm,
    ASTC4x4Unorm = WGPUTextureFormat_ASTC4x4Unorm,
    ASTC4x4UnormSrgb = WGPUTextureFormat_ASTC4x4UnormSrgb,
    ASTC5x4Unorm = WGPUTextureFormat_ASTC5x4Unorm,
    ASTC5x4UnormSrgb = WGPUTextureFormat_ASTC5x4UnormSrgb,
    ASTC5x5Unorm = WGPUTextureFormat_ASTC5x5Unorm,
    ASTC5x5UnormSrgb = WGPUTextureFormat_ASTC5x5UnormSrgb,
    ASTC6x5Unorm = WGPUTextureFormat_ASTC6x5Unorm,
    ASTC6x5UnormSrgb = WGPUTextureFormat_ASTC6x5UnormSrgb,
    ASTC6x6Unorm = WGPUTextureFormat_ASTC6x6Unorm,
    ASTC6x6UnormSrgb = WGPUTextureFormat_ASTC6x6UnormSrgb,
    ASTC8x5Unorm = WGPUTextureFormat_ASTC8x5Unorm,
    ASTC8x5UnormSrgb = WGPUTextureFormat_ASTC8x5UnormSrgb,
    ASTC8x6Unorm = WGPUTextureFormat_ASTC8x6Unorm,
    ASTC8x6UnormSrgb = WGPUTextureFormat_ASTC8x6UnormSrgb,
    ASTC8x8Unorm = WGPUTextureFormat_ASTC8x8Unorm,
    ASTC8x8UnormSrgb = WGPUTextureFormat_ASTC8x8UnormSrgb,
    ASTC10x5Unorm = WGPUTextureFormat_ASTC10x5Unorm,
    ASTC10x5UnormSrgb = WGPUTextureFormat_ASTC10x5UnormSrgb,
    ASTC10x6Unorm = WGPUTextureFormat_ASTC10x6Unorm,
    ASTC10x6UnormSrgb = WGPUTextureFormat_ASTC10x6UnormSrgb,
    ASTC10x8Unorm = WGPUTextureFormat_ASTC10x8Unorm,
    ASTC10x8UnormSrgb = WGPUTextureFormat_ASTC10x8UnormSrgb,
    ASTC10x10Unorm = WGPUTextureFormat_ASTC10x10Unorm,
    ASTC10x10UnormSrgb = WGPUTextureFormat_ASTC10x10UnormSrgb,
    ASTC12x10Unorm = WGPUTextureFormat_ASTC12x10Unorm,
    ASTC12x10UnormSrgb = WGPUTextureFormat_ASTC12x10UnormSrgb,
    ASTC12x12Unorm = WGPUTextureFormat_ASTC12x12Unorm,
    ASTC12x12UnormSrgb = WGPUTextureFormat_ASTC12x12UnormSrgb,
    R8BG8Biplanar420Unorm = WGPUTextureFormat_R8BG8Biplanar420Unorm,
    R10X6BG10X6Biplanar420Unorm = WGPUTextureFormat_R10X6BG10X6Biplanar420Unorm,
    R8BG8A8Triplanar420Unorm = WGPUTextureFormat_R8BG8A8Triplanar420Unorm,
    R8BG8Biplanar422Unorm = WGPUTextureFormat_R8BG8Biplanar422Unorm,
    R8BG8Biplanar444Unorm = WGPUTextureFormat_R8BG8Biplanar444Unorm,
    R10X6BG10X6Biplanar422Unorm = WGPUTextureFormat_R10X6BG10X6Biplanar422Unorm,
    R10X6BG10X6Biplanar444Unorm = WGPUTextureFormat_R10X6BG10X6Biplanar444Unorm,
    OpaqueYCbCrAndroid = WGPUTextureFormat_OpaqueYCbCrAndroid,
};
static_assert(sizeof(TextureFormat) == sizeof(WGPUTextureFormat), "sizeof mismatch for TextureFormat");
static_assert(alignof(TextureFormat) == alignof(WGPUTextureFormat), "alignof mismatch for TextureFormat");

enum class TextureSampleType : uint32_t {
    BindingNotUsed = WGPUTextureSampleType_BindingNotUsed,
    Undefined = WGPUTextureSampleType_Undefined,
    Float = WGPUTextureSampleType_Float,
    UnfilterableFloat = WGPUTextureSampleType_UnfilterableFloat,
    Depth = WGPUTextureSampleType_Depth,
    Sint = WGPUTextureSampleType_Sint,
    Uint = WGPUTextureSampleType_Uint,
};
static_assert(sizeof(TextureSampleType) == sizeof(WGPUTextureSampleType), "sizeof mismatch for TextureSampleType");
static_assert(alignof(TextureSampleType) == alignof(WGPUTextureSampleType), "alignof mismatch for TextureSampleType");

enum class TextureViewDimension : uint32_t {
    Undefined = WGPUTextureViewDimension_Undefined,
    e1D = WGPUTextureViewDimension_1D,
    e2D = WGPUTextureViewDimension_2D,
    e2DArray = WGPUTextureViewDimension_2DArray,
    Cube = WGPUTextureViewDimension_Cube,
    CubeArray = WGPUTextureViewDimension_CubeArray,
    e3D = WGPUTextureViewDimension_3D,
};
static_assert(sizeof(TextureViewDimension) == sizeof(WGPUTextureViewDimension), "sizeof mismatch for TextureViewDimension");
static_assert(alignof(TextureViewDimension) == alignof(WGPUTextureViewDimension), "alignof mismatch for TextureViewDimension");

enum class ToneMappingMode : uint32_t {
    Standard = WGPUToneMappingMode_Standard,
    Extended = WGPUToneMappingMode_Extended,
};
static_assert(sizeof(ToneMappingMode) == sizeof(WGPUToneMappingMode), "sizeof mismatch for ToneMappingMode");
static_assert(alignof(ToneMappingMode) == alignof(WGPUToneMappingMode), "alignof mismatch for ToneMappingMode");

enum class VertexFormat : uint32_t {
    Uint8 = WGPUVertexFormat_Uint8,
    Uint8x2 = WGPUVertexFormat_Uint8x2,
    Uint8x4 = WGPUVertexFormat_Uint8x4,
    Sint8 = WGPUVertexFormat_Sint8,
    Sint8x2 = WGPUVertexFormat_Sint8x2,
    Sint8x4 = WGPUVertexFormat_Sint8x4,
    Unorm8 = WGPUVertexFormat_Unorm8,
    Unorm8x2 = WGPUVertexFormat_Unorm8x2,
    Unorm8x4 = WGPUVertexFormat_Unorm8x4,
    Snorm8 = WGPUVertexFormat_Snorm8,
    Snorm8x2 = WGPUVertexFormat_Snorm8x2,
    Snorm8x4 = WGPUVertexFormat_Snorm8x4,
    Uint16 = WGPUVertexFormat_Uint16,
    Uint16x2 = WGPUVertexFormat_Uint16x2,
    Uint16x4 = WGPUVertexFormat_Uint16x4,
    Sint16 = WGPUVertexFormat_Sint16,
    Sint16x2 = WGPUVertexFormat_Sint16x2,
    Sint16x4 = WGPUVertexFormat_Sint16x4,
    Unorm16 = WGPUVertexFormat_Unorm16,
    Unorm16x2 = WGPUVertexFormat_Unorm16x2,
    Unorm16x4 = WGPUVertexFormat_Unorm16x4,
    Snorm16 = WGPUVertexFormat_Snorm16,
    Snorm16x2 = WGPUVertexFormat_Snorm16x2,
    Snorm16x4 = WGPUVertexFormat_Snorm16x4,
    Float16 = WGPUVertexFormat_Float16,
    Float16x2 = WGPUVertexFormat_Float16x2,
    Float16x4 = WGPUVertexFormat_Float16x4,
    Float32 = WGPUVertexFormat_Float32,
    Float32x2 = WGPUVertexFormat_Float32x2,
    Float32x3 = WGPUVertexFormat_Float32x3,
    Float32x4 = WGPUVertexFormat_Float32x4,
    Uint32 = WGPUVertexFormat_Uint32,
    Uint32x2 = WGPUVertexFormat_Uint32x2,
    Uint32x3 = WGPUVertexFormat_Uint32x3,
    Uint32x4 = WGPUVertexFormat_Uint32x4,
    Sint32 = WGPUVertexFormat_Sint32,
    Sint32x2 = WGPUVertexFormat_Sint32x2,
    Sint32x3 = WGPUVertexFormat_Sint32x3,
    Sint32x4 = WGPUVertexFormat_Sint32x4,
    Unorm10_10_10_2 = WGPUVertexFormat_Unorm10_10_10_2,
    Unorm8x4BGRA = WGPUVertexFormat_Unorm8x4BGRA,
};
static_assert(sizeof(VertexFormat) == sizeof(WGPUVertexFormat), "sizeof mismatch for VertexFormat");
static_assert(alignof(VertexFormat) == alignof(WGPUVertexFormat), "alignof mismatch for VertexFormat");

enum class VertexStepMode : uint32_t {
    Undefined = WGPUVertexStepMode_Undefined,
    Vertex = WGPUVertexStepMode_Vertex,
    Instance = WGPUVertexStepMode_Instance,
};
static_assert(sizeof(VertexStepMode) == sizeof(WGPUVertexStepMode), "sizeof mismatch for VertexStepMode");
static_assert(alignof(VertexStepMode) == alignof(WGPUVertexStepMode), "alignof mismatch for VertexStepMode");

enum class WaitStatus : uint32_t {
    Success = WGPUWaitStatus_Success,
    TimedOut = WGPUWaitStatus_TimedOut,
    Error = WGPUWaitStatus_Error,
};
static_assert(sizeof(WaitStatus) == sizeof(WGPUWaitStatus), "sizeof mismatch for WaitStatus");
static_assert(alignof(WaitStatus) == alignof(WGPUWaitStatus), "alignof mismatch for WaitStatus");

enum class WGSLLanguageFeatureName : uint32_t {
    ReadonlyAndReadwriteStorageTextures = WGPUWGSLLanguageFeatureName_ReadonlyAndReadwriteStorageTextures,
    Packed4x8IntegerDotProduct = WGPUWGSLLanguageFeatureName_Packed4x8IntegerDotProduct,
    UnrestrictedPointerParameters = WGPUWGSLLanguageFeatureName_UnrestrictedPointerParameters,
    PointerCompositeAccess = WGPUWGSLLanguageFeatureName_PointerCompositeAccess,
    UniformBufferStandardLayout = WGPUWGSLLanguageFeatureName_UniformBufferStandardLayout,
    SubgroupId = WGPUWGSLLanguageFeatureName_SubgroupId,
    TextureAndSamplerLet = WGPUWGSLLanguageFeatureName_TextureAndSamplerLet,
    SubgroupUniformity = WGPUWGSLLanguageFeatureName_SubgroupUniformity,
    TextureFormatsTier1 = WGPUWGSLLanguageFeatureName_TextureFormatsTier1,
    LinearIndexing = WGPUWGSLLanguageFeatureName_LinearIndexing,
    ImmediateAddressSpace = WGPUWGSLLanguageFeatureName_ImmediateAddressSpace,
    ChromiumTestingUnimplemented = WGPUWGSLLanguageFeatureName_ChromiumTestingUnimplemented,
    ChromiumTestingUnsafeExperimental = WGPUWGSLLanguageFeatureName_ChromiumTestingUnsafeExperimental,
    ChromiumTestingExperimental = WGPUWGSLLanguageFeatureName_ChromiumTestingExperimental,
    ChromiumTestingShippedWithKillswitch = WGPUWGSLLanguageFeatureName_ChromiumTestingShippedWithKillswitch,
    ChromiumTestingShipped = WGPUWGSLLanguageFeatureName_ChromiumTestingShipped,
    SizedBindingArray = WGPUWGSLLanguageFeatureName_SizedBindingArray,
    TexelBuffers = WGPUWGSLLanguageFeatureName_TexelBuffers,
    ChromiumPrint = WGPUWGSLLanguageFeatureName_ChromiumPrint,
    FragmentDepth = WGPUWGSLLanguageFeatureName_FragmentDepth,
    BufferView = WGPUWGSLLanguageFeatureName_BufferView,
    SwizzleAssignment = WGPUWGSLLanguageFeatureName_SwizzleAssignment,
};
static_assert(sizeof(WGSLLanguageFeatureName) == sizeof(WGPUWGSLLanguageFeatureName), "sizeof mismatch for WGSLLanguageFeatureName");
static_assert(alignof(WGSLLanguageFeatureName) == alignof(WGPUWGSLLanguageFeatureName), "alignof mismatch for WGSLLanguageFeatureName");


enum class BufferUsage : uint64_t {
    None = WGPUBufferUsage_None,
    MapRead = WGPUBufferUsage_MapRead,
    MapWrite = WGPUBufferUsage_MapWrite,
    CopySrc = WGPUBufferUsage_CopySrc,
    CopyDst = WGPUBufferUsage_CopyDst,
    Index = WGPUBufferUsage_Index,
    Vertex = WGPUBufferUsage_Vertex,
    Uniform = WGPUBufferUsage_Uniform,
    Storage = WGPUBufferUsage_Storage,
    Indirect = WGPUBufferUsage_Indirect,
    QueryResolve = WGPUBufferUsage_QueryResolve,
    TexelBuffer = WGPUBufferUsage_TexelBuffer,
};
static_assert(sizeof(BufferUsage) == sizeof(WGPUBufferUsage), "sizeof mismatch for BufferUsage");
static_assert(alignof(BufferUsage) == alignof(WGPUBufferUsage), "alignof mismatch for BufferUsage");

enum class ColorWriteMask : uint64_t {
    None = WGPUColorWriteMask_None,
    Red = WGPUColorWriteMask_Red,
    Green = WGPUColorWriteMask_Green,
    Blue = WGPUColorWriteMask_Blue,
    Alpha = WGPUColorWriteMask_Alpha,
    All = WGPUColorWriteMask_All,
};
static_assert(sizeof(ColorWriteMask) == sizeof(WGPUColorWriteMask), "sizeof mismatch for ColorWriteMask");
static_assert(alignof(ColorWriteMask) == alignof(WGPUColorWriteMask), "alignof mismatch for ColorWriteMask");

enum class HeapProperty : uint64_t {
    None = WGPUHeapProperty_None,
    DeviceLocal = WGPUHeapProperty_DeviceLocal,
    HostVisible = WGPUHeapProperty_HostVisible,
    HostCoherent = WGPUHeapProperty_HostCoherent,
    HostUncached = WGPUHeapProperty_HostUncached,
    HostCached = WGPUHeapProperty_HostCached,
};
static_assert(sizeof(HeapProperty) == sizeof(WGPUHeapProperty), "sizeof mismatch for HeapProperty");
static_assert(alignof(HeapProperty) == alignof(WGPUHeapProperty), "alignof mismatch for HeapProperty");

enum class MapMode : uint64_t {
    None = WGPUMapMode_None,
    Read = WGPUMapMode_Read,
    Write = WGPUMapMode_Write,
};
static_assert(sizeof(MapMode) == sizeof(WGPUMapMode), "sizeof mismatch for MapMode");
static_assert(alignof(MapMode) == alignof(WGPUMapMode), "alignof mismatch for MapMode");

enum class ShaderStage : uint64_t {
    None = WGPUShaderStage_None,
    Vertex = WGPUShaderStage_Vertex,
    Fragment = WGPUShaderStage_Fragment,
    Compute = WGPUShaderStage_Compute,
};
static_assert(sizeof(ShaderStage) == sizeof(WGPUShaderStage), "sizeof mismatch for ShaderStage");
static_assert(alignof(ShaderStage) == alignof(WGPUShaderStage), "alignof mismatch for ShaderStage");

enum class TextureUsage : uint64_t {
    None = WGPUTextureUsage_None,
    CopySrc = WGPUTextureUsage_CopySrc,
    CopyDst = WGPUTextureUsage_CopyDst,
    TextureBinding = WGPUTextureUsage_TextureBinding,
    StorageBinding = WGPUTextureUsage_StorageBinding,
    RenderAttachment = WGPUTextureUsage_RenderAttachment,
    TransientAttachment = WGPUTextureUsage_TransientAttachment,
    StorageAttachment = WGPUTextureUsage_StorageAttachment,
};
static_assert(sizeof(TextureUsage) == sizeof(WGPUTextureUsage), "sizeof mismatch for TextureUsage");
static_assert(alignof(TextureUsage) == alignof(WGPUTextureUsage), "alignof mismatch for TextureUsage");


// TODO(crbug.com/42241461): Update these to not be using the C callback types, and instead be
// defined using C++ types instead. Note that when we remove these, the C++ callback info types
// should also all be removed as they will no longer be necessary given the C++ templated
// functions calls and setter utilities.
using Callback = WGPUCallback;
using DawnLoadCacheDataFunction = WGPUDawnLoadCacheDataFunction;
using DawnStoreCacheDataFunction = WGPUDawnStoreCacheDataFunction;
using Proc = WGPUProc;

// Special class for booleans in order to allow implicit conversions.
class Bool {
  public:
    constexpr Bool() = default;
    explicit(false) constexpr Bool(bool value) : mValue(static_cast<WGPUBool>(value)) {}
    explicit(false) Bool(WGPUBool value): mValue(value) {}

    // NOLINTNEXTLINE(google-explicit-constructor)
    constexpr operator bool() const { return static_cast<bool>(mValue); }

  private:
    friend struct std::hash<Bool>;
    // Default to false.
    WGPUBool mValue = static_cast<WGPUBool>(false);
};

// Special class for optional booleans in order to allow conversions.
class OptionalBool {
  public:
    constexpr OptionalBool() = default;
    explicit(false) constexpr OptionalBool(bool value) : mValue(static_cast<WGPUOptionalBool>(value)) {}
    explicit(false) constexpr OptionalBool(std::optional<bool> value) :
        mValue(value ? static_cast<WGPUOptionalBool>(*value) : WGPUOptionalBool_Undefined) {}
    explicit(false) constexpr OptionalBool(WGPUOptionalBool value): mValue(value) {}

    // Define the values that are equivalent to the enums.
    static const OptionalBool False;
    static const OptionalBool True;
    static const OptionalBool Undefined;

    // Assignment operators.
    OptionalBool& operator=(const bool& value) {
        mValue = static_cast<WGPUOptionalBool>(value);
        return *this;
    }
    OptionalBool& operator=(const std::optional<bool>& value) {
        mValue = value ? static_cast<WGPUOptionalBool>(*value) : WGPUOptionalBool_Undefined;
        return *this;
    }
    OptionalBool& operator=(const WGPUOptionalBool& value) {
        mValue = value;
        return *this;
    }

    // Conversion functions.
    // NOLINTNEXTLINE(google-explicit-constructor)
    operator WGPUOptionalBool() const { return mValue; }
    // NOLINTNEXTLINE(google-explicit-constructor)
    operator std::optional<bool>() const {
        if (mValue == WGPUOptionalBool_Undefined) {
            return std::nullopt;
        }
        return static_cast<bool>(mValue);
    }

    // Comparison functions.
    friend bool operator==(const OptionalBool& lhs, const OptionalBool& rhs) {
        return lhs.mValue == rhs.mValue;
    }
    friend bool operator!=(const OptionalBool& lhs, const OptionalBool& rhs) {
        return lhs.mValue != rhs.mValue;
    }

  private:
    friend struct std::hash<OptionalBool>;
    // Default to undefined.
    WGPUOptionalBool mValue = WGPUOptionalBool_Undefined;
};
inline const OptionalBool OptionalBool::False = OptionalBool(WGPUOptionalBool_False);
inline const OptionalBool OptionalBool::True = OptionalBool(WGPUOptionalBool_True);
inline const OptionalBool OptionalBool::Undefined = OptionalBool(WGPUOptionalBool_Undefined);

// Helper class to wrap Status which allows implicit conversion to bool.
// Used while callers switch to checking the Status enum instead of booleans.
// TODO(crbug.com/42241199): Remove when all callers check the enum.
struct ConvertibleStatus {
    explicit(false) constexpr ConvertibleStatus(Status status) : status(status) {}
    // NOLINTNEXTLINE(google-explicit-constructor)
    constexpr operator bool() const {
        return status == Status::Success;
    }
    // NOLINTNEXTLINE(google-explicit-constructor)
    constexpr operator Status() const {
        return status;
    }
    Status status;
};

template<typename Derived, typename CType>
class ObjectBase {
  public:
    ObjectBase() = default;
    explicit(false) ObjectBase(CType handle): mHandle(handle) {
        if (mHandle) Derived::WGPUAddRef(mHandle);
    }
    ~ObjectBase() {
        if (mHandle) Derived::WGPURelease(mHandle);
    }

    ObjectBase(ObjectBase const& other)
        : ObjectBase(other.Get()) {
    }
    Derived& operator=(ObjectBase const& other) {
        if (&other != this) {
            if (mHandle) Derived::WGPURelease(mHandle);
            mHandle = other.mHandle;
            if (mHandle) Derived::WGPUAddRef(mHandle);
        }

        return static_cast<Derived&>(*this);
    }

    ObjectBase(ObjectBase&& other) {
        mHandle = other.mHandle;
        other.mHandle = nullptr;
    }
    Derived& operator=(ObjectBase&& other) {
        if (&other != this) {
            if (mHandle) Derived::WGPURelease(mHandle);
            mHandle = other.mHandle;
            other.mHandle = nullptr;
        }

        return static_cast<Derived&>(*this);
    }

    explicit(false) ObjectBase(std::nullptr_t) {}
    Derived& operator=(std::nullptr_t) {
        if (mHandle != nullptr) {
            Derived::WGPURelease(mHandle);
            mHandle = nullptr;
        }
        return static_cast<Derived&>(*this);
    }

    bool operator==(std::nullptr_t) const {
        return mHandle == nullptr;
    }
    bool operator!=(std::nullptr_t) const {
        return mHandle != nullptr;
    }

    explicit operator bool() const {
        return mHandle != nullptr;
    }
    CType Get() const {
        return mHandle;
    }
    CType MoveToCHandle() {
        CType result = mHandle;
        mHandle = nullptr;
        return result;
    }
    static Derived Acquire(CType handle) {
        Derived result;
        result.mHandle = handle;
        return result;
    }

  protected:
    CType mHandle = nullptr;
};

class Adapter;
class BindGroup;
class BindGroupLayout;
class Buffer;
class CommandBuffer;
class CommandEncoder;
class ComputePipeline;
class Device;
class ExternalTexture;
class Instance;
class PipelineLayout;
class QuerySet;
class Queue;
class RenderBundle;
class RenderPipeline;
class ResourceTable;
class Sampler;
class ShaderModule;
class SharedBufferMemory;
class SharedFence;
class SharedTextureMemory;
class Surface;
class TexelBufferView;
class Texture;
class TextureView;
class ComputePassEncoder;
class RenderBundleEncoder;
class RenderPassEncoder;

struct StringView;
struct AdapterPropertiesD3D;
struct AdapterPropertiesDrm;
struct AdapterPropertiesVk;
struct AdapterPropertiesWGPU;
struct BindingResource;
struct BlendComponent;
struct BufferBindingLayout;
struct BufferHostMappedPointer;
struct Color;
struct ColorSpaceDawn;
struct ColorTargetStateExpandResolveTextureDawn;
struct CommandBufferDescriptor;
struct CompatibilityModeLimits;
struct ConstantEntry;
struct CopyTextureForBrowserOptions;
struct DawnAdapterPropertiesPowerPreference;
struct DawnBufferDescriptorErrorInfoFromWireClient;
struct DawnCacheDeviceDescriptor;
struct DawnCompilationMessageUtf16;
struct DawnConsumeAdapterDescriptor;
struct DawnDeviceAllocatorControl;
struct DawnDrmFormatProperties;
struct DawnEncoderInternalUsageDescriptor;
struct DawnFakeBufferOOMForTesting;
struct DawnFakeDeviceInitializeErrorForTesting;
struct DawnHostMappedPointerLimits;
struct DawnInjectedInvalidSType;
struct DawnRenderPassSampleCount;
struct DawnShaderModuleSPIRVOptionsDescriptor;
struct DawnTexelCopyBufferRowAlignmentLimits;
struct DawnTextureInternalUsageDescriptor;
struct DawnTogglesDescriptor;
struct DawnWGSLBlocklist;
struct DawnWireWGSLControl;
struct EmscriptenSurfaceSourceCanvasHTMLSelector;
struct Extent2D;
struct Extent3D;
struct ExternalTextureBindingEntry;
struct ExternalTextureBindingLayout;
struct Future;
struct InstanceLimits;
struct INTERNAL_HAVE_EMDAWNWEBGPU_HEADER;
struct MemoryHeapInfo;
struct MultisampleState;
struct Origin2D;
struct Origin3D;
struct PassTimestampWrites;
struct PipelineLayoutResourceTable;
struct PipelineLayoutStorageAttachment;
struct PrimitiveState;
struct QuerySetDescriptor;
struct QueueDescriptor;
struct RenderBundleDescriptor;
struct RenderBundleEncoderDescriptor;
struct RenderPassDepthStencilAttachment;
struct RenderPassDescriptorResolveRect;
struct RenderPassMaxDrawCount;
struct RequestAdapterWebGPUBackendOptions;
struct RequestAdapterWebXROptions;
struct ResourceTableDescriptor;
struct SamplerBindingLayout;
struct ShaderModuleCompilationOptions;
struct ShaderSourceSPIRV;
struct ShaderSourceWGSL;
struct SharedBufferMemoryBeginAccessDescriptor;
struct SharedBufferMemoryEndAccessState;
struct SharedBufferMemoryFromWindowsHandleDescriptor;
struct SharedBufferMemoryProperties;
struct SharedFenceDXGISharedHandleDescriptor;
struct SharedFenceDXGISharedHandleExportInfo;
struct SharedFenceEGLSyncDescriptor;
struct SharedFenceEGLSyncExportInfo;
struct SharedFenceMTLSharedEventDescriptor;
struct SharedFenceMTLSharedEventExportInfo;
struct SharedFenceSyncFDDescriptor;
struct SharedFenceSyncFDExportInfo;
struct SharedFenceVkSemaphoreOpaqueFDDescriptor;
struct SharedFenceVkSemaphoreOpaqueFDExportInfo;
struct SharedFenceVkSemaphoreZirconHandleDescriptor;
struct SharedFenceVkSemaphoreZirconHandleExportInfo;
struct SharedTextureMemoryAHardwareBufferDescriptor;
struct SharedTextureMemoryD3D11BeginState;
struct SharedTextureMemoryD3DSwapchainBeginState;
struct SharedTextureMemoryDmaBufPlane;
struct SharedTextureMemoryDXGISharedHandleDescriptor;
struct SharedTextureMemoryEGLImageDescriptor;
struct SharedTextureMemoryIOSurfaceDescriptor;
struct SharedTextureMemoryOpaqueFDDescriptor;
struct SharedTextureMemoryVkDedicatedAllocationDescriptor;
struct SharedTextureMemoryVkImageLayoutBeginState;
struct SharedTextureMemoryVkImageLayoutEndState;
struct SharedTextureMemoryZirconHandleDescriptor;
struct StaticSamplerBindingLayout;
struct StencilFaceState;
struct StorageTextureBindingLayout;
struct SubgroupMatrixConfig;
struct SupportedFeatures;
struct SupportedInstanceFeatures;
struct SupportedWGSLLanguageFeatures;
struct SurfaceCapabilities;
struct SurfaceColorManagement;
struct SurfaceConfiguration;
struct SurfaceDescriptorFromWindowsCoreWindow;
struct SurfaceDescriptorFromWindowsUWPSwapChainPanel;
struct SurfaceDescriptorFromWindowsWinUISwapChainPanel;
struct SurfaceSourceAndroidNativeWindow;
struct SurfaceSourceMetalLayer;
struct SurfaceSourceWaylandSurface;
struct SurfaceSourceWindowsHWND;
struct SurfaceSourceXCBWindow;
struct SurfaceSourceXlibWindow;
struct SurfaceTexture;
struct TexelBufferBindingEntry;
struct TexelBufferBindingLayout;
struct TexelBufferViewDescriptor;
struct TexelCopyBufferLayout;
struct TextureBindingLayout;
struct TextureBindingViewDimension;
struct TextureComponentSwizzle;
struct VertexAttribute;
struct YCbCrVkDescriptor;
struct AdapterPropertiesMemoryHeaps;
struct AdapterPropertiesSubgroupMatrixConfigs;
struct AHardwareBufferProperties;
struct BindGroupEntry;
struct BindGroupLayoutEntry;
struct BlendState;
struct BufferDescriptor;
struct CommandEncoderDescriptor;
struct CompilationMessage;
struct ComputePassDescriptor;
struct ComputeState;
struct DawnDrmFormatCapabilities;
struct DepthStencilState;
struct ExternalTextureDescriptor;
struct FutureWaitInfo;
struct ImageCopyExternalTexture;
struct InstanceDescriptor;
struct Limits;
struct PipelineLayoutPixelLocalStorage;
struct RenderPassColorAttachment;
struct RenderPassRenderAreaRect;
struct RenderPassStorageAttachment;
struct RequestAdapterOptions;
struct SamplerDescriptor;
struct ShaderModuleDescriptor;
struct SharedBufferMemoryDescriptor;
struct SharedFenceDescriptor;
struct SharedFenceExportInfo;
struct SharedTextureMemoryAHardwareBufferProperties;
struct SharedTextureMemoryBeginAccessDescriptor;
struct SharedTextureMemoryDmaBufDescriptor;
struct SharedTextureMemoryMetalEndAccessState;
struct SurfaceDescriptor;
struct TexelCopyBufferInfo;
struct TexelCopyTextureInfo;
struct TextureComponentSwizzleDescriptor;
struct TextureDescriptor;
struct VertexBufferLayout;
struct AdapterInfo;
struct BindGroupDescriptor;
struct BindGroupLayoutDescriptor;
struct ColorTargetState;
struct CompilationInfo;
struct ComputePipelineDescriptor;
struct DawnFormatCapabilities;
struct DeviceDescriptor;
struct PipelineLayoutDescriptor;
struct RenderPassPixelLocalStorage;
struct SharedTextureMemoryDescriptor;
struct SharedTextureMemoryEndAccessState;
struct SharedTextureMemoryProperties;
struct TextureViewDescriptor;
struct VertexState;
struct FragmentState;
struct RenderPassDescriptor;
struct RenderPipelineDescriptor;

struct StringView {
    char const * data = nullptr;
    size_t length = WGPU_STRLEN;

    inline constexpr StringView() noexcept = default;

    explicit(false) inline constexpr StringView(const std::string_view& sv) noexcept {
        this->data = sv.data();
        this->length = sv.length();
    }

    explicit(false) inline constexpr StringView(const char* s) {
        this->data = s;
        this->length = WGPU_STRLEN;  // use strlen
    }

    explicit(false) inline constexpr StringView(WGPUStringView s) {
        this->data = s.data;
        this->length = s.length;
    }

    inline constexpr StringView(const char* data, size_t length) {
        this->data = data;
        this->length = length;
    }

    explicit(false) inline constexpr StringView(std::nullptr_t) {
        this->data = nullptr;
        this->length = WGPU_STRLEN;
    }

    explicit(false) inline constexpr StringView(std::nullopt_t) {
        this->data = nullptr;
        this->length = WGPU_STRLEN;
    }

    bool IsUndefined() const {
        return this->data == nullptr && this->length == wgpu::kStrlen;
    }

    // NOLINTNEXTLINE(google-explicit-constructor)
    operator std::string_view() const {
        if (this->length == wgpu::kStrlen) {
            if (IsUndefined()) {
                return {};
            }
            return {this->data};
        }
        return {this->data, this->length};
    }

    template <typename View,
              typename = std::enable_if_t<std::is_constructible_v<View, const char*, size_t>>>
    explicit operator View() const {
        if (this->length == wgpu::kStrlen) {
            if (IsUndefined()) {
                return {};
            }
            return {this->data};
        }
        return {this->data, this->length};
    }

};

namespace detail {
constexpr size_t ConstexprMax(size_t a, size_t b) {
    return a > b ? a : b;
}

template <typename T>
inline T& AsNonConstReference(const T& value) {
    return const_cast<T&>(value);
}
}  // namespace detail

namespace detail {
// For callbacks, we support two modes:
//   1) No userdata where we allow a std::function type that can include argument captures.
//   2) Explicit typed userdata where we only allow non-capturing lambdas or function pointers.
template <typename R, typename... Args>
struct CallbackTypeBase;
template <typename R, typename... Args>
struct CallbackTypeBase<R, std::tuple<Args...>> {
    using Callback = std::function<R(Args...)>;
};
template <typename R, typename... Args>
struct CallbackTypeBase<R, std::tuple<Args...>, void> {
    using Callback = R (Args...);
};
template <typename R, typename... Args, typename T>
struct CallbackTypeBase<R, std::tuple<Args...>, T> {
    using Callback = R (Args..., T);
};

// Noexcept specializations of CallbackTypeBase.
template <typename R, typename... Args>
struct CallbackTypeBase<R, std::tuple<Args...>, std::true_type> {
    using Callback = R (Args...) noexcept;
};
template <typename R, typename... Args>
struct CallbackTypeBase<R, std::tuple<Args...>, void, std::true_type> {
    using Callback = R (Args...) noexcept;
};
template <typename R, typename... Args, typename T>
struct CallbackTypeBase<R, std::tuple<Args...>, T, std::true_type> {
    using Callback = R (Args..., T) noexcept;
};
}  // namespace detail
template <typename... T>
using BufferMapCallback = detail::CallbackTypeBase<
    void,
    std::tuple<MapAsyncStatus, StringView>, T...>::Callback;
template <typename... T>
using CompilationInfoCallback = detail::CallbackTypeBase<
    void,
    std::tuple<CompilationInfoRequestStatus, CompilationInfo const *>, T...>::Callback;
template <typename... T>
using CreateComputePipelineAsyncCallback = detail::CallbackTypeBase<
    void,
    std::tuple<CreatePipelineAsyncStatus, ComputePipeline, StringView>, T...>::Callback;
template <typename... T>
using CreateRenderPipelineAsyncCallback = detail::CallbackTypeBase<
    void,
    std::tuple<CreatePipelineAsyncStatus, RenderPipeline, StringView>, T...>::Callback;
template <typename... T>
using DawnLoadCacheDataCallback = detail::CallbackTypeBase<
    size_t,
    std::tuple<size_t, uint8_t const *, size_t, uint8_t *>, T...>::Callback;
template <typename... T>
using DawnStoreCacheDataCallback = detail::CallbackTypeBase<
    void,
    std::tuple<size_t, uint8_t const *, size_t, uint8_t const *>, T...>::Callback;
template <typename... T>
using LoggingCallback = detail::CallbackTypeBase<
    void,
    std::tuple<LoggingType, StringView>, T...>::Callback;
template <typename... T>
using PopErrorScopeCallback = detail::CallbackTypeBase<
    void,
    std::tuple<PopErrorScopeStatus, ErrorType, StringView>, T...>::Callback;
template <typename... T>
using QueueWorkDoneCallback = detail::CallbackTypeBase<
    void,
    std::tuple<QueueWorkDoneStatus, StringView>, T...>::Callback;
template <typename... T>
using RequestAdapterCallback = detail::CallbackTypeBase<
    void,
    std::tuple<RequestAdapterStatus, Adapter, StringView>, T...>::Callback;
template <typename... T>
using RequestDeviceCallback = detail::CallbackTypeBase<
    void,
    std::tuple<RequestDeviceStatus, Device, StringView>, T...>::Callback;
template <typename... T>
using DeviceLostCallback = detail::CallbackTypeBase<void, std::tuple<const Device&, DeviceLostReason, StringView>, T...>::Callback;
template <typename... T>
using UncapturedErrorCallback = detail::CallbackTypeBase<void, std::tuple<const Device&, ErrorType, StringView>, T...>::Callback;

class Adapter : public ObjectBase<Adapter, WGPUAdapter> {
  public:
    using ObjectBase::ObjectBase;
    using ObjectBase::operator=;

    inline Device CreateDevice(DeviceDescriptor const * descriptor = nullptr) const;
    inline void GetFeatures(SupportedFeatures * features) const;
    inline ConvertibleStatus GetFormatCapabilities(TextureFormat format, DawnFormatCapabilities * capabilities) const;
    inline ConvertibleStatus GetInfo(AdapterInfo * info) const;
    inline Instance GetInstance() const;
    inline ConvertibleStatus GetLimits(Limits * limits) const;
    inline Bool HasFeature(FeatureName feature) const;
    template <typename F, typename T>
    Future RequestDevice(DeviceDescriptor const * descriptor, CallbackMode callbackMode, F callback, T userdata) const;
    template <typename F>
    Future RequestDevice(DeviceDescriptor const * descriptor, CallbackMode callbackMode, F callback) const;


  private:
    friend ObjectBase<Adapter, WGPUAdapter>;
    static inline void WGPUAddRef(WGPUAdapter handle);
    static inline void WGPURelease(WGPUAdapter handle);
};

class BindGroup : public ObjectBase<BindGroup, WGPUBindGroup> {
  public:
    using ObjectBase::ObjectBase;
    using ObjectBase::operator=;

    inline void SetLabel(StringView label) const;


  private:
    friend ObjectBase<BindGroup, WGPUBindGroup>;
    static inline void WGPUAddRef(WGPUBindGroup handle);
    static inline void WGPURelease(WGPUBindGroup handle);
};

class BindGroupLayout : public ObjectBase<BindGroupLayout, WGPUBindGroupLayout> {
  public:
    using ObjectBase::ObjectBase;
    using ObjectBase::operator=;

    inline void SetLabel(StringView label) const;


  private:
    friend ObjectBase<BindGroupLayout, WGPUBindGroupLayout>;
    static inline void WGPUAddRef(WGPUBindGroupLayout handle);
    static inline void WGPURelease(WGPUBindGroupLayout handle);
};

class Buffer : public ObjectBase<Buffer, WGPUBuffer> {
  public:
    using ObjectBase::ObjectBase;
    using ObjectBase::operator=;

    inline TexelBufferView CreateTexelView(TexelBufferViewDescriptor const * descriptor) const;
    inline void Destroy() const;
    inline void const * GetConstMappedRange(size_t offset = 0, size_t size = kWholeMapSize) const;
    inline void * GetMappedRange(size_t offset = 0, size_t size = kWholeMapSize) const;
    inline BufferMapState GetMapState() const;
    inline uint64_t GetSize() const;
    inline BufferUsage GetUsage() const;
    template <typename F, typename T>
    Future MapAsync(MapMode mode, size_t offset, size_t size, CallbackMode callbackMode, F callback, T userdata) const;
    template <typename F>
    Future MapAsync(MapMode mode, size_t offset, size_t size, CallbackMode callbackMode, F callback) const;
    inline ConvertibleStatus ReadMappedRange(size_t offset, void * data, size_t size) const;
    inline void SetLabel(StringView label) const;
    inline void Unmap() const;
    inline ConvertibleStatus WriteMappedRange(size_t offset, void const * data, size_t size) const;


  private:
    friend ObjectBase<Buffer, WGPUBuffer>;
    static inline void WGPUAddRef(WGPUBuffer handle);
    static inline void WGPURelease(WGPUBuffer handle);
};

class CommandBuffer : public ObjectBase<CommandBuffer, WGPUCommandBuffer> {
  public:
    using ObjectBase::ObjectBase;
    using ObjectBase::operator=;

    inline void SetLabel(StringView label) const;


  private:
    friend ObjectBase<CommandBuffer, WGPUCommandBuffer>;
    static inline void WGPUAddRef(WGPUCommandBuffer handle);
    static inline void WGPURelease(WGPUCommandBuffer handle);
};

class CommandEncoder : public ObjectBase<CommandEncoder, WGPUCommandEncoder> {
  public:
    using ObjectBase::ObjectBase;
    using ObjectBase::operator=;

    inline ComputePassEncoder BeginComputePass(ComputePassDescriptor const * descriptor = nullptr) const;
    inline RenderPassEncoder BeginRenderPass(RenderPassDescriptor const * descriptor) const;
    inline void ClearBuffer(Buffer const& buffer, uint64_t offset = 0, uint64_t size = kWholeSize) const;
    inline void CopyBufferToBuffer(Buffer const& source, uint64_t sourceOffset, Buffer const& destination, uint64_t destinationOffset, uint64_t size) const;
    inline void CopyBufferToTexture(TexelCopyBufferInfo const * source, TexelCopyTextureInfo const * destination, Extent3D const * copySize) const;
    inline void CopyTextureToBuffer(TexelCopyTextureInfo const * source, TexelCopyBufferInfo const * destination, Extent3D const * copySize) const;
    inline void CopyTextureToTexture(TexelCopyTextureInfo const * source, TexelCopyTextureInfo const * destination, Extent3D const * copySize) const;
    inline CommandBuffer Finish(CommandBufferDescriptor const * descriptor = nullptr) const;
    inline void InjectValidationError(StringView message) const;
    inline void InsertDebugMarker(StringView markerLabel) const;
    inline void PopDebugGroup() const;
    inline void PushDebugGroup(StringView groupLabel) const;
    inline void ResolveQuerySet(QuerySet const& querySet, uint32_t firstQuery, uint32_t queryCount, Buffer const& destination, uint64_t destinationOffset) const;
    inline void SetLabel(StringView label) const;
    inline void WriteBuffer(Buffer const& buffer, uint64_t bufferOffset, uint8_t const * data, uint64_t size) const;
    inline void WriteTimestamp(QuerySet const& querySet, uint32_t queryIndex) const;


  private:
    friend ObjectBase<CommandEncoder, WGPUCommandEncoder>;
    static inline void WGPUAddRef(WGPUCommandEncoder handle);
    static inline void WGPURelease(WGPUCommandEncoder handle);
};

class ComputePipeline : public ObjectBase<ComputePipeline, WGPUComputePipeline> {
  public:
    using ObjectBase::ObjectBase;
    using ObjectBase::operator=;

    inline BindGroupLayout GetBindGroupLayout(uint32_t groupIndex) const;
    inline void SetLabel(StringView label) const;


  private:
    friend ObjectBase<ComputePipeline, WGPUComputePipeline>;
    static inline void WGPUAddRef(WGPUComputePipeline handle);
    static inline void WGPURelease(WGPUComputePipeline handle);
};

class Device : public ObjectBase<Device, WGPUDevice> {
  public:
    using ObjectBase::ObjectBase;
    using ObjectBase::operator=;

    inline BindGroup CreateBindGroup(BindGroupDescriptor const * descriptor) const;
    inline BindGroupLayout CreateBindGroupLayout(BindGroupLayoutDescriptor const * descriptor) const;
    inline Buffer CreateBuffer(BufferDescriptor const * descriptor) const;
    inline CommandEncoder CreateCommandEncoder(CommandEncoderDescriptor const * descriptor = nullptr) const;
    inline ComputePipeline CreateComputePipeline(ComputePipelineDescriptor const * descriptor) const;
    template <typename F, typename T>
    Future CreateComputePipelineAsync(ComputePipelineDescriptor const * descriptor, CallbackMode callbackMode, F callback, T userdata) const;
    template <typename F>
    Future CreateComputePipelineAsync(ComputePipelineDescriptor const * descriptor, CallbackMode callbackMode, F callback) const;
    inline Buffer CreateErrorBuffer(BufferDescriptor const * descriptor) const;
    inline ComputePipeline CreateErrorComputePipeline(StringView label) const;
    inline ExternalTexture CreateErrorExternalTexture() const;
    inline RenderPipeline CreateErrorRenderPipeline(StringView label) const;
    inline ShaderModule CreateErrorShaderModule(ShaderModuleDescriptor const * descriptor, StringView errorMessage) const;
    inline Texture CreateErrorTexture(TextureDescriptor const * descriptor) const;
    inline ExternalTexture CreateExternalTexture(ExternalTextureDescriptor const * externalTextureDescriptor) const;
    inline PipelineLayout CreatePipelineLayout(PipelineLayoutDescriptor const * descriptor) const;
    inline QuerySet CreateQuerySet(QuerySetDescriptor const * descriptor) const;
    inline RenderBundleEncoder CreateRenderBundleEncoder(RenderBundleEncoderDescriptor const * descriptor) const;
    inline RenderPipeline CreateRenderPipeline(RenderPipelineDescriptor const * descriptor) const;
    template <typename F, typename T>
    Future CreateRenderPipelineAsync(RenderPipelineDescriptor const * descriptor, CallbackMode callbackMode, F callback, T userdata) const;
    template <typename F>
    Future CreateRenderPipelineAsync(RenderPipelineDescriptor const * descriptor, CallbackMode callbackMode, F callback) const;
    inline ResourceTable CreateResourceTable(ResourceTableDescriptor const * descriptor) const;
    inline Sampler CreateSampler(SamplerDescriptor const * descriptor = nullptr) const;
    inline ShaderModule CreateShaderModule(ShaderModuleDescriptor const * descriptor) const;
    inline Texture CreateTexture(TextureDescriptor const * descriptor) const;
    inline void Destroy() const;
    inline void ForceLoss(DeviceLostReason type, StringView message) const;
    inline Adapter GetAdapter() const;
    inline ConvertibleStatus GetAdapterInfo(AdapterInfo * adapterInfo) const;
    inline ConvertibleStatus GetAHardwareBufferProperties(void * handle, AHardwareBufferProperties * properties) const;
    inline void GetFeatures(SupportedFeatures * features) const;
    inline ConvertibleStatus GetLimits(Limits * limits) const;
    inline Future GetLostFuture() const;
    inline Queue GetQueue() const;
    inline Bool HasFeature(FeatureName feature) const;
    inline SharedBufferMemory ImportSharedBufferMemory(SharedBufferMemoryDescriptor const * descriptor) const;
    inline SharedFence ImportSharedFence(SharedFenceDescriptor const * descriptor) const;
    inline SharedTextureMemory ImportSharedTextureMemory(SharedTextureMemoryDescriptor const * descriptor) const;
    inline void InjectError(ErrorType type, StringView message) const;
    template <typename F, typename T>
    Future PopErrorScope(CallbackMode callbackMode, F callback, T userdata) const;
    template <typename F>
    Future PopErrorScope(CallbackMode callbackMode, F callback) const;
    inline void PushErrorScope(ErrorFilter filter) const;
    inline void SetLabel(StringView label) const;
    template <typename F, typename T>
    void SetLoggingCallback(F callback, T userdata) const;
    template <typename F>
    void SetLoggingCallback(F callback) const;
    inline void Tick() const;
    inline void ValidateTextureDescriptor(TextureDescriptor const * descriptor) const;


  private:
    friend ObjectBase<Device, WGPUDevice>;
    static inline void WGPUAddRef(WGPUDevice handle);
    static inline void WGPURelease(WGPUDevice handle);
};

class ExternalTexture : public ObjectBase<ExternalTexture, WGPUExternalTexture> {
  public:
    using ObjectBase::ObjectBase;
    using ObjectBase::operator=;

    inline void Destroy() const;
    inline void Expire() const;
    inline void Refresh() const;
    inline void SetLabel(StringView label) const;


  private:
    friend ObjectBase<ExternalTexture, WGPUExternalTexture>;
    static inline void WGPUAddRef(WGPUExternalTexture handle);
    static inline void WGPURelease(WGPUExternalTexture handle);
};

class Instance : public ObjectBase<Instance, WGPUInstance> {
  public:
    using ObjectBase::ObjectBase;
    using ObjectBase::operator=;

    inline Surface CreateSurface(SurfaceDescriptor const * descriptor) const;
    inline void GetWGSLLanguageFeatures(SupportedWGSLLanguageFeatures * features) const;
    inline Bool HasWGSLLanguageFeature(WGSLLanguageFeatureName feature) const;
    inline void ProcessEvents() const;
    template <typename F, typename T>
    Future RequestAdapter(RequestAdapterOptions const * options, CallbackMode callbackMode, F callback, T userdata) const;
    template <typename F>
    Future RequestAdapter(RequestAdapterOptions const * options, CallbackMode callbackMode, F callback) const;
    inline WaitStatus WaitAny(size_t futureCount, FutureWaitInfo * futures, uint64_t timeoutNS) const;

    inline WaitStatus WaitAny(Future f, uint64_t timeout) const;

  private:
    friend ObjectBase<Instance, WGPUInstance>;
    static inline void WGPUAddRef(WGPUInstance handle);
    static inline void WGPURelease(WGPUInstance handle);
};

class PipelineLayout : public ObjectBase<PipelineLayout, WGPUPipelineLayout> {
  public:
    using ObjectBase::ObjectBase;
    using ObjectBase::operator=;

    inline void SetLabel(StringView label) const;


  private:
    friend ObjectBase<PipelineLayout, WGPUPipelineLayout>;
    static inline void WGPUAddRef(WGPUPipelineLayout handle);
    static inline void WGPURelease(WGPUPipelineLayout handle);
};

class QuerySet : public ObjectBase<QuerySet, WGPUQuerySet> {
  public:
    using ObjectBase::ObjectBase;
    using ObjectBase::operator=;

    inline void Destroy() const;
    inline uint32_t GetCount() const;
    inline QueryType GetType() const;
    inline void SetLabel(StringView label) const;


  private:
    friend ObjectBase<QuerySet, WGPUQuerySet>;
    static inline void WGPUAddRef(WGPUQuerySet handle);
    static inline void WGPURelease(WGPUQuerySet handle);
};

class Queue : public ObjectBase<Queue, WGPUQueue> {
  public:
    using ObjectBase::ObjectBase;
    using ObjectBase::operator=;

    inline void CopyExternalTextureForBrowser(ImageCopyExternalTexture const * source, TexelCopyTextureInfo const * destination, Extent3D const * copySize, CopyTextureForBrowserOptions const * options) const;
    inline void CopyTextureForBrowser(TexelCopyTextureInfo const * source, TexelCopyTextureInfo const * destination, Extent3D const * copySize, CopyTextureForBrowserOptions const * options) const;
    template <typename F, typename T>
    Future OnSubmittedWorkDone(CallbackMode callbackMode, F callback, T userdata) const;
    template <typename F>
    Future OnSubmittedWorkDone(CallbackMode callbackMode, F callback) const;
    inline void SetLabel(StringView label) const;
    inline void Submit(size_t commandCount, CommandBuffer const * commands) const;
    inline void WriteBuffer(Buffer const& buffer, uint64_t bufferOffset, void const * data, size_t size) const;
    inline void WriteTexture(TexelCopyTextureInfo const * destination, void const * data, size_t dataSize, TexelCopyBufferLayout const * dataLayout, Extent3D const * writeSize) const;


  private:
    friend ObjectBase<Queue, WGPUQueue>;
    static inline void WGPUAddRef(WGPUQueue handle);
    static inline void WGPURelease(WGPUQueue handle);
};

class RenderBundle : public ObjectBase<RenderBundle, WGPURenderBundle> {
  public:
    using ObjectBase::ObjectBase;
    using ObjectBase::operator=;

    inline void SetLabel(StringView label) const;


  private:
    friend ObjectBase<RenderBundle, WGPURenderBundle>;
    static inline void WGPUAddRef(WGPURenderBundle handle);
    static inline void WGPURelease(WGPURenderBundle handle);
};

class RenderPipeline : public ObjectBase<RenderPipeline, WGPURenderPipeline> {
  public:
    using ObjectBase::ObjectBase;
    using ObjectBase::operator=;

    inline BindGroupLayout GetBindGroupLayout(uint32_t groupIndex) const;
    inline void SetLabel(StringView label) const;


  private:
    friend ObjectBase<RenderPipeline, WGPURenderPipeline>;
    static inline void WGPUAddRef(WGPURenderPipeline handle);
    static inline void WGPURelease(WGPURenderPipeline handle);
};

class ResourceTable : public ObjectBase<ResourceTable, WGPUResourceTable> {
  public:
    using ObjectBase::ObjectBase;
    using ObjectBase::operator=;

    inline void Destroy() const;
    inline uint32_t GetSize() const;
    inline uint32_t InsertBinding(BindingResource const * resource) const;
    inline ConvertibleStatus RemoveBinding(uint32_t slot) const;
    inline void SetLabel(StringView label) const;
    inline ConvertibleStatus Update(uint32_t slot, BindingResource const * resource) const;


  private:
    friend ObjectBase<ResourceTable, WGPUResourceTable>;
    static inline void WGPUAddRef(WGPUResourceTable handle);
    static inline void WGPURelease(WGPUResourceTable handle);
};

class Sampler : public ObjectBase<Sampler, WGPUSampler> {
  public:
    using ObjectBase::ObjectBase;
    using ObjectBase::operator=;

    inline void SetLabel(StringView label) const;


  private:
    friend ObjectBase<Sampler, WGPUSampler>;
    static inline void WGPUAddRef(WGPUSampler handle);
    static inline void WGPURelease(WGPUSampler handle);
};

class ShaderModule : public ObjectBase<ShaderModule, WGPUShaderModule> {
  public:
    using ObjectBase::ObjectBase;
    using ObjectBase::operator=;

    template <typename F, typename T>
    Future GetCompilationInfo(CallbackMode callbackMode, F callback, T userdata) const;
    template <typename F>
    Future GetCompilationInfo(CallbackMode callbackMode, F callback) const;
    inline void SetLabel(StringView label) const;


  private:
    friend ObjectBase<ShaderModule, WGPUShaderModule>;
    static inline void WGPUAddRef(WGPUShaderModule handle);
    static inline void WGPURelease(WGPUShaderModule handle);
};

class SharedBufferMemory : public ObjectBase<SharedBufferMemory, WGPUSharedBufferMemory> {
  public:
    using ObjectBase::ObjectBase;
    using ObjectBase::operator=;

    inline ConvertibleStatus BeginAccess(Buffer const& buffer, SharedBufferMemoryBeginAccessDescriptor const * descriptor) const;
    inline Buffer CreateBuffer(BufferDescriptor const * descriptor = nullptr) const;
    inline ConvertibleStatus EndAccess(Buffer const& buffer, SharedBufferMemoryEndAccessState * descriptor) const;
    inline ConvertibleStatus GetProperties(SharedBufferMemoryProperties * properties) const;
    inline Bool IsDeviceLost() const;
    inline void SetLabel(StringView label) const;


  private:
    friend ObjectBase<SharedBufferMemory, WGPUSharedBufferMemory>;
    static inline void WGPUAddRef(WGPUSharedBufferMemory handle);
    static inline void WGPURelease(WGPUSharedBufferMemory handle);
};

class SharedFence : public ObjectBase<SharedFence, WGPUSharedFence> {
  public:
    using ObjectBase::ObjectBase;
    using ObjectBase::operator=;

    inline void ExportInfo(SharedFenceExportInfo * info) const;
    inline void SetLabel(StringView label) const;


  private:
    friend ObjectBase<SharedFence, WGPUSharedFence>;
    static inline void WGPUAddRef(WGPUSharedFence handle);
    static inline void WGPURelease(WGPUSharedFence handle);
};

class SharedTextureMemory : public ObjectBase<SharedTextureMemory, WGPUSharedTextureMemory> {
  public:
    using ObjectBase::ObjectBase;
    using ObjectBase::operator=;

    inline ConvertibleStatus BeginAccess(Texture const& texture, SharedTextureMemoryBeginAccessDescriptor const * descriptor) const;
    inline Texture CreateTexture(TextureDescriptor const * descriptor = nullptr) const;
    inline ConvertibleStatus EndAccess(Texture const& texture, SharedTextureMemoryEndAccessState * descriptor) const;
    inline ConvertibleStatus GetProperties(SharedTextureMemoryProperties * properties) const;
    inline Bool IsDeviceLost() const;
    inline void SetLabel(StringView label) const;


  private:
    friend ObjectBase<SharedTextureMemory, WGPUSharedTextureMemory>;
    static inline void WGPUAddRef(WGPUSharedTextureMemory handle);
    static inline void WGPURelease(WGPUSharedTextureMemory handle);
};

class Surface : public ObjectBase<Surface, WGPUSurface> {
  public:
    using ObjectBase::ObjectBase;
    using ObjectBase::operator=;

    inline void Configure(SurfaceConfiguration const * config) const;
    inline ConvertibleStatus GetCapabilities(Adapter const& adapter, SurfaceCapabilities * capabilities) const;
    inline void GetCurrentTexture(SurfaceTexture * surfaceTexture) const;
    inline ConvertibleStatus Present() const;
    inline void SetLabel(StringView label) const;
    inline void Unconfigure() const;


  private:
    friend ObjectBase<Surface, WGPUSurface>;
    static inline void WGPUAddRef(WGPUSurface handle);
    static inline void WGPURelease(WGPUSurface handle);
};

class TexelBufferView : public ObjectBase<TexelBufferView, WGPUTexelBufferView> {
  public:
    using ObjectBase::ObjectBase;
    using ObjectBase::operator=;

    inline void SetLabel(StringView label) const;


  private:
    friend ObjectBase<TexelBufferView, WGPUTexelBufferView>;
    static inline void WGPUAddRef(WGPUTexelBufferView handle);
    static inline void WGPURelease(WGPUTexelBufferView handle);
};

class Texture : public ObjectBase<Texture, WGPUTexture> {
  public:
    using ObjectBase::ObjectBase;
    using ObjectBase::operator=;

    inline TextureView CreateErrorView(TextureViewDescriptor const * descriptor = nullptr) const;
    inline TextureView CreateView(TextureViewDescriptor const * descriptor = nullptr) const;
    inline void Destroy() const;
    inline uint32_t GetDepthOrArrayLayers() const;
    inline TextureDimension GetDimension() const;
    inline TextureFormat GetFormat() const;
    inline uint32_t GetHeight() const;
    inline uint32_t GetMipLevelCount() const;
    inline uint32_t GetSampleCount() const;
    inline TextureViewDimension GetTextureBindingViewDimension() const;
    inline TextureUsage GetUsage() const;
    inline uint32_t GetWidth() const;
    inline void SetLabel(StringView label) const;
    inline void SetOwnershipForMemoryDump(uint64_t ownerGuid = 0) const;


  private:
    friend ObjectBase<Texture, WGPUTexture>;
    static inline void WGPUAddRef(WGPUTexture handle);
    static inline void WGPURelease(WGPUTexture handle);
};

class TextureView : public ObjectBase<TextureView, WGPUTextureView> {
  public:
    using ObjectBase::ObjectBase;
    using ObjectBase::operator=;

    inline void SetLabel(StringView label) const;


  private:
    friend ObjectBase<TextureView, WGPUTextureView>;
    static inline void WGPUAddRef(WGPUTextureView handle);
    static inline void WGPURelease(WGPUTextureView handle);
};

class ComputePassEncoder : public ObjectBase<ComputePassEncoder, WGPUComputePassEncoder> {
  public:
    using ObjectBase::ObjectBase;
    using ObjectBase::operator=;

    inline void DispatchWorkgroups(uint32_t workgroupCountX, uint32_t workgroupCountY = 1, uint32_t workgroupCountZ = 1) const;
    inline void DispatchWorkgroupsIndirect(Buffer const& indirectBuffer, uint64_t indirectOffset) const;
    inline void End() const;
    inline void InsertDebugMarker(StringView markerLabel) const;
    inline void PopDebugGroup() const;
    inline void PushDebugGroup(StringView groupLabel) const;
    inline void SetBindGroup(uint32_t groupIndex, BindGroup const& group = nullptr, size_t dynamicOffsetCount = 0, uint32_t const * dynamicOffsets = nullptr) const;
    inline void SetImmediates(uint32_t offset, void const * data, size_t size) const;
    inline void SetLabel(StringView label) const;
    inline void SetPipeline(ComputePipeline const& pipeline) const;
    inline void SetResourceTable(ResourceTable const& table = nullptr) const;
    inline void WriteTimestamp(QuerySet const& querySet, uint32_t queryIndex) const;


  private:
    friend ObjectBase<ComputePassEncoder, WGPUComputePassEncoder>;
    static inline void WGPUAddRef(WGPUComputePassEncoder handle);
    static inline void WGPURelease(WGPUComputePassEncoder handle);
};

class RenderBundleEncoder : public ObjectBase<RenderBundleEncoder, WGPURenderBundleEncoder> {
  public:
    using ObjectBase::ObjectBase;
    using ObjectBase::operator=;

    inline void Draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t firstVertex = 0, uint32_t firstInstance = 0) const;
    inline void DrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t firstIndex = 0, int32_t baseVertex = 0, uint32_t firstInstance = 0) const;
    inline void DrawIndexedIndirect(Buffer const& indirectBuffer, uint64_t indirectOffset) const;
    inline void DrawIndirect(Buffer const& indirectBuffer, uint64_t indirectOffset) const;
    inline RenderBundle Finish(RenderBundleDescriptor const * descriptor = nullptr) const;
    inline void InsertDebugMarker(StringView markerLabel) const;
    inline void PopDebugGroup() const;
    inline void PushDebugGroup(StringView groupLabel) const;
    inline void SetBindGroup(uint32_t groupIndex, BindGroup const& group = nullptr, size_t dynamicOffsetCount = 0, uint32_t const * dynamicOffsets = nullptr) const;
    inline void SetImmediates(uint32_t offset, void const * data, size_t size) const;
    inline void SetIndexBuffer(Buffer const& buffer, IndexFormat format, uint64_t offset = 0, uint64_t size = kWholeSize) const;
    inline void SetLabel(StringView label) const;
    inline void SetPipeline(RenderPipeline const& pipeline) const;
    inline void SetResourceTable(ResourceTable const& table = nullptr) const;
    inline void SetVertexBuffer(uint32_t slot, Buffer const& buffer = nullptr, uint64_t offset = 0, uint64_t size = kWholeSize) const;


  private:
    friend ObjectBase<RenderBundleEncoder, WGPURenderBundleEncoder>;
    static inline void WGPUAddRef(WGPURenderBundleEncoder handle);
    static inline void WGPURelease(WGPURenderBundleEncoder handle);
};

class RenderPassEncoder : public ObjectBase<RenderPassEncoder, WGPURenderPassEncoder> {
  public:
    using ObjectBase::ObjectBase;
    using ObjectBase::operator=;

    inline void BeginOcclusionQuery(uint32_t queryIndex) const;
    inline void Draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t firstVertex = 0, uint32_t firstInstance = 0) const;
    inline void DrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t firstIndex = 0, int32_t baseVertex = 0, uint32_t firstInstance = 0) const;
    inline void DrawIndexedIndirect(Buffer const& indirectBuffer, uint64_t indirectOffset) const;
    inline void DrawIndirect(Buffer const& indirectBuffer, uint64_t indirectOffset) const;
    inline void End() const;
    inline void EndOcclusionQuery() const;
    inline void ExecuteBundles(size_t bundleCount, RenderBundle const * bundles) const;
    inline void InsertDebugMarker(StringView markerLabel) const;
    inline void MultiDrawIndexedIndirect(Buffer const& indirectBuffer, uint64_t indirectOffset, uint32_t maxDrawCount, Buffer const& drawCountBuffer = nullptr, uint64_t drawCountBufferOffset = 0) const;
    inline void MultiDrawIndirect(Buffer const& indirectBuffer, uint64_t indirectOffset, uint32_t maxDrawCount, Buffer const& drawCountBuffer = nullptr, uint64_t drawCountBufferOffset = 0) const;
    inline void PixelLocalStorageBarrier() const;
    inline void PopDebugGroup() const;
    inline void PushDebugGroup(StringView groupLabel) const;
    inline void SetBindGroup(uint32_t groupIndex, BindGroup const& group = nullptr, size_t dynamicOffsetCount = 0, uint32_t const * dynamicOffsets = nullptr) const;
    inline void SetBlendConstant(Color const * color) const;
    inline void SetImmediates(uint32_t offset, void const * data, size_t size) const;
    inline void SetIndexBuffer(Buffer const& buffer, IndexFormat format, uint64_t offset = 0, uint64_t size = kWholeSize) const;
    inline void SetLabel(StringView label) const;
    inline void SetPipeline(RenderPipeline const& pipeline) const;
    inline void SetResourceTable(ResourceTable const& table = nullptr) const;
    inline void SetScissorRect(uint32_t x, uint32_t y, uint32_t width, uint32_t height) const;
    inline void SetStencilReference(uint32_t reference) const;
    inline void SetVertexBuffer(uint32_t slot, Buffer const& buffer = nullptr, uint64_t offset = 0, uint64_t size = kWholeSize) const;
    inline void SetViewport(float x, float y, float width, float height, float minDepth, float maxDepth) const;
    inline void WriteTimestamp(QuerySet const& querySet, uint32_t queryIndex) const;


  private:
    friend ObjectBase<RenderPassEncoder, WGPURenderPassEncoder>;
    static inline void WGPUAddRef(WGPURenderPassEncoder handle);
    static inline void WGPURelease(WGPURenderPassEncoder handle);
};


// ChainedStruct
static_assert(sizeof(ChainedStruct) == sizeof(WGPUChainedStruct),
    "sizeof mismatch for ChainedStruct");
static_assert(alignof(ChainedStruct) == alignof(WGPUChainedStruct),
    "alignof mismatch for ChainedStruct");
static_assert(offsetof(ChainedStruct, nextInChain) == offsetof(WGPUChainedStruct, next),
    "offsetof mismatch for ChainedStruct::nextInChain");
static_assert(offsetof(ChainedStruct, sType) == offsetof(WGPUChainedStruct, sType),
    "offsetof mismatch for ChainedStruct::sType");

// NOLINTBEGIN(bugprone-invalid-enum-default-initialization)

// Can be chained in AdapterInfo
struct AdapterPropertiesD3D : ChainedStructOut {
    inline AdapterPropertiesD3D();
    struct Init;
    explicit(false) inline AdapterPropertiesD3D(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUAdapterPropertiesD3D&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStructOut), alignof(uint32_t));
    alignas(kFirstMemberAlignment) uint32_t shaderModel;
};

// Can be chained in AdapterInfo
struct AdapterPropertiesDrm : ChainedStructOut {
    inline AdapterPropertiesDrm();
    struct Init;
    explicit(false) inline AdapterPropertiesDrm(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUAdapterPropertiesDrm&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStructOut), alignof(Bool));
    alignas(kFirstMemberAlignment) Bool hasPrimary = false;
    Bool hasRender = false;
    uint64_t primaryMajor = 0;
    uint64_t primaryMinor = 0;
    uint64_t renderMajor = 0;
    uint64_t renderMinor = 0;
};

// Can be chained in AdapterInfo
struct AdapterPropertiesVk : ChainedStructOut {
    inline AdapterPropertiesVk();
    struct Init;
    explicit(false) inline AdapterPropertiesVk(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUAdapterPropertiesVk&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStructOut), alignof(uint32_t));
    alignas(kFirstMemberAlignment) uint32_t driverVersion;
};

// Can be chained in AdapterInfo
struct AdapterPropertiesWGPU : ChainedStructOut {
    inline AdapterPropertiesWGPU();
    struct Init;
    explicit(false) inline AdapterPropertiesWGPU(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUAdapterPropertiesWGPU&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStructOut), alignof(BackendType));
    alignas(kFirstMemberAlignment) BackendType backendType = BackendType::Undefined;
};

struct BindingResource {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUBindingResource&() const noexcept;

    ChainedStruct const * nextInChain = nullptr;
    Buffer buffer = nullptr;
    uint64_t offset = 0;
    uint64_t size = kWholeSize;
    Sampler sampler = nullptr;
    TextureView textureView = nullptr;
};

struct BlendComponent {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUBlendComponent&() const noexcept;

    BlendOperation operation = BlendOperation::Undefined;
    BlendFactor srcFactor = BlendFactor::Undefined;
    BlendFactor dstFactor = BlendFactor::Undefined;
};

struct BufferBindingLayout {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUBufferBindingLayout&() const noexcept;

    ChainedStruct const * nextInChain = nullptr;
    BufferBindingType type = BufferBindingType::Undefined;
    Bool hasDynamicOffset = false;
    uint64_t minBindingSize = 0;
};

// Can be chained in BufferDescriptor
struct BufferHostMappedPointer : ChainedStruct {
    inline BufferHostMappedPointer();
    struct Init;
    explicit(false) inline BufferHostMappedPointer(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUBufferHostMappedPointer&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStruct), alignof(void *));
    alignas(kFirstMemberAlignment) void * pointer;
    Callback disposeCallback;
    void * userdata;
};

struct Color {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUColor&() const noexcept;

    double r;
    double g;
    double b;
    double a;
};

struct ColorSpaceDawn {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUColorSpaceDawn&() const noexcept;

    ChainedStruct const * nextInChain = nullptr;
    ColorSpacePrimariesDawn primaries = {};
    ColorSpaceTransferDawn transfer = ColorSpaceTransferDawn::Identity;
    ColorSpaceYCbCrRangeDawn yCbCrRange = ColorSpaceYCbCrRangeDawn::Identity;
    ColorSpaceYCbCrMatrixDawn yCbCrMatrix = ColorSpaceYCbCrMatrixDawn::Identity;
    float hdrReferenceWhiteLuminance = 0;
};

// Can be chained in ColorTargetState
struct ColorTargetStateExpandResolveTextureDawn : ChainedStruct {
    inline ColorTargetStateExpandResolveTextureDawn();
    struct Init;
    explicit(false) inline ColorTargetStateExpandResolveTextureDawn(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUColorTargetStateExpandResolveTextureDawn&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStruct), alignof(Bool));
    alignas(kFirstMemberAlignment) Bool enabled = false;
};

struct CommandBufferDescriptor {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUCommandBufferDescriptor&() const noexcept;

    ChainedStruct const * nextInChain = nullptr;
    StringView label = {};
};

// Can be chained in Limits
struct CompatibilityModeLimits : ChainedStructOut {
    inline CompatibilityModeLimits();
    struct Init;
    explicit(false) inline CompatibilityModeLimits(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUCompatibilityModeLimits&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStructOut), alignof(uint32_t));
    alignas(kFirstMemberAlignment) uint32_t maxStorageBuffersInVertexStage = kLimitU32Undefined;
    uint32_t maxStorageTexturesInVertexStage = kLimitU32Undefined;
    uint32_t maxStorageBuffersInFragmentStage = kLimitU32Undefined;
    uint32_t maxStorageTexturesInFragmentStage = kLimitU32Undefined;
};

struct ConstantEntry {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUConstantEntry&() const noexcept;

    ChainedStruct const * nextInChain = nullptr;
    StringView key = {};
    double value;
};

struct CopyTextureForBrowserOptions {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUCopyTextureForBrowserOptions&() const noexcept;

    ChainedStruct const * nextInChain = nullptr;
    Bool flipY = false;
    Bool needsColorSpaceConversion = false;
    AlphaMode srcAlphaMode = AlphaMode::Unpremultiplied;
    float const * srcTransferFunctionParameters = nullptr;
    float const * conversionMatrix = nullptr;
    float const * dstTransferFunctionParameters = nullptr;
    AlphaMode dstAlphaMode = AlphaMode::Unpremultiplied;
    Bool internalUsage = false;
};

// Can be chained in AdapterInfo
struct DawnAdapterPropertiesPowerPreference : ChainedStructOut {
    inline DawnAdapterPropertiesPowerPreference();
    struct Init;
    explicit(false) inline DawnAdapterPropertiesPowerPreference(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUDawnAdapterPropertiesPowerPreference&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStructOut), alignof(PowerPreference));
    alignas(kFirstMemberAlignment) PowerPreference powerPreference = PowerPreference::Undefined;
};

// Can be chained in BufferDescriptor
struct DawnBufferDescriptorErrorInfoFromWireClient : ChainedStruct {
    inline DawnBufferDescriptorErrorInfoFromWireClient();
    struct Init;
    explicit(false) inline DawnBufferDescriptorErrorInfoFromWireClient(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUDawnBufferDescriptorErrorInfoFromWireClient&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStruct), alignof(Bool));
    alignas(kFirstMemberAlignment) Bool outOfMemory = false;
};

namespace detail {
struct DawnCacheDeviceDescriptor {
    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStruct), alignof(StringView));
    alignas(kFirstMemberAlignment) StringView isolationKey = {};
    WGPUDawnLoadCacheDataCallbackInfo dawnLoadCacheDataCallbackInfo = WGPU_DAWN_LOAD_CACHE_DATA_CALLBACK_INFO_INIT;
    WGPUDawnStoreCacheDataCallbackInfo dawnStoreCacheDataCallbackInfo = WGPU_DAWN_STORE_CACHE_DATA_CALLBACK_INFO_INIT;
};
}  // namespace detail
// Can be chained in DeviceDescriptor
struct DawnCacheDeviceDescriptor : ChainedStruct, protected detail::DawnCacheDeviceDescriptor {
    inline DawnCacheDeviceDescriptor();
    struct Init;
    explicit(false) inline DawnCacheDeviceDescriptor(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUDawnCacheDeviceDescriptor&() const noexcept;

    using detail::DawnCacheDeviceDescriptor::kFirstMemberAlignment;
    using detail::DawnCacheDeviceDescriptor::isolationKey;

    template <typename F, typename T>
    void SetDawnLoadCacheDataCallback(F callback, T userdata);
    template <typename F>
    void SetDawnLoadCacheDataCallback(F callback);
    template <typename F, typename T>
    void SetDawnStoreCacheDataCallback(F callback, T userdata);
    template <typename F>
    void SetDawnStoreCacheDataCallback(F callback);
};

// Can be chained in CompilationMessage
struct DawnCompilationMessageUtf16 : ChainedStruct {
    inline DawnCompilationMessageUtf16();
    struct Init;
    explicit(false) inline DawnCompilationMessageUtf16(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUDawnCompilationMessageUtf16&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStruct), alignof(uint64_t));
    alignas(kFirstMemberAlignment) uint64_t linePos;
    uint64_t offset;
    uint64_t length;
};

// Can be chained in DeviceDescriptor
struct DawnConsumeAdapterDescriptor : ChainedStruct {
    inline DawnConsumeAdapterDescriptor();
    struct Init;
    explicit(false) inline DawnConsumeAdapterDescriptor(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUDawnConsumeAdapterDescriptor&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStruct), alignof(Bool));
    alignas(kFirstMemberAlignment) Bool consumeAdapter = false;
};

// Can be chained in DeviceDescriptor
struct DawnDeviceAllocatorControl : ChainedStruct {
    inline DawnDeviceAllocatorControl();
    struct Init;
    explicit(false) inline DawnDeviceAllocatorControl(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUDawnDeviceAllocatorControl&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStruct), alignof(size_t));
    alignas(kFirstMemberAlignment) size_t allocatorHeapBlockSize = 0;
};

struct DawnDrmFormatProperties {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUDawnDrmFormatProperties&() const noexcept;

    uint64_t modifier;
    uint32_t modifierPlaneCount;
};

// Can be chained in CommandEncoderDescriptor
struct DawnEncoderInternalUsageDescriptor : ChainedStruct {
    inline DawnEncoderInternalUsageDescriptor();
    struct Init;
    explicit(false) inline DawnEncoderInternalUsageDescriptor(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUDawnEncoderInternalUsageDescriptor&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStruct), alignof(Bool));
    alignas(kFirstMemberAlignment) Bool useInternalUsages = false;
};

// Can be chained in BufferDescriptor
struct DawnFakeBufferOOMForTesting : ChainedStruct {
    inline DawnFakeBufferOOMForTesting();
    struct Init;
    explicit(false) inline DawnFakeBufferOOMForTesting(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUDawnFakeBufferOOMForTesting&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStruct), alignof(Bool));
    alignas(kFirstMemberAlignment) Bool fakeOOMAtWireClientMap;
    Bool fakeOOMAtNativeMap;
    Bool fakeOOMAtDevice;
};

// Can be chained in DeviceDescriptor
struct DawnFakeDeviceInitializeErrorForTesting : ChainedStruct {
    inline DawnFakeDeviceInitializeErrorForTesting();
    struct Init;
    explicit(false) inline DawnFakeDeviceInitializeErrorForTesting(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUDawnFakeDeviceInitializeErrorForTesting&() const noexcept;

    };

// Can be chained in Limits
struct DawnHostMappedPointerLimits : ChainedStructOut {
    inline DawnHostMappedPointerLimits();
    struct Init;
    explicit(false) inline DawnHostMappedPointerLimits(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUDawnHostMappedPointerLimits&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStructOut), alignof(uint32_t));
    alignas(kFirstMemberAlignment) uint32_t hostMappedPointerAlignment = kLimitU32Undefined;
};

struct DawnInjectedInvalidSType : ChainedStruct {
    inline DawnInjectedInvalidSType();
    struct Init;
    explicit(false) inline DawnInjectedInvalidSType(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUDawnInjectedInvalidSType&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStruct), alignof(SType));
    alignas(kFirstMemberAlignment) SType invalidSType = {};
};

// Can be chained in RenderPassDescriptor
struct DawnRenderPassSampleCount : ChainedStruct {
    inline DawnRenderPassSampleCount();
    struct Init;
    explicit(false) inline DawnRenderPassSampleCount(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUDawnRenderPassSampleCount&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStruct), alignof(uint32_t));
    alignas(kFirstMemberAlignment) uint32_t sampleCount = 1;
};

// Can be chained in ShaderModuleDescriptor
struct DawnShaderModuleSPIRVOptionsDescriptor : ChainedStruct {
    inline DawnShaderModuleSPIRVOptionsDescriptor();
    struct Init;
    explicit(false) inline DawnShaderModuleSPIRVOptionsDescriptor(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUDawnShaderModuleSPIRVOptionsDescriptor&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStruct), alignof(Bool));
    alignas(kFirstMemberAlignment) Bool allowNonUniformDerivatives = false;
};

// Can be chained in Limits
struct DawnTexelCopyBufferRowAlignmentLimits : ChainedStructOut {
    inline DawnTexelCopyBufferRowAlignmentLimits();
    struct Init;
    explicit(false) inline DawnTexelCopyBufferRowAlignmentLimits(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUDawnTexelCopyBufferRowAlignmentLimits&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStructOut), alignof(uint32_t));
    alignas(kFirstMemberAlignment) uint32_t minTexelCopyBufferRowAlignment = kLimitU32Undefined;
};

// Can be chained in TextureDescriptor
struct DawnTextureInternalUsageDescriptor : ChainedStruct {
    inline DawnTextureInternalUsageDescriptor();
    struct Init;
    explicit(false) inline DawnTextureInternalUsageDescriptor(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUDawnTextureInternalUsageDescriptor&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStruct), alignof(TextureUsage));
    alignas(kFirstMemberAlignment) TextureUsage internalUsage = TextureUsage::None;
};

// Can be chained in InstanceDescriptor
// Can be chained in RequestAdapterOptions
// Can be chained in DeviceDescriptor
struct DawnTogglesDescriptor : ChainedStruct {
    inline DawnTogglesDescriptor();
    struct Init;
    explicit(false) inline DawnTogglesDescriptor(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUDawnTogglesDescriptor&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStruct), alignof(size_t));
    alignas(kFirstMemberAlignment) size_t enabledToggleCount = 0;
    const char* const * enabledToggles = nullptr;
    size_t disabledToggleCount = 0;
    const char* const * disabledToggles = nullptr;
};

// Can be chained in InstanceDescriptor
struct DawnWGSLBlocklist : ChainedStruct {
    inline DawnWGSLBlocklist();
    struct Init;
    explicit(false) inline DawnWGSLBlocklist(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUDawnWGSLBlocklist&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStruct), alignof(size_t));
    alignas(kFirstMemberAlignment) size_t blocklistedFeatureCount = 0;
    const char* const * blocklistedFeatures = nullptr;
};

// Can be chained in InstanceDescriptor
struct DawnWireWGSLControl : ChainedStruct {
    inline DawnWireWGSLControl();
    struct Init;
    explicit(false) inline DawnWireWGSLControl(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUDawnWireWGSLControl&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStruct), alignof(Bool));
    alignas(kFirstMemberAlignment) Bool enableExperimental = false;
    Bool enableUnsafe = false;
    Bool enableTesting = false;
};

// Can be chained in SurfaceDescriptor
struct EmscriptenSurfaceSourceCanvasHTMLSelector : ChainedStruct {
    inline EmscriptenSurfaceSourceCanvasHTMLSelector();
    struct Init;
    explicit(false) inline EmscriptenSurfaceSourceCanvasHTMLSelector(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUEmscriptenSurfaceSourceCanvasHTMLSelector&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStruct), alignof(StringView));
    alignas(kFirstMemberAlignment) StringView selector = {};
};

struct Extent2D {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUExtent2D&() const noexcept;

    uint32_t width;
    uint32_t height;
};

struct Extent3D {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUExtent3D&() const noexcept;

    uint32_t width;
    uint32_t height = 1;
    uint32_t depthOrArrayLayers = 1;
};

// Can be chained in BindGroupEntry
struct ExternalTextureBindingEntry : ChainedStruct {
    inline ExternalTextureBindingEntry();
    struct Init;
    explicit(false) inline ExternalTextureBindingEntry(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUExternalTextureBindingEntry&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStruct), alignof(ExternalTexture));
    alignas(kFirstMemberAlignment) ExternalTexture externalTexture = nullptr;
};

// Can be chained in BindGroupLayoutEntry
struct ExternalTextureBindingLayout : ChainedStruct {
    inline ExternalTextureBindingLayout();
    struct Init;
    explicit(false) inline ExternalTextureBindingLayout(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUExternalTextureBindingLayout&() const noexcept;

    };

struct Future {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUFuture&() const noexcept;

    uint64_t id;
};

struct InstanceLimits {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUInstanceLimits&() const noexcept;

    ChainedStructOut  * nextInChain = nullptr;
    size_t timedWaitAnyMaxCount = 0;
};

struct INTERNAL_HAVE_EMDAWNWEBGPU_HEADER {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUINTERNAL_HAVE_EMDAWNWEBGPU_HEADER&() const noexcept;

    Bool unused = false;
};

struct MemoryHeapInfo {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUMemoryHeapInfo&() const noexcept;

    HeapProperty properties = HeapProperty::None;
    uint64_t size;
};

struct MultisampleState {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUMultisampleState&() const noexcept;

    ChainedStruct const * nextInChain = nullptr;
    uint32_t count = 1;
    uint32_t mask = 0xFFFFFFFF;
    Bool alphaToCoverageEnabled = false;
};

struct Origin2D {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUOrigin2D&() const noexcept;

    uint32_t x = 0;
    uint32_t y = 0;
};

struct Origin3D {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUOrigin3D&() const noexcept;

    uint32_t x = 0;
    uint32_t y = 0;
    uint32_t z = 0;
};

struct PassTimestampWrites {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUPassTimestampWrites&() const noexcept;

    ChainedStruct const * nextInChain = nullptr;
    QuerySet querySet = nullptr;
    uint32_t beginningOfPassWriteIndex = kQuerySetIndexUndefined;
    uint32_t endOfPassWriteIndex = kQuerySetIndexUndefined;
};

// Can be chained in PipelineLayoutDescriptor
struct PipelineLayoutResourceTable : ChainedStruct {
    inline PipelineLayoutResourceTable();
    struct Init;
    explicit(false) inline PipelineLayoutResourceTable(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUPipelineLayoutResourceTable&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStruct), alignof(Bool));
    alignas(kFirstMemberAlignment) Bool usesResourceTable = false;
};

struct PipelineLayoutStorageAttachment {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUPipelineLayoutStorageAttachment&() const noexcept;

    ChainedStruct const * nextInChain = nullptr;
    uint64_t offset = 0;
    TextureFormat format = TextureFormat::Undefined;
};

struct PrimitiveState {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUPrimitiveState&() const noexcept;

    ChainedStruct const * nextInChain = nullptr;
    PrimitiveTopology topology = PrimitiveTopology::Undefined;
    IndexFormat stripIndexFormat = IndexFormat::Undefined;
    FrontFace frontFace = FrontFace::Undefined;
    CullMode cullMode = CullMode::Undefined;
    Bool unclippedDepth = false;
};

struct QuerySetDescriptor {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUQuerySetDescriptor&() const noexcept;

    ChainedStruct const * nextInChain = nullptr;
    StringView label = {};
    QueryType type = {};
    uint32_t count;
};

struct QueueDescriptor {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUQueueDescriptor&() const noexcept;

    ChainedStruct const * nextInChain = nullptr;
    StringView label = {};
};

struct RenderBundleDescriptor {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPURenderBundleDescriptor&() const noexcept;

    ChainedStruct const * nextInChain = nullptr;
    StringView label = {};
};

struct RenderBundleEncoderDescriptor {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPURenderBundleEncoderDescriptor&() const noexcept;

    ChainedStruct const * nextInChain = nullptr;
    StringView label = {};
    size_t colorFormatCount;
    TextureFormat const * colorFormats = nullptr;
    TextureFormat depthStencilFormat = TextureFormat::Undefined;
    uint32_t sampleCount = 1;
    Bool depthReadOnly = false;
    Bool stencilReadOnly = false;
};

struct RenderPassDepthStencilAttachment {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPURenderPassDepthStencilAttachment&() const noexcept;

    ChainedStruct const * nextInChain = nullptr;
    TextureView view = nullptr;
    LoadOp depthLoadOp = LoadOp::Undefined;
    StoreOp depthStoreOp = StoreOp::Undefined;
    float depthClearValue = kDepthClearValueUndefined;
    Bool depthReadOnly = false;
    LoadOp stencilLoadOp = LoadOp::Undefined;
    StoreOp stencilStoreOp = StoreOp::Undefined;
    uint32_t stencilClearValue = 0;
    Bool stencilReadOnly = false;
};

// Can be chained in RenderPassDescriptor
struct RenderPassDescriptorResolveRect : ChainedStruct {
    inline RenderPassDescriptorResolveRect();
    struct Init;
    explicit(false) inline RenderPassDescriptorResolveRect(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPURenderPassDescriptorResolveRect&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStruct), alignof(uint32_t));
    alignas(kFirstMemberAlignment) uint32_t colorOffsetX;
    uint32_t colorOffsetY;
    uint32_t resolveOffsetX;
    uint32_t resolveOffsetY;
    uint32_t width;
    uint32_t height;
};

// Can be chained in RenderPassDescriptor
struct RenderPassMaxDrawCount : ChainedStruct {
    inline RenderPassMaxDrawCount();
    struct Init;
    explicit(false) inline RenderPassMaxDrawCount(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPURenderPassMaxDrawCount&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStruct), alignof(uint64_t));
    alignas(kFirstMemberAlignment) uint64_t maxDrawCount = 50000000;
};

// Can be chained in RequestAdapterOptions
struct RequestAdapterWebGPUBackendOptions : ChainedStruct {
    inline RequestAdapterWebGPUBackendOptions();
    struct Init;
    explicit(false) inline RequestAdapterWebGPUBackendOptions(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPURequestAdapterWebGPUBackendOptions&() const noexcept;

    };

// Can be chained in RequestAdapterOptions
struct RequestAdapterWebXROptions : ChainedStruct {
    inline RequestAdapterWebXROptions();
    struct Init;
    explicit(false) inline RequestAdapterWebXROptions(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPURequestAdapterWebXROptions&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStruct), alignof(Bool));
    alignas(kFirstMemberAlignment) Bool xrCompatible;
};

struct ResourceTableDescriptor {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUResourceTableDescriptor&() const noexcept;

    ChainedStruct const * nextInChain = nullptr;
    StringView label = {};
    uint32_t size;
};

struct SamplerBindingLayout {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUSamplerBindingLayout&() const noexcept;

    ChainedStruct const * nextInChain = nullptr;
    SamplerBindingType type = SamplerBindingType::Undefined;
};

// Can be chained in ShaderModuleDescriptor
struct ShaderModuleCompilationOptions : ChainedStruct {
    inline ShaderModuleCompilationOptions();
    struct Init;
    explicit(false) inline ShaderModuleCompilationOptions(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUShaderModuleCompilationOptions&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStruct), alignof(Bool));
    alignas(kFirstMemberAlignment) Bool strictMath;
};

// Can be chained in ShaderModuleDescriptor
struct ShaderSourceSPIRV : ChainedStruct {
    inline ShaderSourceSPIRV();
    struct Init;
    explicit(false) inline ShaderSourceSPIRV(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUShaderSourceSPIRV&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStruct), alignof(uint32_t));
    alignas(kFirstMemberAlignment) uint32_t codeSize;
    uint32_t const * code = nullptr;
};

// Can be chained in ShaderModuleDescriptor
struct ShaderSourceWGSL : ChainedStruct {
    inline ShaderSourceWGSL();
    struct Init;
    explicit(false) inline ShaderSourceWGSL(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUShaderSourceWGSL&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStruct), alignof(StringView));
    alignas(kFirstMemberAlignment) StringView code = {};
};

struct SharedBufferMemoryBeginAccessDescriptor {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUSharedBufferMemoryBeginAccessDescriptor&() const noexcept;

    ChainedStruct const * nextInChain = nullptr;
    Bool initialized;
    size_t fenceCount = 0;
    SharedFence const * fences = nullptr;
    size_t signaledValueCount = 0;
    uint64_t const * signaledValues = nullptr;
};

struct SharedBufferMemoryEndAccessState {
    inline SharedBufferMemoryEndAccessState();
    inline ~SharedBufferMemoryEndAccessState();
    SharedBufferMemoryEndAccessState(const SharedBufferMemoryEndAccessState&) = delete;
    SharedBufferMemoryEndAccessState& operator=(const SharedBufferMemoryEndAccessState&) = delete;
    inline SharedBufferMemoryEndAccessState(SharedBufferMemoryEndAccessState&&);
    inline SharedBufferMemoryEndAccessState& operator=(SharedBufferMemoryEndAccessState&&);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUSharedBufferMemoryEndAccessState&() const noexcept;

    ChainedStructOut  * nextInChain = nullptr;
    Bool const initialized = {};
    size_t const fenceCount = 0;
    SharedFence const * const fences = nullptr;
    size_t const signaledValueCount = 0;
    uint64_t const * const signaledValues = nullptr;

  private:
    inline void FreeMembers();
    static inline void Reset(SharedBufferMemoryEndAccessState& value);
};

// Can be chained in SharedBufferMemoryDescriptor
struct SharedBufferMemoryFromWindowsHandleDescriptor : ChainedStruct {
    inline SharedBufferMemoryFromWindowsHandleDescriptor();
    struct Init;
    explicit(false) inline SharedBufferMemoryFromWindowsHandleDescriptor(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUSharedBufferMemoryFromWindowsHandleDescriptor&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStruct), alignof(void *));
    alignas(kFirstMemberAlignment) void * handle;
    uint64_t size;
};

struct SharedBufferMemoryProperties {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUSharedBufferMemoryProperties&() const noexcept;

    ChainedStructOut  * nextInChain = nullptr;
    BufferUsage usage = BufferUsage::None;
    uint64_t size;
};

// Can be chained in SharedFenceDescriptor
struct SharedFenceDXGISharedHandleDescriptor : ChainedStruct {
    inline SharedFenceDXGISharedHandleDescriptor();
    struct Init;
    explicit(false) inline SharedFenceDXGISharedHandleDescriptor(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUSharedFenceDXGISharedHandleDescriptor&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStruct), alignof(void *));
    alignas(kFirstMemberAlignment) void * handle;
};

// Can be chained in SharedFenceExportInfo
struct SharedFenceDXGISharedHandleExportInfo : ChainedStructOut {
    inline SharedFenceDXGISharedHandleExportInfo();
    struct Init;
    explicit(false) inline SharedFenceDXGISharedHandleExportInfo(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUSharedFenceDXGISharedHandleExportInfo&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStructOut), alignof(void *));
    alignas(kFirstMemberAlignment) void * handle;
};

// Can be chained in SharedFenceDescriptor
struct SharedFenceEGLSyncDescriptor : ChainedStruct {
    inline SharedFenceEGLSyncDescriptor();
    struct Init;
    explicit(false) inline SharedFenceEGLSyncDescriptor(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUSharedFenceEGLSyncDescriptor&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStruct), alignof(void *));
    alignas(kFirstMemberAlignment) void * sync;
};

// Can be chained in SharedFenceExportInfo
struct SharedFenceEGLSyncExportInfo : ChainedStructOut {
    inline SharedFenceEGLSyncExportInfo();
    struct Init;
    explicit(false) inline SharedFenceEGLSyncExportInfo(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUSharedFenceEGLSyncExportInfo&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStructOut), alignof(void *));
    alignas(kFirstMemberAlignment) void * sync;
};

// Can be chained in SharedFenceDescriptor
struct SharedFenceMTLSharedEventDescriptor : ChainedStruct {
    inline SharedFenceMTLSharedEventDescriptor();
    struct Init;
    explicit(false) inline SharedFenceMTLSharedEventDescriptor(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUSharedFenceMTLSharedEventDescriptor&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStruct), alignof(void *));
    alignas(kFirstMemberAlignment) void * sharedEvent;
};

// Can be chained in SharedFenceExportInfo
struct SharedFenceMTLSharedEventExportInfo : ChainedStructOut {
    inline SharedFenceMTLSharedEventExportInfo();
    struct Init;
    explicit(false) inline SharedFenceMTLSharedEventExportInfo(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUSharedFenceMTLSharedEventExportInfo&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStructOut), alignof(void *));
    alignas(kFirstMemberAlignment) void * sharedEvent;
};

// Can be chained in SharedFenceDescriptor
struct SharedFenceSyncFDDescriptor : ChainedStruct {
    inline SharedFenceSyncFDDescriptor();
    struct Init;
    explicit(false) inline SharedFenceSyncFDDescriptor(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUSharedFenceSyncFDDescriptor&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStruct), alignof(int));
    alignas(kFirstMemberAlignment) int handle;
};

// Can be chained in SharedFenceExportInfo
struct SharedFenceSyncFDExportInfo : ChainedStructOut {
    inline SharedFenceSyncFDExportInfo();
    struct Init;
    explicit(false) inline SharedFenceSyncFDExportInfo(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUSharedFenceSyncFDExportInfo&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStructOut), alignof(int));
    alignas(kFirstMemberAlignment) int handle;
};

// Can be chained in SharedFenceDescriptor
struct SharedFenceVkSemaphoreOpaqueFDDescriptor : ChainedStruct {
    inline SharedFenceVkSemaphoreOpaqueFDDescriptor();
    struct Init;
    explicit(false) inline SharedFenceVkSemaphoreOpaqueFDDescriptor(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUSharedFenceVkSemaphoreOpaqueFDDescriptor&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStruct), alignof(int));
    alignas(kFirstMemberAlignment) int handle;
};

// Can be chained in SharedFenceExportInfo
struct SharedFenceVkSemaphoreOpaqueFDExportInfo : ChainedStructOut {
    inline SharedFenceVkSemaphoreOpaqueFDExportInfo();
    struct Init;
    explicit(false) inline SharedFenceVkSemaphoreOpaqueFDExportInfo(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUSharedFenceVkSemaphoreOpaqueFDExportInfo&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStructOut), alignof(int));
    alignas(kFirstMemberAlignment) int handle;
};

// Can be chained in SharedFenceDescriptor
struct SharedFenceVkSemaphoreZirconHandleDescriptor : ChainedStruct {
    inline SharedFenceVkSemaphoreZirconHandleDescriptor();
    struct Init;
    explicit(false) inline SharedFenceVkSemaphoreZirconHandleDescriptor(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUSharedFenceVkSemaphoreZirconHandleDescriptor&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStruct), alignof(uint32_t));
    alignas(kFirstMemberAlignment) uint32_t handle;
};

// Can be chained in SharedFenceExportInfo
struct SharedFenceVkSemaphoreZirconHandleExportInfo : ChainedStructOut {
    inline SharedFenceVkSemaphoreZirconHandleExportInfo();
    struct Init;
    explicit(false) inline SharedFenceVkSemaphoreZirconHandleExportInfo(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUSharedFenceVkSemaphoreZirconHandleExportInfo&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStructOut), alignof(uint32_t));
    alignas(kFirstMemberAlignment) uint32_t handle;
};

// Can be chained in SharedTextureMemoryDescriptor
struct SharedTextureMemoryAHardwareBufferDescriptor : ChainedStruct {
    inline SharedTextureMemoryAHardwareBufferDescriptor();
    struct Init;
    explicit(false) inline SharedTextureMemoryAHardwareBufferDescriptor(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUSharedTextureMemoryAHardwareBufferDescriptor&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStruct), alignof(void *));
    alignas(kFirstMemberAlignment) void * handle;
};

// Can be chained in SharedTextureMemoryBeginAccessDescriptor
struct SharedTextureMemoryD3D11BeginState : ChainedStruct {
    inline SharedTextureMemoryD3D11BeginState();
    struct Init;
    explicit(false) inline SharedTextureMemoryD3D11BeginState(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUSharedTextureMemoryD3D11BeginState&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStruct), alignof(Bool));
    alignas(kFirstMemberAlignment) Bool requiresEndAccessFence = true;
};

// Can be chained in SharedTextureMemoryBeginAccessDescriptor
struct SharedTextureMemoryD3DSwapchainBeginState : ChainedStruct {
    inline SharedTextureMemoryD3DSwapchainBeginState();
    struct Init;
    explicit(false) inline SharedTextureMemoryD3DSwapchainBeginState(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUSharedTextureMemoryD3DSwapchainBeginState&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStruct), alignof(Bool));
    alignas(kFirstMemberAlignment) Bool isSwapchain = false;
};

struct SharedTextureMemoryDmaBufPlane {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUSharedTextureMemoryDmaBufPlane&() const noexcept;

    int fd;
    uint64_t offset;
    uint32_t stride;
};

// Can be chained in SharedTextureMemoryDescriptor
struct SharedTextureMemoryDXGISharedHandleDescriptor : ChainedStruct {
    inline SharedTextureMemoryDXGISharedHandleDescriptor();
    struct Init;
    explicit(false) inline SharedTextureMemoryDXGISharedHandleDescriptor(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUSharedTextureMemoryDXGISharedHandleDescriptor&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStruct), alignof(void *));
    alignas(kFirstMemberAlignment) void * handle;
    Bool useKeyedMutex;
};

// Can be chained in SharedTextureMemoryDescriptor
struct SharedTextureMemoryEGLImageDescriptor : ChainedStruct {
    inline SharedTextureMemoryEGLImageDescriptor();
    struct Init;
    explicit(false) inline SharedTextureMemoryEGLImageDescriptor(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUSharedTextureMemoryEGLImageDescriptor&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStruct), alignof(void *));
    alignas(kFirstMemberAlignment) void * image;
};

// Can be chained in SharedTextureMemoryDescriptor
struct SharedTextureMemoryIOSurfaceDescriptor : ChainedStruct {
    inline SharedTextureMemoryIOSurfaceDescriptor();
    struct Init;
    explicit(false) inline SharedTextureMemoryIOSurfaceDescriptor(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUSharedTextureMemoryIOSurfaceDescriptor&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStruct), alignof(void *));
    alignas(kFirstMemberAlignment) void * ioSurface;
    Bool allowStorageBinding = true;
};

// Can be chained in SharedTextureMemoryDescriptor
struct SharedTextureMemoryOpaqueFDDescriptor : ChainedStruct {
    inline SharedTextureMemoryOpaqueFDDescriptor();
    struct Init;
    explicit(false) inline SharedTextureMemoryOpaqueFDDescriptor(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUSharedTextureMemoryOpaqueFDDescriptor&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStruct), alignof(void const *));
    alignas(kFirstMemberAlignment) void const * vkImageCreateInfo;
    int memoryFD;
    uint32_t memoryTypeIndex;
    uint64_t allocationSize;
    Bool dedicatedAllocation;
};

// Can be chained in SharedTextureMemoryDescriptor
struct SharedTextureMemoryVkDedicatedAllocationDescriptor : ChainedStruct {
    inline SharedTextureMemoryVkDedicatedAllocationDescriptor();
    struct Init;
    explicit(false) inline SharedTextureMemoryVkDedicatedAllocationDescriptor(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUSharedTextureMemoryVkDedicatedAllocationDescriptor&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStruct), alignof(Bool));
    alignas(kFirstMemberAlignment) Bool dedicatedAllocation;
};

// Can be chained in SharedTextureMemoryBeginAccessDescriptor
struct SharedTextureMemoryVkImageLayoutBeginState : ChainedStruct {
    inline SharedTextureMemoryVkImageLayoutBeginState();
    struct Init;
    explicit(false) inline SharedTextureMemoryVkImageLayoutBeginState(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUSharedTextureMemoryVkImageLayoutBeginState&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStruct), alignof(int32_t));
    alignas(kFirstMemberAlignment) int32_t oldLayout;
    int32_t newLayout;
};

// Can be chained in SharedTextureMemoryEndAccessState
struct SharedTextureMemoryVkImageLayoutEndState : ChainedStructOut {
    inline SharedTextureMemoryVkImageLayoutEndState();
    struct Init;
    explicit(false) inline SharedTextureMemoryVkImageLayoutEndState(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUSharedTextureMemoryVkImageLayoutEndState&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStructOut), alignof(int32_t));
    alignas(kFirstMemberAlignment) int32_t oldLayout;
    int32_t newLayout;
};

// Can be chained in SharedTextureMemoryDescriptor
struct SharedTextureMemoryZirconHandleDescriptor : ChainedStruct {
    inline SharedTextureMemoryZirconHandleDescriptor();
    struct Init;
    explicit(false) inline SharedTextureMemoryZirconHandleDescriptor(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUSharedTextureMemoryZirconHandleDescriptor&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStruct), alignof(uint32_t));
    alignas(kFirstMemberAlignment) uint32_t memoryFD;
    uint64_t allocationSize;
};

// Can be chained in BindGroupLayoutEntry
struct StaticSamplerBindingLayout : ChainedStruct {
    inline StaticSamplerBindingLayout();
    struct Init;
    explicit(false) inline StaticSamplerBindingLayout(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUStaticSamplerBindingLayout&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStruct), alignof(Sampler));
    alignas(kFirstMemberAlignment) Sampler sampler = nullptr;
    uint32_t sampledTextureBinding = kLimitU32Undefined;
};

struct StencilFaceState {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUStencilFaceState&() const noexcept;

    CompareFunction compare = CompareFunction::Undefined;
    StencilOperation failOp = StencilOperation::Undefined;
    StencilOperation depthFailOp = StencilOperation::Undefined;
    StencilOperation passOp = StencilOperation::Undefined;
};

struct StorageTextureBindingLayout {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUStorageTextureBindingLayout&() const noexcept;

    ChainedStruct const * nextInChain = nullptr;
    StorageTextureAccess access = StorageTextureAccess::Undefined;
    TextureFormat format = TextureFormat::Undefined;
    TextureViewDimension viewDimension = TextureViewDimension::Undefined;
};

struct SubgroupMatrixConfig {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUSubgroupMatrixConfig&() const noexcept;

    SubgroupMatrixComponentType componentType = {};
    SubgroupMatrixComponentType resultComponentType = {};
    uint32_t M;
    uint32_t N;
    uint32_t K;
};

struct SupportedFeatures {
    inline SupportedFeatures();
    inline ~SupportedFeatures();
    SupportedFeatures(const SupportedFeatures&) = delete;
    SupportedFeatures& operator=(const SupportedFeatures&) = delete;
    inline SupportedFeatures(SupportedFeatures&&);
    inline SupportedFeatures& operator=(SupportedFeatures&&);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUSupportedFeatures&() const noexcept;

    size_t const featureCount = {};
    FeatureName const * const features = nullptr;

  private:
    inline void FreeMembers();
    static inline void Reset(SupportedFeatures& value);
};

struct SupportedInstanceFeatures {
    inline SupportedInstanceFeatures();
    inline ~SupportedInstanceFeatures();
    SupportedInstanceFeatures(const SupportedInstanceFeatures&) = delete;
    SupportedInstanceFeatures& operator=(const SupportedInstanceFeatures&) = delete;
    inline SupportedInstanceFeatures(SupportedInstanceFeatures&&);
    inline SupportedInstanceFeatures& operator=(SupportedInstanceFeatures&&);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUSupportedInstanceFeatures&() const noexcept;

    size_t const featureCount = {};
    InstanceFeatureName const * const features = nullptr;

  private:
    inline void FreeMembers();
    static inline void Reset(SupportedInstanceFeatures& value);
};

struct SupportedWGSLLanguageFeatures {
    inline SupportedWGSLLanguageFeatures();
    inline ~SupportedWGSLLanguageFeatures();
    SupportedWGSLLanguageFeatures(const SupportedWGSLLanguageFeatures&) = delete;
    SupportedWGSLLanguageFeatures& operator=(const SupportedWGSLLanguageFeatures&) = delete;
    inline SupportedWGSLLanguageFeatures(SupportedWGSLLanguageFeatures&&);
    inline SupportedWGSLLanguageFeatures& operator=(SupportedWGSLLanguageFeatures&&);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUSupportedWGSLLanguageFeatures&() const noexcept;

    size_t const featureCount = {};
    WGSLLanguageFeatureName const * const features = nullptr;

  private:
    inline void FreeMembers();
    static inline void Reset(SupportedWGSLLanguageFeatures& value);
};

struct SurfaceCapabilities {
    inline SurfaceCapabilities();
    inline ~SurfaceCapabilities();
    SurfaceCapabilities(const SurfaceCapabilities&) = delete;
    SurfaceCapabilities& operator=(const SurfaceCapabilities&) = delete;
    inline SurfaceCapabilities(SurfaceCapabilities&&);
    inline SurfaceCapabilities& operator=(SurfaceCapabilities&&);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUSurfaceCapabilities&() const noexcept;

    ChainedStructOut  * nextInChain = nullptr;
    TextureUsage const usages = TextureUsage::None;
    size_t const formatCount = {};
    TextureFormat const * const formats = nullptr;
    size_t const presentModeCount = {};
    PresentMode const * const presentModes = nullptr;
    size_t const alphaModeCount = {};
    CompositeAlphaMode const * const alphaModes = nullptr;

  private:
    inline void FreeMembers();
    static inline void Reset(SurfaceCapabilities& value);
};

// Can be chained in SurfaceDescriptor
struct SurfaceColorManagement : ChainedStruct {
    inline SurfaceColorManagement();
    struct Init;
    explicit(false) inline SurfaceColorManagement(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUSurfaceColorManagement&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStruct), alignof(PredefinedColorSpace));
    alignas(kFirstMemberAlignment) PredefinedColorSpace colorSpace = {};
    ToneMappingMode toneMappingMode = {};
};

struct SurfaceConfiguration {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUSurfaceConfiguration&() const noexcept;

    ChainedStruct const * nextInChain = nullptr;
    Device device = nullptr;
    TextureFormat format = TextureFormat::Undefined;
    TextureUsage usage = TextureUsage::RenderAttachment;
    uint32_t width;
    uint32_t height;
    size_t viewFormatCount = 0;
    TextureFormat const * viewFormats = nullptr;
    CompositeAlphaMode alphaMode = CompositeAlphaMode::Auto;
    PresentMode presentMode = PresentMode::Undefined;
};

// Can be chained in SurfaceDescriptor
struct SurfaceDescriptorFromWindowsCoreWindow : ChainedStruct {
    inline SurfaceDescriptorFromWindowsCoreWindow();
    struct Init;
    explicit(false) inline SurfaceDescriptorFromWindowsCoreWindow(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUSurfaceDescriptorFromWindowsCoreWindow&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStruct), alignof(void *));
    alignas(kFirstMemberAlignment) void * coreWindow = nullptr;
};

// Can be chained in SurfaceDescriptor
struct SurfaceDescriptorFromWindowsUWPSwapChainPanel : ChainedStruct {
    inline SurfaceDescriptorFromWindowsUWPSwapChainPanel();
    struct Init;
    explicit(false) inline SurfaceDescriptorFromWindowsUWPSwapChainPanel(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUSurfaceDescriptorFromWindowsUWPSwapChainPanel&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStruct), alignof(void *));
    alignas(kFirstMemberAlignment) void * swapChainPanel = nullptr;
};

// Can be chained in SurfaceDescriptor
struct SurfaceDescriptorFromWindowsWinUISwapChainPanel : ChainedStruct {
    inline SurfaceDescriptorFromWindowsWinUISwapChainPanel();
    struct Init;
    explicit(false) inline SurfaceDescriptorFromWindowsWinUISwapChainPanel(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUSurfaceDescriptorFromWindowsWinUISwapChainPanel&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStruct), alignof(void *));
    alignas(kFirstMemberAlignment) void * swapChainPanel = nullptr;
};

// Can be chained in SurfaceDescriptor
struct SurfaceSourceAndroidNativeWindow : ChainedStruct {
    inline SurfaceSourceAndroidNativeWindow();
    struct Init;
    explicit(false) inline SurfaceSourceAndroidNativeWindow(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUSurfaceSourceAndroidNativeWindow&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStruct), alignof(void *));
    alignas(kFirstMemberAlignment) void * window;
};

// Can be chained in SurfaceDescriptor
struct SurfaceSourceMetalLayer : ChainedStruct {
    inline SurfaceSourceMetalLayer();
    struct Init;
    explicit(false) inline SurfaceSourceMetalLayer(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUSurfaceSourceMetalLayer&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStruct), alignof(void *));
    alignas(kFirstMemberAlignment) void * layer = nullptr;
};

// Can be chained in SurfaceDescriptor
struct SurfaceSourceWaylandSurface : ChainedStruct {
    inline SurfaceSourceWaylandSurface();
    struct Init;
    explicit(false) inline SurfaceSourceWaylandSurface(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUSurfaceSourceWaylandSurface&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStruct), alignof(void *));
    alignas(kFirstMemberAlignment) void * display = nullptr;
    void * surface = nullptr;
};

// Can be chained in SurfaceDescriptor
struct SurfaceSourceWindowsHWND : ChainedStruct {
    inline SurfaceSourceWindowsHWND();
    struct Init;
    explicit(false) inline SurfaceSourceWindowsHWND(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUSurfaceSourceWindowsHWND&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStruct), alignof(void *));
    alignas(kFirstMemberAlignment) void * hinstance = nullptr;
    void * hwnd = nullptr;
};

// Can be chained in SurfaceDescriptor
struct SurfaceSourceXCBWindow : ChainedStruct {
    inline SurfaceSourceXCBWindow();
    struct Init;
    explicit(false) inline SurfaceSourceXCBWindow(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUSurfaceSourceXCBWindow&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStruct), alignof(void *));
    alignas(kFirstMemberAlignment) void * connection = nullptr;
    uint32_t window;
};

// Can be chained in SurfaceDescriptor
struct SurfaceSourceXlibWindow : ChainedStruct {
    inline SurfaceSourceXlibWindow();
    struct Init;
    explicit(false) inline SurfaceSourceXlibWindow(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUSurfaceSourceXlibWindow&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStruct), alignof(void *));
    alignas(kFirstMemberAlignment) void * display = nullptr;
    uint64_t window;
};

struct SurfaceTexture {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUSurfaceTexture&() const noexcept;

    ChainedStructOut  * nextInChain = nullptr;
    Texture texture = nullptr;
    SurfaceGetCurrentTextureStatus status = {};
};

// Can be chained in BindGroupEntry
struct TexelBufferBindingEntry : ChainedStruct {
    inline TexelBufferBindingEntry();
    struct Init;
    explicit(false) inline TexelBufferBindingEntry(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUTexelBufferBindingEntry&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStruct), alignof(TexelBufferView));
    alignas(kFirstMemberAlignment) TexelBufferView texelBufferView = nullptr;
};

// Can be chained in BindGroupLayoutEntry
struct TexelBufferBindingLayout : ChainedStruct {
    inline TexelBufferBindingLayout();
    struct Init;
    explicit(false) inline TexelBufferBindingLayout(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUTexelBufferBindingLayout&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStruct), alignof(TexelBufferAccess));
    alignas(kFirstMemberAlignment) TexelBufferAccess access = TexelBufferAccess::Undefined;
    TextureFormat format = TextureFormat::Undefined;
};

struct TexelBufferViewDescriptor {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUTexelBufferViewDescriptor&() const noexcept;

    ChainedStruct const * nextInChain = nullptr;
    StringView label = {};
    TextureFormat format = TextureFormat::Undefined;
    uint64_t offset = 0;
    uint64_t size = kWholeSize;
};

struct TexelCopyBufferLayout {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUTexelCopyBufferLayout&() const noexcept;

    uint64_t offset = 0;
    uint32_t bytesPerRow = kCopyStrideUndefined;
    uint32_t rowsPerImage = kCopyStrideUndefined;
};

struct TextureBindingLayout {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUTextureBindingLayout&() const noexcept;

    ChainedStruct const * nextInChain = nullptr;
    TextureSampleType sampleType = TextureSampleType::Undefined;
    TextureViewDimension viewDimension = TextureViewDimension::Undefined;
    Bool multisampled = false;
};

// Can be chained in TextureDescriptor
struct TextureBindingViewDimension : ChainedStruct {
    inline TextureBindingViewDimension();
    struct Init;
    explicit(false) inline TextureBindingViewDimension(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUTextureBindingViewDimension&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStruct), alignof(TextureViewDimension));
    alignas(kFirstMemberAlignment) TextureViewDimension textureBindingViewDimension = TextureViewDimension::Undefined;
};

struct TextureComponentSwizzle {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUTextureComponentSwizzle&() const noexcept;

    ComponentSwizzle r = ComponentSwizzle::Undefined;
    ComponentSwizzle g = ComponentSwizzle::Undefined;
    ComponentSwizzle b = ComponentSwizzle::Undefined;
    ComponentSwizzle a = ComponentSwizzle::Undefined;
};

struct VertexAttribute {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUVertexAttribute&() const noexcept;

    ChainedStruct const * nextInChain = nullptr;
    VertexFormat format = {};
    uint64_t offset;
    uint32_t shaderLocation;
};

// Can be chained in SamplerDescriptor
// Can be chained in TextureViewDescriptor
struct YCbCrVkDescriptor : ChainedStruct {
    inline YCbCrVkDescriptor();
    struct Init;
    explicit(false) inline YCbCrVkDescriptor(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUYCbCrVkDescriptor&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStruct), alignof(uint32_t));
    alignas(kFirstMemberAlignment) uint32_t vkFormat = 0;
    uint32_t vkYCbCrModel = 0;
    uint32_t vkYCbCrRange = 0;
    uint32_t vkComponentSwizzleRed = 0;
    uint32_t vkComponentSwizzleGreen = 0;
    uint32_t vkComponentSwizzleBlue = 0;
    uint32_t vkComponentSwizzleAlpha = 0;
    uint32_t vkXChromaOffset = 0;
    uint32_t vkYChromaOffset = 0;
    FilterMode vkChromaFilter = FilterMode::Undefined;
    Bool forceExplicitReconstruction = false;
    uint64_t externalFormat = 0;
};

// Can be chained in AdapterInfo
struct AdapterPropertiesMemoryHeaps : ChainedStructOut {
    inline AdapterPropertiesMemoryHeaps();
    struct Init;
    explicit(false) inline AdapterPropertiesMemoryHeaps(Init&& init);
    inline ~AdapterPropertiesMemoryHeaps();
    AdapterPropertiesMemoryHeaps(const AdapterPropertiesMemoryHeaps&) = delete;
    AdapterPropertiesMemoryHeaps& operator=(const AdapterPropertiesMemoryHeaps&) = delete;
    inline AdapterPropertiesMemoryHeaps(AdapterPropertiesMemoryHeaps&&);
    inline AdapterPropertiesMemoryHeaps& operator=(AdapterPropertiesMemoryHeaps&&);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUAdapterPropertiesMemoryHeaps&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStructOut), alignof(size_t));
    alignas(kFirstMemberAlignment) size_t const heapCount = {};
    MemoryHeapInfo const * const heapInfo = nullptr;

  private:
    inline void FreeMembers();
    static inline void Reset(AdapterPropertiesMemoryHeaps& value);
};

// Can be chained in AdapterInfo
struct AdapterPropertiesSubgroupMatrixConfigs : ChainedStructOut {
    inline AdapterPropertiesSubgroupMatrixConfigs();
    struct Init;
    explicit(false) inline AdapterPropertiesSubgroupMatrixConfigs(Init&& init);
    inline ~AdapterPropertiesSubgroupMatrixConfigs();
    AdapterPropertiesSubgroupMatrixConfigs(const AdapterPropertiesSubgroupMatrixConfigs&) = delete;
    AdapterPropertiesSubgroupMatrixConfigs& operator=(const AdapterPropertiesSubgroupMatrixConfigs&) = delete;
    inline AdapterPropertiesSubgroupMatrixConfigs(AdapterPropertiesSubgroupMatrixConfigs&&);
    inline AdapterPropertiesSubgroupMatrixConfigs& operator=(AdapterPropertiesSubgroupMatrixConfigs&&);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUAdapterPropertiesSubgroupMatrixConfigs&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStructOut), alignof(size_t));
    alignas(kFirstMemberAlignment) size_t const configCount = {};
    SubgroupMatrixConfig const * const configs = nullptr;

  private:
    inline void FreeMembers();
    static inline void Reset(AdapterPropertiesSubgroupMatrixConfigs& value);
};

struct AHardwareBufferProperties {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUAHardwareBufferProperties&() const noexcept;

    YCbCrVkDescriptor yCbCrInfo = {};
};

struct BindGroupEntry {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUBindGroupEntry&() const noexcept;

    ChainedStruct const * nextInChain = nullptr;
    uint32_t binding;
    Buffer buffer = nullptr;
    uint64_t offset = 0;
    uint64_t size = kWholeSize;
    Sampler sampler = nullptr;
    TextureView textureView = nullptr;
};

struct BindGroupLayoutEntry {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUBindGroupLayoutEntry&() const noexcept;

    ChainedStruct const * nextInChain = nullptr;
    uint32_t binding;
    ShaderStage visibility = ShaderStage::None;
    uint32_t bindingArraySize = 0;
    BufferBindingLayout buffer = { nullptr, BufferBindingType::BindingNotUsed, false, 0 };
    SamplerBindingLayout sampler = { nullptr, SamplerBindingType::BindingNotUsed };
    TextureBindingLayout texture = { nullptr, TextureSampleType::BindingNotUsed, TextureViewDimension::e2D, false };
    StorageTextureBindingLayout storageTexture = { nullptr, StorageTextureAccess::BindingNotUsed, TextureFormat::Undefined, TextureViewDimension::e2D };
};

struct BlendState {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUBlendState&() const noexcept;

    BlendComponent color = {};
    BlendComponent alpha = {};
};

struct BufferDescriptor {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUBufferDescriptor&() const noexcept;

    ChainedStruct const * nextInChain = nullptr;
    StringView label = {};
    BufferUsage usage = BufferUsage::None;
    uint64_t size;
    Bool mappedAtCreation = false;
};

struct CommandEncoderDescriptor {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUCommandEncoderDescriptor&() const noexcept;

    ChainedStruct const * nextInChain = nullptr;
    StringView label = {};
};

struct CompilationMessage {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUCompilationMessage&() const noexcept;

    ChainedStruct const * nextInChain = nullptr;
    StringView message = {};
    CompilationMessageType type = {};
    uint64_t lineNum;
    uint64_t linePos;
    uint64_t offset;
    uint64_t length;
};

struct ComputePassDescriptor {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUComputePassDescriptor&() const noexcept;

    ChainedStruct const * nextInChain = nullptr;
    StringView label = {};
    PassTimestampWrites const * timestampWrites = nullptr;
};

struct ComputeState {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUComputeState&() const noexcept;

    ChainedStruct const * nextInChain = nullptr;
    ShaderModule module = nullptr;
    StringView entryPoint = {};
    size_t constantCount = 0;
    ConstantEntry const * constants = nullptr;
};

// Can be chained in DawnFormatCapabilities
struct DawnDrmFormatCapabilities : ChainedStructOut {
    inline DawnDrmFormatCapabilities();
    struct Init;
    explicit(false) inline DawnDrmFormatCapabilities(Init&& init);
    inline ~DawnDrmFormatCapabilities();
    DawnDrmFormatCapabilities(const DawnDrmFormatCapabilities&) = delete;
    DawnDrmFormatCapabilities& operator=(const DawnDrmFormatCapabilities&) = delete;
    inline DawnDrmFormatCapabilities(DawnDrmFormatCapabilities&&);
    inline DawnDrmFormatCapabilities& operator=(DawnDrmFormatCapabilities&&);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUDawnDrmFormatCapabilities&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStructOut), alignof(size_t));
    alignas(kFirstMemberAlignment) size_t const propertiesCount = {};
    DawnDrmFormatProperties const * const properties = nullptr;

  private:
    inline void FreeMembers();
    static inline void Reset(DawnDrmFormatCapabilities& value);
};

struct DepthStencilState {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUDepthStencilState&() const noexcept;

    ChainedStruct const * nextInChain = nullptr;
    TextureFormat format = TextureFormat::Undefined;
    OptionalBool depthWriteEnabled = OptionalBool::Undefined;
    CompareFunction depthCompare = CompareFunction::Undefined;
    StencilFaceState stencilFront = {};
    StencilFaceState stencilBack = {};
    uint32_t stencilReadMask = 0xFFFFFFFF;
    uint32_t stencilWriteMask = 0xFFFFFFFF;
    int32_t depthBias = 0;
    float depthBiasSlopeScale = 0.f;
    float depthBiasClamp = 0.f;
};

struct ExternalTextureDescriptor {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUExternalTextureDescriptor&() const noexcept;

    ChainedStruct const * nextInChain = nullptr;
    StringView label = {};
    TextureView plane0 = nullptr;
    TextureView plane1 = nullptr;
    Origin2D cropOrigin = {};
    Extent2D cropSize = {};
    Extent2D apparentSize = {};
    Bool doYuvToRgbConversionOnly = false;
    float const * yuvToRgbConversionMatrix = nullptr;
    float const * srcTransferFunctionParameters = nullptr;
    float const * dstTransferFunctionParameters = nullptr;
    float const * gamutConversionMatrix = nullptr;
    Bool mirrored = false;
    ExternalTextureRotation rotation = ExternalTextureRotation::Rotate0Degrees;
};

struct FutureWaitInfo {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUFutureWaitInfo&() const noexcept;

    Future future = {};
    Bool completed = false;
};

struct ImageCopyExternalTexture {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUImageCopyExternalTexture&() const noexcept;

    ChainedStruct const * nextInChain = nullptr;
    ExternalTexture externalTexture = nullptr;
    Origin3D origin = {};
    Extent2D naturalSize = {};
};

struct InstanceDescriptor {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUInstanceDescriptor&() const noexcept;

    ChainedStruct const * nextInChain = nullptr;
    size_t requiredFeatureCount = 0;
    InstanceFeatureName const * requiredFeatures = nullptr;
    InstanceLimits const * requiredLimits = nullptr;
};

struct Limits {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPULimits&() const noexcept;

    ChainedStructOut  * nextInChain = nullptr;
    uint32_t maxTextureDimension1D = kLimitU32Undefined;
    uint32_t maxTextureDimension2D = kLimitU32Undefined;
    uint32_t maxTextureDimension3D = kLimitU32Undefined;
    uint32_t maxTextureArrayLayers = kLimitU32Undefined;
    uint32_t maxBindGroups = kLimitU32Undefined;
    uint32_t maxBindGroupsPlusVertexBuffers = kLimitU32Undefined;
    uint32_t maxBindingsPerBindGroup = kLimitU32Undefined;
    uint32_t maxDynamicUniformBuffersPerPipelineLayout = kLimitU32Undefined;
    uint32_t maxDynamicStorageBuffersPerPipelineLayout = kLimitU32Undefined;
    uint32_t maxSampledTexturesPerShaderStage = kLimitU32Undefined;
    uint32_t maxSamplersPerShaderStage = kLimitU32Undefined;
    uint32_t maxStorageBuffersPerShaderStage = kLimitU32Undefined;
    uint32_t maxStorageTexturesPerShaderStage = kLimitU32Undefined;
    uint32_t maxUniformBuffersPerShaderStage = kLimitU32Undefined;
    uint64_t maxUniformBufferBindingSize = kLimitU64Undefined;
    uint64_t maxStorageBufferBindingSize = kLimitU64Undefined;
    uint32_t minUniformBufferOffsetAlignment = kLimitU32Undefined;
    uint32_t minStorageBufferOffsetAlignment = kLimitU32Undefined;
    uint32_t maxVertexBuffers = kLimitU32Undefined;
    uint64_t maxBufferSize = kLimitU64Undefined;
    uint32_t maxVertexAttributes = kLimitU32Undefined;
    uint32_t maxVertexBufferArrayStride = kLimitU32Undefined;
    uint32_t maxInterStageShaderVariables = kLimitU32Undefined;
    uint32_t maxColorAttachments = kLimitU32Undefined;
    uint32_t maxColorAttachmentBytesPerSample = kLimitU32Undefined;
    uint32_t maxComputeWorkgroupStorageSize = kLimitU32Undefined;
    uint32_t maxComputeInvocationsPerWorkgroup = kLimitU32Undefined;
    uint32_t maxComputeWorkgroupSizeX = kLimitU32Undefined;
    uint32_t maxComputeWorkgroupSizeY = kLimitU32Undefined;
    uint32_t maxComputeWorkgroupSizeZ = kLimitU32Undefined;
    uint32_t maxComputeWorkgroupsPerDimension = kLimitU32Undefined;
    uint32_t maxImmediateSize = kLimitU32Undefined;
};

// Can be chained in PipelineLayoutDescriptor
struct PipelineLayoutPixelLocalStorage : ChainedStruct {
    inline PipelineLayoutPixelLocalStorage();
    struct Init;
    explicit(false) inline PipelineLayoutPixelLocalStorage(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUPipelineLayoutPixelLocalStorage&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStruct), alignof(uint64_t));
    alignas(kFirstMemberAlignment) uint64_t totalPixelLocalStorageSize;
    size_t storageAttachmentCount = 0;
    PipelineLayoutStorageAttachment const * storageAttachments = nullptr;
};

struct RenderPassColorAttachment {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPURenderPassColorAttachment&() const noexcept;

    ChainedStruct const * nextInChain = nullptr;
    TextureView view = nullptr;
    uint32_t depthSlice = kDepthSliceUndefined;
    TextureView resolveTarget = nullptr;
    LoadOp loadOp = LoadOp::Undefined;
    StoreOp storeOp = StoreOp::Undefined;
    Color clearValue = {};
};

// Can be chained in RenderPassDescriptor
struct RenderPassRenderAreaRect : ChainedStruct {
    inline RenderPassRenderAreaRect();
    struct Init;
    explicit(false) inline RenderPassRenderAreaRect(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPURenderPassRenderAreaRect&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStruct), alignof(Origin2D));
    alignas(kFirstMemberAlignment) Origin2D origin = {};
    Extent2D size = {};
};

struct RenderPassStorageAttachment {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPURenderPassStorageAttachment&() const noexcept;

    ChainedStruct const * nextInChain = nullptr;
    uint64_t offset = 0;
    TextureView storage = nullptr;
    LoadOp loadOp = LoadOp::Undefined;
    StoreOp storeOp = StoreOp::Undefined;
    Color clearValue = {};
};

struct RequestAdapterOptions {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPURequestAdapterOptions&() const noexcept;

    ChainedStruct const * nextInChain = nullptr;
    FeatureLevel featureLevel = FeatureLevel::Undefined;
    PowerPreference powerPreference = PowerPreference::Undefined;
    Bool forceFallbackAdapter = false;
    BackendType backendType = BackendType::Undefined;
    Surface compatibleSurface = nullptr;
};

struct SamplerDescriptor {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUSamplerDescriptor&() const noexcept;

    ChainedStruct const * nextInChain = nullptr;
    StringView label = {};
    AddressMode addressModeU = AddressMode::Undefined;
    AddressMode addressModeV = AddressMode::Undefined;
    AddressMode addressModeW = AddressMode::Undefined;
    FilterMode magFilter = FilterMode::Undefined;
    FilterMode minFilter = FilterMode::Undefined;
    MipmapFilterMode mipmapFilter = MipmapFilterMode::Undefined;
    float lodMinClamp = 0.f;
    float lodMaxClamp = 32.f;
    CompareFunction compare = CompareFunction::Undefined;
    uint16_t maxAnisotropy = 1;
};

struct ShaderModuleDescriptor {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUShaderModuleDescriptor&() const noexcept;

    ChainedStruct const * nextInChain = nullptr;
    StringView label = {};
};

struct SharedBufferMemoryDescriptor {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUSharedBufferMemoryDescriptor&() const noexcept;

    ChainedStruct const * nextInChain = nullptr;
    StringView label = {};
};

struct SharedFenceDescriptor {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUSharedFenceDescriptor&() const noexcept;

    ChainedStruct const * nextInChain = nullptr;
    StringView label = {};
};

struct SharedFenceExportInfo {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUSharedFenceExportInfo&() const noexcept;

    ChainedStructOut  * nextInChain = nullptr;
    SharedFenceType type = {};
};

// Can be chained in SharedTextureMemoryProperties
struct SharedTextureMemoryAHardwareBufferProperties : ChainedStructOut {
    inline SharedTextureMemoryAHardwareBufferProperties();
    struct Init;
    explicit(false) inline SharedTextureMemoryAHardwareBufferProperties(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUSharedTextureMemoryAHardwareBufferProperties&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStructOut), alignof(YCbCrVkDescriptor));
    alignas(kFirstMemberAlignment) YCbCrVkDescriptor yCbCrInfo = {};
};

struct SharedTextureMemoryBeginAccessDescriptor {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUSharedTextureMemoryBeginAccessDescriptor&() const noexcept;

    ChainedStruct const * nextInChain = nullptr;
    Bool concurrentRead;
    Bool initialized;
    size_t fenceCount;
    SharedFence const * fences = nullptr;
    size_t signaledValueCount = 0;
    uint64_t const * signaledValues = nullptr;
};

// Can be chained in SharedTextureMemoryDescriptor
struct SharedTextureMemoryDmaBufDescriptor : ChainedStruct {
    inline SharedTextureMemoryDmaBufDescriptor();
    struct Init;
    explicit(false) inline SharedTextureMemoryDmaBufDescriptor(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUSharedTextureMemoryDmaBufDescriptor&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStruct), alignof(Extent3D));
    alignas(kFirstMemberAlignment) Extent3D size = {};
    uint32_t drmFormat;
    uint64_t drmModifier;
    size_t planeCount;
    SharedTextureMemoryDmaBufPlane const * planes = nullptr;
};

// Can be chained in SharedTextureMemoryEndAccessState
struct SharedTextureMemoryMetalEndAccessState : ChainedStructOut {
    inline SharedTextureMemoryMetalEndAccessState();
    struct Init;
    explicit(false) inline SharedTextureMemoryMetalEndAccessState(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUSharedTextureMemoryMetalEndAccessState&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStructOut), alignof(Future));
    alignas(kFirstMemberAlignment) Future commandsScheduledFuture = {};
};

struct SurfaceDescriptor {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUSurfaceDescriptor&() const noexcept;

    ChainedStruct const * nextInChain = nullptr;
    StringView label = {};
};

struct TexelCopyBufferInfo {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUTexelCopyBufferInfo&() const noexcept;

    TexelCopyBufferLayout layout = {};
    Buffer buffer = nullptr;
};

struct TexelCopyTextureInfo {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUTexelCopyTextureInfo&() const noexcept;

    Texture texture = nullptr;
    uint32_t mipLevel = 0;
    Origin3D origin = {};
    TextureAspect aspect = TextureAspect::Undefined;
};

// Can be chained in TextureViewDescriptor
struct TextureComponentSwizzleDescriptor : ChainedStruct {
    inline TextureComponentSwizzleDescriptor();
    struct Init;
    explicit(false) inline TextureComponentSwizzleDescriptor(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUTextureComponentSwizzleDescriptor&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStruct), alignof(TextureComponentSwizzle));
    alignas(kFirstMemberAlignment) TextureComponentSwizzle swizzle = {};
};

struct TextureDescriptor {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUTextureDescriptor&() const noexcept;

    ChainedStruct const * nextInChain = nullptr;
    StringView label = {};
    TextureUsage usage = TextureUsage::None;
    TextureDimension dimension = TextureDimension::Undefined;
    Extent3D size = {};
    TextureFormat format = TextureFormat::Undefined;
    uint32_t mipLevelCount = 1;
    uint32_t sampleCount = 1;
    size_t viewFormatCount = 0;
    TextureFormat const * viewFormats = nullptr;
};

struct VertexBufferLayout {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUVertexBufferLayout&() const noexcept;

    ChainedStruct const * nextInChain = nullptr;
    VertexStepMode stepMode = VertexStepMode::Undefined;
    uint64_t arrayStride;
    size_t attributeCount;
    VertexAttribute const * attributes = nullptr;
};

struct AdapterInfo {
    inline AdapterInfo();
    inline ~AdapterInfo();
    AdapterInfo(const AdapterInfo&) = delete;
    AdapterInfo& operator=(const AdapterInfo&) = delete;
    inline AdapterInfo(AdapterInfo&&);
    inline AdapterInfo& operator=(AdapterInfo&&);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUAdapterInfo&() const noexcept;

    ChainedStructOut  * nextInChain = nullptr;
    StringView const vendor = {};
    StringView const architecture = {};
    StringView const device = {};
    StringView const description = {};
    BackendType const backendType = BackendType::Undefined;
    AdapterType const adapterType = {};
    uint32_t const vendorID = {};
    uint32_t const deviceID = {};
    uint32_t const subgroupMinSize = {};
    uint32_t const subgroupMaxSize = {};

  private:
    inline void FreeMembers();
    static inline void Reset(AdapterInfo& value);
};

struct BindGroupDescriptor {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUBindGroupDescriptor&() const noexcept;

    ChainedStruct const * nextInChain = nullptr;
    StringView label = {};
    BindGroupLayout layout = nullptr;
    size_t entryCount = 0;
    BindGroupEntry const * entries = nullptr;
};

struct BindGroupLayoutDescriptor {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUBindGroupLayoutDescriptor&() const noexcept;

    ChainedStruct const * nextInChain = nullptr;
    StringView label = {};
    size_t entryCount = 0;
    BindGroupLayoutEntry const * entries = nullptr;
};

struct ColorTargetState {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUColorTargetState&() const noexcept;

    ChainedStruct const * nextInChain = nullptr;
    TextureFormat format = TextureFormat::Undefined;
    BlendState const * blend = nullptr;
    ColorWriteMask writeMask = ColorWriteMask::All;
};

struct CompilationInfo {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUCompilationInfo&() const noexcept;

    ChainedStruct const * nextInChain = nullptr;
    size_t messageCount;
    CompilationMessage const * messages = nullptr;
};

struct ComputePipelineDescriptor {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUComputePipelineDescriptor&() const noexcept;

    ChainedStruct const * nextInChain = nullptr;
    StringView label = {};
    PipelineLayout layout = nullptr;
    ComputeState compute = {};
};

struct DawnFormatCapabilities {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUDawnFormatCapabilities&() const noexcept;

    ChainedStructOut  * nextInChain = nullptr;
};

namespace detail {
struct DeviceDescriptor {
    ChainedStruct const * nextInChain = nullptr;
    StringView label = {};
    size_t requiredFeatureCount = 0;
    FeatureName const * requiredFeatures = nullptr;
    Limits const * requiredLimits = nullptr;
    QueueDescriptor defaultQueue = {};
    WGPUDeviceLostCallbackInfo deviceLostCallbackInfo = WGPU_DEVICE_LOST_CALLBACK_INFO_INIT;
    WGPUUncapturedErrorCallbackInfo uncapturedErrorCallbackInfo = WGPU_UNCAPTURED_ERROR_CALLBACK_INFO_INIT;
};
}  // namespace detail
struct DeviceDescriptor : protected detail::DeviceDescriptor {
    inline DeviceDescriptor();
    struct Init;
    explicit(false) inline DeviceDescriptor(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUDeviceDescriptor&() const noexcept;

    using detail::DeviceDescriptor::nextInChain;
    using detail::DeviceDescriptor::label;
    using detail::DeviceDescriptor::requiredFeatureCount;
    using detail::DeviceDescriptor::requiredFeatures;
    using detail::DeviceDescriptor::requiredLimits;
    using detail::DeviceDescriptor::defaultQueue;

    template <typename F, typename T>
    void SetDeviceLostCallback(CallbackMode callbackMode, F callback, T userdata);
    template <typename F>
    void SetDeviceLostCallback(CallbackMode callbackMode, F callback);
    template <typename F, typename T>
    void SetUncapturedErrorCallback(F callback, T userdata);
    template <typename F>
    void SetUncapturedErrorCallback(F callback);
};

struct PipelineLayoutDescriptor {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUPipelineLayoutDescriptor&() const noexcept;

    ChainedStruct const * nextInChain = nullptr;
    StringView label = {};
    size_t bindGroupLayoutCount;
    BindGroupLayout const * bindGroupLayouts = nullptr;
    uint32_t immediateSize = 0;
};

// Can be chained in RenderPassDescriptor
struct RenderPassPixelLocalStorage : ChainedStruct {
    inline RenderPassPixelLocalStorage();
    struct Init;
    explicit(false) inline RenderPassPixelLocalStorage(Init&& init);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPURenderPassPixelLocalStorage&() const noexcept;

    static constexpr size_t kFirstMemberAlignment = detail::ConstexprMax(alignof(ChainedStruct), alignof(uint64_t));
    alignas(kFirstMemberAlignment) uint64_t totalPixelLocalStorageSize;
    size_t storageAttachmentCount = 0;
    RenderPassStorageAttachment const * storageAttachments = nullptr;
};

struct SharedTextureMemoryDescriptor {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUSharedTextureMemoryDescriptor&() const noexcept;

    ChainedStruct const * nextInChain = nullptr;
    StringView label = {};
};

struct SharedTextureMemoryEndAccessState {
    inline SharedTextureMemoryEndAccessState();
    inline ~SharedTextureMemoryEndAccessState();
    SharedTextureMemoryEndAccessState(const SharedTextureMemoryEndAccessState&) = delete;
    SharedTextureMemoryEndAccessState& operator=(const SharedTextureMemoryEndAccessState&) = delete;
    inline SharedTextureMemoryEndAccessState(SharedTextureMemoryEndAccessState&&);
    inline SharedTextureMemoryEndAccessState& operator=(SharedTextureMemoryEndAccessState&&);
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUSharedTextureMemoryEndAccessState&() const noexcept;

    ChainedStructOut  * nextInChain = nullptr;
    Bool const initialized = {};
    size_t const fenceCount = {};
    SharedFence const * const fences = nullptr;
    size_t const signaledValueCount = 0;
    uint64_t const * const signaledValues = nullptr;

  private:
    inline void FreeMembers();
    static inline void Reset(SharedTextureMemoryEndAccessState& value);
};

struct SharedTextureMemoryProperties {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUSharedTextureMemoryProperties&() const noexcept;

    ChainedStructOut  * nextInChain = nullptr;
    TextureUsage usage = TextureUsage::None;
    Extent3D size = {};
    TextureFormat format = TextureFormat::Undefined;
};

struct TextureViewDescriptor {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUTextureViewDescriptor&() const noexcept;

    ChainedStruct const * nextInChain = nullptr;
    StringView label = {};
    TextureFormat format = TextureFormat::Undefined;
    TextureViewDimension dimension = TextureViewDimension::Undefined;
    uint32_t baseMipLevel = 0;
    uint32_t mipLevelCount = kMipLevelCountUndefined;
    uint32_t baseArrayLayer = 0;
    uint32_t arrayLayerCount = kArrayLayerCountUndefined;
    TextureAspect aspect = TextureAspect::Undefined;
    TextureUsage usage = TextureUsage::None;
};

struct VertexState {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUVertexState&() const noexcept;

    ChainedStruct const * nextInChain = nullptr;
    ShaderModule module = nullptr;
    StringView entryPoint = {};
    size_t constantCount = 0;
    ConstantEntry const * constants = nullptr;
    size_t bufferCount = 0;
    VertexBufferLayout const * buffers = nullptr;
};

struct FragmentState {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPUFragmentState&() const noexcept;

    ChainedStruct const * nextInChain = nullptr;
    ShaderModule module = nullptr;
    StringView entryPoint = {};
    size_t constantCount = 0;
    ConstantEntry const * constants = nullptr;
    size_t targetCount;
    ColorTargetState const * targets = nullptr;
};

struct RenderPassDescriptor {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPURenderPassDescriptor&() const noexcept;

    ChainedStruct const * nextInChain = nullptr;
    StringView label = {};
    size_t colorAttachmentCount;
    RenderPassColorAttachment const * colorAttachments = nullptr;
    RenderPassDepthStencilAttachment const * depthStencilAttachment = nullptr;
    QuerySet occlusionQuerySet = nullptr;
    PassTimestampWrites const * timestampWrites = nullptr;
};

struct RenderPipelineDescriptor {
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator const WGPURenderPipelineDescriptor&() const noexcept;

    ChainedStruct const * nextInChain = nullptr;
    StringView label = {};
    PipelineLayout layout = nullptr;
    VertexState vertex = {};
    PrimitiveState primitive = {};
    DepthStencilState const * depthStencil = nullptr;
    MultisampleState multisample = {};
    FragmentState const * fragment = nullptr;
};

// NOLINTEND(bugprone-invalid-enum-default-initialization)

// Callback info handling is generated and/or custom implemented here to convert the types between C and C++.
namespace detail {
struct Untyped {};

// The current interface of CppFTraits exposes the following values:
//   - CppFTraits::capturing: true if the callback is a capturing callback (i.e. a lambda that
//     captures its environment).
//   - CppFTraits::PtrT: The C++ callback function pointer type.
//   - CppFTraits::ReturnT: The return type of the C++ callback function pointer.
//   - CppFTraits::BaseArgsTuple: A tuple of the C++ arguments minus the user specified typed
//     parameter.
//
// Implementation notes:
//   - The |Untyped| struct is only used to differentiate whether |T| is a user specified type
//     in which case we need to strip the type from the callback arguments for speecialization
//     deduction.
template <typename CppFT, typename CppFPtr, typename T>
struct CppFTraitsImpl;
// Specialization for raw function pointers.
template <typename CppFT, typename R, typename... CppArgs, typename T>
struct CppFTraitsImpl<CppFT, R(*)(CppArgs...), T> {
    using PtrT = R(*)(CppArgs...);
    using ReturnT = R;
    static constexpr bool capturing = false;

    static constexpr size_t NumCppArgs = sizeof...(CppArgs);
    using BaseArgsTuple = typename decltype([]<std::size_t... Is>(std::index_sequence<Is...>) {
        return std::type_identity<std::tuple<std::tuple_element_t<Is, std::tuple<CppArgs...>>...>>{};
    }(std::make_index_sequence<std::is_same_v<T, Untyped> ? NumCppArgs : NumCppArgs - 1>{})
    )::type;
};
// Specialization for noexcept raw function pointers.
template <typename CppFT, typename R, typename... CppArgs, typename T>
struct CppFTraitsImpl<CppFT, R(*)(CppArgs...) noexcept, T> {
    using PtrT = R(*)(CppArgs...) noexcept;
    using ReturnT = R;
    static constexpr bool capturing = false;

    static constexpr size_t NumCppArgs = sizeof...(CppArgs);
    using BaseArgsTuple = typename decltype([]<std::size_t... Is>(std::index_sequence<Is...>) {
        return std::type_identity<std::tuple<std::tuple_element_t<Is, std::tuple<CppArgs...>>...>>{};
    }(std::make_index_sequence<std::is_same_v<T, Untyped> ? NumCppArgs : NumCppArgs - 1>{})
    )::type;
};
// Specialization for member function pointers (lambdas);
template <typename CppFT, typename R, typename C, typename... CppArgs, typename T>
struct CppFTraitsImpl<CppFT, R(C::*)(CppArgs...) const, T> {
    using PtrT = R(*)(CppArgs...);
    using ReturnT = R;
    static constexpr bool capturing = !std::is_convertible_v<CppFT, PtrT>;

    static constexpr size_t NumCppArgs = sizeof...(CppArgs);
    using BaseArgsTuple = typename decltype([]<std::size_t... Is>(std::index_sequence<Is...>) {
        return std::type_identity<std::tuple<std::tuple_element_t<Is, std::tuple<CppArgs...>>...>>{};
    }(std::make_index_sequence<std::is_same_v<T, Untyped> ? NumCppArgs : NumCppArgs - 1>{})
    )::type;
};
// Specialization for noexcept member function pointers (noexcept lambdas);
template <typename CppFT, typename R, typename C, typename... CppArgs, typename T>
struct CppFTraitsImpl<CppFT, R(C::*)(CppArgs...) const noexcept, T> {
    using PtrT = R(*)(CppArgs...) noexcept;
    using ReturnT = R;
    static constexpr bool capturing = !std::is_convertible_v<CppFT, PtrT>;

    static constexpr size_t NumCppArgs = sizeof...(CppArgs);
    using BaseArgsTuple = typename decltype([]<std::size_t... Is>(std::index_sequence<Is...>) {
        return std::type_identity<std::tuple<std::tuple_element_t<Is, std::tuple<CppArgs...>>...>>{};
    }(std::make_index_sequence<std::is_same_v<T, Untyped> ? NumCppArgs : NumCppArgs - 1>{})
    )::type;
};
template <typename CppFT, typename T = Untyped>
struct CppFTraits : CppFTraitsImpl<CppFT, decltype(&CppFT::operator()), T> {};
template <typename R, typename... CppArgs, typename T>
struct CppFTraits<R(*)(CppArgs...), T> :
    CppFTraitsImpl<R(*)(CppArgs...), R(*)(CppArgs...), T> {};
template <typename R, typename... CppArgs, typename T>
struct CppFTraits<R(*)(CppArgs...) noexcept, T> :
    CppFTraitsImpl<R(*)(CppArgs...) noexcept, R(*)(CppArgs...) noexcept, T> {};

// CArgConverter are specialization structs that specialize a conversion from a C CallbackInfo's
// callback argument types to a set of valid C++ types. These specializations provide us a way to
// support additional C++ callback types, i.e. converting raw pointers to more C++ friendly
// structures when applicable.
template <typename CInfoT, typename CppArgs>
struct CArgConverter;
template <>
struct CArgConverter<WGPUBufferMapCallbackInfo, std::tuple<MapAsyncStatus, StringView>> {
    using Result = std::tuple<MapAsyncStatus, StringView>;
    static Result Convert(WGPUMapAsyncStatus status, WGPUStringView message) {
        return std::make_tuple(static_cast<MapAsyncStatus>(status), StringView {
    message.data,
    message.length
});
    }
};
template <>
struct CArgConverter<WGPUCompilationInfoCallbackInfo, std::tuple<CompilationInfoRequestStatus, CompilationInfo const *>> {
    using Result = std::tuple<CompilationInfoRequestStatus, CompilationInfo const *>;
    static Result Convert(WGPUCompilationInfoRequestStatus status, WGPUCompilationInfo const * compilationInfo) {
        return std::make_tuple(static_cast<CompilationInfoRequestStatus>(status), reinterpret_cast<CompilationInfo const*>(compilationInfo));
    }
};
template <>
struct CArgConverter<WGPUCreateComputePipelineAsyncCallbackInfo, std::tuple<CreatePipelineAsyncStatus, ComputePipeline, StringView>> {
    using Result = std::tuple<CreatePipelineAsyncStatus, ComputePipeline, StringView>;
    static Result Convert(WGPUCreatePipelineAsyncStatus status, WGPUComputePipeline pipeline, WGPUStringView message) {
        return std::make_tuple(static_cast<CreatePipelineAsyncStatus>(status), ComputePipeline::Acquire(pipeline), StringView {
    message.data,
    message.length
});
    }
};
template <>
struct CArgConverter<WGPUCreateRenderPipelineAsyncCallbackInfo, std::tuple<CreatePipelineAsyncStatus, RenderPipeline, StringView>> {
    using Result = std::tuple<CreatePipelineAsyncStatus, RenderPipeline, StringView>;
    static Result Convert(WGPUCreatePipelineAsyncStatus status, WGPURenderPipeline pipeline, WGPUStringView message) {
        return std::make_tuple(static_cast<CreatePipelineAsyncStatus>(status), RenderPipeline::Acquire(pipeline), StringView {
    message.data,
    message.length
});
    }
};
template <>
struct CArgConverter<WGPULoggingCallbackInfo, std::tuple<LoggingType, StringView>> {
    using Result = std::tuple<LoggingType, StringView>;
    static Result Convert(WGPULoggingType type, WGPUStringView message) {
        return std::make_tuple(static_cast<LoggingType>(type), StringView {
    message.data,
    message.length
});
    }
};
template <>
struct CArgConverter<WGPUPopErrorScopeCallbackInfo, std::tuple<PopErrorScopeStatus, ErrorType, StringView>> {
    using Result = std::tuple<PopErrorScopeStatus, ErrorType, StringView>;
    static Result Convert(WGPUPopErrorScopeStatus status, WGPUErrorType type, WGPUStringView message) {
        return std::make_tuple(static_cast<PopErrorScopeStatus>(status), static_cast<ErrorType>(type), StringView {
    message.data,
    message.length
});
    }
};
template <>
struct CArgConverter<WGPUQueueWorkDoneCallbackInfo, std::tuple<QueueWorkDoneStatus, StringView>> {
    using Result = std::tuple<QueueWorkDoneStatus, StringView>;
    static Result Convert(WGPUQueueWorkDoneStatus status, WGPUStringView message) {
        return std::make_tuple(static_cast<QueueWorkDoneStatus>(status), StringView {
    message.data,
    message.length
});
    }
};
template <>
struct CArgConverter<WGPURequestAdapterCallbackInfo, std::tuple<RequestAdapterStatus, Adapter, StringView>> {
    using Result = std::tuple<RequestAdapterStatus, Adapter, StringView>;
    static Result Convert(WGPURequestAdapterStatus status, WGPUAdapter adapter, WGPUStringView message) {
        return std::make_tuple(static_cast<RequestAdapterStatus>(status), Adapter::Acquire(adapter), StringView {
    message.data,
    message.length
});
    }
};
template <>
struct CArgConverter<WGPURequestDeviceCallbackInfo, std::tuple<RequestDeviceStatus, Device, StringView>> {
    using Result = std::tuple<RequestDeviceStatus, Device, StringView>;
    static Result Convert(WGPURequestDeviceStatus status, WGPUDevice device, WGPUStringView message) {
        return std::make_tuple(static_cast<RequestDeviceStatus>(status), Device::Acquire(device), StringView {
    message.data,
    message.length
});
    }
};
template <>
struct CArgConverter<WGPUDeviceLostCallbackInfo,
                     std::tuple<const Device&, DeviceLostReason, StringView>> {
    using Result = std::tuple<const Device&, DeviceLostReason, StringView>;
    static Result Convert(WGPUDevice const* device,
                          WGPUDeviceLostReason reason,
                          WGPUStringView message) {
        return std::make_tuple(std::cref(*reinterpret_cast<const Device*>(device)),
                               static_cast<DeviceLostReason>(reason), message);
    }
};
template <>
struct CArgConverter<WGPUUncapturedErrorCallbackInfo,
                     std::tuple<const Device&, ErrorType, StringView>> {
    using Result = std::tuple<const Device&, ErrorType, StringView>;
    static Result Convert(WGPUDevice const* device, WGPUErrorType type, WGPUStringView message) {
        return std::make_tuple(std::cref(*reinterpret_cast<const Device*>(device)),
                               static_cast<ErrorType>(type), message);
    }
};
template <>
struct CArgConverter<WGPUDawnLoadCacheDataCallbackInfo,
                     std::tuple<std::span<const std::byte>, std::span<std::byte>>> {
    using Result = std::tuple<std::span<const std::byte>, std::span<std::byte>>;
    static Result Convert(size_t keySize, uint8_t const* key, size_t valueSize, uint8_t* value) {
        return std::make_tuple(
            std::span<const std::byte>(reinterpret_cast<const std::byte*>(key), keySize),
            std::span<std::byte>(reinterpret_cast<std::byte*>(value), valueSize)
        );
    }
};
template <>
struct CArgConverter<WGPUDawnStoreCacheDataCallbackInfo,
                     std::tuple<std::span<const std::byte>, std::span<const std::byte>>> {
    using Result = std::tuple<std::span<const std::byte>, std::span<const std::byte>>;
    static Result Convert(size_t keySize, uint8_t const* key, size_t valueSize, uint8_t const* value) {
        return std::make_tuple(
            std::span<const std::byte>(reinterpret_cast<const std::byte*>(key), keySize),
            std::span<const std::byte>(reinterpret_cast<const std::byte*>(value), valueSize)
        );
    }
};

// The CallbackHelper struct implements the static functions needed to convert the base C callbacks
// into the user provided C++ callbacks. More than anything, it handles converting the real C
// callback arguments (i.e. not the userdata pointers we use internally) to C++ arguments, and
// casts the userdata pointers appropriately to ensure that everything is typed.
template <typename R, typename CInfoT, typename CppF, typename CArgsTuple, typename Indices>
struct CallbackHelperImpl;
template <typename R, typename CInfoT, typename CppF, typename... CArgs, std::size_t... Is>
struct CallbackHelperImpl<R, CInfoT, CppF, std::tuple<CArgs...>, std::index_sequence<Is...>> {
    // Implementation for callbacks without an additional typed argument. We support capturing
    // lambdas if users can specify a callback mode in this case and make an allocation.
    static R Call(std::tuple_element_t<Is, std::tuple<CArgs...>>... cArgs,
                     void* callbackParam, void*) {
        using CppFTraits = CppFTraits<CppF>;
        using Converter = CArgConverter<CInfoT, typename CppFTraits::BaseArgsTuple>;

        if constexpr (CppFTraits::capturing) {
            std::unique_ptr<CppF> callback(reinterpret_cast<CppF*>(callbackParam));
            return std::apply(*callback, Converter::Convert(cArgs...));
        } else {
            auto callback = reinterpret_cast<CppFTraits::PtrT>(callbackParam);
            return std::apply(callback, Converter::Convert(cArgs...));
        }
    }

    // Implementation for callbacks where the user specifies an additional typed argument. We do
    // not support capturing lambdas in this case since the user is already providing a typed
    // argument and would make more sense for any other state to be capturing by that argument
    // instead.
    template <typename T>
    static R Call(std::tuple_element_t<Is, std::tuple<CArgs...>>... cArgs,
                     void* callbackParam, void* userdataParam) {
        using CppFTraits = CppFTraits<CppF, T>;
        using Converter = CArgConverter<CInfoT, typename CppFTraits::BaseArgsTuple>;

        auto callback = reinterpret_cast<CppFTraits::PtrT>(callbackParam);
        auto param = std::make_tuple(static_cast<T>(userdataParam));
        return std::apply(callback, std::tuple_cat(Converter::Convert(cArgs...), param));
    }
};
template <typename CInfoT, typename F, typename CbT>
struct CallbackHelper;
template <typename CInfoT, typename F, typename R, typename... CArgs>
struct CallbackHelper<CInfoT, F, R(*)(CArgs...)> {
    static constexpr size_t NumCArgs = sizeof...(CArgs);
    using CArgsTuple = std::tuple<CArgs...>;
    static_assert(NumCArgs >= 2, "C Function pointers must have two void* trailing arguments.");
    static_assert(
        std::is_same_v<std::tuple_element_t<NumCArgs - 1, CArgsTuple>, void*>,
        "C Function pointer's last argument must be void*."
    );
    static_assert(
        std::is_same_v<std::tuple_element_t<NumCArgs - 2, CArgsTuple>, void*>,
        "C Function pointer's second to last argument must be void*."
    );

    using Impl = CallbackHelperImpl<R, CInfoT, F, CArgsTuple, std::make_index_sequence<NumCArgs - 2>>;
    static R Call(CArgs... args) {
        return Impl::Call(std::forward<CArgs>(args)...);
    }
    template <typename T>
    static R Call(CArgs... args) {
        return Impl::template Call<T>(std::forward<CArgs>(args)...);
    }
};

// The CallbackInfoHelper provides a utility to create a C CallbackInfo struct type from a C++
// callback. This allows us to de-duplicates the logic used for WebGPU structs that have
// CallbackInfo members and WebGPU API functions that return a Future.
template <typename CInfoT, typename F>
struct CallbackInfoHelper {
    static CInfoT Create(F lambda) {
        CInfoT info = {};
        if constexpr (CppFTraits<F>::capturing) {
            if constexpr (requires(CInfoT x) { x.mode; }) {
                // If we have a mode argument, then the callback is guaranteed to be called only
                // once so we can support it by moving it and making an allocation for it.
                std::unique_ptr<F> alloc = std::make_unique<F>(std::move(lambda));
                info.userdata1 = reinterpret_cast<void*>(alloc.release());
            } else {
                // MSVC <=19.39 doesn't support static_assert(false) and this was fixed in later C++
                // standards https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2593r1.html
                constexpr bool kAlwaysFalse = sizeof(F) == 0;
                static_assert(
                    kAlwaysFalse, "capturing lambdas aren't supported for repeatable callbacks"
                );
            }
        } else {
            // Otherwise, if the lambda is not capturing, that means we can refer to it as a
            // pointer directly.
            info.userdata1 = reinterpret_cast<void*>(+lambda);
        }
        info.callback = CallbackHelper<CInfoT, F, decltype(CInfoT::callback)>::Call;
        info.userdata2 = nullptr;
        return info;
    }

    template <typename T>
    requires (!CppFTraits<F>::capturing)
    static CInfoT Create(F lambda, T userdata) {
        CInfoT info = {};
        info.callback = CallbackHelper<CInfoT, F, decltype(CInfoT::callback)>::template Call<T>;
        info.userdata1 = reinterpret_cast<void*>(+lambda);
        info.userdata2 = userdata;
        return info;
    }
};

}  // namespace detail

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
// error: 'offsetof' within non-standard-layout type 'wgpu::XXX' is conditionally-supported
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
#endif
// NOLINTBEGIN(bugprone-invalid-enum-default-initialization)

// AdapterPropertiesD3D implementation
AdapterPropertiesD3D::AdapterPropertiesD3D()
: ChainedStructOut { nullptr, SType::AdapterPropertiesD3D } {}
struct AdapterPropertiesD3D::Init {
    ChainedStructOut *  nextInChain;
    uint32_t shaderModel;
};
AdapterPropertiesD3D::AdapterPropertiesD3D(AdapterPropertiesD3D::Init&& init) :
    ChainedStructOut { init.nextInChain, SType::AdapterPropertiesD3D },
    shaderModel(std::move(init.shaderModel)) {}

AdapterPropertiesD3D::operator const WGPUAdapterPropertiesD3D&() const noexcept {
    return *reinterpret_cast<const WGPUAdapterPropertiesD3D*>(this);
}

static_assert(sizeof(AdapterPropertiesD3D) == sizeof(WGPUAdapterPropertiesD3D), "sizeof mismatch for AdapterPropertiesD3D");
static_assert(alignof(AdapterPropertiesD3D) == alignof(WGPUAdapterPropertiesD3D), "alignof mismatch for AdapterPropertiesD3D");
static_assert(offsetof(AdapterPropertiesD3D, shaderModel) == offsetof(WGPUAdapterPropertiesD3D, shaderModel),
        "offsetof mismatch for AdapterPropertiesD3D::shaderModel");

// AdapterPropertiesDrm implementation
AdapterPropertiesDrm::AdapterPropertiesDrm()
: ChainedStructOut { nullptr, SType::AdapterPropertiesDrm } {}
struct AdapterPropertiesDrm::Init {
    ChainedStructOut *  nextInChain;
    Bool hasPrimary = false;
    Bool hasRender = false;
    uint64_t primaryMajor = 0;
    uint64_t primaryMinor = 0;
    uint64_t renderMajor = 0;
    uint64_t renderMinor = 0;
};
AdapterPropertiesDrm::AdapterPropertiesDrm(AdapterPropertiesDrm::Init&& init) :
    ChainedStructOut { init.nextInChain, SType::AdapterPropertiesDrm },
    hasPrimary(std::move(init.hasPrimary)),
    hasRender(std::move(init.hasRender)),
    primaryMajor(std::move(init.primaryMajor)),
    primaryMinor(std::move(init.primaryMinor)),
    renderMajor(std::move(init.renderMajor)),
    renderMinor(std::move(init.renderMinor)) {}

AdapterPropertiesDrm::operator const WGPUAdapterPropertiesDrm&() const noexcept {
    return *reinterpret_cast<const WGPUAdapterPropertiesDrm*>(this);
}

static_assert(sizeof(AdapterPropertiesDrm) == sizeof(WGPUAdapterPropertiesDrm), "sizeof mismatch for AdapterPropertiesDrm");
static_assert(alignof(AdapterPropertiesDrm) == alignof(WGPUAdapterPropertiesDrm), "alignof mismatch for AdapterPropertiesDrm");
static_assert(offsetof(AdapterPropertiesDrm, hasPrimary) == offsetof(WGPUAdapterPropertiesDrm, hasPrimary),
        "offsetof mismatch for AdapterPropertiesDrm::hasPrimary");
static_assert(offsetof(AdapterPropertiesDrm, hasRender) == offsetof(WGPUAdapterPropertiesDrm, hasRender),
        "offsetof mismatch for AdapterPropertiesDrm::hasRender");
static_assert(offsetof(AdapterPropertiesDrm, primaryMajor) == offsetof(WGPUAdapterPropertiesDrm, primaryMajor),
        "offsetof mismatch for AdapterPropertiesDrm::primaryMajor");
static_assert(offsetof(AdapterPropertiesDrm, primaryMinor) == offsetof(WGPUAdapterPropertiesDrm, primaryMinor),
        "offsetof mismatch for AdapterPropertiesDrm::primaryMinor");
static_assert(offsetof(AdapterPropertiesDrm, renderMajor) == offsetof(WGPUAdapterPropertiesDrm, renderMajor),
        "offsetof mismatch for AdapterPropertiesDrm::renderMajor");
static_assert(offsetof(AdapterPropertiesDrm, renderMinor) == offsetof(WGPUAdapterPropertiesDrm, renderMinor),
        "offsetof mismatch for AdapterPropertiesDrm::renderMinor");

// AdapterPropertiesVk implementation
AdapterPropertiesVk::AdapterPropertiesVk()
: ChainedStructOut { nullptr, SType::AdapterPropertiesVk } {}
struct AdapterPropertiesVk::Init {
    ChainedStructOut *  nextInChain;
    uint32_t driverVersion;
};
AdapterPropertiesVk::AdapterPropertiesVk(AdapterPropertiesVk::Init&& init) :
    ChainedStructOut { init.nextInChain, SType::AdapterPropertiesVk },
    driverVersion(std::move(init.driverVersion)) {}

AdapterPropertiesVk::operator const WGPUAdapterPropertiesVk&() const noexcept {
    return *reinterpret_cast<const WGPUAdapterPropertiesVk*>(this);
}

static_assert(sizeof(AdapterPropertiesVk) == sizeof(WGPUAdapterPropertiesVk), "sizeof mismatch for AdapterPropertiesVk");
static_assert(alignof(AdapterPropertiesVk) == alignof(WGPUAdapterPropertiesVk), "alignof mismatch for AdapterPropertiesVk");
static_assert(offsetof(AdapterPropertiesVk, driverVersion) == offsetof(WGPUAdapterPropertiesVk, driverVersion),
        "offsetof mismatch for AdapterPropertiesVk::driverVersion");

// AdapterPropertiesWGPU implementation
AdapterPropertiesWGPU::AdapterPropertiesWGPU()
: ChainedStructOut { nullptr, SType::AdapterPropertiesWGPU } {}
struct AdapterPropertiesWGPU::Init {
    ChainedStructOut *  nextInChain;
    BackendType backendType = BackendType::Undefined;
};
AdapterPropertiesWGPU::AdapterPropertiesWGPU(AdapterPropertiesWGPU::Init&& init) :
    ChainedStructOut { init.nextInChain, SType::AdapterPropertiesWGPU },
    backendType(std::move(init.backendType)) {}

AdapterPropertiesWGPU::operator const WGPUAdapterPropertiesWGPU&() const noexcept {
    return *reinterpret_cast<const WGPUAdapterPropertiesWGPU*>(this);
}

static_assert(sizeof(AdapterPropertiesWGPU) == sizeof(WGPUAdapterPropertiesWGPU), "sizeof mismatch for AdapterPropertiesWGPU");
static_assert(alignof(AdapterPropertiesWGPU) == alignof(WGPUAdapterPropertiesWGPU), "alignof mismatch for AdapterPropertiesWGPU");
static_assert(offsetof(AdapterPropertiesWGPU, backendType) == offsetof(WGPUAdapterPropertiesWGPU, backendType),
        "offsetof mismatch for AdapterPropertiesWGPU::backendType");

// BindingResource implementation

BindingResource::operator const WGPUBindingResource&() const noexcept {
    return *reinterpret_cast<const WGPUBindingResource*>(this);
}

static_assert(sizeof(BindingResource) == sizeof(WGPUBindingResource), "sizeof mismatch for BindingResource");
static_assert(alignof(BindingResource) == alignof(WGPUBindingResource), "alignof mismatch for BindingResource");
static_assert(offsetof(BindingResource, nextInChain) == offsetof(WGPUBindingResource, nextInChain),
        "offsetof mismatch for BindingResource::nextInChain");
static_assert(offsetof(BindingResource, buffer) == offsetof(WGPUBindingResource, buffer),
        "offsetof mismatch for BindingResource::buffer");
static_assert(offsetof(BindingResource, offset) == offsetof(WGPUBindingResource, offset),
        "offsetof mismatch for BindingResource::offset");
static_assert(offsetof(BindingResource, size) == offsetof(WGPUBindingResource, size),
        "offsetof mismatch for BindingResource::size");
static_assert(offsetof(BindingResource, sampler) == offsetof(WGPUBindingResource, sampler),
        "offsetof mismatch for BindingResource::sampler");
static_assert(offsetof(BindingResource, textureView) == offsetof(WGPUBindingResource, textureView),
        "offsetof mismatch for BindingResource::textureView");

// BlendComponent implementation

BlendComponent::operator const WGPUBlendComponent&() const noexcept {
    return *reinterpret_cast<const WGPUBlendComponent*>(this);
}

static_assert(sizeof(BlendComponent) == sizeof(WGPUBlendComponent), "sizeof mismatch for BlendComponent");
static_assert(alignof(BlendComponent) == alignof(WGPUBlendComponent), "alignof mismatch for BlendComponent");
static_assert(offsetof(BlendComponent, operation) == offsetof(WGPUBlendComponent, operation),
        "offsetof mismatch for BlendComponent::operation");
static_assert(offsetof(BlendComponent, srcFactor) == offsetof(WGPUBlendComponent, srcFactor),
        "offsetof mismatch for BlendComponent::srcFactor");
static_assert(offsetof(BlendComponent, dstFactor) == offsetof(WGPUBlendComponent, dstFactor),
        "offsetof mismatch for BlendComponent::dstFactor");

// BufferBindingLayout implementation

BufferBindingLayout::operator const WGPUBufferBindingLayout&() const noexcept {
    return *reinterpret_cast<const WGPUBufferBindingLayout*>(this);
}

static_assert(sizeof(BufferBindingLayout) == sizeof(WGPUBufferBindingLayout), "sizeof mismatch for BufferBindingLayout");
static_assert(alignof(BufferBindingLayout) == alignof(WGPUBufferBindingLayout), "alignof mismatch for BufferBindingLayout");
static_assert(offsetof(BufferBindingLayout, nextInChain) == offsetof(WGPUBufferBindingLayout, nextInChain),
        "offsetof mismatch for BufferBindingLayout::nextInChain");
static_assert(offsetof(BufferBindingLayout, type) == offsetof(WGPUBufferBindingLayout, type),
        "offsetof mismatch for BufferBindingLayout::type");
static_assert(offsetof(BufferBindingLayout, hasDynamicOffset) == offsetof(WGPUBufferBindingLayout, hasDynamicOffset),
        "offsetof mismatch for BufferBindingLayout::hasDynamicOffset");
static_assert(offsetof(BufferBindingLayout, minBindingSize) == offsetof(WGPUBufferBindingLayout, minBindingSize),
        "offsetof mismatch for BufferBindingLayout::minBindingSize");

// BufferHostMappedPointer implementation
BufferHostMappedPointer::BufferHostMappedPointer()
: ChainedStruct { nullptr, SType::BufferHostMappedPointer } {}
struct BufferHostMappedPointer::Init {
    ChainedStruct * const nextInChain;
    void * pointer;
    Callback disposeCallback;
    void * userdata;
};
BufferHostMappedPointer::BufferHostMappedPointer(BufferHostMappedPointer::Init&& init) :
    ChainedStruct { init.nextInChain, SType::BufferHostMappedPointer },
    pointer(std::move(init.pointer)),
    disposeCallback(std::move(init.disposeCallback)),
    userdata(std::move(init.userdata)) {}

BufferHostMappedPointer::operator const WGPUBufferHostMappedPointer&() const noexcept {
    return *reinterpret_cast<const WGPUBufferHostMappedPointer*>(this);
}

static_assert(sizeof(BufferHostMappedPointer) == sizeof(WGPUBufferHostMappedPointer), "sizeof mismatch for BufferHostMappedPointer");
static_assert(alignof(BufferHostMappedPointer) == alignof(WGPUBufferHostMappedPointer), "alignof mismatch for BufferHostMappedPointer");
static_assert(offsetof(BufferHostMappedPointer, pointer) == offsetof(WGPUBufferHostMappedPointer, pointer),
        "offsetof mismatch for BufferHostMappedPointer::pointer");
static_assert(offsetof(BufferHostMappedPointer, disposeCallback) == offsetof(WGPUBufferHostMappedPointer, disposeCallback),
        "offsetof mismatch for BufferHostMappedPointer::disposeCallback");
static_assert(offsetof(BufferHostMappedPointer, userdata) == offsetof(WGPUBufferHostMappedPointer, userdata),
        "offsetof mismatch for BufferHostMappedPointer::userdata");

// Color implementation

Color::operator const WGPUColor&() const noexcept {
    return *reinterpret_cast<const WGPUColor*>(this);
}

static_assert(sizeof(Color) == sizeof(WGPUColor), "sizeof mismatch for Color");
static_assert(alignof(Color) == alignof(WGPUColor), "alignof mismatch for Color");
static_assert(offsetof(Color, r) == offsetof(WGPUColor, r),
        "offsetof mismatch for Color::r");
static_assert(offsetof(Color, g) == offsetof(WGPUColor, g),
        "offsetof mismatch for Color::g");
static_assert(offsetof(Color, b) == offsetof(WGPUColor, b),
        "offsetof mismatch for Color::b");
static_assert(offsetof(Color, a) == offsetof(WGPUColor, a),
        "offsetof mismatch for Color::a");

// ColorSpaceDawn implementation

ColorSpaceDawn::operator const WGPUColorSpaceDawn&() const noexcept {
    return *reinterpret_cast<const WGPUColorSpaceDawn*>(this);
}

static_assert(sizeof(ColorSpaceDawn) == sizeof(WGPUColorSpaceDawn), "sizeof mismatch for ColorSpaceDawn");
static_assert(alignof(ColorSpaceDawn) == alignof(WGPUColorSpaceDawn), "alignof mismatch for ColorSpaceDawn");
static_assert(offsetof(ColorSpaceDawn, nextInChain) == offsetof(WGPUColorSpaceDawn, nextInChain),
        "offsetof mismatch for ColorSpaceDawn::nextInChain");
static_assert(offsetof(ColorSpaceDawn, primaries) == offsetof(WGPUColorSpaceDawn, primaries),
        "offsetof mismatch for ColorSpaceDawn::primaries");
static_assert(offsetof(ColorSpaceDawn, transfer) == offsetof(WGPUColorSpaceDawn, transfer),
        "offsetof mismatch for ColorSpaceDawn::transfer");
static_assert(offsetof(ColorSpaceDawn, yCbCrRange) == offsetof(WGPUColorSpaceDawn, yCbCrRange),
        "offsetof mismatch for ColorSpaceDawn::yCbCrRange");
static_assert(offsetof(ColorSpaceDawn, yCbCrMatrix) == offsetof(WGPUColorSpaceDawn, yCbCrMatrix),
        "offsetof mismatch for ColorSpaceDawn::yCbCrMatrix");
static_assert(offsetof(ColorSpaceDawn, hdrReferenceWhiteLuminance) == offsetof(WGPUColorSpaceDawn, hdrReferenceWhiteLuminance),
        "offsetof mismatch for ColorSpaceDawn::hdrReferenceWhiteLuminance");

// ColorTargetStateExpandResolveTextureDawn implementation
ColorTargetStateExpandResolveTextureDawn::ColorTargetStateExpandResolveTextureDawn()
: ChainedStruct { nullptr, SType::ColorTargetStateExpandResolveTextureDawn } {}
struct ColorTargetStateExpandResolveTextureDawn::Init {
    ChainedStruct * const nextInChain;
    Bool enabled = false;
};
ColorTargetStateExpandResolveTextureDawn::ColorTargetStateExpandResolveTextureDawn(ColorTargetStateExpandResolveTextureDawn::Init&& init) :
    ChainedStruct { init.nextInChain, SType::ColorTargetStateExpandResolveTextureDawn },
    enabled(std::move(init.enabled)) {}

ColorTargetStateExpandResolveTextureDawn::operator const WGPUColorTargetStateExpandResolveTextureDawn&() const noexcept {
    return *reinterpret_cast<const WGPUColorTargetStateExpandResolveTextureDawn*>(this);
}

static_assert(sizeof(ColorTargetStateExpandResolveTextureDawn) == sizeof(WGPUColorTargetStateExpandResolveTextureDawn), "sizeof mismatch for ColorTargetStateExpandResolveTextureDawn");
static_assert(alignof(ColorTargetStateExpandResolveTextureDawn) == alignof(WGPUColorTargetStateExpandResolveTextureDawn), "alignof mismatch for ColorTargetStateExpandResolveTextureDawn");
static_assert(offsetof(ColorTargetStateExpandResolveTextureDawn, enabled) == offsetof(WGPUColorTargetStateExpandResolveTextureDawn, enabled),
        "offsetof mismatch for ColorTargetStateExpandResolveTextureDawn::enabled");

// CommandBufferDescriptor implementation

CommandBufferDescriptor::operator const WGPUCommandBufferDescriptor&() const noexcept {
    return *reinterpret_cast<const WGPUCommandBufferDescriptor*>(this);
}

static_assert(sizeof(CommandBufferDescriptor) == sizeof(WGPUCommandBufferDescriptor), "sizeof mismatch for CommandBufferDescriptor");
static_assert(alignof(CommandBufferDescriptor) == alignof(WGPUCommandBufferDescriptor), "alignof mismatch for CommandBufferDescriptor");
static_assert(offsetof(CommandBufferDescriptor, nextInChain) == offsetof(WGPUCommandBufferDescriptor, nextInChain),
        "offsetof mismatch for CommandBufferDescriptor::nextInChain");
static_assert(offsetof(CommandBufferDescriptor, label) == offsetof(WGPUCommandBufferDescriptor, label),
        "offsetof mismatch for CommandBufferDescriptor::label");

// CompatibilityModeLimits implementation
CompatibilityModeLimits::CompatibilityModeLimits()
: ChainedStructOut { nullptr, SType::CompatibilityModeLimits } {}
struct CompatibilityModeLimits::Init {
    ChainedStructOut *  nextInChain;
    uint32_t maxStorageBuffersInVertexStage = kLimitU32Undefined;
    uint32_t maxStorageTexturesInVertexStage = kLimitU32Undefined;
    uint32_t maxStorageBuffersInFragmentStage = kLimitU32Undefined;
    uint32_t maxStorageTexturesInFragmentStage = kLimitU32Undefined;
};
CompatibilityModeLimits::CompatibilityModeLimits(CompatibilityModeLimits::Init&& init) :
    ChainedStructOut { init.nextInChain, SType::CompatibilityModeLimits },
    maxStorageBuffersInVertexStage(std::move(init.maxStorageBuffersInVertexStage)),
    maxStorageTexturesInVertexStage(std::move(init.maxStorageTexturesInVertexStage)),
    maxStorageBuffersInFragmentStage(std::move(init.maxStorageBuffersInFragmentStage)),
    maxStorageTexturesInFragmentStage(std::move(init.maxStorageTexturesInFragmentStage)) {}

CompatibilityModeLimits::operator const WGPUCompatibilityModeLimits&() const noexcept {
    return *reinterpret_cast<const WGPUCompatibilityModeLimits*>(this);
}

static_assert(sizeof(CompatibilityModeLimits) == sizeof(WGPUCompatibilityModeLimits), "sizeof mismatch for CompatibilityModeLimits");
static_assert(alignof(CompatibilityModeLimits) == alignof(WGPUCompatibilityModeLimits), "alignof mismatch for CompatibilityModeLimits");
static_assert(offsetof(CompatibilityModeLimits, maxStorageBuffersInVertexStage) == offsetof(WGPUCompatibilityModeLimits, maxStorageBuffersInVertexStage),
        "offsetof mismatch for CompatibilityModeLimits::maxStorageBuffersInVertexStage");
static_assert(offsetof(CompatibilityModeLimits, maxStorageTexturesInVertexStage) == offsetof(WGPUCompatibilityModeLimits, maxStorageTexturesInVertexStage),
        "offsetof mismatch for CompatibilityModeLimits::maxStorageTexturesInVertexStage");
static_assert(offsetof(CompatibilityModeLimits, maxStorageBuffersInFragmentStage) == offsetof(WGPUCompatibilityModeLimits, maxStorageBuffersInFragmentStage),
        "offsetof mismatch for CompatibilityModeLimits::maxStorageBuffersInFragmentStage");
static_assert(offsetof(CompatibilityModeLimits, maxStorageTexturesInFragmentStage) == offsetof(WGPUCompatibilityModeLimits, maxStorageTexturesInFragmentStage),
        "offsetof mismatch for CompatibilityModeLimits::maxStorageTexturesInFragmentStage");

// ConstantEntry implementation

ConstantEntry::operator const WGPUConstantEntry&() const noexcept {
    return *reinterpret_cast<const WGPUConstantEntry*>(this);
}

static_assert(sizeof(ConstantEntry) == sizeof(WGPUConstantEntry), "sizeof mismatch for ConstantEntry");
static_assert(alignof(ConstantEntry) == alignof(WGPUConstantEntry), "alignof mismatch for ConstantEntry");
static_assert(offsetof(ConstantEntry, nextInChain) == offsetof(WGPUConstantEntry, nextInChain),
        "offsetof mismatch for ConstantEntry::nextInChain");
static_assert(offsetof(ConstantEntry, key) == offsetof(WGPUConstantEntry, key),
        "offsetof mismatch for ConstantEntry::key");
static_assert(offsetof(ConstantEntry, value) == offsetof(WGPUConstantEntry, value),
        "offsetof mismatch for ConstantEntry::value");

// CopyTextureForBrowserOptions implementation

CopyTextureForBrowserOptions::operator const WGPUCopyTextureForBrowserOptions&() const noexcept {
    return *reinterpret_cast<const WGPUCopyTextureForBrowserOptions*>(this);
}

static_assert(sizeof(CopyTextureForBrowserOptions) == sizeof(WGPUCopyTextureForBrowserOptions), "sizeof mismatch for CopyTextureForBrowserOptions");
static_assert(alignof(CopyTextureForBrowserOptions) == alignof(WGPUCopyTextureForBrowserOptions), "alignof mismatch for CopyTextureForBrowserOptions");
static_assert(offsetof(CopyTextureForBrowserOptions, nextInChain) == offsetof(WGPUCopyTextureForBrowserOptions, nextInChain),
        "offsetof mismatch for CopyTextureForBrowserOptions::nextInChain");
static_assert(offsetof(CopyTextureForBrowserOptions, flipY) == offsetof(WGPUCopyTextureForBrowserOptions, flipY),
        "offsetof mismatch for CopyTextureForBrowserOptions::flipY");
static_assert(offsetof(CopyTextureForBrowserOptions, needsColorSpaceConversion) == offsetof(WGPUCopyTextureForBrowserOptions, needsColorSpaceConversion),
        "offsetof mismatch for CopyTextureForBrowserOptions::needsColorSpaceConversion");
static_assert(offsetof(CopyTextureForBrowserOptions, srcAlphaMode) == offsetof(WGPUCopyTextureForBrowserOptions, srcAlphaMode),
        "offsetof mismatch for CopyTextureForBrowserOptions::srcAlphaMode");
static_assert(offsetof(CopyTextureForBrowserOptions, srcTransferFunctionParameters) == offsetof(WGPUCopyTextureForBrowserOptions, srcTransferFunctionParameters),
        "offsetof mismatch for CopyTextureForBrowserOptions::srcTransferFunctionParameters");
static_assert(offsetof(CopyTextureForBrowserOptions, conversionMatrix) == offsetof(WGPUCopyTextureForBrowserOptions, conversionMatrix),
        "offsetof mismatch for CopyTextureForBrowserOptions::conversionMatrix");
static_assert(offsetof(CopyTextureForBrowserOptions, dstTransferFunctionParameters) == offsetof(WGPUCopyTextureForBrowserOptions, dstTransferFunctionParameters),
        "offsetof mismatch for CopyTextureForBrowserOptions::dstTransferFunctionParameters");
static_assert(offsetof(CopyTextureForBrowserOptions, dstAlphaMode) == offsetof(WGPUCopyTextureForBrowserOptions, dstAlphaMode),
        "offsetof mismatch for CopyTextureForBrowserOptions::dstAlphaMode");
static_assert(offsetof(CopyTextureForBrowserOptions, internalUsage) == offsetof(WGPUCopyTextureForBrowserOptions, internalUsage),
        "offsetof mismatch for CopyTextureForBrowserOptions::internalUsage");

// DawnAdapterPropertiesPowerPreference implementation
DawnAdapterPropertiesPowerPreference::DawnAdapterPropertiesPowerPreference()
: ChainedStructOut { nullptr, SType::DawnAdapterPropertiesPowerPreference } {}
struct DawnAdapterPropertiesPowerPreference::Init {
    ChainedStructOut *  nextInChain;
    PowerPreference powerPreference = PowerPreference::Undefined;
};
DawnAdapterPropertiesPowerPreference::DawnAdapterPropertiesPowerPreference(DawnAdapterPropertiesPowerPreference::Init&& init) :
    ChainedStructOut { init.nextInChain, SType::DawnAdapterPropertiesPowerPreference },
    powerPreference(std::move(init.powerPreference)) {}

DawnAdapterPropertiesPowerPreference::operator const WGPUDawnAdapterPropertiesPowerPreference&() const noexcept {
    return *reinterpret_cast<const WGPUDawnAdapterPropertiesPowerPreference*>(this);
}

static_assert(sizeof(DawnAdapterPropertiesPowerPreference) == sizeof(WGPUDawnAdapterPropertiesPowerPreference), "sizeof mismatch for DawnAdapterPropertiesPowerPreference");
static_assert(alignof(DawnAdapterPropertiesPowerPreference) == alignof(WGPUDawnAdapterPropertiesPowerPreference), "alignof mismatch for DawnAdapterPropertiesPowerPreference");
static_assert(offsetof(DawnAdapterPropertiesPowerPreference, powerPreference) == offsetof(WGPUDawnAdapterPropertiesPowerPreference, powerPreference),
        "offsetof mismatch for DawnAdapterPropertiesPowerPreference::powerPreference");

// DawnBufferDescriptorErrorInfoFromWireClient implementation
DawnBufferDescriptorErrorInfoFromWireClient::DawnBufferDescriptorErrorInfoFromWireClient()
: ChainedStruct { nullptr, SType::DawnBufferDescriptorErrorInfoFromWireClient } {}
struct DawnBufferDescriptorErrorInfoFromWireClient::Init {
    ChainedStruct * const nextInChain;
    Bool outOfMemory = false;
};
DawnBufferDescriptorErrorInfoFromWireClient::DawnBufferDescriptorErrorInfoFromWireClient(DawnBufferDescriptorErrorInfoFromWireClient::Init&& init) :
    ChainedStruct { init.nextInChain, SType::DawnBufferDescriptorErrorInfoFromWireClient },
    outOfMemory(std::move(init.outOfMemory)) {}

DawnBufferDescriptorErrorInfoFromWireClient::operator const WGPUDawnBufferDescriptorErrorInfoFromWireClient&() const noexcept {
    return *reinterpret_cast<const WGPUDawnBufferDescriptorErrorInfoFromWireClient*>(this);
}

static_assert(sizeof(DawnBufferDescriptorErrorInfoFromWireClient) == sizeof(WGPUDawnBufferDescriptorErrorInfoFromWireClient), "sizeof mismatch for DawnBufferDescriptorErrorInfoFromWireClient");
static_assert(alignof(DawnBufferDescriptorErrorInfoFromWireClient) == alignof(WGPUDawnBufferDescriptorErrorInfoFromWireClient), "alignof mismatch for DawnBufferDescriptorErrorInfoFromWireClient");
static_assert(offsetof(DawnBufferDescriptorErrorInfoFromWireClient, outOfMemory) == offsetof(WGPUDawnBufferDescriptorErrorInfoFromWireClient, outOfMemory),
        "offsetof mismatch for DawnBufferDescriptorErrorInfoFromWireClient::outOfMemory");

// DawnCacheDeviceDescriptor implementation
DawnCacheDeviceDescriptor::DawnCacheDeviceDescriptor()
: ChainedStruct { nullptr, SType::DawnCacheDeviceDescriptor } {}
struct DawnCacheDeviceDescriptor::Init {
    ChainedStruct * const nextInChain;
    StringView isolationKey = {};
};
DawnCacheDeviceDescriptor::DawnCacheDeviceDescriptor(DawnCacheDeviceDescriptor::Init&& init) :
    ChainedStruct { init.nextInChain, SType::DawnCacheDeviceDescriptor },
    detail::DawnCacheDeviceDescriptor {
        std::move(init.isolationKey),
    } {}
template <typename F, typename T>
void DawnCacheDeviceDescriptor::SetDawnLoadCacheDataCallback(F callback, T userdata) {
    static_assert(offsetof(DawnCacheDeviceDescriptor, dawnLoadCacheDataCallbackInfo) == offsetof(WGPUDawnCacheDeviceDescriptor, dawnLoadCacheDataCallbackInfo),
                  "offsetof mismatch for DawnCacheDeviceDescriptor::dawnLoadCacheDataCallbackInfo");

    assert(dawnLoadCacheDataCallbackInfo.callback == nullptr);
    dawnLoadCacheDataCallbackInfo = detail::CallbackInfoHelper<WGPUDawnLoadCacheDataCallbackInfo, F>::Create(std::move(callback), userdata);
}
template <typename F>
void DawnCacheDeviceDescriptor::SetDawnLoadCacheDataCallback(F callback) {
    assert(dawnLoadCacheDataCallbackInfo.callback == nullptr);
    dawnLoadCacheDataCallbackInfo = detail::CallbackInfoHelper<WGPUDawnLoadCacheDataCallbackInfo, F>::Create(std::move(callback));
}
template <typename F, typename T>
void DawnCacheDeviceDescriptor::SetDawnStoreCacheDataCallback(F callback, T userdata) {
    static_assert(offsetof(DawnCacheDeviceDescriptor, dawnStoreCacheDataCallbackInfo) == offsetof(WGPUDawnCacheDeviceDescriptor, dawnStoreCacheDataCallbackInfo),
                  "offsetof mismatch for DawnCacheDeviceDescriptor::dawnStoreCacheDataCallbackInfo");

    assert(dawnStoreCacheDataCallbackInfo.callback == nullptr);
    dawnStoreCacheDataCallbackInfo = detail::CallbackInfoHelper<WGPUDawnStoreCacheDataCallbackInfo, F>::Create(std::move(callback), userdata);
}
template <typename F>
void DawnCacheDeviceDescriptor::SetDawnStoreCacheDataCallback(F callback) {
    assert(dawnStoreCacheDataCallbackInfo.callback == nullptr);
    dawnStoreCacheDataCallbackInfo = detail::CallbackInfoHelper<WGPUDawnStoreCacheDataCallbackInfo, F>::Create(std::move(callback));
}

DawnCacheDeviceDescriptor::operator const WGPUDawnCacheDeviceDescriptor&() const noexcept {
    return *reinterpret_cast<const WGPUDawnCacheDeviceDescriptor*>(this);
}

static_assert(sizeof(DawnCacheDeviceDescriptor) == sizeof(WGPUDawnCacheDeviceDescriptor), "sizeof mismatch for DawnCacheDeviceDescriptor");
static_assert(alignof(DawnCacheDeviceDescriptor) == alignof(WGPUDawnCacheDeviceDescriptor), "alignof mismatch for DawnCacheDeviceDescriptor");
static_assert(offsetof(DawnCacheDeviceDescriptor, isolationKey) == offsetof(WGPUDawnCacheDeviceDescriptor, isolationKey),
        "offsetof mismatch for DawnCacheDeviceDescriptor::isolationKey");

// DawnCompilationMessageUtf16 implementation
DawnCompilationMessageUtf16::DawnCompilationMessageUtf16()
: ChainedStruct { nullptr, SType::DawnCompilationMessageUtf16 } {}
struct DawnCompilationMessageUtf16::Init {
    ChainedStruct * const nextInChain;
    uint64_t linePos;
    uint64_t offset;
    uint64_t length;
};
DawnCompilationMessageUtf16::DawnCompilationMessageUtf16(DawnCompilationMessageUtf16::Init&& init) :
    ChainedStruct { init.nextInChain, SType::DawnCompilationMessageUtf16 },
    linePos(std::move(init.linePos)),
    offset(std::move(init.offset)),
    length(std::move(init.length)) {}

DawnCompilationMessageUtf16::operator const WGPUDawnCompilationMessageUtf16&() const noexcept {
    return *reinterpret_cast<const WGPUDawnCompilationMessageUtf16*>(this);
}

static_assert(sizeof(DawnCompilationMessageUtf16) == sizeof(WGPUDawnCompilationMessageUtf16), "sizeof mismatch for DawnCompilationMessageUtf16");
static_assert(alignof(DawnCompilationMessageUtf16) == alignof(WGPUDawnCompilationMessageUtf16), "alignof mismatch for DawnCompilationMessageUtf16");
static_assert(offsetof(DawnCompilationMessageUtf16, linePos) == offsetof(WGPUDawnCompilationMessageUtf16, linePos),
        "offsetof mismatch for DawnCompilationMessageUtf16::linePos");
static_assert(offsetof(DawnCompilationMessageUtf16, offset) == offsetof(WGPUDawnCompilationMessageUtf16, offset),
        "offsetof mismatch for DawnCompilationMessageUtf16::offset");
static_assert(offsetof(DawnCompilationMessageUtf16, length) == offsetof(WGPUDawnCompilationMessageUtf16, length),
        "offsetof mismatch for DawnCompilationMessageUtf16::length");

// DawnConsumeAdapterDescriptor implementation
DawnConsumeAdapterDescriptor::DawnConsumeAdapterDescriptor()
: ChainedStruct { nullptr, SType::DawnConsumeAdapterDescriptor } {}
struct DawnConsumeAdapterDescriptor::Init {
    ChainedStruct * const nextInChain;
    Bool consumeAdapter = false;
};
DawnConsumeAdapterDescriptor::DawnConsumeAdapterDescriptor(DawnConsumeAdapterDescriptor::Init&& init) :
    ChainedStruct { init.nextInChain, SType::DawnConsumeAdapterDescriptor },
    consumeAdapter(std::move(init.consumeAdapter)) {}

DawnConsumeAdapterDescriptor::operator const WGPUDawnConsumeAdapterDescriptor&() const noexcept {
    return *reinterpret_cast<const WGPUDawnConsumeAdapterDescriptor*>(this);
}

static_assert(sizeof(DawnConsumeAdapterDescriptor) == sizeof(WGPUDawnConsumeAdapterDescriptor), "sizeof mismatch for DawnConsumeAdapterDescriptor");
static_assert(alignof(DawnConsumeAdapterDescriptor) == alignof(WGPUDawnConsumeAdapterDescriptor), "alignof mismatch for DawnConsumeAdapterDescriptor");
static_assert(offsetof(DawnConsumeAdapterDescriptor, consumeAdapter) == offsetof(WGPUDawnConsumeAdapterDescriptor, consumeAdapter),
        "offsetof mismatch for DawnConsumeAdapterDescriptor::consumeAdapter");

// DawnDeviceAllocatorControl implementation
DawnDeviceAllocatorControl::DawnDeviceAllocatorControl()
: ChainedStruct { nullptr, SType::DawnDeviceAllocatorControl } {}
struct DawnDeviceAllocatorControl::Init {
    ChainedStruct * const nextInChain;
    size_t allocatorHeapBlockSize = 0;
};
DawnDeviceAllocatorControl::DawnDeviceAllocatorControl(DawnDeviceAllocatorControl::Init&& init) :
    ChainedStruct { init.nextInChain, SType::DawnDeviceAllocatorControl },
    allocatorHeapBlockSize(std::move(init.allocatorHeapBlockSize)) {}

DawnDeviceAllocatorControl::operator const WGPUDawnDeviceAllocatorControl&() const noexcept {
    return *reinterpret_cast<const WGPUDawnDeviceAllocatorControl*>(this);
}

static_assert(sizeof(DawnDeviceAllocatorControl) == sizeof(WGPUDawnDeviceAllocatorControl), "sizeof mismatch for DawnDeviceAllocatorControl");
static_assert(alignof(DawnDeviceAllocatorControl) == alignof(WGPUDawnDeviceAllocatorControl), "alignof mismatch for DawnDeviceAllocatorControl");
static_assert(offsetof(DawnDeviceAllocatorControl, allocatorHeapBlockSize) == offsetof(WGPUDawnDeviceAllocatorControl, allocatorHeapBlockSize),
        "offsetof mismatch for DawnDeviceAllocatorControl::allocatorHeapBlockSize");

// DawnDrmFormatProperties implementation

DawnDrmFormatProperties::operator const WGPUDawnDrmFormatProperties&() const noexcept {
    return *reinterpret_cast<const WGPUDawnDrmFormatProperties*>(this);
}

static_assert(sizeof(DawnDrmFormatProperties) == sizeof(WGPUDawnDrmFormatProperties), "sizeof mismatch for DawnDrmFormatProperties");
static_assert(alignof(DawnDrmFormatProperties) == alignof(WGPUDawnDrmFormatProperties), "alignof mismatch for DawnDrmFormatProperties");
static_assert(offsetof(DawnDrmFormatProperties, modifier) == offsetof(WGPUDawnDrmFormatProperties, modifier),
        "offsetof mismatch for DawnDrmFormatProperties::modifier");
static_assert(offsetof(DawnDrmFormatProperties, modifierPlaneCount) == offsetof(WGPUDawnDrmFormatProperties, modifierPlaneCount),
        "offsetof mismatch for DawnDrmFormatProperties::modifierPlaneCount");

// DawnEncoderInternalUsageDescriptor implementation
DawnEncoderInternalUsageDescriptor::DawnEncoderInternalUsageDescriptor()
: ChainedStruct { nullptr, SType::DawnEncoderInternalUsageDescriptor } {}
struct DawnEncoderInternalUsageDescriptor::Init {
    ChainedStruct * const nextInChain;
    Bool useInternalUsages = false;
};
DawnEncoderInternalUsageDescriptor::DawnEncoderInternalUsageDescriptor(DawnEncoderInternalUsageDescriptor::Init&& init) :
    ChainedStruct { init.nextInChain, SType::DawnEncoderInternalUsageDescriptor },
    useInternalUsages(std::move(init.useInternalUsages)) {}

DawnEncoderInternalUsageDescriptor::operator const WGPUDawnEncoderInternalUsageDescriptor&() const noexcept {
    return *reinterpret_cast<const WGPUDawnEncoderInternalUsageDescriptor*>(this);
}

static_assert(sizeof(DawnEncoderInternalUsageDescriptor) == sizeof(WGPUDawnEncoderInternalUsageDescriptor), "sizeof mismatch for DawnEncoderInternalUsageDescriptor");
static_assert(alignof(DawnEncoderInternalUsageDescriptor) == alignof(WGPUDawnEncoderInternalUsageDescriptor), "alignof mismatch for DawnEncoderInternalUsageDescriptor");
static_assert(offsetof(DawnEncoderInternalUsageDescriptor, useInternalUsages) == offsetof(WGPUDawnEncoderInternalUsageDescriptor, useInternalUsages),
        "offsetof mismatch for DawnEncoderInternalUsageDescriptor::useInternalUsages");

// DawnFakeBufferOOMForTesting implementation
DawnFakeBufferOOMForTesting::DawnFakeBufferOOMForTesting()
: ChainedStruct { nullptr, SType::DawnFakeBufferOOMForTesting } {}
struct DawnFakeBufferOOMForTesting::Init {
    ChainedStruct * const nextInChain;
    Bool fakeOOMAtWireClientMap;
    Bool fakeOOMAtNativeMap;
    Bool fakeOOMAtDevice;
};
DawnFakeBufferOOMForTesting::DawnFakeBufferOOMForTesting(DawnFakeBufferOOMForTesting::Init&& init) :
    ChainedStruct { init.nextInChain, SType::DawnFakeBufferOOMForTesting },
    fakeOOMAtWireClientMap(std::move(init.fakeOOMAtWireClientMap)),
    fakeOOMAtNativeMap(std::move(init.fakeOOMAtNativeMap)),
    fakeOOMAtDevice(std::move(init.fakeOOMAtDevice)) {}

DawnFakeBufferOOMForTesting::operator const WGPUDawnFakeBufferOOMForTesting&() const noexcept {
    return *reinterpret_cast<const WGPUDawnFakeBufferOOMForTesting*>(this);
}

static_assert(sizeof(DawnFakeBufferOOMForTesting) == sizeof(WGPUDawnFakeBufferOOMForTesting), "sizeof mismatch for DawnFakeBufferOOMForTesting");
static_assert(alignof(DawnFakeBufferOOMForTesting) == alignof(WGPUDawnFakeBufferOOMForTesting), "alignof mismatch for DawnFakeBufferOOMForTesting");
static_assert(offsetof(DawnFakeBufferOOMForTesting, fakeOOMAtWireClientMap) == offsetof(WGPUDawnFakeBufferOOMForTesting, fakeOOMAtWireClientMap),
        "offsetof mismatch for DawnFakeBufferOOMForTesting::fakeOOMAtWireClientMap");
static_assert(offsetof(DawnFakeBufferOOMForTesting, fakeOOMAtNativeMap) == offsetof(WGPUDawnFakeBufferOOMForTesting, fakeOOMAtNativeMap),
        "offsetof mismatch for DawnFakeBufferOOMForTesting::fakeOOMAtNativeMap");
static_assert(offsetof(DawnFakeBufferOOMForTesting, fakeOOMAtDevice) == offsetof(WGPUDawnFakeBufferOOMForTesting, fakeOOMAtDevice),
        "offsetof mismatch for DawnFakeBufferOOMForTesting::fakeOOMAtDevice");

// DawnFakeDeviceInitializeErrorForTesting implementation
DawnFakeDeviceInitializeErrorForTesting::DawnFakeDeviceInitializeErrorForTesting()
: ChainedStruct { nullptr, SType::DawnFakeDeviceInitializeErrorForTesting } {}
struct DawnFakeDeviceInitializeErrorForTesting::Init {
    ChainedStruct * const nextInChain;
};
DawnFakeDeviceInitializeErrorForTesting::DawnFakeDeviceInitializeErrorForTesting(DawnFakeDeviceInitializeErrorForTesting::Init&& init) :
    ChainedStruct { init.nextInChain, SType::DawnFakeDeviceInitializeErrorForTesting } {}

DawnFakeDeviceInitializeErrorForTesting::operator const WGPUDawnFakeDeviceInitializeErrorForTesting&() const noexcept {
    return *reinterpret_cast<const WGPUDawnFakeDeviceInitializeErrorForTesting*>(this);
}

static_assert(sizeof(DawnFakeDeviceInitializeErrorForTesting) == sizeof(WGPUDawnFakeDeviceInitializeErrorForTesting), "sizeof mismatch for DawnFakeDeviceInitializeErrorForTesting");
static_assert(alignof(DawnFakeDeviceInitializeErrorForTesting) == alignof(WGPUDawnFakeDeviceInitializeErrorForTesting), "alignof mismatch for DawnFakeDeviceInitializeErrorForTesting");

// DawnHostMappedPointerLimits implementation
DawnHostMappedPointerLimits::DawnHostMappedPointerLimits()
: ChainedStructOut { nullptr, SType::DawnHostMappedPointerLimits } {}
struct DawnHostMappedPointerLimits::Init {
    ChainedStructOut *  nextInChain;
    uint32_t hostMappedPointerAlignment = kLimitU32Undefined;
};
DawnHostMappedPointerLimits::DawnHostMappedPointerLimits(DawnHostMappedPointerLimits::Init&& init) :
    ChainedStructOut { init.nextInChain, SType::DawnHostMappedPointerLimits },
    hostMappedPointerAlignment(std::move(init.hostMappedPointerAlignment)) {}

DawnHostMappedPointerLimits::operator const WGPUDawnHostMappedPointerLimits&() const noexcept {
    return *reinterpret_cast<const WGPUDawnHostMappedPointerLimits*>(this);
}

static_assert(sizeof(DawnHostMappedPointerLimits) == sizeof(WGPUDawnHostMappedPointerLimits), "sizeof mismatch for DawnHostMappedPointerLimits");
static_assert(alignof(DawnHostMappedPointerLimits) == alignof(WGPUDawnHostMappedPointerLimits), "alignof mismatch for DawnHostMappedPointerLimits");
static_assert(offsetof(DawnHostMappedPointerLimits, hostMappedPointerAlignment) == offsetof(WGPUDawnHostMappedPointerLimits, hostMappedPointerAlignment),
        "offsetof mismatch for DawnHostMappedPointerLimits::hostMappedPointerAlignment");

// DawnInjectedInvalidSType implementation
DawnInjectedInvalidSType::DawnInjectedInvalidSType()
: ChainedStruct { nullptr, SType::DawnInjectedInvalidSType } {}
struct DawnInjectedInvalidSType::Init {
    ChainedStruct * const nextInChain;
    SType invalidSType = {};
};
DawnInjectedInvalidSType::DawnInjectedInvalidSType(DawnInjectedInvalidSType::Init&& init) :
    ChainedStruct { init.nextInChain, SType::DawnInjectedInvalidSType },
    invalidSType(std::move(init.invalidSType)) {}

DawnInjectedInvalidSType::operator const WGPUDawnInjectedInvalidSType&() const noexcept {
    return *reinterpret_cast<const WGPUDawnInjectedInvalidSType*>(this);
}

static_assert(sizeof(DawnInjectedInvalidSType) == sizeof(WGPUDawnInjectedInvalidSType), "sizeof mismatch for DawnInjectedInvalidSType");
static_assert(alignof(DawnInjectedInvalidSType) == alignof(WGPUDawnInjectedInvalidSType), "alignof mismatch for DawnInjectedInvalidSType");
static_assert(offsetof(DawnInjectedInvalidSType, invalidSType) == offsetof(WGPUDawnInjectedInvalidSType, invalidSType),
        "offsetof mismatch for DawnInjectedInvalidSType::invalidSType");

// DawnRenderPassSampleCount implementation
DawnRenderPassSampleCount::DawnRenderPassSampleCount()
: ChainedStruct { nullptr, SType::DawnRenderPassSampleCount } {}
struct DawnRenderPassSampleCount::Init {
    ChainedStruct * const nextInChain;
    uint32_t sampleCount = 1;
};
DawnRenderPassSampleCount::DawnRenderPassSampleCount(DawnRenderPassSampleCount::Init&& init) :
    ChainedStruct { init.nextInChain, SType::DawnRenderPassSampleCount },
    sampleCount(std::move(init.sampleCount)) {}

DawnRenderPassSampleCount::operator const WGPUDawnRenderPassSampleCount&() const noexcept {
    return *reinterpret_cast<const WGPUDawnRenderPassSampleCount*>(this);
}

static_assert(sizeof(DawnRenderPassSampleCount) == sizeof(WGPUDawnRenderPassSampleCount), "sizeof mismatch for DawnRenderPassSampleCount");
static_assert(alignof(DawnRenderPassSampleCount) == alignof(WGPUDawnRenderPassSampleCount), "alignof mismatch for DawnRenderPassSampleCount");
static_assert(offsetof(DawnRenderPassSampleCount, sampleCount) == offsetof(WGPUDawnRenderPassSampleCount, sampleCount),
        "offsetof mismatch for DawnRenderPassSampleCount::sampleCount");

// DawnShaderModuleSPIRVOptionsDescriptor implementation
DawnShaderModuleSPIRVOptionsDescriptor::DawnShaderModuleSPIRVOptionsDescriptor()
: ChainedStruct { nullptr, SType::DawnShaderModuleSPIRVOptionsDescriptor } {}
struct DawnShaderModuleSPIRVOptionsDescriptor::Init {
    ChainedStruct * const nextInChain;
    Bool allowNonUniformDerivatives = false;
};
DawnShaderModuleSPIRVOptionsDescriptor::DawnShaderModuleSPIRVOptionsDescriptor(DawnShaderModuleSPIRVOptionsDescriptor::Init&& init) :
    ChainedStruct { init.nextInChain, SType::DawnShaderModuleSPIRVOptionsDescriptor },
    allowNonUniformDerivatives(std::move(init.allowNonUniformDerivatives)) {}

DawnShaderModuleSPIRVOptionsDescriptor::operator const WGPUDawnShaderModuleSPIRVOptionsDescriptor&() const noexcept {
    return *reinterpret_cast<const WGPUDawnShaderModuleSPIRVOptionsDescriptor*>(this);
}

static_assert(sizeof(DawnShaderModuleSPIRVOptionsDescriptor) == sizeof(WGPUDawnShaderModuleSPIRVOptionsDescriptor), "sizeof mismatch for DawnShaderModuleSPIRVOptionsDescriptor");
static_assert(alignof(DawnShaderModuleSPIRVOptionsDescriptor) == alignof(WGPUDawnShaderModuleSPIRVOptionsDescriptor), "alignof mismatch for DawnShaderModuleSPIRVOptionsDescriptor");
static_assert(offsetof(DawnShaderModuleSPIRVOptionsDescriptor, allowNonUniformDerivatives) == offsetof(WGPUDawnShaderModuleSPIRVOptionsDescriptor, allowNonUniformDerivatives),
        "offsetof mismatch for DawnShaderModuleSPIRVOptionsDescriptor::allowNonUniformDerivatives");

// DawnTexelCopyBufferRowAlignmentLimits implementation
DawnTexelCopyBufferRowAlignmentLimits::DawnTexelCopyBufferRowAlignmentLimits()
: ChainedStructOut { nullptr, SType::DawnTexelCopyBufferRowAlignmentLimits } {}
struct DawnTexelCopyBufferRowAlignmentLimits::Init {
    ChainedStructOut *  nextInChain;
    uint32_t minTexelCopyBufferRowAlignment = kLimitU32Undefined;
};
DawnTexelCopyBufferRowAlignmentLimits::DawnTexelCopyBufferRowAlignmentLimits(DawnTexelCopyBufferRowAlignmentLimits::Init&& init) :
    ChainedStructOut { init.nextInChain, SType::DawnTexelCopyBufferRowAlignmentLimits },
    minTexelCopyBufferRowAlignment(std::move(init.minTexelCopyBufferRowAlignment)) {}

DawnTexelCopyBufferRowAlignmentLimits::operator const WGPUDawnTexelCopyBufferRowAlignmentLimits&() const noexcept {
    return *reinterpret_cast<const WGPUDawnTexelCopyBufferRowAlignmentLimits*>(this);
}

static_assert(sizeof(DawnTexelCopyBufferRowAlignmentLimits) == sizeof(WGPUDawnTexelCopyBufferRowAlignmentLimits), "sizeof mismatch for DawnTexelCopyBufferRowAlignmentLimits");
static_assert(alignof(DawnTexelCopyBufferRowAlignmentLimits) == alignof(WGPUDawnTexelCopyBufferRowAlignmentLimits), "alignof mismatch for DawnTexelCopyBufferRowAlignmentLimits");
static_assert(offsetof(DawnTexelCopyBufferRowAlignmentLimits, minTexelCopyBufferRowAlignment) == offsetof(WGPUDawnTexelCopyBufferRowAlignmentLimits, minTexelCopyBufferRowAlignment),
        "offsetof mismatch for DawnTexelCopyBufferRowAlignmentLimits::minTexelCopyBufferRowAlignment");

// DawnTextureInternalUsageDescriptor implementation
DawnTextureInternalUsageDescriptor::DawnTextureInternalUsageDescriptor()
: ChainedStruct { nullptr, SType::DawnTextureInternalUsageDescriptor } {}
struct DawnTextureInternalUsageDescriptor::Init {
    ChainedStruct * const nextInChain;
    TextureUsage internalUsage = TextureUsage::None;
};
DawnTextureInternalUsageDescriptor::DawnTextureInternalUsageDescriptor(DawnTextureInternalUsageDescriptor::Init&& init) :
    ChainedStruct { init.nextInChain, SType::DawnTextureInternalUsageDescriptor },
    internalUsage(std::move(init.internalUsage)) {}

DawnTextureInternalUsageDescriptor::operator const WGPUDawnTextureInternalUsageDescriptor&() const noexcept {
    return *reinterpret_cast<const WGPUDawnTextureInternalUsageDescriptor*>(this);
}

static_assert(sizeof(DawnTextureInternalUsageDescriptor) == sizeof(WGPUDawnTextureInternalUsageDescriptor), "sizeof mismatch for DawnTextureInternalUsageDescriptor");
static_assert(alignof(DawnTextureInternalUsageDescriptor) == alignof(WGPUDawnTextureInternalUsageDescriptor), "alignof mismatch for DawnTextureInternalUsageDescriptor");
static_assert(offsetof(DawnTextureInternalUsageDescriptor, internalUsage) == offsetof(WGPUDawnTextureInternalUsageDescriptor, internalUsage),
        "offsetof mismatch for DawnTextureInternalUsageDescriptor::internalUsage");

// DawnTogglesDescriptor implementation
DawnTogglesDescriptor::DawnTogglesDescriptor()
: ChainedStruct { nullptr, SType::DawnTogglesDescriptor } {}
struct DawnTogglesDescriptor::Init {
    ChainedStruct * const nextInChain;
    size_t enabledToggleCount = 0;
    const char* const * enabledToggles = nullptr;
    size_t disabledToggleCount = 0;
    const char* const * disabledToggles = nullptr;
};
DawnTogglesDescriptor::DawnTogglesDescriptor(DawnTogglesDescriptor::Init&& init) :
    ChainedStruct { init.nextInChain, SType::DawnTogglesDescriptor },
    enabledToggleCount(std::move(init.enabledToggleCount)),
    enabledToggles(std::move(init.enabledToggles)),
    disabledToggleCount(std::move(init.disabledToggleCount)),
    disabledToggles(std::move(init.disabledToggles)) {}

DawnTogglesDescriptor::operator const WGPUDawnTogglesDescriptor&() const noexcept {
    return *reinterpret_cast<const WGPUDawnTogglesDescriptor*>(this);
}

static_assert(sizeof(DawnTogglesDescriptor) == sizeof(WGPUDawnTogglesDescriptor), "sizeof mismatch for DawnTogglesDescriptor");
static_assert(alignof(DawnTogglesDescriptor) == alignof(WGPUDawnTogglesDescriptor), "alignof mismatch for DawnTogglesDescriptor");
static_assert(offsetof(DawnTogglesDescriptor, enabledToggleCount) == offsetof(WGPUDawnTogglesDescriptor, enabledToggleCount),
        "offsetof mismatch for DawnTogglesDescriptor::enabledToggleCount");
static_assert(offsetof(DawnTogglesDescriptor, enabledToggles) == offsetof(WGPUDawnTogglesDescriptor, enabledToggles),
        "offsetof mismatch for DawnTogglesDescriptor::enabledToggles");
static_assert(offsetof(DawnTogglesDescriptor, disabledToggleCount) == offsetof(WGPUDawnTogglesDescriptor, disabledToggleCount),
        "offsetof mismatch for DawnTogglesDescriptor::disabledToggleCount");
static_assert(offsetof(DawnTogglesDescriptor, disabledToggles) == offsetof(WGPUDawnTogglesDescriptor, disabledToggles),
        "offsetof mismatch for DawnTogglesDescriptor::disabledToggles");

// DawnWGSLBlocklist implementation
DawnWGSLBlocklist::DawnWGSLBlocklist()
: ChainedStruct { nullptr, SType::DawnWGSLBlocklist } {}
struct DawnWGSLBlocklist::Init {
    ChainedStruct * const nextInChain;
    size_t blocklistedFeatureCount = 0;
    const char* const * blocklistedFeatures = nullptr;
};
DawnWGSLBlocklist::DawnWGSLBlocklist(DawnWGSLBlocklist::Init&& init) :
    ChainedStruct { init.nextInChain, SType::DawnWGSLBlocklist },
    blocklistedFeatureCount(std::move(init.blocklistedFeatureCount)),
    blocklistedFeatures(std::move(init.blocklistedFeatures)) {}

DawnWGSLBlocklist::operator const WGPUDawnWGSLBlocklist&() const noexcept {
    return *reinterpret_cast<const WGPUDawnWGSLBlocklist*>(this);
}

static_assert(sizeof(DawnWGSLBlocklist) == sizeof(WGPUDawnWGSLBlocklist), "sizeof mismatch for DawnWGSLBlocklist");
static_assert(alignof(DawnWGSLBlocklist) == alignof(WGPUDawnWGSLBlocklist), "alignof mismatch for DawnWGSLBlocklist");
static_assert(offsetof(DawnWGSLBlocklist, blocklistedFeatureCount) == offsetof(WGPUDawnWGSLBlocklist, blocklistedFeatureCount),
        "offsetof mismatch for DawnWGSLBlocklist::blocklistedFeatureCount");
static_assert(offsetof(DawnWGSLBlocklist, blocklistedFeatures) == offsetof(WGPUDawnWGSLBlocklist, blocklistedFeatures),
        "offsetof mismatch for DawnWGSLBlocklist::blocklistedFeatures");

// DawnWireWGSLControl implementation
DawnWireWGSLControl::DawnWireWGSLControl()
: ChainedStruct { nullptr, SType::DawnWireWGSLControl } {}
struct DawnWireWGSLControl::Init {
    ChainedStruct * const nextInChain;
    Bool enableExperimental = false;
    Bool enableUnsafe = false;
    Bool enableTesting = false;
};
DawnWireWGSLControl::DawnWireWGSLControl(DawnWireWGSLControl::Init&& init) :
    ChainedStruct { init.nextInChain, SType::DawnWireWGSLControl },
    enableExperimental(std::move(init.enableExperimental)),
    enableUnsafe(std::move(init.enableUnsafe)),
    enableTesting(std::move(init.enableTesting)) {}

DawnWireWGSLControl::operator const WGPUDawnWireWGSLControl&() const noexcept {
    return *reinterpret_cast<const WGPUDawnWireWGSLControl*>(this);
}

static_assert(sizeof(DawnWireWGSLControl) == sizeof(WGPUDawnWireWGSLControl), "sizeof mismatch for DawnWireWGSLControl");
static_assert(alignof(DawnWireWGSLControl) == alignof(WGPUDawnWireWGSLControl), "alignof mismatch for DawnWireWGSLControl");
static_assert(offsetof(DawnWireWGSLControl, enableExperimental) == offsetof(WGPUDawnWireWGSLControl, enableExperimental),
        "offsetof mismatch for DawnWireWGSLControl::enableExperimental");
static_assert(offsetof(DawnWireWGSLControl, enableUnsafe) == offsetof(WGPUDawnWireWGSLControl, enableUnsafe),
        "offsetof mismatch for DawnWireWGSLControl::enableUnsafe");
static_assert(offsetof(DawnWireWGSLControl, enableTesting) == offsetof(WGPUDawnWireWGSLControl, enableTesting),
        "offsetof mismatch for DawnWireWGSLControl::enableTesting");

// EmscriptenSurfaceSourceCanvasHTMLSelector implementation
EmscriptenSurfaceSourceCanvasHTMLSelector::EmscriptenSurfaceSourceCanvasHTMLSelector()
: ChainedStruct { nullptr, SType::EmscriptenSurfaceSourceCanvasHTMLSelector } {}
struct EmscriptenSurfaceSourceCanvasHTMLSelector::Init {
    ChainedStruct * const nextInChain;
    StringView selector = {};
};
EmscriptenSurfaceSourceCanvasHTMLSelector::EmscriptenSurfaceSourceCanvasHTMLSelector(EmscriptenSurfaceSourceCanvasHTMLSelector::Init&& init) :
    ChainedStruct { init.nextInChain, SType::EmscriptenSurfaceSourceCanvasHTMLSelector },
    selector(std::move(init.selector)) {}

EmscriptenSurfaceSourceCanvasHTMLSelector::operator const WGPUEmscriptenSurfaceSourceCanvasHTMLSelector&() const noexcept {
    return *reinterpret_cast<const WGPUEmscriptenSurfaceSourceCanvasHTMLSelector*>(this);
}

static_assert(sizeof(EmscriptenSurfaceSourceCanvasHTMLSelector) == sizeof(WGPUEmscriptenSurfaceSourceCanvasHTMLSelector), "sizeof mismatch for EmscriptenSurfaceSourceCanvasHTMLSelector");
static_assert(alignof(EmscriptenSurfaceSourceCanvasHTMLSelector) == alignof(WGPUEmscriptenSurfaceSourceCanvasHTMLSelector), "alignof mismatch for EmscriptenSurfaceSourceCanvasHTMLSelector");
static_assert(offsetof(EmscriptenSurfaceSourceCanvasHTMLSelector, selector) == offsetof(WGPUEmscriptenSurfaceSourceCanvasHTMLSelector, selector),
        "offsetof mismatch for EmscriptenSurfaceSourceCanvasHTMLSelector::selector");

// Extent2D implementation

Extent2D::operator const WGPUExtent2D&() const noexcept {
    return *reinterpret_cast<const WGPUExtent2D*>(this);
}

static_assert(sizeof(Extent2D) == sizeof(WGPUExtent2D), "sizeof mismatch for Extent2D");
static_assert(alignof(Extent2D) == alignof(WGPUExtent2D), "alignof mismatch for Extent2D");
static_assert(offsetof(Extent2D, width) == offsetof(WGPUExtent2D, width),
        "offsetof mismatch for Extent2D::width");
static_assert(offsetof(Extent2D, height) == offsetof(WGPUExtent2D, height),
        "offsetof mismatch for Extent2D::height");

// Extent3D implementation

Extent3D::operator const WGPUExtent3D&() const noexcept {
    return *reinterpret_cast<const WGPUExtent3D*>(this);
}

static_assert(sizeof(Extent3D) == sizeof(WGPUExtent3D), "sizeof mismatch for Extent3D");
static_assert(alignof(Extent3D) == alignof(WGPUExtent3D), "alignof mismatch for Extent3D");
static_assert(offsetof(Extent3D, width) == offsetof(WGPUExtent3D, width),
        "offsetof mismatch for Extent3D::width");
static_assert(offsetof(Extent3D, height) == offsetof(WGPUExtent3D, height),
        "offsetof mismatch for Extent3D::height");
static_assert(offsetof(Extent3D, depthOrArrayLayers) == offsetof(WGPUExtent3D, depthOrArrayLayers),
        "offsetof mismatch for Extent3D::depthOrArrayLayers");

// ExternalTextureBindingEntry implementation
ExternalTextureBindingEntry::ExternalTextureBindingEntry()
: ChainedStruct { nullptr, SType::ExternalTextureBindingEntry } {}
struct ExternalTextureBindingEntry::Init {
    ChainedStruct * const nextInChain;
    ExternalTexture externalTexture = nullptr;
};
ExternalTextureBindingEntry::ExternalTextureBindingEntry(ExternalTextureBindingEntry::Init&& init) :
    ChainedStruct { init.nextInChain, SType::ExternalTextureBindingEntry },
    externalTexture(std::move(init.externalTexture)) {}

ExternalTextureBindingEntry::operator const WGPUExternalTextureBindingEntry&() const noexcept {
    return *reinterpret_cast<const WGPUExternalTextureBindingEntry*>(this);
}

static_assert(sizeof(ExternalTextureBindingEntry) == sizeof(WGPUExternalTextureBindingEntry), "sizeof mismatch for ExternalTextureBindingEntry");
static_assert(alignof(ExternalTextureBindingEntry) == alignof(WGPUExternalTextureBindingEntry), "alignof mismatch for ExternalTextureBindingEntry");
static_assert(offsetof(ExternalTextureBindingEntry, externalTexture) == offsetof(WGPUExternalTextureBindingEntry, externalTexture),
        "offsetof mismatch for ExternalTextureBindingEntry::externalTexture");

// ExternalTextureBindingLayout implementation
ExternalTextureBindingLayout::ExternalTextureBindingLayout()
: ChainedStruct { nullptr, SType::ExternalTextureBindingLayout } {}
struct ExternalTextureBindingLayout::Init {
    ChainedStruct * const nextInChain;
};
ExternalTextureBindingLayout::ExternalTextureBindingLayout(ExternalTextureBindingLayout::Init&& init) :
    ChainedStruct { init.nextInChain, SType::ExternalTextureBindingLayout } {}

ExternalTextureBindingLayout::operator const WGPUExternalTextureBindingLayout&() const noexcept {
    return *reinterpret_cast<const WGPUExternalTextureBindingLayout*>(this);
}

static_assert(sizeof(ExternalTextureBindingLayout) == sizeof(WGPUExternalTextureBindingLayout), "sizeof mismatch for ExternalTextureBindingLayout");
static_assert(alignof(ExternalTextureBindingLayout) == alignof(WGPUExternalTextureBindingLayout), "alignof mismatch for ExternalTextureBindingLayout");

// Future implementation

Future::operator const WGPUFuture&() const noexcept {
    return *reinterpret_cast<const WGPUFuture*>(this);
}

static_assert(sizeof(Future) == sizeof(WGPUFuture), "sizeof mismatch for Future");
static_assert(alignof(Future) == alignof(WGPUFuture), "alignof mismatch for Future");
static_assert(offsetof(Future, id) == offsetof(WGPUFuture, id),
        "offsetof mismatch for Future::id");

// InstanceLimits implementation

InstanceLimits::operator const WGPUInstanceLimits&() const noexcept {
    return *reinterpret_cast<const WGPUInstanceLimits*>(this);
}

static_assert(sizeof(InstanceLimits) == sizeof(WGPUInstanceLimits), "sizeof mismatch for InstanceLimits");
static_assert(alignof(InstanceLimits) == alignof(WGPUInstanceLimits), "alignof mismatch for InstanceLimits");
static_assert(offsetof(InstanceLimits, nextInChain) == offsetof(WGPUInstanceLimits, nextInChain),
        "offsetof mismatch for InstanceLimits::nextInChain");
static_assert(offsetof(InstanceLimits, timedWaitAnyMaxCount) == offsetof(WGPUInstanceLimits, timedWaitAnyMaxCount),
        "offsetof mismatch for InstanceLimits::timedWaitAnyMaxCount");

// INTERNAL_HAVE_EMDAWNWEBGPU_HEADER implementation

INTERNAL_HAVE_EMDAWNWEBGPU_HEADER::operator const WGPUINTERNAL_HAVE_EMDAWNWEBGPU_HEADER&() const noexcept {
    return *reinterpret_cast<const WGPUINTERNAL_HAVE_EMDAWNWEBGPU_HEADER*>(this);
}

static_assert(sizeof(INTERNAL_HAVE_EMDAWNWEBGPU_HEADER) == sizeof(WGPUINTERNAL_HAVE_EMDAWNWEBGPU_HEADER), "sizeof mismatch for INTERNAL_HAVE_EMDAWNWEBGPU_HEADER");
static_assert(alignof(INTERNAL_HAVE_EMDAWNWEBGPU_HEADER) == alignof(WGPUINTERNAL_HAVE_EMDAWNWEBGPU_HEADER), "alignof mismatch for INTERNAL_HAVE_EMDAWNWEBGPU_HEADER");
static_assert(offsetof(INTERNAL_HAVE_EMDAWNWEBGPU_HEADER, unused) == offsetof(WGPUINTERNAL_HAVE_EMDAWNWEBGPU_HEADER, unused),
        "offsetof mismatch for INTERNAL_HAVE_EMDAWNWEBGPU_HEADER::unused");

// MemoryHeapInfo implementation

MemoryHeapInfo::operator const WGPUMemoryHeapInfo&() const noexcept {
    return *reinterpret_cast<const WGPUMemoryHeapInfo*>(this);
}

static_assert(sizeof(MemoryHeapInfo) == sizeof(WGPUMemoryHeapInfo), "sizeof mismatch for MemoryHeapInfo");
static_assert(alignof(MemoryHeapInfo) == alignof(WGPUMemoryHeapInfo), "alignof mismatch for MemoryHeapInfo");
static_assert(offsetof(MemoryHeapInfo, properties) == offsetof(WGPUMemoryHeapInfo, properties),
        "offsetof mismatch for MemoryHeapInfo::properties");
static_assert(offsetof(MemoryHeapInfo, size) == offsetof(WGPUMemoryHeapInfo, size),
        "offsetof mismatch for MemoryHeapInfo::size");

// MultisampleState implementation

MultisampleState::operator const WGPUMultisampleState&() const noexcept {
    return *reinterpret_cast<const WGPUMultisampleState*>(this);
}

static_assert(sizeof(MultisampleState) == sizeof(WGPUMultisampleState), "sizeof mismatch for MultisampleState");
static_assert(alignof(MultisampleState) == alignof(WGPUMultisampleState), "alignof mismatch for MultisampleState");
static_assert(offsetof(MultisampleState, nextInChain) == offsetof(WGPUMultisampleState, nextInChain),
        "offsetof mismatch for MultisampleState::nextInChain");
static_assert(offsetof(MultisampleState, count) == offsetof(WGPUMultisampleState, count),
        "offsetof mismatch for MultisampleState::count");
static_assert(offsetof(MultisampleState, mask) == offsetof(WGPUMultisampleState, mask),
        "offsetof mismatch for MultisampleState::mask");
static_assert(offsetof(MultisampleState, alphaToCoverageEnabled) == offsetof(WGPUMultisampleState, alphaToCoverageEnabled),
        "offsetof mismatch for MultisampleState::alphaToCoverageEnabled");

// Origin2D implementation

Origin2D::operator const WGPUOrigin2D&() const noexcept {
    return *reinterpret_cast<const WGPUOrigin2D*>(this);
}

static_assert(sizeof(Origin2D) == sizeof(WGPUOrigin2D), "sizeof mismatch for Origin2D");
static_assert(alignof(Origin2D) == alignof(WGPUOrigin2D), "alignof mismatch for Origin2D");
static_assert(offsetof(Origin2D, x) == offsetof(WGPUOrigin2D, x),
        "offsetof mismatch for Origin2D::x");
static_assert(offsetof(Origin2D, y) == offsetof(WGPUOrigin2D, y),
        "offsetof mismatch for Origin2D::y");

// Origin3D implementation

Origin3D::operator const WGPUOrigin3D&() const noexcept {
    return *reinterpret_cast<const WGPUOrigin3D*>(this);
}

static_assert(sizeof(Origin3D) == sizeof(WGPUOrigin3D), "sizeof mismatch for Origin3D");
static_assert(alignof(Origin3D) == alignof(WGPUOrigin3D), "alignof mismatch for Origin3D");
static_assert(offsetof(Origin3D, x) == offsetof(WGPUOrigin3D, x),
        "offsetof mismatch for Origin3D::x");
static_assert(offsetof(Origin3D, y) == offsetof(WGPUOrigin3D, y),
        "offsetof mismatch for Origin3D::y");
static_assert(offsetof(Origin3D, z) == offsetof(WGPUOrigin3D, z),
        "offsetof mismatch for Origin3D::z");

// PassTimestampWrites implementation

PassTimestampWrites::operator const WGPUPassTimestampWrites&() const noexcept {
    return *reinterpret_cast<const WGPUPassTimestampWrites*>(this);
}

static_assert(sizeof(PassTimestampWrites) == sizeof(WGPUPassTimestampWrites), "sizeof mismatch for PassTimestampWrites");
static_assert(alignof(PassTimestampWrites) == alignof(WGPUPassTimestampWrites), "alignof mismatch for PassTimestampWrites");
static_assert(offsetof(PassTimestampWrites, nextInChain) == offsetof(WGPUPassTimestampWrites, nextInChain),
        "offsetof mismatch for PassTimestampWrites::nextInChain");
static_assert(offsetof(PassTimestampWrites, querySet) == offsetof(WGPUPassTimestampWrites, querySet),
        "offsetof mismatch for PassTimestampWrites::querySet");
static_assert(offsetof(PassTimestampWrites, beginningOfPassWriteIndex) == offsetof(WGPUPassTimestampWrites, beginningOfPassWriteIndex),
        "offsetof mismatch for PassTimestampWrites::beginningOfPassWriteIndex");
static_assert(offsetof(PassTimestampWrites, endOfPassWriteIndex) == offsetof(WGPUPassTimestampWrites, endOfPassWriteIndex),
        "offsetof mismatch for PassTimestampWrites::endOfPassWriteIndex");

// PipelineLayoutResourceTable implementation
PipelineLayoutResourceTable::PipelineLayoutResourceTable()
: ChainedStruct { nullptr, SType::PipelineLayoutResourceTable } {}
struct PipelineLayoutResourceTable::Init {
    ChainedStruct * const nextInChain;
    Bool usesResourceTable = false;
};
PipelineLayoutResourceTable::PipelineLayoutResourceTable(PipelineLayoutResourceTable::Init&& init) :
    ChainedStruct { init.nextInChain, SType::PipelineLayoutResourceTable },
    usesResourceTable(std::move(init.usesResourceTable)) {}

PipelineLayoutResourceTable::operator const WGPUPipelineLayoutResourceTable&() const noexcept {
    return *reinterpret_cast<const WGPUPipelineLayoutResourceTable*>(this);
}

static_assert(sizeof(PipelineLayoutResourceTable) == sizeof(WGPUPipelineLayoutResourceTable), "sizeof mismatch for PipelineLayoutResourceTable");
static_assert(alignof(PipelineLayoutResourceTable) == alignof(WGPUPipelineLayoutResourceTable), "alignof mismatch for PipelineLayoutResourceTable");
static_assert(offsetof(PipelineLayoutResourceTable, usesResourceTable) == offsetof(WGPUPipelineLayoutResourceTable, usesResourceTable),
        "offsetof mismatch for PipelineLayoutResourceTable::usesResourceTable");

// PipelineLayoutStorageAttachment implementation

PipelineLayoutStorageAttachment::operator const WGPUPipelineLayoutStorageAttachment&() const noexcept {
    return *reinterpret_cast<const WGPUPipelineLayoutStorageAttachment*>(this);
}

static_assert(sizeof(PipelineLayoutStorageAttachment) == sizeof(WGPUPipelineLayoutStorageAttachment), "sizeof mismatch for PipelineLayoutStorageAttachment");
static_assert(alignof(PipelineLayoutStorageAttachment) == alignof(WGPUPipelineLayoutStorageAttachment), "alignof mismatch for PipelineLayoutStorageAttachment");
static_assert(offsetof(PipelineLayoutStorageAttachment, nextInChain) == offsetof(WGPUPipelineLayoutStorageAttachment, nextInChain),
        "offsetof mismatch for PipelineLayoutStorageAttachment::nextInChain");
static_assert(offsetof(PipelineLayoutStorageAttachment, offset) == offsetof(WGPUPipelineLayoutStorageAttachment, offset),
        "offsetof mismatch for PipelineLayoutStorageAttachment::offset");
static_assert(offsetof(PipelineLayoutStorageAttachment, format) == offsetof(WGPUPipelineLayoutStorageAttachment, format),
        "offsetof mismatch for PipelineLayoutStorageAttachment::format");

// PrimitiveState implementation

PrimitiveState::operator const WGPUPrimitiveState&() const noexcept {
    return *reinterpret_cast<const WGPUPrimitiveState*>(this);
}

static_assert(sizeof(PrimitiveState) == sizeof(WGPUPrimitiveState), "sizeof mismatch for PrimitiveState");
static_assert(alignof(PrimitiveState) == alignof(WGPUPrimitiveState), "alignof mismatch for PrimitiveState");
static_assert(offsetof(PrimitiveState, nextInChain) == offsetof(WGPUPrimitiveState, nextInChain),
        "offsetof mismatch for PrimitiveState::nextInChain");
static_assert(offsetof(PrimitiveState, topology) == offsetof(WGPUPrimitiveState, topology),
        "offsetof mismatch for PrimitiveState::topology");
static_assert(offsetof(PrimitiveState, stripIndexFormat) == offsetof(WGPUPrimitiveState, stripIndexFormat),
        "offsetof mismatch for PrimitiveState::stripIndexFormat");
static_assert(offsetof(PrimitiveState, frontFace) == offsetof(WGPUPrimitiveState, frontFace),
        "offsetof mismatch for PrimitiveState::frontFace");
static_assert(offsetof(PrimitiveState, cullMode) == offsetof(WGPUPrimitiveState, cullMode),
        "offsetof mismatch for PrimitiveState::cullMode");
static_assert(offsetof(PrimitiveState, unclippedDepth) == offsetof(WGPUPrimitiveState, unclippedDepth),
        "offsetof mismatch for PrimitiveState::unclippedDepth");

// QuerySetDescriptor implementation

QuerySetDescriptor::operator const WGPUQuerySetDescriptor&() const noexcept {
    return *reinterpret_cast<const WGPUQuerySetDescriptor*>(this);
}

static_assert(sizeof(QuerySetDescriptor) == sizeof(WGPUQuerySetDescriptor), "sizeof mismatch for QuerySetDescriptor");
static_assert(alignof(QuerySetDescriptor) == alignof(WGPUQuerySetDescriptor), "alignof mismatch for QuerySetDescriptor");
static_assert(offsetof(QuerySetDescriptor, nextInChain) == offsetof(WGPUQuerySetDescriptor, nextInChain),
        "offsetof mismatch for QuerySetDescriptor::nextInChain");
static_assert(offsetof(QuerySetDescriptor, label) == offsetof(WGPUQuerySetDescriptor, label),
        "offsetof mismatch for QuerySetDescriptor::label");
static_assert(offsetof(QuerySetDescriptor, type) == offsetof(WGPUQuerySetDescriptor, type),
        "offsetof mismatch for QuerySetDescriptor::type");
static_assert(offsetof(QuerySetDescriptor, count) == offsetof(WGPUQuerySetDescriptor, count),
        "offsetof mismatch for QuerySetDescriptor::count");

// QueueDescriptor implementation

QueueDescriptor::operator const WGPUQueueDescriptor&() const noexcept {
    return *reinterpret_cast<const WGPUQueueDescriptor*>(this);
}

static_assert(sizeof(QueueDescriptor) == sizeof(WGPUQueueDescriptor), "sizeof mismatch for QueueDescriptor");
static_assert(alignof(QueueDescriptor) == alignof(WGPUQueueDescriptor), "alignof mismatch for QueueDescriptor");
static_assert(offsetof(QueueDescriptor, nextInChain) == offsetof(WGPUQueueDescriptor, nextInChain),
        "offsetof mismatch for QueueDescriptor::nextInChain");
static_assert(offsetof(QueueDescriptor, label) == offsetof(WGPUQueueDescriptor, label),
        "offsetof mismatch for QueueDescriptor::label");

// RenderBundleDescriptor implementation

RenderBundleDescriptor::operator const WGPURenderBundleDescriptor&() const noexcept {
    return *reinterpret_cast<const WGPURenderBundleDescriptor*>(this);
}

static_assert(sizeof(RenderBundleDescriptor) == sizeof(WGPURenderBundleDescriptor), "sizeof mismatch for RenderBundleDescriptor");
static_assert(alignof(RenderBundleDescriptor) == alignof(WGPURenderBundleDescriptor), "alignof mismatch for RenderBundleDescriptor");
static_assert(offsetof(RenderBundleDescriptor, nextInChain) == offsetof(WGPURenderBundleDescriptor, nextInChain),
        "offsetof mismatch for RenderBundleDescriptor::nextInChain");
static_assert(offsetof(RenderBundleDescriptor, label) == offsetof(WGPURenderBundleDescriptor, label),
        "offsetof mismatch for RenderBundleDescriptor::label");

// RenderBundleEncoderDescriptor implementation

RenderBundleEncoderDescriptor::operator const WGPURenderBundleEncoderDescriptor&() const noexcept {
    return *reinterpret_cast<const WGPURenderBundleEncoderDescriptor*>(this);
}

static_assert(sizeof(RenderBundleEncoderDescriptor) == sizeof(WGPURenderBundleEncoderDescriptor), "sizeof mismatch for RenderBundleEncoderDescriptor");
static_assert(alignof(RenderBundleEncoderDescriptor) == alignof(WGPURenderBundleEncoderDescriptor), "alignof mismatch for RenderBundleEncoderDescriptor");
static_assert(offsetof(RenderBundleEncoderDescriptor, nextInChain) == offsetof(WGPURenderBundleEncoderDescriptor, nextInChain),
        "offsetof mismatch for RenderBundleEncoderDescriptor::nextInChain");
static_assert(offsetof(RenderBundleEncoderDescriptor, label) == offsetof(WGPURenderBundleEncoderDescriptor, label),
        "offsetof mismatch for RenderBundleEncoderDescriptor::label");
static_assert(offsetof(RenderBundleEncoderDescriptor, colorFormatCount) == offsetof(WGPURenderBundleEncoderDescriptor, colorFormatCount),
        "offsetof mismatch for RenderBundleEncoderDescriptor::colorFormatCount");
static_assert(offsetof(RenderBundleEncoderDescriptor, colorFormats) == offsetof(WGPURenderBundleEncoderDescriptor, colorFormats),
        "offsetof mismatch for RenderBundleEncoderDescriptor::colorFormats");
static_assert(offsetof(RenderBundleEncoderDescriptor, depthStencilFormat) == offsetof(WGPURenderBundleEncoderDescriptor, depthStencilFormat),
        "offsetof mismatch for RenderBundleEncoderDescriptor::depthStencilFormat");
static_assert(offsetof(RenderBundleEncoderDescriptor, sampleCount) == offsetof(WGPURenderBundleEncoderDescriptor, sampleCount),
        "offsetof mismatch for RenderBundleEncoderDescriptor::sampleCount");
static_assert(offsetof(RenderBundleEncoderDescriptor, depthReadOnly) == offsetof(WGPURenderBundleEncoderDescriptor, depthReadOnly),
        "offsetof mismatch for RenderBundleEncoderDescriptor::depthReadOnly");
static_assert(offsetof(RenderBundleEncoderDescriptor, stencilReadOnly) == offsetof(WGPURenderBundleEncoderDescriptor, stencilReadOnly),
        "offsetof mismatch for RenderBundleEncoderDescriptor::stencilReadOnly");

// RenderPassDepthStencilAttachment implementation

RenderPassDepthStencilAttachment::operator const WGPURenderPassDepthStencilAttachment&() const noexcept {
    return *reinterpret_cast<const WGPURenderPassDepthStencilAttachment*>(this);
}

static_assert(sizeof(RenderPassDepthStencilAttachment) == sizeof(WGPURenderPassDepthStencilAttachment), "sizeof mismatch for RenderPassDepthStencilAttachment");
static_assert(alignof(RenderPassDepthStencilAttachment) == alignof(WGPURenderPassDepthStencilAttachment), "alignof mismatch for RenderPassDepthStencilAttachment");
static_assert(offsetof(RenderPassDepthStencilAttachment, nextInChain) == offsetof(WGPURenderPassDepthStencilAttachment, nextInChain),
        "offsetof mismatch for RenderPassDepthStencilAttachment::nextInChain");
static_assert(offsetof(RenderPassDepthStencilAttachment, view) == offsetof(WGPURenderPassDepthStencilAttachment, view),
        "offsetof mismatch for RenderPassDepthStencilAttachment::view");
static_assert(offsetof(RenderPassDepthStencilAttachment, depthLoadOp) == offsetof(WGPURenderPassDepthStencilAttachment, depthLoadOp),
        "offsetof mismatch for RenderPassDepthStencilAttachment::depthLoadOp");
static_assert(offsetof(RenderPassDepthStencilAttachment, depthStoreOp) == offsetof(WGPURenderPassDepthStencilAttachment, depthStoreOp),
        "offsetof mismatch for RenderPassDepthStencilAttachment::depthStoreOp");
static_assert(offsetof(RenderPassDepthStencilAttachment, depthClearValue) == offsetof(WGPURenderPassDepthStencilAttachment, depthClearValue),
        "offsetof mismatch for RenderPassDepthStencilAttachment::depthClearValue");
static_assert(offsetof(RenderPassDepthStencilAttachment, depthReadOnly) == offsetof(WGPURenderPassDepthStencilAttachment, depthReadOnly),
        "offsetof mismatch for RenderPassDepthStencilAttachment::depthReadOnly");
static_assert(offsetof(RenderPassDepthStencilAttachment, stencilLoadOp) == offsetof(WGPURenderPassDepthStencilAttachment, stencilLoadOp),
        "offsetof mismatch for RenderPassDepthStencilAttachment::stencilLoadOp");
static_assert(offsetof(RenderPassDepthStencilAttachment, stencilStoreOp) == offsetof(WGPURenderPassDepthStencilAttachment, stencilStoreOp),
        "offsetof mismatch for RenderPassDepthStencilAttachment::stencilStoreOp");
static_assert(offsetof(RenderPassDepthStencilAttachment, stencilClearValue) == offsetof(WGPURenderPassDepthStencilAttachment, stencilClearValue),
        "offsetof mismatch for RenderPassDepthStencilAttachment::stencilClearValue");
static_assert(offsetof(RenderPassDepthStencilAttachment, stencilReadOnly) == offsetof(WGPURenderPassDepthStencilAttachment, stencilReadOnly),
        "offsetof mismatch for RenderPassDepthStencilAttachment::stencilReadOnly");

// RenderPassDescriptorResolveRect implementation
RenderPassDescriptorResolveRect::RenderPassDescriptorResolveRect()
: ChainedStruct { nullptr, SType::RenderPassDescriptorResolveRect } {}
struct RenderPassDescriptorResolveRect::Init {
    ChainedStruct * const nextInChain;
    uint32_t colorOffsetX;
    uint32_t colorOffsetY;
    uint32_t resolveOffsetX;
    uint32_t resolveOffsetY;
    uint32_t width;
    uint32_t height;
};
RenderPassDescriptorResolveRect::RenderPassDescriptorResolveRect(RenderPassDescriptorResolveRect::Init&& init) :
    ChainedStruct { init.nextInChain, SType::RenderPassDescriptorResolveRect },
    colorOffsetX(std::move(init.colorOffsetX)),
    colorOffsetY(std::move(init.colorOffsetY)),
    resolveOffsetX(std::move(init.resolveOffsetX)),
    resolveOffsetY(std::move(init.resolveOffsetY)),
    width(std::move(init.width)),
    height(std::move(init.height)) {}

RenderPassDescriptorResolveRect::operator const WGPURenderPassDescriptorResolveRect&() const noexcept {
    return *reinterpret_cast<const WGPURenderPassDescriptorResolveRect*>(this);
}

static_assert(sizeof(RenderPassDescriptorResolveRect) == sizeof(WGPURenderPassDescriptorResolveRect), "sizeof mismatch for RenderPassDescriptorResolveRect");
static_assert(alignof(RenderPassDescriptorResolveRect) == alignof(WGPURenderPassDescriptorResolveRect), "alignof mismatch for RenderPassDescriptorResolveRect");
static_assert(offsetof(RenderPassDescriptorResolveRect, colorOffsetX) == offsetof(WGPURenderPassDescriptorResolveRect, colorOffsetX),
        "offsetof mismatch for RenderPassDescriptorResolveRect::colorOffsetX");
static_assert(offsetof(RenderPassDescriptorResolveRect, colorOffsetY) == offsetof(WGPURenderPassDescriptorResolveRect, colorOffsetY),
        "offsetof mismatch for RenderPassDescriptorResolveRect::colorOffsetY");
static_assert(offsetof(RenderPassDescriptorResolveRect, resolveOffsetX) == offsetof(WGPURenderPassDescriptorResolveRect, resolveOffsetX),
        "offsetof mismatch for RenderPassDescriptorResolveRect::resolveOffsetX");
static_assert(offsetof(RenderPassDescriptorResolveRect, resolveOffsetY) == offsetof(WGPURenderPassDescriptorResolveRect, resolveOffsetY),
        "offsetof mismatch for RenderPassDescriptorResolveRect::resolveOffsetY");
static_assert(offsetof(RenderPassDescriptorResolveRect, width) == offsetof(WGPURenderPassDescriptorResolveRect, width),
        "offsetof mismatch for RenderPassDescriptorResolveRect::width");
static_assert(offsetof(RenderPassDescriptorResolveRect, height) == offsetof(WGPURenderPassDescriptorResolveRect, height),
        "offsetof mismatch for RenderPassDescriptorResolveRect::height");

// RenderPassMaxDrawCount implementation
RenderPassMaxDrawCount::RenderPassMaxDrawCount()
: ChainedStruct { nullptr, SType::RenderPassMaxDrawCount } {}
struct RenderPassMaxDrawCount::Init {
    ChainedStruct * const nextInChain;
    uint64_t maxDrawCount = 50000000;
};
RenderPassMaxDrawCount::RenderPassMaxDrawCount(RenderPassMaxDrawCount::Init&& init) :
    ChainedStruct { init.nextInChain, SType::RenderPassMaxDrawCount },
    maxDrawCount(std::move(init.maxDrawCount)) {}

RenderPassMaxDrawCount::operator const WGPURenderPassMaxDrawCount&() const noexcept {
    return *reinterpret_cast<const WGPURenderPassMaxDrawCount*>(this);
}

static_assert(sizeof(RenderPassMaxDrawCount) == sizeof(WGPURenderPassMaxDrawCount), "sizeof mismatch for RenderPassMaxDrawCount");
static_assert(alignof(RenderPassMaxDrawCount) == alignof(WGPURenderPassMaxDrawCount), "alignof mismatch for RenderPassMaxDrawCount");
static_assert(offsetof(RenderPassMaxDrawCount, maxDrawCount) == offsetof(WGPURenderPassMaxDrawCount, maxDrawCount),
        "offsetof mismatch for RenderPassMaxDrawCount::maxDrawCount");

// RequestAdapterWebGPUBackendOptions implementation
RequestAdapterWebGPUBackendOptions::RequestAdapterWebGPUBackendOptions()
: ChainedStruct { nullptr, SType::RequestAdapterWebGPUBackendOptions } {}
struct RequestAdapterWebGPUBackendOptions::Init {
    ChainedStruct * const nextInChain;
};
RequestAdapterWebGPUBackendOptions::RequestAdapterWebGPUBackendOptions(RequestAdapterWebGPUBackendOptions::Init&& init) :
    ChainedStruct { init.nextInChain, SType::RequestAdapterWebGPUBackendOptions } {}

RequestAdapterWebGPUBackendOptions::operator const WGPURequestAdapterWebGPUBackendOptions&() const noexcept {
    return *reinterpret_cast<const WGPURequestAdapterWebGPUBackendOptions*>(this);
}

static_assert(sizeof(RequestAdapterWebGPUBackendOptions) == sizeof(WGPURequestAdapterWebGPUBackendOptions), "sizeof mismatch for RequestAdapterWebGPUBackendOptions");
static_assert(alignof(RequestAdapterWebGPUBackendOptions) == alignof(WGPURequestAdapterWebGPUBackendOptions), "alignof mismatch for RequestAdapterWebGPUBackendOptions");

// RequestAdapterWebXROptions implementation
RequestAdapterWebXROptions::RequestAdapterWebXROptions()
: ChainedStruct { nullptr, SType::RequestAdapterWebXROptions } {}
struct RequestAdapterWebXROptions::Init {
    ChainedStruct * const nextInChain;
    Bool xrCompatible;
};
RequestAdapterWebXROptions::RequestAdapterWebXROptions(RequestAdapterWebXROptions::Init&& init) :
    ChainedStruct { init.nextInChain, SType::RequestAdapterWebXROptions },
    xrCompatible(std::move(init.xrCompatible)) {}

RequestAdapterWebXROptions::operator const WGPURequestAdapterWebXROptions&() const noexcept {
    return *reinterpret_cast<const WGPURequestAdapterWebXROptions*>(this);
}

static_assert(sizeof(RequestAdapterWebXROptions) == sizeof(WGPURequestAdapterWebXROptions), "sizeof mismatch for RequestAdapterWebXROptions");
static_assert(alignof(RequestAdapterWebXROptions) == alignof(WGPURequestAdapterWebXROptions), "alignof mismatch for RequestAdapterWebXROptions");
static_assert(offsetof(RequestAdapterWebXROptions, xrCompatible) == offsetof(WGPURequestAdapterWebXROptions, xrCompatible),
        "offsetof mismatch for RequestAdapterWebXROptions::xrCompatible");

// ResourceTableDescriptor implementation

ResourceTableDescriptor::operator const WGPUResourceTableDescriptor&() const noexcept {
    return *reinterpret_cast<const WGPUResourceTableDescriptor*>(this);
}

static_assert(sizeof(ResourceTableDescriptor) == sizeof(WGPUResourceTableDescriptor), "sizeof mismatch for ResourceTableDescriptor");
static_assert(alignof(ResourceTableDescriptor) == alignof(WGPUResourceTableDescriptor), "alignof mismatch for ResourceTableDescriptor");
static_assert(offsetof(ResourceTableDescriptor, nextInChain) == offsetof(WGPUResourceTableDescriptor, nextInChain),
        "offsetof mismatch for ResourceTableDescriptor::nextInChain");
static_assert(offsetof(ResourceTableDescriptor, label) == offsetof(WGPUResourceTableDescriptor, label),
        "offsetof mismatch for ResourceTableDescriptor::label");
static_assert(offsetof(ResourceTableDescriptor, size) == offsetof(WGPUResourceTableDescriptor, size),
        "offsetof mismatch for ResourceTableDescriptor::size");

// SamplerBindingLayout implementation

SamplerBindingLayout::operator const WGPUSamplerBindingLayout&() const noexcept {
    return *reinterpret_cast<const WGPUSamplerBindingLayout*>(this);
}

static_assert(sizeof(SamplerBindingLayout) == sizeof(WGPUSamplerBindingLayout), "sizeof mismatch for SamplerBindingLayout");
static_assert(alignof(SamplerBindingLayout) == alignof(WGPUSamplerBindingLayout), "alignof mismatch for SamplerBindingLayout");
static_assert(offsetof(SamplerBindingLayout, nextInChain) == offsetof(WGPUSamplerBindingLayout, nextInChain),
        "offsetof mismatch for SamplerBindingLayout::nextInChain");
static_assert(offsetof(SamplerBindingLayout, type) == offsetof(WGPUSamplerBindingLayout, type),
        "offsetof mismatch for SamplerBindingLayout::type");

// ShaderModuleCompilationOptions implementation
ShaderModuleCompilationOptions::ShaderModuleCompilationOptions()
: ChainedStruct { nullptr, SType::ShaderModuleCompilationOptions } {}
struct ShaderModuleCompilationOptions::Init {
    ChainedStruct * const nextInChain;
    Bool strictMath;
};
ShaderModuleCompilationOptions::ShaderModuleCompilationOptions(ShaderModuleCompilationOptions::Init&& init) :
    ChainedStruct { init.nextInChain, SType::ShaderModuleCompilationOptions },
    strictMath(std::move(init.strictMath)) {}

ShaderModuleCompilationOptions::operator const WGPUShaderModuleCompilationOptions&() const noexcept {
    return *reinterpret_cast<const WGPUShaderModuleCompilationOptions*>(this);
}

static_assert(sizeof(ShaderModuleCompilationOptions) == sizeof(WGPUShaderModuleCompilationOptions), "sizeof mismatch for ShaderModuleCompilationOptions");
static_assert(alignof(ShaderModuleCompilationOptions) == alignof(WGPUShaderModuleCompilationOptions), "alignof mismatch for ShaderModuleCompilationOptions");
static_assert(offsetof(ShaderModuleCompilationOptions, strictMath) == offsetof(WGPUShaderModuleCompilationOptions, strictMath),
        "offsetof mismatch for ShaderModuleCompilationOptions::strictMath");

// ShaderSourceSPIRV implementation
ShaderSourceSPIRV::ShaderSourceSPIRV()
: ChainedStruct { nullptr, SType::ShaderSourceSPIRV } {}
struct ShaderSourceSPIRV::Init {
    ChainedStruct * const nextInChain;
    uint32_t codeSize;
    uint32_t const * code = nullptr;
};
ShaderSourceSPIRV::ShaderSourceSPIRV(ShaderSourceSPIRV::Init&& init) :
    ChainedStruct { init.nextInChain, SType::ShaderSourceSPIRV },
    codeSize(std::move(init.codeSize)),
    code(std::move(init.code)) {}

ShaderSourceSPIRV::operator const WGPUShaderSourceSPIRV&() const noexcept {
    return *reinterpret_cast<const WGPUShaderSourceSPIRV*>(this);
}

static_assert(sizeof(ShaderSourceSPIRV) == sizeof(WGPUShaderSourceSPIRV), "sizeof mismatch for ShaderSourceSPIRV");
static_assert(alignof(ShaderSourceSPIRV) == alignof(WGPUShaderSourceSPIRV), "alignof mismatch for ShaderSourceSPIRV");
static_assert(offsetof(ShaderSourceSPIRV, codeSize) == offsetof(WGPUShaderSourceSPIRV, codeSize),
        "offsetof mismatch for ShaderSourceSPIRV::codeSize");
static_assert(offsetof(ShaderSourceSPIRV, code) == offsetof(WGPUShaderSourceSPIRV, code),
        "offsetof mismatch for ShaderSourceSPIRV::code");

// ShaderSourceWGSL implementation
ShaderSourceWGSL::ShaderSourceWGSL()
: ChainedStruct { nullptr, SType::ShaderSourceWGSL } {}
struct ShaderSourceWGSL::Init {
    ChainedStruct * const nextInChain;
    StringView code = {};
};
ShaderSourceWGSL::ShaderSourceWGSL(ShaderSourceWGSL::Init&& init) :
    ChainedStruct { init.nextInChain, SType::ShaderSourceWGSL },
    code(std::move(init.code)) {}

ShaderSourceWGSL::operator const WGPUShaderSourceWGSL&() const noexcept {
    return *reinterpret_cast<const WGPUShaderSourceWGSL*>(this);
}

static_assert(sizeof(ShaderSourceWGSL) == sizeof(WGPUShaderSourceWGSL), "sizeof mismatch for ShaderSourceWGSL");
static_assert(alignof(ShaderSourceWGSL) == alignof(WGPUShaderSourceWGSL), "alignof mismatch for ShaderSourceWGSL");
static_assert(offsetof(ShaderSourceWGSL, code) == offsetof(WGPUShaderSourceWGSL, code),
        "offsetof mismatch for ShaderSourceWGSL::code");

// SharedBufferMemoryBeginAccessDescriptor implementation

SharedBufferMemoryBeginAccessDescriptor::operator const WGPUSharedBufferMemoryBeginAccessDescriptor&() const noexcept {
    return *reinterpret_cast<const WGPUSharedBufferMemoryBeginAccessDescriptor*>(this);
}

static_assert(sizeof(SharedBufferMemoryBeginAccessDescriptor) == sizeof(WGPUSharedBufferMemoryBeginAccessDescriptor), "sizeof mismatch for SharedBufferMemoryBeginAccessDescriptor");
static_assert(alignof(SharedBufferMemoryBeginAccessDescriptor) == alignof(WGPUSharedBufferMemoryBeginAccessDescriptor), "alignof mismatch for SharedBufferMemoryBeginAccessDescriptor");
static_assert(offsetof(SharedBufferMemoryBeginAccessDescriptor, nextInChain) == offsetof(WGPUSharedBufferMemoryBeginAccessDescriptor, nextInChain),
        "offsetof mismatch for SharedBufferMemoryBeginAccessDescriptor::nextInChain");
static_assert(offsetof(SharedBufferMemoryBeginAccessDescriptor, initialized) == offsetof(WGPUSharedBufferMemoryBeginAccessDescriptor, initialized),
        "offsetof mismatch for SharedBufferMemoryBeginAccessDescriptor::initialized");
static_assert(offsetof(SharedBufferMemoryBeginAccessDescriptor, fenceCount) == offsetof(WGPUSharedBufferMemoryBeginAccessDescriptor, fenceCount),
        "offsetof mismatch for SharedBufferMemoryBeginAccessDescriptor::fenceCount");
static_assert(offsetof(SharedBufferMemoryBeginAccessDescriptor, fences) == offsetof(WGPUSharedBufferMemoryBeginAccessDescriptor, fences),
        "offsetof mismatch for SharedBufferMemoryBeginAccessDescriptor::fences");
static_assert(offsetof(SharedBufferMemoryBeginAccessDescriptor, signaledValueCount) == offsetof(WGPUSharedBufferMemoryBeginAccessDescriptor, signaledValueCount),
        "offsetof mismatch for SharedBufferMemoryBeginAccessDescriptor::signaledValueCount");
static_assert(offsetof(SharedBufferMemoryBeginAccessDescriptor, signaledValues) == offsetof(WGPUSharedBufferMemoryBeginAccessDescriptor, signaledValues),
        "offsetof mismatch for SharedBufferMemoryBeginAccessDescriptor::signaledValues");

// SharedBufferMemoryEndAccessState implementation
SharedBufferMemoryEndAccessState::SharedBufferMemoryEndAccessState() = default;
SharedBufferMemoryEndAccessState::~SharedBufferMemoryEndAccessState() {
    FreeMembers();
}

SharedBufferMemoryEndAccessState::SharedBufferMemoryEndAccessState(SharedBufferMemoryEndAccessState&& rhs)
    : initialized(rhs.initialized),
            fenceCount(rhs.fenceCount),
            fences(rhs.fences),
            signaledValueCount(rhs.signaledValueCount),
            signaledValues(rhs.signaledValues){
    Reset(rhs);
}

SharedBufferMemoryEndAccessState& SharedBufferMemoryEndAccessState::operator=(SharedBufferMemoryEndAccessState&& rhs) {
    if (&rhs == this) {
        return *this;
    }
    FreeMembers();
    detail::AsNonConstReference(this->initialized) = std::move(rhs.initialized);
    detail::AsNonConstReference(this->fenceCount) = std::move(rhs.fenceCount);
    detail::AsNonConstReference(this->fences) = std::move(rhs.fences);
    detail::AsNonConstReference(this->signaledValueCount) = std::move(rhs.signaledValueCount);
    detail::AsNonConstReference(this->signaledValues) = std::move(rhs.signaledValues);
    Reset(rhs);
    return *this;
}

void SharedBufferMemoryEndAccessState::FreeMembers() {
    bool needsFreeing = false;    if (this->fences != nullptr) { needsFreeing = true; }    if (this->signaledValues != nullptr) { needsFreeing = true; }if (needsFreeing) {
        wgpuSharedBufferMemoryEndAccessStateFreeMembers(
            *reinterpret_cast<WGPUSharedBufferMemoryEndAccessState*>(this));
    }
}

// static
void SharedBufferMemoryEndAccessState::Reset(SharedBufferMemoryEndAccessState& value) {
    SharedBufferMemoryEndAccessState defaultValue{};
    detail::AsNonConstReference(value.initialized) = defaultValue.initialized;
    detail::AsNonConstReference(value.fenceCount) = defaultValue.fenceCount;
    detail::AsNonConstReference(value.fences) = defaultValue.fences;
    detail::AsNonConstReference(value.signaledValueCount) = defaultValue.signaledValueCount;
    detail::AsNonConstReference(value.signaledValues) = defaultValue.signaledValues;
}

SharedBufferMemoryEndAccessState::operator const WGPUSharedBufferMemoryEndAccessState&() const noexcept {
    return *reinterpret_cast<const WGPUSharedBufferMemoryEndAccessState*>(this);
}

static_assert(sizeof(SharedBufferMemoryEndAccessState) == sizeof(WGPUSharedBufferMemoryEndAccessState), "sizeof mismatch for SharedBufferMemoryEndAccessState");
static_assert(alignof(SharedBufferMemoryEndAccessState) == alignof(WGPUSharedBufferMemoryEndAccessState), "alignof mismatch for SharedBufferMemoryEndAccessState");
static_assert(offsetof(SharedBufferMemoryEndAccessState, nextInChain) == offsetof(WGPUSharedBufferMemoryEndAccessState, nextInChain),
        "offsetof mismatch for SharedBufferMemoryEndAccessState::nextInChain");
static_assert(offsetof(SharedBufferMemoryEndAccessState, initialized) == offsetof(WGPUSharedBufferMemoryEndAccessState, initialized),
        "offsetof mismatch for SharedBufferMemoryEndAccessState::initialized");
static_assert(offsetof(SharedBufferMemoryEndAccessState, fenceCount) == offsetof(WGPUSharedBufferMemoryEndAccessState, fenceCount),
        "offsetof mismatch for SharedBufferMemoryEndAccessState::fenceCount");
static_assert(offsetof(SharedBufferMemoryEndAccessState, fences) == offsetof(WGPUSharedBufferMemoryEndAccessState, fences),
        "offsetof mismatch for SharedBufferMemoryEndAccessState::fences");
static_assert(offsetof(SharedBufferMemoryEndAccessState, signaledValueCount) == offsetof(WGPUSharedBufferMemoryEndAccessState, signaledValueCount),
        "offsetof mismatch for SharedBufferMemoryEndAccessState::signaledValueCount");
static_assert(offsetof(SharedBufferMemoryEndAccessState, signaledValues) == offsetof(WGPUSharedBufferMemoryEndAccessState, signaledValues),
        "offsetof mismatch for SharedBufferMemoryEndAccessState::signaledValues");

// SharedBufferMemoryFromWindowsHandleDescriptor implementation
SharedBufferMemoryFromWindowsHandleDescriptor::SharedBufferMemoryFromWindowsHandleDescriptor()
: ChainedStruct { nullptr, SType::SharedBufferMemoryFromWindowsHandleDescriptor } {}
struct SharedBufferMemoryFromWindowsHandleDescriptor::Init {
    ChainedStruct * const nextInChain;
    void * handle;
    uint64_t size;
};
SharedBufferMemoryFromWindowsHandleDescriptor::SharedBufferMemoryFromWindowsHandleDescriptor(SharedBufferMemoryFromWindowsHandleDescriptor::Init&& init) :
    ChainedStruct { init.nextInChain, SType::SharedBufferMemoryFromWindowsHandleDescriptor },
    handle(std::move(init.handle)),
    size(std::move(init.size)) {}

SharedBufferMemoryFromWindowsHandleDescriptor::operator const WGPUSharedBufferMemoryFromWindowsHandleDescriptor&() const noexcept {
    return *reinterpret_cast<const WGPUSharedBufferMemoryFromWindowsHandleDescriptor*>(this);
}

static_assert(sizeof(SharedBufferMemoryFromWindowsHandleDescriptor) == sizeof(WGPUSharedBufferMemoryFromWindowsHandleDescriptor), "sizeof mismatch for SharedBufferMemoryFromWindowsHandleDescriptor");
static_assert(alignof(SharedBufferMemoryFromWindowsHandleDescriptor) == alignof(WGPUSharedBufferMemoryFromWindowsHandleDescriptor), "alignof mismatch for SharedBufferMemoryFromWindowsHandleDescriptor");
static_assert(offsetof(SharedBufferMemoryFromWindowsHandleDescriptor, handle) == offsetof(WGPUSharedBufferMemoryFromWindowsHandleDescriptor, handle),
        "offsetof mismatch for SharedBufferMemoryFromWindowsHandleDescriptor::handle");
static_assert(offsetof(SharedBufferMemoryFromWindowsHandleDescriptor, size) == offsetof(WGPUSharedBufferMemoryFromWindowsHandleDescriptor, size),
        "offsetof mismatch for SharedBufferMemoryFromWindowsHandleDescriptor::size");

// SharedBufferMemoryProperties implementation

SharedBufferMemoryProperties::operator const WGPUSharedBufferMemoryProperties&() const noexcept {
    return *reinterpret_cast<const WGPUSharedBufferMemoryProperties*>(this);
}

static_assert(sizeof(SharedBufferMemoryProperties) == sizeof(WGPUSharedBufferMemoryProperties), "sizeof mismatch for SharedBufferMemoryProperties");
static_assert(alignof(SharedBufferMemoryProperties) == alignof(WGPUSharedBufferMemoryProperties), "alignof mismatch for SharedBufferMemoryProperties");
static_assert(offsetof(SharedBufferMemoryProperties, nextInChain) == offsetof(WGPUSharedBufferMemoryProperties, nextInChain),
        "offsetof mismatch for SharedBufferMemoryProperties::nextInChain");
static_assert(offsetof(SharedBufferMemoryProperties, usage) == offsetof(WGPUSharedBufferMemoryProperties, usage),
        "offsetof mismatch for SharedBufferMemoryProperties::usage");
static_assert(offsetof(SharedBufferMemoryProperties, size) == offsetof(WGPUSharedBufferMemoryProperties, size),
        "offsetof mismatch for SharedBufferMemoryProperties::size");

// SharedFenceDXGISharedHandleDescriptor implementation
SharedFenceDXGISharedHandleDescriptor::SharedFenceDXGISharedHandleDescriptor()
: ChainedStruct { nullptr, SType::SharedFenceDXGISharedHandleDescriptor } {}
struct SharedFenceDXGISharedHandleDescriptor::Init {
    ChainedStruct * const nextInChain;
    void * handle;
};
SharedFenceDXGISharedHandleDescriptor::SharedFenceDXGISharedHandleDescriptor(SharedFenceDXGISharedHandleDescriptor::Init&& init) :
    ChainedStruct { init.nextInChain, SType::SharedFenceDXGISharedHandleDescriptor },
    handle(std::move(init.handle)) {}

SharedFenceDXGISharedHandleDescriptor::operator const WGPUSharedFenceDXGISharedHandleDescriptor&() const noexcept {
    return *reinterpret_cast<const WGPUSharedFenceDXGISharedHandleDescriptor*>(this);
}

static_assert(sizeof(SharedFenceDXGISharedHandleDescriptor) == sizeof(WGPUSharedFenceDXGISharedHandleDescriptor), "sizeof mismatch for SharedFenceDXGISharedHandleDescriptor");
static_assert(alignof(SharedFenceDXGISharedHandleDescriptor) == alignof(WGPUSharedFenceDXGISharedHandleDescriptor), "alignof mismatch for SharedFenceDXGISharedHandleDescriptor");
static_assert(offsetof(SharedFenceDXGISharedHandleDescriptor, handle) == offsetof(WGPUSharedFenceDXGISharedHandleDescriptor, handle),
        "offsetof mismatch for SharedFenceDXGISharedHandleDescriptor::handle");

// SharedFenceDXGISharedHandleExportInfo implementation
SharedFenceDXGISharedHandleExportInfo::SharedFenceDXGISharedHandleExportInfo()
: ChainedStructOut { nullptr, SType::SharedFenceDXGISharedHandleExportInfo } {}
struct SharedFenceDXGISharedHandleExportInfo::Init {
    ChainedStructOut *  nextInChain;
    void * handle;
};
SharedFenceDXGISharedHandleExportInfo::SharedFenceDXGISharedHandleExportInfo(SharedFenceDXGISharedHandleExportInfo::Init&& init) :
    ChainedStructOut { init.nextInChain, SType::SharedFenceDXGISharedHandleExportInfo },
    handle(std::move(init.handle)) {}

SharedFenceDXGISharedHandleExportInfo::operator const WGPUSharedFenceDXGISharedHandleExportInfo&() const noexcept {
    return *reinterpret_cast<const WGPUSharedFenceDXGISharedHandleExportInfo*>(this);
}

static_assert(sizeof(SharedFenceDXGISharedHandleExportInfo) == sizeof(WGPUSharedFenceDXGISharedHandleExportInfo), "sizeof mismatch for SharedFenceDXGISharedHandleExportInfo");
static_assert(alignof(SharedFenceDXGISharedHandleExportInfo) == alignof(WGPUSharedFenceDXGISharedHandleExportInfo), "alignof mismatch for SharedFenceDXGISharedHandleExportInfo");
static_assert(offsetof(SharedFenceDXGISharedHandleExportInfo, handle) == offsetof(WGPUSharedFenceDXGISharedHandleExportInfo, handle),
        "offsetof mismatch for SharedFenceDXGISharedHandleExportInfo::handle");

// SharedFenceEGLSyncDescriptor implementation
SharedFenceEGLSyncDescriptor::SharedFenceEGLSyncDescriptor()
: ChainedStruct { nullptr, SType::SharedFenceEGLSyncDescriptor } {}
struct SharedFenceEGLSyncDescriptor::Init {
    ChainedStruct * const nextInChain;
    void * sync;
};
SharedFenceEGLSyncDescriptor::SharedFenceEGLSyncDescriptor(SharedFenceEGLSyncDescriptor::Init&& init) :
    ChainedStruct { init.nextInChain, SType::SharedFenceEGLSyncDescriptor },
    sync(std::move(init.sync)) {}

SharedFenceEGLSyncDescriptor::operator const WGPUSharedFenceEGLSyncDescriptor&() const noexcept {
    return *reinterpret_cast<const WGPUSharedFenceEGLSyncDescriptor*>(this);
}

static_assert(sizeof(SharedFenceEGLSyncDescriptor) == sizeof(WGPUSharedFenceEGLSyncDescriptor), "sizeof mismatch for SharedFenceEGLSyncDescriptor");
static_assert(alignof(SharedFenceEGLSyncDescriptor) == alignof(WGPUSharedFenceEGLSyncDescriptor), "alignof mismatch for SharedFenceEGLSyncDescriptor");
static_assert(offsetof(SharedFenceEGLSyncDescriptor, sync) == offsetof(WGPUSharedFenceEGLSyncDescriptor, sync),
        "offsetof mismatch for SharedFenceEGLSyncDescriptor::sync");

// SharedFenceEGLSyncExportInfo implementation
SharedFenceEGLSyncExportInfo::SharedFenceEGLSyncExportInfo()
: ChainedStructOut { nullptr, SType::SharedFenceEGLSyncExportInfo } {}
struct SharedFenceEGLSyncExportInfo::Init {
    ChainedStructOut *  nextInChain;
    void * sync;
};
SharedFenceEGLSyncExportInfo::SharedFenceEGLSyncExportInfo(SharedFenceEGLSyncExportInfo::Init&& init) :
    ChainedStructOut { init.nextInChain, SType::SharedFenceEGLSyncExportInfo },
    sync(std::move(init.sync)) {}

SharedFenceEGLSyncExportInfo::operator const WGPUSharedFenceEGLSyncExportInfo&() const noexcept {
    return *reinterpret_cast<const WGPUSharedFenceEGLSyncExportInfo*>(this);
}

static_assert(sizeof(SharedFenceEGLSyncExportInfo) == sizeof(WGPUSharedFenceEGLSyncExportInfo), "sizeof mismatch for SharedFenceEGLSyncExportInfo");
static_assert(alignof(SharedFenceEGLSyncExportInfo) == alignof(WGPUSharedFenceEGLSyncExportInfo), "alignof mismatch for SharedFenceEGLSyncExportInfo");
static_assert(offsetof(SharedFenceEGLSyncExportInfo, sync) == offsetof(WGPUSharedFenceEGLSyncExportInfo, sync),
        "offsetof mismatch for SharedFenceEGLSyncExportInfo::sync");

// SharedFenceMTLSharedEventDescriptor implementation
SharedFenceMTLSharedEventDescriptor::SharedFenceMTLSharedEventDescriptor()
: ChainedStruct { nullptr, SType::SharedFenceMTLSharedEventDescriptor } {}
struct SharedFenceMTLSharedEventDescriptor::Init {
    ChainedStruct * const nextInChain;
    void * sharedEvent;
};
SharedFenceMTLSharedEventDescriptor::SharedFenceMTLSharedEventDescriptor(SharedFenceMTLSharedEventDescriptor::Init&& init) :
    ChainedStruct { init.nextInChain, SType::SharedFenceMTLSharedEventDescriptor },
    sharedEvent(std::move(init.sharedEvent)) {}

SharedFenceMTLSharedEventDescriptor::operator const WGPUSharedFenceMTLSharedEventDescriptor&() const noexcept {
    return *reinterpret_cast<const WGPUSharedFenceMTLSharedEventDescriptor*>(this);
}

static_assert(sizeof(SharedFenceMTLSharedEventDescriptor) == sizeof(WGPUSharedFenceMTLSharedEventDescriptor), "sizeof mismatch for SharedFenceMTLSharedEventDescriptor");
static_assert(alignof(SharedFenceMTLSharedEventDescriptor) == alignof(WGPUSharedFenceMTLSharedEventDescriptor), "alignof mismatch for SharedFenceMTLSharedEventDescriptor");
static_assert(offsetof(SharedFenceMTLSharedEventDescriptor, sharedEvent) == offsetof(WGPUSharedFenceMTLSharedEventDescriptor, sharedEvent),
        "offsetof mismatch for SharedFenceMTLSharedEventDescriptor::sharedEvent");

// SharedFenceMTLSharedEventExportInfo implementation
SharedFenceMTLSharedEventExportInfo::SharedFenceMTLSharedEventExportInfo()
: ChainedStructOut { nullptr, SType::SharedFenceMTLSharedEventExportInfo } {}
struct SharedFenceMTLSharedEventExportInfo::Init {
    ChainedStructOut *  nextInChain;
    void * sharedEvent;
};
SharedFenceMTLSharedEventExportInfo::SharedFenceMTLSharedEventExportInfo(SharedFenceMTLSharedEventExportInfo::Init&& init) :
    ChainedStructOut { init.nextInChain, SType::SharedFenceMTLSharedEventExportInfo },
    sharedEvent(std::move(init.sharedEvent)) {}

SharedFenceMTLSharedEventExportInfo::operator const WGPUSharedFenceMTLSharedEventExportInfo&() const noexcept {
    return *reinterpret_cast<const WGPUSharedFenceMTLSharedEventExportInfo*>(this);
}

static_assert(sizeof(SharedFenceMTLSharedEventExportInfo) == sizeof(WGPUSharedFenceMTLSharedEventExportInfo), "sizeof mismatch for SharedFenceMTLSharedEventExportInfo");
static_assert(alignof(SharedFenceMTLSharedEventExportInfo) == alignof(WGPUSharedFenceMTLSharedEventExportInfo), "alignof mismatch for SharedFenceMTLSharedEventExportInfo");
static_assert(offsetof(SharedFenceMTLSharedEventExportInfo, sharedEvent) == offsetof(WGPUSharedFenceMTLSharedEventExportInfo, sharedEvent),
        "offsetof mismatch for SharedFenceMTLSharedEventExportInfo::sharedEvent");

// SharedFenceSyncFDDescriptor implementation
SharedFenceSyncFDDescriptor::SharedFenceSyncFDDescriptor()
: ChainedStruct { nullptr, SType::SharedFenceSyncFDDescriptor } {}
struct SharedFenceSyncFDDescriptor::Init {
    ChainedStruct * const nextInChain;
    int handle;
};
SharedFenceSyncFDDescriptor::SharedFenceSyncFDDescriptor(SharedFenceSyncFDDescriptor::Init&& init) :
    ChainedStruct { init.nextInChain, SType::SharedFenceSyncFDDescriptor },
    handle(std::move(init.handle)) {}

SharedFenceSyncFDDescriptor::operator const WGPUSharedFenceSyncFDDescriptor&() const noexcept {
    return *reinterpret_cast<const WGPUSharedFenceSyncFDDescriptor*>(this);
}

static_assert(sizeof(SharedFenceSyncFDDescriptor) == sizeof(WGPUSharedFenceSyncFDDescriptor), "sizeof mismatch for SharedFenceSyncFDDescriptor");
static_assert(alignof(SharedFenceSyncFDDescriptor) == alignof(WGPUSharedFenceSyncFDDescriptor), "alignof mismatch for SharedFenceSyncFDDescriptor");
static_assert(offsetof(SharedFenceSyncFDDescriptor, handle) == offsetof(WGPUSharedFenceSyncFDDescriptor, handle),
        "offsetof mismatch for SharedFenceSyncFDDescriptor::handle");

// SharedFenceSyncFDExportInfo implementation
SharedFenceSyncFDExportInfo::SharedFenceSyncFDExportInfo()
: ChainedStructOut { nullptr, SType::SharedFenceSyncFDExportInfo } {}
struct SharedFenceSyncFDExportInfo::Init {
    ChainedStructOut *  nextInChain;
    int handle;
};
SharedFenceSyncFDExportInfo::SharedFenceSyncFDExportInfo(SharedFenceSyncFDExportInfo::Init&& init) :
    ChainedStructOut { init.nextInChain, SType::SharedFenceSyncFDExportInfo },
    handle(std::move(init.handle)) {}

SharedFenceSyncFDExportInfo::operator const WGPUSharedFenceSyncFDExportInfo&() const noexcept {
    return *reinterpret_cast<const WGPUSharedFenceSyncFDExportInfo*>(this);
}

static_assert(sizeof(SharedFenceSyncFDExportInfo) == sizeof(WGPUSharedFenceSyncFDExportInfo), "sizeof mismatch for SharedFenceSyncFDExportInfo");
static_assert(alignof(SharedFenceSyncFDExportInfo) == alignof(WGPUSharedFenceSyncFDExportInfo), "alignof mismatch for SharedFenceSyncFDExportInfo");
static_assert(offsetof(SharedFenceSyncFDExportInfo, handle) == offsetof(WGPUSharedFenceSyncFDExportInfo, handle),
        "offsetof mismatch for SharedFenceSyncFDExportInfo::handle");

// SharedFenceVkSemaphoreOpaqueFDDescriptor implementation
SharedFenceVkSemaphoreOpaqueFDDescriptor::SharedFenceVkSemaphoreOpaqueFDDescriptor()
: ChainedStruct { nullptr, SType::SharedFenceVkSemaphoreOpaqueFDDescriptor } {}
struct SharedFenceVkSemaphoreOpaqueFDDescriptor::Init {
    ChainedStruct * const nextInChain;
    int handle;
};
SharedFenceVkSemaphoreOpaqueFDDescriptor::SharedFenceVkSemaphoreOpaqueFDDescriptor(SharedFenceVkSemaphoreOpaqueFDDescriptor::Init&& init) :
    ChainedStruct { init.nextInChain, SType::SharedFenceVkSemaphoreOpaqueFDDescriptor },
    handle(std::move(init.handle)) {}

SharedFenceVkSemaphoreOpaqueFDDescriptor::operator const WGPUSharedFenceVkSemaphoreOpaqueFDDescriptor&() const noexcept {
    return *reinterpret_cast<const WGPUSharedFenceVkSemaphoreOpaqueFDDescriptor*>(this);
}

static_assert(sizeof(SharedFenceVkSemaphoreOpaqueFDDescriptor) == sizeof(WGPUSharedFenceVkSemaphoreOpaqueFDDescriptor), "sizeof mismatch for SharedFenceVkSemaphoreOpaqueFDDescriptor");
static_assert(alignof(SharedFenceVkSemaphoreOpaqueFDDescriptor) == alignof(WGPUSharedFenceVkSemaphoreOpaqueFDDescriptor), "alignof mismatch for SharedFenceVkSemaphoreOpaqueFDDescriptor");
static_assert(offsetof(SharedFenceVkSemaphoreOpaqueFDDescriptor, handle) == offsetof(WGPUSharedFenceVkSemaphoreOpaqueFDDescriptor, handle),
        "offsetof mismatch for SharedFenceVkSemaphoreOpaqueFDDescriptor::handle");

// SharedFenceVkSemaphoreOpaqueFDExportInfo implementation
SharedFenceVkSemaphoreOpaqueFDExportInfo::SharedFenceVkSemaphoreOpaqueFDExportInfo()
: ChainedStructOut { nullptr, SType::SharedFenceVkSemaphoreOpaqueFDExportInfo } {}
struct SharedFenceVkSemaphoreOpaqueFDExportInfo::Init {
    ChainedStructOut *  nextInChain;
    int handle;
};
SharedFenceVkSemaphoreOpaqueFDExportInfo::SharedFenceVkSemaphoreOpaqueFDExportInfo(SharedFenceVkSemaphoreOpaqueFDExportInfo::Init&& init) :
    ChainedStructOut { init.nextInChain, SType::SharedFenceVkSemaphoreOpaqueFDExportInfo },
    handle(std::move(init.handle)) {}

SharedFenceVkSemaphoreOpaqueFDExportInfo::operator const WGPUSharedFenceVkSemaphoreOpaqueFDExportInfo&() const noexcept {
    return *reinterpret_cast<const WGPUSharedFenceVkSemaphoreOpaqueFDExportInfo*>(this);
}

static_assert(sizeof(SharedFenceVkSemaphoreOpaqueFDExportInfo) == sizeof(WGPUSharedFenceVkSemaphoreOpaqueFDExportInfo), "sizeof mismatch for SharedFenceVkSemaphoreOpaqueFDExportInfo");
static_assert(alignof(SharedFenceVkSemaphoreOpaqueFDExportInfo) == alignof(WGPUSharedFenceVkSemaphoreOpaqueFDExportInfo), "alignof mismatch for SharedFenceVkSemaphoreOpaqueFDExportInfo");
static_assert(offsetof(SharedFenceVkSemaphoreOpaqueFDExportInfo, handle) == offsetof(WGPUSharedFenceVkSemaphoreOpaqueFDExportInfo, handle),
        "offsetof mismatch for SharedFenceVkSemaphoreOpaqueFDExportInfo::handle");

// SharedFenceVkSemaphoreZirconHandleDescriptor implementation
SharedFenceVkSemaphoreZirconHandleDescriptor::SharedFenceVkSemaphoreZirconHandleDescriptor()
: ChainedStruct { nullptr, SType::SharedFenceVkSemaphoreZirconHandleDescriptor } {}
struct SharedFenceVkSemaphoreZirconHandleDescriptor::Init {
    ChainedStruct * const nextInChain;
    uint32_t handle;
};
SharedFenceVkSemaphoreZirconHandleDescriptor::SharedFenceVkSemaphoreZirconHandleDescriptor(SharedFenceVkSemaphoreZirconHandleDescriptor::Init&& init) :
    ChainedStruct { init.nextInChain, SType::SharedFenceVkSemaphoreZirconHandleDescriptor },
    handle(std::move(init.handle)) {}

SharedFenceVkSemaphoreZirconHandleDescriptor::operator const WGPUSharedFenceVkSemaphoreZirconHandleDescriptor&() const noexcept {
    return *reinterpret_cast<const WGPUSharedFenceVkSemaphoreZirconHandleDescriptor*>(this);
}

static_assert(sizeof(SharedFenceVkSemaphoreZirconHandleDescriptor) == sizeof(WGPUSharedFenceVkSemaphoreZirconHandleDescriptor), "sizeof mismatch for SharedFenceVkSemaphoreZirconHandleDescriptor");
static_assert(alignof(SharedFenceVkSemaphoreZirconHandleDescriptor) == alignof(WGPUSharedFenceVkSemaphoreZirconHandleDescriptor), "alignof mismatch for SharedFenceVkSemaphoreZirconHandleDescriptor");
static_assert(offsetof(SharedFenceVkSemaphoreZirconHandleDescriptor, handle) == offsetof(WGPUSharedFenceVkSemaphoreZirconHandleDescriptor, handle),
        "offsetof mismatch for SharedFenceVkSemaphoreZirconHandleDescriptor::handle");

// SharedFenceVkSemaphoreZirconHandleExportInfo implementation
SharedFenceVkSemaphoreZirconHandleExportInfo::SharedFenceVkSemaphoreZirconHandleExportInfo()
: ChainedStructOut { nullptr, SType::SharedFenceVkSemaphoreZirconHandleExportInfo } {}
struct SharedFenceVkSemaphoreZirconHandleExportInfo::Init {
    ChainedStructOut *  nextInChain;
    uint32_t handle;
};
SharedFenceVkSemaphoreZirconHandleExportInfo::SharedFenceVkSemaphoreZirconHandleExportInfo(SharedFenceVkSemaphoreZirconHandleExportInfo::Init&& init) :
    ChainedStructOut { init.nextInChain, SType::SharedFenceVkSemaphoreZirconHandleExportInfo },
    handle(std::move(init.handle)) {}

SharedFenceVkSemaphoreZirconHandleExportInfo::operator const WGPUSharedFenceVkSemaphoreZirconHandleExportInfo&() const noexcept {
    return *reinterpret_cast<const WGPUSharedFenceVkSemaphoreZirconHandleExportInfo*>(this);
}

static_assert(sizeof(SharedFenceVkSemaphoreZirconHandleExportInfo) == sizeof(WGPUSharedFenceVkSemaphoreZirconHandleExportInfo), "sizeof mismatch for SharedFenceVkSemaphoreZirconHandleExportInfo");
static_assert(alignof(SharedFenceVkSemaphoreZirconHandleExportInfo) == alignof(WGPUSharedFenceVkSemaphoreZirconHandleExportInfo), "alignof mismatch for SharedFenceVkSemaphoreZirconHandleExportInfo");
static_assert(offsetof(SharedFenceVkSemaphoreZirconHandleExportInfo, handle) == offsetof(WGPUSharedFenceVkSemaphoreZirconHandleExportInfo, handle),
        "offsetof mismatch for SharedFenceVkSemaphoreZirconHandleExportInfo::handle");

// SharedTextureMemoryAHardwareBufferDescriptor implementation
SharedTextureMemoryAHardwareBufferDescriptor::SharedTextureMemoryAHardwareBufferDescriptor()
: ChainedStruct { nullptr, SType::SharedTextureMemoryAHardwareBufferDescriptor } {}
struct SharedTextureMemoryAHardwareBufferDescriptor::Init {
    ChainedStruct * const nextInChain;
    void * handle;
};
SharedTextureMemoryAHardwareBufferDescriptor::SharedTextureMemoryAHardwareBufferDescriptor(SharedTextureMemoryAHardwareBufferDescriptor::Init&& init) :
    ChainedStruct { init.nextInChain, SType::SharedTextureMemoryAHardwareBufferDescriptor },
    handle(std::move(init.handle)) {}

SharedTextureMemoryAHardwareBufferDescriptor::operator const WGPUSharedTextureMemoryAHardwareBufferDescriptor&() const noexcept {
    return *reinterpret_cast<const WGPUSharedTextureMemoryAHardwareBufferDescriptor*>(this);
}

static_assert(sizeof(SharedTextureMemoryAHardwareBufferDescriptor) == sizeof(WGPUSharedTextureMemoryAHardwareBufferDescriptor), "sizeof mismatch for SharedTextureMemoryAHardwareBufferDescriptor");
static_assert(alignof(SharedTextureMemoryAHardwareBufferDescriptor) == alignof(WGPUSharedTextureMemoryAHardwareBufferDescriptor), "alignof mismatch for SharedTextureMemoryAHardwareBufferDescriptor");
static_assert(offsetof(SharedTextureMemoryAHardwareBufferDescriptor, handle) == offsetof(WGPUSharedTextureMemoryAHardwareBufferDescriptor, handle),
        "offsetof mismatch for SharedTextureMemoryAHardwareBufferDescriptor::handle");

// SharedTextureMemoryD3D11BeginState implementation
SharedTextureMemoryD3D11BeginState::SharedTextureMemoryD3D11BeginState()
: ChainedStruct { nullptr, SType::SharedTextureMemoryD3D11BeginState } {}
struct SharedTextureMemoryD3D11BeginState::Init {
    ChainedStruct * const nextInChain;
    Bool requiresEndAccessFence = true;
};
SharedTextureMemoryD3D11BeginState::SharedTextureMemoryD3D11BeginState(SharedTextureMemoryD3D11BeginState::Init&& init) :
    ChainedStruct { init.nextInChain, SType::SharedTextureMemoryD3D11BeginState },
    requiresEndAccessFence(std::move(init.requiresEndAccessFence)) {}

SharedTextureMemoryD3D11BeginState::operator const WGPUSharedTextureMemoryD3D11BeginState&() const noexcept {
    return *reinterpret_cast<const WGPUSharedTextureMemoryD3D11BeginState*>(this);
}

static_assert(sizeof(SharedTextureMemoryD3D11BeginState) == sizeof(WGPUSharedTextureMemoryD3D11BeginState), "sizeof mismatch for SharedTextureMemoryD3D11BeginState");
static_assert(alignof(SharedTextureMemoryD3D11BeginState) == alignof(WGPUSharedTextureMemoryD3D11BeginState), "alignof mismatch for SharedTextureMemoryD3D11BeginState");
static_assert(offsetof(SharedTextureMemoryD3D11BeginState, requiresEndAccessFence) == offsetof(WGPUSharedTextureMemoryD3D11BeginState, requiresEndAccessFence),
        "offsetof mismatch for SharedTextureMemoryD3D11BeginState::requiresEndAccessFence");

// SharedTextureMemoryD3DSwapchainBeginState implementation
SharedTextureMemoryD3DSwapchainBeginState::SharedTextureMemoryD3DSwapchainBeginState()
: ChainedStruct { nullptr, SType::SharedTextureMemoryD3DSwapchainBeginState } {}
struct SharedTextureMemoryD3DSwapchainBeginState::Init {
    ChainedStruct * const nextInChain;
    Bool isSwapchain = false;
};
SharedTextureMemoryD3DSwapchainBeginState::SharedTextureMemoryD3DSwapchainBeginState(SharedTextureMemoryD3DSwapchainBeginState::Init&& init) :
    ChainedStruct { init.nextInChain, SType::SharedTextureMemoryD3DSwapchainBeginState },
    isSwapchain(std::move(init.isSwapchain)) {}

SharedTextureMemoryD3DSwapchainBeginState::operator const WGPUSharedTextureMemoryD3DSwapchainBeginState&() const noexcept {
    return *reinterpret_cast<const WGPUSharedTextureMemoryD3DSwapchainBeginState*>(this);
}

static_assert(sizeof(SharedTextureMemoryD3DSwapchainBeginState) == sizeof(WGPUSharedTextureMemoryD3DSwapchainBeginState), "sizeof mismatch for SharedTextureMemoryD3DSwapchainBeginState");
static_assert(alignof(SharedTextureMemoryD3DSwapchainBeginState) == alignof(WGPUSharedTextureMemoryD3DSwapchainBeginState), "alignof mismatch for SharedTextureMemoryD3DSwapchainBeginState");
static_assert(offsetof(SharedTextureMemoryD3DSwapchainBeginState, isSwapchain) == offsetof(WGPUSharedTextureMemoryD3DSwapchainBeginState, isSwapchain),
        "offsetof mismatch for SharedTextureMemoryD3DSwapchainBeginState::isSwapchain");

// SharedTextureMemoryDmaBufPlane implementation

SharedTextureMemoryDmaBufPlane::operator const WGPUSharedTextureMemoryDmaBufPlane&() const noexcept {
    return *reinterpret_cast<const WGPUSharedTextureMemoryDmaBufPlane*>(this);
}

static_assert(sizeof(SharedTextureMemoryDmaBufPlane) == sizeof(WGPUSharedTextureMemoryDmaBufPlane), "sizeof mismatch for SharedTextureMemoryDmaBufPlane");
static_assert(alignof(SharedTextureMemoryDmaBufPlane) == alignof(WGPUSharedTextureMemoryDmaBufPlane), "alignof mismatch for SharedTextureMemoryDmaBufPlane");
static_assert(offsetof(SharedTextureMemoryDmaBufPlane, fd) == offsetof(WGPUSharedTextureMemoryDmaBufPlane, fd),
        "offsetof mismatch for SharedTextureMemoryDmaBufPlane::fd");
static_assert(offsetof(SharedTextureMemoryDmaBufPlane, offset) == offsetof(WGPUSharedTextureMemoryDmaBufPlane, offset),
        "offsetof mismatch for SharedTextureMemoryDmaBufPlane::offset");
static_assert(offsetof(SharedTextureMemoryDmaBufPlane, stride) == offsetof(WGPUSharedTextureMemoryDmaBufPlane, stride),
        "offsetof mismatch for SharedTextureMemoryDmaBufPlane::stride");

// SharedTextureMemoryDXGISharedHandleDescriptor implementation
SharedTextureMemoryDXGISharedHandleDescriptor::SharedTextureMemoryDXGISharedHandleDescriptor()
: ChainedStruct { nullptr, SType::SharedTextureMemoryDXGISharedHandleDescriptor } {}
struct SharedTextureMemoryDXGISharedHandleDescriptor::Init {
    ChainedStruct * const nextInChain;
    void * handle;
    Bool useKeyedMutex;
};
SharedTextureMemoryDXGISharedHandleDescriptor::SharedTextureMemoryDXGISharedHandleDescriptor(SharedTextureMemoryDXGISharedHandleDescriptor::Init&& init) :
    ChainedStruct { init.nextInChain, SType::SharedTextureMemoryDXGISharedHandleDescriptor },
    handle(std::move(init.handle)),
    useKeyedMutex(std::move(init.useKeyedMutex)) {}

SharedTextureMemoryDXGISharedHandleDescriptor::operator const WGPUSharedTextureMemoryDXGISharedHandleDescriptor&() const noexcept {
    return *reinterpret_cast<const WGPUSharedTextureMemoryDXGISharedHandleDescriptor*>(this);
}

static_assert(sizeof(SharedTextureMemoryDXGISharedHandleDescriptor) == sizeof(WGPUSharedTextureMemoryDXGISharedHandleDescriptor), "sizeof mismatch for SharedTextureMemoryDXGISharedHandleDescriptor");
static_assert(alignof(SharedTextureMemoryDXGISharedHandleDescriptor) == alignof(WGPUSharedTextureMemoryDXGISharedHandleDescriptor), "alignof mismatch for SharedTextureMemoryDXGISharedHandleDescriptor");
static_assert(offsetof(SharedTextureMemoryDXGISharedHandleDescriptor, handle) == offsetof(WGPUSharedTextureMemoryDXGISharedHandleDescriptor, handle),
        "offsetof mismatch for SharedTextureMemoryDXGISharedHandleDescriptor::handle");
static_assert(offsetof(SharedTextureMemoryDXGISharedHandleDescriptor, useKeyedMutex) == offsetof(WGPUSharedTextureMemoryDXGISharedHandleDescriptor, useKeyedMutex),
        "offsetof mismatch for SharedTextureMemoryDXGISharedHandleDescriptor::useKeyedMutex");

// SharedTextureMemoryEGLImageDescriptor implementation
SharedTextureMemoryEGLImageDescriptor::SharedTextureMemoryEGLImageDescriptor()
: ChainedStruct { nullptr, SType::SharedTextureMemoryEGLImageDescriptor } {}
struct SharedTextureMemoryEGLImageDescriptor::Init {
    ChainedStruct * const nextInChain;
    void * image;
};
SharedTextureMemoryEGLImageDescriptor::SharedTextureMemoryEGLImageDescriptor(SharedTextureMemoryEGLImageDescriptor::Init&& init) :
    ChainedStruct { init.nextInChain, SType::SharedTextureMemoryEGLImageDescriptor },
    image(std::move(init.image)) {}

SharedTextureMemoryEGLImageDescriptor::operator const WGPUSharedTextureMemoryEGLImageDescriptor&() const noexcept {
    return *reinterpret_cast<const WGPUSharedTextureMemoryEGLImageDescriptor*>(this);
}

static_assert(sizeof(SharedTextureMemoryEGLImageDescriptor) == sizeof(WGPUSharedTextureMemoryEGLImageDescriptor), "sizeof mismatch for SharedTextureMemoryEGLImageDescriptor");
static_assert(alignof(SharedTextureMemoryEGLImageDescriptor) == alignof(WGPUSharedTextureMemoryEGLImageDescriptor), "alignof mismatch for SharedTextureMemoryEGLImageDescriptor");
static_assert(offsetof(SharedTextureMemoryEGLImageDescriptor, image) == offsetof(WGPUSharedTextureMemoryEGLImageDescriptor, image),
        "offsetof mismatch for SharedTextureMemoryEGLImageDescriptor::image");

// SharedTextureMemoryIOSurfaceDescriptor implementation
SharedTextureMemoryIOSurfaceDescriptor::SharedTextureMemoryIOSurfaceDescriptor()
: ChainedStruct { nullptr, SType::SharedTextureMemoryIOSurfaceDescriptor } {}
struct SharedTextureMemoryIOSurfaceDescriptor::Init {
    ChainedStruct * const nextInChain;
    void * ioSurface;
    Bool allowStorageBinding = true;
};
SharedTextureMemoryIOSurfaceDescriptor::SharedTextureMemoryIOSurfaceDescriptor(SharedTextureMemoryIOSurfaceDescriptor::Init&& init) :
    ChainedStruct { init.nextInChain, SType::SharedTextureMemoryIOSurfaceDescriptor },
    ioSurface(std::move(init.ioSurface)),
    allowStorageBinding(std::move(init.allowStorageBinding)) {}

SharedTextureMemoryIOSurfaceDescriptor::operator const WGPUSharedTextureMemoryIOSurfaceDescriptor&() const noexcept {
    return *reinterpret_cast<const WGPUSharedTextureMemoryIOSurfaceDescriptor*>(this);
}

static_assert(sizeof(SharedTextureMemoryIOSurfaceDescriptor) == sizeof(WGPUSharedTextureMemoryIOSurfaceDescriptor), "sizeof mismatch for SharedTextureMemoryIOSurfaceDescriptor");
static_assert(alignof(SharedTextureMemoryIOSurfaceDescriptor) == alignof(WGPUSharedTextureMemoryIOSurfaceDescriptor), "alignof mismatch for SharedTextureMemoryIOSurfaceDescriptor");
static_assert(offsetof(SharedTextureMemoryIOSurfaceDescriptor, ioSurface) == offsetof(WGPUSharedTextureMemoryIOSurfaceDescriptor, ioSurface),
        "offsetof mismatch for SharedTextureMemoryIOSurfaceDescriptor::ioSurface");
static_assert(offsetof(SharedTextureMemoryIOSurfaceDescriptor, allowStorageBinding) == offsetof(WGPUSharedTextureMemoryIOSurfaceDescriptor, allowStorageBinding),
        "offsetof mismatch for SharedTextureMemoryIOSurfaceDescriptor::allowStorageBinding");

// SharedTextureMemoryOpaqueFDDescriptor implementation
SharedTextureMemoryOpaqueFDDescriptor::SharedTextureMemoryOpaqueFDDescriptor()
: ChainedStruct { nullptr, SType::SharedTextureMemoryOpaqueFDDescriptor } {}
struct SharedTextureMemoryOpaqueFDDescriptor::Init {
    ChainedStruct * const nextInChain;
    void const * vkImageCreateInfo;
    int memoryFD;
    uint32_t memoryTypeIndex;
    uint64_t allocationSize;
    Bool dedicatedAllocation;
};
SharedTextureMemoryOpaqueFDDescriptor::SharedTextureMemoryOpaqueFDDescriptor(SharedTextureMemoryOpaqueFDDescriptor::Init&& init) :
    ChainedStruct { init.nextInChain, SType::SharedTextureMemoryOpaqueFDDescriptor },
    vkImageCreateInfo(std::move(init.vkImageCreateInfo)),
    memoryFD(std::move(init.memoryFD)),
    memoryTypeIndex(std::move(init.memoryTypeIndex)),
    allocationSize(std::move(init.allocationSize)),
    dedicatedAllocation(std::move(init.dedicatedAllocation)) {}

SharedTextureMemoryOpaqueFDDescriptor::operator const WGPUSharedTextureMemoryOpaqueFDDescriptor&() const noexcept {
    return *reinterpret_cast<const WGPUSharedTextureMemoryOpaqueFDDescriptor*>(this);
}

static_assert(sizeof(SharedTextureMemoryOpaqueFDDescriptor) == sizeof(WGPUSharedTextureMemoryOpaqueFDDescriptor), "sizeof mismatch for SharedTextureMemoryOpaqueFDDescriptor");
static_assert(alignof(SharedTextureMemoryOpaqueFDDescriptor) == alignof(WGPUSharedTextureMemoryOpaqueFDDescriptor), "alignof mismatch for SharedTextureMemoryOpaqueFDDescriptor");
static_assert(offsetof(SharedTextureMemoryOpaqueFDDescriptor, vkImageCreateInfo) == offsetof(WGPUSharedTextureMemoryOpaqueFDDescriptor, vkImageCreateInfo),
        "offsetof mismatch for SharedTextureMemoryOpaqueFDDescriptor::vkImageCreateInfo");
static_assert(offsetof(SharedTextureMemoryOpaqueFDDescriptor, memoryFD) == offsetof(WGPUSharedTextureMemoryOpaqueFDDescriptor, memoryFD),
        "offsetof mismatch for SharedTextureMemoryOpaqueFDDescriptor::memoryFD");
static_assert(offsetof(SharedTextureMemoryOpaqueFDDescriptor, memoryTypeIndex) == offsetof(WGPUSharedTextureMemoryOpaqueFDDescriptor, memoryTypeIndex),
        "offsetof mismatch for SharedTextureMemoryOpaqueFDDescriptor::memoryTypeIndex");
static_assert(offsetof(SharedTextureMemoryOpaqueFDDescriptor, allocationSize) == offsetof(WGPUSharedTextureMemoryOpaqueFDDescriptor, allocationSize),
        "offsetof mismatch for SharedTextureMemoryOpaqueFDDescriptor::allocationSize");
static_assert(offsetof(SharedTextureMemoryOpaqueFDDescriptor, dedicatedAllocation) == offsetof(WGPUSharedTextureMemoryOpaqueFDDescriptor, dedicatedAllocation),
        "offsetof mismatch for SharedTextureMemoryOpaqueFDDescriptor::dedicatedAllocation");

// SharedTextureMemoryVkDedicatedAllocationDescriptor implementation
SharedTextureMemoryVkDedicatedAllocationDescriptor::SharedTextureMemoryVkDedicatedAllocationDescriptor()
: ChainedStruct { nullptr, SType::SharedTextureMemoryVkDedicatedAllocationDescriptor } {}
struct SharedTextureMemoryVkDedicatedAllocationDescriptor::Init {
    ChainedStruct * const nextInChain;
    Bool dedicatedAllocation;
};
SharedTextureMemoryVkDedicatedAllocationDescriptor::SharedTextureMemoryVkDedicatedAllocationDescriptor(SharedTextureMemoryVkDedicatedAllocationDescriptor::Init&& init) :
    ChainedStruct { init.nextInChain, SType::SharedTextureMemoryVkDedicatedAllocationDescriptor },
    dedicatedAllocation(std::move(init.dedicatedAllocation)) {}

SharedTextureMemoryVkDedicatedAllocationDescriptor::operator const WGPUSharedTextureMemoryVkDedicatedAllocationDescriptor&() const noexcept {
    return *reinterpret_cast<const WGPUSharedTextureMemoryVkDedicatedAllocationDescriptor*>(this);
}

static_assert(sizeof(SharedTextureMemoryVkDedicatedAllocationDescriptor) == sizeof(WGPUSharedTextureMemoryVkDedicatedAllocationDescriptor), "sizeof mismatch for SharedTextureMemoryVkDedicatedAllocationDescriptor");
static_assert(alignof(SharedTextureMemoryVkDedicatedAllocationDescriptor) == alignof(WGPUSharedTextureMemoryVkDedicatedAllocationDescriptor), "alignof mismatch for SharedTextureMemoryVkDedicatedAllocationDescriptor");
static_assert(offsetof(SharedTextureMemoryVkDedicatedAllocationDescriptor, dedicatedAllocation) == offsetof(WGPUSharedTextureMemoryVkDedicatedAllocationDescriptor, dedicatedAllocation),
        "offsetof mismatch for SharedTextureMemoryVkDedicatedAllocationDescriptor::dedicatedAllocation");

// SharedTextureMemoryVkImageLayoutBeginState implementation
SharedTextureMemoryVkImageLayoutBeginState::SharedTextureMemoryVkImageLayoutBeginState()
: ChainedStruct { nullptr, SType::SharedTextureMemoryVkImageLayoutBeginState } {}
struct SharedTextureMemoryVkImageLayoutBeginState::Init {
    ChainedStruct * const nextInChain;
    int32_t oldLayout;
    int32_t newLayout;
};
SharedTextureMemoryVkImageLayoutBeginState::SharedTextureMemoryVkImageLayoutBeginState(SharedTextureMemoryVkImageLayoutBeginState::Init&& init) :
    ChainedStruct { init.nextInChain, SType::SharedTextureMemoryVkImageLayoutBeginState },
    oldLayout(std::move(init.oldLayout)),
    newLayout(std::move(init.newLayout)) {}

SharedTextureMemoryVkImageLayoutBeginState::operator const WGPUSharedTextureMemoryVkImageLayoutBeginState&() const noexcept {
    return *reinterpret_cast<const WGPUSharedTextureMemoryVkImageLayoutBeginState*>(this);
}

static_assert(sizeof(SharedTextureMemoryVkImageLayoutBeginState) == sizeof(WGPUSharedTextureMemoryVkImageLayoutBeginState), "sizeof mismatch for SharedTextureMemoryVkImageLayoutBeginState");
static_assert(alignof(SharedTextureMemoryVkImageLayoutBeginState) == alignof(WGPUSharedTextureMemoryVkImageLayoutBeginState), "alignof mismatch for SharedTextureMemoryVkImageLayoutBeginState");
static_assert(offsetof(SharedTextureMemoryVkImageLayoutBeginState, oldLayout) == offsetof(WGPUSharedTextureMemoryVkImageLayoutBeginState, oldLayout),
        "offsetof mismatch for SharedTextureMemoryVkImageLayoutBeginState::oldLayout");
static_assert(offsetof(SharedTextureMemoryVkImageLayoutBeginState, newLayout) == offsetof(WGPUSharedTextureMemoryVkImageLayoutBeginState, newLayout),
        "offsetof mismatch for SharedTextureMemoryVkImageLayoutBeginState::newLayout");

// SharedTextureMemoryVkImageLayoutEndState implementation
SharedTextureMemoryVkImageLayoutEndState::SharedTextureMemoryVkImageLayoutEndState()
: ChainedStructOut { nullptr, SType::SharedTextureMemoryVkImageLayoutEndState } {}
struct SharedTextureMemoryVkImageLayoutEndState::Init {
    ChainedStructOut *  nextInChain;
    int32_t oldLayout;
    int32_t newLayout;
};
SharedTextureMemoryVkImageLayoutEndState::SharedTextureMemoryVkImageLayoutEndState(SharedTextureMemoryVkImageLayoutEndState::Init&& init) :
    ChainedStructOut { init.nextInChain, SType::SharedTextureMemoryVkImageLayoutEndState },
    oldLayout(std::move(init.oldLayout)),
    newLayout(std::move(init.newLayout)) {}

SharedTextureMemoryVkImageLayoutEndState::operator const WGPUSharedTextureMemoryVkImageLayoutEndState&() const noexcept {
    return *reinterpret_cast<const WGPUSharedTextureMemoryVkImageLayoutEndState*>(this);
}

static_assert(sizeof(SharedTextureMemoryVkImageLayoutEndState) == sizeof(WGPUSharedTextureMemoryVkImageLayoutEndState), "sizeof mismatch for SharedTextureMemoryVkImageLayoutEndState");
static_assert(alignof(SharedTextureMemoryVkImageLayoutEndState) == alignof(WGPUSharedTextureMemoryVkImageLayoutEndState), "alignof mismatch for SharedTextureMemoryVkImageLayoutEndState");
static_assert(offsetof(SharedTextureMemoryVkImageLayoutEndState, oldLayout) == offsetof(WGPUSharedTextureMemoryVkImageLayoutEndState, oldLayout),
        "offsetof mismatch for SharedTextureMemoryVkImageLayoutEndState::oldLayout");
static_assert(offsetof(SharedTextureMemoryVkImageLayoutEndState, newLayout) == offsetof(WGPUSharedTextureMemoryVkImageLayoutEndState, newLayout),
        "offsetof mismatch for SharedTextureMemoryVkImageLayoutEndState::newLayout");

// SharedTextureMemoryZirconHandleDescriptor implementation
SharedTextureMemoryZirconHandleDescriptor::SharedTextureMemoryZirconHandleDescriptor()
: ChainedStruct { nullptr, SType::SharedTextureMemoryZirconHandleDescriptor } {}
struct SharedTextureMemoryZirconHandleDescriptor::Init {
    ChainedStruct * const nextInChain;
    uint32_t memoryFD;
    uint64_t allocationSize;
};
SharedTextureMemoryZirconHandleDescriptor::SharedTextureMemoryZirconHandleDescriptor(SharedTextureMemoryZirconHandleDescriptor::Init&& init) :
    ChainedStruct { init.nextInChain, SType::SharedTextureMemoryZirconHandleDescriptor },
    memoryFD(std::move(init.memoryFD)),
    allocationSize(std::move(init.allocationSize)) {}

SharedTextureMemoryZirconHandleDescriptor::operator const WGPUSharedTextureMemoryZirconHandleDescriptor&() const noexcept {
    return *reinterpret_cast<const WGPUSharedTextureMemoryZirconHandleDescriptor*>(this);
}

static_assert(sizeof(SharedTextureMemoryZirconHandleDescriptor) == sizeof(WGPUSharedTextureMemoryZirconHandleDescriptor), "sizeof mismatch for SharedTextureMemoryZirconHandleDescriptor");
static_assert(alignof(SharedTextureMemoryZirconHandleDescriptor) == alignof(WGPUSharedTextureMemoryZirconHandleDescriptor), "alignof mismatch for SharedTextureMemoryZirconHandleDescriptor");
static_assert(offsetof(SharedTextureMemoryZirconHandleDescriptor, memoryFD) == offsetof(WGPUSharedTextureMemoryZirconHandleDescriptor, memoryFD),
        "offsetof mismatch for SharedTextureMemoryZirconHandleDescriptor::memoryFD");
static_assert(offsetof(SharedTextureMemoryZirconHandleDescriptor, allocationSize) == offsetof(WGPUSharedTextureMemoryZirconHandleDescriptor, allocationSize),
        "offsetof mismatch for SharedTextureMemoryZirconHandleDescriptor::allocationSize");

// StaticSamplerBindingLayout implementation
StaticSamplerBindingLayout::StaticSamplerBindingLayout()
: ChainedStruct { nullptr, SType::StaticSamplerBindingLayout } {}
struct StaticSamplerBindingLayout::Init {
    ChainedStruct * const nextInChain;
    Sampler sampler = nullptr;
    uint32_t sampledTextureBinding = kLimitU32Undefined;
};
StaticSamplerBindingLayout::StaticSamplerBindingLayout(StaticSamplerBindingLayout::Init&& init) :
    ChainedStruct { init.nextInChain, SType::StaticSamplerBindingLayout },
    sampler(std::move(init.sampler)),
    sampledTextureBinding(std::move(init.sampledTextureBinding)) {}

StaticSamplerBindingLayout::operator const WGPUStaticSamplerBindingLayout&() const noexcept {
    return *reinterpret_cast<const WGPUStaticSamplerBindingLayout*>(this);
}

static_assert(sizeof(StaticSamplerBindingLayout) == sizeof(WGPUStaticSamplerBindingLayout), "sizeof mismatch for StaticSamplerBindingLayout");
static_assert(alignof(StaticSamplerBindingLayout) == alignof(WGPUStaticSamplerBindingLayout), "alignof mismatch for StaticSamplerBindingLayout");
static_assert(offsetof(StaticSamplerBindingLayout, sampler) == offsetof(WGPUStaticSamplerBindingLayout, sampler),
        "offsetof mismatch for StaticSamplerBindingLayout::sampler");
static_assert(offsetof(StaticSamplerBindingLayout, sampledTextureBinding) == offsetof(WGPUStaticSamplerBindingLayout, sampledTextureBinding),
        "offsetof mismatch for StaticSamplerBindingLayout::sampledTextureBinding");

// StencilFaceState implementation

StencilFaceState::operator const WGPUStencilFaceState&() const noexcept {
    return *reinterpret_cast<const WGPUStencilFaceState*>(this);
}

static_assert(sizeof(StencilFaceState) == sizeof(WGPUStencilFaceState), "sizeof mismatch for StencilFaceState");
static_assert(alignof(StencilFaceState) == alignof(WGPUStencilFaceState), "alignof mismatch for StencilFaceState");
static_assert(offsetof(StencilFaceState, compare) == offsetof(WGPUStencilFaceState, compare),
        "offsetof mismatch for StencilFaceState::compare");
static_assert(offsetof(StencilFaceState, failOp) == offsetof(WGPUStencilFaceState, failOp),
        "offsetof mismatch for StencilFaceState::failOp");
static_assert(offsetof(StencilFaceState, depthFailOp) == offsetof(WGPUStencilFaceState, depthFailOp),
        "offsetof mismatch for StencilFaceState::depthFailOp");
static_assert(offsetof(StencilFaceState, passOp) == offsetof(WGPUStencilFaceState, passOp),
        "offsetof mismatch for StencilFaceState::passOp");

// StorageTextureBindingLayout implementation

StorageTextureBindingLayout::operator const WGPUStorageTextureBindingLayout&() const noexcept {
    return *reinterpret_cast<const WGPUStorageTextureBindingLayout*>(this);
}

static_assert(sizeof(StorageTextureBindingLayout) == sizeof(WGPUStorageTextureBindingLayout), "sizeof mismatch for StorageTextureBindingLayout");
static_assert(alignof(StorageTextureBindingLayout) == alignof(WGPUStorageTextureBindingLayout), "alignof mismatch for StorageTextureBindingLayout");
static_assert(offsetof(StorageTextureBindingLayout, nextInChain) == offsetof(WGPUStorageTextureBindingLayout, nextInChain),
        "offsetof mismatch for StorageTextureBindingLayout::nextInChain");
static_assert(offsetof(StorageTextureBindingLayout, access) == offsetof(WGPUStorageTextureBindingLayout, access),
        "offsetof mismatch for StorageTextureBindingLayout::access");
static_assert(offsetof(StorageTextureBindingLayout, format) == offsetof(WGPUStorageTextureBindingLayout, format),
        "offsetof mismatch for StorageTextureBindingLayout::format");
static_assert(offsetof(StorageTextureBindingLayout, viewDimension) == offsetof(WGPUStorageTextureBindingLayout, viewDimension),
        "offsetof mismatch for StorageTextureBindingLayout::viewDimension");

// SubgroupMatrixConfig implementation

SubgroupMatrixConfig::operator const WGPUSubgroupMatrixConfig&() const noexcept {
    return *reinterpret_cast<const WGPUSubgroupMatrixConfig*>(this);
}

static_assert(sizeof(SubgroupMatrixConfig) == sizeof(WGPUSubgroupMatrixConfig), "sizeof mismatch for SubgroupMatrixConfig");
static_assert(alignof(SubgroupMatrixConfig) == alignof(WGPUSubgroupMatrixConfig), "alignof mismatch for SubgroupMatrixConfig");
static_assert(offsetof(SubgroupMatrixConfig, componentType) == offsetof(WGPUSubgroupMatrixConfig, componentType),
        "offsetof mismatch for SubgroupMatrixConfig::componentType");
static_assert(offsetof(SubgroupMatrixConfig, resultComponentType) == offsetof(WGPUSubgroupMatrixConfig, resultComponentType),
        "offsetof mismatch for SubgroupMatrixConfig::resultComponentType");
static_assert(offsetof(SubgroupMatrixConfig, M) == offsetof(WGPUSubgroupMatrixConfig, M),
        "offsetof mismatch for SubgroupMatrixConfig::M");
static_assert(offsetof(SubgroupMatrixConfig, N) == offsetof(WGPUSubgroupMatrixConfig, N),
        "offsetof mismatch for SubgroupMatrixConfig::N");
static_assert(offsetof(SubgroupMatrixConfig, K) == offsetof(WGPUSubgroupMatrixConfig, K),
        "offsetof mismatch for SubgroupMatrixConfig::K");

// SupportedFeatures implementation
SupportedFeatures::SupportedFeatures() = default;
SupportedFeatures::~SupportedFeatures() {
    FreeMembers();
}

SupportedFeatures::SupportedFeatures(SupportedFeatures&& rhs)
    : featureCount(rhs.featureCount),
            features(rhs.features){
    Reset(rhs);
}

SupportedFeatures& SupportedFeatures::operator=(SupportedFeatures&& rhs) {
    if (&rhs == this) {
        return *this;
    }
    FreeMembers();
    detail::AsNonConstReference(this->featureCount) = std::move(rhs.featureCount);
    detail::AsNonConstReference(this->features) = std::move(rhs.features);
    Reset(rhs);
    return *this;
}

void SupportedFeatures::FreeMembers() {
    bool needsFreeing = false;    if (this->features != nullptr) { needsFreeing = true; }if (needsFreeing) {
        wgpuSupportedFeaturesFreeMembers(
            *reinterpret_cast<WGPUSupportedFeatures*>(this));
    }
}

// static
void SupportedFeatures::Reset(SupportedFeatures& value) {
    SupportedFeatures defaultValue{};
    detail::AsNonConstReference(value.featureCount) = defaultValue.featureCount;
    detail::AsNonConstReference(value.features) = defaultValue.features;
}

SupportedFeatures::operator const WGPUSupportedFeatures&() const noexcept {
    return *reinterpret_cast<const WGPUSupportedFeatures*>(this);
}

static_assert(sizeof(SupportedFeatures) == sizeof(WGPUSupportedFeatures), "sizeof mismatch for SupportedFeatures");
static_assert(alignof(SupportedFeatures) == alignof(WGPUSupportedFeatures), "alignof mismatch for SupportedFeatures");
static_assert(offsetof(SupportedFeatures, featureCount) == offsetof(WGPUSupportedFeatures, featureCount),
        "offsetof mismatch for SupportedFeatures::featureCount");
static_assert(offsetof(SupportedFeatures, features) == offsetof(WGPUSupportedFeatures, features),
        "offsetof mismatch for SupportedFeatures::features");

// SupportedInstanceFeatures implementation
SupportedInstanceFeatures::SupportedInstanceFeatures() = default;
SupportedInstanceFeatures::~SupportedInstanceFeatures() {
    FreeMembers();
}

SupportedInstanceFeatures::SupportedInstanceFeatures(SupportedInstanceFeatures&& rhs)
    : featureCount(rhs.featureCount),
            features(rhs.features){
    Reset(rhs);
}

SupportedInstanceFeatures& SupportedInstanceFeatures::operator=(SupportedInstanceFeatures&& rhs) {
    if (&rhs == this) {
        return *this;
    }
    FreeMembers();
    detail::AsNonConstReference(this->featureCount) = std::move(rhs.featureCount);
    detail::AsNonConstReference(this->features) = std::move(rhs.features);
    Reset(rhs);
    return *this;
}

void SupportedInstanceFeatures::FreeMembers() {
    bool needsFreeing = false;    if (this->features != nullptr) { needsFreeing = true; }if (needsFreeing) {
        wgpuSupportedInstanceFeaturesFreeMembers(
            *reinterpret_cast<WGPUSupportedInstanceFeatures*>(this));
    }
}

// static
void SupportedInstanceFeatures::Reset(SupportedInstanceFeatures& value) {
    SupportedInstanceFeatures defaultValue{};
    detail::AsNonConstReference(value.featureCount) = defaultValue.featureCount;
    detail::AsNonConstReference(value.features) = defaultValue.features;
}

SupportedInstanceFeatures::operator const WGPUSupportedInstanceFeatures&() const noexcept {
    return *reinterpret_cast<const WGPUSupportedInstanceFeatures*>(this);
}

static_assert(sizeof(SupportedInstanceFeatures) == sizeof(WGPUSupportedInstanceFeatures), "sizeof mismatch for SupportedInstanceFeatures");
static_assert(alignof(SupportedInstanceFeatures) == alignof(WGPUSupportedInstanceFeatures), "alignof mismatch for SupportedInstanceFeatures");
static_assert(offsetof(SupportedInstanceFeatures, featureCount) == offsetof(WGPUSupportedInstanceFeatures, featureCount),
        "offsetof mismatch for SupportedInstanceFeatures::featureCount");
static_assert(offsetof(SupportedInstanceFeatures, features) == offsetof(WGPUSupportedInstanceFeatures, features),
        "offsetof mismatch for SupportedInstanceFeatures::features");

// SupportedWGSLLanguageFeatures implementation
SupportedWGSLLanguageFeatures::SupportedWGSLLanguageFeatures() = default;
SupportedWGSLLanguageFeatures::~SupportedWGSLLanguageFeatures() {
    FreeMembers();
}

SupportedWGSLLanguageFeatures::SupportedWGSLLanguageFeatures(SupportedWGSLLanguageFeatures&& rhs)
    : featureCount(rhs.featureCount),
            features(rhs.features){
    Reset(rhs);
}

SupportedWGSLLanguageFeatures& SupportedWGSLLanguageFeatures::operator=(SupportedWGSLLanguageFeatures&& rhs) {
    if (&rhs == this) {
        return *this;
    }
    FreeMembers();
    detail::AsNonConstReference(this->featureCount) = std::move(rhs.featureCount);
    detail::AsNonConstReference(this->features) = std::move(rhs.features);
    Reset(rhs);
    return *this;
}

void SupportedWGSLLanguageFeatures::FreeMembers() {
    bool needsFreeing = false;    if (this->features != nullptr) { needsFreeing = true; }if (needsFreeing) {
        wgpuSupportedWGSLLanguageFeaturesFreeMembers(
            *reinterpret_cast<WGPUSupportedWGSLLanguageFeatures*>(this));
    }
}

// static
void SupportedWGSLLanguageFeatures::Reset(SupportedWGSLLanguageFeatures& value) {
    SupportedWGSLLanguageFeatures defaultValue{};
    detail::AsNonConstReference(value.featureCount) = defaultValue.featureCount;
    detail::AsNonConstReference(value.features) = defaultValue.features;
}

SupportedWGSLLanguageFeatures::operator const WGPUSupportedWGSLLanguageFeatures&() const noexcept {
    return *reinterpret_cast<const WGPUSupportedWGSLLanguageFeatures*>(this);
}

static_assert(sizeof(SupportedWGSLLanguageFeatures) == sizeof(WGPUSupportedWGSLLanguageFeatures), "sizeof mismatch for SupportedWGSLLanguageFeatures");
static_assert(alignof(SupportedWGSLLanguageFeatures) == alignof(WGPUSupportedWGSLLanguageFeatures), "alignof mismatch for SupportedWGSLLanguageFeatures");
static_assert(offsetof(SupportedWGSLLanguageFeatures, featureCount) == offsetof(WGPUSupportedWGSLLanguageFeatures, featureCount),
        "offsetof mismatch for SupportedWGSLLanguageFeatures::featureCount");
static_assert(offsetof(SupportedWGSLLanguageFeatures, features) == offsetof(WGPUSupportedWGSLLanguageFeatures, features),
        "offsetof mismatch for SupportedWGSLLanguageFeatures::features");

// SurfaceCapabilities implementation
SurfaceCapabilities::SurfaceCapabilities() = default;
SurfaceCapabilities::~SurfaceCapabilities() {
    FreeMembers();
}

SurfaceCapabilities::SurfaceCapabilities(SurfaceCapabilities&& rhs)
    : usages(rhs.usages),
            formatCount(rhs.formatCount),
            formats(rhs.formats),
            presentModeCount(rhs.presentModeCount),
            presentModes(rhs.presentModes),
            alphaModeCount(rhs.alphaModeCount),
            alphaModes(rhs.alphaModes){
    Reset(rhs);
}

SurfaceCapabilities& SurfaceCapabilities::operator=(SurfaceCapabilities&& rhs) {
    if (&rhs == this) {
        return *this;
    }
    FreeMembers();
    detail::AsNonConstReference(this->usages) = std::move(rhs.usages);
    detail::AsNonConstReference(this->formatCount) = std::move(rhs.formatCount);
    detail::AsNonConstReference(this->formats) = std::move(rhs.formats);
    detail::AsNonConstReference(this->presentModeCount) = std::move(rhs.presentModeCount);
    detail::AsNonConstReference(this->presentModes) = std::move(rhs.presentModes);
    detail::AsNonConstReference(this->alphaModeCount) = std::move(rhs.alphaModeCount);
    detail::AsNonConstReference(this->alphaModes) = std::move(rhs.alphaModes);
    Reset(rhs);
    return *this;
}

void SurfaceCapabilities::FreeMembers() {
    bool needsFreeing = false;    if (this->formats != nullptr) { needsFreeing = true; }    if (this->presentModes != nullptr) { needsFreeing = true; }    if (this->alphaModes != nullptr) { needsFreeing = true; }if (needsFreeing) {
        wgpuSurfaceCapabilitiesFreeMembers(
            *reinterpret_cast<WGPUSurfaceCapabilities*>(this));
    }
}

// static
void SurfaceCapabilities::Reset(SurfaceCapabilities& value) {
    SurfaceCapabilities defaultValue{};
    detail::AsNonConstReference(value.usages) = defaultValue.usages;
    detail::AsNonConstReference(value.formatCount) = defaultValue.formatCount;
    detail::AsNonConstReference(value.formats) = defaultValue.formats;
    detail::AsNonConstReference(value.presentModeCount) = defaultValue.presentModeCount;
    detail::AsNonConstReference(value.presentModes) = defaultValue.presentModes;
    detail::AsNonConstReference(value.alphaModeCount) = defaultValue.alphaModeCount;
    detail::AsNonConstReference(value.alphaModes) = defaultValue.alphaModes;
}

SurfaceCapabilities::operator const WGPUSurfaceCapabilities&() const noexcept {
    return *reinterpret_cast<const WGPUSurfaceCapabilities*>(this);
}

static_assert(sizeof(SurfaceCapabilities) == sizeof(WGPUSurfaceCapabilities), "sizeof mismatch for SurfaceCapabilities");
static_assert(alignof(SurfaceCapabilities) == alignof(WGPUSurfaceCapabilities), "alignof mismatch for SurfaceCapabilities");
static_assert(offsetof(SurfaceCapabilities, nextInChain) == offsetof(WGPUSurfaceCapabilities, nextInChain),
        "offsetof mismatch for SurfaceCapabilities::nextInChain");
static_assert(offsetof(SurfaceCapabilities, usages) == offsetof(WGPUSurfaceCapabilities, usages),
        "offsetof mismatch for SurfaceCapabilities::usages");
static_assert(offsetof(SurfaceCapabilities, formatCount) == offsetof(WGPUSurfaceCapabilities, formatCount),
        "offsetof mismatch for SurfaceCapabilities::formatCount");
static_assert(offsetof(SurfaceCapabilities, formats) == offsetof(WGPUSurfaceCapabilities, formats),
        "offsetof mismatch for SurfaceCapabilities::formats");
static_assert(offsetof(SurfaceCapabilities, presentModeCount) == offsetof(WGPUSurfaceCapabilities, presentModeCount),
        "offsetof mismatch for SurfaceCapabilities::presentModeCount");
static_assert(offsetof(SurfaceCapabilities, presentModes) == offsetof(WGPUSurfaceCapabilities, presentModes),
        "offsetof mismatch for SurfaceCapabilities::presentModes");
static_assert(offsetof(SurfaceCapabilities, alphaModeCount) == offsetof(WGPUSurfaceCapabilities, alphaModeCount),
        "offsetof mismatch for SurfaceCapabilities::alphaModeCount");
static_assert(offsetof(SurfaceCapabilities, alphaModes) == offsetof(WGPUSurfaceCapabilities, alphaModes),
        "offsetof mismatch for SurfaceCapabilities::alphaModes");

// SurfaceColorManagement implementation
SurfaceColorManagement::SurfaceColorManagement()
: ChainedStruct { nullptr, SType::SurfaceColorManagement } {}
struct SurfaceColorManagement::Init {
    ChainedStruct * const nextInChain;
    PredefinedColorSpace colorSpace = {};
    ToneMappingMode toneMappingMode = {};
};
SurfaceColorManagement::SurfaceColorManagement(SurfaceColorManagement::Init&& init) :
    ChainedStruct { init.nextInChain, SType::SurfaceColorManagement },
    colorSpace(std::move(init.colorSpace)),
    toneMappingMode(std::move(init.toneMappingMode)) {}

SurfaceColorManagement::operator const WGPUSurfaceColorManagement&() const noexcept {
    return *reinterpret_cast<const WGPUSurfaceColorManagement*>(this);
}

static_assert(sizeof(SurfaceColorManagement) == sizeof(WGPUSurfaceColorManagement), "sizeof mismatch for SurfaceColorManagement");
static_assert(alignof(SurfaceColorManagement) == alignof(WGPUSurfaceColorManagement), "alignof mismatch for SurfaceColorManagement");
static_assert(offsetof(SurfaceColorManagement, colorSpace) == offsetof(WGPUSurfaceColorManagement, colorSpace),
        "offsetof mismatch for SurfaceColorManagement::colorSpace");
static_assert(offsetof(SurfaceColorManagement, toneMappingMode) == offsetof(WGPUSurfaceColorManagement, toneMappingMode),
        "offsetof mismatch for SurfaceColorManagement::toneMappingMode");

// SurfaceConfiguration implementation

SurfaceConfiguration::operator const WGPUSurfaceConfiguration&() const noexcept {
    return *reinterpret_cast<const WGPUSurfaceConfiguration*>(this);
}

static_assert(sizeof(SurfaceConfiguration) == sizeof(WGPUSurfaceConfiguration), "sizeof mismatch for SurfaceConfiguration");
static_assert(alignof(SurfaceConfiguration) == alignof(WGPUSurfaceConfiguration), "alignof mismatch for SurfaceConfiguration");
static_assert(offsetof(SurfaceConfiguration, nextInChain) == offsetof(WGPUSurfaceConfiguration, nextInChain),
        "offsetof mismatch for SurfaceConfiguration::nextInChain");
static_assert(offsetof(SurfaceConfiguration, device) == offsetof(WGPUSurfaceConfiguration, device),
        "offsetof mismatch for SurfaceConfiguration::device");
static_assert(offsetof(SurfaceConfiguration, format) == offsetof(WGPUSurfaceConfiguration, format),
        "offsetof mismatch for SurfaceConfiguration::format");
static_assert(offsetof(SurfaceConfiguration, usage) == offsetof(WGPUSurfaceConfiguration, usage),
        "offsetof mismatch for SurfaceConfiguration::usage");
static_assert(offsetof(SurfaceConfiguration, width) == offsetof(WGPUSurfaceConfiguration, width),
        "offsetof mismatch for SurfaceConfiguration::width");
static_assert(offsetof(SurfaceConfiguration, height) == offsetof(WGPUSurfaceConfiguration, height),
        "offsetof mismatch for SurfaceConfiguration::height");
static_assert(offsetof(SurfaceConfiguration, viewFormatCount) == offsetof(WGPUSurfaceConfiguration, viewFormatCount),
        "offsetof mismatch for SurfaceConfiguration::viewFormatCount");
static_assert(offsetof(SurfaceConfiguration, viewFormats) == offsetof(WGPUSurfaceConfiguration, viewFormats),
        "offsetof mismatch for SurfaceConfiguration::viewFormats");
static_assert(offsetof(SurfaceConfiguration, alphaMode) == offsetof(WGPUSurfaceConfiguration, alphaMode),
        "offsetof mismatch for SurfaceConfiguration::alphaMode");
static_assert(offsetof(SurfaceConfiguration, presentMode) == offsetof(WGPUSurfaceConfiguration, presentMode),
        "offsetof mismatch for SurfaceConfiguration::presentMode");

// SurfaceDescriptorFromWindowsCoreWindow implementation
SurfaceDescriptorFromWindowsCoreWindow::SurfaceDescriptorFromWindowsCoreWindow()
: ChainedStruct { nullptr, SType::SurfaceDescriptorFromWindowsCoreWindow } {}
struct SurfaceDescriptorFromWindowsCoreWindow::Init {
    ChainedStruct * const nextInChain;
    void * coreWindow = nullptr;
};
SurfaceDescriptorFromWindowsCoreWindow::SurfaceDescriptorFromWindowsCoreWindow(SurfaceDescriptorFromWindowsCoreWindow::Init&& init) :
    ChainedStruct { init.nextInChain, SType::SurfaceDescriptorFromWindowsCoreWindow },
    coreWindow(std::move(init.coreWindow)) {}

SurfaceDescriptorFromWindowsCoreWindow::operator const WGPUSurfaceDescriptorFromWindowsCoreWindow&() const noexcept {
    return *reinterpret_cast<const WGPUSurfaceDescriptorFromWindowsCoreWindow*>(this);
}

static_assert(sizeof(SurfaceDescriptorFromWindowsCoreWindow) == sizeof(WGPUSurfaceDescriptorFromWindowsCoreWindow), "sizeof mismatch for SurfaceDescriptorFromWindowsCoreWindow");
static_assert(alignof(SurfaceDescriptorFromWindowsCoreWindow) == alignof(WGPUSurfaceDescriptorFromWindowsCoreWindow), "alignof mismatch for SurfaceDescriptorFromWindowsCoreWindow");
static_assert(offsetof(SurfaceDescriptorFromWindowsCoreWindow, coreWindow) == offsetof(WGPUSurfaceDescriptorFromWindowsCoreWindow, coreWindow),
        "offsetof mismatch for SurfaceDescriptorFromWindowsCoreWindow::coreWindow");

// SurfaceDescriptorFromWindowsUWPSwapChainPanel implementation
SurfaceDescriptorFromWindowsUWPSwapChainPanel::SurfaceDescriptorFromWindowsUWPSwapChainPanel()
: ChainedStruct { nullptr, SType::SurfaceDescriptorFromWindowsUWPSwapChainPanel } {}
struct SurfaceDescriptorFromWindowsUWPSwapChainPanel::Init {
    ChainedStruct * const nextInChain;
    void * swapChainPanel = nullptr;
};
SurfaceDescriptorFromWindowsUWPSwapChainPanel::SurfaceDescriptorFromWindowsUWPSwapChainPanel(SurfaceDescriptorFromWindowsUWPSwapChainPanel::Init&& init) :
    ChainedStruct { init.nextInChain, SType::SurfaceDescriptorFromWindowsUWPSwapChainPanel },
    swapChainPanel(std::move(init.swapChainPanel)) {}

SurfaceDescriptorFromWindowsUWPSwapChainPanel::operator const WGPUSurfaceDescriptorFromWindowsUWPSwapChainPanel&() const noexcept {
    return *reinterpret_cast<const WGPUSurfaceDescriptorFromWindowsUWPSwapChainPanel*>(this);
}

static_assert(sizeof(SurfaceDescriptorFromWindowsUWPSwapChainPanel) == sizeof(WGPUSurfaceDescriptorFromWindowsUWPSwapChainPanel), "sizeof mismatch for SurfaceDescriptorFromWindowsUWPSwapChainPanel");
static_assert(alignof(SurfaceDescriptorFromWindowsUWPSwapChainPanel) == alignof(WGPUSurfaceDescriptorFromWindowsUWPSwapChainPanel), "alignof mismatch for SurfaceDescriptorFromWindowsUWPSwapChainPanel");
static_assert(offsetof(SurfaceDescriptorFromWindowsUWPSwapChainPanel, swapChainPanel) == offsetof(WGPUSurfaceDescriptorFromWindowsUWPSwapChainPanel, swapChainPanel),
        "offsetof mismatch for SurfaceDescriptorFromWindowsUWPSwapChainPanel::swapChainPanel");

// SurfaceDescriptorFromWindowsWinUISwapChainPanel implementation
SurfaceDescriptorFromWindowsWinUISwapChainPanel::SurfaceDescriptorFromWindowsWinUISwapChainPanel()
: ChainedStruct { nullptr, SType::SurfaceDescriptorFromWindowsWinUISwapChainPanel } {}
struct SurfaceDescriptorFromWindowsWinUISwapChainPanel::Init {
    ChainedStruct * const nextInChain;
    void * swapChainPanel = nullptr;
};
SurfaceDescriptorFromWindowsWinUISwapChainPanel::SurfaceDescriptorFromWindowsWinUISwapChainPanel(SurfaceDescriptorFromWindowsWinUISwapChainPanel::Init&& init) :
    ChainedStruct { init.nextInChain, SType::SurfaceDescriptorFromWindowsWinUISwapChainPanel },
    swapChainPanel(std::move(init.swapChainPanel)) {}

SurfaceDescriptorFromWindowsWinUISwapChainPanel::operator const WGPUSurfaceDescriptorFromWindowsWinUISwapChainPanel&() const noexcept {
    return *reinterpret_cast<const WGPUSurfaceDescriptorFromWindowsWinUISwapChainPanel*>(this);
}

static_assert(sizeof(SurfaceDescriptorFromWindowsWinUISwapChainPanel) == sizeof(WGPUSurfaceDescriptorFromWindowsWinUISwapChainPanel), "sizeof mismatch for SurfaceDescriptorFromWindowsWinUISwapChainPanel");
static_assert(alignof(SurfaceDescriptorFromWindowsWinUISwapChainPanel) == alignof(WGPUSurfaceDescriptorFromWindowsWinUISwapChainPanel), "alignof mismatch for SurfaceDescriptorFromWindowsWinUISwapChainPanel");
static_assert(offsetof(SurfaceDescriptorFromWindowsWinUISwapChainPanel, swapChainPanel) == offsetof(WGPUSurfaceDescriptorFromWindowsWinUISwapChainPanel, swapChainPanel),
        "offsetof mismatch for SurfaceDescriptorFromWindowsWinUISwapChainPanel::swapChainPanel");

// SurfaceSourceAndroidNativeWindow implementation
SurfaceSourceAndroidNativeWindow::SurfaceSourceAndroidNativeWindow()
: ChainedStruct { nullptr, SType::SurfaceSourceAndroidNativeWindow } {}
struct SurfaceSourceAndroidNativeWindow::Init {
    ChainedStruct * const nextInChain;
    void * window;
};
SurfaceSourceAndroidNativeWindow::SurfaceSourceAndroidNativeWindow(SurfaceSourceAndroidNativeWindow::Init&& init) :
    ChainedStruct { init.nextInChain, SType::SurfaceSourceAndroidNativeWindow },
    window(std::move(init.window)) {}

SurfaceSourceAndroidNativeWindow::operator const WGPUSurfaceSourceAndroidNativeWindow&() const noexcept {
    return *reinterpret_cast<const WGPUSurfaceSourceAndroidNativeWindow*>(this);
}

static_assert(sizeof(SurfaceSourceAndroidNativeWindow) == sizeof(WGPUSurfaceSourceAndroidNativeWindow), "sizeof mismatch for SurfaceSourceAndroidNativeWindow");
static_assert(alignof(SurfaceSourceAndroidNativeWindow) == alignof(WGPUSurfaceSourceAndroidNativeWindow), "alignof mismatch for SurfaceSourceAndroidNativeWindow");
static_assert(offsetof(SurfaceSourceAndroidNativeWindow, window) == offsetof(WGPUSurfaceSourceAndroidNativeWindow, window),
        "offsetof mismatch for SurfaceSourceAndroidNativeWindow::window");

// SurfaceSourceMetalLayer implementation
SurfaceSourceMetalLayer::SurfaceSourceMetalLayer()
: ChainedStruct { nullptr, SType::SurfaceSourceMetalLayer } {}
struct SurfaceSourceMetalLayer::Init {
    ChainedStruct * const nextInChain;
    void * layer = nullptr;
};
SurfaceSourceMetalLayer::SurfaceSourceMetalLayer(SurfaceSourceMetalLayer::Init&& init) :
    ChainedStruct { init.nextInChain, SType::SurfaceSourceMetalLayer },
    layer(std::move(init.layer)) {}

SurfaceSourceMetalLayer::operator const WGPUSurfaceSourceMetalLayer&() const noexcept {
    return *reinterpret_cast<const WGPUSurfaceSourceMetalLayer*>(this);
}

static_assert(sizeof(SurfaceSourceMetalLayer) == sizeof(WGPUSurfaceSourceMetalLayer), "sizeof mismatch for SurfaceSourceMetalLayer");
static_assert(alignof(SurfaceSourceMetalLayer) == alignof(WGPUSurfaceSourceMetalLayer), "alignof mismatch for SurfaceSourceMetalLayer");
static_assert(offsetof(SurfaceSourceMetalLayer, layer) == offsetof(WGPUSurfaceSourceMetalLayer, layer),
        "offsetof mismatch for SurfaceSourceMetalLayer::layer");

// SurfaceSourceWaylandSurface implementation
SurfaceSourceWaylandSurface::SurfaceSourceWaylandSurface()
: ChainedStruct { nullptr, SType::SurfaceSourceWaylandSurface } {}
struct SurfaceSourceWaylandSurface::Init {
    ChainedStruct * const nextInChain;
    void * display = nullptr;
    void * surface = nullptr;
};
SurfaceSourceWaylandSurface::SurfaceSourceWaylandSurface(SurfaceSourceWaylandSurface::Init&& init) :
    ChainedStruct { init.nextInChain, SType::SurfaceSourceWaylandSurface },
    display(std::move(init.display)),
    surface(std::move(init.surface)) {}

SurfaceSourceWaylandSurface::operator const WGPUSurfaceSourceWaylandSurface&() const noexcept {
    return *reinterpret_cast<const WGPUSurfaceSourceWaylandSurface*>(this);
}

static_assert(sizeof(SurfaceSourceWaylandSurface) == sizeof(WGPUSurfaceSourceWaylandSurface), "sizeof mismatch for SurfaceSourceWaylandSurface");
static_assert(alignof(SurfaceSourceWaylandSurface) == alignof(WGPUSurfaceSourceWaylandSurface), "alignof mismatch for SurfaceSourceWaylandSurface");
static_assert(offsetof(SurfaceSourceWaylandSurface, display) == offsetof(WGPUSurfaceSourceWaylandSurface, display),
        "offsetof mismatch for SurfaceSourceWaylandSurface::display");
static_assert(offsetof(SurfaceSourceWaylandSurface, surface) == offsetof(WGPUSurfaceSourceWaylandSurface, surface),
        "offsetof mismatch for SurfaceSourceWaylandSurface::surface");

// SurfaceSourceWindowsHWND implementation
SurfaceSourceWindowsHWND::SurfaceSourceWindowsHWND()
: ChainedStruct { nullptr, SType::SurfaceSourceWindowsHWND } {}
struct SurfaceSourceWindowsHWND::Init {
    ChainedStruct * const nextInChain;
    void * hinstance = nullptr;
    void * hwnd = nullptr;
};
SurfaceSourceWindowsHWND::SurfaceSourceWindowsHWND(SurfaceSourceWindowsHWND::Init&& init) :
    ChainedStruct { init.nextInChain, SType::SurfaceSourceWindowsHWND },
    hinstance(std::move(init.hinstance)),
    hwnd(std::move(init.hwnd)) {}

SurfaceSourceWindowsHWND::operator const WGPUSurfaceSourceWindowsHWND&() const noexcept {
    return *reinterpret_cast<const WGPUSurfaceSourceWindowsHWND*>(this);
}

static_assert(sizeof(SurfaceSourceWindowsHWND) == sizeof(WGPUSurfaceSourceWindowsHWND), "sizeof mismatch for SurfaceSourceWindowsHWND");
static_assert(alignof(SurfaceSourceWindowsHWND) == alignof(WGPUSurfaceSourceWindowsHWND), "alignof mismatch for SurfaceSourceWindowsHWND");
static_assert(offsetof(SurfaceSourceWindowsHWND, hinstance) == offsetof(WGPUSurfaceSourceWindowsHWND, hinstance),
        "offsetof mismatch for SurfaceSourceWindowsHWND::hinstance");
static_assert(offsetof(SurfaceSourceWindowsHWND, hwnd) == offsetof(WGPUSurfaceSourceWindowsHWND, hwnd),
        "offsetof mismatch for SurfaceSourceWindowsHWND::hwnd");

// SurfaceSourceXCBWindow implementation
SurfaceSourceXCBWindow::SurfaceSourceXCBWindow()
: ChainedStruct { nullptr, SType::SurfaceSourceXCBWindow } {}
struct SurfaceSourceXCBWindow::Init {
    ChainedStruct * const nextInChain;
    void * connection = nullptr;
    uint32_t window;
};
SurfaceSourceXCBWindow::SurfaceSourceXCBWindow(SurfaceSourceXCBWindow::Init&& init) :
    ChainedStruct { init.nextInChain, SType::SurfaceSourceXCBWindow },
    connection(std::move(init.connection)),
    window(std::move(init.window)) {}

SurfaceSourceXCBWindow::operator const WGPUSurfaceSourceXCBWindow&() const noexcept {
    return *reinterpret_cast<const WGPUSurfaceSourceXCBWindow*>(this);
}

static_assert(sizeof(SurfaceSourceXCBWindow) == sizeof(WGPUSurfaceSourceXCBWindow), "sizeof mismatch for SurfaceSourceXCBWindow");
static_assert(alignof(SurfaceSourceXCBWindow) == alignof(WGPUSurfaceSourceXCBWindow), "alignof mismatch for SurfaceSourceXCBWindow");
static_assert(offsetof(SurfaceSourceXCBWindow, connection) == offsetof(WGPUSurfaceSourceXCBWindow, connection),
        "offsetof mismatch for SurfaceSourceXCBWindow::connection");
static_assert(offsetof(SurfaceSourceXCBWindow, window) == offsetof(WGPUSurfaceSourceXCBWindow, window),
        "offsetof mismatch for SurfaceSourceXCBWindow::window");

// SurfaceSourceXlibWindow implementation
SurfaceSourceXlibWindow::SurfaceSourceXlibWindow()
: ChainedStruct { nullptr, SType::SurfaceSourceXlibWindow } {}
struct SurfaceSourceXlibWindow::Init {
    ChainedStruct * const nextInChain;
    void * display = nullptr;
    uint64_t window;
};
SurfaceSourceXlibWindow::SurfaceSourceXlibWindow(SurfaceSourceXlibWindow::Init&& init) :
    ChainedStruct { init.nextInChain, SType::SurfaceSourceXlibWindow },
    display(std::move(init.display)),
    window(std::move(init.window)) {}

SurfaceSourceXlibWindow::operator const WGPUSurfaceSourceXlibWindow&() const noexcept {
    return *reinterpret_cast<const WGPUSurfaceSourceXlibWindow*>(this);
}

static_assert(sizeof(SurfaceSourceXlibWindow) == sizeof(WGPUSurfaceSourceXlibWindow), "sizeof mismatch for SurfaceSourceXlibWindow");
static_assert(alignof(SurfaceSourceXlibWindow) == alignof(WGPUSurfaceSourceXlibWindow), "alignof mismatch for SurfaceSourceXlibWindow");
static_assert(offsetof(SurfaceSourceXlibWindow, display) == offsetof(WGPUSurfaceSourceXlibWindow, display),
        "offsetof mismatch for SurfaceSourceXlibWindow::display");
static_assert(offsetof(SurfaceSourceXlibWindow, window) == offsetof(WGPUSurfaceSourceXlibWindow, window),
        "offsetof mismatch for SurfaceSourceXlibWindow::window");

// SurfaceTexture implementation

SurfaceTexture::operator const WGPUSurfaceTexture&() const noexcept {
    return *reinterpret_cast<const WGPUSurfaceTexture*>(this);
}

static_assert(sizeof(SurfaceTexture) == sizeof(WGPUSurfaceTexture), "sizeof mismatch for SurfaceTexture");
static_assert(alignof(SurfaceTexture) == alignof(WGPUSurfaceTexture), "alignof mismatch for SurfaceTexture");
static_assert(offsetof(SurfaceTexture, nextInChain) == offsetof(WGPUSurfaceTexture, nextInChain),
        "offsetof mismatch for SurfaceTexture::nextInChain");
static_assert(offsetof(SurfaceTexture, texture) == offsetof(WGPUSurfaceTexture, texture),
        "offsetof mismatch for SurfaceTexture::texture");
static_assert(offsetof(SurfaceTexture, status) == offsetof(WGPUSurfaceTexture, status),
        "offsetof mismatch for SurfaceTexture::status");

// TexelBufferBindingEntry implementation
TexelBufferBindingEntry::TexelBufferBindingEntry()
: ChainedStruct { nullptr, SType::TexelBufferBindingEntry } {}
struct TexelBufferBindingEntry::Init {
    ChainedStruct * const nextInChain;
    TexelBufferView texelBufferView = nullptr;
};
TexelBufferBindingEntry::TexelBufferBindingEntry(TexelBufferBindingEntry::Init&& init) :
    ChainedStruct { init.nextInChain, SType::TexelBufferBindingEntry },
    texelBufferView(std::move(init.texelBufferView)) {}

TexelBufferBindingEntry::operator const WGPUTexelBufferBindingEntry&() const noexcept {
    return *reinterpret_cast<const WGPUTexelBufferBindingEntry*>(this);
}

static_assert(sizeof(TexelBufferBindingEntry) == sizeof(WGPUTexelBufferBindingEntry), "sizeof mismatch for TexelBufferBindingEntry");
static_assert(alignof(TexelBufferBindingEntry) == alignof(WGPUTexelBufferBindingEntry), "alignof mismatch for TexelBufferBindingEntry");
static_assert(offsetof(TexelBufferBindingEntry, texelBufferView) == offsetof(WGPUTexelBufferBindingEntry, texelBufferView),
        "offsetof mismatch for TexelBufferBindingEntry::texelBufferView");

// TexelBufferBindingLayout implementation
TexelBufferBindingLayout::TexelBufferBindingLayout()
: ChainedStruct { nullptr, SType::TexelBufferBindingLayout } {}
struct TexelBufferBindingLayout::Init {
    ChainedStruct * const nextInChain;
    TexelBufferAccess access = TexelBufferAccess::Undefined;
    TextureFormat format = TextureFormat::Undefined;
};
TexelBufferBindingLayout::TexelBufferBindingLayout(TexelBufferBindingLayout::Init&& init) :
    ChainedStruct { init.nextInChain, SType::TexelBufferBindingLayout },
    access(std::move(init.access)),
    format(std::move(init.format)) {}

TexelBufferBindingLayout::operator const WGPUTexelBufferBindingLayout&() const noexcept {
    return *reinterpret_cast<const WGPUTexelBufferBindingLayout*>(this);
}

static_assert(sizeof(TexelBufferBindingLayout) == sizeof(WGPUTexelBufferBindingLayout), "sizeof mismatch for TexelBufferBindingLayout");
static_assert(alignof(TexelBufferBindingLayout) == alignof(WGPUTexelBufferBindingLayout), "alignof mismatch for TexelBufferBindingLayout");
static_assert(offsetof(TexelBufferBindingLayout, access) == offsetof(WGPUTexelBufferBindingLayout, access),
        "offsetof mismatch for TexelBufferBindingLayout::access");
static_assert(offsetof(TexelBufferBindingLayout, format) == offsetof(WGPUTexelBufferBindingLayout, format),
        "offsetof mismatch for TexelBufferBindingLayout::format");

// TexelBufferViewDescriptor implementation

TexelBufferViewDescriptor::operator const WGPUTexelBufferViewDescriptor&() const noexcept {
    return *reinterpret_cast<const WGPUTexelBufferViewDescriptor*>(this);
}

static_assert(sizeof(TexelBufferViewDescriptor) == sizeof(WGPUTexelBufferViewDescriptor), "sizeof mismatch for TexelBufferViewDescriptor");
static_assert(alignof(TexelBufferViewDescriptor) == alignof(WGPUTexelBufferViewDescriptor), "alignof mismatch for TexelBufferViewDescriptor");
static_assert(offsetof(TexelBufferViewDescriptor, nextInChain) == offsetof(WGPUTexelBufferViewDescriptor, nextInChain),
        "offsetof mismatch for TexelBufferViewDescriptor::nextInChain");
static_assert(offsetof(TexelBufferViewDescriptor, label) == offsetof(WGPUTexelBufferViewDescriptor, label),
        "offsetof mismatch for TexelBufferViewDescriptor::label");
static_assert(offsetof(TexelBufferViewDescriptor, format) == offsetof(WGPUTexelBufferViewDescriptor, format),
        "offsetof mismatch for TexelBufferViewDescriptor::format");
static_assert(offsetof(TexelBufferViewDescriptor, offset) == offsetof(WGPUTexelBufferViewDescriptor, offset),
        "offsetof mismatch for TexelBufferViewDescriptor::offset");
static_assert(offsetof(TexelBufferViewDescriptor, size) == offsetof(WGPUTexelBufferViewDescriptor, size),
        "offsetof mismatch for TexelBufferViewDescriptor::size");

// TexelCopyBufferLayout implementation

TexelCopyBufferLayout::operator const WGPUTexelCopyBufferLayout&() const noexcept {
    return *reinterpret_cast<const WGPUTexelCopyBufferLayout*>(this);
}

static_assert(sizeof(TexelCopyBufferLayout) == sizeof(WGPUTexelCopyBufferLayout), "sizeof mismatch for TexelCopyBufferLayout");
static_assert(alignof(TexelCopyBufferLayout) == alignof(WGPUTexelCopyBufferLayout), "alignof mismatch for TexelCopyBufferLayout");
static_assert(offsetof(TexelCopyBufferLayout, offset) == offsetof(WGPUTexelCopyBufferLayout, offset),
        "offsetof mismatch for TexelCopyBufferLayout::offset");
static_assert(offsetof(TexelCopyBufferLayout, bytesPerRow) == offsetof(WGPUTexelCopyBufferLayout, bytesPerRow),
        "offsetof mismatch for TexelCopyBufferLayout::bytesPerRow");
static_assert(offsetof(TexelCopyBufferLayout, rowsPerImage) == offsetof(WGPUTexelCopyBufferLayout, rowsPerImage),
        "offsetof mismatch for TexelCopyBufferLayout::rowsPerImage");

// TextureBindingLayout implementation

TextureBindingLayout::operator const WGPUTextureBindingLayout&() const noexcept {
    return *reinterpret_cast<const WGPUTextureBindingLayout*>(this);
}

static_assert(sizeof(TextureBindingLayout) == sizeof(WGPUTextureBindingLayout), "sizeof mismatch for TextureBindingLayout");
static_assert(alignof(TextureBindingLayout) == alignof(WGPUTextureBindingLayout), "alignof mismatch for TextureBindingLayout");
static_assert(offsetof(TextureBindingLayout, nextInChain) == offsetof(WGPUTextureBindingLayout, nextInChain),
        "offsetof mismatch for TextureBindingLayout::nextInChain");
static_assert(offsetof(TextureBindingLayout, sampleType) == offsetof(WGPUTextureBindingLayout, sampleType),
        "offsetof mismatch for TextureBindingLayout::sampleType");
static_assert(offsetof(TextureBindingLayout, viewDimension) == offsetof(WGPUTextureBindingLayout, viewDimension),
        "offsetof mismatch for TextureBindingLayout::viewDimension");
static_assert(offsetof(TextureBindingLayout, multisampled) == offsetof(WGPUTextureBindingLayout, multisampled),
        "offsetof mismatch for TextureBindingLayout::multisampled");

// TextureBindingViewDimension implementation
TextureBindingViewDimension::TextureBindingViewDimension()
: ChainedStruct { nullptr, SType::TextureBindingViewDimension } {}
struct TextureBindingViewDimension::Init {
    ChainedStruct * const nextInChain;
    TextureViewDimension textureBindingViewDimension = TextureViewDimension::Undefined;
};
TextureBindingViewDimension::TextureBindingViewDimension(TextureBindingViewDimension::Init&& init) :
    ChainedStruct { init.nextInChain, SType::TextureBindingViewDimension },
    textureBindingViewDimension(std::move(init.textureBindingViewDimension)) {}

TextureBindingViewDimension::operator const WGPUTextureBindingViewDimension&() const noexcept {
    return *reinterpret_cast<const WGPUTextureBindingViewDimension*>(this);
}

static_assert(sizeof(TextureBindingViewDimension) == sizeof(WGPUTextureBindingViewDimension), "sizeof mismatch for TextureBindingViewDimension");
static_assert(alignof(TextureBindingViewDimension) == alignof(WGPUTextureBindingViewDimension), "alignof mismatch for TextureBindingViewDimension");
static_assert(offsetof(TextureBindingViewDimension, textureBindingViewDimension) == offsetof(WGPUTextureBindingViewDimension, textureBindingViewDimension),
        "offsetof mismatch for TextureBindingViewDimension::textureBindingViewDimension");

// TextureComponentSwizzle implementation

TextureComponentSwizzle::operator const WGPUTextureComponentSwizzle&() const noexcept {
    return *reinterpret_cast<const WGPUTextureComponentSwizzle*>(this);
}

static_assert(sizeof(TextureComponentSwizzle) == sizeof(WGPUTextureComponentSwizzle), "sizeof mismatch for TextureComponentSwizzle");
static_assert(alignof(TextureComponentSwizzle) == alignof(WGPUTextureComponentSwizzle), "alignof mismatch for TextureComponentSwizzle");
static_assert(offsetof(TextureComponentSwizzle, r) == offsetof(WGPUTextureComponentSwizzle, r),
        "offsetof mismatch for TextureComponentSwizzle::r");
static_assert(offsetof(TextureComponentSwizzle, g) == offsetof(WGPUTextureComponentSwizzle, g),
        "offsetof mismatch for TextureComponentSwizzle::g");
static_assert(offsetof(TextureComponentSwizzle, b) == offsetof(WGPUTextureComponentSwizzle, b),
        "offsetof mismatch for TextureComponentSwizzle::b");
static_assert(offsetof(TextureComponentSwizzle, a) == offsetof(WGPUTextureComponentSwizzle, a),
        "offsetof mismatch for TextureComponentSwizzle::a");

// VertexAttribute implementation

VertexAttribute::operator const WGPUVertexAttribute&() const noexcept {
    return *reinterpret_cast<const WGPUVertexAttribute*>(this);
}

static_assert(sizeof(VertexAttribute) == sizeof(WGPUVertexAttribute), "sizeof mismatch for VertexAttribute");
static_assert(alignof(VertexAttribute) == alignof(WGPUVertexAttribute), "alignof mismatch for VertexAttribute");
static_assert(offsetof(VertexAttribute, nextInChain) == offsetof(WGPUVertexAttribute, nextInChain),
        "offsetof mismatch for VertexAttribute::nextInChain");
static_assert(offsetof(VertexAttribute, format) == offsetof(WGPUVertexAttribute, format),
        "offsetof mismatch for VertexAttribute::format");
static_assert(offsetof(VertexAttribute, offset) == offsetof(WGPUVertexAttribute, offset),
        "offsetof mismatch for VertexAttribute::offset");
static_assert(offsetof(VertexAttribute, shaderLocation) == offsetof(WGPUVertexAttribute, shaderLocation),
        "offsetof mismatch for VertexAttribute::shaderLocation");

// YCbCrVkDescriptor implementation
YCbCrVkDescriptor::YCbCrVkDescriptor()
: ChainedStruct { nullptr, SType::YCbCrVkDescriptor } {}
struct YCbCrVkDescriptor::Init {
    ChainedStruct * const nextInChain;
    uint32_t vkFormat = 0;
    uint32_t vkYCbCrModel = 0;
    uint32_t vkYCbCrRange = 0;
    uint32_t vkComponentSwizzleRed = 0;
    uint32_t vkComponentSwizzleGreen = 0;
    uint32_t vkComponentSwizzleBlue = 0;
    uint32_t vkComponentSwizzleAlpha = 0;
    uint32_t vkXChromaOffset = 0;
    uint32_t vkYChromaOffset = 0;
    FilterMode vkChromaFilter = FilterMode::Undefined;
    Bool forceExplicitReconstruction = false;
    uint64_t externalFormat = 0;
};
YCbCrVkDescriptor::YCbCrVkDescriptor(YCbCrVkDescriptor::Init&& init) :
    ChainedStruct { init.nextInChain, SType::YCbCrVkDescriptor },
    vkFormat(std::move(init.vkFormat)),
    vkYCbCrModel(std::move(init.vkYCbCrModel)),
    vkYCbCrRange(std::move(init.vkYCbCrRange)),
    vkComponentSwizzleRed(std::move(init.vkComponentSwizzleRed)),
    vkComponentSwizzleGreen(std::move(init.vkComponentSwizzleGreen)),
    vkComponentSwizzleBlue(std::move(init.vkComponentSwizzleBlue)),
    vkComponentSwizzleAlpha(std::move(init.vkComponentSwizzleAlpha)),
    vkXChromaOffset(std::move(init.vkXChromaOffset)),
    vkYChromaOffset(std::move(init.vkYChromaOffset)),
    vkChromaFilter(std::move(init.vkChromaFilter)),
    forceExplicitReconstruction(std::move(init.forceExplicitReconstruction)),
    externalFormat(std::move(init.externalFormat)) {}

YCbCrVkDescriptor::operator const WGPUYCbCrVkDescriptor&() const noexcept {
    return *reinterpret_cast<const WGPUYCbCrVkDescriptor*>(this);
}

static_assert(sizeof(YCbCrVkDescriptor) == sizeof(WGPUYCbCrVkDescriptor), "sizeof mismatch for YCbCrVkDescriptor");
static_assert(alignof(YCbCrVkDescriptor) == alignof(WGPUYCbCrVkDescriptor), "alignof mismatch for YCbCrVkDescriptor");
static_assert(offsetof(YCbCrVkDescriptor, vkFormat) == offsetof(WGPUYCbCrVkDescriptor, vkFormat),
        "offsetof mismatch for YCbCrVkDescriptor::vkFormat");
static_assert(offsetof(YCbCrVkDescriptor, vkYCbCrModel) == offsetof(WGPUYCbCrVkDescriptor, vkYCbCrModel),
        "offsetof mismatch for YCbCrVkDescriptor::vkYCbCrModel");
static_assert(offsetof(YCbCrVkDescriptor, vkYCbCrRange) == offsetof(WGPUYCbCrVkDescriptor, vkYCbCrRange),
        "offsetof mismatch for YCbCrVkDescriptor::vkYCbCrRange");
static_assert(offsetof(YCbCrVkDescriptor, vkComponentSwizzleRed) == offsetof(WGPUYCbCrVkDescriptor, vkComponentSwizzleRed),
        "offsetof mismatch for YCbCrVkDescriptor::vkComponentSwizzleRed");
static_assert(offsetof(YCbCrVkDescriptor, vkComponentSwizzleGreen) == offsetof(WGPUYCbCrVkDescriptor, vkComponentSwizzleGreen),
        "offsetof mismatch for YCbCrVkDescriptor::vkComponentSwizzleGreen");
static_assert(offsetof(YCbCrVkDescriptor, vkComponentSwizzleBlue) == offsetof(WGPUYCbCrVkDescriptor, vkComponentSwizzleBlue),
        "offsetof mismatch for YCbCrVkDescriptor::vkComponentSwizzleBlue");
static_assert(offsetof(YCbCrVkDescriptor, vkComponentSwizzleAlpha) == offsetof(WGPUYCbCrVkDescriptor, vkComponentSwizzleAlpha),
        "offsetof mismatch for YCbCrVkDescriptor::vkComponentSwizzleAlpha");
static_assert(offsetof(YCbCrVkDescriptor, vkXChromaOffset) == offsetof(WGPUYCbCrVkDescriptor, vkXChromaOffset),
        "offsetof mismatch for YCbCrVkDescriptor::vkXChromaOffset");
static_assert(offsetof(YCbCrVkDescriptor, vkYChromaOffset) == offsetof(WGPUYCbCrVkDescriptor, vkYChromaOffset),
        "offsetof mismatch for YCbCrVkDescriptor::vkYChromaOffset");
static_assert(offsetof(YCbCrVkDescriptor, vkChromaFilter) == offsetof(WGPUYCbCrVkDescriptor, vkChromaFilter),
        "offsetof mismatch for YCbCrVkDescriptor::vkChromaFilter");
static_assert(offsetof(YCbCrVkDescriptor, forceExplicitReconstruction) == offsetof(WGPUYCbCrVkDescriptor, forceExplicitReconstruction),
        "offsetof mismatch for YCbCrVkDescriptor::forceExplicitReconstruction");
static_assert(offsetof(YCbCrVkDescriptor, externalFormat) == offsetof(WGPUYCbCrVkDescriptor, externalFormat),
        "offsetof mismatch for YCbCrVkDescriptor::externalFormat");

// AdapterPropertiesMemoryHeaps implementation
AdapterPropertiesMemoryHeaps::AdapterPropertiesMemoryHeaps()
: ChainedStructOut { nullptr, SType::AdapterPropertiesMemoryHeaps } {}
struct AdapterPropertiesMemoryHeaps::Init {
    ChainedStructOut *  nextInChain;
    size_t const heapCount = {};
    MemoryHeapInfo const * const heapInfo = nullptr;
};
AdapterPropertiesMemoryHeaps::AdapterPropertiesMemoryHeaps(AdapterPropertiesMemoryHeaps::Init&& init) :
    ChainedStructOut { init.nextInChain, SType::AdapterPropertiesMemoryHeaps },
    heapCount(std::move(init.heapCount)),
    heapInfo(std::move(init.heapInfo)) {}
AdapterPropertiesMemoryHeaps::~AdapterPropertiesMemoryHeaps() {
    FreeMembers();
}

AdapterPropertiesMemoryHeaps::AdapterPropertiesMemoryHeaps(AdapterPropertiesMemoryHeaps&& rhs)
    : heapCount(rhs.heapCount),
            heapInfo(rhs.heapInfo){
    Reset(rhs);
}

AdapterPropertiesMemoryHeaps& AdapterPropertiesMemoryHeaps::operator=(AdapterPropertiesMemoryHeaps&& rhs) {
    if (&rhs == this) {
        return *this;
    }
    FreeMembers();
    detail::AsNonConstReference(this->heapCount) = std::move(rhs.heapCount);
    detail::AsNonConstReference(this->heapInfo) = std::move(rhs.heapInfo);
    Reset(rhs);
    return *this;
}

void AdapterPropertiesMemoryHeaps::FreeMembers() {
    bool needsFreeing = false;    if (this->heapInfo != nullptr) { needsFreeing = true; }if (needsFreeing) {
        wgpuAdapterPropertiesMemoryHeapsFreeMembers(
            *reinterpret_cast<WGPUAdapterPropertiesMemoryHeaps*>(this));
    }
}

// static
void AdapterPropertiesMemoryHeaps::Reset(AdapterPropertiesMemoryHeaps& value) {
    AdapterPropertiesMemoryHeaps defaultValue{};
    detail::AsNonConstReference(value.heapCount) = defaultValue.heapCount;
    detail::AsNonConstReference(value.heapInfo) = defaultValue.heapInfo;
}

AdapterPropertiesMemoryHeaps::operator const WGPUAdapterPropertiesMemoryHeaps&() const noexcept {
    return *reinterpret_cast<const WGPUAdapterPropertiesMemoryHeaps*>(this);
}

static_assert(sizeof(AdapterPropertiesMemoryHeaps) == sizeof(WGPUAdapterPropertiesMemoryHeaps), "sizeof mismatch for AdapterPropertiesMemoryHeaps");
static_assert(alignof(AdapterPropertiesMemoryHeaps) == alignof(WGPUAdapterPropertiesMemoryHeaps), "alignof mismatch for AdapterPropertiesMemoryHeaps");
static_assert(offsetof(AdapterPropertiesMemoryHeaps, heapCount) == offsetof(WGPUAdapterPropertiesMemoryHeaps, heapCount),
        "offsetof mismatch for AdapterPropertiesMemoryHeaps::heapCount");
static_assert(offsetof(AdapterPropertiesMemoryHeaps, heapInfo) == offsetof(WGPUAdapterPropertiesMemoryHeaps, heapInfo),
        "offsetof mismatch for AdapterPropertiesMemoryHeaps::heapInfo");

// AdapterPropertiesSubgroupMatrixConfigs implementation
AdapterPropertiesSubgroupMatrixConfigs::AdapterPropertiesSubgroupMatrixConfigs()
: ChainedStructOut { nullptr, SType::AdapterPropertiesSubgroupMatrixConfigs } {}
struct AdapterPropertiesSubgroupMatrixConfigs::Init {
    ChainedStructOut *  nextInChain;
    size_t const configCount = {};
    SubgroupMatrixConfig const * const configs = nullptr;
};
AdapterPropertiesSubgroupMatrixConfigs::AdapterPropertiesSubgroupMatrixConfigs(AdapterPropertiesSubgroupMatrixConfigs::Init&& init) :
    ChainedStructOut { init.nextInChain, SType::AdapterPropertiesSubgroupMatrixConfigs },
    configCount(std::move(init.configCount)),
    configs(std::move(init.configs)) {}
AdapterPropertiesSubgroupMatrixConfigs::~AdapterPropertiesSubgroupMatrixConfigs() {
    FreeMembers();
}

AdapterPropertiesSubgroupMatrixConfigs::AdapterPropertiesSubgroupMatrixConfigs(AdapterPropertiesSubgroupMatrixConfigs&& rhs)
    : configCount(rhs.configCount),
            configs(rhs.configs){
    Reset(rhs);
}

AdapterPropertiesSubgroupMatrixConfigs& AdapterPropertiesSubgroupMatrixConfigs::operator=(AdapterPropertiesSubgroupMatrixConfigs&& rhs) {
    if (&rhs == this) {
        return *this;
    }
    FreeMembers();
    detail::AsNonConstReference(this->configCount) = std::move(rhs.configCount);
    detail::AsNonConstReference(this->configs) = std::move(rhs.configs);
    Reset(rhs);
    return *this;
}

void AdapterPropertiesSubgroupMatrixConfigs::FreeMembers() {
    bool needsFreeing = false;    if (this->configs != nullptr) { needsFreeing = true; }if (needsFreeing) {
        wgpuAdapterPropertiesSubgroupMatrixConfigsFreeMembers(
            *reinterpret_cast<WGPUAdapterPropertiesSubgroupMatrixConfigs*>(this));
    }
}

// static
void AdapterPropertiesSubgroupMatrixConfigs::Reset(AdapterPropertiesSubgroupMatrixConfigs& value) {
    AdapterPropertiesSubgroupMatrixConfigs defaultValue{};
    detail::AsNonConstReference(value.configCount) = defaultValue.configCount;
    detail::AsNonConstReference(value.configs) = defaultValue.configs;
}

AdapterPropertiesSubgroupMatrixConfigs::operator const WGPUAdapterPropertiesSubgroupMatrixConfigs&() const noexcept {
    return *reinterpret_cast<const WGPUAdapterPropertiesSubgroupMatrixConfigs*>(this);
}

static_assert(sizeof(AdapterPropertiesSubgroupMatrixConfigs) == sizeof(WGPUAdapterPropertiesSubgroupMatrixConfigs), "sizeof mismatch for AdapterPropertiesSubgroupMatrixConfigs");
static_assert(alignof(AdapterPropertiesSubgroupMatrixConfigs) == alignof(WGPUAdapterPropertiesSubgroupMatrixConfigs), "alignof mismatch for AdapterPropertiesSubgroupMatrixConfigs");
static_assert(offsetof(AdapterPropertiesSubgroupMatrixConfigs, configCount) == offsetof(WGPUAdapterPropertiesSubgroupMatrixConfigs, configCount),
        "offsetof mismatch for AdapterPropertiesSubgroupMatrixConfigs::configCount");
static_assert(offsetof(AdapterPropertiesSubgroupMatrixConfigs, configs) == offsetof(WGPUAdapterPropertiesSubgroupMatrixConfigs, configs),
        "offsetof mismatch for AdapterPropertiesSubgroupMatrixConfigs::configs");

// AHardwareBufferProperties implementation

AHardwareBufferProperties::operator const WGPUAHardwareBufferProperties&() const noexcept {
    return *reinterpret_cast<const WGPUAHardwareBufferProperties*>(this);
}

static_assert(sizeof(AHardwareBufferProperties) == sizeof(WGPUAHardwareBufferProperties), "sizeof mismatch for AHardwareBufferProperties");
static_assert(alignof(AHardwareBufferProperties) == alignof(WGPUAHardwareBufferProperties), "alignof mismatch for AHardwareBufferProperties");
static_assert(offsetof(AHardwareBufferProperties, yCbCrInfo) == offsetof(WGPUAHardwareBufferProperties, yCbCrInfo),
        "offsetof mismatch for AHardwareBufferProperties::yCbCrInfo");

// BindGroupEntry implementation

BindGroupEntry::operator const WGPUBindGroupEntry&() const noexcept {
    return *reinterpret_cast<const WGPUBindGroupEntry*>(this);
}

static_assert(sizeof(BindGroupEntry) == sizeof(WGPUBindGroupEntry), "sizeof mismatch for BindGroupEntry");
static_assert(alignof(BindGroupEntry) == alignof(WGPUBindGroupEntry), "alignof mismatch for BindGroupEntry");
static_assert(offsetof(BindGroupEntry, nextInChain) == offsetof(WGPUBindGroupEntry, nextInChain),
        "offsetof mismatch for BindGroupEntry::nextInChain");
static_assert(offsetof(BindGroupEntry, binding) == offsetof(WGPUBindGroupEntry, binding),
        "offsetof mismatch for BindGroupEntry::binding");
static_assert(offsetof(BindGroupEntry, buffer) == offsetof(WGPUBindGroupEntry, buffer),
        "offsetof mismatch for BindGroupEntry::buffer");
static_assert(offsetof(BindGroupEntry, offset) == offsetof(WGPUBindGroupEntry, offset),
        "offsetof mismatch for BindGroupEntry::offset");
static_assert(offsetof(BindGroupEntry, size) == offsetof(WGPUBindGroupEntry, size),
        "offsetof mismatch for BindGroupEntry::size");
static_assert(offsetof(BindGroupEntry, sampler) == offsetof(WGPUBindGroupEntry, sampler),
        "offsetof mismatch for BindGroupEntry::sampler");
static_assert(offsetof(BindGroupEntry, textureView) == offsetof(WGPUBindGroupEntry, textureView),
        "offsetof mismatch for BindGroupEntry::textureView");

// BindGroupLayoutEntry implementation

BindGroupLayoutEntry::operator const WGPUBindGroupLayoutEntry&() const noexcept {
    return *reinterpret_cast<const WGPUBindGroupLayoutEntry*>(this);
}

static_assert(sizeof(BindGroupLayoutEntry) == sizeof(WGPUBindGroupLayoutEntry), "sizeof mismatch for BindGroupLayoutEntry");
static_assert(alignof(BindGroupLayoutEntry) == alignof(WGPUBindGroupLayoutEntry), "alignof mismatch for BindGroupLayoutEntry");
static_assert(offsetof(BindGroupLayoutEntry, nextInChain) == offsetof(WGPUBindGroupLayoutEntry, nextInChain),
        "offsetof mismatch for BindGroupLayoutEntry::nextInChain");
static_assert(offsetof(BindGroupLayoutEntry, binding) == offsetof(WGPUBindGroupLayoutEntry, binding),
        "offsetof mismatch for BindGroupLayoutEntry::binding");
static_assert(offsetof(BindGroupLayoutEntry, visibility) == offsetof(WGPUBindGroupLayoutEntry, visibility),
        "offsetof mismatch for BindGroupLayoutEntry::visibility");
static_assert(offsetof(BindGroupLayoutEntry, bindingArraySize) == offsetof(WGPUBindGroupLayoutEntry, bindingArraySize),
        "offsetof mismatch for BindGroupLayoutEntry::bindingArraySize");
static_assert(offsetof(BindGroupLayoutEntry, buffer) == offsetof(WGPUBindGroupLayoutEntry, buffer),
        "offsetof mismatch for BindGroupLayoutEntry::buffer");
static_assert(offsetof(BindGroupLayoutEntry, sampler) == offsetof(WGPUBindGroupLayoutEntry, sampler),
        "offsetof mismatch for BindGroupLayoutEntry::sampler");
static_assert(offsetof(BindGroupLayoutEntry, texture) == offsetof(WGPUBindGroupLayoutEntry, texture),
        "offsetof mismatch for BindGroupLayoutEntry::texture");
static_assert(offsetof(BindGroupLayoutEntry, storageTexture) == offsetof(WGPUBindGroupLayoutEntry, storageTexture),
        "offsetof mismatch for BindGroupLayoutEntry::storageTexture");

// BlendState implementation

BlendState::operator const WGPUBlendState&() const noexcept {
    return *reinterpret_cast<const WGPUBlendState*>(this);
}

static_assert(sizeof(BlendState) == sizeof(WGPUBlendState), "sizeof mismatch for BlendState");
static_assert(alignof(BlendState) == alignof(WGPUBlendState), "alignof mismatch for BlendState");
static_assert(offsetof(BlendState, color) == offsetof(WGPUBlendState, color),
        "offsetof mismatch for BlendState::color");
static_assert(offsetof(BlendState, alpha) == offsetof(WGPUBlendState, alpha),
        "offsetof mismatch for BlendState::alpha");

// BufferDescriptor implementation

BufferDescriptor::operator const WGPUBufferDescriptor&() const noexcept {
    return *reinterpret_cast<const WGPUBufferDescriptor*>(this);
}

static_assert(sizeof(BufferDescriptor) == sizeof(WGPUBufferDescriptor), "sizeof mismatch for BufferDescriptor");
static_assert(alignof(BufferDescriptor) == alignof(WGPUBufferDescriptor), "alignof mismatch for BufferDescriptor");
static_assert(offsetof(BufferDescriptor, nextInChain) == offsetof(WGPUBufferDescriptor, nextInChain),
        "offsetof mismatch for BufferDescriptor::nextInChain");
static_assert(offsetof(BufferDescriptor, label) == offsetof(WGPUBufferDescriptor, label),
        "offsetof mismatch for BufferDescriptor::label");
static_assert(offsetof(BufferDescriptor, usage) == offsetof(WGPUBufferDescriptor, usage),
        "offsetof mismatch for BufferDescriptor::usage");
static_assert(offsetof(BufferDescriptor, size) == offsetof(WGPUBufferDescriptor, size),
        "offsetof mismatch for BufferDescriptor::size");
static_assert(offsetof(BufferDescriptor, mappedAtCreation) == offsetof(WGPUBufferDescriptor, mappedAtCreation),
        "offsetof mismatch for BufferDescriptor::mappedAtCreation");

// CommandEncoderDescriptor implementation

CommandEncoderDescriptor::operator const WGPUCommandEncoderDescriptor&() const noexcept {
    return *reinterpret_cast<const WGPUCommandEncoderDescriptor*>(this);
}

static_assert(sizeof(CommandEncoderDescriptor) == sizeof(WGPUCommandEncoderDescriptor), "sizeof mismatch for CommandEncoderDescriptor");
static_assert(alignof(CommandEncoderDescriptor) == alignof(WGPUCommandEncoderDescriptor), "alignof mismatch for CommandEncoderDescriptor");
static_assert(offsetof(CommandEncoderDescriptor, nextInChain) == offsetof(WGPUCommandEncoderDescriptor, nextInChain),
        "offsetof mismatch for CommandEncoderDescriptor::nextInChain");
static_assert(offsetof(CommandEncoderDescriptor, label) == offsetof(WGPUCommandEncoderDescriptor, label),
        "offsetof mismatch for CommandEncoderDescriptor::label");

// CompilationMessage implementation

CompilationMessage::operator const WGPUCompilationMessage&() const noexcept {
    return *reinterpret_cast<const WGPUCompilationMessage*>(this);
}

static_assert(sizeof(CompilationMessage) == sizeof(WGPUCompilationMessage), "sizeof mismatch for CompilationMessage");
static_assert(alignof(CompilationMessage) == alignof(WGPUCompilationMessage), "alignof mismatch for CompilationMessage");
static_assert(offsetof(CompilationMessage, nextInChain) == offsetof(WGPUCompilationMessage, nextInChain),
        "offsetof mismatch for CompilationMessage::nextInChain");
static_assert(offsetof(CompilationMessage, message) == offsetof(WGPUCompilationMessage, message),
        "offsetof mismatch for CompilationMessage::message");
static_assert(offsetof(CompilationMessage, type) == offsetof(WGPUCompilationMessage, type),
        "offsetof mismatch for CompilationMessage::type");
static_assert(offsetof(CompilationMessage, lineNum) == offsetof(WGPUCompilationMessage, lineNum),
        "offsetof mismatch for CompilationMessage::lineNum");
static_assert(offsetof(CompilationMessage, linePos) == offsetof(WGPUCompilationMessage, linePos),
        "offsetof mismatch for CompilationMessage::linePos");
static_assert(offsetof(CompilationMessage, offset) == offsetof(WGPUCompilationMessage, offset),
        "offsetof mismatch for CompilationMessage::offset");
static_assert(offsetof(CompilationMessage, length) == offsetof(WGPUCompilationMessage, length),
        "offsetof mismatch for CompilationMessage::length");

// ComputePassDescriptor implementation

ComputePassDescriptor::operator const WGPUComputePassDescriptor&() const noexcept {
    return *reinterpret_cast<const WGPUComputePassDescriptor*>(this);
}

static_assert(sizeof(ComputePassDescriptor) == sizeof(WGPUComputePassDescriptor), "sizeof mismatch for ComputePassDescriptor");
static_assert(alignof(ComputePassDescriptor) == alignof(WGPUComputePassDescriptor), "alignof mismatch for ComputePassDescriptor");
static_assert(offsetof(ComputePassDescriptor, nextInChain) == offsetof(WGPUComputePassDescriptor, nextInChain),
        "offsetof mismatch for ComputePassDescriptor::nextInChain");
static_assert(offsetof(ComputePassDescriptor, label) == offsetof(WGPUComputePassDescriptor, label),
        "offsetof mismatch for ComputePassDescriptor::label");
static_assert(offsetof(ComputePassDescriptor, timestampWrites) == offsetof(WGPUComputePassDescriptor, timestampWrites),
        "offsetof mismatch for ComputePassDescriptor::timestampWrites");

// ComputeState implementation

ComputeState::operator const WGPUComputeState&() const noexcept {
    return *reinterpret_cast<const WGPUComputeState*>(this);
}

static_assert(sizeof(ComputeState) == sizeof(WGPUComputeState), "sizeof mismatch for ComputeState");
static_assert(alignof(ComputeState) == alignof(WGPUComputeState), "alignof mismatch for ComputeState");
static_assert(offsetof(ComputeState, nextInChain) == offsetof(WGPUComputeState, nextInChain),
        "offsetof mismatch for ComputeState::nextInChain");
static_assert(offsetof(ComputeState, module) == offsetof(WGPUComputeState, module),
        "offsetof mismatch for ComputeState::module");
static_assert(offsetof(ComputeState, entryPoint) == offsetof(WGPUComputeState, entryPoint),
        "offsetof mismatch for ComputeState::entryPoint");
static_assert(offsetof(ComputeState, constantCount) == offsetof(WGPUComputeState, constantCount),
        "offsetof mismatch for ComputeState::constantCount");
static_assert(offsetof(ComputeState, constants) == offsetof(WGPUComputeState, constants),
        "offsetof mismatch for ComputeState::constants");

// DawnDrmFormatCapabilities implementation
DawnDrmFormatCapabilities::DawnDrmFormatCapabilities()
: ChainedStructOut { nullptr, SType::DawnDrmFormatCapabilities } {}
struct DawnDrmFormatCapabilities::Init {
    ChainedStructOut *  nextInChain;
    size_t const propertiesCount = {};
    DawnDrmFormatProperties const * const properties = nullptr;
};
DawnDrmFormatCapabilities::DawnDrmFormatCapabilities(DawnDrmFormatCapabilities::Init&& init) :
    ChainedStructOut { init.nextInChain, SType::DawnDrmFormatCapabilities },
    propertiesCount(std::move(init.propertiesCount)),
    properties(std::move(init.properties)) {}
DawnDrmFormatCapabilities::~DawnDrmFormatCapabilities() {
    FreeMembers();
}

DawnDrmFormatCapabilities::DawnDrmFormatCapabilities(DawnDrmFormatCapabilities&& rhs)
    : propertiesCount(rhs.propertiesCount),
            properties(rhs.properties){
    Reset(rhs);
}

DawnDrmFormatCapabilities& DawnDrmFormatCapabilities::operator=(DawnDrmFormatCapabilities&& rhs) {
    if (&rhs == this) {
        return *this;
    }
    FreeMembers();
    detail::AsNonConstReference(this->propertiesCount) = std::move(rhs.propertiesCount);
    detail::AsNonConstReference(this->properties) = std::move(rhs.properties);
    Reset(rhs);
    return *this;
}

void DawnDrmFormatCapabilities::FreeMembers() {
    bool needsFreeing = false;    if (this->properties != nullptr) { needsFreeing = true; }if (needsFreeing) {
        wgpuDawnDrmFormatCapabilitiesFreeMembers(
            *reinterpret_cast<WGPUDawnDrmFormatCapabilities*>(this));
    }
}

// static
void DawnDrmFormatCapabilities::Reset(DawnDrmFormatCapabilities& value) {
    DawnDrmFormatCapabilities defaultValue{};
    detail::AsNonConstReference(value.propertiesCount) = defaultValue.propertiesCount;
    detail::AsNonConstReference(value.properties) = defaultValue.properties;
}

DawnDrmFormatCapabilities::operator const WGPUDawnDrmFormatCapabilities&() const noexcept {
    return *reinterpret_cast<const WGPUDawnDrmFormatCapabilities*>(this);
}

static_assert(sizeof(DawnDrmFormatCapabilities) == sizeof(WGPUDawnDrmFormatCapabilities), "sizeof mismatch for DawnDrmFormatCapabilities");
static_assert(alignof(DawnDrmFormatCapabilities) == alignof(WGPUDawnDrmFormatCapabilities), "alignof mismatch for DawnDrmFormatCapabilities");
static_assert(offsetof(DawnDrmFormatCapabilities, propertiesCount) == offsetof(WGPUDawnDrmFormatCapabilities, propertiesCount),
        "offsetof mismatch for DawnDrmFormatCapabilities::propertiesCount");
static_assert(offsetof(DawnDrmFormatCapabilities, properties) == offsetof(WGPUDawnDrmFormatCapabilities, properties),
        "offsetof mismatch for DawnDrmFormatCapabilities::properties");

// DepthStencilState implementation

DepthStencilState::operator const WGPUDepthStencilState&() const noexcept {
    return *reinterpret_cast<const WGPUDepthStencilState*>(this);
}

static_assert(sizeof(DepthStencilState) == sizeof(WGPUDepthStencilState), "sizeof mismatch for DepthStencilState");
static_assert(alignof(DepthStencilState) == alignof(WGPUDepthStencilState), "alignof mismatch for DepthStencilState");
static_assert(offsetof(DepthStencilState, nextInChain) == offsetof(WGPUDepthStencilState, nextInChain),
        "offsetof mismatch for DepthStencilState::nextInChain");
static_assert(offsetof(DepthStencilState, format) == offsetof(WGPUDepthStencilState, format),
        "offsetof mismatch for DepthStencilState::format");
static_assert(offsetof(DepthStencilState, depthWriteEnabled) == offsetof(WGPUDepthStencilState, depthWriteEnabled),
        "offsetof mismatch for DepthStencilState::depthWriteEnabled");
static_assert(offsetof(DepthStencilState, depthCompare) == offsetof(WGPUDepthStencilState, depthCompare),
        "offsetof mismatch for DepthStencilState::depthCompare");
static_assert(offsetof(DepthStencilState, stencilFront) == offsetof(WGPUDepthStencilState, stencilFront),
        "offsetof mismatch for DepthStencilState::stencilFront");
static_assert(offsetof(DepthStencilState, stencilBack) == offsetof(WGPUDepthStencilState, stencilBack),
        "offsetof mismatch for DepthStencilState::stencilBack");
static_assert(offsetof(DepthStencilState, stencilReadMask) == offsetof(WGPUDepthStencilState, stencilReadMask),
        "offsetof mismatch for DepthStencilState::stencilReadMask");
static_assert(offsetof(DepthStencilState, stencilWriteMask) == offsetof(WGPUDepthStencilState, stencilWriteMask),
        "offsetof mismatch for DepthStencilState::stencilWriteMask");
static_assert(offsetof(DepthStencilState, depthBias) == offsetof(WGPUDepthStencilState, depthBias),
        "offsetof mismatch for DepthStencilState::depthBias");
static_assert(offsetof(DepthStencilState, depthBiasSlopeScale) == offsetof(WGPUDepthStencilState, depthBiasSlopeScale),
        "offsetof mismatch for DepthStencilState::depthBiasSlopeScale");
static_assert(offsetof(DepthStencilState, depthBiasClamp) == offsetof(WGPUDepthStencilState, depthBiasClamp),
        "offsetof mismatch for DepthStencilState::depthBiasClamp");

// ExternalTextureDescriptor implementation

ExternalTextureDescriptor::operator const WGPUExternalTextureDescriptor&() const noexcept {
    return *reinterpret_cast<const WGPUExternalTextureDescriptor*>(this);
}

static_assert(sizeof(ExternalTextureDescriptor) == sizeof(WGPUExternalTextureDescriptor), "sizeof mismatch for ExternalTextureDescriptor");
static_assert(alignof(ExternalTextureDescriptor) == alignof(WGPUExternalTextureDescriptor), "alignof mismatch for ExternalTextureDescriptor");
static_assert(offsetof(ExternalTextureDescriptor, nextInChain) == offsetof(WGPUExternalTextureDescriptor, nextInChain),
        "offsetof mismatch for ExternalTextureDescriptor::nextInChain");
static_assert(offsetof(ExternalTextureDescriptor, label) == offsetof(WGPUExternalTextureDescriptor, label),
        "offsetof mismatch for ExternalTextureDescriptor::label");
static_assert(offsetof(ExternalTextureDescriptor, plane0) == offsetof(WGPUExternalTextureDescriptor, plane0),
        "offsetof mismatch for ExternalTextureDescriptor::plane0");
static_assert(offsetof(ExternalTextureDescriptor, plane1) == offsetof(WGPUExternalTextureDescriptor, plane1),
        "offsetof mismatch for ExternalTextureDescriptor::plane1");
static_assert(offsetof(ExternalTextureDescriptor, cropOrigin) == offsetof(WGPUExternalTextureDescriptor, cropOrigin),
        "offsetof mismatch for ExternalTextureDescriptor::cropOrigin");
static_assert(offsetof(ExternalTextureDescriptor, cropSize) == offsetof(WGPUExternalTextureDescriptor, cropSize),
        "offsetof mismatch for ExternalTextureDescriptor::cropSize");
static_assert(offsetof(ExternalTextureDescriptor, apparentSize) == offsetof(WGPUExternalTextureDescriptor, apparentSize),
        "offsetof mismatch for ExternalTextureDescriptor::apparentSize");
static_assert(offsetof(ExternalTextureDescriptor, doYuvToRgbConversionOnly) == offsetof(WGPUExternalTextureDescriptor, doYuvToRgbConversionOnly),
        "offsetof mismatch for ExternalTextureDescriptor::doYuvToRgbConversionOnly");
static_assert(offsetof(ExternalTextureDescriptor, yuvToRgbConversionMatrix) == offsetof(WGPUExternalTextureDescriptor, yuvToRgbConversionMatrix),
        "offsetof mismatch for ExternalTextureDescriptor::yuvToRgbConversionMatrix");
static_assert(offsetof(ExternalTextureDescriptor, srcTransferFunctionParameters) == offsetof(WGPUExternalTextureDescriptor, srcTransferFunctionParameters),
        "offsetof mismatch for ExternalTextureDescriptor::srcTransferFunctionParameters");
static_assert(offsetof(ExternalTextureDescriptor, dstTransferFunctionParameters) == offsetof(WGPUExternalTextureDescriptor, dstTransferFunctionParameters),
        "offsetof mismatch for ExternalTextureDescriptor::dstTransferFunctionParameters");
static_assert(offsetof(ExternalTextureDescriptor, gamutConversionMatrix) == offsetof(WGPUExternalTextureDescriptor, gamutConversionMatrix),
        "offsetof mismatch for ExternalTextureDescriptor::gamutConversionMatrix");
static_assert(offsetof(ExternalTextureDescriptor, mirrored) == offsetof(WGPUExternalTextureDescriptor, mirrored),
        "offsetof mismatch for ExternalTextureDescriptor::mirrored");
static_assert(offsetof(ExternalTextureDescriptor, rotation) == offsetof(WGPUExternalTextureDescriptor, rotation),
        "offsetof mismatch for ExternalTextureDescriptor::rotation");

// FutureWaitInfo implementation

FutureWaitInfo::operator const WGPUFutureWaitInfo&() const noexcept {
    return *reinterpret_cast<const WGPUFutureWaitInfo*>(this);
}

static_assert(sizeof(FutureWaitInfo) == sizeof(WGPUFutureWaitInfo), "sizeof mismatch for FutureWaitInfo");
static_assert(alignof(FutureWaitInfo) == alignof(WGPUFutureWaitInfo), "alignof mismatch for FutureWaitInfo");
static_assert(offsetof(FutureWaitInfo, future) == offsetof(WGPUFutureWaitInfo, future),
        "offsetof mismatch for FutureWaitInfo::future");
static_assert(offsetof(FutureWaitInfo, completed) == offsetof(WGPUFutureWaitInfo, completed),
        "offsetof mismatch for FutureWaitInfo::completed");

// ImageCopyExternalTexture implementation

ImageCopyExternalTexture::operator const WGPUImageCopyExternalTexture&() const noexcept {
    return *reinterpret_cast<const WGPUImageCopyExternalTexture*>(this);
}

static_assert(sizeof(ImageCopyExternalTexture) == sizeof(WGPUImageCopyExternalTexture), "sizeof mismatch for ImageCopyExternalTexture");
static_assert(alignof(ImageCopyExternalTexture) == alignof(WGPUImageCopyExternalTexture), "alignof mismatch for ImageCopyExternalTexture");
static_assert(offsetof(ImageCopyExternalTexture, nextInChain) == offsetof(WGPUImageCopyExternalTexture, nextInChain),
        "offsetof mismatch for ImageCopyExternalTexture::nextInChain");
static_assert(offsetof(ImageCopyExternalTexture, externalTexture) == offsetof(WGPUImageCopyExternalTexture, externalTexture),
        "offsetof mismatch for ImageCopyExternalTexture::externalTexture");
static_assert(offsetof(ImageCopyExternalTexture, origin) == offsetof(WGPUImageCopyExternalTexture, origin),
        "offsetof mismatch for ImageCopyExternalTexture::origin");
static_assert(offsetof(ImageCopyExternalTexture, naturalSize) == offsetof(WGPUImageCopyExternalTexture, naturalSize),
        "offsetof mismatch for ImageCopyExternalTexture::naturalSize");

// InstanceDescriptor implementation

InstanceDescriptor::operator const WGPUInstanceDescriptor&() const noexcept {
    return *reinterpret_cast<const WGPUInstanceDescriptor*>(this);
}

static_assert(sizeof(InstanceDescriptor) == sizeof(WGPUInstanceDescriptor), "sizeof mismatch for InstanceDescriptor");
static_assert(alignof(InstanceDescriptor) == alignof(WGPUInstanceDescriptor), "alignof mismatch for InstanceDescriptor");
static_assert(offsetof(InstanceDescriptor, nextInChain) == offsetof(WGPUInstanceDescriptor, nextInChain),
        "offsetof mismatch for InstanceDescriptor::nextInChain");
static_assert(offsetof(InstanceDescriptor, requiredFeatureCount) == offsetof(WGPUInstanceDescriptor, requiredFeatureCount),
        "offsetof mismatch for InstanceDescriptor::requiredFeatureCount");
static_assert(offsetof(InstanceDescriptor, requiredFeatures) == offsetof(WGPUInstanceDescriptor, requiredFeatures),
        "offsetof mismatch for InstanceDescriptor::requiredFeatures");
static_assert(offsetof(InstanceDescriptor, requiredLimits) == offsetof(WGPUInstanceDescriptor, requiredLimits),
        "offsetof mismatch for InstanceDescriptor::requiredLimits");

// Limits implementation

Limits::operator const WGPULimits&() const noexcept {
    return *reinterpret_cast<const WGPULimits*>(this);
}

static_assert(sizeof(Limits) == sizeof(WGPULimits), "sizeof mismatch for Limits");
static_assert(alignof(Limits) == alignof(WGPULimits), "alignof mismatch for Limits");
static_assert(offsetof(Limits, nextInChain) == offsetof(WGPULimits, nextInChain),
        "offsetof mismatch for Limits::nextInChain");
static_assert(offsetof(Limits, maxTextureDimension1D) == offsetof(WGPULimits, maxTextureDimension1D),
        "offsetof mismatch for Limits::maxTextureDimension1D");
static_assert(offsetof(Limits, maxTextureDimension2D) == offsetof(WGPULimits, maxTextureDimension2D),
        "offsetof mismatch for Limits::maxTextureDimension2D");
static_assert(offsetof(Limits, maxTextureDimension3D) == offsetof(WGPULimits, maxTextureDimension3D),
        "offsetof mismatch for Limits::maxTextureDimension3D");
static_assert(offsetof(Limits, maxTextureArrayLayers) == offsetof(WGPULimits, maxTextureArrayLayers),
        "offsetof mismatch for Limits::maxTextureArrayLayers");
static_assert(offsetof(Limits, maxBindGroups) == offsetof(WGPULimits, maxBindGroups),
        "offsetof mismatch for Limits::maxBindGroups");
static_assert(offsetof(Limits, maxBindGroupsPlusVertexBuffers) == offsetof(WGPULimits, maxBindGroupsPlusVertexBuffers),
        "offsetof mismatch for Limits::maxBindGroupsPlusVertexBuffers");
static_assert(offsetof(Limits, maxBindingsPerBindGroup) == offsetof(WGPULimits, maxBindingsPerBindGroup),
        "offsetof mismatch for Limits::maxBindingsPerBindGroup");
static_assert(offsetof(Limits, maxDynamicUniformBuffersPerPipelineLayout) == offsetof(WGPULimits, maxDynamicUniformBuffersPerPipelineLayout),
        "offsetof mismatch for Limits::maxDynamicUniformBuffersPerPipelineLayout");
static_assert(offsetof(Limits, maxDynamicStorageBuffersPerPipelineLayout) == offsetof(WGPULimits, maxDynamicStorageBuffersPerPipelineLayout),
        "offsetof mismatch for Limits::maxDynamicStorageBuffersPerPipelineLayout");
static_assert(offsetof(Limits, maxSampledTexturesPerShaderStage) == offsetof(WGPULimits, maxSampledTexturesPerShaderStage),
        "offsetof mismatch for Limits::maxSampledTexturesPerShaderStage");
static_assert(offsetof(Limits, maxSamplersPerShaderStage) == offsetof(WGPULimits, maxSamplersPerShaderStage),
        "offsetof mismatch for Limits::maxSamplersPerShaderStage");
static_assert(offsetof(Limits, maxStorageBuffersPerShaderStage) == offsetof(WGPULimits, maxStorageBuffersPerShaderStage),
        "offsetof mismatch for Limits::maxStorageBuffersPerShaderStage");
static_assert(offsetof(Limits, maxStorageTexturesPerShaderStage) == offsetof(WGPULimits, maxStorageTexturesPerShaderStage),
        "offsetof mismatch for Limits::maxStorageTexturesPerShaderStage");
static_assert(offsetof(Limits, maxUniformBuffersPerShaderStage) == offsetof(WGPULimits, maxUniformBuffersPerShaderStage),
        "offsetof mismatch for Limits::maxUniformBuffersPerShaderStage");
static_assert(offsetof(Limits, maxUniformBufferBindingSize) == offsetof(WGPULimits, maxUniformBufferBindingSize),
        "offsetof mismatch for Limits::maxUniformBufferBindingSize");
static_assert(offsetof(Limits, maxStorageBufferBindingSize) == offsetof(WGPULimits, maxStorageBufferBindingSize),
        "offsetof mismatch for Limits::maxStorageBufferBindingSize");
static_assert(offsetof(Limits, minUniformBufferOffsetAlignment) == offsetof(WGPULimits, minUniformBufferOffsetAlignment),
        "offsetof mismatch for Limits::minUniformBufferOffsetAlignment");
static_assert(offsetof(Limits, minStorageBufferOffsetAlignment) == offsetof(WGPULimits, minStorageBufferOffsetAlignment),
        "offsetof mismatch for Limits::minStorageBufferOffsetAlignment");
static_assert(offsetof(Limits, maxVertexBuffers) == offsetof(WGPULimits, maxVertexBuffers),
        "offsetof mismatch for Limits::maxVertexBuffers");
static_assert(offsetof(Limits, maxBufferSize) == offsetof(WGPULimits, maxBufferSize),
        "offsetof mismatch for Limits::maxBufferSize");
static_assert(offsetof(Limits, maxVertexAttributes) == offsetof(WGPULimits, maxVertexAttributes),
        "offsetof mismatch for Limits::maxVertexAttributes");
static_assert(offsetof(Limits, maxVertexBufferArrayStride) == offsetof(WGPULimits, maxVertexBufferArrayStride),
        "offsetof mismatch for Limits::maxVertexBufferArrayStride");
static_assert(offsetof(Limits, maxInterStageShaderVariables) == offsetof(WGPULimits, maxInterStageShaderVariables),
        "offsetof mismatch for Limits::maxInterStageShaderVariables");
static_assert(offsetof(Limits, maxColorAttachments) == offsetof(WGPULimits, maxColorAttachments),
        "offsetof mismatch for Limits::maxColorAttachments");
static_assert(offsetof(Limits, maxColorAttachmentBytesPerSample) == offsetof(WGPULimits, maxColorAttachmentBytesPerSample),
        "offsetof mismatch for Limits::maxColorAttachmentBytesPerSample");
static_assert(offsetof(Limits, maxComputeWorkgroupStorageSize) == offsetof(WGPULimits, maxComputeWorkgroupStorageSize),
        "offsetof mismatch for Limits::maxComputeWorkgroupStorageSize");
static_assert(offsetof(Limits, maxComputeInvocationsPerWorkgroup) == offsetof(WGPULimits, maxComputeInvocationsPerWorkgroup),
        "offsetof mismatch for Limits::maxComputeInvocationsPerWorkgroup");
static_assert(offsetof(Limits, maxComputeWorkgroupSizeX) == offsetof(WGPULimits, maxComputeWorkgroupSizeX),
        "offsetof mismatch for Limits::maxComputeWorkgroupSizeX");
static_assert(offsetof(Limits, maxComputeWorkgroupSizeY) == offsetof(WGPULimits, maxComputeWorkgroupSizeY),
        "offsetof mismatch for Limits::maxComputeWorkgroupSizeY");
static_assert(offsetof(Limits, maxComputeWorkgroupSizeZ) == offsetof(WGPULimits, maxComputeWorkgroupSizeZ),
        "offsetof mismatch for Limits::maxComputeWorkgroupSizeZ");
static_assert(offsetof(Limits, maxComputeWorkgroupsPerDimension) == offsetof(WGPULimits, maxComputeWorkgroupsPerDimension),
        "offsetof mismatch for Limits::maxComputeWorkgroupsPerDimension");
static_assert(offsetof(Limits, maxImmediateSize) == offsetof(WGPULimits, maxImmediateSize),
        "offsetof mismatch for Limits::maxImmediateSize");

// PipelineLayoutPixelLocalStorage implementation
PipelineLayoutPixelLocalStorage::PipelineLayoutPixelLocalStorage()
: ChainedStruct { nullptr, SType::PipelineLayoutPixelLocalStorage } {}
struct PipelineLayoutPixelLocalStorage::Init {
    ChainedStruct * const nextInChain;
    uint64_t totalPixelLocalStorageSize;
    size_t storageAttachmentCount = 0;
    PipelineLayoutStorageAttachment const * storageAttachments = nullptr;
};
PipelineLayoutPixelLocalStorage::PipelineLayoutPixelLocalStorage(PipelineLayoutPixelLocalStorage::Init&& init) :
    ChainedStruct { init.nextInChain, SType::PipelineLayoutPixelLocalStorage },
    totalPixelLocalStorageSize(std::move(init.totalPixelLocalStorageSize)),
    storageAttachmentCount(std::move(init.storageAttachmentCount)),
    storageAttachments(std::move(init.storageAttachments)) {}

PipelineLayoutPixelLocalStorage::operator const WGPUPipelineLayoutPixelLocalStorage&() const noexcept {
    return *reinterpret_cast<const WGPUPipelineLayoutPixelLocalStorage*>(this);
}

static_assert(sizeof(PipelineLayoutPixelLocalStorage) == sizeof(WGPUPipelineLayoutPixelLocalStorage), "sizeof mismatch for PipelineLayoutPixelLocalStorage");
static_assert(alignof(PipelineLayoutPixelLocalStorage) == alignof(WGPUPipelineLayoutPixelLocalStorage), "alignof mismatch for PipelineLayoutPixelLocalStorage");
static_assert(offsetof(PipelineLayoutPixelLocalStorage, totalPixelLocalStorageSize) == offsetof(WGPUPipelineLayoutPixelLocalStorage, totalPixelLocalStorageSize),
        "offsetof mismatch for PipelineLayoutPixelLocalStorage::totalPixelLocalStorageSize");
static_assert(offsetof(PipelineLayoutPixelLocalStorage, storageAttachmentCount) == offsetof(WGPUPipelineLayoutPixelLocalStorage, storageAttachmentCount),
        "offsetof mismatch for PipelineLayoutPixelLocalStorage::storageAttachmentCount");
static_assert(offsetof(PipelineLayoutPixelLocalStorage, storageAttachments) == offsetof(WGPUPipelineLayoutPixelLocalStorage, storageAttachments),
        "offsetof mismatch for PipelineLayoutPixelLocalStorage::storageAttachments");

// RenderPassColorAttachment implementation

RenderPassColorAttachment::operator const WGPURenderPassColorAttachment&() const noexcept {
    return *reinterpret_cast<const WGPURenderPassColorAttachment*>(this);
}

static_assert(sizeof(RenderPassColorAttachment) == sizeof(WGPURenderPassColorAttachment), "sizeof mismatch for RenderPassColorAttachment");
static_assert(alignof(RenderPassColorAttachment) == alignof(WGPURenderPassColorAttachment), "alignof mismatch for RenderPassColorAttachment");
static_assert(offsetof(RenderPassColorAttachment, nextInChain) == offsetof(WGPURenderPassColorAttachment, nextInChain),
        "offsetof mismatch for RenderPassColorAttachment::nextInChain");
static_assert(offsetof(RenderPassColorAttachment, view) == offsetof(WGPURenderPassColorAttachment, view),
        "offsetof mismatch for RenderPassColorAttachment::view");
static_assert(offsetof(RenderPassColorAttachment, depthSlice) == offsetof(WGPURenderPassColorAttachment, depthSlice),
        "offsetof mismatch for RenderPassColorAttachment::depthSlice");
static_assert(offsetof(RenderPassColorAttachment, resolveTarget) == offsetof(WGPURenderPassColorAttachment, resolveTarget),
        "offsetof mismatch for RenderPassColorAttachment::resolveTarget");
static_assert(offsetof(RenderPassColorAttachment, loadOp) == offsetof(WGPURenderPassColorAttachment, loadOp),
        "offsetof mismatch for RenderPassColorAttachment::loadOp");
static_assert(offsetof(RenderPassColorAttachment, storeOp) == offsetof(WGPURenderPassColorAttachment, storeOp),
        "offsetof mismatch for RenderPassColorAttachment::storeOp");
static_assert(offsetof(RenderPassColorAttachment, clearValue) == offsetof(WGPURenderPassColorAttachment, clearValue),
        "offsetof mismatch for RenderPassColorAttachment::clearValue");

// RenderPassRenderAreaRect implementation
RenderPassRenderAreaRect::RenderPassRenderAreaRect()
: ChainedStruct { nullptr, SType::RenderPassRenderAreaRect } {}
struct RenderPassRenderAreaRect::Init {
    ChainedStruct * const nextInChain;
    Origin2D origin = {};
    Extent2D size = {};
};
RenderPassRenderAreaRect::RenderPassRenderAreaRect(RenderPassRenderAreaRect::Init&& init) :
    ChainedStruct { init.nextInChain, SType::RenderPassRenderAreaRect },
    origin(std::move(init.origin)),
    size(std::move(init.size)) {}

RenderPassRenderAreaRect::operator const WGPURenderPassRenderAreaRect&() const noexcept {
    return *reinterpret_cast<const WGPURenderPassRenderAreaRect*>(this);
}

static_assert(sizeof(RenderPassRenderAreaRect) == sizeof(WGPURenderPassRenderAreaRect), "sizeof mismatch for RenderPassRenderAreaRect");
static_assert(alignof(RenderPassRenderAreaRect) == alignof(WGPURenderPassRenderAreaRect), "alignof mismatch for RenderPassRenderAreaRect");
static_assert(offsetof(RenderPassRenderAreaRect, origin) == offsetof(WGPURenderPassRenderAreaRect, origin),
        "offsetof mismatch for RenderPassRenderAreaRect::origin");
static_assert(offsetof(RenderPassRenderAreaRect, size) == offsetof(WGPURenderPassRenderAreaRect, size),
        "offsetof mismatch for RenderPassRenderAreaRect::size");

// RenderPassStorageAttachment implementation

RenderPassStorageAttachment::operator const WGPURenderPassStorageAttachment&() const noexcept {
    return *reinterpret_cast<const WGPURenderPassStorageAttachment*>(this);
}

static_assert(sizeof(RenderPassStorageAttachment) == sizeof(WGPURenderPassStorageAttachment), "sizeof mismatch for RenderPassStorageAttachment");
static_assert(alignof(RenderPassStorageAttachment) == alignof(WGPURenderPassStorageAttachment), "alignof mismatch for RenderPassStorageAttachment");
static_assert(offsetof(RenderPassStorageAttachment, nextInChain) == offsetof(WGPURenderPassStorageAttachment, nextInChain),
        "offsetof mismatch for RenderPassStorageAttachment::nextInChain");
static_assert(offsetof(RenderPassStorageAttachment, offset) == offsetof(WGPURenderPassStorageAttachment, offset),
        "offsetof mismatch for RenderPassStorageAttachment::offset");
static_assert(offsetof(RenderPassStorageAttachment, storage) == offsetof(WGPURenderPassStorageAttachment, storage),
        "offsetof mismatch for RenderPassStorageAttachment::storage");
static_assert(offsetof(RenderPassStorageAttachment, loadOp) == offsetof(WGPURenderPassStorageAttachment, loadOp),
        "offsetof mismatch for RenderPassStorageAttachment::loadOp");
static_assert(offsetof(RenderPassStorageAttachment, storeOp) == offsetof(WGPURenderPassStorageAttachment, storeOp),
        "offsetof mismatch for RenderPassStorageAttachment::storeOp");
static_assert(offsetof(RenderPassStorageAttachment, clearValue) == offsetof(WGPURenderPassStorageAttachment, clearValue),
        "offsetof mismatch for RenderPassStorageAttachment::clearValue");

// RequestAdapterOptions implementation

RequestAdapterOptions::operator const WGPURequestAdapterOptions&() const noexcept {
    return *reinterpret_cast<const WGPURequestAdapterOptions*>(this);
}

static_assert(sizeof(RequestAdapterOptions) == sizeof(WGPURequestAdapterOptions), "sizeof mismatch for RequestAdapterOptions");
static_assert(alignof(RequestAdapterOptions) == alignof(WGPURequestAdapterOptions), "alignof mismatch for RequestAdapterOptions");
static_assert(offsetof(RequestAdapterOptions, nextInChain) == offsetof(WGPURequestAdapterOptions, nextInChain),
        "offsetof mismatch for RequestAdapterOptions::nextInChain");
static_assert(offsetof(RequestAdapterOptions, featureLevel) == offsetof(WGPURequestAdapterOptions, featureLevel),
        "offsetof mismatch for RequestAdapterOptions::featureLevel");
static_assert(offsetof(RequestAdapterOptions, powerPreference) == offsetof(WGPURequestAdapterOptions, powerPreference),
        "offsetof mismatch for RequestAdapterOptions::powerPreference");
static_assert(offsetof(RequestAdapterOptions, forceFallbackAdapter) == offsetof(WGPURequestAdapterOptions, forceFallbackAdapter),
        "offsetof mismatch for RequestAdapterOptions::forceFallbackAdapter");
static_assert(offsetof(RequestAdapterOptions, backendType) == offsetof(WGPURequestAdapterOptions, backendType),
        "offsetof mismatch for RequestAdapterOptions::backendType");
static_assert(offsetof(RequestAdapterOptions, compatibleSurface) == offsetof(WGPURequestAdapterOptions, compatibleSurface),
        "offsetof mismatch for RequestAdapterOptions::compatibleSurface");

// SamplerDescriptor implementation

SamplerDescriptor::operator const WGPUSamplerDescriptor&() const noexcept {
    return *reinterpret_cast<const WGPUSamplerDescriptor*>(this);
}

static_assert(sizeof(SamplerDescriptor) == sizeof(WGPUSamplerDescriptor), "sizeof mismatch for SamplerDescriptor");
static_assert(alignof(SamplerDescriptor) == alignof(WGPUSamplerDescriptor), "alignof mismatch for SamplerDescriptor");
static_assert(offsetof(SamplerDescriptor, nextInChain) == offsetof(WGPUSamplerDescriptor, nextInChain),
        "offsetof mismatch for SamplerDescriptor::nextInChain");
static_assert(offsetof(SamplerDescriptor, label) == offsetof(WGPUSamplerDescriptor, label),
        "offsetof mismatch for SamplerDescriptor::label");
static_assert(offsetof(SamplerDescriptor, addressModeU) == offsetof(WGPUSamplerDescriptor, addressModeU),
        "offsetof mismatch for SamplerDescriptor::addressModeU");
static_assert(offsetof(SamplerDescriptor, addressModeV) == offsetof(WGPUSamplerDescriptor, addressModeV),
        "offsetof mismatch for SamplerDescriptor::addressModeV");
static_assert(offsetof(SamplerDescriptor, addressModeW) == offsetof(WGPUSamplerDescriptor, addressModeW),
        "offsetof mismatch for SamplerDescriptor::addressModeW");
static_assert(offsetof(SamplerDescriptor, magFilter) == offsetof(WGPUSamplerDescriptor, magFilter),
        "offsetof mismatch for SamplerDescriptor::magFilter");
static_assert(offsetof(SamplerDescriptor, minFilter) == offsetof(WGPUSamplerDescriptor, minFilter),
        "offsetof mismatch for SamplerDescriptor::minFilter");
static_assert(offsetof(SamplerDescriptor, mipmapFilter) == offsetof(WGPUSamplerDescriptor, mipmapFilter),
        "offsetof mismatch for SamplerDescriptor::mipmapFilter");
static_assert(offsetof(SamplerDescriptor, lodMinClamp) == offsetof(WGPUSamplerDescriptor, lodMinClamp),
        "offsetof mismatch for SamplerDescriptor::lodMinClamp");
static_assert(offsetof(SamplerDescriptor, lodMaxClamp) == offsetof(WGPUSamplerDescriptor, lodMaxClamp),
        "offsetof mismatch for SamplerDescriptor::lodMaxClamp");
static_assert(offsetof(SamplerDescriptor, compare) == offsetof(WGPUSamplerDescriptor, compare),
        "offsetof mismatch for SamplerDescriptor::compare");
static_assert(offsetof(SamplerDescriptor, maxAnisotropy) == offsetof(WGPUSamplerDescriptor, maxAnisotropy),
        "offsetof mismatch for SamplerDescriptor::maxAnisotropy");

// ShaderModuleDescriptor implementation

ShaderModuleDescriptor::operator const WGPUShaderModuleDescriptor&() const noexcept {
    return *reinterpret_cast<const WGPUShaderModuleDescriptor*>(this);
}

static_assert(sizeof(ShaderModuleDescriptor) == sizeof(WGPUShaderModuleDescriptor), "sizeof mismatch for ShaderModuleDescriptor");
static_assert(alignof(ShaderModuleDescriptor) == alignof(WGPUShaderModuleDescriptor), "alignof mismatch for ShaderModuleDescriptor");
static_assert(offsetof(ShaderModuleDescriptor, nextInChain) == offsetof(WGPUShaderModuleDescriptor, nextInChain),
        "offsetof mismatch for ShaderModuleDescriptor::nextInChain");
static_assert(offsetof(ShaderModuleDescriptor, label) == offsetof(WGPUShaderModuleDescriptor, label),
        "offsetof mismatch for ShaderModuleDescriptor::label");

// SharedBufferMemoryDescriptor implementation

SharedBufferMemoryDescriptor::operator const WGPUSharedBufferMemoryDescriptor&() const noexcept {
    return *reinterpret_cast<const WGPUSharedBufferMemoryDescriptor*>(this);
}

static_assert(sizeof(SharedBufferMemoryDescriptor) == sizeof(WGPUSharedBufferMemoryDescriptor), "sizeof mismatch for SharedBufferMemoryDescriptor");
static_assert(alignof(SharedBufferMemoryDescriptor) == alignof(WGPUSharedBufferMemoryDescriptor), "alignof mismatch for SharedBufferMemoryDescriptor");
static_assert(offsetof(SharedBufferMemoryDescriptor, nextInChain) == offsetof(WGPUSharedBufferMemoryDescriptor, nextInChain),
        "offsetof mismatch for SharedBufferMemoryDescriptor::nextInChain");
static_assert(offsetof(SharedBufferMemoryDescriptor, label) == offsetof(WGPUSharedBufferMemoryDescriptor, label),
        "offsetof mismatch for SharedBufferMemoryDescriptor::label");

// SharedFenceDescriptor implementation

SharedFenceDescriptor::operator const WGPUSharedFenceDescriptor&() const noexcept {
    return *reinterpret_cast<const WGPUSharedFenceDescriptor*>(this);
}

static_assert(sizeof(SharedFenceDescriptor) == sizeof(WGPUSharedFenceDescriptor), "sizeof mismatch for SharedFenceDescriptor");
static_assert(alignof(SharedFenceDescriptor) == alignof(WGPUSharedFenceDescriptor), "alignof mismatch for SharedFenceDescriptor");
static_assert(offsetof(SharedFenceDescriptor, nextInChain) == offsetof(WGPUSharedFenceDescriptor, nextInChain),
        "offsetof mismatch for SharedFenceDescriptor::nextInChain");
static_assert(offsetof(SharedFenceDescriptor, label) == offsetof(WGPUSharedFenceDescriptor, label),
        "offsetof mismatch for SharedFenceDescriptor::label");

// SharedFenceExportInfo implementation

SharedFenceExportInfo::operator const WGPUSharedFenceExportInfo&() const noexcept {
    return *reinterpret_cast<const WGPUSharedFenceExportInfo*>(this);
}

static_assert(sizeof(SharedFenceExportInfo) == sizeof(WGPUSharedFenceExportInfo), "sizeof mismatch for SharedFenceExportInfo");
static_assert(alignof(SharedFenceExportInfo) == alignof(WGPUSharedFenceExportInfo), "alignof mismatch for SharedFenceExportInfo");
static_assert(offsetof(SharedFenceExportInfo, nextInChain) == offsetof(WGPUSharedFenceExportInfo, nextInChain),
        "offsetof mismatch for SharedFenceExportInfo::nextInChain");
static_assert(offsetof(SharedFenceExportInfo, type) == offsetof(WGPUSharedFenceExportInfo, type),
        "offsetof mismatch for SharedFenceExportInfo::type");

// SharedTextureMemoryAHardwareBufferProperties implementation
SharedTextureMemoryAHardwareBufferProperties::SharedTextureMemoryAHardwareBufferProperties()
: ChainedStructOut { nullptr, SType::SharedTextureMemoryAHardwareBufferProperties } {}
struct SharedTextureMemoryAHardwareBufferProperties::Init {
    ChainedStructOut *  nextInChain;
    YCbCrVkDescriptor yCbCrInfo = {};
};
SharedTextureMemoryAHardwareBufferProperties::SharedTextureMemoryAHardwareBufferProperties(SharedTextureMemoryAHardwareBufferProperties::Init&& init) :
    ChainedStructOut { init.nextInChain, SType::SharedTextureMemoryAHardwareBufferProperties },
    yCbCrInfo(std::move(init.yCbCrInfo)) {}

SharedTextureMemoryAHardwareBufferProperties::operator const WGPUSharedTextureMemoryAHardwareBufferProperties&() const noexcept {
    return *reinterpret_cast<const WGPUSharedTextureMemoryAHardwareBufferProperties*>(this);
}

static_assert(sizeof(SharedTextureMemoryAHardwareBufferProperties) == sizeof(WGPUSharedTextureMemoryAHardwareBufferProperties), "sizeof mismatch for SharedTextureMemoryAHardwareBufferProperties");
static_assert(alignof(SharedTextureMemoryAHardwareBufferProperties) == alignof(WGPUSharedTextureMemoryAHardwareBufferProperties), "alignof mismatch for SharedTextureMemoryAHardwareBufferProperties");
static_assert(offsetof(SharedTextureMemoryAHardwareBufferProperties, yCbCrInfo) == offsetof(WGPUSharedTextureMemoryAHardwareBufferProperties, yCbCrInfo),
        "offsetof mismatch for SharedTextureMemoryAHardwareBufferProperties::yCbCrInfo");

// SharedTextureMemoryBeginAccessDescriptor implementation

SharedTextureMemoryBeginAccessDescriptor::operator const WGPUSharedTextureMemoryBeginAccessDescriptor&() const noexcept {
    return *reinterpret_cast<const WGPUSharedTextureMemoryBeginAccessDescriptor*>(this);
}

static_assert(sizeof(SharedTextureMemoryBeginAccessDescriptor) == sizeof(WGPUSharedTextureMemoryBeginAccessDescriptor), "sizeof mismatch for SharedTextureMemoryBeginAccessDescriptor");
static_assert(alignof(SharedTextureMemoryBeginAccessDescriptor) == alignof(WGPUSharedTextureMemoryBeginAccessDescriptor), "alignof mismatch for SharedTextureMemoryBeginAccessDescriptor");
static_assert(offsetof(SharedTextureMemoryBeginAccessDescriptor, nextInChain) == offsetof(WGPUSharedTextureMemoryBeginAccessDescriptor, nextInChain),
        "offsetof mismatch for SharedTextureMemoryBeginAccessDescriptor::nextInChain");
static_assert(offsetof(SharedTextureMemoryBeginAccessDescriptor, concurrentRead) == offsetof(WGPUSharedTextureMemoryBeginAccessDescriptor, concurrentRead),
        "offsetof mismatch for SharedTextureMemoryBeginAccessDescriptor::concurrentRead");
static_assert(offsetof(SharedTextureMemoryBeginAccessDescriptor, initialized) == offsetof(WGPUSharedTextureMemoryBeginAccessDescriptor, initialized),
        "offsetof mismatch for SharedTextureMemoryBeginAccessDescriptor::initialized");
static_assert(offsetof(SharedTextureMemoryBeginAccessDescriptor, fenceCount) == offsetof(WGPUSharedTextureMemoryBeginAccessDescriptor, fenceCount),
        "offsetof mismatch for SharedTextureMemoryBeginAccessDescriptor::fenceCount");
static_assert(offsetof(SharedTextureMemoryBeginAccessDescriptor, fences) == offsetof(WGPUSharedTextureMemoryBeginAccessDescriptor, fences),
        "offsetof mismatch for SharedTextureMemoryBeginAccessDescriptor::fences");
static_assert(offsetof(SharedTextureMemoryBeginAccessDescriptor, signaledValueCount) == offsetof(WGPUSharedTextureMemoryBeginAccessDescriptor, signaledValueCount),
        "offsetof mismatch for SharedTextureMemoryBeginAccessDescriptor::signaledValueCount");
static_assert(offsetof(SharedTextureMemoryBeginAccessDescriptor, signaledValues) == offsetof(WGPUSharedTextureMemoryBeginAccessDescriptor, signaledValues),
        "offsetof mismatch for SharedTextureMemoryBeginAccessDescriptor::signaledValues");

// SharedTextureMemoryDmaBufDescriptor implementation
SharedTextureMemoryDmaBufDescriptor::SharedTextureMemoryDmaBufDescriptor()
: ChainedStruct { nullptr, SType::SharedTextureMemoryDmaBufDescriptor } {}
struct SharedTextureMemoryDmaBufDescriptor::Init {
    ChainedStruct * const nextInChain;
    Extent3D size = {};
    uint32_t drmFormat;
    uint64_t drmModifier;
    size_t planeCount;
    SharedTextureMemoryDmaBufPlane const * planes = nullptr;
};
SharedTextureMemoryDmaBufDescriptor::SharedTextureMemoryDmaBufDescriptor(SharedTextureMemoryDmaBufDescriptor::Init&& init) :
    ChainedStruct { init.nextInChain, SType::SharedTextureMemoryDmaBufDescriptor },
    size(std::move(init.size)),
    drmFormat(std::move(init.drmFormat)),
    drmModifier(std::move(init.drmModifier)),
    planeCount(std::move(init.planeCount)),
    planes(std::move(init.planes)) {}

SharedTextureMemoryDmaBufDescriptor::operator const WGPUSharedTextureMemoryDmaBufDescriptor&() const noexcept {
    return *reinterpret_cast<const WGPUSharedTextureMemoryDmaBufDescriptor*>(this);
}

static_assert(sizeof(SharedTextureMemoryDmaBufDescriptor) == sizeof(WGPUSharedTextureMemoryDmaBufDescriptor), "sizeof mismatch for SharedTextureMemoryDmaBufDescriptor");
static_assert(alignof(SharedTextureMemoryDmaBufDescriptor) == alignof(WGPUSharedTextureMemoryDmaBufDescriptor), "alignof mismatch for SharedTextureMemoryDmaBufDescriptor");
static_assert(offsetof(SharedTextureMemoryDmaBufDescriptor, size) == offsetof(WGPUSharedTextureMemoryDmaBufDescriptor, size),
        "offsetof mismatch for SharedTextureMemoryDmaBufDescriptor::size");
static_assert(offsetof(SharedTextureMemoryDmaBufDescriptor, drmFormat) == offsetof(WGPUSharedTextureMemoryDmaBufDescriptor, drmFormat),
        "offsetof mismatch for SharedTextureMemoryDmaBufDescriptor::drmFormat");
static_assert(offsetof(SharedTextureMemoryDmaBufDescriptor, drmModifier) == offsetof(WGPUSharedTextureMemoryDmaBufDescriptor, drmModifier),
        "offsetof mismatch for SharedTextureMemoryDmaBufDescriptor::drmModifier");
static_assert(offsetof(SharedTextureMemoryDmaBufDescriptor, planeCount) == offsetof(WGPUSharedTextureMemoryDmaBufDescriptor, planeCount),
        "offsetof mismatch for SharedTextureMemoryDmaBufDescriptor::planeCount");
static_assert(offsetof(SharedTextureMemoryDmaBufDescriptor, planes) == offsetof(WGPUSharedTextureMemoryDmaBufDescriptor, planes),
        "offsetof mismatch for SharedTextureMemoryDmaBufDescriptor::planes");

// SharedTextureMemoryMetalEndAccessState implementation
SharedTextureMemoryMetalEndAccessState::SharedTextureMemoryMetalEndAccessState()
: ChainedStructOut { nullptr, SType::SharedTextureMemoryMetalEndAccessState } {}
struct SharedTextureMemoryMetalEndAccessState::Init {
    ChainedStructOut *  nextInChain;
    Future commandsScheduledFuture = {};
};
SharedTextureMemoryMetalEndAccessState::SharedTextureMemoryMetalEndAccessState(SharedTextureMemoryMetalEndAccessState::Init&& init) :
    ChainedStructOut { init.nextInChain, SType::SharedTextureMemoryMetalEndAccessState },
    commandsScheduledFuture(std::move(init.commandsScheduledFuture)) {}

SharedTextureMemoryMetalEndAccessState::operator const WGPUSharedTextureMemoryMetalEndAccessState&() const noexcept {
    return *reinterpret_cast<const WGPUSharedTextureMemoryMetalEndAccessState*>(this);
}

static_assert(sizeof(SharedTextureMemoryMetalEndAccessState) == sizeof(WGPUSharedTextureMemoryMetalEndAccessState), "sizeof mismatch for SharedTextureMemoryMetalEndAccessState");
static_assert(alignof(SharedTextureMemoryMetalEndAccessState) == alignof(WGPUSharedTextureMemoryMetalEndAccessState), "alignof mismatch for SharedTextureMemoryMetalEndAccessState");
static_assert(offsetof(SharedTextureMemoryMetalEndAccessState, commandsScheduledFuture) == offsetof(WGPUSharedTextureMemoryMetalEndAccessState, commandsScheduledFuture),
        "offsetof mismatch for SharedTextureMemoryMetalEndAccessState::commandsScheduledFuture");

// SurfaceDescriptor implementation

SurfaceDescriptor::operator const WGPUSurfaceDescriptor&() const noexcept {
    return *reinterpret_cast<const WGPUSurfaceDescriptor*>(this);
}

static_assert(sizeof(SurfaceDescriptor) == sizeof(WGPUSurfaceDescriptor), "sizeof mismatch for SurfaceDescriptor");
static_assert(alignof(SurfaceDescriptor) == alignof(WGPUSurfaceDescriptor), "alignof mismatch for SurfaceDescriptor");
static_assert(offsetof(SurfaceDescriptor, nextInChain) == offsetof(WGPUSurfaceDescriptor, nextInChain),
        "offsetof mismatch for SurfaceDescriptor::nextInChain");
static_assert(offsetof(SurfaceDescriptor, label) == offsetof(WGPUSurfaceDescriptor, label),
        "offsetof mismatch for SurfaceDescriptor::label");

// TexelCopyBufferInfo implementation

TexelCopyBufferInfo::operator const WGPUTexelCopyBufferInfo&() const noexcept {
    return *reinterpret_cast<const WGPUTexelCopyBufferInfo*>(this);
}

static_assert(sizeof(TexelCopyBufferInfo) == sizeof(WGPUTexelCopyBufferInfo), "sizeof mismatch for TexelCopyBufferInfo");
static_assert(alignof(TexelCopyBufferInfo) == alignof(WGPUTexelCopyBufferInfo), "alignof mismatch for TexelCopyBufferInfo");
static_assert(offsetof(TexelCopyBufferInfo, layout) == offsetof(WGPUTexelCopyBufferInfo, layout),
        "offsetof mismatch for TexelCopyBufferInfo::layout");
static_assert(offsetof(TexelCopyBufferInfo, buffer) == offsetof(WGPUTexelCopyBufferInfo, buffer),
        "offsetof mismatch for TexelCopyBufferInfo::buffer");

// TexelCopyTextureInfo implementation

TexelCopyTextureInfo::operator const WGPUTexelCopyTextureInfo&() const noexcept {
    return *reinterpret_cast<const WGPUTexelCopyTextureInfo*>(this);
}

static_assert(sizeof(TexelCopyTextureInfo) == sizeof(WGPUTexelCopyTextureInfo), "sizeof mismatch for TexelCopyTextureInfo");
static_assert(alignof(TexelCopyTextureInfo) == alignof(WGPUTexelCopyTextureInfo), "alignof mismatch for TexelCopyTextureInfo");
static_assert(offsetof(TexelCopyTextureInfo, texture) == offsetof(WGPUTexelCopyTextureInfo, texture),
        "offsetof mismatch for TexelCopyTextureInfo::texture");
static_assert(offsetof(TexelCopyTextureInfo, mipLevel) == offsetof(WGPUTexelCopyTextureInfo, mipLevel),
        "offsetof mismatch for TexelCopyTextureInfo::mipLevel");
static_assert(offsetof(TexelCopyTextureInfo, origin) == offsetof(WGPUTexelCopyTextureInfo, origin),
        "offsetof mismatch for TexelCopyTextureInfo::origin");
static_assert(offsetof(TexelCopyTextureInfo, aspect) == offsetof(WGPUTexelCopyTextureInfo, aspect),
        "offsetof mismatch for TexelCopyTextureInfo::aspect");

// TextureComponentSwizzleDescriptor implementation
TextureComponentSwizzleDescriptor::TextureComponentSwizzleDescriptor()
: ChainedStruct { nullptr, SType::TextureComponentSwizzleDescriptor } {}
struct TextureComponentSwizzleDescriptor::Init {
    ChainedStruct * const nextInChain;
    TextureComponentSwizzle swizzle = {};
};
TextureComponentSwizzleDescriptor::TextureComponentSwizzleDescriptor(TextureComponentSwizzleDescriptor::Init&& init) :
    ChainedStruct { init.nextInChain, SType::TextureComponentSwizzleDescriptor },
    swizzle(std::move(init.swizzle)) {}

TextureComponentSwizzleDescriptor::operator const WGPUTextureComponentSwizzleDescriptor&() const noexcept {
    return *reinterpret_cast<const WGPUTextureComponentSwizzleDescriptor*>(this);
}

static_assert(sizeof(TextureComponentSwizzleDescriptor) == sizeof(WGPUTextureComponentSwizzleDescriptor), "sizeof mismatch for TextureComponentSwizzleDescriptor");
static_assert(alignof(TextureComponentSwizzleDescriptor) == alignof(WGPUTextureComponentSwizzleDescriptor), "alignof mismatch for TextureComponentSwizzleDescriptor");
static_assert(offsetof(TextureComponentSwizzleDescriptor, swizzle) == offsetof(WGPUTextureComponentSwizzleDescriptor, swizzle),
        "offsetof mismatch for TextureComponentSwizzleDescriptor::swizzle");

// TextureDescriptor implementation

TextureDescriptor::operator const WGPUTextureDescriptor&() const noexcept {
    return *reinterpret_cast<const WGPUTextureDescriptor*>(this);
}

static_assert(sizeof(TextureDescriptor) == sizeof(WGPUTextureDescriptor), "sizeof mismatch for TextureDescriptor");
static_assert(alignof(TextureDescriptor) == alignof(WGPUTextureDescriptor), "alignof mismatch for TextureDescriptor");
static_assert(offsetof(TextureDescriptor, nextInChain) == offsetof(WGPUTextureDescriptor, nextInChain),
        "offsetof mismatch for TextureDescriptor::nextInChain");
static_assert(offsetof(TextureDescriptor, label) == offsetof(WGPUTextureDescriptor, label),
        "offsetof mismatch for TextureDescriptor::label");
static_assert(offsetof(TextureDescriptor, usage) == offsetof(WGPUTextureDescriptor, usage),
        "offsetof mismatch for TextureDescriptor::usage");
static_assert(offsetof(TextureDescriptor, dimension) == offsetof(WGPUTextureDescriptor, dimension),
        "offsetof mismatch for TextureDescriptor::dimension");
static_assert(offsetof(TextureDescriptor, size) == offsetof(WGPUTextureDescriptor, size),
        "offsetof mismatch for TextureDescriptor::size");
static_assert(offsetof(TextureDescriptor, format) == offsetof(WGPUTextureDescriptor, format),
        "offsetof mismatch for TextureDescriptor::format");
static_assert(offsetof(TextureDescriptor, mipLevelCount) == offsetof(WGPUTextureDescriptor, mipLevelCount),
        "offsetof mismatch for TextureDescriptor::mipLevelCount");
static_assert(offsetof(TextureDescriptor, sampleCount) == offsetof(WGPUTextureDescriptor, sampleCount),
        "offsetof mismatch for TextureDescriptor::sampleCount");
static_assert(offsetof(TextureDescriptor, viewFormatCount) == offsetof(WGPUTextureDescriptor, viewFormatCount),
        "offsetof mismatch for TextureDescriptor::viewFormatCount");
static_assert(offsetof(TextureDescriptor, viewFormats) == offsetof(WGPUTextureDescriptor, viewFormats),
        "offsetof mismatch for TextureDescriptor::viewFormats");

// VertexBufferLayout implementation

VertexBufferLayout::operator const WGPUVertexBufferLayout&() const noexcept {
    return *reinterpret_cast<const WGPUVertexBufferLayout*>(this);
}

static_assert(sizeof(VertexBufferLayout) == sizeof(WGPUVertexBufferLayout), "sizeof mismatch for VertexBufferLayout");
static_assert(alignof(VertexBufferLayout) == alignof(WGPUVertexBufferLayout), "alignof mismatch for VertexBufferLayout");
static_assert(offsetof(VertexBufferLayout, nextInChain) == offsetof(WGPUVertexBufferLayout, nextInChain),
        "offsetof mismatch for VertexBufferLayout::nextInChain");
static_assert(offsetof(VertexBufferLayout, stepMode) == offsetof(WGPUVertexBufferLayout, stepMode),
        "offsetof mismatch for VertexBufferLayout::stepMode");
static_assert(offsetof(VertexBufferLayout, arrayStride) == offsetof(WGPUVertexBufferLayout, arrayStride),
        "offsetof mismatch for VertexBufferLayout::arrayStride");
static_assert(offsetof(VertexBufferLayout, attributeCount) == offsetof(WGPUVertexBufferLayout, attributeCount),
        "offsetof mismatch for VertexBufferLayout::attributeCount");
static_assert(offsetof(VertexBufferLayout, attributes) == offsetof(WGPUVertexBufferLayout, attributes),
        "offsetof mismatch for VertexBufferLayout::attributes");

// AdapterInfo implementation
AdapterInfo::AdapterInfo() = default;
AdapterInfo::~AdapterInfo() {
    FreeMembers();
}

AdapterInfo::AdapterInfo(AdapterInfo&& rhs)
    : vendor(rhs.vendor),
            architecture(rhs.architecture),
            device(rhs.device),
            description(rhs.description),
            backendType(rhs.backendType),
            adapterType(rhs.adapterType),
            vendorID(rhs.vendorID),
            deviceID(rhs.deviceID),
            subgroupMinSize(rhs.subgroupMinSize),
            subgroupMaxSize(rhs.subgroupMaxSize){
    Reset(rhs);
}

AdapterInfo& AdapterInfo::operator=(AdapterInfo&& rhs) {
    if (&rhs == this) {
        return *this;
    }
    FreeMembers();
    detail::AsNonConstReference(this->vendor) = std::move(rhs.vendor);
    detail::AsNonConstReference(this->architecture) = std::move(rhs.architecture);
    detail::AsNonConstReference(this->device) = std::move(rhs.device);
    detail::AsNonConstReference(this->description) = std::move(rhs.description);
    detail::AsNonConstReference(this->backendType) = std::move(rhs.backendType);
    detail::AsNonConstReference(this->adapterType) = std::move(rhs.adapterType);
    detail::AsNonConstReference(this->vendorID) = std::move(rhs.vendorID);
    detail::AsNonConstReference(this->deviceID) = std::move(rhs.deviceID);
    detail::AsNonConstReference(this->subgroupMinSize) = std::move(rhs.subgroupMinSize);
    detail::AsNonConstReference(this->subgroupMaxSize) = std::move(rhs.subgroupMaxSize);
    Reset(rhs);
    return *this;
}

void AdapterInfo::FreeMembers() {
    bool needsFreeing = false;    if (this->vendor.data != nullptr) { needsFreeing = true; }    if (this->architecture.data != nullptr) { needsFreeing = true; }    if (this->device.data != nullptr) { needsFreeing = true; }    if (this->description.data != nullptr) { needsFreeing = true; }if (needsFreeing) {
        wgpuAdapterInfoFreeMembers(
            *reinterpret_cast<WGPUAdapterInfo*>(this));
    }
}

// static
void AdapterInfo::Reset(AdapterInfo& value) {
    AdapterInfo defaultValue{};
    detail::AsNonConstReference(value.vendor) = defaultValue.vendor;
    detail::AsNonConstReference(value.architecture) = defaultValue.architecture;
    detail::AsNonConstReference(value.device) = defaultValue.device;
    detail::AsNonConstReference(value.description) = defaultValue.description;
    detail::AsNonConstReference(value.backendType) = defaultValue.backendType;
    detail::AsNonConstReference(value.adapterType) = defaultValue.adapterType;
    detail::AsNonConstReference(value.vendorID) = defaultValue.vendorID;
    detail::AsNonConstReference(value.deviceID) = defaultValue.deviceID;
    detail::AsNonConstReference(value.subgroupMinSize) = defaultValue.subgroupMinSize;
    detail::AsNonConstReference(value.subgroupMaxSize) = defaultValue.subgroupMaxSize;
}

AdapterInfo::operator const WGPUAdapterInfo&() const noexcept {
    return *reinterpret_cast<const WGPUAdapterInfo*>(this);
}

static_assert(sizeof(AdapterInfo) == sizeof(WGPUAdapterInfo), "sizeof mismatch for AdapterInfo");
static_assert(alignof(AdapterInfo) == alignof(WGPUAdapterInfo), "alignof mismatch for AdapterInfo");
static_assert(offsetof(AdapterInfo, nextInChain) == offsetof(WGPUAdapterInfo, nextInChain),
        "offsetof mismatch for AdapterInfo::nextInChain");
static_assert(offsetof(AdapterInfo, vendor) == offsetof(WGPUAdapterInfo, vendor),
        "offsetof mismatch for AdapterInfo::vendor");
static_assert(offsetof(AdapterInfo, architecture) == offsetof(WGPUAdapterInfo, architecture),
        "offsetof mismatch for AdapterInfo::architecture");
static_assert(offsetof(AdapterInfo, device) == offsetof(WGPUAdapterInfo, device),
        "offsetof mismatch for AdapterInfo::device");
static_assert(offsetof(AdapterInfo, description) == offsetof(WGPUAdapterInfo, description),
        "offsetof mismatch for AdapterInfo::description");
static_assert(offsetof(AdapterInfo, backendType) == offsetof(WGPUAdapterInfo, backendType),
        "offsetof mismatch for AdapterInfo::backendType");
static_assert(offsetof(AdapterInfo, adapterType) == offsetof(WGPUAdapterInfo, adapterType),
        "offsetof mismatch for AdapterInfo::adapterType");
static_assert(offsetof(AdapterInfo, vendorID) == offsetof(WGPUAdapterInfo, vendorID),
        "offsetof mismatch for AdapterInfo::vendorID");
static_assert(offsetof(AdapterInfo, deviceID) == offsetof(WGPUAdapterInfo, deviceID),
        "offsetof mismatch for AdapterInfo::deviceID");
static_assert(offsetof(AdapterInfo, subgroupMinSize) == offsetof(WGPUAdapterInfo, subgroupMinSize),
        "offsetof mismatch for AdapterInfo::subgroupMinSize");
static_assert(offsetof(AdapterInfo, subgroupMaxSize) == offsetof(WGPUAdapterInfo, subgroupMaxSize),
        "offsetof mismatch for AdapterInfo::subgroupMaxSize");

// BindGroupDescriptor implementation

BindGroupDescriptor::operator const WGPUBindGroupDescriptor&() const noexcept {
    return *reinterpret_cast<const WGPUBindGroupDescriptor*>(this);
}

static_assert(sizeof(BindGroupDescriptor) == sizeof(WGPUBindGroupDescriptor), "sizeof mismatch for BindGroupDescriptor");
static_assert(alignof(BindGroupDescriptor) == alignof(WGPUBindGroupDescriptor), "alignof mismatch for BindGroupDescriptor");
static_assert(offsetof(BindGroupDescriptor, nextInChain) == offsetof(WGPUBindGroupDescriptor, nextInChain),
        "offsetof mismatch for BindGroupDescriptor::nextInChain");
static_assert(offsetof(BindGroupDescriptor, label) == offsetof(WGPUBindGroupDescriptor, label),
        "offsetof mismatch for BindGroupDescriptor::label");
static_assert(offsetof(BindGroupDescriptor, layout) == offsetof(WGPUBindGroupDescriptor, layout),
        "offsetof mismatch for BindGroupDescriptor::layout");
static_assert(offsetof(BindGroupDescriptor, entryCount) == offsetof(WGPUBindGroupDescriptor, entryCount),
        "offsetof mismatch for BindGroupDescriptor::entryCount");
static_assert(offsetof(BindGroupDescriptor, entries) == offsetof(WGPUBindGroupDescriptor, entries),
        "offsetof mismatch for BindGroupDescriptor::entries");

// BindGroupLayoutDescriptor implementation

BindGroupLayoutDescriptor::operator const WGPUBindGroupLayoutDescriptor&() const noexcept {
    return *reinterpret_cast<const WGPUBindGroupLayoutDescriptor*>(this);
}

static_assert(sizeof(BindGroupLayoutDescriptor) == sizeof(WGPUBindGroupLayoutDescriptor), "sizeof mismatch for BindGroupLayoutDescriptor");
static_assert(alignof(BindGroupLayoutDescriptor) == alignof(WGPUBindGroupLayoutDescriptor), "alignof mismatch for BindGroupLayoutDescriptor");
static_assert(offsetof(BindGroupLayoutDescriptor, nextInChain) == offsetof(WGPUBindGroupLayoutDescriptor, nextInChain),
        "offsetof mismatch for BindGroupLayoutDescriptor::nextInChain");
static_assert(offsetof(BindGroupLayoutDescriptor, label) == offsetof(WGPUBindGroupLayoutDescriptor, label),
        "offsetof mismatch for BindGroupLayoutDescriptor::label");
static_assert(offsetof(BindGroupLayoutDescriptor, entryCount) == offsetof(WGPUBindGroupLayoutDescriptor, entryCount),
        "offsetof mismatch for BindGroupLayoutDescriptor::entryCount");
static_assert(offsetof(BindGroupLayoutDescriptor, entries) == offsetof(WGPUBindGroupLayoutDescriptor, entries),
        "offsetof mismatch for BindGroupLayoutDescriptor::entries");

// ColorTargetState implementation

ColorTargetState::operator const WGPUColorTargetState&() const noexcept {
    return *reinterpret_cast<const WGPUColorTargetState*>(this);
}

static_assert(sizeof(ColorTargetState) == sizeof(WGPUColorTargetState), "sizeof mismatch for ColorTargetState");
static_assert(alignof(ColorTargetState) == alignof(WGPUColorTargetState), "alignof mismatch for ColorTargetState");
static_assert(offsetof(ColorTargetState, nextInChain) == offsetof(WGPUColorTargetState, nextInChain),
        "offsetof mismatch for ColorTargetState::nextInChain");
static_assert(offsetof(ColorTargetState, format) == offsetof(WGPUColorTargetState, format),
        "offsetof mismatch for ColorTargetState::format");
static_assert(offsetof(ColorTargetState, blend) == offsetof(WGPUColorTargetState, blend),
        "offsetof mismatch for ColorTargetState::blend");
static_assert(offsetof(ColorTargetState, writeMask) == offsetof(WGPUColorTargetState, writeMask),
        "offsetof mismatch for ColorTargetState::writeMask");

// CompilationInfo implementation

CompilationInfo::operator const WGPUCompilationInfo&() const noexcept {
    return *reinterpret_cast<const WGPUCompilationInfo*>(this);
}

static_assert(sizeof(CompilationInfo) == sizeof(WGPUCompilationInfo), "sizeof mismatch for CompilationInfo");
static_assert(alignof(CompilationInfo) == alignof(WGPUCompilationInfo), "alignof mismatch for CompilationInfo");
static_assert(offsetof(CompilationInfo, nextInChain) == offsetof(WGPUCompilationInfo, nextInChain),
        "offsetof mismatch for CompilationInfo::nextInChain");
static_assert(offsetof(CompilationInfo, messageCount) == offsetof(WGPUCompilationInfo, messageCount),
        "offsetof mismatch for CompilationInfo::messageCount");
static_assert(offsetof(CompilationInfo, messages) == offsetof(WGPUCompilationInfo, messages),
        "offsetof mismatch for CompilationInfo::messages");

// ComputePipelineDescriptor implementation

ComputePipelineDescriptor::operator const WGPUComputePipelineDescriptor&() const noexcept {
    return *reinterpret_cast<const WGPUComputePipelineDescriptor*>(this);
}

static_assert(sizeof(ComputePipelineDescriptor) == sizeof(WGPUComputePipelineDescriptor), "sizeof mismatch for ComputePipelineDescriptor");
static_assert(alignof(ComputePipelineDescriptor) == alignof(WGPUComputePipelineDescriptor), "alignof mismatch for ComputePipelineDescriptor");
static_assert(offsetof(ComputePipelineDescriptor, nextInChain) == offsetof(WGPUComputePipelineDescriptor, nextInChain),
        "offsetof mismatch for ComputePipelineDescriptor::nextInChain");
static_assert(offsetof(ComputePipelineDescriptor, label) == offsetof(WGPUComputePipelineDescriptor, label),
        "offsetof mismatch for ComputePipelineDescriptor::label");
static_assert(offsetof(ComputePipelineDescriptor, layout) == offsetof(WGPUComputePipelineDescriptor, layout),
        "offsetof mismatch for ComputePipelineDescriptor::layout");
static_assert(offsetof(ComputePipelineDescriptor, compute) == offsetof(WGPUComputePipelineDescriptor, compute),
        "offsetof mismatch for ComputePipelineDescriptor::compute");

// DawnFormatCapabilities implementation

DawnFormatCapabilities::operator const WGPUDawnFormatCapabilities&() const noexcept {
    return *reinterpret_cast<const WGPUDawnFormatCapabilities*>(this);
}

static_assert(sizeof(DawnFormatCapabilities) == sizeof(WGPUDawnFormatCapabilities), "sizeof mismatch for DawnFormatCapabilities");
static_assert(alignof(DawnFormatCapabilities) == alignof(WGPUDawnFormatCapabilities), "alignof mismatch for DawnFormatCapabilities");
static_assert(offsetof(DawnFormatCapabilities, nextInChain) == offsetof(WGPUDawnFormatCapabilities, nextInChain),
        "offsetof mismatch for DawnFormatCapabilities::nextInChain");

// DeviceDescriptor implementation
DeviceDescriptor::DeviceDescriptor() = default;
struct DeviceDescriptor::Init {
    ChainedStruct * const nextInChain;
    StringView label = {};
    size_t requiredFeatureCount = 0;
    FeatureName const * requiredFeatures = nullptr;
    Limits const * requiredLimits = nullptr;
    QueueDescriptor defaultQueue = {};
};
DeviceDescriptor::DeviceDescriptor(DeviceDescriptor::Init&& init) :
    detail::DeviceDescriptor {
        init.nextInChain,
        std::move(init.label),
        std::move(init.requiredFeatureCount),
        std::move(init.requiredFeatures),
        std::move(init.requiredLimits),
        std::move(init.defaultQueue),
    } {}
template <typename F, typename T>
void DeviceDescriptor::SetDeviceLostCallback(CallbackMode callbackMode, F callback, T userdata) {
    static_assert(offsetof(DeviceDescriptor, deviceLostCallbackInfo) == offsetof(WGPUDeviceDescriptor, deviceLostCallbackInfo),
                  "offsetof mismatch for DeviceDescriptor::deviceLostCallbackInfo");

    assert(deviceLostCallbackInfo.callback == nullptr);
    deviceLostCallbackInfo = detail::CallbackInfoHelper<WGPUDeviceLostCallbackInfo, F>::Create(std::move(callback), userdata);
    deviceLostCallbackInfo.mode = static_cast<WGPUCallbackMode>(callbackMode);
}
template <typename F>
void DeviceDescriptor::SetDeviceLostCallback(CallbackMode callbackMode, F callback) {
    assert(deviceLostCallbackInfo.callback == nullptr);
    deviceLostCallbackInfo = detail::CallbackInfoHelper<WGPUDeviceLostCallbackInfo, F>::Create(std::move(callback));
    deviceLostCallbackInfo.mode = static_cast<WGPUCallbackMode>(callbackMode);
}
template <typename F, typename T>
void DeviceDescriptor::SetUncapturedErrorCallback(F callback, T userdata) {
    static_assert(offsetof(DeviceDescriptor, uncapturedErrorCallbackInfo) == offsetof(WGPUDeviceDescriptor, uncapturedErrorCallbackInfo),
                  "offsetof mismatch for DeviceDescriptor::uncapturedErrorCallbackInfo");

    assert(uncapturedErrorCallbackInfo.callback == nullptr);
    uncapturedErrorCallbackInfo = detail::CallbackInfoHelper<WGPUUncapturedErrorCallbackInfo, F>::Create(std::move(callback), userdata);
}
template <typename F>
void DeviceDescriptor::SetUncapturedErrorCallback(F callback) {
    assert(uncapturedErrorCallbackInfo.callback == nullptr);
    uncapturedErrorCallbackInfo = detail::CallbackInfoHelper<WGPUUncapturedErrorCallbackInfo, F>::Create(std::move(callback));
}

DeviceDescriptor::operator const WGPUDeviceDescriptor&() const noexcept {
    return *reinterpret_cast<const WGPUDeviceDescriptor*>(this);
}

static_assert(sizeof(DeviceDescriptor) == sizeof(WGPUDeviceDescriptor), "sizeof mismatch for DeviceDescriptor");
static_assert(alignof(DeviceDescriptor) == alignof(WGPUDeviceDescriptor), "alignof mismatch for DeviceDescriptor");
static_assert(offsetof(DeviceDescriptor, nextInChain) == offsetof(WGPUDeviceDescriptor, nextInChain),
        "offsetof mismatch for DeviceDescriptor::nextInChain");
static_assert(offsetof(DeviceDescriptor, label) == offsetof(WGPUDeviceDescriptor, label),
        "offsetof mismatch for DeviceDescriptor::label");
static_assert(offsetof(DeviceDescriptor, requiredFeatureCount) == offsetof(WGPUDeviceDescriptor, requiredFeatureCount),
        "offsetof mismatch for DeviceDescriptor::requiredFeatureCount");
static_assert(offsetof(DeviceDescriptor, requiredFeatures) == offsetof(WGPUDeviceDescriptor, requiredFeatures),
        "offsetof mismatch for DeviceDescriptor::requiredFeatures");
static_assert(offsetof(DeviceDescriptor, requiredLimits) == offsetof(WGPUDeviceDescriptor, requiredLimits),
        "offsetof mismatch for DeviceDescriptor::requiredLimits");
static_assert(offsetof(DeviceDescriptor, defaultQueue) == offsetof(WGPUDeviceDescriptor, defaultQueue),
        "offsetof mismatch for DeviceDescriptor::defaultQueue");

// PipelineLayoutDescriptor implementation

PipelineLayoutDescriptor::operator const WGPUPipelineLayoutDescriptor&() const noexcept {
    return *reinterpret_cast<const WGPUPipelineLayoutDescriptor*>(this);
}

static_assert(sizeof(PipelineLayoutDescriptor) == sizeof(WGPUPipelineLayoutDescriptor), "sizeof mismatch for PipelineLayoutDescriptor");
static_assert(alignof(PipelineLayoutDescriptor) == alignof(WGPUPipelineLayoutDescriptor), "alignof mismatch for PipelineLayoutDescriptor");
static_assert(offsetof(PipelineLayoutDescriptor, nextInChain) == offsetof(WGPUPipelineLayoutDescriptor, nextInChain),
        "offsetof mismatch for PipelineLayoutDescriptor::nextInChain");
static_assert(offsetof(PipelineLayoutDescriptor, label) == offsetof(WGPUPipelineLayoutDescriptor, label),
        "offsetof mismatch for PipelineLayoutDescriptor::label");
static_assert(offsetof(PipelineLayoutDescriptor, bindGroupLayoutCount) == offsetof(WGPUPipelineLayoutDescriptor, bindGroupLayoutCount),
        "offsetof mismatch for PipelineLayoutDescriptor::bindGroupLayoutCount");
static_assert(offsetof(PipelineLayoutDescriptor, bindGroupLayouts) == offsetof(WGPUPipelineLayoutDescriptor, bindGroupLayouts),
        "offsetof mismatch for PipelineLayoutDescriptor::bindGroupLayouts");
static_assert(offsetof(PipelineLayoutDescriptor, immediateSize) == offsetof(WGPUPipelineLayoutDescriptor, immediateSize),
        "offsetof mismatch for PipelineLayoutDescriptor::immediateSize");

// RenderPassPixelLocalStorage implementation
RenderPassPixelLocalStorage::RenderPassPixelLocalStorage()
: ChainedStruct { nullptr, SType::RenderPassPixelLocalStorage } {}
struct RenderPassPixelLocalStorage::Init {
    ChainedStruct * const nextInChain;
    uint64_t totalPixelLocalStorageSize;
    size_t storageAttachmentCount = 0;
    RenderPassStorageAttachment const * storageAttachments = nullptr;
};
RenderPassPixelLocalStorage::RenderPassPixelLocalStorage(RenderPassPixelLocalStorage::Init&& init) :
    ChainedStruct { init.nextInChain, SType::RenderPassPixelLocalStorage },
    totalPixelLocalStorageSize(std::move(init.totalPixelLocalStorageSize)),
    storageAttachmentCount(std::move(init.storageAttachmentCount)),
    storageAttachments(std::move(init.storageAttachments)) {}

RenderPassPixelLocalStorage::operator const WGPURenderPassPixelLocalStorage&() const noexcept {
    return *reinterpret_cast<const WGPURenderPassPixelLocalStorage*>(this);
}

static_assert(sizeof(RenderPassPixelLocalStorage) == sizeof(WGPURenderPassPixelLocalStorage), "sizeof mismatch for RenderPassPixelLocalStorage");
static_assert(alignof(RenderPassPixelLocalStorage) == alignof(WGPURenderPassPixelLocalStorage), "alignof mismatch for RenderPassPixelLocalStorage");
static_assert(offsetof(RenderPassPixelLocalStorage, totalPixelLocalStorageSize) == offsetof(WGPURenderPassPixelLocalStorage, totalPixelLocalStorageSize),
        "offsetof mismatch for RenderPassPixelLocalStorage::totalPixelLocalStorageSize");
static_assert(offsetof(RenderPassPixelLocalStorage, storageAttachmentCount) == offsetof(WGPURenderPassPixelLocalStorage, storageAttachmentCount),
        "offsetof mismatch for RenderPassPixelLocalStorage::storageAttachmentCount");
static_assert(offsetof(RenderPassPixelLocalStorage, storageAttachments) == offsetof(WGPURenderPassPixelLocalStorage, storageAttachments),
        "offsetof mismatch for RenderPassPixelLocalStorage::storageAttachments");

// SharedTextureMemoryDescriptor implementation

SharedTextureMemoryDescriptor::operator const WGPUSharedTextureMemoryDescriptor&() const noexcept {
    return *reinterpret_cast<const WGPUSharedTextureMemoryDescriptor*>(this);
}

static_assert(sizeof(SharedTextureMemoryDescriptor) == sizeof(WGPUSharedTextureMemoryDescriptor), "sizeof mismatch for SharedTextureMemoryDescriptor");
static_assert(alignof(SharedTextureMemoryDescriptor) == alignof(WGPUSharedTextureMemoryDescriptor), "alignof mismatch for SharedTextureMemoryDescriptor");
static_assert(offsetof(SharedTextureMemoryDescriptor, nextInChain) == offsetof(WGPUSharedTextureMemoryDescriptor, nextInChain),
        "offsetof mismatch for SharedTextureMemoryDescriptor::nextInChain");
static_assert(offsetof(SharedTextureMemoryDescriptor, label) == offsetof(WGPUSharedTextureMemoryDescriptor, label),
        "offsetof mismatch for SharedTextureMemoryDescriptor::label");

// SharedTextureMemoryEndAccessState implementation
SharedTextureMemoryEndAccessState::SharedTextureMemoryEndAccessState() = default;
SharedTextureMemoryEndAccessState::~SharedTextureMemoryEndAccessState() {
    FreeMembers();
}

SharedTextureMemoryEndAccessState::SharedTextureMemoryEndAccessState(SharedTextureMemoryEndAccessState&& rhs)
    : initialized(rhs.initialized),
            fenceCount(rhs.fenceCount),
            fences(rhs.fences),
            signaledValueCount(rhs.signaledValueCount),
            signaledValues(rhs.signaledValues){
    Reset(rhs);
}

SharedTextureMemoryEndAccessState& SharedTextureMemoryEndAccessState::operator=(SharedTextureMemoryEndAccessState&& rhs) {
    if (&rhs == this) {
        return *this;
    }
    FreeMembers();
    detail::AsNonConstReference(this->initialized) = std::move(rhs.initialized);
    detail::AsNonConstReference(this->fenceCount) = std::move(rhs.fenceCount);
    detail::AsNonConstReference(this->fences) = std::move(rhs.fences);
    detail::AsNonConstReference(this->signaledValueCount) = std::move(rhs.signaledValueCount);
    detail::AsNonConstReference(this->signaledValues) = std::move(rhs.signaledValues);
    Reset(rhs);
    return *this;
}

void SharedTextureMemoryEndAccessState::FreeMembers() {
    bool needsFreeing = false;    if (this->fences != nullptr) { needsFreeing = true; }    if (this->signaledValues != nullptr) { needsFreeing = true; }if (needsFreeing) {
        wgpuSharedTextureMemoryEndAccessStateFreeMembers(
            *reinterpret_cast<WGPUSharedTextureMemoryEndAccessState*>(this));
    }
}

// static
void SharedTextureMemoryEndAccessState::Reset(SharedTextureMemoryEndAccessState& value) {
    SharedTextureMemoryEndAccessState defaultValue{};
    detail::AsNonConstReference(value.initialized) = defaultValue.initialized;
    detail::AsNonConstReference(value.fenceCount) = defaultValue.fenceCount;
    detail::AsNonConstReference(value.fences) = defaultValue.fences;
    detail::AsNonConstReference(value.signaledValueCount) = defaultValue.signaledValueCount;
    detail::AsNonConstReference(value.signaledValues) = defaultValue.signaledValues;
}

SharedTextureMemoryEndAccessState::operator const WGPUSharedTextureMemoryEndAccessState&() const noexcept {
    return *reinterpret_cast<const WGPUSharedTextureMemoryEndAccessState*>(this);
}

static_assert(sizeof(SharedTextureMemoryEndAccessState) == sizeof(WGPUSharedTextureMemoryEndAccessState), "sizeof mismatch for SharedTextureMemoryEndAccessState");
static_assert(alignof(SharedTextureMemoryEndAccessState) == alignof(WGPUSharedTextureMemoryEndAccessState), "alignof mismatch for SharedTextureMemoryEndAccessState");
static_assert(offsetof(SharedTextureMemoryEndAccessState, nextInChain) == offsetof(WGPUSharedTextureMemoryEndAccessState, nextInChain),
        "offsetof mismatch for SharedTextureMemoryEndAccessState::nextInChain");
static_assert(offsetof(SharedTextureMemoryEndAccessState, initialized) == offsetof(WGPUSharedTextureMemoryEndAccessState, initialized),
        "offsetof mismatch for SharedTextureMemoryEndAccessState::initialized");
static_assert(offsetof(SharedTextureMemoryEndAccessState, fenceCount) == offsetof(WGPUSharedTextureMemoryEndAccessState, fenceCount),
        "offsetof mismatch for SharedTextureMemoryEndAccessState::fenceCount");
static_assert(offsetof(SharedTextureMemoryEndAccessState, fences) == offsetof(WGPUSharedTextureMemoryEndAccessState, fences),
        "offsetof mismatch for SharedTextureMemoryEndAccessState::fences");
static_assert(offsetof(SharedTextureMemoryEndAccessState, signaledValueCount) == offsetof(WGPUSharedTextureMemoryEndAccessState, signaledValueCount),
        "offsetof mismatch for SharedTextureMemoryEndAccessState::signaledValueCount");
static_assert(offsetof(SharedTextureMemoryEndAccessState, signaledValues) == offsetof(WGPUSharedTextureMemoryEndAccessState, signaledValues),
        "offsetof mismatch for SharedTextureMemoryEndAccessState::signaledValues");

// SharedTextureMemoryProperties implementation

SharedTextureMemoryProperties::operator const WGPUSharedTextureMemoryProperties&() const noexcept {
    return *reinterpret_cast<const WGPUSharedTextureMemoryProperties*>(this);
}

static_assert(sizeof(SharedTextureMemoryProperties) == sizeof(WGPUSharedTextureMemoryProperties), "sizeof mismatch for SharedTextureMemoryProperties");
static_assert(alignof(SharedTextureMemoryProperties) == alignof(WGPUSharedTextureMemoryProperties), "alignof mismatch for SharedTextureMemoryProperties");
static_assert(offsetof(SharedTextureMemoryProperties, nextInChain) == offsetof(WGPUSharedTextureMemoryProperties, nextInChain),
        "offsetof mismatch for SharedTextureMemoryProperties::nextInChain");
static_assert(offsetof(SharedTextureMemoryProperties, usage) == offsetof(WGPUSharedTextureMemoryProperties, usage),
        "offsetof mismatch for SharedTextureMemoryProperties::usage");
static_assert(offsetof(SharedTextureMemoryProperties, size) == offsetof(WGPUSharedTextureMemoryProperties, size),
        "offsetof mismatch for SharedTextureMemoryProperties::size");
static_assert(offsetof(SharedTextureMemoryProperties, format) == offsetof(WGPUSharedTextureMemoryProperties, format),
        "offsetof mismatch for SharedTextureMemoryProperties::format");

// TextureViewDescriptor implementation

TextureViewDescriptor::operator const WGPUTextureViewDescriptor&() const noexcept {
    return *reinterpret_cast<const WGPUTextureViewDescriptor*>(this);
}

static_assert(sizeof(TextureViewDescriptor) == sizeof(WGPUTextureViewDescriptor), "sizeof mismatch for TextureViewDescriptor");
static_assert(alignof(TextureViewDescriptor) == alignof(WGPUTextureViewDescriptor), "alignof mismatch for TextureViewDescriptor");
static_assert(offsetof(TextureViewDescriptor, nextInChain) == offsetof(WGPUTextureViewDescriptor, nextInChain),
        "offsetof mismatch for TextureViewDescriptor::nextInChain");
static_assert(offsetof(TextureViewDescriptor, label) == offsetof(WGPUTextureViewDescriptor, label),
        "offsetof mismatch for TextureViewDescriptor::label");
static_assert(offsetof(TextureViewDescriptor, format) == offsetof(WGPUTextureViewDescriptor, format),
        "offsetof mismatch for TextureViewDescriptor::format");
static_assert(offsetof(TextureViewDescriptor, dimension) == offsetof(WGPUTextureViewDescriptor, dimension),
        "offsetof mismatch for TextureViewDescriptor::dimension");
static_assert(offsetof(TextureViewDescriptor, baseMipLevel) == offsetof(WGPUTextureViewDescriptor, baseMipLevel),
        "offsetof mismatch for TextureViewDescriptor::baseMipLevel");
static_assert(offsetof(TextureViewDescriptor, mipLevelCount) == offsetof(WGPUTextureViewDescriptor, mipLevelCount),
        "offsetof mismatch for TextureViewDescriptor::mipLevelCount");
static_assert(offsetof(TextureViewDescriptor, baseArrayLayer) == offsetof(WGPUTextureViewDescriptor, baseArrayLayer),
        "offsetof mismatch for TextureViewDescriptor::baseArrayLayer");
static_assert(offsetof(TextureViewDescriptor, arrayLayerCount) == offsetof(WGPUTextureViewDescriptor, arrayLayerCount),
        "offsetof mismatch for TextureViewDescriptor::arrayLayerCount");
static_assert(offsetof(TextureViewDescriptor, aspect) == offsetof(WGPUTextureViewDescriptor, aspect),
        "offsetof mismatch for TextureViewDescriptor::aspect");
static_assert(offsetof(TextureViewDescriptor, usage) == offsetof(WGPUTextureViewDescriptor, usage),
        "offsetof mismatch for TextureViewDescriptor::usage");

// VertexState implementation

VertexState::operator const WGPUVertexState&() const noexcept {
    return *reinterpret_cast<const WGPUVertexState*>(this);
}

static_assert(sizeof(VertexState) == sizeof(WGPUVertexState), "sizeof mismatch for VertexState");
static_assert(alignof(VertexState) == alignof(WGPUVertexState), "alignof mismatch for VertexState");
static_assert(offsetof(VertexState, nextInChain) == offsetof(WGPUVertexState, nextInChain),
        "offsetof mismatch for VertexState::nextInChain");
static_assert(offsetof(VertexState, module) == offsetof(WGPUVertexState, module),
        "offsetof mismatch for VertexState::module");
static_assert(offsetof(VertexState, entryPoint) == offsetof(WGPUVertexState, entryPoint),
        "offsetof mismatch for VertexState::entryPoint");
static_assert(offsetof(VertexState, constantCount) == offsetof(WGPUVertexState, constantCount),
        "offsetof mismatch for VertexState::constantCount");
static_assert(offsetof(VertexState, constants) == offsetof(WGPUVertexState, constants),
        "offsetof mismatch for VertexState::constants");
static_assert(offsetof(VertexState, bufferCount) == offsetof(WGPUVertexState, bufferCount),
        "offsetof mismatch for VertexState::bufferCount");
static_assert(offsetof(VertexState, buffers) == offsetof(WGPUVertexState, buffers),
        "offsetof mismatch for VertexState::buffers");

// FragmentState implementation

FragmentState::operator const WGPUFragmentState&() const noexcept {
    return *reinterpret_cast<const WGPUFragmentState*>(this);
}

static_assert(sizeof(FragmentState) == sizeof(WGPUFragmentState), "sizeof mismatch for FragmentState");
static_assert(alignof(FragmentState) == alignof(WGPUFragmentState), "alignof mismatch for FragmentState");
static_assert(offsetof(FragmentState, nextInChain) == offsetof(WGPUFragmentState, nextInChain),
        "offsetof mismatch for FragmentState::nextInChain");
static_assert(offsetof(FragmentState, module) == offsetof(WGPUFragmentState, module),
        "offsetof mismatch for FragmentState::module");
static_assert(offsetof(FragmentState, entryPoint) == offsetof(WGPUFragmentState, entryPoint),
        "offsetof mismatch for FragmentState::entryPoint");
static_assert(offsetof(FragmentState, constantCount) == offsetof(WGPUFragmentState, constantCount),
        "offsetof mismatch for FragmentState::constantCount");
static_assert(offsetof(FragmentState, constants) == offsetof(WGPUFragmentState, constants),
        "offsetof mismatch for FragmentState::constants");
static_assert(offsetof(FragmentState, targetCount) == offsetof(WGPUFragmentState, targetCount),
        "offsetof mismatch for FragmentState::targetCount");
static_assert(offsetof(FragmentState, targets) == offsetof(WGPUFragmentState, targets),
        "offsetof mismatch for FragmentState::targets");

// RenderPassDescriptor implementation

RenderPassDescriptor::operator const WGPURenderPassDescriptor&() const noexcept {
    return *reinterpret_cast<const WGPURenderPassDescriptor*>(this);
}

static_assert(sizeof(RenderPassDescriptor) == sizeof(WGPURenderPassDescriptor), "sizeof mismatch for RenderPassDescriptor");
static_assert(alignof(RenderPassDescriptor) == alignof(WGPURenderPassDescriptor), "alignof mismatch for RenderPassDescriptor");
static_assert(offsetof(RenderPassDescriptor, nextInChain) == offsetof(WGPURenderPassDescriptor, nextInChain),
        "offsetof mismatch for RenderPassDescriptor::nextInChain");
static_assert(offsetof(RenderPassDescriptor, label) == offsetof(WGPURenderPassDescriptor, label),
        "offsetof mismatch for RenderPassDescriptor::label");
static_assert(offsetof(RenderPassDescriptor, colorAttachmentCount) == offsetof(WGPURenderPassDescriptor, colorAttachmentCount),
        "offsetof mismatch for RenderPassDescriptor::colorAttachmentCount");
static_assert(offsetof(RenderPassDescriptor, colorAttachments) == offsetof(WGPURenderPassDescriptor, colorAttachments),
        "offsetof mismatch for RenderPassDescriptor::colorAttachments");
static_assert(offsetof(RenderPassDescriptor, depthStencilAttachment) == offsetof(WGPURenderPassDescriptor, depthStencilAttachment),
        "offsetof mismatch for RenderPassDescriptor::depthStencilAttachment");
static_assert(offsetof(RenderPassDescriptor, occlusionQuerySet) == offsetof(WGPURenderPassDescriptor, occlusionQuerySet),
        "offsetof mismatch for RenderPassDescriptor::occlusionQuerySet");
static_assert(offsetof(RenderPassDescriptor, timestampWrites) == offsetof(WGPURenderPassDescriptor, timestampWrites),
        "offsetof mismatch for RenderPassDescriptor::timestampWrites");

// RenderPipelineDescriptor implementation

RenderPipelineDescriptor::operator const WGPURenderPipelineDescriptor&() const noexcept {
    return *reinterpret_cast<const WGPURenderPipelineDescriptor*>(this);
}

static_assert(sizeof(RenderPipelineDescriptor) == sizeof(WGPURenderPipelineDescriptor), "sizeof mismatch for RenderPipelineDescriptor");
static_assert(alignof(RenderPipelineDescriptor) == alignof(WGPURenderPipelineDescriptor), "alignof mismatch for RenderPipelineDescriptor");
static_assert(offsetof(RenderPipelineDescriptor, nextInChain) == offsetof(WGPURenderPipelineDescriptor, nextInChain),
        "offsetof mismatch for RenderPipelineDescriptor::nextInChain");
static_assert(offsetof(RenderPipelineDescriptor, label) == offsetof(WGPURenderPipelineDescriptor, label),
        "offsetof mismatch for RenderPipelineDescriptor::label");
static_assert(offsetof(RenderPipelineDescriptor, layout) == offsetof(WGPURenderPipelineDescriptor, layout),
        "offsetof mismatch for RenderPipelineDescriptor::layout");
static_assert(offsetof(RenderPipelineDescriptor, vertex) == offsetof(WGPURenderPipelineDescriptor, vertex),
        "offsetof mismatch for RenderPipelineDescriptor::vertex");
static_assert(offsetof(RenderPipelineDescriptor, primitive) == offsetof(WGPURenderPipelineDescriptor, primitive),
        "offsetof mismatch for RenderPipelineDescriptor::primitive");
static_assert(offsetof(RenderPipelineDescriptor, depthStencil) == offsetof(WGPURenderPipelineDescriptor, depthStencil),
        "offsetof mismatch for RenderPipelineDescriptor::depthStencil");
static_assert(offsetof(RenderPipelineDescriptor, multisample) == offsetof(WGPURenderPipelineDescriptor, multisample),
        "offsetof mismatch for RenderPipelineDescriptor::multisample");
static_assert(offsetof(RenderPipelineDescriptor, fragment) == offsetof(WGPURenderPipelineDescriptor, fragment),
        "offsetof mismatch for RenderPipelineDescriptor::fragment");

// NOLINTEND(bugprone-invalid-enum-default-initialization)
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

// Adapter implementation

Device Adapter::CreateDevice(DeviceDescriptor const * descriptor) const {
    auto result = wgpuAdapterCreateDevice(Get(), reinterpret_cast<WGPUDeviceDescriptor const *>(descriptor));
    return Device::Acquire(result);
}
void Adapter::GetFeatures(SupportedFeatures * features) const {
    *features = SupportedFeatures();
    wgpuAdapterGetFeatures(Get(), reinterpret_cast<WGPUSupportedFeatures *>(features));
}
ConvertibleStatus Adapter::GetFormatCapabilities(TextureFormat format, DawnFormatCapabilities * capabilities) const {
    auto result = wgpuAdapterGetFormatCapabilities(Get(), static_cast<WGPUTextureFormat>(format), reinterpret_cast<WGPUDawnFormatCapabilities *>(capabilities));
    return static_cast<Status>(result);
}
ConvertibleStatus Adapter::GetInfo(AdapterInfo * info) const {
    *info = AdapterInfo();
    auto result = wgpuAdapterGetInfo(Get(), reinterpret_cast<WGPUAdapterInfo *>(info));
    return static_cast<Status>(result);
}
Instance Adapter::GetInstance() const {
    auto result = wgpuAdapterGetInstance(Get());
    return Instance::Acquire(result);
}
ConvertibleStatus Adapter::GetLimits(Limits * limits) const {
    auto result = wgpuAdapterGetLimits(Get(), reinterpret_cast<WGPULimits *>(limits));
    return static_cast<Status>(result);
}
Bool Adapter::HasFeature(FeatureName feature) const {
    auto result = wgpuAdapterHasFeature(Get(), static_cast<WGPUFeatureName>(feature));
    return result;
}
template <typename F, typename T>
Future Adapter::RequestDevice(DeviceDescriptor const * descriptor, CallbackMode callbackMode, F callback, T userdata) const {
    auto callbackInfo = detail::CallbackInfoHelper<WGPURequestDeviceCallbackInfo, F>::Create(std::move(callback), userdata);
    callbackInfo.mode = static_cast<WGPUCallbackMode>(callbackMode);
    auto result = wgpuAdapterRequestDevice(Get(), reinterpret_cast<WGPUDeviceDescriptor const *>(descriptor), callbackInfo);
    return Future {
        result.id
    };
}
template <typename F>
Future Adapter::RequestDevice(DeviceDescriptor const * descriptor, CallbackMode callbackMode, F callback) const {
    auto callbackInfo = detail::CallbackInfoHelper<WGPURequestDeviceCallbackInfo, F>::Create(std::move(callback));
    callbackInfo.mode = static_cast<WGPUCallbackMode>(callbackMode);
    auto result = wgpuAdapterRequestDevice(Get(), reinterpret_cast<WGPUDeviceDescriptor const *>(descriptor), callbackInfo);
    return Future {
        result.id
    };
}


void Adapter::WGPUAddRef(WGPUAdapter handle) {
    if (handle != nullptr) {
        wgpuAdapterAddRef(handle);
    }
}
void Adapter::WGPURelease(WGPUAdapter handle) {
    if (handle != nullptr) {
        wgpuAdapterRelease(handle);
    }
}
static_assert(sizeof(Adapter) == sizeof(WGPUAdapter), "sizeof mismatch for Adapter");
static_assert(alignof(Adapter) == alignof(WGPUAdapter), "alignof mismatch for Adapter");

// BindGroup implementation

void BindGroup::SetLabel(StringView label) const {
    wgpuBindGroupSetLabel(Get(), *reinterpret_cast<WGPUStringView const*>(&label));
}


void BindGroup::WGPUAddRef(WGPUBindGroup handle) {
    if (handle != nullptr) {
        wgpuBindGroupAddRef(handle);
    }
}
void BindGroup::WGPURelease(WGPUBindGroup handle) {
    if (handle != nullptr) {
        wgpuBindGroupRelease(handle);
    }
}
static_assert(sizeof(BindGroup) == sizeof(WGPUBindGroup), "sizeof mismatch for BindGroup");
static_assert(alignof(BindGroup) == alignof(WGPUBindGroup), "alignof mismatch for BindGroup");

// BindGroupLayout implementation

void BindGroupLayout::SetLabel(StringView label) const {
    wgpuBindGroupLayoutSetLabel(Get(), *reinterpret_cast<WGPUStringView const*>(&label));
}


void BindGroupLayout::WGPUAddRef(WGPUBindGroupLayout handle) {
    if (handle != nullptr) {
        wgpuBindGroupLayoutAddRef(handle);
    }
}
void BindGroupLayout::WGPURelease(WGPUBindGroupLayout handle) {
    if (handle != nullptr) {
        wgpuBindGroupLayoutRelease(handle);
    }
}
static_assert(sizeof(BindGroupLayout) == sizeof(WGPUBindGroupLayout), "sizeof mismatch for BindGroupLayout");
static_assert(alignof(BindGroupLayout) == alignof(WGPUBindGroupLayout), "alignof mismatch for BindGroupLayout");

// Buffer implementation

TexelBufferView Buffer::CreateTexelView(TexelBufferViewDescriptor const * descriptor) const {
    auto result = wgpuBufferCreateTexelView(Get(), reinterpret_cast<WGPUTexelBufferViewDescriptor const *>(descriptor));
    return TexelBufferView::Acquire(result);
}
void Buffer::Destroy() const {
    wgpuBufferDestroy(Get());
}
void const * Buffer::GetConstMappedRange(size_t offset, size_t size) const {
    auto result = wgpuBufferGetConstMappedRange(Get(), offset, size);
    return result;
}
void * Buffer::GetMappedRange(size_t offset, size_t size) const {
    auto result = wgpuBufferGetMappedRange(Get(), offset, size);
    return result;
}
BufferMapState Buffer::GetMapState() const {
    auto result = wgpuBufferGetMapState(Get());
    return static_cast<BufferMapState>(result);
}
uint64_t Buffer::GetSize() const {
    auto result = wgpuBufferGetSize(Get());
    return result;
}
BufferUsage Buffer::GetUsage() const {
    auto result = wgpuBufferGetUsage(Get());
    return static_cast<BufferUsage>(result);
}
template <typename F, typename T>
Future Buffer::MapAsync(MapMode mode, size_t offset, size_t size, CallbackMode callbackMode, F callback, T userdata) const {
    auto callbackInfo = detail::CallbackInfoHelper<WGPUBufferMapCallbackInfo, F>::Create(std::move(callback), userdata);
    callbackInfo.mode = static_cast<WGPUCallbackMode>(callbackMode);
    auto result = wgpuBufferMapAsync(Get(), static_cast<WGPUMapMode>(mode), offset, size, callbackInfo);
    return Future {
        result.id
    };
}
template <typename F>
Future Buffer::MapAsync(MapMode mode, size_t offset, size_t size, CallbackMode callbackMode, F callback) const {
    auto callbackInfo = detail::CallbackInfoHelper<WGPUBufferMapCallbackInfo, F>::Create(std::move(callback));
    callbackInfo.mode = static_cast<WGPUCallbackMode>(callbackMode);
    auto result = wgpuBufferMapAsync(Get(), static_cast<WGPUMapMode>(mode), offset, size, callbackInfo);
    return Future {
        result.id
    };
}
ConvertibleStatus Buffer::ReadMappedRange(size_t offset, void * data, size_t size) const {
    auto result = wgpuBufferReadMappedRange(Get(), offset, reinterpret_cast<void *>(data), size);
    return static_cast<Status>(result);
}
void Buffer::SetLabel(StringView label) const {
    wgpuBufferSetLabel(Get(), *reinterpret_cast<WGPUStringView const*>(&label));
}
void Buffer::Unmap() const {
    wgpuBufferUnmap(Get());
}
ConvertibleStatus Buffer::WriteMappedRange(size_t offset, void const * data, size_t size) const {
    auto result = wgpuBufferWriteMappedRange(Get(), offset, reinterpret_cast<void const *>(data), size);
    return static_cast<Status>(result);
}


void Buffer::WGPUAddRef(WGPUBuffer handle) {
    if (handle != nullptr) {
        wgpuBufferAddRef(handle);
    }
}
void Buffer::WGPURelease(WGPUBuffer handle) {
    if (handle != nullptr) {
        wgpuBufferRelease(handle);
    }
}
static_assert(sizeof(Buffer) == sizeof(WGPUBuffer), "sizeof mismatch for Buffer");
static_assert(alignof(Buffer) == alignof(WGPUBuffer), "alignof mismatch for Buffer");

// CommandBuffer implementation

void CommandBuffer::SetLabel(StringView label) const {
    wgpuCommandBufferSetLabel(Get(), *reinterpret_cast<WGPUStringView const*>(&label));
}


void CommandBuffer::WGPUAddRef(WGPUCommandBuffer handle) {
    if (handle != nullptr) {
        wgpuCommandBufferAddRef(handle);
    }
}
void CommandBuffer::WGPURelease(WGPUCommandBuffer handle) {
    if (handle != nullptr) {
        wgpuCommandBufferRelease(handle);
    }
}
static_assert(sizeof(CommandBuffer) == sizeof(WGPUCommandBuffer), "sizeof mismatch for CommandBuffer");
static_assert(alignof(CommandBuffer) == alignof(WGPUCommandBuffer), "alignof mismatch for CommandBuffer");

// CommandEncoder implementation

ComputePassEncoder CommandEncoder::BeginComputePass(ComputePassDescriptor const * descriptor) const {
    auto result = wgpuCommandEncoderBeginComputePass(Get(), reinterpret_cast<WGPUComputePassDescriptor const *>(descriptor));
    return ComputePassEncoder::Acquire(result);
}
RenderPassEncoder CommandEncoder::BeginRenderPass(RenderPassDescriptor const * descriptor) const {
    auto result = wgpuCommandEncoderBeginRenderPass(Get(), reinterpret_cast<WGPURenderPassDescriptor const *>(descriptor));
    return RenderPassEncoder::Acquire(result);
}
void CommandEncoder::ClearBuffer(Buffer const& buffer, uint64_t offset, uint64_t size) const {
    wgpuCommandEncoderClearBuffer(Get(), buffer.Get(), offset, size);
}
void CommandEncoder::CopyBufferToBuffer(Buffer const& source, uint64_t sourceOffset, Buffer const& destination, uint64_t destinationOffset, uint64_t size) const {
    wgpuCommandEncoderCopyBufferToBuffer(Get(), source.Get(), sourceOffset, destination.Get(), destinationOffset, size);
}
void CommandEncoder::CopyBufferToTexture(TexelCopyBufferInfo const * source, TexelCopyTextureInfo const * destination, Extent3D const * copySize) const {
    wgpuCommandEncoderCopyBufferToTexture(Get(), reinterpret_cast<WGPUTexelCopyBufferInfo const *>(source), reinterpret_cast<WGPUTexelCopyTextureInfo const *>(destination), reinterpret_cast<WGPUExtent3D const *>(copySize));
}
void CommandEncoder::CopyTextureToBuffer(TexelCopyTextureInfo const * source, TexelCopyBufferInfo const * destination, Extent3D const * copySize) const {
    wgpuCommandEncoderCopyTextureToBuffer(Get(), reinterpret_cast<WGPUTexelCopyTextureInfo const *>(source), reinterpret_cast<WGPUTexelCopyBufferInfo const *>(destination), reinterpret_cast<WGPUExtent3D const *>(copySize));
}
void CommandEncoder::CopyTextureToTexture(TexelCopyTextureInfo const * source, TexelCopyTextureInfo const * destination, Extent3D const * copySize) const {
    wgpuCommandEncoderCopyTextureToTexture(Get(), reinterpret_cast<WGPUTexelCopyTextureInfo const *>(source), reinterpret_cast<WGPUTexelCopyTextureInfo const *>(destination), reinterpret_cast<WGPUExtent3D const *>(copySize));
}
CommandBuffer CommandEncoder::Finish(CommandBufferDescriptor const * descriptor) const {
    auto result = wgpuCommandEncoderFinish(Get(), reinterpret_cast<WGPUCommandBufferDescriptor const *>(descriptor));
    return CommandBuffer::Acquire(result);
}
void CommandEncoder::InjectValidationError(StringView message) const {
    wgpuCommandEncoderInjectValidationError(Get(), *reinterpret_cast<WGPUStringView const*>(&message));
}
void CommandEncoder::InsertDebugMarker(StringView markerLabel) const {
    wgpuCommandEncoderInsertDebugMarker(Get(), *reinterpret_cast<WGPUStringView const*>(&markerLabel));
}
void CommandEncoder::PopDebugGroup() const {
    wgpuCommandEncoderPopDebugGroup(Get());
}
void CommandEncoder::PushDebugGroup(StringView groupLabel) const {
    wgpuCommandEncoderPushDebugGroup(Get(), *reinterpret_cast<WGPUStringView const*>(&groupLabel));
}
void CommandEncoder::ResolveQuerySet(QuerySet const& querySet, uint32_t firstQuery, uint32_t queryCount, Buffer const& destination, uint64_t destinationOffset) const {
    wgpuCommandEncoderResolveQuerySet(Get(), querySet.Get(), firstQuery, queryCount, destination.Get(), destinationOffset);
}
void CommandEncoder::SetLabel(StringView label) const {
    wgpuCommandEncoderSetLabel(Get(), *reinterpret_cast<WGPUStringView const*>(&label));
}
void CommandEncoder::WriteBuffer(Buffer const& buffer, uint64_t bufferOffset, uint8_t const * data, uint64_t size) const {
    wgpuCommandEncoderWriteBuffer(Get(), buffer.Get(), bufferOffset, reinterpret_cast<uint8_t const *>(data), size);
}
void CommandEncoder::WriteTimestamp(QuerySet const& querySet, uint32_t queryIndex) const {
    wgpuCommandEncoderWriteTimestamp(Get(), querySet.Get(), queryIndex);
}


void CommandEncoder::WGPUAddRef(WGPUCommandEncoder handle) {
    if (handle != nullptr) {
        wgpuCommandEncoderAddRef(handle);
    }
}
void CommandEncoder::WGPURelease(WGPUCommandEncoder handle) {
    if (handle != nullptr) {
        wgpuCommandEncoderRelease(handle);
    }
}
static_assert(sizeof(CommandEncoder) == sizeof(WGPUCommandEncoder), "sizeof mismatch for CommandEncoder");
static_assert(alignof(CommandEncoder) == alignof(WGPUCommandEncoder), "alignof mismatch for CommandEncoder");

// ComputePipeline implementation

BindGroupLayout ComputePipeline::GetBindGroupLayout(uint32_t groupIndex) const {
    auto result = wgpuComputePipelineGetBindGroupLayout(Get(), groupIndex);
    return BindGroupLayout::Acquire(result);
}
void ComputePipeline::SetLabel(StringView label) const {
    wgpuComputePipelineSetLabel(Get(), *reinterpret_cast<WGPUStringView const*>(&label));
}


void ComputePipeline::WGPUAddRef(WGPUComputePipeline handle) {
    if (handle != nullptr) {
        wgpuComputePipelineAddRef(handle);
    }
}
void ComputePipeline::WGPURelease(WGPUComputePipeline handle) {
    if (handle != nullptr) {
        wgpuComputePipelineRelease(handle);
    }
}
static_assert(sizeof(ComputePipeline) == sizeof(WGPUComputePipeline), "sizeof mismatch for ComputePipeline");
static_assert(alignof(ComputePipeline) == alignof(WGPUComputePipeline), "alignof mismatch for ComputePipeline");

// Device implementation

BindGroup Device::CreateBindGroup(BindGroupDescriptor const * descriptor) const {
    auto result = wgpuDeviceCreateBindGroup(Get(), reinterpret_cast<WGPUBindGroupDescriptor const *>(descriptor));
    return BindGroup::Acquire(result);
}
BindGroupLayout Device::CreateBindGroupLayout(BindGroupLayoutDescriptor const * descriptor) const {
    auto result = wgpuDeviceCreateBindGroupLayout(Get(), reinterpret_cast<WGPUBindGroupLayoutDescriptor const *>(descriptor));
    return BindGroupLayout::Acquire(result);
}
Buffer Device::CreateBuffer(BufferDescriptor const * descriptor) const {
    auto result = wgpuDeviceCreateBuffer(Get(), reinterpret_cast<WGPUBufferDescriptor const *>(descriptor));
    return Buffer::Acquire(result);
}
CommandEncoder Device::CreateCommandEncoder(CommandEncoderDescriptor const * descriptor) const {
    auto result = wgpuDeviceCreateCommandEncoder(Get(), reinterpret_cast<WGPUCommandEncoderDescriptor const *>(descriptor));
    return CommandEncoder::Acquire(result);
}
ComputePipeline Device::CreateComputePipeline(ComputePipelineDescriptor const * descriptor) const {
    auto result = wgpuDeviceCreateComputePipeline(Get(), reinterpret_cast<WGPUComputePipelineDescriptor const *>(descriptor));
    return ComputePipeline::Acquire(result);
}
template <typename F, typename T>
Future Device::CreateComputePipelineAsync(ComputePipelineDescriptor const * descriptor, CallbackMode callbackMode, F callback, T userdata) const {
    auto callbackInfo = detail::CallbackInfoHelper<WGPUCreateComputePipelineAsyncCallbackInfo, F>::Create(std::move(callback), userdata);
    callbackInfo.mode = static_cast<WGPUCallbackMode>(callbackMode);
    auto result = wgpuDeviceCreateComputePipelineAsync(Get(), reinterpret_cast<WGPUComputePipelineDescriptor const *>(descriptor), callbackInfo);
    return Future {
        result.id
    };
}
template <typename F>
Future Device::CreateComputePipelineAsync(ComputePipelineDescriptor const * descriptor, CallbackMode callbackMode, F callback) const {
    auto callbackInfo = detail::CallbackInfoHelper<WGPUCreateComputePipelineAsyncCallbackInfo, F>::Create(std::move(callback));
    callbackInfo.mode = static_cast<WGPUCallbackMode>(callbackMode);
    auto result = wgpuDeviceCreateComputePipelineAsync(Get(), reinterpret_cast<WGPUComputePipelineDescriptor const *>(descriptor), callbackInfo);
    return Future {
        result.id
    };
}
Buffer Device::CreateErrorBuffer(BufferDescriptor const * descriptor) const {
    auto result = wgpuDeviceCreateErrorBuffer(Get(), reinterpret_cast<WGPUBufferDescriptor const *>(descriptor));
    return Buffer::Acquire(result);
}
ComputePipeline Device::CreateErrorComputePipeline(StringView label) const {
    auto result = wgpuDeviceCreateErrorComputePipeline(Get(), *reinterpret_cast<WGPUStringView const*>(&label));
    return ComputePipeline::Acquire(result);
}
ExternalTexture Device::CreateErrorExternalTexture() const {
    auto result = wgpuDeviceCreateErrorExternalTexture(Get());
    return ExternalTexture::Acquire(result);
}
RenderPipeline Device::CreateErrorRenderPipeline(StringView label) const {
    auto result = wgpuDeviceCreateErrorRenderPipeline(Get(), *reinterpret_cast<WGPUStringView const*>(&label));
    return RenderPipeline::Acquire(result);
}
ShaderModule Device::CreateErrorShaderModule(ShaderModuleDescriptor const * descriptor, StringView errorMessage) const {
    auto result = wgpuDeviceCreateErrorShaderModule(Get(), reinterpret_cast<WGPUShaderModuleDescriptor const *>(descriptor), *reinterpret_cast<WGPUStringView const*>(&errorMessage));
    return ShaderModule::Acquire(result);
}
Texture Device::CreateErrorTexture(TextureDescriptor const * descriptor) const {
    auto result = wgpuDeviceCreateErrorTexture(Get(), reinterpret_cast<WGPUTextureDescriptor const *>(descriptor));
    return Texture::Acquire(result);
}
ExternalTexture Device::CreateExternalTexture(ExternalTextureDescriptor const * externalTextureDescriptor) const {
    auto result = wgpuDeviceCreateExternalTexture(Get(), reinterpret_cast<WGPUExternalTextureDescriptor const *>(externalTextureDescriptor));
    return ExternalTexture::Acquire(result);
}
PipelineLayout Device::CreatePipelineLayout(PipelineLayoutDescriptor const * descriptor) const {
    auto result = wgpuDeviceCreatePipelineLayout(Get(), reinterpret_cast<WGPUPipelineLayoutDescriptor const *>(descriptor));
    return PipelineLayout::Acquire(result);
}
QuerySet Device::CreateQuerySet(QuerySetDescriptor const * descriptor) const {
    auto result = wgpuDeviceCreateQuerySet(Get(), reinterpret_cast<WGPUQuerySetDescriptor const *>(descriptor));
    return QuerySet::Acquire(result);
}
RenderBundleEncoder Device::CreateRenderBundleEncoder(RenderBundleEncoderDescriptor const * descriptor) const {
    auto result = wgpuDeviceCreateRenderBundleEncoder(Get(), reinterpret_cast<WGPURenderBundleEncoderDescriptor const *>(descriptor));
    return RenderBundleEncoder::Acquire(result);
}
RenderPipeline Device::CreateRenderPipeline(RenderPipelineDescriptor const * descriptor) const {
    auto result = wgpuDeviceCreateRenderPipeline(Get(), reinterpret_cast<WGPURenderPipelineDescriptor const *>(descriptor));
    return RenderPipeline::Acquire(result);
}
template <typename F, typename T>
Future Device::CreateRenderPipelineAsync(RenderPipelineDescriptor const * descriptor, CallbackMode callbackMode, F callback, T userdata) const {
    auto callbackInfo = detail::CallbackInfoHelper<WGPUCreateRenderPipelineAsyncCallbackInfo, F>::Create(std::move(callback), userdata);
    callbackInfo.mode = static_cast<WGPUCallbackMode>(callbackMode);
    auto result = wgpuDeviceCreateRenderPipelineAsync(Get(), reinterpret_cast<WGPURenderPipelineDescriptor const *>(descriptor), callbackInfo);
    return Future {
        result.id
    };
}
template <typename F>
Future Device::CreateRenderPipelineAsync(RenderPipelineDescriptor const * descriptor, CallbackMode callbackMode, F callback) const {
    auto callbackInfo = detail::CallbackInfoHelper<WGPUCreateRenderPipelineAsyncCallbackInfo, F>::Create(std::move(callback));
    callbackInfo.mode = static_cast<WGPUCallbackMode>(callbackMode);
    auto result = wgpuDeviceCreateRenderPipelineAsync(Get(), reinterpret_cast<WGPURenderPipelineDescriptor const *>(descriptor), callbackInfo);
    return Future {
        result.id
    };
}
ResourceTable Device::CreateResourceTable(ResourceTableDescriptor const * descriptor) const {
    auto result = wgpuDeviceCreateResourceTable(Get(), reinterpret_cast<WGPUResourceTableDescriptor const *>(descriptor));
    return ResourceTable::Acquire(result);
}
Sampler Device::CreateSampler(SamplerDescriptor const * descriptor) const {
    auto result = wgpuDeviceCreateSampler(Get(), reinterpret_cast<WGPUSamplerDescriptor const *>(descriptor));
    return Sampler::Acquire(result);
}
ShaderModule Device::CreateShaderModule(ShaderModuleDescriptor const * descriptor) const {
    auto result = wgpuDeviceCreateShaderModule(Get(), reinterpret_cast<WGPUShaderModuleDescriptor const *>(descriptor));
    return ShaderModule::Acquire(result);
}
Texture Device::CreateTexture(TextureDescriptor const * descriptor) const {
    auto result = wgpuDeviceCreateTexture(Get(), reinterpret_cast<WGPUTextureDescriptor const *>(descriptor));
    return Texture::Acquire(result);
}
void Device::Destroy() const {
    wgpuDeviceDestroy(Get());
}
void Device::ForceLoss(DeviceLostReason type, StringView message) const {
    wgpuDeviceForceLoss(Get(), static_cast<WGPUDeviceLostReason>(type), *reinterpret_cast<WGPUStringView const*>(&message));
}
Adapter Device::GetAdapter() const {
    auto result = wgpuDeviceGetAdapter(Get());
    return Adapter::Acquire(result);
}
ConvertibleStatus Device::GetAdapterInfo(AdapterInfo * adapterInfo) const {
    *adapterInfo = AdapterInfo();
    auto result = wgpuDeviceGetAdapterInfo(Get(), reinterpret_cast<WGPUAdapterInfo *>(adapterInfo));
    return static_cast<Status>(result);
}
ConvertibleStatus Device::GetAHardwareBufferProperties(void * handle, AHardwareBufferProperties * properties) const {
    auto result = wgpuDeviceGetAHardwareBufferProperties(Get(), handle, reinterpret_cast<WGPUAHardwareBufferProperties *>(properties));
    return static_cast<Status>(result);
}
void Device::GetFeatures(SupportedFeatures * features) const {
    *features = SupportedFeatures();
    wgpuDeviceGetFeatures(Get(), reinterpret_cast<WGPUSupportedFeatures *>(features));
}
ConvertibleStatus Device::GetLimits(Limits * limits) const {
    auto result = wgpuDeviceGetLimits(Get(), reinterpret_cast<WGPULimits *>(limits));
    return static_cast<Status>(result);
}
Future Device::GetLostFuture() const {
    auto result = wgpuDeviceGetLostFuture(Get());
    return Future {
            result.id
        };
}
Queue Device::GetQueue() const {
    auto result = wgpuDeviceGetQueue(Get());
    return Queue::Acquire(result);
}
Bool Device::HasFeature(FeatureName feature) const {
    auto result = wgpuDeviceHasFeature(Get(), static_cast<WGPUFeatureName>(feature));
    return result;
}
SharedBufferMemory Device::ImportSharedBufferMemory(SharedBufferMemoryDescriptor const * descriptor) const {
    auto result = wgpuDeviceImportSharedBufferMemory(Get(), reinterpret_cast<WGPUSharedBufferMemoryDescriptor const *>(descriptor));
    return SharedBufferMemory::Acquire(result);
}
SharedFence Device::ImportSharedFence(SharedFenceDescriptor const * descriptor) const {
    auto result = wgpuDeviceImportSharedFence(Get(), reinterpret_cast<WGPUSharedFenceDescriptor const *>(descriptor));
    return SharedFence::Acquire(result);
}
SharedTextureMemory Device::ImportSharedTextureMemory(SharedTextureMemoryDescriptor const * descriptor) const {
    auto result = wgpuDeviceImportSharedTextureMemory(Get(), reinterpret_cast<WGPUSharedTextureMemoryDescriptor const *>(descriptor));
    return SharedTextureMemory::Acquire(result);
}
void Device::InjectError(ErrorType type, StringView message) const {
    wgpuDeviceInjectError(Get(), static_cast<WGPUErrorType>(type), *reinterpret_cast<WGPUStringView const*>(&message));
}
template <typename F, typename T>
Future Device::PopErrorScope(CallbackMode callbackMode, F callback, T userdata) const {
    auto callbackInfo = detail::CallbackInfoHelper<WGPUPopErrorScopeCallbackInfo, F>::Create(std::move(callback), userdata);
    callbackInfo.mode = static_cast<WGPUCallbackMode>(callbackMode);
    auto result = wgpuDevicePopErrorScope(Get(), callbackInfo);
    return Future {
        result.id
    };
}
template <typename F>
Future Device::PopErrorScope(CallbackMode callbackMode, F callback) const {
    auto callbackInfo = detail::CallbackInfoHelper<WGPUPopErrorScopeCallbackInfo, F>::Create(std::move(callback));
    callbackInfo.mode = static_cast<WGPUCallbackMode>(callbackMode);
    auto result = wgpuDevicePopErrorScope(Get(), callbackInfo);
    return Future {
        result.id
    };
}
void Device::PushErrorScope(ErrorFilter filter) const {
    wgpuDevicePushErrorScope(Get(), static_cast<WGPUErrorFilter>(filter));
}
void Device::SetLabel(StringView label) const {
    wgpuDeviceSetLabel(Get(), *reinterpret_cast<WGPUStringView const*>(&label));
}
template <typename F, typename T>
void Device::SetLoggingCallback(F callback, T userdata) const {
    auto callbackInfo = detail::CallbackInfoHelper<WGPULoggingCallbackInfo, F>::Create(std::move(callback), userdata);
    return wgpuDeviceSetLoggingCallback(Get(), callbackInfo);
}
template <typename F>
void Device::SetLoggingCallback(F callback) const {
    auto callbackInfo = detail::CallbackInfoHelper<WGPULoggingCallbackInfo, F>::Create(std::move(callback));
    return wgpuDeviceSetLoggingCallback(Get(), callbackInfo);
}
void Device::Tick() const {
    wgpuDeviceTick(Get());
}
void Device::ValidateTextureDescriptor(TextureDescriptor const * descriptor) const {
    wgpuDeviceValidateTextureDescriptor(Get(), reinterpret_cast<WGPUTextureDescriptor const *>(descriptor));
}


void Device::WGPUAddRef(WGPUDevice handle) {
    if (handle != nullptr) {
        wgpuDeviceAddRef(handle);
    }
}
void Device::WGPURelease(WGPUDevice handle) {
    if (handle != nullptr) {
        wgpuDeviceRelease(handle);
    }
}
static_assert(sizeof(Device) == sizeof(WGPUDevice), "sizeof mismatch for Device");
static_assert(alignof(Device) == alignof(WGPUDevice), "alignof mismatch for Device");

// ExternalTexture implementation

void ExternalTexture::Destroy() const {
    wgpuExternalTextureDestroy(Get());
}
void ExternalTexture::Expire() const {
    wgpuExternalTextureExpire(Get());
}
void ExternalTexture::Refresh() const {
    wgpuExternalTextureRefresh(Get());
}
void ExternalTexture::SetLabel(StringView label) const {
    wgpuExternalTextureSetLabel(Get(), *reinterpret_cast<WGPUStringView const*>(&label));
}


void ExternalTexture::WGPUAddRef(WGPUExternalTexture handle) {
    if (handle != nullptr) {
        wgpuExternalTextureAddRef(handle);
    }
}
void ExternalTexture::WGPURelease(WGPUExternalTexture handle) {
    if (handle != nullptr) {
        wgpuExternalTextureRelease(handle);
    }
}
static_assert(sizeof(ExternalTexture) == sizeof(WGPUExternalTexture), "sizeof mismatch for ExternalTexture");
static_assert(alignof(ExternalTexture) == alignof(WGPUExternalTexture), "alignof mismatch for ExternalTexture");

// Instance implementation

Surface Instance::CreateSurface(SurfaceDescriptor const * descriptor) const {
    auto result = wgpuInstanceCreateSurface(Get(), reinterpret_cast<WGPUSurfaceDescriptor const *>(descriptor));
    return Surface::Acquire(result);
}
void Instance::GetWGSLLanguageFeatures(SupportedWGSLLanguageFeatures * features) const {
    *features = SupportedWGSLLanguageFeatures();
    wgpuInstanceGetWGSLLanguageFeatures(Get(), reinterpret_cast<WGPUSupportedWGSLLanguageFeatures *>(features));
}
Bool Instance::HasWGSLLanguageFeature(WGSLLanguageFeatureName feature) const {
    auto result = wgpuInstanceHasWGSLLanguageFeature(Get(), static_cast<WGPUWGSLLanguageFeatureName>(feature));
    return result;
}
void Instance::ProcessEvents() const {
    wgpuInstanceProcessEvents(Get());
}
template <typename F, typename T>
Future Instance::RequestAdapter(RequestAdapterOptions const * options, CallbackMode callbackMode, F callback, T userdata) const {
    auto callbackInfo = detail::CallbackInfoHelper<WGPURequestAdapterCallbackInfo, F>::Create(std::move(callback), userdata);
    callbackInfo.mode = static_cast<WGPUCallbackMode>(callbackMode);
    auto result = wgpuInstanceRequestAdapter(Get(), reinterpret_cast<WGPURequestAdapterOptions const *>(options), callbackInfo);
    return Future {
        result.id
    };
}
template <typename F>
Future Instance::RequestAdapter(RequestAdapterOptions const * options, CallbackMode callbackMode, F callback) const {
    auto callbackInfo = detail::CallbackInfoHelper<WGPURequestAdapterCallbackInfo, F>::Create(std::move(callback));
    callbackInfo.mode = static_cast<WGPUCallbackMode>(callbackMode);
    auto result = wgpuInstanceRequestAdapter(Get(), reinterpret_cast<WGPURequestAdapterOptions const *>(options), callbackInfo);
    return Future {
        result.id
    };
}
WaitStatus Instance::WaitAny(size_t futureCount, FutureWaitInfo * futures, uint64_t timeoutNS) const {
    auto result = wgpuInstanceWaitAny(Get(), futureCount, reinterpret_cast<WGPUFutureWaitInfo *>(futures), timeoutNS);
    return static_cast<WaitStatus>(result);
}

WaitStatus Instance::WaitAny(Future f, uint64_t timeout) const {
    FutureWaitInfo waitInfo { f };
    return WaitAny(1, &waitInfo, timeout);
}

void Instance::WGPUAddRef(WGPUInstance handle) {
    if (handle != nullptr) {
        wgpuInstanceAddRef(handle);
    }
}
void Instance::WGPURelease(WGPUInstance handle) {
    if (handle != nullptr) {
        wgpuInstanceRelease(handle);
    }
}
static_assert(sizeof(Instance) == sizeof(WGPUInstance), "sizeof mismatch for Instance");
static_assert(alignof(Instance) == alignof(WGPUInstance), "alignof mismatch for Instance");

// PipelineLayout implementation

void PipelineLayout::SetLabel(StringView label) const {
    wgpuPipelineLayoutSetLabel(Get(), *reinterpret_cast<WGPUStringView const*>(&label));
}


void PipelineLayout::WGPUAddRef(WGPUPipelineLayout handle) {
    if (handle != nullptr) {
        wgpuPipelineLayoutAddRef(handle);
    }
}
void PipelineLayout::WGPURelease(WGPUPipelineLayout handle) {
    if (handle != nullptr) {
        wgpuPipelineLayoutRelease(handle);
    }
}
static_assert(sizeof(PipelineLayout) == sizeof(WGPUPipelineLayout), "sizeof mismatch for PipelineLayout");
static_assert(alignof(PipelineLayout) == alignof(WGPUPipelineLayout), "alignof mismatch for PipelineLayout");

// QuerySet implementation

void QuerySet::Destroy() const {
    wgpuQuerySetDestroy(Get());
}
uint32_t QuerySet::GetCount() const {
    auto result = wgpuQuerySetGetCount(Get());
    return result;
}
QueryType QuerySet::GetType() const {
    auto result = wgpuQuerySetGetType(Get());
    return static_cast<QueryType>(result);
}
void QuerySet::SetLabel(StringView label) const {
    wgpuQuerySetSetLabel(Get(), *reinterpret_cast<WGPUStringView const*>(&label));
}


void QuerySet::WGPUAddRef(WGPUQuerySet handle) {
    if (handle != nullptr) {
        wgpuQuerySetAddRef(handle);
    }
}
void QuerySet::WGPURelease(WGPUQuerySet handle) {
    if (handle != nullptr) {
        wgpuQuerySetRelease(handle);
    }
}
static_assert(sizeof(QuerySet) == sizeof(WGPUQuerySet), "sizeof mismatch for QuerySet");
static_assert(alignof(QuerySet) == alignof(WGPUQuerySet), "alignof mismatch for QuerySet");

// Queue implementation

void Queue::CopyExternalTextureForBrowser(ImageCopyExternalTexture const * source, TexelCopyTextureInfo const * destination, Extent3D const * copySize, CopyTextureForBrowserOptions const * options) const {
    wgpuQueueCopyExternalTextureForBrowser(Get(), reinterpret_cast<WGPUImageCopyExternalTexture const *>(source), reinterpret_cast<WGPUTexelCopyTextureInfo const *>(destination), reinterpret_cast<WGPUExtent3D const *>(copySize), reinterpret_cast<WGPUCopyTextureForBrowserOptions const *>(options));
}
void Queue::CopyTextureForBrowser(TexelCopyTextureInfo const * source, TexelCopyTextureInfo const * destination, Extent3D const * copySize, CopyTextureForBrowserOptions const * options) const {
    wgpuQueueCopyTextureForBrowser(Get(), reinterpret_cast<WGPUTexelCopyTextureInfo const *>(source), reinterpret_cast<WGPUTexelCopyTextureInfo const *>(destination), reinterpret_cast<WGPUExtent3D const *>(copySize), reinterpret_cast<WGPUCopyTextureForBrowserOptions const *>(options));
}
template <typename F, typename T>
Future Queue::OnSubmittedWorkDone(CallbackMode callbackMode, F callback, T userdata) const {
    auto callbackInfo = detail::CallbackInfoHelper<WGPUQueueWorkDoneCallbackInfo, F>::Create(std::move(callback), userdata);
    callbackInfo.mode = static_cast<WGPUCallbackMode>(callbackMode);
    auto result = wgpuQueueOnSubmittedWorkDone(Get(), callbackInfo);
    return Future {
        result.id
    };
}
template <typename F>
Future Queue::OnSubmittedWorkDone(CallbackMode callbackMode, F callback) const {
    auto callbackInfo = detail::CallbackInfoHelper<WGPUQueueWorkDoneCallbackInfo, F>::Create(std::move(callback));
    callbackInfo.mode = static_cast<WGPUCallbackMode>(callbackMode);
    auto result = wgpuQueueOnSubmittedWorkDone(Get(), callbackInfo);
    return Future {
        result.id
    };
}
void Queue::SetLabel(StringView label) const {
    wgpuQueueSetLabel(Get(), *reinterpret_cast<WGPUStringView const*>(&label));
}
void Queue::Submit(size_t commandCount, CommandBuffer const * commands) const {
    wgpuQueueSubmit(Get(), commandCount, reinterpret_cast<WGPUCommandBuffer const *>(commands));
}
void Queue::WriteBuffer(Buffer const& buffer, uint64_t bufferOffset, void const * data, size_t size) const {
    wgpuQueueWriteBuffer(Get(), buffer.Get(), bufferOffset, reinterpret_cast<void const *>(data), size);
}
void Queue::WriteTexture(TexelCopyTextureInfo const * destination, void const * data, size_t dataSize, TexelCopyBufferLayout const * dataLayout, Extent3D const * writeSize) const {
    wgpuQueueWriteTexture(Get(), reinterpret_cast<WGPUTexelCopyTextureInfo const *>(destination), reinterpret_cast<void const *>(data), dataSize, reinterpret_cast<WGPUTexelCopyBufferLayout const *>(dataLayout), reinterpret_cast<WGPUExtent3D const *>(writeSize));
}


void Queue::WGPUAddRef(WGPUQueue handle) {
    if (handle != nullptr) {
        wgpuQueueAddRef(handle);
    }
}
void Queue::WGPURelease(WGPUQueue handle) {
    if (handle != nullptr) {
        wgpuQueueRelease(handle);
    }
}
static_assert(sizeof(Queue) == sizeof(WGPUQueue), "sizeof mismatch for Queue");
static_assert(alignof(Queue) == alignof(WGPUQueue), "alignof mismatch for Queue");

// RenderBundle implementation

void RenderBundle::SetLabel(StringView label) const {
    wgpuRenderBundleSetLabel(Get(), *reinterpret_cast<WGPUStringView const*>(&label));
}


void RenderBundle::WGPUAddRef(WGPURenderBundle handle) {
    if (handle != nullptr) {
        wgpuRenderBundleAddRef(handle);
    }
}
void RenderBundle::WGPURelease(WGPURenderBundle handle) {
    if (handle != nullptr) {
        wgpuRenderBundleRelease(handle);
    }
}
static_assert(sizeof(RenderBundle) == sizeof(WGPURenderBundle), "sizeof mismatch for RenderBundle");
static_assert(alignof(RenderBundle) == alignof(WGPURenderBundle), "alignof mismatch for RenderBundle");

// RenderPipeline implementation

BindGroupLayout RenderPipeline::GetBindGroupLayout(uint32_t groupIndex) const {
    auto result = wgpuRenderPipelineGetBindGroupLayout(Get(), groupIndex);
    return BindGroupLayout::Acquire(result);
}
void RenderPipeline::SetLabel(StringView label) const {
    wgpuRenderPipelineSetLabel(Get(), *reinterpret_cast<WGPUStringView const*>(&label));
}


void RenderPipeline::WGPUAddRef(WGPURenderPipeline handle) {
    if (handle != nullptr) {
        wgpuRenderPipelineAddRef(handle);
    }
}
void RenderPipeline::WGPURelease(WGPURenderPipeline handle) {
    if (handle != nullptr) {
        wgpuRenderPipelineRelease(handle);
    }
}
static_assert(sizeof(RenderPipeline) == sizeof(WGPURenderPipeline), "sizeof mismatch for RenderPipeline");
static_assert(alignof(RenderPipeline) == alignof(WGPURenderPipeline), "alignof mismatch for RenderPipeline");

// ResourceTable implementation

void ResourceTable::Destroy() const {
    wgpuResourceTableDestroy(Get());
}
uint32_t ResourceTable::GetSize() const {
    auto result = wgpuResourceTableGetSize(Get());
    return result;
}
uint32_t ResourceTable::InsertBinding(BindingResource const * resource) const {
    auto result = wgpuResourceTableInsertBinding(Get(), reinterpret_cast<WGPUBindingResource const *>(resource));
    return result;
}
ConvertibleStatus ResourceTable::RemoveBinding(uint32_t slot) const {
    auto result = wgpuResourceTableRemoveBinding(Get(), slot);
    return static_cast<Status>(result);
}
void ResourceTable::SetLabel(StringView label) const {
    wgpuResourceTableSetLabel(Get(), *reinterpret_cast<WGPUStringView const*>(&label));
}
ConvertibleStatus ResourceTable::Update(uint32_t slot, BindingResource const * resource) const {
    auto result = wgpuResourceTableUpdate(Get(), slot, reinterpret_cast<WGPUBindingResource const *>(resource));
    return static_cast<Status>(result);
}


void ResourceTable::WGPUAddRef(WGPUResourceTable handle) {
    if (handle != nullptr) {
        wgpuResourceTableAddRef(handle);
    }
}
void ResourceTable::WGPURelease(WGPUResourceTable handle) {
    if (handle != nullptr) {
        wgpuResourceTableRelease(handle);
    }
}
static_assert(sizeof(ResourceTable) == sizeof(WGPUResourceTable), "sizeof mismatch for ResourceTable");
static_assert(alignof(ResourceTable) == alignof(WGPUResourceTable), "alignof mismatch for ResourceTable");

// Sampler implementation

void Sampler::SetLabel(StringView label) const {
    wgpuSamplerSetLabel(Get(), *reinterpret_cast<WGPUStringView const*>(&label));
}


void Sampler::WGPUAddRef(WGPUSampler handle) {
    if (handle != nullptr) {
        wgpuSamplerAddRef(handle);
    }
}
void Sampler::WGPURelease(WGPUSampler handle) {
    if (handle != nullptr) {
        wgpuSamplerRelease(handle);
    }
}
static_assert(sizeof(Sampler) == sizeof(WGPUSampler), "sizeof mismatch for Sampler");
static_assert(alignof(Sampler) == alignof(WGPUSampler), "alignof mismatch for Sampler");

// ShaderModule implementation

template <typename F, typename T>
Future ShaderModule::GetCompilationInfo(CallbackMode callbackMode, F callback, T userdata) const {
    auto callbackInfo = detail::CallbackInfoHelper<WGPUCompilationInfoCallbackInfo, F>::Create(std::move(callback), userdata);
    callbackInfo.mode = static_cast<WGPUCallbackMode>(callbackMode);
    auto result = wgpuShaderModuleGetCompilationInfo(Get(), callbackInfo);
    return Future {
        result.id
    };
}
template <typename F>
Future ShaderModule::GetCompilationInfo(CallbackMode callbackMode, F callback) const {
    auto callbackInfo = detail::CallbackInfoHelper<WGPUCompilationInfoCallbackInfo, F>::Create(std::move(callback));
    callbackInfo.mode = static_cast<WGPUCallbackMode>(callbackMode);
    auto result = wgpuShaderModuleGetCompilationInfo(Get(), callbackInfo);
    return Future {
        result.id
    };
}
void ShaderModule::SetLabel(StringView label) const {
    wgpuShaderModuleSetLabel(Get(), *reinterpret_cast<WGPUStringView const*>(&label));
}


void ShaderModule::WGPUAddRef(WGPUShaderModule handle) {
    if (handle != nullptr) {
        wgpuShaderModuleAddRef(handle);
    }
}
void ShaderModule::WGPURelease(WGPUShaderModule handle) {
    if (handle != nullptr) {
        wgpuShaderModuleRelease(handle);
    }
}
static_assert(sizeof(ShaderModule) == sizeof(WGPUShaderModule), "sizeof mismatch for ShaderModule");
static_assert(alignof(ShaderModule) == alignof(WGPUShaderModule), "alignof mismatch for ShaderModule");

// SharedBufferMemory implementation

ConvertibleStatus SharedBufferMemory::BeginAccess(Buffer const& buffer, SharedBufferMemoryBeginAccessDescriptor const * descriptor) const {
    auto result = wgpuSharedBufferMemoryBeginAccess(Get(), buffer.Get(), reinterpret_cast<WGPUSharedBufferMemoryBeginAccessDescriptor const *>(descriptor));
    return static_cast<Status>(result);
}
Buffer SharedBufferMemory::CreateBuffer(BufferDescriptor const * descriptor) const {
    auto result = wgpuSharedBufferMemoryCreateBuffer(Get(), reinterpret_cast<WGPUBufferDescriptor const *>(descriptor));
    return Buffer::Acquire(result);
}
ConvertibleStatus SharedBufferMemory::EndAccess(Buffer const& buffer, SharedBufferMemoryEndAccessState * descriptor) const {
    *descriptor = SharedBufferMemoryEndAccessState();
    auto result = wgpuSharedBufferMemoryEndAccess(Get(), buffer.Get(), reinterpret_cast<WGPUSharedBufferMemoryEndAccessState *>(descriptor));
    return static_cast<Status>(result);
}
ConvertibleStatus SharedBufferMemory::GetProperties(SharedBufferMemoryProperties * properties) const {
    auto result = wgpuSharedBufferMemoryGetProperties(Get(), reinterpret_cast<WGPUSharedBufferMemoryProperties *>(properties));
    return static_cast<Status>(result);
}
Bool SharedBufferMemory::IsDeviceLost() const {
    auto result = wgpuSharedBufferMemoryIsDeviceLost(Get());
    return result;
}
void SharedBufferMemory::SetLabel(StringView label) const {
    wgpuSharedBufferMemorySetLabel(Get(), *reinterpret_cast<WGPUStringView const*>(&label));
}


void SharedBufferMemory::WGPUAddRef(WGPUSharedBufferMemory handle) {
    if (handle != nullptr) {
        wgpuSharedBufferMemoryAddRef(handle);
    }
}
void SharedBufferMemory::WGPURelease(WGPUSharedBufferMemory handle) {
    if (handle != nullptr) {
        wgpuSharedBufferMemoryRelease(handle);
    }
}
static_assert(sizeof(SharedBufferMemory) == sizeof(WGPUSharedBufferMemory), "sizeof mismatch for SharedBufferMemory");
static_assert(alignof(SharedBufferMemory) == alignof(WGPUSharedBufferMemory), "alignof mismatch for SharedBufferMemory");

// SharedFence implementation

void SharedFence::ExportInfo(SharedFenceExportInfo * info) const {
    wgpuSharedFenceExportInfo(Get(), reinterpret_cast<WGPUSharedFenceExportInfo *>(info));
}
void SharedFence::SetLabel(StringView label) const {
    wgpuSharedFenceSetLabel(Get(), *reinterpret_cast<WGPUStringView const*>(&label));
}


void SharedFence::WGPUAddRef(WGPUSharedFence handle) {
    if (handle != nullptr) {
        wgpuSharedFenceAddRef(handle);
    }
}
void SharedFence::WGPURelease(WGPUSharedFence handle) {
    if (handle != nullptr) {
        wgpuSharedFenceRelease(handle);
    }
}
static_assert(sizeof(SharedFence) == sizeof(WGPUSharedFence), "sizeof mismatch for SharedFence");
static_assert(alignof(SharedFence) == alignof(WGPUSharedFence), "alignof mismatch for SharedFence");

// SharedTextureMemory implementation

ConvertibleStatus SharedTextureMemory::BeginAccess(Texture const& texture, SharedTextureMemoryBeginAccessDescriptor const * descriptor) const {
    auto result = wgpuSharedTextureMemoryBeginAccess(Get(), texture.Get(), reinterpret_cast<WGPUSharedTextureMemoryBeginAccessDescriptor const *>(descriptor));
    return static_cast<Status>(result);
}
Texture SharedTextureMemory::CreateTexture(TextureDescriptor const * descriptor) const {
    auto result = wgpuSharedTextureMemoryCreateTexture(Get(), reinterpret_cast<WGPUTextureDescriptor const *>(descriptor));
    return Texture::Acquire(result);
}
ConvertibleStatus SharedTextureMemory::EndAccess(Texture const& texture, SharedTextureMemoryEndAccessState * descriptor) const {
    *descriptor = SharedTextureMemoryEndAccessState();
    auto result = wgpuSharedTextureMemoryEndAccess(Get(), texture.Get(), reinterpret_cast<WGPUSharedTextureMemoryEndAccessState *>(descriptor));
    return static_cast<Status>(result);
}
ConvertibleStatus SharedTextureMemory::GetProperties(SharedTextureMemoryProperties * properties) const {
    auto result = wgpuSharedTextureMemoryGetProperties(Get(), reinterpret_cast<WGPUSharedTextureMemoryProperties *>(properties));
    return static_cast<Status>(result);
}
Bool SharedTextureMemory::IsDeviceLost() const {
    auto result = wgpuSharedTextureMemoryIsDeviceLost(Get());
    return result;
}
void SharedTextureMemory::SetLabel(StringView label) const {
    wgpuSharedTextureMemorySetLabel(Get(), *reinterpret_cast<WGPUStringView const*>(&label));
}


void SharedTextureMemory::WGPUAddRef(WGPUSharedTextureMemory handle) {
    if (handle != nullptr) {
        wgpuSharedTextureMemoryAddRef(handle);
    }
}
void SharedTextureMemory::WGPURelease(WGPUSharedTextureMemory handle) {
    if (handle != nullptr) {
        wgpuSharedTextureMemoryRelease(handle);
    }
}
static_assert(sizeof(SharedTextureMemory) == sizeof(WGPUSharedTextureMemory), "sizeof mismatch for SharedTextureMemory");
static_assert(alignof(SharedTextureMemory) == alignof(WGPUSharedTextureMemory), "alignof mismatch for SharedTextureMemory");

// Surface implementation

void Surface::Configure(SurfaceConfiguration const * config) const {
    wgpuSurfaceConfigure(Get(), reinterpret_cast<WGPUSurfaceConfiguration const *>(config));
}
ConvertibleStatus Surface::GetCapabilities(Adapter const& adapter, SurfaceCapabilities * capabilities) const {
    *capabilities = SurfaceCapabilities();
    auto result = wgpuSurfaceGetCapabilities(Get(), adapter.Get(), reinterpret_cast<WGPUSurfaceCapabilities *>(capabilities));
    return static_cast<Status>(result);
}
void Surface::GetCurrentTexture(SurfaceTexture * surfaceTexture) const {
    wgpuSurfaceGetCurrentTexture(Get(), reinterpret_cast<WGPUSurfaceTexture *>(surfaceTexture));
}
ConvertibleStatus Surface::Present() const {
    auto result = wgpuSurfacePresent(Get());
    return static_cast<Status>(result);
}
void Surface::SetLabel(StringView label) const {
    wgpuSurfaceSetLabel(Get(), *reinterpret_cast<WGPUStringView const*>(&label));
}
void Surface::Unconfigure() const {
    wgpuSurfaceUnconfigure(Get());
}


void Surface::WGPUAddRef(WGPUSurface handle) {
    if (handle != nullptr) {
        wgpuSurfaceAddRef(handle);
    }
}
void Surface::WGPURelease(WGPUSurface handle) {
    if (handle != nullptr) {
        wgpuSurfaceRelease(handle);
    }
}
static_assert(sizeof(Surface) == sizeof(WGPUSurface), "sizeof mismatch for Surface");
static_assert(alignof(Surface) == alignof(WGPUSurface), "alignof mismatch for Surface");

// TexelBufferView implementation

void TexelBufferView::SetLabel(StringView label) const {
    wgpuTexelBufferViewSetLabel(Get(), *reinterpret_cast<WGPUStringView const*>(&label));
}


void TexelBufferView::WGPUAddRef(WGPUTexelBufferView handle) {
    if (handle != nullptr) {
        wgpuTexelBufferViewAddRef(handle);
    }
}
void TexelBufferView::WGPURelease(WGPUTexelBufferView handle) {
    if (handle != nullptr) {
        wgpuTexelBufferViewRelease(handle);
    }
}
static_assert(sizeof(TexelBufferView) == sizeof(WGPUTexelBufferView), "sizeof mismatch for TexelBufferView");
static_assert(alignof(TexelBufferView) == alignof(WGPUTexelBufferView), "alignof mismatch for TexelBufferView");

// Texture implementation

TextureView Texture::CreateErrorView(TextureViewDescriptor const * descriptor) const {
    auto result = wgpuTextureCreateErrorView(Get(), reinterpret_cast<WGPUTextureViewDescriptor const *>(descriptor));
    return TextureView::Acquire(result);
}
TextureView Texture::CreateView(TextureViewDescriptor const * descriptor) const {
    auto result = wgpuTextureCreateView(Get(), reinterpret_cast<WGPUTextureViewDescriptor const *>(descriptor));
    return TextureView::Acquire(result);
}
void Texture::Destroy() const {
    wgpuTextureDestroy(Get());
}
uint32_t Texture::GetDepthOrArrayLayers() const {
    auto result = wgpuTextureGetDepthOrArrayLayers(Get());
    return result;
}
TextureDimension Texture::GetDimension() const {
    auto result = wgpuTextureGetDimension(Get());
    return static_cast<TextureDimension>(result);
}
TextureFormat Texture::GetFormat() const {
    auto result = wgpuTextureGetFormat(Get());
    return static_cast<TextureFormat>(result);
}
uint32_t Texture::GetHeight() const {
    auto result = wgpuTextureGetHeight(Get());
    return result;
}
uint32_t Texture::GetMipLevelCount() const {
    auto result = wgpuTextureGetMipLevelCount(Get());
    return result;
}
uint32_t Texture::GetSampleCount() const {
    auto result = wgpuTextureGetSampleCount(Get());
    return result;
}
TextureViewDimension Texture::GetTextureBindingViewDimension() const {
    auto result = wgpuTextureGetTextureBindingViewDimension(Get());
    return static_cast<TextureViewDimension>(result);
}
TextureUsage Texture::GetUsage() const {
    auto result = wgpuTextureGetUsage(Get());
    return static_cast<TextureUsage>(result);
}
uint32_t Texture::GetWidth() const {
    auto result = wgpuTextureGetWidth(Get());
    return result;
}
void Texture::SetLabel(StringView label) const {
    wgpuTextureSetLabel(Get(), *reinterpret_cast<WGPUStringView const*>(&label));
}
void Texture::SetOwnershipForMemoryDump(uint64_t ownerGuid) const {
    wgpuTextureSetOwnershipForMemoryDump(Get(), ownerGuid);
}


void Texture::WGPUAddRef(WGPUTexture handle) {
    if (handle != nullptr) {
        wgpuTextureAddRef(handle);
    }
}
void Texture::WGPURelease(WGPUTexture handle) {
    if (handle != nullptr) {
        wgpuTextureRelease(handle);
    }
}
static_assert(sizeof(Texture) == sizeof(WGPUTexture), "sizeof mismatch for Texture");
static_assert(alignof(Texture) == alignof(WGPUTexture), "alignof mismatch for Texture");

// TextureView implementation

void TextureView::SetLabel(StringView label) const {
    wgpuTextureViewSetLabel(Get(), *reinterpret_cast<WGPUStringView const*>(&label));
}


void TextureView::WGPUAddRef(WGPUTextureView handle) {
    if (handle != nullptr) {
        wgpuTextureViewAddRef(handle);
    }
}
void TextureView::WGPURelease(WGPUTextureView handle) {
    if (handle != nullptr) {
        wgpuTextureViewRelease(handle);
    }
}
static_assert(sizeof(TextureView) == sizeof(WGPUTextureView), "sizeof mismatch for TextureView");
static_assert(alignof(TextureView) == alignof(WGPUTextureView), "alignof mismatch for TextureView");

// ComputePassEncoder implementation

void ComputePassEncoder::DispatchWorkgroups(uint32_t workgroupCountX, uint32_t workgroupCountY, uint32_t workgroupCountZ) const {
    wgpuComputePassEncoderDispatchWorkgroups(Get(), workgroupCountX, workgroupCountY, workgroupCountZ);
}
void ComputePassEncoder::DispatchWorkgroupsIndirect(Buffer const& indirectBuffer, uint64_t indirectOffset) const {
    wgpuComputePassEncoderDispatchWorkgroupsIndirect(Get(), indirectBuffer.Get(), indirectOffset);
}
void ComputePassEncoder::End() const {
    wgpuComputePassEncoderEnd(Get());
}
void ComputePassEncoder::InsertDebugMarker(StringView markerLabel) const {
    wgpuComputePassEncoderInsertDebugMarker(Get(), *reinterpret_cast<WGPUStringView const*>(&markerLabel));
}
void ComputePassEncoder::PopDebugGroup() const {
    wgpuComputePassEncoderPopDebugGroup(Get());
}
void ComputePassEncoder::PushDebugGroup(StringView groupLabel) const {
    wgpuComputePassEncoderPushDebugGroup(Get(), *reinterpret_cast<WGPUStringView const*>(&groupLabel));
}
void ComputePassEncoder::SetBindGroup(uint32_t groupIndex, BindGroup const& group, size_t dynamicOffsetCount, uint32_t const * dynamicOffsets) const {
    wgpuComputePassEncoderSetBindGroup(Get(), groupIndex, group.Get(), dynamicOffsetCount, reinterpret_cast<uint32_t const *>(dynamicOffsets));
}
void ComputePassEncoder::SetImmediates(uint32_t offset, void const * data, size_t size) const {
    wgpuComputePassEncoderSetImmediates(Get(), offset, reinterpret_cast<void const *>(data), size);
}
void ComputePassEncoder::SetLabel(StringView label) const {
    wgpuComputePassEncoderSetLabel(Get(), *reinterpret_cast<WGPUStringView const*>(&label));
}
void ComputePassEncoder::SetPipeline(ComputePipeline const& pipeline) const {
    wgpuComputePassEncoderSetPipeline(Get(), pipeline.Get());
}
void ComputePassEncoder::SetResourceTable(ResourceTable const& table) const {
    wgpuComputePassEncoderSetResourceTable(Get(), table.Get());
}
void ComputePassEncoder::WriteTimestamp(QuerySet const& querySet, uint32_t queryIndex) const {
    wgpuComputePassEncoderWriteTimestamp(Get(), querySet.Get(), queryIndex);
}


void ComputePassEncoder::WGPUAddRef(WGPUComputePassEncoder handle) {
    if (handle != nullptr) {
        wgpuComputePassEncoderAddRef(handle);
    }
}
void ComputePassEncoder::WGPURelease(WGPUComputePassEncoder handle) {
    if (handle != nullptr) {
        wgpuComputePassEncoderRelease(handle);
    }
}
static_assert(sizeof(ComputePassEncoder) == sizeof(WGPUComputePassEncoder), "sizeof mismatch for ComputePassEncoder");
static_assert(alignof(ComputePassEncoder) == alignof(WGPUComputePassEncoder), "alignof mismatch for ComputePassEncoder");

// RenderBundleEncoder implementation

void RenderBundleEncoder::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) const {
    wgpuRenderBundleEncoderDraw(Get(), vertexCount, instanceCount, firstVertex, firstInstance);
}
void RenderBundleEncoder::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t baseVertex, uint32_t firstInstance) const {
    wgpuRenderBundleEncoderDrawIndexed(Get(), indexCount, instanceCount, firstIndex, baseVertex, firstInstance);
}
void RenderBundleEncoder::DrawIndexedIndirect(Buffer const& indirectBuffer, uint64_t indirectOffset) const {
    wgpuRenderBundleEncoderDrawIndexedIndirect(Get(), indirectBuffer.Get(), indirectOffset);
}
void RenderBundleEncoder::DrawIndirect(Buffer const& indirectBuffer, uint64_t indirectOffset) const {
    wgpuRenderBundleEncoderDrawIndirect(Get(), indirectBuffer.Get(), indirectOffset);
}
RenderBundle RenderBundleEncoder::Finish(RenderBundleDescriptor const * descriptor) const {
    auto result = wgpuRenderBundleEncoderFinish(Get(), reinterpret_cast<WGPURenderBundleDescriptor const *>(descriptor));
    return RenderBundle::Acquire(result);
}
void RenderBundleEncoder::InsertDebugMarker(StringView markerLabel) const {
    wgpuRenderBundleEncoderInsertDebugMarker(Get(), *reinterpret_cast<WGPUStringView const*>(&markerLabel));
}
void RenderBundleEncoder::PopDebugGroup() const {
    wgpuRenderBundleEncoderPopDebugGroup(Get());
}
void RenderBundleEncoder::PushDebugGroup(StringView groupLabel) const {
    wgpuRenderBundleEncoderPushDebugGroup(Get(), *reinterpret_cast<WGPUStringView const*>(&groupLabel));
}
void RenderBundleEncoder::SetBindGroup(uint32_t groupIndex, BindGroup const& group, size_t dynamicOffsetCount, uint32_t const * dynamicOffsets) const {
    wgpuRenderBundleEncoderSetBindGroup(Get(), groupIndex, group.Get(), dynamicOffsetCount, reinterpret_cast<uint32_t const *>(dynamicOffsets));
}
void RenderBundleEncoder::SetImmediates(uint32_t offset, void const * data, size_t size) const {
    wgpuRenderBundleEncoderSetImmediates(Get(), offset, reinterpret_cast<void const *>(data), size);
}
void RenderBundleEncoder::SetIndexBuffer(Buffer const& buffer, IndexFormat format, uint64_t offset, uint64_t size) const {
    wgpuRenderBundleEncoderSetIndexBuffer(Get(), buffer.Get(), static_cast<WGPUIndexFormat>(format), offset, size);
}
void RenderBundleEncoder::SetLabel(StringView label) const {
    wgpuRenderBundleEncoderSetLabel(Get(), *reinterpret_cast<WGPUStringView const*>(&label));
}
void RenderBundleEncoder::SetPipeline(RenderPipeline const& pipeline) const {
    wgpuRenderBundleEncoderSetPipeline(Get(), pipeline.Get());
}
void RenderBundleEncoder::SetResourceTable(ResourceTable const& table) const {
    wgpuRenderBundleEncoderSetResourceTable(Get(), table.Get());
}
void RenderBundleEncoder::SetVertexBuffer(uint32_t slot, Buffer const& buffer, uint64_t offset, uint64_t size) const {
    wgpuRenderBundleEncoderSetVertexBuffer(Get(), slot, buffer.Get(), offset, size);
}


void RenderBundleEncoder::WGPUAddRef(WGPURenderBundleEncoder handle) {
    if (handle != nullptr) {
        wgpuRenderBundleEncoderAddRef(handle);
    }
}
void RenderBundleEncoder::WGPURelease(WGPURenderBundleEncoder handle) {
    if (handle != nullptr) {
        wgpuRenderBundleEncoderRelease(handle);
    }
}
static_assert(sizeof(RenderBundleEncoder) == sizeof(WGPURenderBundleEncoder), "sizeof mismatch for RenderBundleEncoder");
static_assert(alignof(RenderBundleEncoder) == alignof(WGPURenderBundleEncoder), "alignof mismatch for RenderBundleEncoder");

// RenderPassEncoder implementation

void RenderPassEncoder::BeginOcclusionQuery(uint32_t queryIndex) const {
    wgpuRenderPassEncoderBeginOcclusionQuery(Get(), queryIndex);
}
void RenderPassEncoder::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) const {
    wgpuRenderPassEncoderDraw(Get(), vertexCount, instanceCount, firstVertex, firstInstance);
}
void RenderPassEncoder::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t baseVertex, uint32_t firstInstance) const {
    wgpuRenderPassEncoderDrawIndexed(Get(), indexCount, instanceCount, firstIndex, baseVertex, firstInstance);
}
void RenderPassEncoder::DrawIndexedIndirect(Buffer const& indirectBuffer, uint64_t indirectOffset) const {
    wgpuRenderPassEncoderDrawIndexedIndirect(Get(), indirectBuffer.Get(), indirectOffset);
}
void RenderPassEncoder::DrawIndirect(Buffer const& indirectBuffer, uint64_t indirectOffset) const {
    wgpuRenderPassEncoderDrawIndirect(Get(), indirectBuffer.Get(), indirectOffset);
}
void RenderPassEncoder::End() const {
    wgpuRenderPassEncoderEnd(Get());
}
void RenderPassEncoder::EndOcclusionQuery() const {
    wgpuRenderPassEncoderEndOcclusionQuery(Get());
}
void RenderPassEncoder::ExecuteBundles(size_t bundleCount, RenderBundle const * bundles) const {
    wgpuRenderPassEncoderExecuteBundles(Get(), bundleCount, reinterpret_cast<WGPURenderBundle const *>(bundles));
}
void RenderPassEncoder::InsertDebugMarker(StringView markerLabel) const {
    wgpuRenderPassEncoderInsertDebugMarker(Get(), *reinterpret_cast<WGPUStringView const*>(&markerLabel));
}
void RenderPassEncoder::MultiDrawIndexedIndirect(Buffer const& indirectBuffer, uint64_t indirectOffset, uint32_t maxDrawCount, Buffer const& drawCountBuffer, uint64_t drawCountBufferOffset) const {
    wgpuRenderPassEncoderMultiDrawIndexedIndirect(Get(), indirectBuffer.Get(), indirectOffset, maxDrawCount, drawCountBuffer.Get(), drawCountBufferOffset);
}
void RenderPassEncoder::MultiDrawIndirect(Buffer const& indirectBuffer, uint64_t indirectOffset, uint32_t maxDrawCount, Buffer const& drawCountBuffer, uint64_t drawCountBufferOffset) const {
    wgpuRenderPassEncoderMultiDrawIndirect(Get(), indirectBuffer.Get(), indirectOffset, maxDrawCount, drawCountBuffer.Get(), drawCountBufferOffset);
}
void RenderPassEncoder::PixelLocalStorageBarrier() const {
    wgpuRenderPassEncoderPixelLocalStorageBarrier(Get());
}
void RenderPassEncoder::PopDebugGroup() const {
    wgpuRenderPassEncoderPopDebugGroup(Get());
}
void RenderPassEncoder::PushDebugGroup(StringView groupLabel) const {
    wgpuRenderPassEncoderPushDebugGroup(Get(), *reinterpret_cast<WGPUStringView const*>(&groupLabel));
}
void RenderPassEncoder::SetBindGroup(uint32_t groupIndex, BindGroup const& group, size_t dynamicOffsetCount, uint32_t const * dynamicOffsets) const {
    wgpuRenderPassEncoderSetBindGroup(Get(), groupIndex, group.Get(), dynamicOffsetCount, reinterpret_cast<uint32_t const *>(dynamicOffsets));
}
void RenderPassEncoder::SetBlendConstant(Color const * color) const {
    wgpuRenderPassEncoderSetBlendConstant(Get(), reinterpret_cast<WGPUColor const *>(color));
}
void RenderPassEncoder::SetImmediates(uint32_t offset, void const * data, size_t size) const {
    wgpuRenderPassEncoderSetImmediates(Get(), offset, reinterpret_cast<void const *>(data), size);
}
void RenderPassEncoder::SetIndexBuffer(Buffer const& buffer, IndexFormat format, uint64_t offset, uint64_t size) const {
    wgpuRenderPassEncoderSetIndexBuffer(Get(), buffer.Get(), static_cast<WGPUIndexFormat>(format), offset, size);
}
void RenderPassEncoder::SetLabel(StringView label) const {
    wgpuRenderPassEncoderSetLabel(Get(), *reinterpret_cast<WGPUStringView const*>(&label));
}
void RenderPassEncoder::SetPipeline(RenderPipeline const& pipeline) const {
    wgpuRenderPassEncoderSetPipeline(Get(), pipeline.Get());
}
void RenderPassEncoder::SetResourceTable(ResourceTable const& table) const {
    wgpuRenderPassEncoderSetResourceTable(Get(), table.Get());
}
void RenderPassEncoder::SetScissorRect(uint32_t x, uint32_t y, uint32_t width, uint32_t height) const {
    wgpuRenderPassEncoderSetScissorRect(Get(), x, y, width, height);
}
void RenderPassEncoder::SetStencilReference(uint32_t reference) const {
    wgpuRenderPassEncoderSetStencilReference(Get(), reference);
}
void RenderPassEncoder::SetVertexBuffer(uint32_t slot, Buffer const& buffer, uint64_t offset, uint64_t size) const {
    wgpuRenderPassEncoderSetVertexBuffer(Get(), slot, buffer.Get(), offset, size);
}
void RenderPassEncoder::SetViewport(float x, float y, float width, float height, float minDepth, float maxDepth) const {
    wgpuRenderPassEncoderSetViewport(Get(), x, y, width, height, minDepth, maxDepth);
}
void RenderPassEncoder::WriteTimestamp(QuerySet const& querySet, uint32_t queryIndex) const {
    wgpuRenderPassEncoderWriteTimestamp(Get(), querySet.Get(), queryIndex);
}


void RenderPassEncoder::WGPUAddRef(WGPURenderPassEncoder handle) {
    if (handle != nullptr) {
        wgpuRenderPassEncoderAddRef(handle);
    }
}
void RenderPassEncoder::WGPURelease(WGPURenderPassEncoder handle) {
    if (handle != nullptr) {
        wgpuRenderPassEncoderRelease(handle);
    }
}
static_assert(sizeof(RenderPassEncoder) == sizeof(WGPURenderPassEncoder), "sizeof mismatch for RenderPassEncoder");
static_assert(alignof(RenderPassEncoder) == alignof(WGPURenderPassEncoder), "alignof mismatch for RenderPassEncoder");




// Free Functions
inline Instance CreateInstance(InstanceDescriptor const * descriptor = nullptr) {
    auto result = wgpuCreateInstance(reinterpret_cast<WGPUInstanceDescriptor const *>(descriptor));
    return Instance::Acquire(result);
}
inline void GetInstanceFeatures(SupportedInstanceFeatures * features) {
    wgpuGetInstanceFeatures(reinterpret_cast<WGPUSupportedInstanceFeatures *>(features));
}
inline Status GetInstanceLimits(InstanceLimits * limits) {
    auto result = wgpuGetInstanceLimits(reinterpret_cast<WGPUInstanceLimits *>(limits));
    return static_cast<Status>(result);
}
inline Bool HasInstanceFeature(InstanceFeatureName feature) {
    auto result = wgpuHasInstanceFeature(static_cast<WGPUInstanceFeatureName>(feature));
    return result;
}
inline Proc GetProcAddress(StringView procName) {
    auto result = wgpuGetProcAddress(*reinterpret_cast<WGPUStringView const*>(&procName));
    return reinterpret_cast<Proc>(result);
}

}  // namespace wgpu

namespace wgpu {

template<>
struct IsWGPUBitmask<wgpu::BufferUsage> {
    static constexpr bool enable = true;
};

template<>
struct IsWGPUBitmask<wgpu::ColorWriteMask> {
    static constexpr bool enable = true;
};

template<>
struct IsWGPUBitmask<wgpu::HeapProperty> {
    static constexpr bool enable = true;
};

template<>
struct IsWGPUBitmask<wgpu::MapMode> {
    static constexpr bool enable = true;
};

template<>
struct IsWGPUBitmask<wgpu::ShaderStage> {
    static constexpr bool enable = true;
};

template<>
struct IsWGPUBitmask<wgpu::TextureUsage> {
    static constexpr bool enable = true;
};


inline bool operator==(const TextureComponentSwizzle& s1, const TextureComponentSwizzle& s2) {
    return s1.r == s2.r && s1.g == s2.g && s1.b == s2.b && s1.a == s2.a;
}
inline bool operator!=(const TextureComponentSwizzle& s1, const TextureComponentSwizzle& s2) {
    return !(s1 == s2);
}

} // namespace wgpu

namespace std {
// Custom boolean class needs corresponding hash function so that it appears as a transparent bool.
template <>
struct hash<wgpu::Bool> {
  public:
    size_t operator()(const wgpu::Bool &v) const {
        return hash<bool>()(v);
    }
};
template <>
struct hash<wgpu::OptionalBool> {
  public:
    size_t operator()(const wgpu::OptionalBool &v) const {
        return hash<WGPUOptionalBool>()(v.mValue);
    }
};
}  // namespace std

#endif // WEBGPU_CPP_H_
