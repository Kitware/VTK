/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellCenters.h
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
// .NAME vtkCellCenters - generate points at center of cells
// .SECTION Description
// vtkCellCenters is a filter that takes as input any dataset and 
// generates on output points at the center of the cells in the dataset.
// These points can be used for placing glyphs (vtkGlyph3D) or labeling 
// (vtkLabeledDataMapper). (The center is the parametric center of the
// cell, not necessarily the geometric or bounding box center.) The cell
// attributes will be associated with the points on output.
// 
// .SECTION Caveats
// You can choose to generate just points or points and vertex cells.
// Vertex cells are drawn during rendering; points are not. Use the ivar
// VertexCells to generate cells.

// .SECTION See Also
// vtkGlyph3D vtkLabeledDataMapper

#ifndef __vtkCellCenters_h
#define __vtkCellCenters_h

#include "vtkDataSetToPolyDataFilter.h"

class VTK_GRAPHICS_EXPORT vtkCellCenters : public vtkDataSetToPolyDataFilter
{
public:
  vtkTypeMacro(vtkCellCenters,vtkDataSetToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct object with vertex cell generation turned off.
  static vtkCellCenters *New();

  // Description:
  // Enable/disable the generation of vertex cells.
  vtkSetMacro(VertexCells,int);
  vtkGetMacro(VertexCells,int);
  vtkBooleanMacro(VertexCells,int);

protected:
  vtkCellCenters();
  ~vtkCellCenters() {};
  vtkCellCenters(const vtkCellCenters&);
  void operator=(const vtkCellCenters&);

  void Execute();

  int VertexCells;
};

#endif
