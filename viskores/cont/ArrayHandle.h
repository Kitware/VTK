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
#ifndef viskores_cont_ArrayHandle_h
#define viskores_cont_ArrayHandle_h

#include <viskores/cont/viskores_cont_export.h>

#include <viskores/Assert.h>
#include <viskores/Flags.h>
#include <viskores/Types.h>

#include <viskores/cont/DeviceAdapterList.h>
#include <viskores/cont/ErrorBadValue.h>
#include <viskores/cont/ErrorInternal.h>
#include <viskores/cont/Storage.h>
#include <viskores/cont/Token.h>

#include <viskores/internal/ArrayPortalHelpers.h>

#include <algorithm>
#include <deque>
#include <iterator>
#include <memory>
#include <mutex>
#include <vector>

#include <viskores/cont/internal/Buffer.h>

namespace viskores
{
namespace cont
{

// Normally this would be defined in ArrayHandleBasic.h, but we need this declared early for
// the default storage.

/// A tag for the basic implementation of a Storage object.
struct VISKORES_ALWAYS_EXPORT StorageTagBasic
{
};
}
} // namespace viskores::cont

#if VISKORES_STORAGE == VISKORES_STORAGE_BASIC

#define VISKORES_DEFAULT_STORAGE_TAG ::viskores::cont::StorageTagBasic

#elif VISKORES_STORAGE == VISKORES_STORAGE_ERROR

#include <viskores/cont/internal/StorageError.h>
#define VISKORES_DEFAULT_STORAGE_TAG ::viskores::cont::internal::StorageTagError

#elif (VISKORES_STORAGE == VISKORES_STORAGE_UNDEFINED) || !defined(VISKORES_STORAGE)

#ifndef VISKORES_DEFAULT_STORAGE_TAG
#warning If array storage is undefined, VISKORES_DEFAULT_STORAGE_TAG must be defined.
#endif

#endif

namespace viskores
{
namespace cont
{

namespace internal
{

/// \brief Base class of all ArrayHandle classes.
///
/// This is an empty class that is used to check if something is an \c
/// ArrayHandle class (or at least something that behaves exactly like one).
/// The \c ArrayHandle template class inherits from this.
///
class VISKORES_CONT_EXPORT ArrayHandleBase
{
};

/// Checks to see if the given type and storage forms a valid array handle
/// (some storage objects cannot support all types). This check is compatible
/// with C++11 type_traits.
///
template <typename T, typename StorageTag>
using IsValidArrayHandle = std::integral_constant<
  bool,
  !(std::is_base_of<viskores::cont::internal::UndefinedStorage,
                    viskores::cont::internal::Storage<T, StorageTag>>::value)>;

/// Checks to see if the given type and storage forms a invalid array handle
/// (some storage objects cannot support all types). This check is compatible
/// with C++11 type_traits.
///
template <typename T, typename StorageTag>
using IsInvalidArrayHandle =
  std::integral_constant<bool, !IsValidArrayHandle<T, StorageTag>::value>;

/// Checks to see if the ArrayHandle allows writing, as some ArrayHandles
/// (Implicit) don't support writing. These will be defined as either
/// std::true_type or std::false_type.
///
/// \sa viskores::internal::PortalSupportsSets
///
template <typename ArrayHandle>
using IsWritableArrayHandle =
  viskores::internal::PortalSupportsSets<typename std::decay<ArrayHandle>::type::WritePortalType>;

/// Checks to see if the given object is an array handle. This check is
/// compatible with C++11 type_traits. It a typedef named \c type that is
/// either std::true_type or std::false_type. Both of these have a typedef
/// named value with the respective boolean value.
///
/// Unlike \c IsValidArrayHandle, if an \c ArrayHandle is used with this
/// class, then it must be created by the compiler and therefore must already
/// be valid. Where \c IsValidArrayHandle is used when you know something is
/// an \c ArrayHandle but you are not sure if the \c StorageTag is valid, this
/// class is used to ensure that a given type is an \c ArrayHandle. It is
/// used internally in the VISKORES_IS_ARRAY_HANDLE macro.
///
template <typename T>
struct ArrayHandleCheck
  : std::is_base_of<viskores::cont::internal::ArrayHandleBase, std::decay_t<T>>::type
{
};

/// @brief Checks that the given type is a `viskores::cont::ArrayHandle`.
///
/// If the type is not a `viskores::cont::ArrayHandle` or a subclass, a static assert will
/// cause a compile exception. This is a good way to ensure that a template argument
/// that is assumed to be an array handle type actually is.
#define VISKORES_IS_ARRAY_HANDLE(T) \
  VISKORES_STATIC_ASSERT(::viskores::cont::internal::ArrayHandleCheck<T>{})

} // namespace internal

namespace detail
{

template <typename T>
struct GetTypeInParentheses;
template <typename T>
struct GetTypeInParentheses<void(T)>
{
  using type = T;
};

} // namespace detail

// Implementation for VISKORES_ARRAY_HANDLE_SUBCLASS macros
#define VISKORES_ARRAY_HANDLE_SUBCLASS_IMPL(classname, fullclasstype, superclass, typename__) \
  using Thisclass =                                                                           \
    typename__ viskores::cont::detail::GetTypeInParentheses<void fullclasstype>::type;        \
  using Superclass =                                                                          \
    typename__ viskores::cont::detail::GetTypeInParentheses<void superclass>::type;           \
                                                                                              \
  VISKORES_IS_ARRAY_HANDLE(Superclass);                                                       \
                                                                                              \
  VISKORES_CONT                                                                               \
  classname()                                                                                 \
  {                                                                                           \
  }                                                                                           \
                                                                                              \
  VISKORES_CONT                                                                               \
  classname(const Thisclass& src)                                                             \
    : Superclass(src)                                                                         \
  {                                                                                           \
  }                                                                                           \
                                                                                              \
  VISKORES_CONT                                                                               \
  classname(Thisclass&& src) noexcept                                                         \
    : Superclass(std::move(src))                                                              \
  {                                                                                           \
  }                                                                                           \
                                                                                              \
  VISKORES_CONT                                                                               \
  classname(const viskores::cont::ArrayHandle<typename__ Superclass::ValueType,               \
                                              typename__ Superclass::StorageTag>& src)        \
    : Superclass(src)                                                                         \
  {                                                                                           \
  }                                                                                           \
                                                                                              \
  VISKORES_CONT                                                                               \
  classname(viskores::cont::ArrayHandle<typename__ Superclass::ValueType,                     \
                                        typename__ Superclass::StorageTag>&& src) noexcept    \
    : Superclass(std::move(src))                                                              \
  {                                                                                           \
  }                                                                                           \
                                                                                              \
  VISKORES_CONT                                                                               \
  explicit classname(const std::vector<viskores::cont::internal::Buffer>& buffers)            \
    : Superclass(buffers)                                                                     \
  {                                                                                           \
  }                                                                                           \
                                                                                              \
  VISKORES_CONT                                                                               \
  explicit classname(std::vector<viskores::cont::internal::Buffer>&& buffers) noexcept        \
    : Superclass(std::move(buffers))                                                          \
  {                                                                                           \
  }                                                                                           \
                                                                                              \
  VISKORES_CONT                                                                               \
  Thisclass& operator=(const Thisclass& src)                                                  \
  {                                                                                           \
    this->Superclass::operator=(src);                                                         \
    return *this;                                                                             \
  }                                                                                           \
                                                                                              \
  VISKORES_CONT                                                                               \
  Thisclass& operator=(Thisclass&& src) noexcept                                              \
  {                                                                                           \
    this->Superclass::operator=(std::move(src));                                              \
    return *this;                                                                             \
  }                                                                                           \
                                                                                              \
  using ValueType = typename__ Superclass::ValueType;                                         \
  using StorageTag = typename__ Superclass::StorageTag;                                       \
  using StorageType = typename__ Superclass::StorageType;                                     \
  using ReadPortalType = typename__ Superclass::ReadPortalType;                               \
  using WritePortalType = typename__ Superclass::WritePortalType

/// \brief Macro to make default methods in ArrayHandle subclasses.
///
/// This macro defines the default constructors, destructors and assignment
/// operators for ArrayHandle subclasses that are templates. The ArrayHandle
/// subclasses are assumed to be empty convenience classes. The macro should be
/// defined after a \c public: declaration.
///
/// This macro takes three arguments. The first argument is the classname.
/// The second argument is the full class type. The third argument is the
/// superclass type (either \c ArrayHandle or another sublcass). Because
/// C macros do not handle template parameters very well (the preprocessor
/// thinks the template commas are macro argument commas), the second and
/// third arguments must be wrapped in parentheses.
///
/// This macro also defines a Superclass typedef as well as ValueType and
/// StorageTag.
///
/// Note that this macro only works on ArrayHandle subclasses that are
/// templated. For ArrayHandle sublcasses that are not templates, use
/// VISKORES_ARRAY_HANDLE_SUBCLASS_NT.
///
#define VISKORES_ARRAY_HANDLE_SUBCLASS(classname, fullclasstype, superclass) \
  VISKORES_ARRAY_HANDLE_SUBCLASS_IMPL(classname, fullclasstype, superclass, typename)

/// \brief Macro to make default methods in ArrayHandle subclasses.
///
/// This macro defines the default constructors, destructors and assignment
/// operators for ArrayHandle subclasses that are not templates. The
/// ArrayHandle subclasses are assumed to be empty convenience classes. The
/// macro should be defined after a \c public: declaration.
///
/// This macro takes two arguments. The first argument is the classname. The
/// second argument is the superclass type (either \c ArrayHandle or another
/// sublcass). Because C macros do not handle template parameters very well
/// (the preprocessor thinks the template commas are macro argument commas),
/// the second argument must be wrapped in parentheses.
///
/// This macro also defines a Superclass typedef as well as ValueType and
/// StorageTag.
///
/// Note that this macro only works on ArrayHandle subclasses that are not
/// templated. For ArrayHandle sublcasses that are templates, use
/// VISKORES_ARRAY_HANDLE_SUBCLASS.
///
#define VISKORES_ARRAY_HANDLE_SUBCLASS_NT(classname, superclass) \
  VISKORES_ARRAY_HANDLE_SUBCLASS_IMPL(classname, (classname), superclass, )

namespace detail
{

VISKORES_CONT_EXPORT VISKORES_CONT void ArrayHandleReleaseResourcesExecution(
  const std::vector<viskores::cont::internal::Buffer>& buffers);

VISKORES_CONT_EXPORT VISKORES_CONT bool ArrayHandleIsOnDevice(
  const std::vector<viskores::cont::internal::Buffer>& buffers,
  viskores::cont::DeviceAdapterId device);

} // namespace detail

/// @brief Manages an array-worth of data.
///
/// `ArrayHandle` manages as array of data that can be manipulated by Viskores
/// algorithms. The `ArrayHandle` may have up to two copies of the array, one
/// for the control environment and one for the execution environment, although
/// depending on the device and how the array is being used, the `ArrayHandle`
/// will only have one copy when possible.
///
/// An `ArrayHandle` is often constructed by instantiating one of the `ArrayHandle`
/// subclasses. Several basic `ArrayHandle` types can also be constructed directly
/// and then allocated. The `ArrayHandleBasic` subclass provides mechanisms for
/// importing user arrays into an `ArrayHandle`.
///
/// `ArrayHandle` behaves like a shared smart pointer in that when it is copied
/// each copy holds a reference to the same array.  These copies are reference
/// counted so that when all copies of the `ArrayHandle` are destroyed, any
/// allocated memory is released.
///
template <typename T, typename StorageTag_ = VISKORES_DEFAULT_STORAGE_TAG>
class VISKORES_ALWAYS_EXPORT ArrayHandle : public internal::ArrayHandleBase
{
  VISKORES_STATIC_ASSERT_MSG(
    (internal::IsValidArrayHandle<T, StorageTag_>::value),
    "Attempted to create an ArrayHandle with an invalid type/storage combination.");

public:
  using ValueType = T;
  using StorageTag = StorageTag_;
  using StorageType = viskores::cont::internal::Storage<ValueType, StorageTag>;

  /// The type of portal used when accessing data in a read-only mode.
  using ReadPortalType = typename StorageType::ReadPortalType;
  /// The type of portal used when accessing data in a read-write mode.
  using WritePortalType = typename StorageType::WritePortalType;

  /// Constructs an empty ArrayHandle.
  ///
  VISKORES_CONT ArrayHandle()
    : Buffers(StorageType::CreateBuffers())
  {
  }

  /// Copy constructor.
  ///
  /// Implemented so that it is defined exclusively in the control environment.
  /// If there is a separate device for the execution environment (for example,
  /// with CUDA), then the automatically generated copy constructor could be
  /// created for all devices, and it would not be valid for all devices.
  ///
  VISKORES_CONT ArrayHandle(const viskores::cont::ArrayHandle<ValueType, StorageTag>& src)
    : Buffers(src.Buffers)
  {
  }

  /// Move constructor.
  ///
  /// Implemented so that it is defined exclusively in the control environment.
  /// If there is a separate device for the execution environment (for example,
  /// with CUDA), then the automatically generated move constructor could be
  /// created for all devices, and it would not be valid for all devices.
  ///
  VISKORES_CONT ArrayHandle(viskores::cont::ArrayHandle<ValueType, StorageTag>&& src) noexcept
    : Buffers(std::move(src.Buffers))
  {
  }

  /// Special constructor for subclass specializations that need to set the
  /// initial state array. Used when pulling data from other sources.
  ///
  VISKORES_CONT explicit ArrayHandle(const std::vector<viskores::cont::internal::Buffer>& buffers)
    : Buffers(buffers)
  {
  }

  /// Special constructor for subclass specializations that need to set the
  /// initial state array. Used when pulling data from other sources.
  ///
  VISKORES_CONT explicit ArrayHandle(
    std::vector<viskores::cont::internal::Buffer>&& buffers) noexcept
    : Buffers(std::move(buffers))
  {
  }

  /// Destructs an empty ArrayHandle.
  ///
  /// Implemented so that it is defined exclusively in the control environment.
  /// If there is a separate device for the execution environment (for example,
  /// with CUDA), then the automatically generated destructor could be
  /// created for all devices, and it would not be valid for all devices.
  ///
  VISKORES_CONT ~ArrayHandle() {}

  /// @brief Shallow copies an ArrayHandle
  ///
  VISKORES_CONT
  viskores::cont::ArrayHandle<ValueType, StorageTag>& operator=(
    const viskores::cont::ArrayHandle<ValueType, StorageTag>& src)
  {
    this->Buffers = src.Buffers;
    return *this;
  }

  /// @brief Move and Assignment of an ArrayHandle
  ///
  VISKORES_CONT
  viskores::cont::ArrayHandle<ValueType, StorageTag>& operator=(
    viskores::cont::ArrayHandle<ValueType, StorageTag>&& src) noexcept
  {
    this->Buffers = std::move(src.Buffers);
    return *this;
  }

  /// Like a pointer, two `ArrayHandle`s are considered equal if they point
  /// to the same location in memory.
  ///
  VISKORES_CONT
  bool operator==(const ArrayHandle<ValueType, StorageTag>& rhs) const
  {
    return this->Buffers == rhs.Buffers;
  }

  VISKORES_CONT
  bool operator!=(const ArrayHandle<ValueType, StorageTag>& rhs) const
  {
    return this->Buffers != rhs.Buffers;
  }

  template <typename VT, typename ST>
  VISKORES_CONT bool operator==(const ArrayHandle<VT, ST>&) const
  {
    return false; // different valuetype and/or storage
  }

  template <typename VT, typename ST>
  VISKORES_CONT bool operator!=(const ArrayHandle<VT, ST>&) const
  {
    return true; // different valuetype and/or storage
  }

  /// Get the storage.
  ///
  VISKORES_CONT StorageType GetStorage() const { return StorageType{}; }

  /// @brief Get an array portal that can be used in the control environment.
  ///
  /// The returned array can be used in the control environment to read values from the array. (It
  /// is not possible to write to the returned portal. That is `Get` will work on the portal, but
  /// `Set` will not.)
  ///
  /// **Note:** The returned portal cannot be used in the execution environment. This is because
  /// the portal will not work on some devices like GPUs. To get a portal that will work in the
  /// execution environment, use `PrepareForInput`.
  ///
  VISKORES_CONT ReadPortalType ReadPortal() const
  {
    viskores::cont::Token token;
    return this->ReadPortal(token);
  }
  /// @copydoc ReadPortalType
  VISKORES_CONT ReadPortalType ReadPortal(viskores::cont::Token& token) const
  {
    return StorageType::CreateReadPortal(
      this->GetBuffers(), viskores::cont::DeviceAdapterTagUndefined{}, token);
  }

  /// @brief Get an array portal that can be used in the control environment.
  ///
  /// The returned array can be used in the control environment to reand and write values to the
  /// array.
  ///
  /// **Note:** The returned portal cannot be used in the execution environment. This is because
  /// the portal will not work on some devices like GPUs. To get a portal that will work in the
  /// execution environment, use `PrepareForInput`.
  ///
  VISKORES_CONT WritePortalType WritePortal() const
  {
    viskores::cont::Token token;
    return this->WritePortal(token);
  }
  /// @copydoc WritePortal
  VISKORES_CONT WritePortalType WritePortal(viskores::cont::Token& token) const
  {
    return StorageType::CreateWritePortal(
      this->GetBuffers(), viskores::cont::DeviceAdapterTagUndefined{}, token);
  }

  /// Returns the number of entries in the array.
  ///
  VISKORES_CONT viskores::Id GetNumberOfValues() const
  {
    return StorageType::GetNumberOfValues(this->GetBuffers());
  }

  /// @copydoc viskores::cont::UnknownArrayHandle::GetNumberOfComponentsFlat
  VISKORES_CONT viskores::IdComponent GetNumberOfComponentsFlat() const
  {
    return StorageType::GetNumberOfComponentsFlat(this->GetBuffers());
  }

  /// @brief Allocates an array large enough to hold the given number of values.
  ///
  /// The allocation may be done on an already existing array. If so, then the data
  /// are preserved as best as possible if the preserve flag is set to `viskores::CopyFlag::On`.
  /// If the preserve flag is set to `viskores::CopyFlag::Off` (the default), any existing data
  /// could be wiped out.
  ///
  /// This method can throw `viskores::cont::ErrorBadAllocation` if the array cannot be allocated or
  /// `viskores::cont::ErrorBadValue` if the allocation is not feasible (for example, the
  /// array storage is read-only).
  ///
  VISKORES_CONT void Allocate(viskores::Id numberOfValues,
                              viskores::CopyFlag preserve,
                              viskores::cont::Token& token) const
  {
    StorageType::ResizeBuffers(numberOfValues, this->GetBuffers(), preserve, token);
  }

  /// @copydoc Allocate
  VISKORES_CONT void Allocate(viskores::Id numberOfValues,
                              viskores::CopyFlag preserve = viskores::CopyFlag::Off) const
  {
    viskores::cont::Token token;
    this->Allocate(numberOfValues, preserve, token);
  }

  /// @brief Allocates an array and fills it with an initial value.
  ///
  /// `AllocateAndFill` behaves similar to `Allocate` except that after allocation it fills
  /// the array with a given `fillValue`. This method is convenient when you wish to initialize
  /// the array.
  ///
  /// If the `preserve` flag is `viskores::CopyFlag::On`, then any data that existed before the
  /// call to `AllocateAndFill` will remain after the call (assuming the new array size is
  /// large enough). If the array size is expanded, then the new values at the end will be
  /// filled.
  ///
  /// If the `preserve` flag is `viskores::CopyFlag::Off` (the default), the entire array is
  /// filled with the given `fillValue`.
  ///
  VISKORES_CONT void AllocateAndFill(viskores::Id numberOfValues,
                                     const ValueType& fillValue,
                                     viskores::CopyFlag preserve,
                                     viskores::cont::Token& token) const
  {
    // Note that there is a slight potential for a race condition here. It is possible for someone
    // else to resize the array in between getting the startIndex and locking the array in the
    // Allocate call. If there really are 2 threads trying to allocate this array at the same time,
    // you probably have bigger problems than filling at the wrong index.
    viskores::Id startIndex = (preserve == viskores::CopyFlag::On) ? this->GetNumberOfValues() : 0;

    this->Allocate(numberOfValues, preserve, token);

    if (startIndex < numberOfValues)
    {
      this->Fill(fillValue, startIndex, numberOfValues, token);
    }
  }

  /// @copydoc AllocateAndFill
  VISKORES_CONT void AllocateAndFill(viskores::Id numberOfValues,
                                     const ValueType& fillValue,
                                     viskores::CopyFlag preserve = viskores::CopyFlag::Off) const
  {
    viskores::cont::Token token;
    this->AllocateAndFill(numberOfValues, fillValue, preserve, token);
  }

  /// @brief Fills the array with a given value.
  ///
  /// After calling this method, every entry in the array from `startIndex` (inclusive)
  /// to `endIndex` (exclusive) of the array is set to `fillValue`. If `startIndex` or
  /// `endIndex` is not specified, then the fill happens from the begining or end,
  /// respectively.
  ///
  VISKORES_CONT void Fill(const ValueType& fillValue,
                          viskores::Id startIndex,
                          viskores::Id endIndex,
                          viskores::cont::Token& token) const
  {
    StorageType::Fill(this->GetBuffers(), fillValue, startIndex, endIndex, token);
  }
  /// @copydoc Fill
  VISKORES_CONT void Fill(const ValueType& fillValue,
                          viskores::Id startIndex,
                          viskores::Id endIndex) const
  {
    viskores::cont::Token token;
    this->Fill(fillValue, startIndex, endIndex, token);
  }
  /// @copydoc Fill
  VISKORES_CONT void Fill(const ValueType& fillValue, viskores::Id startIndex = 0) const
  {
    viskores::cont::Token token;
    this->Fill(fillValue, startIndex, this->GetNumberOfValues(), token);
  }

  /// Releases any resources being used in the execution environment (that are
  /// not being shared by the control environment).
  ///
  VISKORES_CONT void ReleaseResourcesExecution() const
  {
    detail::ArrayHandleReleaseResourcesExecution(this->Buffers);
  }

  /// Releases all resources in both the control and execution environments.
  ///
  VISKORES_CONT void ReleaseResources() const { this->Allocate(0); }

  /// Prepares this array to be used as an input to an operation in the
  /// execution environment. If necessary, copies data to the execution
  /// environment. Can throw an exception if this array does not yet contain
  /// any data. Returns a portal that can be used in code running in the
  /// execution environment.
  ///
  /// The `Token` object provided will be attached to this `ArrayHandle`.
  /// The returned portal is guaranteed to be valid while the `Token` is
  /// still attached and in scope. Other operations on this `ArrayHandle`
  /// that would invalidate the returned portal will block until the `Token`
  /// is released. Likewise, this method will block if another `Token` is
  /// already attached. This can potentially lead to deadlocks.
  ///
  VISKORES_CONT ReadPortalType PrepareForInput(viskores::cont::DeviceAdapterId device,
                                               viskores::cont::Token& token) const
  {
    return StorageType::CreateReadPortal(this->GetBuffers(), device, token);
  }

  /// Prepares this array to be used in an in-place operation (both as input
  /// and output) in the execution environment. If necessary, copies data to
  /// the execution environment. Can throw an exception if this array does not
  /// yet contain any data. Returns a portal that can be used in code running
  /// in the execution environment.
  ///
  /// The `Token` object provided will be attached to this `ArrayHandle`.
  /// The returned portal is guaranteed to be valid while the `Token` is
  /// still attached and in scope. Other operations on this `ArrayHandle`
  /// that would invalidate the returned portal will block until the `Token`
  /// is released. Likewise, this method will block if another `Token` is
  /// already attached. This can potentially lead to deadlocks.
  ///
  VISKORES_CONT WritePortalType PrepareForInPlace(viskores::cont::DeviceAdapterId device,
                                                  viskores::cont::Token& token) const
  {
    return StorageType::CreateWritePortal(this->GetBuffers(), device, token);
  }

  /// Prepares (allocates) this array to be used as an output from an operation
  /// in the execution environment. The internal state of this class is set to
  /// have valid data in the execution array with the assumption that the array
  /// will be filled soon (i.e. before any other methods of this object are
  /// called). Returns a portal that can be used in code running in the
  /// execution environment.
  ///
  /// The `Token` object provided will be attached to this `ArrayHandle`.
  /// The returned portal is guaranteed to be valid while the `Token` is
  /// still attached and in scope. Other operations on this `ArrayHandle`
  /// that would invalidate the returned portal will block until the `Token`
  /// is released. Likewise, this method will block if another `Token` is
  /// already attached. This can potentially lead to deadlocks.
  ///
  VISKORES_CONT WritePortalType PrepareForOutput(viskores::Id numberOfValues,
                                                 viskores::cont::DeviceAdapterId device,
                                                 viskores::cont::Token& token) const
  {
    this->Allocate(numberOfValues, viskores::CopyFlag::Off, token);
    return StorageType::CreateWritePortal(this->GetBuffers(), device, token);
  }

  /// Returns true if the ArrayHandle's data is on the given device. If the data are on the given
  /// device, then preparing for that device should not require any data movement.
  ///
  VISKORES_CONT bool IsOnDevice(viskores::cont::DeviceAdapterId device) const
  {
    return detail::ArrayHandleIsOnDevice(this->Buffers, device);
  }

  /// Returns true if the ArrayHandle's data is on the host. If the data are on the given
  /// device, then calling `ReadPortal` or `WritePortal` should not require any data movement.
  ///
  VISKORES_CONT bool IsOnHost() const
  {
    return this->IsOnDevice(viskores::cont::DeviceAdapterTagUndefined{});
  }

  /// Synchronizes the control array with the execution array. If either the
  /// user array or control array is already valid, this method does nothing
  /// (because the data is already available in the control environment).
  /// Although the internal state of this class can change, the method is
  /// declared const because logically the data does not.
  ///
  VISKORES_CONT void SyncControlArray() const
  {
    // Creating a host read portal will force the data to be synced to the host.
    this->ReadPortal();
  }

  /// \brief Enqueue a token for access to this ArrayHandle.
  ///
  /// This method places the given `Token` into the queue of `Token`s waiting for
  /// access to this `ArrayHandle` and then returns immediately. When this token
  /// is later used to get data from this `ArrayHandle` (for example, in a call to
  /// `PrepareForInput`), it will use this place in the queue while waiting for
  /// access.
  ///
  /// This method is to be used to ensure that a set of accesses to an `ArrayHandle`
  /// that happen on multiple threads occur in a specified order. For example, if
  /// you spawn of a job to modify data in an `ArrayHandle` and then spawn off a job
  /// that reads that same data, you need to make sure that the first job gets
  /// access to the `ArrayHandle` before the second. If they both just attempt to call
  /// their respective `Prepare` methods, there is no guarantee which order they
  /// will occur. Having the spawning thread first call this method will ensure the order.
  ///
  /// \warning After calling this method it is required to subsequently
  /// call a method like one of the `Prepare` methods that attaches the token
  /// to this `ArrayHandle`. Otherwise, the enqueued token will block any subsequent
  /// access to the `ArrayHandle`, even if the `Token` is destroyed.
  ///
  VISKORES_CONT void Enqueue(const viskores::cont::Token& token) const
  {
    for (auto&& buffer : this->Buffers)
    {
      buffer.Enqueue(token);
    }
  }

  /// \brief Deep copies the data in the array.
  ///
  /// Takes the data that is in \a source and copies that data into this array.
  ///
  VISKORES_CONT void DeepCopyFrom(
    const viskores::cont::ArrayHandle<ValueType, StorageTag>& source) const
  {
    VISKORES_ASSERT(this->Buffers.size() == source.Buffers.size());

    for (std::size_t bufferIndex = 0; bufferIndex < this->Buffers.size(); ++bufferIndex)
    {
      this->Buffers[bufferIndex].DeepCopyFrom(source.Buffers[bufferIndex]);
    }
  }

  /// \brief Returns the internal `Buffer` structures that hold the data.
  ///
  /// Note that great care should be taken when modifying buffers outside of the ArrayHandle.
  ///
  VISKORES_CONT const std::vector<viskores::cont::internal::Buffer>& GetBuffers() const
  {
    return this->Buffers;
  }
  VISKORES_CONT std::vector<viskores::cont::internal::Buffer>& GetBuffers()
  {
    return this->Buffers;
  }

private:
  mutable std::vector<viskores::cont::internal::Buffer> Buffers;

protected:
  VISKORES_CONT void SetBuffer(viskores::IdComponent index,
                               const viskores::cont::internal::Buffer& buffer)
  {
    this->Buffers[static_cast<std::size_t>(index)] = buffer;
  }

  VISKORES_CONT void SetBuffers(const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    this->Buffers = buffers;
  }
  VISKORES_CONT void SetBuffers(std::vector<viskores::cont::internal::Buffer>&& buffers)
  {
    this->Buffers = std::move(buffers);
  }
};

namespace detail
{

template <typename T>
VISKORES_NEVER_EXPORT VISKORES_CONT inline void printSummary_ArrayHandle_Value(
  const T& value,
  std::ostream& out,
  viskores::VecTraitsTagSingleComponent)
{
  out << value;
}

VISKORES_NEVER_EXPORT
VISKORES_CONT
inline void printSummary_ArrayHandle_Value(viskores::UInt8 value,
                                           std::ostream& out,
                                           viskores::VecTraitsTagSingleComponent)
{
  out << static_cast<int>(value);
}

VISKORES_NEVER_EXPORT
VISKORES_CONT
inline void printSummary_ArrayHandle_Value(viskores::Int8 value,
                                           std::ostream& out,
                                           viskores::VecTraitsTagSingleComponent)
{
  out << static_cast<int>(value);
}

template <typename T>
VISKORES_NEVER_EXPORT VISKORES_CONT inline void printSummary_ArrayHandle_Value(
  const T& value,
  std::ostream& out,
  viskores::VecTraitsTagMultipleComponents)
{
  using Traits = viskores::VecTraits<T>;
  using ComponentType = typename Traits::ComponentType;
  using IsVecOfVec = typename viskores::VecTraits<ComponentType>::HasMultipleComponents;
  viskores::IdComponent numComponents = Traits::GetNumberOfComponents(value);
  out << "(";
  printSummary_ArrayHandle_Value(Traits::GetComponent(value, 0), out, IsVecOfVec());
  for (viskores::IdComponent index = 1; index < numComponents; ++index)
  {
    out << ",";
    printSummary_ArrayHandle_Value(Traits::GetComponent(value, index), out, IsVecOfVec());
  }
  out << ")";
}

template <typename T1, typename T2>
VISKORES_NEVER_EXPORT VISKORES_CONT inline void printSummary_ArrayHandle_Value(
  const viskores::Pair<T1, T2>& value,
  std::ostream& out,
  viskores::VecTraitsTagSingleComponent)
{
  out << "{";
  printSummary_ArrayHandle_Value(
    value.first, out, typename viskores::VecTraits<T1>::HasMultipleComponents());
  out << ",";
  printSummary_ArrayHandle_Value(
    value.second, out, typename viskores::VecTraits<T2>::HasMultipleComponents());
  out << "}";
}



} // namespace detail

template <typename T, typename StorageT>
VISKORES_NEVER_EXPORT VISKORES_CONT inline void printSummary_ArrayHandle(
  const viskores::cont::ArrayHandle<T, StorageT>& array,
  std::ostream& out,
  bool full = false)
{
  using ArrayType = viskores::cont::ArrayHandle<T, StorageT>;
  using PortalType = typename ArrayType::ReadPortalType;
  using IsVec = typename viskores::VecTraits<T>::HasMultipleComponents;

  viskores::Id sz = array.GetNumberOfValues();

  out << "valueType=" << viskores::cont::TypeToString<T>()
      << " storageType=" << viskores::cont::TypeToString<StorageT>() << " " << sz
      << " values occupying " << (static_cast<size_t>(sz) * sizeof(T)) << " bytes [";

  PortalType portal = array.ReadPortal();
  if (full || sz <= 7)
  {
    for (viskores::Id i = 0; i < sz; i++)
    {
      detail::printSummary_ArrayHandle_Value(portal.Get(i), out, IsVec());
      if (i != (sz - 1))
      {
        out << " ";
      }
    }
  }
  else
  {
    detail::printSummary_ArrayHandle_Value(portal.Get(0), out, IsVec());
    out << " ";
    detail::printSummary_ArrayHandle_Value(portal.Get(1), out, IsVec());
    out << " ";
    detail::printSummary_ArrayHandle_Value(portal.Get(2), out, IsVec());
    out << " ... ";
    detail::printSummary_ArrayHandle_Value(portal.Get(sz - 3), out, IsVec());
    out << " ";
    detail::printSummary_ArrayHandle_Value(portal.Get(sz - 2), out, IsVec());
    out << " ";
    detail::printSummary_ArrayHandle_Value(portal.Get(sz - 1), out, IsVec());
  }
  out << "]\n";
}

namespace internal
{

namespace detail
{

VISKORES_CONT inline void CreateBuffersImpl(std::vector<viskores::cont::internal::Buffer>&);
template <typename T, typename S, typename... Args>
VISKORES_CONT inline void CreateBuffersImpl(std::vector<viskores::cont::internal::Buffer>& buffers,
                                            const viskores::cont::ArrayHandle<T, S>& array,
                                            const Args&... args);
template <typename... Args>
VISKORES_CONT inline void CreateBuffersImpl(std::vector<viskores::cont::internal::Buffer>& buffers,
                                            const viskores::cont::internal::Buffer& buffer,
                                            const Args&... args);

template <typename... Args>
VISKORES_CONT inline void CreateBuffersImpl(
  std::vector<viskores::cont::internal::Buffer>& buffers,
  const std::vector<viskores::cont::internal::Buffer>& addbuffs,
  const Args&... args);
template <typename Arg0, typename... Args>
VISKORES_CONT inline void CreateBuffersImpl(std::vector<viskores::cont::internal::Buffer>& buffers,
                                            const Arg0& arg0,
                                            const Args&... args);

VISKORES_CONT inline void CreateBuffersImpl(std::vector<viskores::cont::internal::Buffer>&)
{
  // Nothing left to add.
}

template <typename T, typename S, typename... Args>
VISKORES_CONT inline void CreateBuffersImpl(std::vector<viskores::cont::internal::Buffer>& buffers,
                                            const viskores::cont::ArrayHandle<T, S>& array,
                                            const Args&... args)
{
  CreateBuffersImpl(buffers, array.GetBuffers(), args...);
}

template <typename... Args>
VISKORES_CONT inline void CreateBuffersImpl(std::vector<viskores::cont::internal::Buffer>& buffers,
                                            const viskores::cont::internal::Buffer& buffer,
                                            const Args&... args)
{
  buffers.push_back(buffer);
  CreateBuffersImpl(buffers, args...);
}

template <typename... Args>
VISKORES_CONT inline void CreateBuffersImpl(
  std::vector<viskores::cont::internal::Buffer>& buffers,
  const std::vector<viskores::cont::internal::Buffer>& addbuffs,
  const Args&... args)
{
  buffers.insert(buffers.end(), addbuffs.begin(), addbuffs.end());
  CreateBuffersImpl(buffers, args...);
}

template <typename T, typename S, typename... Args>
VISKORES_CONT inline void CreateBuffersResolveArrays(
  std::vector<viskores::cont::internal::Buffer>& buffers,
  std::true_type,
  const viskores::cont::ArrayHandle<T, S>& array,
  const Args&... args)
{
  CreateBuffersImpl(buffers, array, args...);
}

template <typename MetaData, typename... Args>
VISKORES_CONT inline void CreateBuffersResolveArrays(
  std::vector<viskores::cont::internal::Buffer>& buffers,
  std::false_type,
  const MetaData& metadata,
  const Args&... args)
{
  viskores::cont::internal::Buffer buffer;
  buffer.SetMetaData(metadata);
  buffers.push_back(std::move(buffer));
  CreateBuffersImpl(buffers, args...);
}

template <typename Arg0, typename... Args>
VISKORES_CONT inline void CreateBuffersImpl(std::vector<viskores::cont::internal::Buffer>& buffers,
                                            const Arg0& arg0,
                                            const Args&... args)
{
  // If the argument is a subclass of ArrayHandle, the template resolution will pick this
  // overload instead of the correct ArrayHandle overload. To resolve that, check to see
  // if the type is an `ArrayHandle` and use `CreateBuffersResolveArrays` to choose the
  // right path.
  using IsArray = typename viskores::cont::internal::ArrayHandleCheck<Arg0>::type::type;
  CreateBuffersResolveArrays(buffers, IsArray{}, arg0, args...);
}

} // namespace detail

/// \brief Create the buffers for an `ArrayHandle` specialization.
///
/// When creating an `ArrayHandle` specialization, it is important to build a
/// `std::vector` of `Buffer` objects. This function simplifies creating
/// these buffer objects. Simply pass as arguments the things you want in the
/// buffers. The parameters to `CreateBuffers` are added to the `Buffer` `vector`
/// in the order provided. The actual object(s) added depends on the type of
/// parameter:
///
///   - `ArrayHandle`: The buffers from the `ArrayHandle` are added to the list.
///   - `Buffer`: A copy of the buffer is added to the list.
///   - `std::vector<Buffer>`: A copy of all buffers in this vector are added to the list.
///   - Anything else: A buffer with the given object attached as metadata is added to the list.
///
template <typename... Args>
VISKORES_CONT inline std::vector<viskores::cont::internal::Buffer> CreateBuffers(
  const Args&... args)
{
  std::vector<viskores::cont::internal::Buffer> buffers;
  buffers.reserve(sizeof...(args));
  detail::CreateBuffersImpl(buffers, args...);
  return buffers;
}

} // namespace internal

}
} //namespace viskores::cont

#ifndef viskores_cont_ArrayHandleBasic_h
#include <viskores/cont/ArrayHandleBasic.h>
#endif

#endif //viskores_cont_ArrayHandle_h
