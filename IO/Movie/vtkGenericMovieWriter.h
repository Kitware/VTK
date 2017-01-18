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
/**
 * @class   vtkGenericMovieWriter
 * @brief   an abstract movie writer class.
 *
 * vtkGenericMovieWriter is the abstract base class for several movie
 * writers. The input type is a vtkImageData. The Start() method will
 * open and create the file, the Write() method will output a frame to
 * the file (i.e. the contents of the vtkImageData), End() will finalize
 * and close the file.
 * @sa
 * vtkAVIWriter vtkMPEG2Writer
*/

#ifndef vtkGenericMovieWriter_h
#define vtkGenericMovieWriter_h

#include "vtkIOMovieModule.h" // For export macro
#include "vtkImageAlgorithm.h"

class vtkImageData;

class VTKIOMOVIE_EXPORT vtkGenericMovieWriter : public vtkImageAlgorithm
{
public:
  vtkTypeMacro(vtkGenericMovieWriter,vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Specify file name of avi file.
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * These methods start writing an Movie file, write a frame to the file
   * and then end the writing process.
   */
  virtual void Start() =0;
  virtual void Write() =0;
  virtual void End() =0;
  //@}

  //@{
  /**
   * Was there an error on the last write performed?
   */
  vtkGetMacro(Error,int);
  //@}

  /**
   * Converts vtkErrorCodes and vtkGenericMovieWriter errors to strings.
   */
  static const char *GetStringFromErrorCode(unsigned long event);

  enum MovieWriterErrorIds {
    UserError = 40000, //must match vtkErrorCode::UserError
    InitError,
    NoInputError,
    CanNotCompress,
    CanNotFormat,
    ChangedResolutionError
  };

protected:
  vtkGenericMovieWriter();
  ~vtkGenericMovieWriter() VTK_OVERRIDE;

  char *FileName;
  int Error;

private:
  vtkGenericMovieWriter(const vtkGenericMovieWriter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkGenericMovieWriter&) VTK_DELETE_FUNCTION;
};

#endif



