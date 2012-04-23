/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBMPWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkBMPWriter - Writes Windows BMP files.
// .SECTION Description
// vtkBMPWriter writes BMP files. The data type
// of the file is unsigned char regardless of the input type.

// .SECTION See Also
// vtkBMPReader

#ifndef __vtkBMPWriter_h
#define __vtkBMPWriter_h

#include "vtkIOImageModule.h" // For export macro
#include "vtkImageWriter.h"

class VTKIOIMAGE_EXPORT vtkBMPWriter : public vtkImageWriter
{
public:
  static vtkBMPWriter *New();
  vtkTypeMacro(vtkBMPWriter,vtkImageWriter);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkBMPWriter();
  ~vtkBMPWriter() {};

  virtual void WriteFile(ofstream *file, vtkImageData *data, int ext[6], int wExt[6]);
  virtual void WriteFileHeader(ofstream *, vtkImageData *, int wExt[6]);
private:
  vtkBMPWriter(const vtkBMPWriter&);  // Not implemented.
  void operator=(const vtkBMPWriter&);  // Not implemented.
};

#endif


