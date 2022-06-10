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
 * reader only supports nrrd files in raw, ascii and gzip format.  Other encodings
 * like hex will result in errors.  When reading in detached headers, this only
 * supports reading one file that is detached.
 *
 */

#ifndef vtkNrrdReader_h
#define vtkNrrdReader_h

#include "vtkIOImageModule.h" // For export macro
#include "vtkImageReader.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkCharArray;

class VTKIOIMAGE_EXPORT vtkNrrdReader : public vtkImageReader
{
public:
  vtkTypeMacro(vtkNrrdReader, vtkImageReader);
  static vtkNrrdReader* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  int CanReadFile(VTK_FILEPATH const char* filename) override;

protected:
  vtkNrrdReader();
  ~vtkNrrdReader() override;

  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  int ReadHeaderInternal(vtkCharArray* headerBuffer);
  virtual int ReadHeader();
  virtual int ReadHeader(vtkCharArray* headerBuffer);

  virtual int ReadDataAscii(vtkImageData* output);

  template <typename T>
  int vtkNrrdReaderReadDataGZipTemplate(vtkImageData* output, T* outBuffer);
  virtual int ReadDataGZip(vtkImageData* output);

  vtkStringArray* DataFiles;

  enum
  {
    ENCODING_RAW,
    ENCODING_ASCII,
    ENCODING_GZIP
  };

  int Encoding;

private:
  vtkNrrdReader(const vtkNrrdReader&) = delete;
  void operator=(const vtkNrrdReader&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkNrrdReader_h
