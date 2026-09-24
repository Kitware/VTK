// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkMemoryDescriptor
 * @brief   Describes a memory buffer on host or device, and who keeps it alive.
 *
 * A descriptor for one contiguous buffer that may reside on the host or on a
 * GPU device. It carries only primitive values -- pointer as an integer, size,
 * memory space and role -- so it passes freely between C++, Python and external
 * frameworks such as CuPy, PyTorch or Tack.
 *
 * Memory spaces:
 * - "host"       -- CPU / system memory
 * - "cuda"       -- NVIDIA device memory
 * - "hip"        -- AMD device memory
 * - "level_zero" -- Intel Level Zero device memory
 *
 * There is deliberately no "metal" space. Viskores has no Metal device
 * adapter and vtkDataArray::MemorySpace has no Metal value, so nothing in VTK
 * could branch on one. Metal memory arrives as "host": a shared-storage
 * MTLBuffer on Apple Silicon is unified memory, so its pointer really is a
 * host pointer, and a private-storage buffer has no host-readable pointer at
 * all and is rejected rather than described.
 *
 * A space names *what kind* of memory this is, not *which* device or context
 * it belongs to. There is no device ordinal here and no context handle. That
 * is enough for host, CUDA and HIP, where a pointer identifies itself to the
 * runtime, but not for Level Zero, whose pointers are only meaningful inside
 * the ze_context_handle_t they were allocated from -- which is why
 * vtkmDataArrayFactory refuses "level_zero" rather than guessing.
 *
 * ## Lifetime
 *
 * A descriptor that only described memory would be unusable for anything but
 * an immediate copy: the consumer has no way to stop the producer freeing the
 * buffer out from under it. So a descriptor also carries *how to release the
 * producer's hold*, in one of two forms:
 *
 * - `SetOwner()` for memory owned by a VTK object. The descriptor holds a
 *   reference, so the array cannot be destroyed while the descriptor lives.
 *   This is the natural form when VTK is the producer.
 *
 * - `SetRelease()` for memory owned by something outside VTK -- a CuPy array,
 *   a Tack field, a simulation's own allocator. The callback is invoked with
 *   its context when the hold is dropped. This is the same shape as DLPack's
 *   `manager_ctx` + `deleter`, and for good reason: it is the only form that
 *   survives a language boundary.
 *
 * Either way the rule is the same -- while a descriptor holding a release
 * exists, the memory is valid. A descriptor with neither describes memory
 * whose lifetime the caller is managing some other way, which is legal but
 * has to be a deliberate choice.
 *
 * `TakeRelease()` hands the release duty to someone else, and is how memory
 * gets handed to a consumer that will own it from then on -- see
 * vtkmDataArrayFactory, which transfers it into a Viskores buffer so the
 * producer's hold is dropped when Viskores is finished with the memory.
 *
 * @sa vtkmDataArrayFactory, vtkDataArray::NewMemoryDescriptors
 */

#ifndef vtkMemoryDescriptor_h
#define vtkMemoryDescriptor_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkObject.h"
#include "vtkType.h" // For vtkTypeInt64

VTK_ABI_NAMESPACE_BEGIN

class VTKCOMMONCORE_EXPORT vtkMemoryDescriptor : public vtkObject
{
public:
  static vtkMemoryDescriptor* New();
  vtkTypeMacro(vtkMemoryDescriptor, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Signature of a release callback. Called once, with the context given to
   * `SetRelease()`, when the hold on the memory is dropped.
   *
   * It may be called from any thread, including during VTK teardown. An
   * implementation that needs a runtime -- taking the Python GIL, for
   * instance -- is responsible for acquiring it itself.
   */
  using ReleaseFunction = void (*)(void* context);

  ///@{
  /**
   * The memory address as a 64-bit integer.
   * Cast from a raw pointer: reinterpret_cast<vtkTypeInt64>(ptr).
   */
  vtkSetMacro(Pointer, vtkTypeInt64);
  vtkGetMacro(Pointer, vtkTypeInt64);
  ///@}

  ///@{
  /**
   * Size of the buffer in bytes.
   */
  vtkSetMacro(SizeInBytes, vtkTypeInt64);
  vtkGetMacro(SizeInBytes, vtkTypeInt64);
  ///@}

  ///@{
  /**
   * Memory space identifier: "host", "cuda", "hip", "level_zero".
   */
  vtkSetStringMacro(MemorySpace);
  vtkGetStringMacro(MemorySpace);
  ///@}

  ///@{
  /**
   * Role of this buffer within the array.
   * Common roles:
   * - "data"        -- single flat buffer (AoS layout)
   * - "component_0" -- first component (SoA layout)
   * - "component_1" -- second component, etc.
   */
  vtkSetStringMacro(Role);
  vtkGetStringMacro(Role);
  ///@}

  /**
   * Convenience: set the description in one call. Does not touch the release.
   */
  void Set(vtkTypeInt64 pointer, vtkTypeInt64 sizeInBytes, const char* memorySpace,
    const char* role = "data");

  ///@{
  /**
   * Keep a VTK object alive for as long as this descriptor lives.
   *
   * Use when the buffer belongs to a VTK object -- typically the array the
   * descriptor was extracted from. The descriptor registers the object and
   * unregisters it on destruction.
   *
   * Replaces any previous owner or release callback.
   */
  void SetOwner(vtkObject* owner);
  vtkGetObjectMacro(Owner, vtkObject);
  ///@}

  /**
   * Keep non-VTK memory alive by holding a release callback.
   *
   * @a release is invoked with @a context when this descriptor is destroyed,
   * or when another party takes the release via `TakeRelease()`. Pass nullptr
   * for @a release to drop the hold without invoking it.
   *
   * Replaces any previous owner or release callback.
   */
  void SetRelease(ReleaseFunction release, void* context);

  /**
   * As `SetRelease()`, taking both addresses as integers.
   *
   * The wrapping tools cannot express a function pointer, so `SetRelease()`
   * does not appear in the wrapped interface at all -- which would leave a
   * wrapped language no way to hand VTK memory that it owns, and so no way
   * to import anything safely. Python passes
   * `ctypes.cast(callback, ctypes.c_void_p).value` here instead.
   *
   * The callback must outlive the descriptor: a ctypes callback object that
   * goes out of scope leaves this holding a dangling pointer.
   */
  void SetReleaseAddress(vtkTypeInt64 release, vtkTypeInt64 context);

  ///@{
  /**
   * The release callback and its context, or nullptr if none is held.
   */
  ReleaseFunction GetRelease() const { return this->Release; }
  void* GetReleaseContext() const { return this->ReleaseContext; }
  ///@}

  ///@{
  /**
   * The release callback and its context as integers, for wrapped callers.
   * Zero when none is held.
   */
  vtkTypeInt64 GetReleaseAddress() const { return reinterpret_cast<vtkTypeInt64>(this->Release); }
  vtkTypeInt64 GetReleaseContextAddress() const
  {
    return reinterpret_cast<vtkTypeInt64>(this->ReleaseContext);
  }
  ///@}

  /**
   * Whether this descriptor currently keeps the memory alive.
   */
  bool HasOwnership() const { return this->Owner != nullptr || this->Release != nullptr; }

  /**
   * Hand the release duty to the caller.
   *
   * On success @a release and @a context receive the callback, this descriptor
   * stops holding it, and the *caller* becomes responsible for invoking it
   * exactly once. Returns false if no release callback is held, in which case
   * the outputs are set to nullptr.
   *
   * A descriptor holding a VTK owner rather than a callback returns false;
   * use `GetOwner()` and register it yourself.
   */
  bool TakeRelease(ReleaseFunction& release, void*& context);

protected:
  vtkMemoryDescriptor();
  ~vtkMemoryDescriptor() override;

  vtkTypeInt64 Pointer = 0;
  vtkTypeInt64 SizeInBytes = 0;
  char* MemorySpace = nullptr;
  char* Role = nullptr;

  vtkObject* Owner = nullptr;
  ReleaseFunction Release = nullptr;
  void* ReleaseContext = nullptr;

private:
  vtkMemoryDescriptor(const vtkMemoryDescriptor&) = delete;
  void operator=(const vtkMemoryDescriptor&) = delete;

  /// Drop whatever hold is currently held, invoking the callback if set.
  void ClearOwnership();
};

VTK_ABI_NAMESPACE_END
#endif // vtkMemoryDescriptor_h
