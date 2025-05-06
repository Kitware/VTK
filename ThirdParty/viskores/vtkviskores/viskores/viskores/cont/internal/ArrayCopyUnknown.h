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
#ifndef viskores_cont_internal_ArrayCopyUnknown_h
#define viskores_cont_internal_ArrayCopyUnknown_h

#include <viskores/cont/viskores_cont_export.h>

namespace viskores
{
namespace cont
{

// Rather than include UnknownArrayHandle.h, we just forward declare the class so
// we can declare our functions and prevent any circular header dependencies from
// core classes.
class UnknownArrayHandle;

namespace internal
{

/// Same as `ArrayCopy` with `UnknownArrayHandle` except that it can be used without
/// using a device compiler.
///
VISKORES_CONT_EXPORT void ArrayCopyUnknown(const viskores::cont::UnknownArrayHandle& source,
                                           viskores::cont::UnknownArrayHandle& destination);

VISKORES_CONT_EXPORT void ArrayCopyUnknown(const viskores::cont::UnknownArrayHandle& source,
                                           const viskores::cont::UnknownArrayHandle& destination);


} // namespace viskores::cont::internal
} // namespace viskores::cont
} // namespace viskores

#endif //viskores_cont_internal_ArrayCopyUnknown_h
