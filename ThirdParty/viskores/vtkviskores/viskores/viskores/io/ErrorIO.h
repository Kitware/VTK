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
#ifndef viskores_io_ErrorIO_h
#define viskores_io_ErrorIO_h

#include <viskores/cont/Error.h>

namespace viskores
{
namespace io
{

VISKORES_SILENCE_WEAK_VTABLE_WARNING_START

/// This class is thrown when Viskores encounters an error with the file system.
/// This can happen if there is a problem with reading or writing a file such
/// as a bad filename.
class VISKORES_ALWAYS_EXPORT ErrorIO : public viskores::cont::Error
{
public:
  ErrorIO() {}
  ErrorIO(const std::string message)
    : Error(message)
  {
  }
};

VISKORES_SILENCE_WEAK_VTABLE_WARNING_END
}
} // namespace viskores::io

#endif //viskores_io_ErrorIO_h
