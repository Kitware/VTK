/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSpherePuzzle.h
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
// .NAME vtkSpherePuzzle - create a polygonal sphere centered at the origin
// .SECTION Description
// vtkSpherePuzzle creates 

#ifndef __vtkSpherePuzzle_h
#define __vtkSpherePuzzle_h

#include "vtkPolyDataSource.h"
#include "vtkTransform.h"

#define VTK_MAX_SPHERE_RESOLUTION 1024

class VTK_EXPORT vtkSpherePuzzle : public vtkPolyDataSource 
{
public:
  vtkTypeMacro(vtkSpherePuzzle,vtkPolyDataSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  static vtkSpherePuzzle *New();

  // Description:
  // Reset the state of this puzzle back to its original state.
  void Reset();

  // Description:
  // Move the top/bottom half one segment either direction.
  void MoveHorizontal(int section, int percentage, int rightFlag);

  // Description:
  // Rotate vertical half of sphere along one of the longitude lines.
  void MoveVertical(int section, int percentage, int rightFlag);
  
  // Description:
  // SetPoint will be called as the mouse moves over the screen.
  // The output will change to indicate the pending move.
  // SetPoint returns zero if move is not activated by point.
  // Otherwise it encodes the move into a unique integer so that
  // the caller can determine if the move state has changed.
  // This will answer the question, "Should I render."
  int SetPoint(float x, float y, float z);  

  // Description:
  // Move actually implements the pending move. When percentage
  // is 100, the pending move becomes inactive, and SetPoint
  // will have to be called again to setup another move.
  void MovePoint(int percentage);  

  // Description:
  // For drawing state as arrows.
  int *GetState() {return this->State;}

protected:
  vtkSpherePuzzle();
  ~vtkSpherePuzzle();
  vtkSpherePuzzle(const vtkSpherePuzzle&);
  void operator=(const vtkSpherePuzzle&);

  void Execute();
  void MarkVertical(int section);
  void MarkHorizontal(int section);  
  
  int State[32];
  
  // Stuff for storing a partial move.
  int PieceMask[32];
  vtkTransform *Transform;

  // Colors for faces.
  unsigned char Colors[96];

  // State for potential move.
  int Active;
  int VerticalFlag;
  int RightFlag;
  int Section;
};

#endif


