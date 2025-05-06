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
#ifndef viskores_cont_DeviceAdapterTag_h
#define viskores_cont_DeviceAdapterTag_h

#include <viskores/StaticAssert.h>
#include <viskores/Types.h>
#include <viskores/internal/Configure.h>
#include <viskores/internal/ExportMacros.h>

#include <viskores/cont/viskores_cont_export.h>

#include <string>

#ifdef VISKORES_DEVICE_ADAPTER
// Rather than use defines to specify the default device adapter
// Viskores now builds for all device adapters and uses runtime controls
// to determine where execution occurs
#error The VISKORES_DEVICE_ADAPTER define is no longer obeyed and needs to be removed
#endif
#ifdef VISKORES_DEFAULT_DEVICE_ADAPTER_TAG
// Rather than use device adapter tag that had no shared parent
// Viskores now uses a runtime device adapter implementation that
// allows for runtime execution selection of what device to execute on
#error The VISKORES_DEFAULT_DEVICE_ADAPTER_TAG define is no longer required and needs to be removed
#endif

#define VISKORES_DEVICE_ADAPTER_UNDEFINED -1
#define VISKORES_DEVICE_ADAPTER_SERIAL 1
#define VISKORES_DEVICE_ADAPTER_CUDA 2
#define VISKORES_DEVICE_ADAPTER_TBB 3
#define VISKORES_DEVICE_ADAPTER_OPENMP 4
#define VISKORES_DEVICE_ADAPTER_KOKKOS 5
//VISKORES_DEVICE_ADAPTER_TestAlgorithmGeneral 7
#define VISKORES_MAX_DEVICE_ADAPTER_ID 8
#define VISKORES_DEVICE_ADAPTER_ANY 127

namespace viskores
{
namespace cont
{

using DeviceAdapterNameType = std::string;

/// @brief An object used to specify a device.
///
/// `viskores::cont::DeviceAdapterId` can be used to specify a device to use when
/// executing some code. Each `DeviceAdapterTag` object inherits from
/// `viskores::cont::DeviceAdapterId`. Functions can accept a `viskores::cont::DeviceAdapterId`
/// object rather than a templated tag to select a device adapter at runtime.
struct DeviceAdapterId
{
  constexpr bool operator==(DeviceAdapterId other) const { return this->Value == other.Value; }
  constexpr bool operator!=(DeviceAdapterId other) const { return this->Value != other.Value; }
  constexpr bool operator<(DeviceAdapterId other) const { return this->Value < other.Value; }

  /// @brief Return whether this object represents a valid type of device.
  ///
  /// This method will return true if the id represents a specific, valid device.
  /// It will return true even if the device is disabled in by the runtime tracker
  /// or if the device is not supported by the Viskores build configuration.
  ///
  /// It should be noted that this method return false for tags that are not specific
  /// devices. This includes `viskores::cont::DeviceAdapterTagAny` and
  /// `viskores::cont::DeviceAdapterTagUndefined`.
  constexpr bool IsValueValid() const
  {
    return this->Value > 0 && this->Value < VISKORES_MAX_DEVICE_ADAPTER_ID;
  }

  /// @brief Returns the numeric value of the index.
  constexpr viskores::Int8 GetValue() const { return this->Value; }

  /// @brief Return a name representing the device.
  ///
  /// The string returned from this method is stored in a type named
  /// `viskores::cont::DeviceAdapterNameType`, which is currently aliased to
  /// `std::string`. The device adapter name is useful for printing information
  /// about a device being used.
  VISKORES_CONT_EXPORT
  DeviceAdapterNameType GetName() const;

protected:
  friend DeviceAdapterId make_DeviceAdapterId(viskores::Int8 id);

  constexpr explicit DeviceAdapterId(viskores::Int8 id)
    : Value(id)
  {
  }

private:
  viskores::Int8 Value;
};

/// Construct a device adapter id from a runtime string
/// The string is case-insensitive. So CUDA will be selected with 'cuda', 'Cuda', or 'CUDA'.
VISKORES_CONT_EXPORT
DeviceAdapterId make_DeviceAdapterId(const DeviceAdapterNameType& name);

/// Construct a device adapter id a viskores::Int8.
/// The mapping of integer value to devices are:
///
/// DeviceAdapterTagSerial == 1
/// DeviceAdapterTagCuda == 2
/// DeviceAdapterTagTBB == 3
/// DeviceAdapterTagOpenMP == 4
/// DeviceAdapterTagKokkos == 5
///
inline DeviceAdapterId make_DeviceAdapterId(viskores::Int8 id)
{
  return DeviceAdapterId(id);
}

template <typename DeviceAdapter>
struct DeviceAdapterTraits;
}
}

/// Creates a tag named viskores::cont::DeviceAdapterTagName and associated MPL
/// structures to use this tag. Always use this macro (in the base namespace)
/// when creating a device adapter.
#define VISKORES_VALID_DEVICE_ADAPTER(Name, Id)                          \
  namespace viskores                                                     \
  {                                                                      \
  namespace cont                                                         \
  {                                                                      \
  struct VISKORES_ALWAYS_EXPORT DeviceAdapterTag##Name : DeviceAdapterId \
  {                                                                      \
    constexpr DeviceAdapterTag##Name()                                   \
      : DeviceAdapterId(Id)                                              \
    {                                                                    \
    }                                                                    \
    static constexpr bool IsEnabled = true;                              \
  };                                                                     \
  template <>                                                            \
  struct DeviceAdapterTraits<viskores::cont::DeviceAdapterTag##Name>     \
  {                                                                      \
    static DeviceAdapterNameType GetName()                               \
    {                                                                    \
      return DeviceAdapterNameType(#Name);                               \
    }                                                                    \
  };                                                                     \
  }                                                                      \
  }

/// Marks the tag named viskores::cont::DeviceAdapterTagName and associated
/// structures as invalid to use. Always use this macro (in the base namespace)
/// when creating a device adapter.
#define VISKORES_INVALID_DEVICE_ADAPTER(Name, Id)                        \
  namespace viskores                                                     \
  {                                                                      \
  namespace cont                                                         \
  {                                                                      \
  struct VISKORES_ALWAYS_EXPORT DeviceAdapterTag##Name : DeviceAdapterId \
  {                                                                      \
    constexpr DeviceAdapterTag##Name()                                   \
      : DeviceAdapterId(Id)                                              \
    {                                                                    \
    }                                                                    \
    static constexpr bool IsEnabled = false;                             \
  };                                                                     \
  template <>                                                            \
  struct DeviceAdapterTraits<viskores::cont::DeviceAdapterTag##Name>     \
  {                                                                      \
    static DeviceAdapterNameType GetName()                               \
    {                                                                    \
      return DeviceAdapterNameType(#Name);                               \
    }                                                                    \
  };                                                                     \
  }                                                                      \
  }

/// @struct viskores::cont::DeviceAdapterTagAny
/// @brief Tag for a device adapter used to specify that any device may be used
/// for an operation.
///
/// In practice this is limited to devices that are currently available.

VISKORES_VALID_DEVICE_ADAPTER(Any, VISKORES_DEVICE_ADAPTER_ANY)

/// @struct viskores::cont::DeviceAdapterTagUndefined
/// @brief Tag for a device adapter used to avoid specifying a device.
///
/// Useful as a placeholder when a device can be specified but none is given.

VISKORES_INVALID_DEVICE_ADAPTER(Undefined, VISKORES_DEVICE_ADAPTER_UNDEFINED)

/// Checks that the argument is a proper device adapter tag. This is a handy
/// concept check for functions and classes to make sure that a template
/// argument is actually a device adapter tag. (You can get weird errors
/// elsewhere in the code when a mistake is made.)
///
#define VISKORES_IS_DEVICE_ADAPTER_TAG(tag)                                     \
  static_assert(std::is_base_of<viskores::cont::DeviceAdapterId, tag>::value && \
                  !std::is_same<viskores::cont::DeviceAdapterId, tag>::value,   \
                "Provided type is not a valid Viskores device adapter tag.")

#endif //viskores_cont_DeviceAdapterTag_h
