// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @file vtkWebGPUHandle.h
 * @brief Reference-counted ownership for WebGPU C API handles.
 *
 * WebGPU's C API hands out opaque, reference-counted handles (`WGPUBuffer`,
 * `WGPUTexture`, ...). Every handle type has a matching `wgpu<Type>AddRef` /
 * `wgpu<Type>Release` pair, but the raw handles carry no ownership semantics of
 * their own, so tracking those calls by hand is error prone.
 *
 * vtkWebGPU::Handle wraps a raw handle and issues the AddRef/Release calls on
 * copy, move and destruction, giving the same convenience the `wgpu::` C++
 * wrapper provided without requiring a generated C++ wrapper header (and
 * therefore without requiring C++20). A named handle implicitly converts back
 * to the raw handle, so a vtkWebGPU::Buffer can be passed directly to any C
 * API entry point. The conversion is deleted for temporaries, so a handle
 * cannot be dropped into a raw pointer field and released out from under it.
 *
 * Ownership is explicit at construction:
 * - `Acquire(raw)` adopts a handle the caller already owns a reference to (the
 *   usual case for the result of a `wgpuDeviceCreate*` call).
 * - `Reference(raw)` takes a new reference to a handle owned by someone else.
 *
 * Ownership in this module follows one rule, with one exception forced by VTK's
 * header layout:
 *
 * - Anything that owns a WebGPU object holds it in a `vtkWebGPU::Handle`. That
 *   covers every implementation file and every private header.
 * - A public header cannot include this one - it is not installed - so classes
 *   whose members are declared in a public header keep raw `WGPU*` members and
 *   release them through `vtkWebGPU::ReleaseAndNull`, never by calling
 *   `wgpu<Type>Release` directly. That keeps the "assigning nullptr leaks"
 *   footgun in one helper instead of at every call site.
 * - Raw handles obtained from an accessor are borrowed. Do not release them,
 *   and do not store them past the lifetime of the owner.
 *
 * `wgpu<Type>Destroy` is not a release: it frees the object's memory but leaves
 * the reference count alone, so a `Destroy` still needs its matching release.
 *
 * This is a private implementation header; it is not installed.
 */

#ifndef vtkWebGPUHandle_h
#define vtkWebGPUHandle_h

#include "vtkABINamespace.h" // for VTK_ABI_NAMESPACE_BEGIN
#include "vtk_wgpu.h"        // for the WebGPU C API

#include <cstddef> // for std::nullptr_t
#include <utility> // for std::swap, std::exchange

namespace vtkWebGPU
{
VTK_ABI_NAMESPACE_BEGIN

/**
 * Reference-counted owner of a WebGPU C API handle.
 *
 * @tparam T          the raw handle type, e.g. `WGPUBuffer`
 * @tparam Traits     a type with static `AddRef(T)` and `Release(T)` members
 */
template <typename T, typename Traits>
class Handle
{
public:
  Handle() = default;

  /**
   * A null handle. Unlike construction from a raw handle, nullptr carries no
   * ownership question, so this one is allowed to be implicit.
   */
  Handle(std::nullptr_t) {} // NOLINT(google-explicit-constructor)

  /**
   * Adopt `raw` without adding a reference. Use for handles returned by
   * `wgpu*Create*` entry points, which hand back an owned reference.
   */
  static Handle Acquire(T raw)
  {
    Handle handle;
    handle.Raw = raw;
    return handle;
  }

  /**
   * Take a new reference to `raw`, which stays owned by the caller.
   */
  static Handle Reference(T raw)
  {
    if (raw != nullptr)
    {
      Traits::AddRef(raw);
    }
    return Handle::Acquire(raw);
  }

  ~Handle() { this->Reset(); }

  Handle(const Handle& other)
    : Raw(other.Raw)
  {
    if (this->Raw != nullptr)
    {
      Traits::AddRef(this->Raw);
    }
  }

  Handle(Handle&& other) noexcept
    : Raw(std::exchange(other.Raw, nullptr))
  {
  }

  /**
   * Release whatever is held and become null.
   */
  Handle& operator=(std::nullptr_t)
  {
    this->Reset();
    return *this;
  }

  Handle& operator=(Handle other) noexcept
  {
    // Copy-and-swap: `other` is built by the copy or move constructor above, so
    // this handles self-assignment and both value categories in one operator.
    std::swap(this->Raw, other.Raw);
    return *this;
  }

  /**
   * Release the referenced object, if any, and become null.
   */
  void Reset()
  {
    if (T raw = std::exchange(this->Raw, nullptr))
    {
      Traits::Release(raw);
    }
  }

  /**
   * Relinquish ownership to the caller, who becomes responsible for releasing
   * the returned handle. This handle becomes null.
   */
  T Release() { return std::exchange(this->Raw, nullptr); }

  ///@{
  /**
   * Access the underlying raw handle without transferring ownership. The
   * implicit conversion lets a Handle be passed straight to the C API.
   */
  T Get() const { return this->Raw; }
  operator T() const& { return this->Raw; } // NOLINT(google-explicit-constructor)

  /**
   * Deleted so that a temporary cannot silently decay to a raw handle. Storing
   * the result of `Handle::Acquire(...)` into a raw field would release the
   * object at the end of the statement and leave the field dangling; bind the
   * handle to a named variable that outlives the use, or call `Get()`/`Release()`
   * to say explicitly which lifetime is intended.
   */
  operator T() const&& = delete;
  ///@}

  explicit operator bool() const { return this->Raw != nullptr; }

private:
  T Raw = nullptr;
};

template <typename T, typename Traits>
bool operator==(const Handle<T, Traits>& handle, std::nullptr_t)
{
  return handle.Get() == nullptr;
}
template <typename T, typename Traits>
bool operator!=(const Handle<T, Traits>& handle, std::nullptr_t)
{
  return handle.Get() != nullptr;
}
template <typename T, typename Traits>
bool operator==(std::nullptr_t, const Handle<T, Traits>& handle)
{
  return handle.Get() == nullptr;
}
template <typename T, typename Traits>
bool operator!=(std::nullptr_t, const Handle<T, Traits>& handle)
{
  return handle.Get() != nullptr;
}

/**
 * Release `raw` if it is non-null and set it to null.
 *
 * For the raw `WGPU*` members that live in public headers, which cannot include
 * this header and so cannot use Handle. Assigning nullptr to such a member does
 * not release it - the `wgpu::` wrapper members they replaced did that in their
 * destructor, and losing it is a silent leak.
 */
template <typename T, typename ReleaseFn>
void ReleaseAndNull(T& raw, ReleaseFn release)
{
  if (raw != nullptr)
  {
    release(raw);
    raw = nullptr;
  }
}

// The `wgpu*` names redirect to dispatch table entries, whose addresses are not
// constant expressions and so cannot be template arguments.
#define vtkWebGPUDeclareHandle(Name)                                                               \
  struct Name##Traits                                                                              \
  {                                                                                                \
    static void AddRef(WGPU##Name handle)                                                          \
    {                                                                                              \
      wgpu##Name##AddRef(handle);                                                                  \
    }                                                                                              \
    static void Release(WGPU##Name handle)                                                         \
    {                                                                                              \
      wgpu##Name##Release(handle);                                                                 \
    }                                                                                              \
  };                                                                                               \
  using Name = Handle<WGPU##Name, Name##Traits>

vtkWebGPUDeclareHandle(Adapter);
vtkWebGPUDeclareHandle(BindGroup);
vtkWebGPUDeclareHandle(BindGroupLayout);
vtkWebGPUDeclareHandle(Buffer);
vtkWebGPUDeclareHandle(CommandBuffer);
vtkWebGPUDeclareHandle(CommandEncoder);
vtkWebGPUDeclareHandle(ComputePassEncoder);
vtkWebGPUDeclareHandle(ComputePipeline);
vtkWebGPUDeclareHandle(Device);
vtkWebGPUDeclareHandle(Instance);
vtkWebGPUDeclareHandle(PipelineLayout);
vtkWebGPUDeclareHandle(QuerySet);
vtkWebGPUDeclareHandle(Queue);
vtkWebGPUDeclareHandle(RenderBundle);
vtkWebGPUDeclareHandle(RenderBundleEncoder);
vtkWebGPUDeclareHandle(RenderPassEncoder);
vtkWebGPUDeclareHandle(RenderPipeline);
vtkWebGPUDeclareHandle(Sampler);
vtkWebGPUDeclareHandle(ShaderModule);
vtkWebGPUDeclareHandle(Surface);
vtkWebGPUDeclareHandle(Texture);
vtkWebGPUDeclareHandle(TextureView);

#undef vtkWebGPUDeclareHandle

VTK_ABI_NAMESPACE_END
} // namespace vtkWebGPU

#endif
// VTK-HeaderTest-Exclude: vtkWebGPUHandle.h
