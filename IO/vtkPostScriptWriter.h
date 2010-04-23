/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPostScriptWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPostScriptWriter - Writes an image as a PostScript file.
// .SECTION Description
// vtkPostScriptWriter writes an image as a PostScript file using some
// reasonable scalings and centered on the page which is assumed to be
// about 8.5 by 11 inches. This is based loosely off of the code from
// pnmtops.c. Right now there aren't any real options.


#ifndef __vtkPostScriptWriter_h
#define __vtkPostScriptWriter_h

#include "vtkImageWriter.h"

class VTK_IO_EXPORT vtkPostScriptWriter : public vtkImageWriter
{
public:
  static vtkPostScriptWriter *New();
  vtkTypeMacro(vtkPostScriptWriter,vtkImageWriter);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkPostScriptWriter() {};
  ~vtkPostScriptWriter() {};

  virtual void WriteFile(ofstream *file, vtkImageData *data, int extent[6]);
  virtual void WriteFileHeader(ofstream *, vtkImageData *);
  virtual void WriteFileTrailer(ofstream *, vtkImageData *);
private:
  vtkPostScriptWriter(const vtkPostScriptWriter&);  // Not implemented.
  void operator=(const vtkPostScriptWriter&);  // Not implemented.
};

#endif


