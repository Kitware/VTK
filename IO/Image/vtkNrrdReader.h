// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkNrrdReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

/**
 * @class   vtkNrrdReader
 * @brief   Read nrrd files file system
 *
 *
 *
 *
 * @bug
 * There are several limitations on what type of nrrd files we can read.  This
 * reader only supports nrrd files in raw or ascii format.  Other encodings
 * like hex will result in errors.  When reading in detached headers, this only
 * supports reading one file that is detached.
 *
*/

#ifndef vtkNrrdReader_h
#define vtkNrrdReader_h

#include "vtkIOImageModule.h" // For export macro
#include "vtkImageReader.h"

class vtkCharArray;

class VTKIOIMAGE_EXPORT vtkNrrdReader : public vtkImageReader
{
public:
  vtkTypeMacro(vtkNrrdReader, vtkImageReader);
  static vtkNrrdReader *New();
  void PrintSelf(ostream &os, vtkIndent indent) VTK_OVERRIDE;

  int CanReadFile(const char *filename) VTK_OVERRIDE;

protected:
  vtkNrrdReader();
  ~vtkNrrdReader() VTK_OVERRIDE;

  int RequestInformation(vtkInformation *request,
                                 vtkInformationVector **inputVector,
                                 vtkInformationVector *outputVector) VTK_OVERRIDE;

  int RequestData(vtkInformation *request,
                          vtkInformationVector **inputVector,
                          vtkInformationVector *outputVector) VTK_OVERRIDE;

  int ReadHeaderInternal(vtkCharArray *headerBuffer);
  virtual int ReadHeader();
  virtual int ReadHeader(vtkCharArray *headerBuffer);

  virtual int ReadDataAscii(vtkImageData *output);

  vtkStringArray *DataFiles;

  enum {
    ENCODING_RAW,
    ENCODING_ASCII
  };

  int Encoding;

private:
  vtkNrrdReader(const vtkNrrdReader &) VTK_DELETE_FUNCTION;
  void operator=(const vtkNrrdReader &) VTK_DELETE_FUNCTION;
};

#endif //vtkNrrdReader_h
