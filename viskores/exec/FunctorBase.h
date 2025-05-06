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
#ifndef viskores_exec_FunctorBase_h
#define viskores_exec_FunctorBase_h

#include <viskores/Types.h>

#include <viskores/exec/internal/ErrorMessageBuffer.h>

#include <viskores/cont/viskores_cont_export.h>

namespace viskores
{
namespace exec
{

/// Base class for all user worklets invoked in the execution environment from a
/// call to viskores::cont::DeviceAdapterAlgorithm::Schedule.
///
/// This class contains a public method named RaiseError that can be called in
/// the execution environment to signal a problem.
///
class VISKORES_ALWAYS_EXPORT FunctorBase
{
public:
  VISKORES_EXEC_CONT
  FunctorBase() {}

  VISKORES_EXEC
  void RaiseError(const char* message) const { this->ErrorMessage.RaiseError(message); }

  /// Set the error message buffer so that running algorithms can report
  /// errors. This is supposed to be set by the dispatcher. This method may be
  /// replaced as the execution semantics change.
  ///
  VISKORES_CONT
  void SetErrorMessageBuffer(const viskores::exec::internal::ErrorMessageBuffer& buffer)
  {
    this->ErrorMessage = buffer;
  }

private:
  viskores::exec::internal::ErrorMessageBuffer ErrorMessage;
};
}
} // namespace viskores::exec

#endif //viskores_exec_FunctorBase_h
