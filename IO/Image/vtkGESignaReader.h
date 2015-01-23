/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGESignaReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkGESignaReader - read GE Signa ximg files
// .SECTION Description
// vtkGESignaReader is a source object that reads some GE Signa ximg files It
// does support reading in pixel spacing, slice spacing and it computes an
// origin for the image in millimeters. It always produces greyscale unsigned
// short data and it supports reading in rectangular, packed, compressed, and
// packed&compressed. It does not read in slice orientation, or position
// right now. To use it you just need to specify a filename or a file prefix
// and pattern.

//
// .SECTION See Also
// vtkImageReader2

#ifndef vtkGESignaReader_h
#define vtkGESignaReader_h

#include "vtkIOImageModule.h" // For export macro
#include "vtkMedicalImageReader2.h"

class VTKIOIMAGE_EXPORT vtkGESignaReader : public vtkMedicalImageReader2
{
public:
  static vtkGESignaReader *New();
  vtkTypeMacro(vtkGESignaReader,vtkMedicalImageReader2);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Is the given file a GESigna file?
  virtual int CanReadFile(const char* fname);

  // Description:
  // Valid extentsions
  virtual const char* GetFileExtensions()
    {
      return ".MR .CT";
    }

  // Description:
  // A descriptive name for this format
  virtual const char* GetDescriptiveName()
    {
      return "GESigna";
    }

protected:
  vtkGESignaReader() {}
  ~vtkGESignaReader() {}

  virtual void ExecuteInformation();
  virtual void ExecuteDataWithInformation(vtkDataObject *out, vtkInformation* outInfo);

private:
  vtkGESignaReader(const vtkGESignaReader&);  // Not implemented.
  void operator=(const vtkGESignaReader&);  // Not implemented.
};
#endif


