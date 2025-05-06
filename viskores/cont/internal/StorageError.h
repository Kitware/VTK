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
#ifndef viskores_cont_internal_StorageError_h
#define viskores_cont_internal_StorageError_h

#include <viskores/internal/ExportMacros.h>

namespace viskores
{
namespace cont
{
namespace internal
{

/// This is an invalid Storage. The point of this class is to include the
/// header file to make this invalid class the default Storage. From that
/// point, you have to specify an appropriate Storage or else get a compile
/// error.
///
struct VISKORES_ALWAYS_EXPORT StorageTagError
{
  // Not implemented.
};
}
}
} // namespace viskores::cont::internal

#endif //viskores_cont_internal_StorageError_h
