//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#ifndef fides_DataContainer_H_
#define fides_DataContainer_H_

#include <memory>
#include <type_traits>
#include <utility>

#include <fides/fides_export.h>

namespace fides
{

/// Type erasing data holder
struct FIDES_EXPORT DataContainer
{
  virtual ~DataContainer() = default;
};

/// Type printer for debugging
namespace internal
{
template <typename Expected, typename Received>
struct Conversion_Error;
}

/// Templated wrapper to hold arbitrary type
template <typename T>
struct FIDES_EXPORT ConcreteDataWrapper : public DataContainer
{
  T Data;

  // Perfect forwarding constructor
  template <typename U>
  explicit ConcreteDataWrapper(U&& d)
    : Data(std::forward<U>(d))
  {
    // Try to be helpful in situations where compiler can't construct a T from a U
    static_assert(std::is_constructible_v<T, U>,
                  "Fides ABI Error: Wrapper storage type T cannot be initialized by type U.");
    if constexpr (!std::is_constructible_v<T, U>)
    {
      internal::Conversion_Error<T, U> print_types;
      (void)print_types;
    }
  }
};

/// Type-safe retrieval of wrapped data from a container. Returns nullptr
/// if the type T does not match the stored data.
template <typename T>
T* GetDataAs(DataContainer& container)
{
  static_assert(!std::is_pointer_v<T>, "GetDataAs expects raw type, not pointer.");
  auto* wrapper = dynamic_cast<ConcreteDataWrapper<T>*>(&container);
  return wrapper ? &(wrapper->Data) : nullptr;
}

}

#endif
