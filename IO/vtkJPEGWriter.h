/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkJPEGWriter.h
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
// .NAME vtkJPEGWriter - Writes JPEG files.
// .SECTION Description
// vtkJPEGWriter writes JPEG files. It supports 1 and 3 component data of
// unsigned char. It relies on the IJG's libjpeg.  Thanks to IJG for
// supplying a public jpeg IO library.

// .SECTION See Also
// vtkJPEGReader

#ifndef __vtkJPEGWriter_h
#define __vtkJPEGWriter_h

#include "vtkImageWriter.h"

class VTK_IO_EXPORT vtkJPEGWriter : public vtkImageWriter
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
  vtkSetClampMacro(Quality, unsigned int, 0, 100);
  vtkGetMacro(Quality, unsigned int);

  // Description:
  // Progressive JPEG generation.
  vtkSetMacro(Progressive, unsigned int);
  vtkGetMacro(Progressive, unsigned int);
  vtkBooleanMacro(Progressive, unsigned int);

protected:
  vtkJPEGWriter();
  ~vtkJPEGWriter() {};
  
  void WriteSlice(vtkImageData *data);

private:
  unsigned int Quality;
  unsigned int Progressive;

private:
  vtkJPEGWriter(const vtkJPEGWriter&);  // Not implemented.
  void operator=(const vtkJPEGWriter&);  // Not implemented.
};

#endif


