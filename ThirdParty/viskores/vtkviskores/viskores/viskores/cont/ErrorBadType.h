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
#ifndef viskores_cont_ErrorBadType_h
#define viskores_cont_ErrorBadType_h

#include <viskores/cont/Error.h>

namespace viskores
{
namespace cont
{

VISKORES_SILENCE_WEAK_VTABLE_WARNING_START

/// This class is thrown when Viskores encounters data of a type that is
/// incompatible with the current operation.
///
class VISKORES_ALWAYS_EXPORT ErrorBadType : public Error
{
public:
  ErrorBadType(const std::string& message)
    : Error(message, true)
  {
  }
};

VISKORES_SILENCE_WEAK_VTABLE_WARNING_END

/// Throws an ErrorBadType exception with the following message:
/// Cast failed: \c baseType --> \c derivedType".
/// This is generally caused by asking for a casting of a UnknownArrayHandle
/// or UncertainArrayhandle with an insufficient type list.
//
VISKORES_CONT_EXPORT void throwFailedDynamicCast(const std::string& baseType,
                                                 const std::string& derivedType);
}
} // namespace viskores::cont

#endif //viskores_cont_ErrorBadType_h
