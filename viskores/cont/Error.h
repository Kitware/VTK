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
#ifndef viskores_cont_Error_h
#define viskores_cont_Error_h

// Note that this class and (most likely) all of its subclasses are not
// templated.  If there is any reason to create a Viskores control library,
// this class and its subclasses should probably go there.

#include <exception>
#include <string>

#include <viskores/cont/Logging.h>

#include <viskores/internal/ExportMacros.h>

namespace viskores
{
namespace cont
{

VISKORES_SILENCE_WEAK_VTABLE_WARNING_START

/// The superclass of all exceptions thrown by any Viskores function or method.
///
class VISKORES_ALWAYS_EXPORT Error : public std::exception
{
public:
//See note about GetMessage macro below.
#ifndef GetMessage
  /// @brief Returns a message describing what caused the error.
  const std::string& GetMessage() const { return this->Message; }
#endif
  /// @brief Provides a stack trace to the location where this error was thrown.
  const std::string& GetStackTrace() const { return this->StackTrace; }

//GetMessage is a macro defined by <windows.h> to redirrect to
//GetMessageA or W depending on if you are using ansi or unicode.
//To get around this we make our own A/W variants on windows.
#ifdef _WIN32
  const std::string& GetMessageA() const { return this->Message; }
  const std::string& GetMessageW() const { return this->Message; }
#endif

  /// @brief Returns the message for the error and the stack trace for it.
  ///
  /// This method is provided for `std::exception` compatibility.
  const char* what() const noexcept override { return this->What.c_str(); }

  /// Returns true if this exception is device independent. For exceptions that
  /// are not device independent, `viskores::TryExecute`, for example, may try
  /// executing the code on other available devices.
  bool GetIsDeviceIndependent() const { return this->IsDeviceIndependent; }

protected:
  Error()
    : StackTrace(viskores::cont::GetStackTrace(1))
    , What("Undescribed error\n" + StackTrace)
    , IsDeviceIndependent(false)
  {
  }
  Error(const std::string& message, bool is_device_independent = false)
    : Message(message)
    , StackTrace(viskores::cont::GetStackTrace(1))
    , What(Message + "\n" + StackTrace)
    , IsDeviceIndependent(is_device_independent)
  {
  }

  void SetMessage(const std::string& message)
  {
    this->Message = message;
    this->What = this->Message + "\n" + this->StackTrace;
  }

private:
  std::string Message;
  std::string StackTrace;
  std::string What;
  bool IsDeviceIndependent;
};

VISKORES_SILENCE_WEAK_VTABLE_WARNING_END
}
} // namespace viskores::cont

#endif //viskores_cont_Error_h
