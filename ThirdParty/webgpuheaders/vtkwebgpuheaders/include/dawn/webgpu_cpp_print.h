// Copyright 2021 The Dawn & Tint Authors
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

#ifndef WEBGPU_CPP_PRINT_H_
#define WEBGPU_CPP_PRINT_H_

#include "webgpu/webgpu_cpp.h"

#include <iomanip>
#include <ios>
#include <ostream>
#include <type_traits>

namespace wgpu {

  template <typename CharT, typename Traits>
  std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& o, AdapterType value) {
      switch (value) {
      case AdapterType::DiscreteGPU:
        o << "AdapterType::DiscreteGPU";
        break;
      case AdapterType::IntegratedGPU:
        o << "AdapterType::IntegratedGPU";
        break;
      case AdapterType::CPU:
        o << "AdapterType::CPU";
        break;
      case AdapterType::Unknown:
        o << "AdapterType::Unknown";
        break;
          default:
            o << "AdapterType::" << std::showbase << std::hex << std::setfill('0') << std::setw(4) << static_cast<std::underlying_type<AdapterType>::type>(value);
      }
      return o;
  }
  template <typename CharT, typename Traits>
  std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& o, AddressMode value) {
      switch (value) {
      case AddressMode::Undefined:
        o << "AddressMode::Undefined";
        break;
      case AddressMode::ClampToEdge:
        o << "AddressMode::ClampToEdge";
        break;
      case AddressMode::Repeat:
        o << "AddressMode::Repeat";
        break;
      case AddressMode::MirrorRepeat:
        o << "AddressMode::MirrorRepeat";
        break;
          default:
            o << "AddressMode::" << std::showbase << std::hex << std::setfill('0') << std::setw(4) << static_cast<std::underlying_type<AddressMode>::type>(value);
      }
      return o;
  }
  template <typename CharT, typename Traits>
  std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& o, AlphaMode value) {
      switch (value) {
      case AlphaMode::Opaque:
        o << "AlphaMode::Opaque";
        break;
      case AlphaMode::Premultiplied:
        o << "AlphaMode::Premultiplied";
        break;
      case AlphaMode::Unpremultiplied:
        o << "AlphaMode::Unpremultiplied";
        break;
          default:
            o << "AlphaMode::" << std::showbase << std::hex << std::setfill('0') << std::setw(4) << static_cast<std::underlying_type<AlphaMode>::type>(value);
      }
      return o;
  }
  template <typename CharT, typename Traits>
  std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& o, BackendType value) {
      switch (value) {
      case BackendType::Undefined:
        o << "BackendType::Undefined";
        break;
      case BackendType::Null:
        o << "BackendType::Null";
        break;
      case BackendType::WebGPU:
        o << "BackendType::WebGPU";
        break;
      case BackendType::D3D11:
        o << "BackendType::D3D11";
        break;
      case BackendType::D3D12:
        o << "BackendType::D3D12";
        break;
      case BackendType::Metal:
        o << "BackendType::Metal";
        break;
      case BackendType::Vulkan:
        o << "BackendType::Vulkan";
        break;
      case BackendType::OpenGL:
        o << "BackendType::OpenGL";
        break;
      case BackendType::OpenGLES:
        o << "BackendType::OpenGLES";
        break;
          default:
            o << "BackendType::" << std::showbase << std::hex << std::setfill('0') << std::setw(4) << static_cast<std::underlying_type<BackendType>::type>(value);
      }
      return o;
  }
  template <typename CharT, typename Traits>
  std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& o, BlendFactor value) {
      switch (value) {
      case BlendFactor::Undefined:
        o << "BlendFactor::Undefined";
        break;
      case BlendFactor::Zero:
        o << "BlendFactor::Zero";
        break;
      case BlendFactor::One:
        o << "BlendFactor::One";
        break;
      case BlendFactor::Src:
        o << "BlendFactor::Src";
        break;
      case BlendFactor::OneMinusSrc:
        o << "BlendFactor::OneMinusSrc";
        break;
      case BlendFactor::SrcAlpha:
        o << "BlendFactor::SrcAlpha";
        break;
      case BlendFactor::OneMinusSrcAlpha:
        o << "BlendFactor::OneMinusSrcAlpha";
        break;
      case BlendFactor::Dst:
        o << "BlendFactor::Dst";
        break;
      case BlendFactor::OneMinusDst:
        o << "BlendFactor::OneMinusDst";
        break;
      case BlendFactor::DstAlpha:
        o << "BlendFactor::DstAlpha";
        break;
      case BlendFactor::OneMinusDstAlpha:
        o << "BlendFactor::OneMinusDstAlpha";
        break;
      case BlendFactor::SrcAlphaSaturated:
        o << "BlendFactor::SrcAlphaSaturated";
        break;
      case BlendFactor::Constant:
        o << "BlendFactor::Constant";
        break;
      case BlendFactor::OneMinusConstant:
        o << "BlendFactor::OneMinusConstant";
        break;
      case BlendFactor::Src1:
        o << "BlendFactor::Src1";
        break;
      case BlendFactor::OneMinusSrc1:
        o << "BlendFactor::OneMinusSrc1";
        break;
      case BlendFactor::Src1Alpha:
        o << "BlendFactor::Src1Alpha";
        break;
      case BlendFactor::OneMinusSrc1Alpha:
        o << "BlendFactor::OneMinusSrc1Alpha";
        break;
          default:
            o << "BlendFactor::" << std::showbase << std::hex << std::setfill('0') << std::setw(4) << static_cast<std::underlying_type<BlendFactor>::type>(value);
      }
      return o;
  }
  template <typename CharT, typename Traits>
  std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& o, BlendOperation value) {
      switch (value) {
      case BlendOperation::Undefined:
        o << "BlendOperation::Undefined";
        break;
      case BlendOperation::Add:
        o << "BlendOperation::Add";
        break;
      case BlendOperation::Subtract:
        o << "BlendOperation::Subtract";
        break;
      case BlendOperation::ReverseSubtract:
        o << "BlendOperation::ReverseSubtract";
        break;
      case BlendOperation::Min:
        o << "BlendOperation::Min";
        break;
      case BlendOperation::Max:
        o << "BlendOperation::Max";
        break;
          default:
            o << "BlendOperation::" << std::showbase << std::hex << std::setfill('0') << std::setw(4) << static_cast<std::underlying_type<BlendOperation>::type>(value);
      }
      return o;
  }
  template <typename CharT, typename Traits>
  std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& o, BufferBindingType value) {
      switch (value) {
      case BufferBindingType::BindingNotUsed:
        o << "BufferBindingType::BindingNotUsed";
        break;
      case BufferBindingType::Undefined:
        o << "BufferBindingType::Undefined";
        break;
      case BufferBindingType::Uniform:
        o << "BufferBindingType::Uniform";
        break;
      case BufferBindingType::Storage:
        o << "BufferBindingType::Storage";
        break;
      case BufferBindingType::ReadOnlyStorage:
        o << "BufferBindingType::ReadOnlyStorage";
        break;
          default:
            o << "BufferBindingType::" << std::showbase << std::hex << std::setfill('0') << std::setw(4) << static_cast<std::underlying_type<BufferBindingType>::type>(value);
      }
      return o;
  }
  template <typename CharT, typename Traits>
  std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& o, BufferMapState value) {
      switch (value) {
      case BufferMapState::Unmapped:
        o << "BufferMapState::Unmapped";
        break;
      case BufferMapState::Pending:
        o << "BufferMapState::Pending";
        break;
      case BufferMapState::Mapped:
        o << "BufferMapState::Mapped";
        break;
          default:
            o << "BufferMapState::" << std::showbase << std::hex << std::setfill('0') << std::setw(4) << static_cast<std::underlying_type<BufferMapState>::type>(value);
      }
      return o;
  }
  template <typename CharT, typename Traits>
  std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& o, CallbackMode value) {
      switch (value) {
      case CallbackMode::WaitAnyOnly:
        o << "CallbackMode::WaitAnyOnly";
        break;
      case CallbackMode::AllowProcessEvents:
        o << "CallbackMode::AllowProcessEvents";
        break;
      case CallbackMode::AllowSpontaneous:
        o << "CallbackMode::AllowSpontaneous";
        break;
          default:
            o << "CallbackMode::" << std::showbase << std::hex << std::setfill('0') << std::setw(4) << static_cast<std::underlying_type<CallbackMode>::type>(value);
      }
      return o;
  }
  template <typename CharT, typename Traits>
  std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& o, ColorSpacePrimariesDawn value) {
      switch (value) {
      case ColorSpacePrimariesDawn::SRGB:
        o << "ColorSpacePrimariesDawn::SRGB";
        break;
      case ColorSpacePrimariesDawn::Rec601:
        o << "ColorSpacePrimariesDawn::Rec601";
        break;
      case ColorSpacePrimariesDawn::Rec2020:
        o << "ColorSpacePrimariesDawn::Rec2020";
        break;
      case ColorSpacePrimariesDawn::DisplayP3:
        o << "ColorSpacePrimariesDawn::DisplayP3";
        break;
          default:
            o << "ColorSpacePrimariesDawn::" << std::showbase << std::hex << std::setfill('0') << std::setw(4) << static_cast<std::underlying_type<ColorSpacePrimariesDawn>::type>(value);
      }
      return o;
  }
  template <typename CharT, typename Traits>
  std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& o, ColorSpaceTransferDawn value) {
      switch (value) {
      case ColorSpaceTransferDawn::Identity:
        o << "ColorSpaceTransferDawn::Identity";
        break;
      case ColorSpaceTransferDawn::SRGB:
        o << "ColorSpaceTransferDawn::SRGB";
        break;
      case ColorSpaceTransferDawn::DisplayP3:
        o << "ColorSpaceTransferDawn::DisplayP3";
        break;
      case ColorSpaceTransferDawn::SMPTE_170M:
        o << "ColorSpaceTransferDawn::SMPTE_170M";
        break;
      case ColorSpaceTransferDawn::HLG:
        o << "ColorSpaceTransferDawn::HLG";
        break;
      case ColorSpaceTransferDawn::PQ:
        o << "ColorSpaceTransferDawn::PQ";
        break;
          default:
            o << "ColorSpaceTransferDawn::" << std::showbase << std::hex << std::setfill('0') << std::setw(4) << static_cast<std::underlying_type<ColorSpaceTransferDawn>::type>(value);
      }
      return o;
  }
  template <typename CharT, typename Traits>
  std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& o, ColorSpaceYCbCrMatrixDawn value) {
      switch (value) {
      case ColorSpaceYCbCrMatrixDawn::Identity:
        o << "ColorSpaceYCbCrMatrixDawn::Identity";
        break;
      case ColorSpaceYCbCrMatrixDawn::Rec601:
        o << "ColorSpaceYCbCrMatrixDawn::Rec601";
        break;
      case ColorSpaceYCbCrMatrixDawn::Rec709:
        o << "ColorSpaceYCbCrMatrixDawn::Rec709";
        break;
      case ColorSpaceYCbCrMatrixDawn::Rec2020:
        o << "ColorSpaceYCbCrMatrixDawn::Rec2020";
        break;
          default:
            o << "ColorSpaceYCbCrMatrixDawn::" << std::showbase << std::hex << std::setfill('0') << std::setw(4) << static_cast<std::underlying_type<ColorSpaceYCbCrMatrixDawn>::type>(value);
      }
      return o;
  }
  template <typename CharT, typename Traits>
  std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& o, ColorSpaceYCbCrRangeDawn value) {
      switch (value) {
      case ColorSpaceYCbCrRangeDawn::Identity:
        o << "ColorSpaceYCbCrRangeDawn::Identity";
        break;
      case ColorSpaceYCbCrRangeDawn::Narrow:
        o << "ColorSpaceYCbCrRangeDawn::Narrow";
        break;
      case ColorSpaceYCbCrRangeDawn::Full:
        o << "ColorSpaceYCbCrRangeDawn::Full";
        break;
          default:
            o << "ColorSpaceYCbCrRangeDawn::" << std::showbase << std::hex << std::setfill('0') << std::setw(4) << static_cast<std::underlying_type<ColorSpaceYCbCrRangeDawn>::type>(value);
      }
      return o;
  }
  template <typename CharT, typename Traits>
  std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& o, CompareFunction value) {
      switch (value) {
      case CompareFunction::Undefined:
        o << "CompareFunction::Undefined";
        break;
      case CompareFunction::Never:
        o << "CompareFunction::Never";
        break;
      case CompareFunction::Less:
        o << "CompareFunction::Less";
        break;
      case CompareFunction::Equal:
        o << "CompareFunction::Equal";
        break;
      case CompareFunction::LessEqual:
        o << "CompareFunction::LessEqual";
        break;
      case CompareFunction::Greater:
        o << "CompareFunction::Greater";
        break;
      case CompareFunction::NotEqual:
        o << "CompareFunction::NotEqual";
        break;
      case CompareFunction::GreaterEqual:
        o << "CompareFunction::GreaterEqual";
        break;
      case CompareFunction::Always:
        o << "CompareFunction::Always";
        break;
          default:
            o << "CompareFunction::" << std::showbase << std::hex << std::setfill('0') << std::setw(4) << static_cast<std::underlying_type<CompareFunction>::type>(value);
      }
      return o;
  }
  template <typename CharT, typename Traits>
  std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& o, CompilationInfoRequestStatus value) {
      switch (value) {
      case CompilationInfoRequestStatus::Success:
        o << "CompilationInfoRequestStatus::Success";
        break;
      case CompilationInfoRequestStatus::CallbackCancelled:
        o << "CompilationInfoRequestStatus::CallbackCancelled";
        break;
          default:
            o << "CompilationInfoRequestStatus::" << std::showbase << std::hex << std::setfill('0') << std::setw(4) << static_cast<std::underlying_type<CompilationInfoRequestStatus>::type>(value);
      }
      return o;
  }
  template <typename CharT, typename Traits>
  std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& o, CompilationMessageType value) {
      switch (value) {
      case CompilationMessageType::Error:
        o << "CompilationMessageType::Error";
        break;
      case CompilationMessageType::Warning:
        o << "CompilationMessageType::Warning";
        break;
      case CompilationMessageType::Info:
        o << "CompilationMessageType::Info";
        break;
          default:
            o << "CompilationMessageType::" << std::showbase << std::hex << std::setfill('0') << std::setw(4) << static_cast<std::underlying_type<CompilationMessageType>::type>(value);
      }
      return o;
  }
  template <typename CharT, typename Traits>
  std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& o, ComponentSwizzle value) {
      switch (value) {
      case ComponentSwizzle::Undefined:
        o << "ComponentSwizzle::Undefined";
        break;
      case ComponentSwizzle::Zero:
        o << "ComponentSwizzle::Zero";
        break;
      case ComponentSwizzle::One:
        o << "ComponentSwizzle::One";
        break;
      case ComponentSwizzle::R:
        o << "ComponentSwizzle::R";
        break;
      case ComponentSwizzle::G:
        o << "ComponentSwizzle::G";
        break;
      case ComponentSwizzle::B:
        o << "ComponentSwizzle::B";
        break;
      case ComponentSwizzle::A:
        o << "ComponentSwizzle::A";
        break;
          default:
            o << "ComponentSwizzle::" << std::showbase << std::hex << std::setfill('0') << std::setw(4) << static_cast<std::underlying_type<ComponentSwizzle>::type>(value);
      }
      return o;
  }
  template <typename CharT, typename Traits>
  std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& o, CompositeAlphaMode value) {
      switch (value) {
      case CompositeAlphaMode::Auto:
        o << "CompositeAlphaMode::Auto";
        break;
      case CompositeAlphaMode::Opaque:
        o << "CompositeAlphaMode::Opaque";
        break;
      case CompositeAlphaMode::Premultiplied:
        o << "CompositeAlphaMode::Premultiplied";
        break;
      case CompositeAlphaMode::Unpremultiplied:
        o << "CompositeAlphaMode::Unpremultiplied";
        break;
      case CompositeAlphaMode::Inherit:
        o << "CompositeAlphaMode::Inherit";
        break;
          default:
            o << "CompositeAlphaMode::" << std::showbase << std::hex << std::setfill('0') << std::setw(4) << static_cast<std::underlying_type<CompositeAlphaMode>::type>(value);
      }
      return o;
  }
  template <typename CharT, typename Traits>
  std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& o, CreatePipelineAsyncStatus value) {
      switch (value) {
      case CreatePipelineAsyncStatus::Success:
        o << "CreatePipelineAsyncStatus::Success";
        break;
      case CreatePipelineAsyncStatus::CallbackCancelled:
        o << "CreatePipelineAsyncStatus::CallbackCancelled";
        break;
      case CreatePipelineAsyncStatus::ValidationError:
        o << "CreatePipelineAsyncStatus::ValidationError";
        break;
      case CreatePipelineAsyncStatus::InternalError:
        o << "CreatePipelineAsyncStatus::InternalError";
        break;
          default:
            o << "CreatePipelineAsyncStatus::" << std::showbase << std::hex << std::setfill('0') << std::setw(4) << static_cast<std::underlying_type<CreatePipelineAsyncStatus>::type>(value);
      }
      return o;
  }
  template <typename CharT, typename Traits>
  std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& o, CullMode value) {
      switch (value) {
      case CullMode::Undefined:
        o << "CullMode::Undefined";
        break;
      case CullMode::None:
        o << "CullMode::None";
        break;
      case CullMode::Front:
        o << "CullMode::Front";
        break;
      case CullMode::Back:
        o << "CullMode::Back";
        break;
          default:
            o << "CullMode::" << std::showbase << std::hex << std::setfill('0') << std::setw(4) << static_cast<std::underlying_type<CullMode>::type>(value);
      }
      return o;
  }
  template <typename CharT, typename Traits>
  std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& o, DeviceLostReason value) {
      switch (value) {
      case DeviceLostReason::Unknown:
        o << "DeviceLostReason::Unknown";
        break;
      case DeviceLostReason::Destroyed:
        o << "DeviceLostReason::Destroyed";
        break;
      case DeviceLostReason::CallbackCancelled:
        o << "DeviceLostReason::CallbackCancelled";
        break;
      case DeviceLostReason::FailedCreation:
        o << "DeviceLostReason::FailedCreation";
        break;
          default:
            o << "DeviceLostReason::" << std::showbase << std::hex << std::setfill('0') << std::setw(4) << static_cast<std::underlying_type<DeviceLostReason>::type>(value);
      }
      return o;
  }
  template <typename CharT, typename Traits>
  std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& o, ErrorFilter value) {
      switch (value) {
      case ErrorFilter::Validation:
        o << "ErrorFilter::Validation";
        break;
      case ErrorFilter::OutOfMemory:
        o << "ErrorFilter::OutOfMemory";
        break;
      case ErrorFilter::Internal:
        o << "ErrorFilter::Internal";
        break;
          default:
            o << "ErrorFilter::" << std::showbase << std::hex << std::setfill('0') << std::setw(4) << static_cast<std::underlying_type<ErrorFilter>::type>(value);
      }
      return o;
  }
  template <typename CharT, typename Traits>
  std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& o, ErrorType value) {
      switch (value) {
      case ErrorType::NoError:
        o << "ErrorType::NoError";
        break;
      case ErrorType::Validation:
        o << "ErrorType::Validation";
        break;
      case ErrorType::OutOfMemory:
        o << "ErrorType::OutOfMemory";
        break;
      case ErrorType::Internal:
        o << "ErrorType::Internal";
        break;
      case ErrorType::Unknown:
        o << "ErrorType::Unknown";
        break;
          default:
            o << "ErrorType::" << std::showbase << std::hex << std::setfill('0') << std::setw(4) << static_cast<std::underlying_type<ErrorType>::type>(value);
      }
      return o;
  }
  template <typename CharT, typename Traits>
  std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& o, ExternalTextureRotation value) {
      switch (value) {
      case ExternalTextureRotation::Rotate0Degrees:
        o << "ExternalTextureRotation::Rotate0Degrees";
        break;
      case ExternalTextureRotation::Rotate90Degrees:
        o << "ExternalTextureRotation::Rotate90Degrees";
        break;
      case ExternalTextureRotation::Rotate180Degrees:
        o << "ExternalTextureRotation::Rotate180Degrees";
        break;
      case ExternalTextureRotation::Rotate270Degrees:
        o << "ExternalTextureRotation::Rotate270Degrees";
        break;
          default:
            o << "ExternalTextureRotation::" << std::showbase << std::hex << std::setfill('0') << std::setw(4) << static_cast<std::underlying_type<ExternalTextureRotation>::type>(value);
      }
      return o;
  }
  template <typename CharT, typename Traits>
  std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& o, FeatureLevel value) {
      switch (value) {
      case FeatureLevel::Undefined:
        o << "FeatureLevel::Undefined";
        break;
      case FeatureLevel::Compatibility:
        o << "FeatureLevel::Compatibility";
        break;
      case FeatureLevel::Core:
        o << "FeatureLevel::Core";
        break;
          default:
            o << "FeatureLevel::" << std::showbase << std::hex << std::setfill('0') << std::setw(4) << static_cast<std::underlying_type<FeatureLevel>::type>(value);
      }
      return o;
  }
  template <typename CharT, typename Traits>
  std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& o, FeatureName value) {
      switch (value) {
      case FeatureName::CoreFeaturesAndLimits:
        o << "FeatureName::CoreFeaturesAndLimits";
        break;
      case FeatureName::DepthClipControl:
        o << "FeatureName::DepthClipControl";
        break;
      case FeatureName::Depth32FloatStencil8:
        o << "FeatureName::Depth32FloatStencil8";
        break;
      case FeatureName::TextureCompressionBC:
        o << "FeatureName::TextureCompressionBC";
        break;
      case FeatureName::TextureCompressionBCSliced3D:
        o << "FeatureName::TextureCompressionBCSliced3D";
        break;
      case FeatureName::TextureCompressionETC2:
        o << "FeatureName::TextureCompressionETC2";
        break;
      case FeatureName::TextureCompressionASTC:
        o << "FeatureName::TextureCompressionASTC";
        break;
      case FeatureName::TextureCompressionASTCSliced3D:
        o << "FeatureName::TextureCompressionASTCSliced3D";
        break;
      case FeatureName::TimestampQuery:
        o << "FeatureName::TimestampQuery";
        break;
      case FeatureName::IndirectFirstInstance:
        o << "FeatureName::IndirectFirstInstance";
        break;
      case FeatureName::ShaderF16:
        o << "FeatureName::ShaderF16";
        break;
      case FeatureName::RG11B10UfloatRenderable:
        o << "FeatureName::RG11B10UfloatRenderable";
        break;
      case FeatureName::BGRA8UnormStorage:
        o << "FeatureName::BGRA8UnormStorage";
        break;
      case FeatureName::Float32Filterable:
        o << "FeatureName::Float32Filterable";
        break;
      case FeatureName::Float32Blendable:
        o << "FeatureName::Float32Blendable";
        break;
      case FeatureName::ClipDistances:
        o << "FeatureName::ClipDistances";
        break;
      case FeatureName::DualSourceBlending:
        o << "FeatureName::DualSourceBlending";
        break;
      case FeatureName::Subgroups:
        o << "FeatureName::Subgroups";
        break;
      case FeatureName::TextureFormatsTier1:
        o << "FeatureName::TextureFormatsTier1";
        break;
      case FeatureName::TextureFormatsTier2:
        o << "FeatureName::TextureFormatsTier2";
        break;
      case FeatureName::PrimitiveIndex:
        o << "FeatureName::PrimitiveIndex";
        break;
      case FeatureName::TextureComponentSwizzle:
        o << "FeatureName::TextureComponentSwizzle";
        break;
      case FeatureName::SubgroupSizeControl:
        o << "FeatureName::SubgroupSizeControl";
        break;
      case FeatureName::DawnInternalUsages:
        o << "FeatureName::DawnInternalUsages";
        break;
      case FeatureName::DawnMultiPlanarFormats:
        o << "FeatureName::DawnMultiPlanarFormats";
        break;
      case FeatureName::DawnNative:
        o << "FeatureName::DawnNative";
        break;
      case FeatureName::ChromiumExperimentalTimestampQueryInsidePasses:
        o << "FeatureName::ChromiumExperimentalTimestampQueryInsidePasses";
        break;
      case FeatureName::ImplicitDeviceSynchronization:
        o << "FeatureName::ImplicitDeviceSynchronization";
        break;
      case FeatureName::TransientAttachments:
        o << "FeatureName::TransientAttachments";
        break;
      case FeatureName::MSAARenderToSingleSampled:
        o << "FeatureName::MSAARenderToSingleSampled";
        break;
      case FeatureName::D3D11MultithreadProtected:
        o << "FeatureName::D3D11MultithreadProtected";
        break;
      case FeatureName::ANGLETextureSharing:
        o << "FeatureName::ANGLETextureSharing";
        break;
      case FeatureName::PixelLocalStorageCoherent:
        o << "FeatureName::PixelLocalStorageCoherent";
        break;
      case FeatureName::PixelLocalStorageNonCoherent:
        o << "FeatureName::PixelLocalStorageNonCoherent";
        break;
      case FeatureName::Unorm16TextureFormats:
        o << "FeatureName::Unorm16TextureFormats";
        break;
      case FeatureName::MultiPlanarFormatExtendedUsages:
        o << "FeatureName::MultiPlanarFormatExtendedUsages";
        break;
      case FeatureName::MultiPlanarFormatP010:
        o << "FeatureName::MultiPlanarFormatP010";
        break;
      case FeatureName::HostMappedPointer:
        o << "FeatureName::HostMappedPointer";
        break;
      case FeatureName::MultiPlanarRenderTargets:
        o << "FeatureName::MultiPlanarRenderTargets";
        break;
      case FeatureName::MultiPlanarFormatNv12a:
        o << "FeatureName::MultiPlanarFormatNv12a";
        break;
      case FeatureName::FramebufferFetch:
        o << "FeatureName::FramebufferFetch";
        break;
      case FeatureName::BufferMapExtendedUsages:
        o << "FeatureName::BufferMapExtendedUsages";
        break;
      case FeatureName::AdapterPropertiesMemoryHeaps:
        o << "FeatureName::AdapterPropertiesMemoryHeaps";
        break;
      case FeatureName::AdapterPropertiesD3D:
        o << "FeatureName::AdapterPropertiesD3D";
        break;
      case FeatureName::AdapterPropertiesVk:
        o << "FeatureName::AdapterPropertiesVk";
        break;
      case FeatureName::DawnFormatCapabilities:
        o << "FeatureName::DawnFormatCapabilities";
        break;
      case FeatureName::DawnDrmFormatCapabilities:
        o << "FeatureName::DawnDrmFormatCapabilities";
        break;
      case FeatureName::MultiPlanarFormatNv16:
        o << "FeatureName::MultiPlanarFormatNv16";
        break;
      case FeatureName::MultiPlanarFormatNv24:
        o << "FeatureName::MultiPlanarFormatNv24";
        break;
      case FeatureName::MultiPlanarFormatP210:
        o << "FeatureName::MultiPlanarFormatP210";
        break;
      case FeatureName::MultiPlanarFormatP410:
        o << "FeatureName::MultiPlanarFormatP410";
        break;
      case FeatureName::SharedTextureMemoryVkDedicatedAllocation:
        o << "FeatureName::SharedTextureMemoryVkDedicatedAllocation";
        break;
      case FeatureName::SharedTextureMemoryAHardwareBuffer:
        o << "FeatureName::SharedTextureMemoryAHardwareBuffer";
        break;
      case FeatureName::SharedTextureMemoryDmaBuf:
        o << "FeatureName::SharedTextureMemoryDmaBuf";
        break;
      case FeatureName::SharedTextureMemoryOpaqueFD:
        o << "FeatureName::SharedTextureMemoryOpaqueFD";
        break;
      case FeatureName::SharedTextureMemoryZirconHandle:
        o << "FeatureName::SharedTextureMemoryZirconHandle";
        break;
      case FeatureName::SharedTextureMemoryDXGISharedHandle:
        o << "FeatureName::SharedTextureMemoryDXGISharedHandle";
        break;
      case FeatureName::SharedTextureMemoryD3D11Texture2D:
        o << "FeatureName::SharedTextureMemoryD3D11Texture2D";
        break;
      case FeatureName::SharedTextureMemoryIOSurface:
        o << "FeatureName::SharedTextureMemoryIOSurface";
        break;
      case FeatureName::SharedTextureMemoryEGLImage:
        o << "FeatureName::SharedTextureMemoryEGLImage";
        break;
      case FeatureName::SharedFenceVkSemaphoreOpaqueFD:
        o << "FeatureName::SharedFenceVkSemaphoreOpaqueFD";
        break;
      case FeatureName::SharedFenceSyncFD:
        o << "FeatureName::SharedFenceSyncFD";
        break;
      case FeatureName::SharedFenceVkSemaphoreZirconHandle:
        o << "FeatureName::SharedFenceVkSemaphoreZirconHandle";
        break;
      case FeatureName::SharedFenceDXGISharedHandle:
        o << "FeatureName::SharedFenceDXGISharedHandle";
        break;
      case FeatureName::SharedFenceMTLSharedEvent:
        o << "FeatureName::SharedFenceMTLSharedEvent";
        break;
      case FeatureName::SharedBufferMemoryD3D12Resource:
        o << "FeatureName::SharedBufferMemoryD3D12Resource";
        break;
      case FeatureName::StaticSamplers:
        o << "FeatureName::StaticSamplers";
        break;
      case FeatureName::YCbCrVulkanSamplers:
        o << "FeatureName::YCbCrVulkanSamplers";
        break;
      case FeatureName::ShaderModuleCompilationOptions:
        o << "FeatureName::ShaderModuleCompilationOptions";
        break;
      case FeatureName::DawnLoadResolveTexture:
        o << "FeatureName::DawnLoadResolveTexture";
        break;
      case FeatureName::DawnPartialLoadResolveTexture:
        o << "FeatureName::DawnPartialLoadResolveTexture";
        break;
      case FeatureName::MultiDrawIndirect:
        o << "FeatureName::MultiDrawIndirect";
        break;
      case FeatureName::DawnTexelCopyBufferRowAlignment:
        o << "FeatureName::DawnTexelCopyBufferRowAlignment";
        break;
      case FeatureName::FlexibleTextureViews:
        o << "FeatureName::FlexibleTextureViews";
        break;
      case FeatureName::ChromiumExperimentalSubgroupMatrix:
        o << "FeatureName::ChromiumExperimentalSubgroupMatrix";
        break;
      case FeatureName::SharedFenceEGLSync:
        o << "FeatureName::SharedFenceEGLSync";
        break;
      case FeatureName::DawnDeviceAllocatorControl:
        o << "FeatureName::DawnDeviceAllocatorControl";
        break;
      case FeatureName::AdapterPropertiesWGPU:
        o << "FeatureName::AdapterPropertiesWGPU";
        break;
      case FeatureName::SharedBufferMemoryFromWindowsHandle:
        o << "FeatureName::SharedBufferMemoryFromWindowsHandle";
        break;
      case FeatureName::SharedTextureMemoryD3D12Resource:
        o << "FeatureName::SharedTextureMemoryD3D12Resource";
        break;
      case FeatureName::ChromiumExperimentalSamplingResourceTable:
        o << "FeatureName::ChromiumExperimentalSamplingResourceTable";
        break;
      case FeatureName::AtomicVec2uMinMax:
        o << "FeatureName::AtomicVec2uMinMax";
        break;
      case FeatureName::Unorm16FormatsForExternalTexture:
        o << "FeatureName::Unorm16FormatsForExternalTexture";
        break;
      case FeatureName::OpaqueYCbCrAndroidForExternalTexture:
        o << "FeatureName::OpaqueYCbCrAndroidForExternalTexture";
        break;
      case FeatureName::Unorm16Filterable:
        o << "FeatureName::Unorm16Filterable";
        break;
      case FeatureName::RenderPassRenderArea:
        o << "FeatureName::RenderPassRenderArea";
        break;
      case FeatureName::AdapterPropertiesDrm:
        o << "FeatureName::AdapterPropertiesDrm";
        break;
      case FeatureName::TextureCompressionUnaligned:
        o << "FeatureName::TextureCompressionUnaligned";
        break;
          default:
            o << "FeatureName::" << std::showbase << std::hex << std::setfill('0') << std::setw(4) << static_cast<std::underlying_type<FeatureName>::type>(value);
      }
      return o;
  }
  template <typename CharT, typename Traits>
  std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& o, FilterMode value) {
      switch (value) {
      case FilterMode::Undefined:
        o << "FilterMode::Undefined";
        break;
      case FilterMode::Nearest:
        o << "FilterMode::Nearest";
        break;
      case FilterMode::Linear:
        o << "FilterMode::Linear";
        break;
          default:
            o << "FilterMode::" << std::showbase << std::hex << std::setfill('0') << std::setw(4) << static_cast<std::underlying_type<FilterMode>::type>(value);
      }
      return o;
  }
  template <typename CharT, typename Traits>
  std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& o, FrontFace value) {
      switch (value) {
      case FrontFace::Undefined:
        o << "FrontFace::Undefined";
        break;
      case FrontFace::CCW:
        o << "FrontFace::CCW";
        break;
      case FrontFace::CW:
        o << "FrontFace::CW";
        break;
          default:
            o << "FrontFace::" << std::showbase << std::hex << std::setfill('0') << std::setw(4) << static_cast<std::underlying_type<FrontFace>::type>(value);
      }
      return o;
  }
  template <typename CharT, typename Traits>
  std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& o, IndexFormat value) {
      switch (value) {
      case IndexFormat::Undefined:
        o << "IndexFormat::Undefined";
        break;
      case IndexFormat::Uint16:
        o << "IndexFormat::Uint16";
        break;
      case IndexFormat::Uint32:
        o << "IndexFormat::Uint32";
        break;
          default:
            o << "IndexFormat::" << std::showbase << std::hex << std::setfill('0') << std::setw(4) << static_cast<std::underlying_type<IndexFormat>::type>(value);
      }
      return o;
  }
  template <typename CharT, typename Traits>
  std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& o, InstanceFeatureName value) {
      switch (value) {
      case InstanceFeatureName::TimedWaitAny:
        o << "InstanceFeatureName::TimedWaitAny";
        break;
      case InstanceFeatureName::ShaderSourceSPIRV:
        o << "InstanceFeatureName::ShaderSourceSPIRV";
        break;
      case InstanceFeatureName::MultipleDevicesPerAdapter:
        o << "InstanceFeatureName::MultipleDevicesPerAdapter";
        break;
          default:
            o << "InstanceFeatureName::" << std::showbase << std::hex << std::setfill('0') << std::setw(4) << static_cast<std::underlying_type<InstanceFeatureName>::type>(value);
      }
      return o;
  }
  template <typename CharT, typename Traits>
  std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& o, LoadOp value) {
      switch (value) {
      case LoadOp::Undefined:
        o << "LoadOp::Undefined";
        break;
      case LoadOp::Load:
        o << "LoadOp::Load";
        break;
      case LoadOp::Clear:
        o << "LoadOp::Clear";
        break;
      case LoadOp::ExpandResolveTexture:
        o << "LoadOp::ExpandResolveTexture";
        break;
          default:
            o << "LoadOp::" << std::showbase << std::hex << std::setfill('0') << std::setw(4) << static_cast<std::underlying_type<LoadOp>::type>(value);
      }
      return o;
  }
  template <typename CharT, typename Traits>
  std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& o, LoggingType value) {
      switch (value) {
      case LoggingType::Verbose:
        o << "LoggingType::Verbose";
        break;
      case LoggingType::Info:
        o << "LoggingType::Info";
        break;
      case LoggingType::Warning:
        o << "LoggingType::Warning";
        break;
      case LoggingType::Error:
        o << "LoggingType::Error";
        break;
          default:
            o << "LoggingType::" << std::showbase << std::hex << std::setfill('0') << std::setw(4) << static_cast<std::underlying_type<LoggingType>::type>(value);
      }
      return o;
  }
  template <typename CharT, typename Traits>
  std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& o, MapAsyncStatus value) {
      switch (value) {
      case MapAsyncStatus::Success:
        o << "MapAsyncStatus::Success";
        break;
      case MapAsyncStatus::CallbackCancelled:
        o << "MapAsyncStatus::CallbackCancelled";
        break;
      case MapAsyncStatus::Error:
        o << "MapAsyncStatus::Error";
        break;
      case MapAsyncStatus::Aborted:
        o << "MapAsyncStatus::Aborted";
        break;
          default:
            o << "MapAsyncStatus::" << std::showbase << std::hex << std::setfill('0') << std::setw(4) << static_cast<std::underlying_type<MapAsyncStatus>::type>(value);
      }
      return o;
  }
  template <typename CharT, typename Traits>
  std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& o, MipmapFilterMode value) {
      switch (value) {
      case MipmapFilterMode::Undefined:
        o << "MipmapFilterMode::Undefined";
        break;
      case MipmapFilterMode::Nearest:
        o << "MipmapFilterMode::Nearest";
        break;
      case MipmapFilterMode::Linear:
        o << "MipmapFilterMode::Linear";
        break;
          default:
            o << "MipmapFilterMode::" << std::showbase << std::hex << std::setfill('0') << std::setw(4) << static_cast<std::underlying_type<MipmapFilterMode>::type>(value);
      }
      return o;
  }
  template <typename CharT, typename Traits>
  std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& o, PopErrorScopeStatus value) {
      switch (value) {
      case PopErrorScopeStatus::Success:
        o << "PopErrorScopeStatus::Success";
        break;
      case PopErrorScopeStatus::CallbackCancelled:
        o << "PopErrorScopeStatus::CallbackCancelled";
        break;
      case PopErrorScopeStatus::Error:
        o << "PopErrorScopeStatus::Error";
        break;
          default:
            o << "PopErrorScopeStatus::" << std::showbase << std::hex << std::setfill('0') << std::setw(4) << static_cast<std::underlying_type<PopErrorScopeStatus>::type>(value);
      }
      return o;
  }
  template <typename CharT, typename Traits>
  std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& o, PowerPreference value) {
      switch (value) {
      case PowerPreference::Undefined:
        o << "PowerPreference::Undefined";
        break;
      case PowerPreference::LowPower:
        o << "PowerPreference::LowPower";
        break;
      case PowerPreference::HighPerformance:
        o << "PowerPreference::HighPerformance";
        break;
          default:
            o << "PowerPreference::" << std::showbase << std::hex << std::setfill('0') << std::setw(4) << static_cast<std::underlying_type<PowerPreference>::type>(value);
      }
      return o;
  }
  template <typename CharT, typename Traits>
  std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& o, PredefinedColorSpace value) {
      switch (value) {
      case PredefinedColorSpace::SRGB:
        o << "PredefinedColorSpace::SRGB";
        break;
      case PredefinedColorSpace::DisplayP3:
        o << "PredefinedColorSpace::DisplayP3";
        break;
      case PredefinedColorSpace::SRGBLinear:
        o << "PredefinedColorSpace::SRGBLinear";
        break;
      case PredefinedColorSpace::DisplayP3Linear:
        o << "PredefinedColorSpace::DisplayP3Linear";
        break;
      case PredefinedColorSpace::Rec2020Linear:
        o << "PredefinedColorSpace::Rec2020Linear";
        break;
          default:
            o << "PredefinedColorSpace::" << std::showbase << std::hex << std::setfill('0') << std::setw(4) << static_cast<std::underlying_type<PredefinedColorSpace>::type>(value);
      }
      return o;
  }
  template <typename CharT, typename Traits>
  std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& o, PresentMode value) {
      switch (value) {
      case PresentMode::Undefined:
        o << "PresentMode::Undefined";
        break;
      case PresentMode::Fifo:
        o << "PresentMode::Fifo";
        break;
      case PresentMode::FifoRelaxed:
        o << "PresentMode::FifoRelaxed";
        break;
      case PresentMode::Immediate:
        o << "PresentMode::Immediate";
        break;
      case PresentMode::Mailbox:
        o << "PresentMode::Mailbox";
        break;
          default:
            o << "PresentMode::" << std::showbase << std::hex << std::setfill('0') << std::setw(4) << static_cast<std::underlying_type<PresentMode>::type>(value);
      }
      return o;
  }
  template <typename CharT, typename Traits>
  std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& o, PrimitiveTopology value) {
      switch (value) {
      case PrimitiveTopology::Undefined:
        o << "PrimitiveTopology::Undefined";
        break;
      case PrimitiveTopology::PointList:
        o << "PrimitiveTopology::PointList";
        break;
      case PrimitiveTopology::LineList:
        o << "PrimitiveTopology::LineList";
        break;
      case PrimitiveTopology::LineStrip:
        o << "PrimitiveTopology::LineStrip";
        break;
      case PrimitiveTopology::TriangleList:
        o << "PrimitiveTopology::TriangleList";
        break;
      case PrimitiveTopology::TriangleStrip:
        o << "PrimitiveTopology::TriangleStrip";
        break;
          default:
            o << "PrimitiveTopology::" << std::showbase << std::hex << std::setfill('0') << std::setw(4) << static_cast<std::underlying_type<PrimitiveTopology>::type>(value);
      }
      return o;
  }
  template <typename CharT, typename Traits>
  std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& o, QueryType value) {
      switch (value) {
      case QueryType::Occlusion:
        o << "QueryType::Occlusion";
        break;
      case QueryType::Timestamp:
        o << "QueryType::Timestamp";
        break;
          default:
            o << "QueryType::" << std::showbase << std::hex << std::setfill('0') << std::setw(4) << static_cast<std::underlying_type<QueryType>::type>(value);
      }
      return o;
  }
  template <typename CharT, typename Traits>
  std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& o, QueueWorkDoneStatus value) {
      switch (value) {
      case QueueWorkDoneStatus::Success:
        o << "QueueWorkDoneStatus::Success";
        break;
      case QueueWorkDoneStatus::CallbackCancelled:
        o << "QueueWorkDoneStatus::CallbackCancelled";
        break;
      case QueueWorkDoneStatus::Error:
        o << "QueueWorkDoneStatus::Error";
        break;
          default:
            o << "QueueWorkDoneStatus::" << std::showbase << std::hex << std::setfill('0') << std::setw(4) << static_cast<std::underlying_type<QueueWorkDoneStatus>::type>(value);
      }
      return o;
  }
  template <typename CharT, typename Traits>
  std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& o, RequestAdapterStatus value) {
      switch (value) {
      case RequestAdapterStatus::Success:
        o << "RequestAdapterStatus::Success";
        break;
      case RequestAdapterStatus::CallbackCancelled:
        o << "RequestAdapterStatus::CallbackCancelled";
        break;
      case RequestAdapterStatus::Unavailable:
        o << "RequestAdapterStatus::Unavailable";
        break;
      case RequestAdapterStatus::Error:
        o << "RequestAdapterStatus::Error";
        break;
          default:
            o << "RequestAdapterStatus::" << std::showbase << std::hex << std::setfill('0') << std::setw(4) << static_cast<std::underlying_type<RequestAdapterStatus>::type>(value);
      }
      return o;
  }
  template <typename CharT, typename Traits>
  std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& o, RequestDeviceStatus value) {
      switch (value) {
      case RequestDeviceStatus::Success:
        o << "RequestDeviceStatus::Success";
        break;
      case RequestDeviceStatus::CallbackCancelled:
        o << "RequestDeviceStatus::CallbackCancelled";
        break;
      case RequestDeviceStatus::Error:
        o << "RequestDeviceStatus::Error";
        break;
          default:
            o << "RequestDeviceStatus::" << std::showbase << std::hex << std::setfill('0') << std::setw(4) << static_cast<std::underlying_type<RequestDeviceStatus>::type>(value);
      }
      return o;
  }
  template <typename CharT, typename Traits>
  std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& o, SamplerBindingType value) {
      switch (value) {
      case SamplerBindingType::BindingNotUsed:
        o << "SamplerBindingType::BindingNotUsed";
        break;
      case SamplerBindingType::Undefined:
        o << "SamplerBindingType::Undefined";
        break;
      case SamplerBindingType::Filtering:
        o << "SamplerBindingType::Filtering";
        break;
      case SamplerBindingType::NonFiltering:
        o << "SamplerBindingType::NonFiltering";
        break;
      case SamplerBindingType::Comparison:
        o << "SamplerBindingType::Comparison";
        break;
          default:
            o << "SamplerBindingType::" << std::showbase << std::hex << std::setfill('0') << std::setw(4) << static_cast<std::underlying_type<SamplerBindingType>::type>(value);
      }
      return o;
  }
  template <typename CharT, typename Traits>
  std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& o, SharedFenceType value) {
      switch (value) {
      case SharedFenceType::VkSemaphoreOpaqueFD:
        o << "SharedFenceType::VkSemaphoreOpaqueFD";
        break;
      case SharedFenceType::SyncFD:
        o << "SharedFenceType::SyncFD";
        break;
      case SharedFenceType::VkSemaphoreZirconHandle:
        o << "SharedFenceType::VkSemaphoreZirconHandle";
        break;
      case SharedFenceType::DXGISharedHandle:
        o << "SharedFenceType::DXGISharedHandle";
        break;
      case SharedFenceType::MTLSharedEvent:
        o << "SharedFenceType::MTLSharedEvent";
        break;
      case SharedFenceType::EGLSync:
        o << "SharedFenceType::EGLSync";
        break;
          default:
            o << "SharedFenceType::" << std::showbase << std::hex << std::setfill('0') << std::setw(4) << static_cast<std::underlying_type<SharedFenceType>::type>(value);
      }
      return o;
  }
  template <typename CharT, typename Traits>
  std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& o, Status value) {
      switch (value) {
      case Status::Success:
        o << "Status::Success";
        break;
      case Status::Error:
        o << "Status::Error";
        break;
          default:
            o << "Status::" << std::showbase << std::hex << std::setfill('0') << std::setw(4) << static_cast<std::underlying_type<Status>::type>(value);
      }
      return o;
  }
  template <typename CharT, typename Traits>
  std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& o, StencilOperation value) {
      switch (value) {
      case StencilOperation::Undefined:
        o << "StencilOperation::Undefined";
        break;
      case StencilOperation::Keep:
        o << "StencilOperation::Keep";
        break;
      case StencilOperation::Zero:
        o << "StencilOperation::Zero";
        break;
      case StencilOperation::Replace:
        o << "StencilOperation::Replace";
        break;
      case StencilOperation::Invert:
        o << "StencilOperation::Invert";
        break;
      case StencilOperation::IncrementClamp:
        o << "StencilOperation::IncrementClamp";
        break;
      case StencilOperation::DecrementClamp:
        o << "StencilOperation::DecrementClamp";
        break;
      case StencilOperation::IncrementWrap:
        o << "StencilOperation::IncrementWrap";
        break;
      case StencilOperation::DecrementWrap:
        o << "StencilOperation::DecrementWrap";
        break;
          default:
            o << "StencilOperation::" << std::showbase << std::hex << std::setfill('0') << std::setw(4) << static_cast<std::underlying_type<StencilOperation>::type>(value);
      }
      return o;
  }
  template <typename CharT, typename Traits>
  std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& o, StorageTextureAccess value) {
      switch (value) {
      case StorageTextureAccess::BindingNotUsed:
        o << "StorageTextureAccess::BindingNotUsed";
        break;
      case StorageTextureAccess::Undefined:
        o << "StorageTextureAccess::Undefined";
        break;
      case StorageTextureAccess::WriteOnly:
        o << "StorageTextureAccess::WriteOnly";
        break;
      case StorageTextureAccess::ReadOnly:
        o << "StorageTextureAccess::ReadOnly";
        break;
      case StorageTextureAccess::ReadWrite:
        o << "StorageTextureAccess::ReadWrite";
        break;
          default:
            o << "StorageTextureAccess::" << std::showbase << std::hex << std::setfill('0') << std::setw(4) << static_cast<std::underlying_type<StorageTextureAccess>::type>(value);
      }
      return o;
  }
  template <typename CharT, typename Traits>
  std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& o, StoreOp value) {
      switch (value) {
      case StoreOp::Undefined:
        o << "StoreOp::Undefined";
        break;
      case StoreOp::Store:
        o << "StoreOp::Store";
        break;
      case StoreOp::Discard:
        o << "StoreOp::Discard";
        break;
          default:
            o << "StoreOp::" << std::showbase << std::hex << std::setfill('0') << std::setw(4) << static_cast<std::underlying_type<StoreOp>::type>(value);
      }
      return o;
  }
  template <typename CharT, typename Traits>
  std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& o, SType value) {
      switch (value) {
      case SType::ShaderSourceSPIRV:
        o << "SType::ShaderSourceSPIRV";
        break;
      case SType::ShaderSourceWGSL:
        o << "SType::ShaderSourceWGSL";
        break;
      case SType::RenderPassMaxDrawCount:
        o << "SType::RenderPassMaxDrawCount";
        break;
      case SType::SurfaceSourceMetalLayer:
        o << "SType::SurfaceSourceMetalLayer";
        break;
      case SType::SurfaceSourceWindowsHWND:
        o << "SType::SurfaceSourceWindowsHWND";
        break;
      case SType::SurfaceSourceXlibWindow:
        o << "SType::SurfaceSourceXlibWindow";
        break;
      case SType::SurfaceSourceWaylandSurface:
        o << "SType::SurfaceSourceWaylandSurface";
        break;
      case SType::SurfaceSourceAndroidNativeWindow:
        o << "SType::SurfaceSourceAndroidNativeWindow";
        break;
      case SType::SurfaceSourceXCBWindow:
        o << "SType::SurfaceSourceXCBWindow";
        break;
      case SType::SurfaceColorManagement:
        o << "SType::SurfaceColorManagement";
        break;
      case SType::RequestAdapterWebXROptions:
        o << "SType::RequestAdapterWebXROptions";
        break;
      case SType::TextureComponentSwizzleDescriptor:
        o << "SType::TextureComponentSwizzleDescriptor";
        break;
      case SType::ExternalTextureBindingLayout:
        o << "SType::ExternalTextureBindingLayout";
        break;
      case SType::ExternalTextureBindingEntry:
        o << "SType::ExternalTextureBindingEntry";
        break;
      case SType::CompatibilityModeLimits:
        o << "SType::CompatibilityModeLimits";
        break;
      case SType::TextureBindingViewDimension:
        o << "SType::TextureBindingViewDimension";
        break;
      case SType::SurfaceDescriptorFromWindowsCoreWindow:
        o << "SType::SurfaceDescriptorFromWindowsCoreWindow";
        break;
      case SType::SurfaceDescriptorFromWindowsUWPSwapChainPanel:
        o << "SType::SurfaceDescriptorFromWindowsUWPSwapChainPanel";
        break;
      case SType::DawnTextureInternalUsageDescriptor:
        o << "SType::DawnTextureInternalUsageDescriptor";
        break;
      case SType::DawnEncoderInternalUsageDescriptor:
        o << "SType::DawnEncoderInternalUsageDescriptor";
        break;
      case SType::DawnInstanceDescriptor:
        o << "SType::DawnInstanceDescriptor";
        break;
      case SType::DawnCacheDeviceDescriptor:
        o << "SType::DawnCacheDeviceDescriptor";
        break;
      case SType::DawnAdapterPropertiesPowerPreference:
        o << "SType::DawnAdapterPropertiesPowerPreference";
        break;
      case SType::DawnBufferDescriptorErrorInfoFromWireClient:
        o << "SType::DawnBufferDescriptorErrorInfoFromWireClient";
        break;
      case SType::DawnTogglesDescriptor:
        o << "SType::DawnTogglesDescriptor";
        break;
      case SType::DawnShaderModuleSPIRVOptionsDescriptor:
        o << "SType::DawnShaderModuleSPIRVOptionsDescriptor";
        break;
      case SType::RequestAdapterOptionsLUID:
        o << "SType::RequestAdapterOptionsLUID";
        break;
      case SType::RequestAdapterOptionsGetGLProc:
        o << "SType::RequestAdapterOptionsGetGLProc";
        break;
      case SType::RequestAdapterOptionsD3D11Device:
        o << "SType::RequestAdapterOptionsD3D11Device";
        break;
      case SType::DawnRenderPassSampleCount:
        o << "SType::DawnRenderPassSampleCount";
        break;
      case SType::RenderPassPixelLocalStorage:
        o << "SType::RenderPassPixelLocalStorage";
        break;
      case SType::PipelineLayoutPixelLocalStorage:
        o << "SType::PipelineLayoutPixelLocalStorage";
        break;
      case SType::BufferHostMappedPointer:
        o << "SType::BufferHostMappedPointer";
        break;
      case SType::AdapterPropertiesMemoryHeaps:
        o << "SType::AdapterPropertiesMemoryHeaps";
        break;
      case SType::AdapterPropertiesD3D:
        o << "SType::AdapterPropertiesD3D";
        break;
      case SType::AdapterPropertiesVk:
        o << "SType::AdapterPropertiesVk";
        break;
      case SType::DawnWireWGSLControl:
        o << "SType::DawnWireWGSLControl";
        break;
      case SType::DawnWGSLBlocklist:
        o << "SType::DawnWGSLBlocklist";
        break;
      case SType::DawnDrmFormatCapabilities:
        o << "SType::DawnDrmFormatCapabilities";
        break;
      case SType::ShaderModuleCompilationOptions:
        o << "SType::ShaderModuleCompilationOptions";
        break;
      case SType::ColorTargetStateExpandResolveTextureDawn:
        o << "SType::ColorTargetStateExpandResolveTextureDawn";
        break;
      case SType::RenderPassRenderAreaRect:
        o << "SType::RenderPassRenderAreaRect";
        break;
      case SType::SharedTextureMemoryVkDedicatedAllocationDescriptor:
        o << "SType::SharedTextureMemoryVkDedicatedAllocationDescriptor";
        break;
      case SType::SharedTextureMemoryAHardwareBufferDescriptor:
        o << "SType::SharedTextureMemoryAHardwareBufferDescriptor";
        break;
      case SType::SharedTextureMemoryDmaBufDescriptor:
        o << "SType::SharedTextureMemoryDmaBufDescriptor";
        break;
      case SType::SharedTextureMemoryOpaqueFDDescriptor:
        o << "SType::SharedTextureMemoryOpaqueFDDescriptor";
        break;
      case SType::SharedTextureMemoryZirconHandleDescriptor:
        o << "SType::SharedTextureMemoryZirconHandleDescriptor";
        break;
      case SType::SharedTextureMemoryDXGISharedHandleDescriptor:
        o << "SType::SharedTextureMemoryDXGISharedHandleDescriptor";
        break;
      case SType::SharedTextureMemoryD3D11Texture2DDescriptor:
        o << "SType::SharedTextureMemoryD3D11Texture2DDescriptor";
        break;
      case SType::SharedTextureMemoryIOSurfaceDescriptor:
        o << "SType::SharedTextureMemoryIOSurfaceDescriptor";
        break;
      case SType::SharedTextureMemoryEGLImageDescriptor:
        o << "SType::SharedTextureMemoryEGLImageDescriptor";
        break;
      case SType::SharedTextureMemoryInitializedBeginState:
        o << "SType::SharedTextureMemoryInitializedBeginState";
        break;
      case SType::SharedTextureMemoryInitializedEndState:
        o << "SType::SharedTextureMemoryInitializedEndState";
        break;
      case SType::SharedTextureMemoryVkImageLayoutBeginState:
        o << "SType::SharedTextureMemoryVkImageLayoutBeginState";
        break;
      case SType::SharedTextureMemoryVkImageLayoutEndState:
        o << "SType::SharedTextureMemoryVkImageLayoutEndState";
        break;
      case SType::SharedTextureMemoryD3DSwapchainBeginState:
        o << "SType::SharedTextureMemoryD3DSwapchainBeginState";
        break;
      case SType::SharedFenceVkSemaphoreOpaqueFDDescriptor:
        o << "SType::SharedFenceVkSemaphoreOpaqueFDDescriptor";
        break;
      case SType::SharedFenceVkSemaphoreOpaqueFDExportInfo:
        o << "SType::SharedFenceVkSemaphoreOpaqueFDExportInfo";
        break;
      case SType::SharedFenceSyncFDDescriptor:
        o << "SType::SharedFenceSyncFDDescriptor";
        break;
      case SType::SharedFenceSyncFDExportInfo:
        o << "SType::SharedFenceSyncFDExportInfo";
        break;
      case SType::SharedFenceVkSemaphoreZirconHandleDescriptor:
        o << "SType::SharedFenceVkSemaphoreZirconHandleDescriptor";
        break;
      case SType::SharedFenceVkSemaphoreZirconHandleExportInfo:
        o << "SType::SharedFenceVkSemaphoreZirconHandleExportInfo";
        break;
      case SType::SharedFenceDXGISharedHandleDescriptor:
        o << "SType::SharedFenceDXGISharedHandleDescriptor";
        break;
      case SType::SharedFenceDXGISharedHandleExportInfo:
        o << "SType::SharedFenceDXGISharedHandleExportInfo";
        break;
      case SType::SharedFenceMTLSharedEventDescriptor:
        o << "SType::SharedFenceMTLSharedEventDescriptor";
        break;
      case SType::SharedFenceMTLSharedEventExportInfo:
        o << "SType::SharedFenceMTLSharedEventExportInfo";
        break;
      case SType::SharedBufferMemoryD3D12ResourceDescriptor:
        o << "SType::SharedBufferMemoryD3D12ResourceDescriptor";
        break;
      case SType::StaticSamplerBindingLayout:
        o << "SType::StaticSamplerBindingLayout";
        break;
      case SType::YCbCrVkDescriptor:
        o << "SType::YCbCrVkDescriptor";
        break;
      case SType::SharedTextureMemoryAHardwareBufferProperties:
        o << "SType::SharedTextureMemoryAHardwareBufferProperties";
        break;
      case SType::AHardwareBufferProperties:
        o << "SType::AHardwareBufferProperties";
        break;
      case SType::DawnTexelCopyBufferRowAlignmentLimits:
        o << "SType::DawnTexelCopyBufferRowAlignmentLimits";
        break;
      case SType::AdapterPropertiesSubgroupMatrixConfigs:
        o << "SType::AdapterPropertiesSubgroupMatrixConfigs";
        break;
      case SType::SharedFenceEGLSyncDescriptor:
        o << "SType::SharedFenceEGLSyncDescriptor";
        break;
      case SType::SharedFenceEGLSyncExportInfo:
        o << "SType::SharedFenceEGLSyncExportInfo";
        break;
      case SType::DawnInjectedInvalidSType:
        o << "SType::DawnInjectedInvalidSType";
        break;
      case SType::DawnCompilationMessageUtf16:
        o << "SType::DawnCompilationMessageUtf16";
        break;
      case SType::DawnFakeBufferOOMForTesting:
        o << "SType::DawnFakeBufferOOMForTesting";
        break;
      case SType::SurfaceDescriptorFromWindowsWinUISwapChainPanel:
        o << "SType::SurfaceDescriptorFromWindowsWinUISwapChainPanel";
        break;
      case SType::DawnDeviceAllocatorControl:
        o << "SType::DawnDeviceAllocatorControl";
        break;
      case SType::DawnHostMappedPointerLimits:
        o << "SType::DawnHostMappedPointerLimits";
        break;
      case SType::RenderPassDescriptorResolveRect:
        o << "SType::RenderPassDescriptorResolveRect";
        break;
      case SType::RequestAdapterWebGPUBackendOptions:
        o << "SType::RequestAdapterWebGPUBackendOptions";
        break;
      case SType::DawnFakeDeviceInitializeErrorForTesting:
        o << "SType::DawnFakeDeviceInitializeErrorForTesting";
        break;
      case SType::SharedTextureMemoryD3D11BeginState:
        o << "SType::SharedTextureMemoryD3D11BeginState";
        break;
      case SType::DawnConsumeAdapterDescriptor:
        o << "SType::DawnConsumeAdapterDescriptor";
        break;
      case SType::TexelBufferBindingEntry:
        o << "SType::TexelBufferBindingEntry";
        break;
      case SType::TexelBufferBindingLayout:
        o << "SType::TexelBufferBindingLayout";
        break;
      case SType::SharedTextureMemoryMetalEndAccessState:
        o << "SType::SharedTextureMemoryMetalEndAccessState";
        break;
      case SType::AdapterPropertiesWGPU:
        o << "SType::AdapterPropertiesWGPU";
        break;
      case SType::SharedBufferMemoryFromWindowsHandleDescriptor:
        o << "SType::SharedBufferMemoryFromWindowsHandleDescriptor";
        break;
      case SType::SharedTextureMemoryD3D12ResourceDescriptor:
        o << "SType::SharedTextureMemoryD3D12ResourceDescriptor";
        break;
      case SType::RequestAdapterOptionsAngleVirtualizationGroup:
        o << "SType::RequestAdapterOptionsAngleVirtualizationGroup";
        break;
      case SType::PipelineLayoutResourceTable:
        o << "SType::PipelineLayoutResourceTable";
        break;
      case SType::AdapterPropertiesDrm:
        o << "SType::AdapterPropertiesDrm";
        break;
          default:
            o << "SType::" << std::showbase << std::hex << std::setfill('0') << std::setw(4) << static_cast<std::underlying_type<SType>::type>(value);
      }
      return o;
  }
  template <typename CharT, typename Traits>
  std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& o, SubgroupMatrixComponentType value) {
      switch (value) {
      case SubgroupMatrixComponentType::F32:
        o << "SubgroupMatrixComponentType::F32";
        break;
      case SubgroupMatrixComponentType::F16:
        o << "SubgroupMatrixComponentType::F16";
        break;
      case SubgroupMatrixComponentType::U32:
        o << "SubgroupMatrixComponentType::U32";
        break;
      case SubgroupMatrixComponentType::I32:
        o << "SubgroupMatrixComponentType::I32";
        break;
      case SubgroupMatrixComponentType::U8:
        o << "SubgroupMatrixComponentType::U8";
        break;
      case SubgroupMatrixComponentType::I8:
        o << "SubgroupMatrixComponentType::I8";
        break;
          default:
            o << "SubgroupMatrixComponentType::" << std::showbase << std::hex << std::setfill('0') << std::setw(4) << static_cast<std::underlying_type<SubgroupMatrixComponentType>::type>(value);
      }
      return o;
  }
  template <typename CharT, typename Traits>
  std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& o, SurfaceGetCurrentTextureStatus value) {
      switch (value) {
      case SurfaceGetCurrentTextureStatus::SuccessOptimal:
        o << "SurfaceGetCurrentTextureStatus::SuccessOptimal";
        break;
      case SurfaceGetCurrentTextureStatus::SuccessSuboptimal:
        o << "SurfaceGetCurrentTextureStatus::SuccessSuboptimal";
        break;
      case SurfaceGetCurrentTextureStatus::Timeout:
        o << "SurfaceGetCurrentTextureStatus::Timeout";
        break;
      case SurfaceGetCurrentTextureStatus::Outdated:
        o << "SurfaceGetCurrentTextureStatus::Outdated";
        break;
      case SurfaceGetCurrentTextureStatus::Lost:
        o << "SurfaceGetCurrentTextureStatus::Lost";
        break;
      case SurfaceGetCurrentTextureStatus::Error:
        o << "SurfaceGetCurrentTextureStatus::Error";
        break;
          default:
            o << "SurfaceGetCurrentTextureStatus::" << std::showbase << std::hex << std::setfill('0') << std::setw(4) << static_cast<std::underlying_type<SurfaceGetCurrentTextureStatus>::type>(value);
      }
      return o;
  }
  template <typename CharT, typename Traits>
  std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& o, TexelBufferAccess value) {
      switch (value) {
      case TexelBufferAccess::Undefined:
        o << "TexelBufferAccess::Undefined";
        break;
      case TexelBufferAccess::ReadOnly:
        o << "TexelBufferAccess::ReadOnly";
        break;
      case TexelBufferAccess::ReadWrite:
        o << "TexelBufferAccess::ReadWrite";
        break;
          default:
            o << "TexelBufferAccess::" << std::showbase << std::hex << std::setfill('0') << std::setw(4) << static_cast<std::underlying_type<TexelBufferAccess>::type>(value);
      }
      return o;
  }
  template <typename CharT, typename Traits>
  std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& o, TextureAspect value) {
      switch (value) {
      case TextureAspect::Undefined:
        o << "TextureAspect::Undefined";
        break;
      case TextureAspect::All:
        o << "TextureAspect::All";
        break;
      case TextureAspect::StencilOnly:
        o << "TextureAspect::StencilOnly";
        break;
      case TextureAspect::DepthOnly:
        o << "TextureAspect::DepthOnly";
        break;
      case TextureAspect::Plane0Only:
        o << "TextureAspect::Plane0Only";
        break;
      case TextureAspect::Plane1Only:
        o << "TextureAspect::Plane1Only";
        break;
      case TextureAspect::Plane2Only:
        o << "TextureAspect::Plane2Only";
        break;
          default:
            o << "TextureAspect::" << std::showbase << std::hex << std::setfill('0') << std::setw(4) << static_cast<std::underlying_type<TextureAspect>::type>(value);
      }
      return o;
  }
  template <typename CharT, typename Traits>
  std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& o, TextureDimension value) {
      switch (value) {
      case TextureDimension::Undefined:
        o << "TextureDimension::Undefined";
        break;
      case TextureDimension::e1D:
        o << "TextureDimension::e1D";
        break;
      case TextureDimension::e2D:
        o << "TextureDimension::e2D";
        break;
      case TextureDimension::e3D:
        o << "TextureDimension::e3D";
        break;
          default:
            o << "TextureDimension::" << std::showbase << std::hex << std::setfill('0') << std::setw(4) << static_cast<std::underlying_type<TextureDimension>::type>(value);
      }
      return o;
  }
  template <typename CharT, typename Traits>
  std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& o, TextureFormat value) {
      switch (value) {
      case TextureFormat::Undefined:
        o << "TextureFormat::Undefined";
        break;
      case TextureFormat::R8Unorm:
        o << "TextureFormat::R8Unorm";
        break;
      case TextureFormat::R8Snorm:
        o << "TextureFormat::R8Snorm";
        break;
      case TextureFormat::R8Uint:
        o << "TextureFormat::R8Uint";
        break;
      case TextureFormat::R8Sint:
        o << "TextureFormat::R8Sint";
        break;
      case TextureFormat::R16Unorm:
        o << "TextureFormat::R16Unorm";
        break;
      case TextureFormat::R16Snorm:
        o << "TextureFormat::R16Snorm";
        break;
      case TextureFormat::R16Uint:
        o << "TextureFormat::R16Uint";
        break;
      case TextureFormat::R16Sint:
        o << "TextureFormat::R16Sint";
        break;
      case TextureFormat::R16Float:
        o << "TextureFormat::R16Float";
        break;
      case TextureFormat::RG8Unorm:
        o << "TextureFormat::RG8Unorm";
        break;
      case TextureFormat::RG8Snorm:
        o << "TextureFormat::RG8Snorm";
        break;
      case TextureFormat::RG8Uint:
        o << "TextureFormat::RG8Uint";
        break;
      case TextureFormat::RG8Sint:
        o << "TextureFormat::RG8Sint";
        break;
      case TextureFormat::R32Float:
        o << "TextureFormat::R32Float";
        break;
      case TextureFormat::R32Uint:
        o << "TextureFormat::R32Uint";
        break;
      case TextureFormat::R32Sint:
        o << "TextureFormat::R32Sint";
        break;
      case TextureFormat::RG16Unorm:
        o << "TextureFormat::RG16Unorm";
        break;
      case TextureFormat::RG16Snorm:
        o << "TextureFormat::RG16Snorm";
        break;
      case TextureFormat::RG16Uint:
        o << "TextureFormat::RG16Uint";
        break;
      case TextureFormat::RG16Sint:
        o << "TextureFormat::RG16Sint";
        break;
      case TextureFormat::RG16Float:
        o << "TextureFormat::RG16Float";
        break;
      case TextureFormat::RGBA8Unorm:
        o << "TextureFormat::RGBA8Unorm";
        break;
      case TextureFormat::RGBA8UnormSrgb:
        o << "TextureFormat::RGBA8UnormSrgb";
        break;
      case TextureFormat::RGBA8Snorm:
        o << "TextureFormat::RGBA8Snorm";
        break;
      case TextureFormat::RGBA8Uint:
        o << "TextureFormat::RGBA8Uint";
        break;
      case TextureFormat::RGBA8Sint:
        o << "TextureFormat::RGBA8Sint";
        break;
      case TextureFormat::BGRA8Unorm:
        o << "TextureFormat::BGRA8Unorm";
        break;
      case TextureFormat::BGRA8UnormSrgb:
        o << "TextureFormat::BGRA8UnormSrgb";
        break;
      case TextureFormat::RGB10A2Uint:
        o << "TextureFormat::RGB10A2Uint";
        break;
      case TextureFormat::RGB10A2Unorm:
        o << "TextureFormat::RGB10A2Unorm";
        break;
      case TextureFormat::RG11B10Ufloat:
        o << "TextureFormat::RG11B10Ufloat";
        break;
      case TextureFormat::RGB9E5Ufloat:
        o << "TextureFormat::RGB9E5Ufloat";
        break;
      case TextureFormat::RG32Float:
        o << "TextureFormat::RG32Float";
        break;
      case TextureFormat::RG32Uint:
        o << "TextureFormat::RG32Uint";
        break;
      case TextureFormat::RG32Sint:
        o << "TextureFormat::RG32Sint";
        break;
      case TextureFormat::RGBA16Unorm:
        o << "TextureFormat::RGBA16Unorm";
        break;
      case TextureFormat::RGBA16Snorm:
        o << "TextureFormat::RGBA16Snorm";
        break;
      case TextureFormat::RGBA16Uint:
        o << "TextureFormat::RGBA16Uint";
        break;
      case TextureFormat::RGBA16Sint:
        o << "TextureFormat::RGBA16Sint";
        break;
      case TextureFormat::RGBA16Float:
        o << "TextureFormat::RGBA16Float";
        break;
      case TextureFormat::RGBA32Float:
        o << "TextureFormat::RGBA32Float";
        break;
      case TextureFormat::RGBA32Uint:
        o << "TextureFormat::RGBA32Uint";
        break;
      case TextureFormat::RGBA32Sint:
        o << "TextureFormat::RGBA32Sint";
        break;
      case TextureFormat::Stencil8:
        o << "TextureFormat::Stencil8";
        break;
      case TextureFormat::Depth16Unorm:
        o << "TextureFormat::Depth16Unorm";
        break;
      case TextureFormat::Depth24Plus:
        o << "TextureFormat::Depth24Plus";
        break;
      case TextureFormat::Depth24PlusStencil8:
        o << "TextureFormat::Depth24PlusStencil8";
        break;
      case TextureFormat::Depth32Float:
        o << "TextureFormat::Depth32Float";
        break;
      case TextureFormat::Depth32FloatStencil8:
        o << "TextureFormat::Depth32FloatStencil8";
        break;
      case TextureFormat::BC1RGBAUnorm:
        o << "TextureFormat::BC1RGBAUnorm";
        break;
      case TextureFormat::BC1RGBAUnormSrgb:
        o << "TextureFormat::BC1RGBAUnormSrgb";
        break;
      case TextureFormat::BC2RGBAUnorm:
        o << "TextureFormat::BC2RGBAUnorm";
        break;
      case TextureFormat::BC2RGBAUnormSrgb:
        o << "TextureFormat::BC2RGBAUnormSrgb";
        break;
      case TextureFormat::BC3RGBAUnorm:
        o << "TextureFormat::BC3RGBAUnorm";
        break;
      case TextureFormat::BC3RGBAUnormSrgb:
        o << "TextureFormat::BC3RGBAUnormSrgb";
        break;
      case TextureFormat::BC4RUnorm:
        o << "TextureFormat::BC4RUnorm";
        break;
      case TextureFormat::BC4RSnorm:
        o << "TextureFormat::BC4RSnorm";
        break;
      case TextureFormat::BC5RGUnorm:
        o << "TextureFormat::BC5RGUnorm";
        break;
      case TextureFormat::BC5RGSnorm:
        o << "TextureFormat::BC5RGSnorm";
        break;
      case TextureFormat::BC6HRGBUfloat:
        o << "TextureFormat::BC6HRGBUfloat";
        break;
      case TextureFormat::BC6HRGBFloat:
        o << "TextureFormat::BC6HRGBFloat";
        break;
      case TextureFormat::BC7RGBAUnorm:
        o << "TextureFormat::BC7RGBAUnorm";
        break;
      case TextureFormat::BC7RGBAUnormSrgb:
        o << "TextureFormat::BC7RGBAUnormSrgb";
        break;
      case TextureFormat::ETC2RGB8Unorm:
        o << "TextureFormat::ETC2RGB8Unorm";
        break;
      case TextureFormat::ETC2RGB8UnormSrgb:
        o << "TextureFormat::ETC2RGB8UnormSrgb";
        break;
      case TextureFormat::ETC2RGB8A1Unorm:
        o << "TextureFormat::ETC2RGB8A1Unorm";
        break;
      case TextureFormat::ETC2RGB8A1UnormSrgb:
        o << "TextureFormat::ETC2RGB8A1UnormSrgb";
        break;
      case TextureFormat::ETC2RGBA8Unorm:
        o << "TextureFormat::ETC2RGBA8Unorm";
        break;
      case TextureFormat::ETC2RGBA8UnormSrgb:
        o << "TextureFormat::ETC2RGBA8UnormSrgb";
        break;
      case TextureFormat::EACR11Unorm:
        o << "TextureFormat::EACR11Unorm";
        break;
      case TextureFormat::EACR11Snorm:
        o << "TextureFormat::EACR11Snorm";
        break;
      case TextureFormat::EACRG11Unorm:
        o << "TextureFormat::EACRG11Unorm";
        break;
      case TextureFormat::EACRG11Snorm:
        o << "TextureFormat::EACRG11Snorm";
        break;
      case TextureFormat::ASTC4x4Unorm:
        o << "TextureFormat::ASTC4x4Unorm";
        break;
      case TextureFormat::ASTC4x4UnormSrgb:
        o << "TextureFormat::ASTC4x4UnormSrgb";
        break;
      case TextureFormat::ASTC5x4Unorm:
        o << "TextureFormat::ASTC5x4Unorm";
        break;
      case TextureFormat::ASTC5x4UnormSrgb:
        o << "TextureFormat::ASTC5x4UnormSrgb";
        break;
      case TextureFormat::ASTC5x5Unorm:
        o << "TextureFormat::ASTC5x5Unorm";
        break;
      case TextureFormat::ASTC5x5UnormSrgb:
        o << "TextureFormat::ASTC5x5UnormSrgb";
        break;
      case TextureFormat::ASTC6x5Unorm:
        o << "TextureFormat::ASTC6x5Unorm";
        break;
      case TextureFormat::ASTC6x5UnormSrgb:
        o << "TextureFormat::ASTC6x5UnormSrgb";
        break;
      case TextureFormat::ASTC6x6Unorm:
        o << "TextureFormat::ASTC6x6Unorm";
        break;
      case TextureFormat::ASTC6x6UnormSrgb:
        o << "TextureFormat::ASTC6x6UnormSrgb";
        break;
      case TextureFormat::ASTC8x5Unorm:
        o << "TextureFormat::ASTC8x5Unorm";
        break;
      case TextureFormat::ASTC8x5UnormSrgb:
        o << "TextureFormat::ASTC8x5UnormSrgb";
        break;
      case TextureFormat::ASTC8x6Unorm:
        o << "TextureFormat::ASTC8x6Unorm";
        break;
      case TextureFormat::ASTC8x6UnormSrgb:
        o << "TextureFormat::ASTC8x6UnormSrgb";
        break;
      case TextureFormat::ASTC8x8Unorm:
        o << "TextureFormat::ASTC8x8Unorm";
        break;
      case TextureFormat::ASTC8x8UnormSrgb:
        o << "TextureFormat::ASTC8x8UnormSrgb";
        break;
      case TextureFormat::ASTC10x5Unorm:
        o << "TextureFormat::ASTC10x5Unorm";
        break;
      case TextureFormat::ASTC10x5UnormSrgb:
        o << "TextureFormat::ASTC10x5UnormSrgb";
        break;
      case TextureFormat::ASTC10x6Unorm:
        o << "TextureFormat::ASTC10x6Unorm";
        break;
      case TextureFormat::ASTC10x6UnormSrgb:
        o << "TextureFormat::ASTC10x6UnormSrgb";
        break;
      case TextureFormat::ASTC10x8Unorm:
        o << "TextureFormat::ASTC10x8Unorm";
        break;
      case TextureFormat::ASTC10x8UnormSrgb:
        o << "TextureFormat::ASTC10x8UnormSrgb";
        break;
      case TextureFormat::ASTC10x10Unorm:
        o << "TextureFormat::ASTC10x10Unorm";
        break;
      case TextureFormat::ASTC10x10UnormSrgb:
        o << "TextureFormat::ASTC10x10UnormSrgb";
        break;
      case TextureFormat::ASTC12x10Unorm:
        o << "TextureFormat::ASTC12x10Unorm";
        break;
      case TextureFormat::ASTC12x10UnormSrgb:
        o << "TextureFormat::ASTC12x10UnormSrgb";
        break;
      case TextureFormat::ASTC12x12Unorm:
        o << "TextureFormat::ASTC12x12Unorm";
        break;
      case TextureFormat::ASTC12x12UnormSrgb:
        o << "TextureFormat::ASTC12x12UnormSrgb";
        break;
      case TextureFormat::R8BG8Biplanar420Unorm:
        o << "TextureFormat::R8BG8Biplanar420Unorm";
        break;
      case TextureFormat::R10X6BG10X6Biplanar420Unorm:
        o << "TextureFormat::R10X6BG10X6Biplanar420Unorm";
        break;
      case TextureFormat::R8BG8A8Triplanar420Unorm:
        o << "TextureFormat::R8BG8A8Triplanar420Unorm";
        break;
      case TextureFormat::R8BG8Biplanar422Unorm:
        o << "TextureFormat::R8BG8Biplanar422Unorm";
        break;
      case TextureFormat::R8BG8Biplanar444Unorm:
        o << "TextureFormat::R8BG8Biplanar444Unorm";
        break;
      case TextureFormat::R10X6BG10X6Biplanar422Unorm:
        o << "TextureFormat::R10X6BG10X6Biplanar422Unorm";
        break;
      case TextureFormat::R10X6BG10X6Biplanar444Unorm:
        o << "TextureFormat::R10X6BG10X6Biplanar444Unorm";
        break;
      case TextureFormat::OpaqueYCbCrAndroid:
        o << "TextureFormat::OpaqueYCbCrAndroid";
        break;
          default:
            o << "TextureFormat::" << std::showbase << std::hex << std::setfill('0') << std::setw(4) << static_cast<std::underlying_type<TextureFormat>::type>(value);
      }
      return o;
  }
  template <typename CharT, typename Traits>
  std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& o, TextureSampleType value) {
      switch (value) {
      case TextureSampleType::BindingNotUsed:
        o << "TextureSampleType::BindingNotUsed";
        break;
      case TextureSampleType::Undefined:
        o << "TextureSampleType::Undefined";
        break;
      case TextureSampleType::Float:
        o << "TextureSampleType::Float";
        break;
      case TextureSampleType::UnfilterableFloat:
        o << "TextureSampleType::UnfilterableFloat";
        break;
      case TextureSampleType::Depth:
        o << "TextureSampleType::Depth";
        break;
      case TextureSampleType::Sint:
        o << "TextureSampleType::Sint";
        break;
      case TextureSampleType::Uint:
        o << "TextureSampleType::Uint";
        break;
          default:
            o << "TextureSampleType::" << std::showbase << std::hex << std::setfill('0') << std::setw(4) << static_cast<std::underlying_type<TextureSampleType>::type>(value);
      }
      return o;
  }
  template <typename CharT, typename Traits>
  std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& o, TextureViewDimension value) {
      switch (value) {
      case TextureViewDimension::Undefined:
        o << "TextureViewDimension::Undefined";
        break;
      case TextureViewDimension::e1D:
        o << "TextureViewDimension::e1D";
        break;
      case TextureViewDimension::e2D:
        o << "TextureViewDimension::e2D";
        break;
      case TextureViewDimension::e2DArray:
        o << "TextureViewDimension::e2DArray";
        break;
      case TextureViewDimension::Cube:
        o << "TextureViewDimension::Cube";
        break;
      case TextureViewDimension::CubeArray:
        o << "TextureViewDimension::CubeArray";
        break;
      case TextureViewDimension::e3D:
        o << "TextureViewDimension::e3D";
        break;
          default:
            o << "TextureViewDimension::" << std::showbase << std::hex << std::setfill('0') << std::setw(4) << static_cast<std::underlying_type<TextureViewDimension>::type>(value);
      }
      return o;
  }
  template <typename CharT, typename Traits>
  std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& o, ToneMappingMode value) {
      switch (value) {
      case ToneMappingMode::Standard:
        o << "ToneMappingMode::Standard";
        break;
      case ToneMappingMode::Extended:
        o << "ToneMappingMode::Extended";
        break;
          default:
            o << "ToneMappingMode::" << std::showbase << std::hex << std::setfill('0') << std::setw(4) << static_cast<std::underlying_type<ToneMappingMode>::type>(value);
      }
      return o;
  }
  template <typename CharT, typename Traits>
  std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& o, VertexFormat value) {
      switch (value) {
      case VertexFormat::Uint8:
        o << "VertexFormat::Uint8";
        break;
      case VertexFormat::Uint8x2:
        o << "VertexFormat::Uint8x2";
        break;
      case VertexFormat::Uint8x4:
        o << "VertexFormat::Uint8x4";
        break;
      case VertexFormat::Sint8:
        o << "VertexFormat::Sint8";
        break;
      case VertexFormat::Sint8x2:
        o << "VertexFormat::Sint8x2";
        break;
      case VertexFormat::Sint8x4:
        o << "VertexFormat::Sint8x4";
        break;
      case VertexFormat::Unorm8:
        o << "VertexFormat::Unorm8";
        break;
      case VertexFormat::Unorm8x2:
        o << "VertexFormat::Unorm8x2";
        break;
      case VertexFormat::Unorm8x4:
        o << "VertexFormat::Unorm8x4";
        break;
      case VertexFormat::Snorm8:
        o << "VertexFormat::Snorm8";
        break;
      case VertexFormat::Snorm8x2:
        o << "VertexFormat::Snorm8x2";
        break;
      case VertexFormat::Snorm8x4:
        o << "VertexFormat::Snorm8x4";
        break;
      case VertexFormat::Uint16:
        o << "VertexFormat::Uint16";
        break;
      case VertexFormat::Uint16x2:
        o << "VertexFormat::Uint16x2";
        break;
      case VertexFormat::Uint16x4:
        o << "VertexFormat::Uint16x4";
        break;
      case VertexFormat::Sint16:
        o << "VertexFormat::Sint16";
        break;
      case VertexFormat::Sint16x2:
        o << "VertexFormat::Sint16x2";
        break;
      case VertexFormat::Sint16x4:
        o << "VertexFormat::Sint16x4";
        break;
      case VertexFormat::Unorm16:
        o << "VertexFormat::Unorm16";
        break;
      case VertexFormat::Unorm16x2:
        o << "VertexFormat::Unorm16x2";
        break;
      case VertexFormat::Unorm16x4:
        o << "VertexFormat::Unorm16x4";
        break;
      case VertexFormat::Snorm16:
        o << "VertexFormat::Snorm16";
        break;
      case VertexFormat::Snorm16x2:
        o << "VertexFormat::Snorm16x2";
        break;
      case VertexFormat::Snorm16x4:
        o << "VertexFormat::Snorm16x4";
        break;
      case VertexFormat::Float16:
        o << "VertexFormat::Float16";
        break;
      case VertexFormat::Float16x2:
        o << "VertexFormat::Float16x2";
        break;
      case VertexFormat::Float16x4:
        o << "VertexFormat::Float16x4";
        break;
      case VertexFormat::Float32:
        o << "VertexFormat::Float32";
        break;
      case VertexFormat::Float32x2:
        o << "VertexFormat::Float32x2";
        break;
      case VertexFormat::Float32x3:
        o << "VertexFormat::Float32x3";
        break;
      case VertexFormat::Float32x4:
        o << "VertexFormat::Float32x4";
        break;
      case VertexFormat::Uint32:
        o << "VertexFormat::Uint32";
        break;
      case VertexFormat::Uint32x2:
        o << "VertexFormat::Uint32x2";
        break;
      case VertexFormat::Uint32x3:
        o << "VertexFormat::Uint32x3";
        break;
      case VertexFormat::Uint32x4:
        o << "VertexFormat::Uint32x4";
        break;
      case VertexFormat::Sint32:
        o << "VertexFormat::Sint32";
        break;
      case VertexFormat::Sint32x2:
        o << "VertexFormat::Sint32x2";
        break;
      case VertexFormat::Sint32x3:
        o << "VertexFormat::Sint32x3";
        break;
      case VertexFormat::Sint32x4:
        o << "VertexFormat::Sint32x4";
        break;
      case VertexFormat::Unorm10_10_10_2:
        o << "VertexFormat::Unorm10_10_10_2";
        break;
      case VertexFormat::Unorm8x4BGRA:
        o << "VertexFormat::Unorm8x4BGRA";
        break;
          default:
            o << "VertexFormat::" << std::showbase << std::hex << std::setfill('0') << std::setw(4) << static_cast<std::underlying_type<VertexFormat>::type>(value);
      }
      return o;
  }
  template <typename CharT, typename Traits>
  std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& o, VertexStepMode value) {
      switch (value) {
      case VertexStepMode::Undefined:
        o << "VertexStepMode::Undefined";
        break;
      case VertexStepMode::Vertex:
        o << "VertexStepMode::Vertex";
        break;
      case VertexStepMode::Instance:
        o << "VertexStepMode::Instance";
        break;
          default:
            o << "VertexStepMode::" << std::showbase << std::hex << std::setfill('0') << std::setw(4) << static_cast<std::underlying_type<VertexStepMode>::type>(value);
      }
      return o;
  }
  template <typename CharT, typename Traits>
  std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& o, WaitStatus value) {
      switch (value) {
      case WaitStatus::Success:
        o << "WaitStatus::Success";
        break;
      case WaitStatus::TimedOut:
        o << "WaitStatus::TimedOut";
        break;
      case WaitStatus::Error:
        o << "WaitStatus::Error";
        break;
          default:
            o << "WaitStatus::" << std::showbase << std::hex << std::setfill('0') << std::setw(4) << static_cast<std::underlying_type<WaitStatus>::type>(value);
      }
      return o;
  }
  template <typename CharT, typename Traits>
  std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& o, WGSLLanguageFeatureName value) {
      switch (value) {
      case WGSLLanguageFeatureName::ReadonlyAndReadwriteStorageTextures:
        o << "WGSLLanguageFeatureName::ReadonlyAndReadwriteStorageTextures";
        break;
      case WGSLLanguageFeatureName::Packed4x8IntegerDotProduct:
        o << "WGSLLanguageFeatureName::Packed4x8IntegerDotProduct";
        break;
      case WGSLLanguageFeatureName::UnrestrictedPointerParameters:
        o << "WGSLLanguageFeatureName::UnrestrictedPointerParameters";
        break;
      case WGSLLanguageFeatureName::PointerCompositeAccess:
        o << "WGSLLanguageFeatureName::PointerCompositeAccess";
        break;
      case WGSLLanguageFeatureName::UniformBufferStandardLayout:
        o << "WGSLLanguageFeatureName::UniformBufferStandardLayout";
        break;
      case WGSLLanguageFeatureName::SubgroupId:
        o << "WGSLLanguageFeatureName::SubgroupId";
        break;
      case WGSLLanguageFeatureName::TextureAndSamplerLet:
        o << "WGSLLanguageFeatureName::TextureAndSamplerLet";
        break;
      case WGSLLanguageFeatureName::SubgroupUniformity:
        o << "WGSLLanguageFeatureName::SubgroupUniformity";
        break;
      case WGSLLanguageFeatureName::TextureFormatsTier1:
        o << "WGSLLanguageFeatureName::TextureFormatsTier1";
        break;
      case WGSLLanguageFeatureName::LinearIndexing:
        o << "WGSLLanguageFeatureName::LinearIndexing";
        break;
      case WGSLLanguageFeatureName::ImmediateAddressSpace:
        o << "WGSLLanguageFeatureName::ImmediateAddressSpace";
        break;
      case WGSLLanguageFeatureName::ChromiumTestingUnimplemented:
        o << "WGSLLanguageFeatureName::ChromiumTestingUnimplemented";
        break;
      case WGSLLanguageFeatureName::ChromiumTestingUnsafeExperimental:
        o << "WGSLLanguageFeatureName::ChromiumTestingUnsafeExperimental";
        break;
      case WGSLLanguageFeatureName::ChromiumTestingExperimental:
        o << "WGSLLanguageFeatureName::ChromiumTestingExperimental";
        break;
      case WGSLLanguageFeatureName::ChromiumTestingShippedWithKillswitch:
        o << "WGSLLanguageFeatureName::ChromiumTestingShippedWithKillswitch";
        break;
      case WGSLLanguageFeatureName::ChromiumTestingShipped:
        o << "WGSLLanguageFeatureName::ChromiumTestingShipped";
        break;
      case WGSLLanguageFeatureName::SizedBindingArray:
        o << "WGSLLanguageFeatureName::SizedBindingArray";
        break;
      case WGSLLanguageFeatureName::TexelBuffers:
        o << "WGSLLanguageFeatureName::TexelBuffers";
        break;
      case WGSLLanguageFeatureName::ChromiumPrint:
        o << "WGSLLanguageFeatureName::ChromiumPrint";
        break;
      case WGSLLanguageFeatureName::FragmentDepth:
        o << "WGSLLanguageFeatureName::FragmentDepth";
        break;
      case WGSLLanguageFeatureName::BufferView:
        o << "WGSLLanguageFeatureName::BufferView";
        break;
      case WGSLLanguageFeatureName::SwizzleAssignment:
        o << "WGSLLanguageFeatureName::SwizzleAssignment";
        break;
          default:
            o << "WGSLLanguageFeatureName::" << std::showbase << std::hex << std::setfill('0') << std::setw(4) << static_cast<std::underlying_type<WGSLLanguageFeatureName>::type>(value);
      }
      return o;
  }

  template <typename CharT, typename Traits>
  std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& o, BufferUsage value) {
    o << "BufferUsage::";
    if (!static_cast<bool>(value)) {
    // 0 is often explicitly declared as None.
    o << "None";
      return o;
    }

    bool moreThanOneBit = !HasZeroOrOneBits(value);
    if (moreThanOneBit) {
      o << "(";
    }

    bool first = true;
  if (value & BufferUsage::MapRead) {
    if (!first) {
      o << "|";
    }
    first = false;
    o << "MapRead";
    value &= ~BufferUsage::MapRead;
  }
  if (value & BufferUsage::MapWrite) {
    if (!first) {
      o << "|";
    }
    first = false;
    o << "MapWrite";
    value &= ~BufferUsage::MapWrite;
  }
  if (value & BufferUsage::CopySrc) {
    if (!first) {
      o << "|";
    }
    first = false;
    o << "CopySrc";
    value &= ~BufferUsage::CopySrc;
  }
  if (value & BufferUsage::CopyDst) {
    if (!first) {
      o << "|";
    }
    first = false;
    o << "CopyDst";
    value &= ~BufferUsage::CopyDst;
  }
  if (value & BufferUsage::Index) {
    if (!first) {
      o << "|";
    }
    first = false;
    o << "Index";
    value &= ~BufferUsage::Index;
  }
  if (value & BufferUsage::Vertex) {
    if (!first) {
      o << "|";
    }
    first = false;
    o << "Vertex";
    value &= ~BufferUsage::Vertex;
  }
  if (value & BufferUsage::Uniform) {
    if (!first) {
      o << "|";
    }
    first = false;
    o << "Uniform";
    value &= ~BufferUsage::Uniform;
  }
  if (value & BufferUsage::Storage) {
    if (!first) {
      o << "|";
    }
    first = false;
    o << "Storage";
    value &= ~BufferUsage::Storage;
  }
  if (value & BufferUsage::Indirect) {
    if (!first) {
      o << "|";
    }
    first = false;
    o << "Indirect";
    value &= ~BufferUsage::Indirect;
  }
  if (value & BufferUsage::QueryResolve) {
    if (!first) {
      o << "|";
    }
    first = false;
    o << "QueryResolve";
    value &= ~BufferUsage::QueryResolve;
  }
  if (value & BufferUsage::TexelBuffer) {
    if (!first) {
      o << "|";
    }
    first = false;
    o << "TexelBuffer";
    value &= ~BufferUsage::TexelBuffer;
  }

    if (static_cast<bool>(value)) {
      if (!first) {
        o << "|";
      }
      o << std::showbase << std::hex << std::setfill('0') << std::setw(4) << static_cast<std::underlying_type<BufferUsage>::type>(value);
    }

    if (moreThanOneBit) {
      o << ")";
    }
    return o;
  }
  template <typename CharT, typename Traits>
  std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& o, ColorWriteMask value) {
    o << "ColorWriteMask::";
    if (!static_cast<bool>(value)) {
    // 0 is often explicitly declared as None.
    o << "None";
      return o;
    }

    bool moreThanOneBit = !HasZeroOrOneBits(value);
    if (moreThanOneBit) {
      o << "(";
    }

    bool first = true;
  if (value & ColorWriteMask::Red) {
    if (!first) {
      o << "|";
    }
    first = false;
    o << "Red";
    value &= ~ColorWriteMask::Red;
  }
  if (value & ColorWriteMask::Green) {
    if (!first) {
      o << "|";
    }
    first = false;
    o << "Green";
    value &= ~ColorWriteMask::Green;
  }
  if (value & ColorWriteMask::Blue) {
    if (!first) {
      o << "|";
    }
    first = false;
    o << "Blue";
    value &= ~ColorWriteMask::Blue;
  }
  if (value & ColorWriteMask::Alpha) {
    if (!first) {
      o << "|";
    }
    first = false;
    o << "Alpha";
    value &= ~ColorWriteMask::Alpha;
  }
  if (value & ColorWriteMask::All) {
    if (!first) {
      o << "|";
    }
    first = false;
    o << "All";
    value &= ~ColorWriteMask::All;
  }

    if (static_cast<bool>(value)) {
      if (!first) {
        o << "|";
      }
      o << std::showbase << std::hex << std::setfill('0') << std::setw(4) << static_cast<std::underlying_type<ColorWriteMask>::type>(value);
    }

    if (moreThanOneBit) {
      o << ")";
    }
    return o;
  }
  template <typename CharT, typename Traits>
  std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& o, HeapProperty value) {
    o << "HeapProperty::";
    if (!static_cast<bool>(value)) {
    // 0 is often explicitly declared as None.
    o << "None";
      return o;
    }

    bool moreThanOneBit = !HasZeroOrOneBits(value);
    if (moreThanOneBit) {
      o << "(";
    }

    bool first = true;
  if (value & HeapProperty::DeviceLocal) {
    if (!first) {
      o << "|";
    }
    first = false;
    o << "DeviceLocal";
    value &= ~HeapProperty::DeviceLocal;
  }
  if (value & HeapProperty::HostVisible) {
    if (!first) {
      o << "|";
    }
    first = false;
    o << "HostVisible";
    value &= ~HeapProperty::HostVisible;
  }
  if (value & HeapProperty::HostCoherent) {
    if (!first) {
      o << "|";
    }
    first = false;
    o << "HostCoherent";
    value &= ~HeapProperty::HostCoherent;
  }
  if (value & HeapProperty::HostUncached) {
    if (!first) {
      o << "|";
    }
    first = false;
    o << "HostUncached";
    value &= ~HeapProperty::HostUncached;
  }
  if (value & HeapProperty::HostCached) {
    if (!first) {
      o << "|";
    }
    first = false;
    o << "HostCached";
    value &= ~HeapProperty::HostCached;
  }

    if (static_cast<bool>(value)) {
      if (!first) {
        o << "|";
      }
      o << std::showbase << std::hex << std::setfill('0') << std::setw(4) << static_cast<std::underlying_type<HeapProperty>::type>(value);
    }

    if (moreThanOneBit) {
      o << ")";
    }
    return o;
  }
  template <typename CharT, typename Traits>
  std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& o, MapMode value) {
    o << "MapMode::";
    if (!static_cast<bool>(value)) {
    // 0 is often explicitly declared as None.
    o << "None";
      return o;
    }

    bool moreThanOneBit = !HasZeroOrOneBits(value);
    if (moreThanOneBit) {
      o << "(";
    }

    bool first = true;
  if (value & MapMode::Read) {
    if (!first) {
      o << "|";
    }
    first = false;
    o << "Read";
    value &= ~MapMode::Read;
  }
  if (value & MapMode::Write) {
    if (!first) {
      o << "|";
    }
    first = false;
    o << "Write";
    value &= ~MapMode::Write;
  }

    if (static_cast<bool>(value)) {
      if (!first) {
        o << "|";
      }
      o << std::showbase << std::hex << std::setfill('0') << std::setw(4) << static_cast<std::underlying_type<MapMode>::type>(value);
    }

    if (moreThanOneBit) {
      o << ")";
    }
    return o;
  }
  template <typename CharT, typename Traits>
  std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& o, ShaderStage value) {
    o << "ShaderStage::";
    if (!static_cast<bool>(value)) {
    // 0 is often explicitly declared as None.
    o << "None";
      return o;
    }

    bool moreThanOneBit = !HasZeroOrOneBits(value);
    if (moreThanOneBit) {
      o << "(";
    }

    bool first = true;
  if (value & ShaderStage::Vertex) {
    if (!first) {
      o << "|";
    }
    first = false;
    o << "Vertex";
    value &= ~ShaderStage::Vertex;
  }
  if (value & ShaderStage::Fragment) {
    if (!first) {
      o << "|";
    }
    first = false;
    o << "Fragment";
    value &= ~ShaderStage::Fragment;
  }
  if (value & ShaderStage::Compute) {
    if (!first) {
      o << "|";
    }
    first = false;
    o << "Compute";
    value &= ~ShaderStage::Compute;
  }

    if (static_cast<bool>(value)) {
      if (!first) {
        o << "|";
      }
      o << std::showbase << std::hex << std::setfill('0') << std::setw(4) << static_cast<std::underlying_type<ShaderStage>::type>(value);
    }

    if (moreThanOneBit) {
      o << ")";
    }
    return o;
  }
  template <typename CharT, typename Traits>
  std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& o, TextureUsage value) {
    o << "TextureUsage::";
    if (!static_cast<bool>(value)) {
    // 0 is often explicitly declared as None.
    o << "None";
      return o;
    }

    bool moreThanOneBit = !HasZeroOrOneBits(value);
    if (moreThanOneBit) {
      o << "(";
    }

    bool first = true;
  if (value & TextureUsage::CopySrc) {
    if (!first) {
      o << "|";
    }
    first = false;
    o << "CopySrc";
    value &= ~TextureUsage::CopySrc;
  }
  if (value & TextureUsage::CopyDst) {
    if (!first) {
      o << "|";
    }
    first = false;
    o << "CopyDst";
    value &= ~TextureUsage::CopyDst;
  }
  if (value & TextureUsage::TextureBinding) {
    if (!first) {
      o << "|";
    }
    first = false;
    o << "TextureBinding";
    value &= ~TextureUsage::TextureBinding;
  }
  if (value & TextureUsage::StorageBinding) {
    if (!first) {
      o << "|";
    }
    first = false;
    o << "StorageBinding";
    value &= ~TextureUsage::StorageBinding;
  }
  if (value & TextureUsage::RenderAttachment) {
    if (!first) {
      o << "|";
    }
    first = false;
    o << "RenderAttachment";
    value &= ~TextureUsage::RenderAttachment;
  }
  if (value & TextureUsage::TransientAttachment) {
    if (!first) {
      o << "|";
    }
    first = false;
    o << "TransientAttachment";
    value &= ~TextureUsage::TransientAttachment;
  }
  if (value & TextureUsage::StorageAttachment) {
    if (!first) {
      o << "|";
    }
    first = false;
    o << "StorageAttachment";
    value &= ~TextureUsage::StorageAttachment;
  }

    if (static_cast<bool>(value)) {
      if (!first) {
        o << "|";
      }
      o << std::showbase << std::hex << std::setfill('0') << std::setw(4) << static_cast<std::underlying_type<TextureUsage>::type>(value);
    }

    if (moreThanOneBit) {
      o << ")";
    }
    return o;
  }

}  // namespace wgpu

namespace wgpu {

  template <typename CharT, typename Traits>
  std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& o, StringView value) {
      o << std::string_view(value);
      return o;
  }

}  // namespace wgpu

#endif // WEBGPU_CPP_PRINT_H_
