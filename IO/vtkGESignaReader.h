/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGESignaReader.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
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

#ifndef __vtkGESignaReader_h
#define __vtkGESignaReader_h

#include <stdio.h>
#include "vtkMedicalImageReader2.h"

class VTK_IO_EXPORT vtkGESignaReader : public vtkMedicalImageReader2
{
public:
  static vtkGESignaReader *New();
  vtkTypeRevisionMacro(vtkGESignaReader,vtkImageReader2);

  // Description: is the given file name a GESigna file?
  virtual int CanReadFile(const char* fname);
protected:
  vtkGESignaReader() {};
  ~vtkGESignaReader() {};

  virtual void ExecuteInformation();
  virtual void ExecuteData(vtkDataObject *out);

private:
  vtkGESignaReader(const vtkGESignaReader&);  // Not implemented.
  void operator=(const vtkGESignaReader&);  // Not implemented.
};
#endif


