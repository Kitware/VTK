/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkJPEGWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkJPEGWriter - Writes JPEG files.
// .SECTION Description
// vtkJPEGWriter writes JPEG files. It supports 1 and 3 component data of
// unsigned char. It relies on the IJG's libjpeg.  Thanks to IJG for
// supplying a public jpeg IO library.

// .SECTION See Also
// vtkJPEGReader

#ifndef __vtkJPEGWriter_h
#define __vtkJPEGWriter_h

#include "vtkIOImageModule.h" // For export macro
#include "vtkImageWriter.h"

class vtkUnsignedCharArray;
class vtkImageData;

class VTKIOIMAGE_EXPORT vtkJPEGWriter : public vtkImageWriter
{
public:
  static vtkJPEGWriter *New();
  vtkTypeMacro(vtkJPEGWriter,vtkImageWriter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The main interface which triggers the writer to start.
  virtual void Write();

  // Description:
  // Compression quality. 0 = Low quality, 100 = High quality
  vtkSetClampMacro(Quality, int, 0, 100);
  vtkGetMacro(Quality, int);

  // Description:
  // Progressive JPEG generation.
  vtkSetMacro(Progressive, unsigned int);
  vtkGetMacro(Progressive, unsigned int);
  vtkBooleanMacro(Progressive, unsigned int);

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
  vtkJPEGWriter();
  ~vtkJPEGWriter();

  void WriteSlice(vtkImageData *data, int* uExtent);

private:
  int Quality;
  unsigned int Progressive;
  unsigned int WriteToMemory;
  vtkUnsignedCharArray *Result;
  FILE *TempFP;

private:
  vtkJPEGWriter(const vtkJPEGWriter&);  // Not implemented.
  void operator=(const vtkJPEGWriter&);  // Not implemented.
};

#endif


