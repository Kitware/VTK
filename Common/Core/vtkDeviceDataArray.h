// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDeviceDataArray
 * @brief   a data array over GPU memory another library owns
 *
 * vtkDeviceDataArray holds interleaved values that live in a GPU's memory,
 * described by a vtkMemoryDescriptor: typically a tensor imported through
 * DLPack (`vtkmodules.util.dlpack_support`). The array keeps the descriptor,
 * and with it whatever keeps the memory alive, until it is destroyed.
 *
 * It does what VTK needs to pass GPU data along without touching it:
 * GetMemorySpace(), GetDeviceVoidPointer() and NewMemoryDescriptors() give the
 * memory back as it is, so a pipeline of GPU filters exchanges it with no
 * copies. VTK code that reads values on the host -- filters, writers,
 * rendering, numpy -- reads a copy, made from the whole buffer on first use
 * and refreshed after Modified(). GetVoidPointer() returns that copy.
 *
 * Any Modified() marks the copy stale, including the ones metadata changes
 * make (SetName(), SetLookupTable(), ...): the array cannot tell those from a
 * write to the device memory, so the next host read copies the whole buffer
 * again and waits for the device. Set metadata before reading on the host
 * where that matters.
 *
 * If the device memory cannot be read, an error is reported and the copy
 * holds NaN (zeros for integer types) in its place.
 *
 * A pointer into the copy stays valid when the copy is refreshed. It does not
 * survive SetDescriptor() or ShallowCopy() to memory with more values: the
 * next host read then reallocates the copy.
 *
 * The array is read-only on the host and fixed in size: a host write, or
 * growing the array, is refused with an error. The device memory is the only
 * real copy, and a write to the host copy would never reach it. NewInstance()
 * therefore returns a vtkAOSDataArrayTemplate of the same value type, so
 * filters that build output arrays "like" this one get ordinary host arrays.
 * Shrinking is allowed and keeps the memory.
 *
 * VTK does not link against a GPU runtime for this: vtkDeviceMemory loads the
 * vendor's driver or runtime when a copy is first needed: CUDA, HIP and
 * Level Zero memory.
 *
 * @sa vtkDeviceArrayFactory vtkMemoryDescriptor
 */

#ifndef vtkDeviceDataArray_h
#define vtkDeviceDataArray_h

#include "vtkAOSDataArrayTemplate.h" // For NewInstance
#include "vtkCommonCoreModule.h"     // For export macro
#include "vtkGenericDataArray.h"
#include "vtkMemoryDescriptor.h" // For the held descriptor
#include "vtkSmartPointer.h"     // For the held descriptor

#include <atomic>      // For the host copy's state
#include <mutex>       // For the host copy
#include <type_traits> // For std::is_arithmetic
#include <typeinfo>    // For the type macro
#include <vector>      // For the host copy

VTK_ABI_NAMESPACE_BEGIN
template <typename ValueTypeT>
class vtkDeviceDataArray
  : public vtkGenericDataArray<vtkDeviceDataArray<ValueTypeT>, ValueTypeT,
      vtkArrayTypes::VTK_DEVICE_DATA_ARRAY>
{
  static_assert(std::is_arithmetic<ValueTypeT>::value, "ValueTypeT must be a number");

  using GenericDataArrayType = vtkGenericDataArray<vtkDeviceDataArray<ValueTypeT>, ValueTypeT,
    vtkArrayTypes::VTK_DEVICE_DATA_ARRAY>;

public:
  using SelfType = vtkDeviceDataArray<ValueTypeT>;
  // NewInstance() makes a host array: see the class description.
  vtkAbstractTypeMacroWithNewInstanceType(
    SelfType, GenericDataArrayType, vtkAOSDataArrayTemplate<ValueTypeT>, typeid(SelfType).name());
  using typename Superclass::ValueType;

  static vtkDeviceDataArray* New();

  /**
   * Hold the memory `descriptor` describes as `numberOfTuples` tuples of
   * `numberOfComponents` interleaved values. The descriptor's memory space
   * must be a device this VTK can read and its size must cover the values.
   * Returns false, with an error and the array left empty, otherwise.
   */
  bool SetDescriptor(
    vtkMemoryDescriptor* descriptor, vtkIdType numberOfTuples, int numberOfComponents);

  ///@{
  /**
   * The device memory, as it is: where it lives, a pointer to it, and a
   * descriptor holding this array (and so the memory) alive.
   */
  vtkDataArray::MemorySpace GetMemorySpace() override;
  void* GetDeviceVoidPointer(vtkIdType valueIdx) override;
  VTK_NEWINSTANCE
  vtkCollection* NewMemoryDescriptors() override;
  ///@}

  /**
   * The host copy of the values, made on first use. Read only: writes to it
   * never reach the device memory.
   */
  void* GetVoidPointer(vtkIdType valueIdx) override;

  /**
   * Share another vtkDeviceDataArray's memory. Any other array is refused:
   * this one cannot hold host data.
   */
  void ShallowCopy(vtkDataArray* other) override;
  using vtkAbstractArray::ShallowCopy;

  /**
   * Refuse to grow past the device memory. Asking for no more than it holds
   * succeeds without allocating.
   */
  vtkTypeBool ReserveTuples(vtkIdType numTuples) override;

  /**
   * Mark the host copy stale, so the next host read copies the device memory
   * again. Call it after the memory's owner has written to it.
   */
  void Modified() override;
  void DataChanged() override;

  ///@{
  /**
   * The vtkGenericDataArray concept methods. Reads come from the host copy;
   * writes are refused.
   */
  ValueType GetValue(vtkIdType valueIdx) const { return this->HostValues()[valueIdx]; }
  void SetValue(vtkIdType valueIdx, ValueType value);
  void GetTypedTuple(vtkIdType tupleIdx, ValueType* tuple) const;
  void SetTypedTuple(vtkIdType tupleIdx, const ValueType* tuple);
  ValueType GetTypedComponent(vtkIdType tupleIdx, int compIdx) const
  {
    return this->HostValues()[tupleIdx * this->NumberOfComponents + compIdx];
  }
  void SetTypedComponent(vtkIdType tupleIdx, int compIdx, ValueType value);
  ///@}

protected:
  vtkDeviceDataArray();
  ~vtkDeviceDataArray() override;

  vtkObjectBase* NewInstanceInternal() const override
  {
    return vtkAOSDataArrayTemplate<ValueTypeT>::New();
  }

  ///@{
  /**
   * Concept methods: these succeed only within the device memory.
   */
  bool AllocateTuples(vtkIdType numTuples);
  bool ReallocateTuples(vtkIdType numTuples);
  ///@}

private:
  friend class vtkGenericDataArray<SelfType, ValueType, vtkArrayTypes::VTK_DEVICE_DATA_ARRAY>;

  // The host copy, made (again) if it is missing or stale.
  const ValueType* HostValues() const;
  // Report a refused write, once per array.
  void RefuseWrite();

  vtkSmartPointer<vtkMemoryDescriptor> Descriptor;
  vtkDataArray::MemorySpace Space = vtkDataArray::HostMemory;
  vtkIdType DeviceValues = 0; // how many values the device memory holds

  mutable std::vector<ValueType> Host;
  mutable std::atomic<bool> HostCurrent{ false };
  mutable std::mutex HostMutex;
  std::atomic<bool> WriteRefused{ false };

  vtkDeviceDataArray(const vtkDeviceDataArray&) = delete;
  void operator=(const vtkDeviceDataArray&) = delete;
};

// Declare vtkArrayDownCast implementations for vtkDeviceDataArray:
vtkArrayDownCast_TemplateFastCastMacro(vtkDeviceDataArray);

#ifndef vtkDeviceDataArray_cxx
#ifdef _MSC_VER
#pragma warning(push)
// Needed when vtkDeviceDataArray is used from another class in vtkCommonCore.
#pragma warning(disable : 4910) // extern and dllexport incompatible
#endif
extern template class VTKCOMMONCORE_EXPORT vtkDeviceDataArray<char>;
extern template class VTKCOMMONCORE_EXPORT vtkDeviceDataArray<double>;
extern template class VTKCOMMONCORE_EXPORT vtkDeviceDataArray<float>;
extern template class VTKCOMMONCORE_EXPORT vtkDeviceDataArray<int>;
extern template class VTKCOMMONCORE_EXPORT vtkDeviceDataArray<long>;
extern template class VTKCOMMONCORE_EXPORT vtkDeviceDataArray<long long>;
extern template class VTKCOMMONCORE_EXPORT vtkDeviceDataArray<short>;
extern template class VTKCOMMONCORE_EXPORT vtkDeviceDataArray<signed char>;
extern template class VTKCOMMONCORE_EXPORT vtkDeviceDataArray<unsigned char>;
extern template class VTKCOMMONCORE_EXPORT vtkDeviceDataArray<unsigned int>;
extern template class VTKCOMMONCORE_EXPORT vtkDeviceDataArray<unsigned long>;
extern template class VTKCOMMONCORE_EXPORT vtkDeviceDataArray<unsigned long long>;
extern template class VTKCOMMONCORE_EXPORT vtkDeviceDataArray<unsigned short>;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#endif

VTK_ABI_NAMESPACE_END
#endif // vtkDeviceDataArray_h
// VTK-HeaderTest-Exclude: vtkDeviceDataArray.h
