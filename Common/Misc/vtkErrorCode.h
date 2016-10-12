/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkErrorCode.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
class VTKCOMMONMISC_EXPORT vtkErrorCode
{
public:
  static const char *GetStringFromErrorCode(unsigned long event);
  static unsigned long GetErrorCodeFromString(const char *event);
  static unsigned long GetLastSystemError();

  // all the currently defined error codes
  // developers can use -- vtkErrorCode::UserError + int to
  // specify their own errors.
  // if this list is adjusted, be sure to adjust vtkErrorCodeErrorStrings
  // in vtkErrorCode.cxx to match.
  enum ErrorIds {
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

#endif /* vtkErrorCode_h */

// VTK-HeaderTest-Exclude: vtkErrorCode.h
