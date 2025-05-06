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
#ifndef viskores_cont_internal_OptionParser_h
#define viskores_cont_internal_OptionParser_h

// For warning suppression macros:
#include <viskores/internal/ExportMacros.h>

VISKORES_THIRDPARTY_PRE_INCLUDE

// Preemptively load any includes required by optionparser.h so they don't get embedded in
// our namespace.
#ifdef _MSC_VER
#include <intrin.h>
#pragma intrinsic(_BitScanReverse)
#endif

// We are embedding the code in optionparser.h in a Viskores namespace so that if other code
// is using a different version the two don't get mixed up.

namespace viskores
{
namespace cont
{
namespace internal
{


// Check to make sure that optionparser.h has not been included before. If it has, remove its
// header guard so we can include it again under our namespace.
#ifdef OPTIONPARSER_H_
#undef OPTIONPARSER_H_
#define VISKORES_REMOVED_OPTIONPARSER_HEADER_GUARD
#endif

// Include from third party.
// @cond NONE
#include <viskores/thirdparty/optionparser/viskoresoptionparser/optionparser.h>
// @endcond

// Now restore the header guards as before so that other includes of (possibly different versions
// of) optionparser.h work as expected.
#ifdef VISKORES_REMOVED_OPTIONPARSER_HEADER_GUARD
// Keep header guard, but remove the macro we defined to detect that it was there.
#undef VISKORES_REMOVED_OPTIONPARSER_HEADER_GUARD
#else
// Remove the header guard for other inclusion.
#undef OPTIONPARSER_H_
#endif

} // namespace viskores::cont::internal
} // namespace viskores::cont
} // namespace viskores

VISKORES_THIRDPARTY_POST_INCLUDE

#endif //viskores_cont_internal_OptionParser_h
