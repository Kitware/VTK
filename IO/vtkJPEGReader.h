/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkJPEGReader.h
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
// .NAME vtkJPEGReader - read JPEG files
// .SECTION Description
// vtkJPEGReader is a source object that reads JPEG files.
// It should be able to read most any JPEG file
//
// .SECTION See Also
// vtkJPEGWriter

#ifndef __vtkJPEGReader_h
#define __vtkJPEGReader_h

#include <stdio.h>
#include "vtkImageReader2.h"

class VTK_IO_EXPORT vtkJPEGReader : public vtkImageReader2
{
public:
  static vtkJPEGReader *New();
  vtkTypeRevisionMacro(vtkJPEGReader,vtkImageReader2);

  // Description:
  // Is the given file a JPEG file?
  int CanReadFile(const char* fname);
  
protected:
  vtkJPEGReader() {};
  ~vtkJPEGReader() {};

  virtual void ExecuteInformation();
  virtual void ExecuteData(vtkDataObject *out);
private:
  vtkJPEGReader(const vtkJPEGReader&);  // Not implemented.
  void operator=(const vtkJPEGReader&);  // Not implemented.
};
#endif


