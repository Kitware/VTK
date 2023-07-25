// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkErrorCode
 * @brief   superclass for error codes
 *
 * vtkErrorCode is an mechanism for (currently) reader object to
 * return errors during reading file.
 */

#ifndef vtkErrorCode_h
#define vtkErrorCode_h
#include "vtkCommonMiscModule.h" // For export macro
#include "vtkSystemIncludes.h"

// The superclass that all commands should be subclasses of
VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONMISC_EXPORT vtkErrorCode
{
public:
  static const char* GetStringFromErrorCode(unsigned long error);
  static unsigned long GetErrorCodeFromString(const char* error);
  static unsigned long GetLastSystemError();

  // all the currently defined error codes
  // developers can use -- vtkErrorCode::UserError + int to
  // specify their own errors.
  // if this list is adjusted, be sure to adjust vtkErrorCodeErrorStrings
  // in vtkErrorCode.cxx to match.
  enum ErrorIds
  {
    NoError = 0,
    FirstVTKErrorCode = 20000,
    FileNotFoundError,
    CannotOpenFileError,
    UnrecognizedFileTypeError,
    PrematureEndOfFileError,
    FileFormatError,
    NoFileNameError,
    OutOfDiskSpaceError,
    UnknownError,
    UserError = 40000
  };
};

VTK_ABI_NAMESPACE_END
#endif /* vtkErrorCode_h */

// VTK-HeaderTest-Exclude: vtkErrorCode.h
