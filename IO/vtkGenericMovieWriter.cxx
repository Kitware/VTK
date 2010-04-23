/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericMovieWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGenericMovieWriter.h"

#include "vtkImageData.h"
#include "vtkErrorCode.h"


//---------------------------------------------------------------------------
vtkGenericMovieWriter::vtkGenericMovieWriter()
{
  this->FileName = NULL;
  this->Error = 0;
}

//---------------------------------------------------------------------------
vtkGenericMovieWriter::~vtkGenericMovieWriter()
{
  this->SetFileName(0);
}

//----------------------------------------------------------------------------
void vtkGenericMovieWriter::SetInput(vtkImageData *input)
{
  this->vtkProcessObject::SetNthInput(0, input);
}

//----------------------------------------------------------------------------
vtkImageData *vtkGenericMovieWriter::GetInput()
{
  if (this->NumberOfInputs < 1)
    {
    return NULL;
    }
  
  return (vtkImageData *)(this->Inputs[0]);
}


//----------------------------------------------------------------------------
void vtkGenericMovieWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "FileName: "
     << (this->FileName ? this->FileName : "(none)") << endl;
  os << indent << "Error: " << this->Error << endl;
}

//----------------------------------------------------------------------------
static const char *vtkMovieWriterErrorStrings[] = {
  "Unassigned Error", 
  "Initialize Error",
  "No Input Error",
  "Can Not Compress Error",
  "Can Not Format Error",
  "Changed Resolution Error",
  NULL
};

const char *vtkGenericMovieWriter::GetStringFromErrorCode(unsigned long error)
{
  static unsigned long numerrors = 0;
  if(error < UserError)
    {
    return vtkErrorCode::GetStringFromErrorCode(error);
    }
  else
    {
    error -= UserError;
    }
  
  if (!numerrors)
    {
    while (vtkMovieWriterErrorStrings[numerrors] != NULL)
      {
      numerrors++;
      }
    }

  if (error < numerrors)
    {
    return vtkMovieWriterErrorStrings[error];
    }
  else
    {
    return "Unknown Error";
    }
}


