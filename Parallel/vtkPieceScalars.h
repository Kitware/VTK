/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPieceScalars.h
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
// .NAME vtkPieceScalars - Sets all cell scalars from the update piece.
// .SECTION Description
// vtkPieceScalars is meant to display which piece is being requested
// as scalar values.  It is usefull for visualizing the partioning for
// streaming or distributed pipelines.
// .SECTION See Also
// vtkPolyDataStreamer

#ifndef __vtkPieceScalars_h
#define __vtkPieceScalars_h

#include "vtkDataSetToDataSetFilter.h"

class VTK_PARALLEL_EXPORT vtkPieceScalars : public vtkDataSetToDataSetFilter
{
public:
  static vtkPieceScalars *New();

  vtkTypeMacro(vtkPieceScalars,vtkDataSetToDataSetFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Option to centerate cell scalars of poi9nts scalars.  Default is point scalars.
  void SetScalarModeToCellData() {this->SetCellScalarsFlag(1);}
  void SetScalarModeToPointData() {this->SetCellScalarsFlag(0);}
  int GetScalarMode() {return this->CellScalarsFlag;}
  
  // Dscription:
  // This option uses a random mapping between pieces and scalar values.
  // The scalar values are choosen between 0 and 1.  By default, random mode is off.
  vtkSetMacro(RandomMode, int);
  vtkGetMacro(RandomMode, int);
  vtkBooleanMacro(RandomMode, int);
  
protected:
  vtkPieceScalars();
  ~vtkPieceScalars();
  vtkPieceScalars(const vtkPieceScalars&);
  void operator=(const vtkPieceScalars&);
  
  // Append the pieces.
  void Execute();
  
  vtkIntArray *MakePieceScalars(int piece, vtkIdType numScalars);
  vtkFloatArray *MakeRandomScalars(int piece, vtkIdType numScalars);
  
  vtkSetMacro(CellScalarsFlag,int);
  int CellScalarsFlag;
  int RandomMode;
};

#endif
