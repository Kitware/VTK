/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataReader.h
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
// .NAME vtkPolyDataReader - read vtk polygonal data file
// .SECTION Description
// vtkPolyDataReader is a source object that reads ASCII or binary 
// polygonal data files in vtk format (see text for format details).
// The output of this reader is a single vtkPolyData data object.
// The superclass of this class, vtkDataReader, provides many methods for
// controlling the reading of the data file, see vtkDataReader for more
// information.
// .SECTION Caveats
// Binary files written on one system may not be readable on other systems.
// .SECTION See Also
// vtkPolyData vtkDataReader

#ifndef __vtkPolyDataReader_h
#define __vtkPolyDataReader_h

#include "vtkDataReader.h"
#include "vtkPolyData.h"

class VTK_EXPORT vtkPolyDataReader : public vtkDataReader
{
public:
  static vtkPolyDataReader *New();
  vtkTypeMacro(vtkPolyDataReader,vtkDataReader);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the output of this reader.
  vtkPolyData *GetOutput();
  vtkPolyData *GetOutput(int idx)
    {return (vtkPolyData *) this->vtkSource::GetOutput(idx); };
  void SetOutput(vtkPolyData *output);

protected:
  vtkPolyDataReader();
  ~vtkPolyDataReader();
  vtkPolyDataReader(const vtkPolyDataReader&);
  void operator=(const vtkPolyDataReader&);

  void Execute();

  // Update extent of PolyData is specified in pieces.  
  // Since all DataObjects should be able to set UpdateExent as pieces,
  // just copy output->UpdateExtent  all Inputs.
  void ComputeInputUpdateExtents(vtkDataObject *output);
  
  // Used by streaming: The extent of the output being processed
  // by the execute method. Set in the ComputeInputUpdateExtents method.
  int ExecutePiece;
  int ExecuteNumberOfPieces;
  int ExecuteGhostLevel;
};

#endif


