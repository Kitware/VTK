/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractPolyDataPiece.h
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
// .NAME vtkExtractPolyDataPiece - Return specified piece, including specified
// number of ghost levels.

#ifndef __vtkExtractPolyDataPiece_h
#define __vtkExtractPolyDataPiece_h

#include "vtkPolyDataToPolyDataFilter.h"

class VTK_EXPORT vtkExtractPolyDataPiece : public vtkPolyDataToPolyDataFilter
{
public:
  static vtkExtractPolyDataPiece *New();
  vtkTypeMacro(vtkExtractPolyDataPiece, vtkPolyDataToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Turn on/off creating ghost cells (on by default).
  vtkSetMacro(CreateGhostCells, int);
  vtkGetMacro(CreateGhostCells, int);
  vtkBooleanMacro(CreateGhostCells, int);
  
protected:
  vtkExtractPolyDataPiece();
  ~vtkExtractPolyDataPiece() {};
  vtkExtractPolyDataPiece(const vtkExtractPolyDataPiece&);
  void operator=(const vtkExtractPolyDataPiece&);

  // Usual data generation method
  void Execute();
  void ExecuteInformation();
  void ComputeInputUpdateExtents(vtkDataObject *out);
 
  // A method for labeling which piece the cells belong to.
  void ComputeCellTags(vtkIntArray *cellTags, vtkIdList *pointOwnership,
		       int piece, int numPieces);
  
  void AddGhostLevel(vtkPolyData *input, vtkIntArray *cellTags, int ghostLevel);
  
  int CreateGhostCells;
};

#endif
