/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPostScriptWriter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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


