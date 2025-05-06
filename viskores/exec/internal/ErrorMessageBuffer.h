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
#ifndef viskores_exec_internal_ErrorMessageBuffer_h
#define viskores_exec_internal_ErrorMessageBuffer_h

#include <viskores/Types.h>

namespace viskores
{
namespace exec
{
namespace internal
{

/// Used to hold an error in the execution environment until the parallel
/// execution can complete. This is to be used in conjunction with a
/// DeviceAdapter's Schedule function to implement errors in execution
/// environments that cannot throw errors. This string should be global to all
/// threads. If the first entry in the string is '\0' (the C string
/// terminator), then we consider it as no error. Otherwise, the array contains
/// the string describing the error.
///
/// Before scheduling worklets, the global array should be cleared to have no
/// error. This can only be reliably done by the device adapter.
///
class VISKORES_ALWAYS_EXPORT ErrorMessageBuffer
{
public:
  VISKORES_EXEC_CONT ErrorMessageBuffer()
    : MessageBuffer(nullptr)
    , MessageBufferSize(0)
  {
  }

  VISKORES_EXEC_CONT
  ErrorMessageBuffer(char* messageBuffer, viskores::Id bufferSize)
    : MessageBuffer(messageBuffer)
    , MessageBufferSize(bufferSize)
  {
  }

  VISKORES_EXEC void RaiseError(const char* message) const
  {
    // Only raise the error if one has not been raised yet. This check is not
    // guaranteed to work across threads. However, chances are that if two or
    // more threads simultaneously pass this test, they will be writing the
    // same error, which is fine. Even in the much less likely case that two
    // threads simultaneously write different error messages, the worst case is
    // that you get a mangled message. That's not good (and it's what we are
    // trying to avoid), but it's not critical.
    if (this->IsErrorRaised())
    {
      return;
    }

    // Safely copy message into array.
    for (viskores::Id index = 0; index < this->MessageBufferSize; index++)
    {
      this->MessageBuffer[index] = message[index];
      if (message[index] == '\0')
      {
        break;
      }
    }

    // Make sure message is null terminated.
    this->MessageBuffer[this->MessageBufferSize - 1] = '\0';
  }

  VISKORES_EXEC_CONT bool IsErrorRaised() const
  {
    if (this->MessageBufferSize > 0)
    {
      return (this->MessageBuffer[0] != '\0');
    }
    else
    {
      // If there is no buffer set, then always report an error.
      return true;
    }
  }

private:
  char* MessageBuffer;
  viskores::Id MessageBufferSize;
};
}
}
} // namespace viskores::exec::internal

#endif // viskores_exec_internal_ErrorMessageBuffer_h
