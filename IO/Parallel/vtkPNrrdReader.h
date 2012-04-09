// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPNrrdReader.h

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

// .NAME vtkPNrrdReader - Read nrrd files efficiently from parallel file systems (and reasonably well elsewhere).
//
// .SECTION Description
//
// vtkPNrrdReader is a subclass of vtkMPIImageReader that will read Nrrd format
// header information of the image before reading the data.  This means that the
// reader will automatically set information like file dimensions.
//
// .SECTION Bugs
//
// There are several limitations on what type of nrrd files we can read.  This
// reader only supports nrrd files in raw format.  Other encodings like ascii
// and hex will result in errors.  When reading in detached headers, this only
// supports reading one file that is detached.
//

#ifndef __vtkPNrrdReader_h
#define __vtkPNrrdReader_h

#include "vtkMPIImageReader.h"

class vtkCharArray;

class VTK_PARALLEL_EXPORT vtkPNrrdReader : public vtkMPIImageReader
{
public:
  vtkTypeMacro(vtkPNrrdReader, vtkMPIImageReader);
  static vtkPNrrdReader *New();
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  virtual int CanReadFile(const char *filename);

protected:
  vtkPNrrdReader();
  ~vtkPNrrdReader();

  virtual int RequestInformation(vtkInformation *request,
                                 vtkInformationVector **inputVector,
                                 vtkInformationVector *outputVector);

  virtual int RequestData(vtkInformation *request,
                          vtkInformationVector **inputVector,
                          vtkInformationVector *outputVector);

  virtual int ReadHeader();
  virtual int ReadHeader(vtkCharArray *headerBuffer);

  vtkStringArray *DataFiles;

private:
  vtkPNrrdReader(const vtkPNrrdReader &);       // Not implemented.
  void operator=(const vtkPNrrdReader &);        // Not implemented.
};

#endif //__vtkPNrrdReader_h
