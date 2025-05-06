//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef viskores_cont_exec_ArrayHandleExecutionManager_h
#define viskores_cont_exec_ArrayHandleExecutionManager_h

#include <viskores/cont/ErrorInternal.h>
#include <viskores/cont/Storage.h>
#include <viskores/cont/Token.h>

#include <viskores/cont/internal/ArrayTransfer.h>

namespace viskores
{
namespace cont
{
namespace internal
{

/// The common base for ArrayHandleExecutionManager. This is the interface
/// used when the type of the device is not known at run time.
///
template <typename T, typename Storage>
class ArrayHandleExecutionManagerBase
{
private:
  using StorageType = viskores::cont::internal::Storage<T, Storage>;

public:
  template <typename DeviceAdapter>
  struct ExecutionTypes
  {
  private:
    using ArrayTransferType = viskores::cont::internal::ArrayTransfer<T, Storage, DeviceAdapter>;

  public:
    using Portal = typename ArrayTransferType::PortalExecution;
    using PortalConst = typename ArrayTransferType::PortalConstExecution;
  };

  /// The type of value held in the array (viskores::FloatDefault, viskores::Vec, etc.)
  ///
  using ValueType = T;

  /// An array portal that can be used in the control environment.
  ///
  using PortalControl = typename StorageType::PortalType;
  using PortalConstControl = typename StorageType::PortalConstType;

  VISKORES_CONT
  virtual ~ArrayHandleExecutionManagerBase() {}

  /// Returns the number of values stored in the array.  Results are undefined
  /// if data has not been loaded or allocated.
  ///
  VISKORES_CONT
  viskores::Id GetNumberOfValues() const { return this->GetNumberOfValuesImpl(); }

  /// Prepares the data for use as input in the execution environment. If the
  /// flag \c updateData is true, then data is transferred to the execution
  /// environment. Otherwise, this transfer should be skipped.
  ///
  /// Returns a constant array portal valid in the execution environment.
  ///
  template <typename DeviceAdapter>
  VISKORES_CONT typename ExecutionTypes<DeviceAdapter>::PortalConst
  PrepareForInput(bool updateData, DeviceAdapter, viskores::cont::Token& token)
  {
    this->VerifyDeviceAdapter(DeviceAdapter());

    typename ExecutionTypes<DeviceAdapter>::PortalConst portal;
    this->PrepareForInputImpl(updateData, &portal, token);
    return portal;
  }

  /// Prepares the data for use as both input and output in the execution
  /// environment. If the flag \c updateData is true, then data is transferred
  /// to the execution environment. Otherwise, this transfer should be skipped.
  ///
  /// Returns a read-write array portal valid in the execution environment.
  ///
  template <typename DeviceAdapter>
  VISKORES_CONT typename ExecutionTypes<DeviceAdapter>::Portal
  PrepareForInPlace(bool updateData, DeviceAdapter, viskores::cont::Token& token)
  {
    this->VerifyDeviceAdapter(DeviceAdapter());

    typename ExecutionTypes<DeviceAdapter>::Portal portal;
    this->PrepareForInPlaceImpl(updateData, &portal, token);
    return portal;
  }

  /// Allocates an array in the execution environment of the specified size. If
  /// control and execution share arrays, then this class can allocate data
  /// using the given Storage it can be used directly in the execution
  /// environment.
  ///
  /// Returns a writable array portal valid in the execution environment.
  ///
  template <typename DeviceAdapter>
  VISKORES_CONT typename ExecutionTypes<DeviceAdapter>::Portal
  PrepareForOutput(viskores::Id numberOfValues, DeviceAdapter, viskores::cont::Token& token)
  {
    this->VerifyDeviceAdapter(DeviceAdapter());

    typename ExecutionTypes<DeviceAdapter>::Portal portal;
    this->PrepareForOutputImpl(numberOfValues, &portal, token);
    return portal;
  }

  /// Allocates data in the given Storage and copies data held in the execution
  /// environment (managed by this class) into the storage object. The
  /// reference to the storage given is the same as that passed to the
  /// constructor. If control and execution share arrays, this can be no
  /// operation. This method should only be called after PrepareForOutput is
  /// called.
  ///
  VISKORES_CONT
  void RetrieveOutputData(StorageType* storage) const { this->RetrieveOutputDataImpl(storage); }

  /// \brief Reduces the size of the array without changing its values.
  ///
  /// This method allows you to resize the array without reallocating it. The
  /// number of entries in the array is changed to \c numberOfValues. The data
  /// in the array (from indices 0 to \c numberOfValues - 1) are the same, but
  /// \c numberOfValues must be equal or less than the preexisting size
  /// (returned from GetNumberOfValues). That is, this method can only be used
  /// to shorten the array, not lengthen.
  ///
  VISKORES_CONT
  void Shrink(viskores::Id numberOfValues) { this->ShrinkImpl(numberOfValues); }

  /// Frees any resources (i.e. memory) allocated for the exeuction
  /// environment, if any.
  ///
  VISKORES_CONT
  void ReleaseResources() { this->ReleaseResourcesImpl(); }

  template <typename DeviceAdapter>
  VISKORES_CONT bool IsDeviceAdapter(DeviceAdapter device) const
  {
    return this->IsDeviceAdapterImpl(device);
  }

  VISKORES_CONT
  DeviceAdapterId GetDeviceAdapterId() const { return this->GetDeviceAdapterIdImpl(); }

protected:
  virtual viskores::Id GetNumberOfValuesImpl() const = 0;

  virtual void PrepareForInputImpl(bool updateData,
                                   void* portalExecutionVoid,
                                   viskores::cont::Token& token) = 0;

  virtual void PrepareForInPlaceImpl(bool updateData,
                                     void* portalExecutionVoid,
                                     viskores::cont::Token& token) = 0;

  virtual void PrepareForOutputImpl(viskores::Id numberOfValues,
                                    void* portalExecution,
                                    viskores::cont::Token& token) = 0;

  virtual void RetrieveOutputDataImpl(StorageType* storage) const = 0;

  virtual void ShrinkImpl(Id numberOfValues) = 0;

  virtual void ReleaseResourcesImpl() = 0;

  virtual bool IsDeviceAdapterImpl(const viskores::cont::DeviceAdapterId& id) const = 0;

  virtual DeviceAdapterId GetDeviceAdapterIdImpl() const = 0;

private:
  template <typename DeviceAdapter>
  VISKORES_CONT void VerifyDeviceAdapter(DeviceAdapter device) const
  {
    if (!this->IsDeviceAdapter(device))
    {
      throw viskores::cont::ErrorInternal("Device Adapter Mismatch");
    }
  }
};

/// \brief Used by ArrayHandle to manage execution arrays
///
/// This is an internal class used by ArrayHandle to manage execution arrays.
/// This class uses virtual method polymorphism to allocate and transfer data
/// in the execution environment. This virtual method polymorphism allows the
/// ArrayHandle to change its device at run time.
///
template <typename T, typename Storage, typename DeviceAdapter>
class ArrayHandleExecutionManager : public ArrayHandleExecutionManagerBase<T, Storage>
{
  using Superclass = ArrayHandleExecutionManagerBase<T, Storage>;
  using ArrayTransferType = viskores::cont::internal::ArrayTransfer<T, Storage, DeviceAdapter>;
  using StorageType = viskores::cont::internal::Storage<T, Storage>;

public:
  using PortalControl = typename ArrayTransferType::PortalControl;
  using PortalConstControl = typename ArrayTransferType::PortalConstControl;

  using PortalExecution = typename ArrayTransferType::PortalExecution;
  using PortalConstExecution = typename ArrayTransferType::PortalConstExecution;

  VISKORES_CONT
  ArrayHandleExecutionManager(StorageType* storage)
    : Transfer(storage)
  {
  }

protected:
  VISKORES_CONT
  viskores::Id GetNumberOfValuesImpl() const { return this->Transfer.GetNumberOfValues(); }

  VISKORES_CONT
  void PrepareForInputImpl(bool updateData, void* portalExecutionVoid, viskores::cont::Token& token)
  {
    PortalConstExecution portal = this->Transfer.PrepareForInput(updateData, token);
    *reinterpret_cast<PortalConstExecution*>(portalExecutionVoid) = portal;
  }

  VISKORES_CONT
  void PrepareForInPlaceImpl(bool updateData,
                             void* portalExecutionVoid,
                             viskores::cont::Token& token)
  {
    PortalExecution portal = this->Transfer.PrepareForInPlace(updateData, token);
    *reinterpret_cast<PortalExecution*>(portalExecutionVoid) = portal;
  }

  VISKORES_CONT
  void PrepareForOutputImpl(viskores::Id numberOfValues,
                            void* portalExecutionVoid,
                            viskores::cont::Token& token)
  {
    PortalExecution portal = this->Transfer.PrepareForOutput(numberOfValues, token);
    *reinterpret_cast<PortalExecution*>(portalExecutionVoid) = portal;
  }

  VISKORES_CONT
  void RetrieveOutputDataImpl(StorageType* storage) const
  {
    this->Transfer.RetrieveOutputData(storage);
  }

  VISKORES_CONT
  void ShrinkImpl(Id numberOfValues) { this->Transfer.Shrink(numberOfValues); }

  VISKORES_CONT
  void ReleaseResourcesImpl() { this->Transfer.ReleaseResources(); }

  VISKORES_CONT
  bool IsDeviceAdapterImpl(const DeviceAdapterId& id) const { return id == DeviceAdapter(); }

  VISKORES_CONT
  DeviceAdapterId GetDeviceAdapterIdImpl() const { return DeviceAdapter(); }

private:
  ArrayTransferType Transfer;
};
}
}
} // namespace viskores::cont::internal

#endif //viskores_cont_exec_ArrayHandleExecutionManager_h
