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

// .NAME vtkNrrdReader - Read nrrd files file system
//
// .SECTION Description
//
// .SECTION Bugs
//
// There are several limitations on what type of nrrd files we can read.  This
// reader only supports nrrd files in raw or ascii format.  Other encodings
// like hex will result in errors.  When reading in detached headers, this only
// supports reading one file that is detached.
//

#ifndef __vtkNrrdReader_h
#define __vtkNrrdReader_h

#include "vtkIOImageModule.h" // For export macro
#include "vtkImageReader.h"

class vtkCharArray;

class VTKIOIMAGE_EXPORT vtkNrrdReader : public vtkImageReader
{
public:
  vtkTypeMacro(vtkNrrdReader, vtkImageReader);
  static vtkNrrdReader *New();
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  virtual int CanReadFile(const char *filename);

protected:
  vtkNrrdReader();
  ~vtkNrrdReader();

  virtual int RequestInformation(vtkInformation *request,
                                 vtkInformationVector **inputVector,
                                 vtkInformationVector *outputVector);

  virtual int RequestData(vtkInformation *request,
                          vtkInformationVector **inputVector,
                          vtkInformationVector *outputVector);

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
  vtkNrrdReader(const vtkNrrdReader &);       // Not implemented.
  void operator=(const vtkNrrdReader &);        // Not implemented.
};

#endif //__vtkNrrdReader_h
