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
/**
 * @class   vtkPostScriptWriter
 * @brief   Writes an image as a PostScript file.
 *
 * vtkPostScriptWriter writes an image as a PostScript file using some
 * reasonable scalings and centered on the page which is assumed to be
 * about 8.5 by 11 inches. This is based loosely off of the code from
 * pnmtops.c. Right now there aren't any real options.
*/

#ifndef vtkPostScriptWriter_h
#define vtkPostScriptWriter_h

#include "vtkIOImageModule.h" // For export macro
#include "vtkImageWriter.h"

class VTKIOIMAGE_EXPORT vtkPostScriptWriter : public vtkImageWriter
{
public:
  static vtkPostScriptWriter *New();
  vtkTypeMacro(vtkPostScriptWriter,vtkImageWriter);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

protected:
  vtkPostScriptWriter() {}
  ~vtkPostScriptWriter() {}

  virtual void WriteFile(
    ofstream *file, vtkImageData *data, int extent[6], int wExt[6]);
  virtual void WriteFileHeader(ofstream *, vtkImageData *, int wExt[6]);
  virtual void WriteFileTrailer(ofstream *, vtkImageData *);
private:
  vtkPostScriptWriter(const vtkPostScriptWriter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPostScriptWriter&) VTK_DELETE_FUNCTION;
};

#endif


