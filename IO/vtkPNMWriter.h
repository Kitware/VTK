/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPNMWriter.h
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
// .NAME vtkPNMWriter - Writes PNM (portable any map)  files.
// .SECTION Description
// vtkPNMWriter writes PNM file. The data type
// of the file is unsigned char regardless of the input type.


#ifndef __vtkPNMWriter_h
#define __vtkPNMWriter_h

#include "vtkImageWriter.h"

class VTK_IO_EXPORT vtkPNMWriter : public vtkImageWriter
{
public:
  static vtkPNMWriter *New();
  vtkTypeRevisionMacro(vtkPNMWriter,vtkImageWriter);

protected:
  vtkPNMWriter() {};
  ~vtkPNMWriter() {};

  virtual void WriteFile(ofstream *file, vtkImageData *data, int extent[6]);
  virtual void WriteFileHeader(ofstream *, vtkImageData *);
private:
  vtkPNMWriter(const vtkPNMWriter&);  // Not implemented.
  void operator=(const vtkPNMWriter&);  // Not implemented.
};

#endif


