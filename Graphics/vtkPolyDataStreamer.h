/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataStreamer.h
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
// .NAME vtkPolyDataStreamer - Stream appends input pieces to the output.
// .SECTION Description
// vtkPolyDataStreamer initiates streaming by requesting pieces from its 
// single input it appends these pieces it to the requested output.
// Note that since vtkPolyDataStreamer uses an append filter, all the
// polygons generated have to be kept in memory before rendering. If
// these do not fit in the memory, it is possible to make the vtkPolyDataMapper
// stream. Since the mapper will render each piece separately, all the
// polygons do not have to stored in memory.
// .SECTION Note
// The output may be slightly different if the pipeline does not handle 
// ghost cells properly (i.e. you might see seames between the pieces).
// .SECTION See Also
// vtkAppendFilter

#ifndef __vtkPolyDataStreamer_h
#define __vtkPolyDataStreamer_h

#include "vtkPolyDataToPolyDataFilter.h"

class VTK_GRAPHICS_EXPORT vtkPolyDataStreamer : public vtkPolyDataToPolyDataFilter
{
public:
  static vtkPolyDataStreamer *New();

  vtkTypeMacro(vtkPolyDataStreamer,vtkPolyDataToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the number of pieces to divide the problem into.
  void SetNumberOfStreamDivisions(int num);
  vtkGetMacro(NumberOfStreamDivisions,int);
  
  // Description:
  // By default, this option is off.  When it is on, cell scalars are generated
  // based on which piece they are in.
  vtkSetMacro(ColorByPiece, int);
  vtkGetMacro(ColorByPiece, int);
  vtkBooleanMacro(ColorByPiece, int);


protected:
  vtkPolyDataStreamer();
  ~vtkPolyDataStreamer();
  vtkPolyDataStreamer(const vtkPolyDataStreamer&);
  void operator=(const vtkPolyDataStreamer&);
  
  // Append the pieces.
  void Execute();
  void ComputeInputUpdateExtents(vtkDataObject *output);
  
  int NumberOfStreamDivisions;
  int ColorByPiece;
};

#endif





