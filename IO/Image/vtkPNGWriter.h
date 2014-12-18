/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPNGWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPNGWriter - Writes PNG files.
// .SECTION Description
// vtkPNGWriter writes PNG files. It supports 1 to 4 component data of
// unsigned char or unsigned short

// .SECTION See Also
// vtkPNGReader

#ifndef vtkPNGWriter_h
#define vtkPNGWriter_h

#include "vtkIOImageModule.h" // For export macro
#include "vtkImageWriter.h"

class vtkImageData;
class vtkUnsignedCharArray;

class VTKIOIMAGE_EXPORT vtkPNGWriter : public vtkImageWriter
{
public:
  static vtkPNGWriter *New();
  vtkTypeMacro(vtkPNGWriter,vtkImageWriter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The main interface which triggers the writer to start.
  virtual void Write();

  // Description:
  // Set/Get the zlib compression level.
  // The range is 0-9, with 0 meaning no compression
  // corresponding to the largest file size, and 9 meaning
  // best compression, corresponding to the smallest file size.
  // The default is 5.
  vtkSetClampMacro(CompressionLevel, int, 0, 9);
  vtkGetMacro(CompressionLevel, int);

  // Description:
  // Write the image to memory (a vtkUnsignedCharArray)
  vtkSetMacro(WriteToMemory, unsigned int);
  vtkGetMacro(WriteToMemory, unsigned int);
  vtkBooleanMacro(WriteToMemory, unsigned int);

  // Description:
  // When writing to memory this is the result, it will be NULL until the
  // data is written the first time
  virtual void SetResult(vtkUnsignedCharArray*);
  vtkGetObjectMacro(Result, vtkUnsignedCharArray);

protected:
  vtkPNGWriter();
  ~vtkPNGWriter();

  void WriteSlice(vtkImageData *data, int* uExtent);
  int CompressionLevel;
  unsigned int WriteToMemory;
  vtkUnsignedCharArray *Result;
  FILE *TempFP;

private:
  vtkPNGWriter(const vtkPNGWriter&);  // Not implemented.
  void operator=(const vtkPNGWriter&);  // Not implemented.
};

#endif


