/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredPointsWriter.h
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
// .NAME vtkStructuredPointsWriter - write vtk structured points data file
// .SECTION Description
// vtkStructuredPointsWriter is a source object that writes ASCII or binary 
// structured points data in vtk file format. See text for format details.
// .SECTION Caveats
// Binary files written on one system may not be writable on other systems.

#ifndef __vtkStructuredPointsWriter_h
#define __vtkStructuredPointsWriter_h

#include "vtkDataWriter.h"
#include "vtkImageData.h"

class VTK_EXPORT vtkStructuredPointsWriter : public vtkDataWriter
{
public:
  static vtkStructuredPointsWriter *New();
  vtkTypeMacro(vtkStructuredPointsWriter,vtkDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set / get  the input data or filter.
  void SetInput(vtkImageData *input);
  vtkImageData *GetInput();
                               
protected:
  vtkStructuredPointsWriter() {};
  ~vtkStructuredPointsWriter() {};
  vtkStructuredPointsWriter(const vtkStructuredPointsWriter&);
  void operator=(const vtkStructuredPointsWriter&);

  void WriteData();

};

#endif


