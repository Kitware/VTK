/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTIFFWriter.h
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
// .NAME vtkTIFFWriter - write out structured points as a TIFF file
// .SECTION Description
// vtkTIFFWriter writes structured points as a non-compressed TIFF data file.

#ifndef __vtkTIFFWriter_h
#define __vtkTIFFWriter_h

#include <stdio.h>
#include "vtkImageWriter.h"

class VTK_IO_EXPORT vtkTIFFWriter : public vtkImageWriter
{
public:
  static vtkTIFFWriter *New();
  vtkTypeRevisionMacro(vtkTIFFWriter,vtkImageWriter);

protected:
  vtkTIFFWriter() {};
  ~vtkTIFFWriter() {};

  virtual void WriteFile(ofstream *file, vtkImageData *data, 
                         int ext[6]);
  virtual void WriteFileHeader(ofstream *, vtkImageData *);
private:
  vtkTIFFWriter(const vtkTIFFWriter&);  // Not implemented.
  void operator=(const vtkTIFFWriter&);  // Not implemented.
};

#endif

