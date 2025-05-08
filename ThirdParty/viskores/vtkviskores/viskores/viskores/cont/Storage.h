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
#ifndef viskores_cont_Storage_h
#define viskores_cont_Storage_h

#define VISKORES_STORAGE_ERROR -2
#define VISKORES_STORAGE_UNDEFINED -1
#define VISKORES_STORAGE_BASIC 1

#ifndef VISKORES_STORAGE
#define VISKORES_STORAGE VISKORES_STORAGE_BASIC
#endif

#include <viskores/Flags.h>
#include <viskores/StaticAssert.h>

#include <viskores/internal/ArrayPortalDummy.h>

#include <viskores/cont/ErrorBadAllocation.h>
#include <viskores/cont/Logging.h>
#include <viskores/cont/Token.h>

#include <viskores/cont/internal/Buffer.h>

namespace viskores
{
namespace cont
{

#ifdef VISKORES_DOXYGEN_ONLY
/// \brief A tag specifying client memory allocation.
///
/// A Storage tag specifies how an ArrayHandle allocates and frees memory. The
/// tag StorageTag___ does not actually exist. Rather, this documentation is
/// provided to describe how array storage objects are specified. Loading the
/// viskores/cont/Storage.h header will set a default array storage. You can
/// specify the default storage by first setting the VISKORES_STORAGE macro.
/// Currently it can only be set to VISKORES_STORAGE_BASIC.
///
/// User code external to Viskores is free to make its own StorageTag. This is a
/// good way to get Viskores to read data directly in and out of arrays from other
/// libraries. However, care should be taken when creating a Storage. One
/// particular problem that is likely is a storage that "constructs" all the
/// items in the array. If done incorrectly, then memory of the array can be
/// incorrectly bound to the wrong processor. If you do provide your own
/// StorageTag, please be diligent in comparing its performance to the
/// StorageTagBasic.
///
/// To implement your own StorageTag, you first must create a tag class (an
/// empty struct) defining your tag (i.e. struct VISKORES_ALWAYS_EXPORT StorageTagMyAlloc { };). Then
/// provide a partial template specialization of viskores::cont::internal::Storage
/// for your new tag. Note that because the StorageTag is being used for
/// template specialization, storage tags cannot use inheritance (or, rather,
/// inheritance won't have any effect). You can, however, have a partial template
/// specialization of viskores::cont::internal::Storage inherit from a different
/// specialization. So, for example, you could not have StorageTagFoo inherit from
/// StorageTagBase, but you could have viskores::cont::internal::Storage<T, StorageTagFoo>
/// inherit from viskores::cont::internal::Storage<T, StorageTagBase>.
///
struct VISKORES_ALWAYS_EXPORT StorageTag___
{
};
#endif // VISKORES_DOXYGEN_ONLY

namespace internal
{

struct UndefinedStorage
{
};

namespace detail
{

// This class should never be used. It is used as a placeholder for undefined
// Storage objects. If you get a compiler error involving this object, then it
// probably comes from trying to use an ArrayHandle with bad template
// arguments.
template <typename T>
struct UndefinedArrayPortal
{
  VISKORES_STATIC_ASSERT(sizeof(T) == static_cast<size_t>(-1));
};

} // namespace detail

/// This templated class must be partially specialized for each StorageTag
/// created, which will define the implementation for that tag.
///
template <typename T, class StorageTag>
class Storage
#ifndef VISKORES_DOXYGEN_ONLY
  : public viskores::cont::internal::UndefinedStorage
{
public:
  using ReadPortalType = viskores::cont::internal::detail::UndefinedArrayPortal<T>;
  using WritePortalType = viskores::cont::internal::detail::UndefinedArrayPortal<T>;
};
#else  //VISKORES_DOXYGEN_ONLY
{
public:
  /// The type of each item in the array.
  ///
  using ValueType = T;

  /// \brief The type of portal objects for the array (read only).
  ///
  using ReadPortalType = viskores::internal::ArrayPortalBasicRead<T>;

  /// \brief The type of portal objects for the array (read/write).
  ///
  using WritePortalType = viskores::internal::ArrayPortalBasicWrite<T>;

  /// \brief Create the buffers for an empty array.
  ///
  /// This is used by the `ArrayHandle` base class when constructed with no arguments.
  /// A convenience subclass may construct the buffers in a different way based on
  /// some provided objects.
  ///
  VISKORES_CONT static std::vector<viskores::cont::internal::Buffer> CreateBuffers();

  /// \brief Resizes the array by changing the size of the buffers.
  ///
  /// Can also modify any metadata attached to the buffers.
  ///
  VISKORES_CONT static void ResizeBuffers(
    viskores::Id numValues,
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::CopyFlag preserve,
    viskores::cont::Token& token);

  /// \brief Returns the number of entries allocated in the array.
  VISKORES_CONT static viskores::Id GetNumberOfValues(
    const std::vector<viskores::cont::internal::Buffer>& buffers);

  /// \brief Fills the array with the given value starting and ending at the given indices.
  ///
  VISKORES_CONT static void Fill(const std::vector<viskores::cont::internal::Buffer>& buffers,
                                 const ValueType& fillValue,
                                 viskores::Id startIndex,
                                 viskores::Id endIndex,
                                 viskores::cont::Token& token);

  /// \brief Create a read-only portal on the specified device.
  ///
  VISKORES_CONT static ReadPortalType CreateReadPortal(
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token);

  /// \brief Create a read/write portal on the specified device.
  ///
  VISKORES_CONT static WritePortalType CreateWritePortal(
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token)
};
#endif // VISKORES_DOXYGEN_ONLY

namespace detail
{

VISKORES_CONT_EXPORT void StorageNoResizeImpl(viskores::Id currentNumValues,
                                              viskores::Id requestedNumValues,
                                              std::string storageTagName);

} // namespace detail

template <typename StorageType>
struct StorageTraits;

template <typename T, typename S>
struct StorageTraits<viskores::cont::internal::Storage<T, S>>
{
  using ValueType = T;
  using Tag = S;
};

#define VISKORES_STORAGE_NO_RESIZE                                          \
  VISKORES_CONT static void ResizeBuffers(                                  \
    viskores::Id numValues,                                                 \
    const std::vector<viskores::cont::internal::Buffer>& buffers,           \
    viskores::CopyFlag,                                                     \
    viskores::cont::Token&)                                                 \
  {                                                                         \
    viskores::cont::internal::detail::StorageNoResizeImpl(                  \
      GetNumberOfValues(buffers),                                           \
      numValues,                                                            \
      viskores::cont::TypeToString<                                         \
        typename viskores::cont::internal::StorageTraits<Storage>::Tag>()); \
  }                                                                         \
  using ResizeBuffersEatComma = void

#define VISKORES_STORAGE_NO_WRITE_PORTAL                                         \
  using WritePortalType = viskores::internal::ArrayPortalDummy<                  \
    typename viskores::cont::internal::StorageTraits<Storage>::ValueType>;       \
  VISKORES_CONT static void Fill(                                                \
    const std::vector<viskores::cont::internal::Buffer>&,                        \
    const typename viskores::cont::internal::StorageTraits<Storage>::ValueType&, \
    viskores::Id,                                                                \
    viskores::Id,                                                                \
    viskores::cont::Token&)                                                      \
  {                                                                              \
    throw viskores::cont::ErrorBadAllocation(                                    \
      "Cannot write to arrays with storage type of " +                           \
      viskores::cont::TypeToString<                                              \
        typename viskores::cont::internal::StorageTraits<Storage>::Tag>());      \
  }                                                                              \
  VISKORES_CONT static WritePortalType CreateWritePortal(                        \
    const std::vector<viskores::cont::internal::Buffer>&,                        \
    viskores::cont::DeviceAdapterId,                                             \
    viskores::cont::Token&)                                                      \
  {                                                                              \
    throw viskores::cont::ErrorBadAllocation(                                    \
      "Cannot write to arrays with storage type of " +                           \
      viskores::cont::TypeToString<                                              \
        typename viskores::cont::internal::StorageTraits<Storage>::Tag>());      \
  }                                                                              \
  using CreateWritePortalEatComma = void

} // namespace internal
}
} // namespace viskores::cont

#endif //viskores_cont_Storage_h
