/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPNGReader.h
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
// .NAME vtkPNGReader - read PNG files
// .SECTION Description
// vtkPNGReader is a source object that reads PNG files.
// It should be able to read most any PNG file
//
// .SECTION See Also
// vtkPNGWriter

#ifndef __vtkPNGReader_h
#define __vtkPNGReader_h

#include <stdio.h>
#include "vtkImageReader2.h"

class VTK_IO_EXPORT vtkPNGReader : public vtkImageReader2
{
public:
  static vtkPNGReader *New();
  vtkTypeRevisionMacro(vtkPNGReader,vtkImageReader2);

protected:
  vtkPNGReader() {};
  ~vtkPNGReader() {};
  
  // Description: is the given file name a png file?
  virtual int CanReadFile(const char* fname);

  virtual void ExecuteInformation();
  virtual void ExecuteData(vtkDataObject *out);
private:
  vtkPNGReader(const vtkPNGReader&);  // Not implemented.
  void operator=(const vtkPNGReader&);  // Not implemented.
};
#endif


