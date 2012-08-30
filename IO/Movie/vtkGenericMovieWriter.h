/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericMovieWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkGenericMovieWriter - an abstract movie writer class.
// .SECTION Description
// vtkGenericMovieWriter is the abstract base class for several movie
// writers. The input type is a vtkImageData. The Start() method will
// open and create the file, the Write() method will output a frame to
// the file (i.e. the contents of the vtkImageData), End() will finalize
// and close the file.
// .SECTION See Also
// vtkAVIWriter vtkMPEG2Writer

#ifndef __vtkGenericMovieWriter_h
#define __vtkGenericMovieWriter_h

#include "vtkIOMovieModule.h" // For export macro
#include "vtkImageAlgorithm.h"

class vtkImageData;

class VTKIOMOVIE_EXPORT vtkGenericMovieWriter : public vtkImageAlgorithm
{
public:
  vtkTypeMacro(vtkGenericMovieWriter,vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify file name of avi file.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Description:
  // These methods start writing an Movie file, write a frame to the file
  // and then end the writing process.
  virtual void Start() =0;
  virtual void Write() =0;
  virtual void End() =0;

  // Description:
  // Was there an error on the last write performed?
  vtkGetMacro(Error,int);

  // Description:
  // Converts vtkErrorCodes and vtkGenericMovieWriter errors to strings.
  static const char *GetStringFromErrorCode(unsigned long event);

  //BTX
  enum MovieWriterErrorIds {
    UserError = 40000, //must match vtkErrorCode::UserError
    InitError,
    NoInputError,
    CanNotCompress,
    CanNotFormat,
    ChangedResolutionError
  };
  //ETX

protected:
  vtkGenericMovieWriter();
  ~vtkGenericMovieWriter();

  char *FileName;
  int Error;

private:
  vtkGenericMovieWriter(const vtkGenericMovieWriter&); // Not implemented
  void operator=(const vtkGenericMovieWriter&); // Not implemented
};

#endif



