/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkErrorCode.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkErrorCode.h"

#include <string.h>
#include <ctype.h>
#include <errno.h>

// this list should only contain the initial, contiguous
// set of error codes and should not include UserError
static const char *vtkErrorCodeErrorStrings[] = {
  "NoError",
  "FileNotFoundError",
  "CannotOpenFileError",
  "UnrecognizedFileTypeError",
  "PrematureEndOfFileError",
  "FileFormatError",
  "NoFileNameError",
  "OutOfDiskSpaceError",
  "UnknownError",
  "UserError",
  NULL
};

const char *vtkErrorCode::GetStringFromErrorCode(unsigned long error)
{
  static unsigned long numerrors = 0;
  if(error < FirstVTKErrorCode)
    {
    return strerror(static_cast<int>(error));
    }
  else
    {
    error -= FirstVTKErrorCode;
    }

  // find length of table
  if (!numerrors)
    {
    while (vtkErrorCodeErrorStrings[numerrors] != NULL)
      {
      numerrors++;
      }
    }
  if (error < numerrors)
    {
    return vtkErrorCodeErrorStrings[error];
    }
  else if (error == vtkErrorCode::UserError)
    {
    return "UserError";
    }
  else
    {
    return "NoError";
    }
}

unsigned long vtkErrorCode::GetErrorCodeFromString(const char *error)
{
  unsigned long i;

  for (i = 0; vtkErrorCodeErrorStrings[i] != NULL; i++)
    {
    if (!strcmp(vtkErrorCodeErrorStrings[i],error))
      {
      return i;
      }
    }
  if (!strcmp("UserError",error))
    {
    return vtkErrorCode::UserError;
    }
  return vtkErrorCode::NoError;
}


unsigned long vtkErrorCode::GetLastSystemError()
{
  return static_cast<unsigned long>(errno);
}



