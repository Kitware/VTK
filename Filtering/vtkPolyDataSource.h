/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataSource.h
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
// .NAME vtkPolyDataSource - abstract class whose subclasses generate polygonal data
// .SECTION Description
// vtkPolyDataSource is an abstract class whose subclasses generate polygonal
// data.

// .SECTION See Also
// vtkPolyDataReader vtkAxes vtkBYUReader vtkConeSource vtkCubeSource
// vtkCursor3D vtkCyberReader vtkCylinderSource vtkDiskSource vtkLineSource
// vtkMCubesReader vtkOutlineSource vtkPlaneSource vtkPointSource vtkSTLReader
// vtkSphereSource vtkTextSource vtkUGFacetReader vtkVectorText

#ifndef __vtkPolyDataSource_h
#define __vtkPolyDataSource_h

#include "vtkSource.h"
#include "vtkPolyData.h"

class VTK_FILTERING_EXPORT vtkPolyDataSource : public vtkSource
{
public:
  static vtkPolyDataSource *New();
  vtkTypeMacro(vtkPolyDataSource,vtkSource);

  // Description:
  // Get the output of this source.
  vtkPolyData *GetOutput();
  vtkPolyData *GetOutput(int idx)
    {return (vtkPolyData *) this->vtkSource::GetOutput(idx); };
  void SetOutput(vtkPolyData *output);

protected:
  vtkPolyDataSource();
  ~vtkPolyDataSource() {};
  
  // Update extent of PolyData is specified in pieces.  
  // Since all DataObjects should be able to set UpdateExent as pieces,
  // just copy output->UpdateExtent  all Inputs.
  void ComputeInputUpdateExtents(vtkDataObject *output);
  
  // Used by streaming: The extent of the output being processed
  // by the execute method. Set in the ComputeInputUpdateExtents method.
  int ExecutePiece;
  int ExecuteNumberOfPieces;
  
  int ExecuteGhostLevel;
private:
  vtkPolyDataSource(const vtkPolyDataSource&);  // Not implemented.
  void operator=(const vtkPolyDataSource&);  // Not implemented.
};

#endif





