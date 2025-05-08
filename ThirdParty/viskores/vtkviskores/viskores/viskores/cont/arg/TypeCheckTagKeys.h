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
#ifndef viskores_cont_arg_TypeCheckTagKeys_h
#define viskores_cont_arg_TypeCheckTagKeys_h

#include <viskores/cont/arg/TypeCheck.h>

namespace viskores
{
namespace cont
{
namespace arg
{

/// Check for a Keys object.
///
struct TypeCheckTagKeys
{
};

// The specialization that actually checks for Keys types is implemented in viskores/worklet/Keys.h.
// That class is not accessible from here due to Viskores package dependencies.
}
}
} // namespace viskores::cont::arg

#endif //viskores_cont_arg_TypeCheckTagKeys_h
