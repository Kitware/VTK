/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBranchExtentTranslator.h
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
// .NAME vtkBranchExtentTranslator - Uses alternative source for whole extent.
// .SECTION Description
// vtkBranchExtentTranslator is like extent translator, but it uses an 
// alternative source as a whole extent. The whole extent passed is assumed 
// to be a subextent of the original source.  we simply take the intersection 
// of the split extent and the whole extdent passed in.  We are attempting to
// make branching pipelines request consistent extents with the same piece 
// requests.  

// .SECTION Caveats
// This object is still under development.

#ifndef __vtkBranchExtentTranslator_h
#define __vtkBranchExtentTranslator_h

#include "vtkExtentTranslator.h"
#include "vtkImageData.h"


class VTK_EXPORT vtkBranchExtentTranslator : public vtkExtentTranslator
{
public:
  static vtkBranchExtentTranslator *New();

  vtkTypeMacro(vtkBranchExtentTranslator,vtkExtentTranslator);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This is the original upstream image source.
  vtkSetObjectMacro(OriginalSource,vtkImageData);
  vtkGetObjectMacro(OriginalSource,vtkImageData);

  // Description:
  // Generates the extent from the pieces.
  int PieceToExtent();

  // Description:
  // This unstructured extent/piece is store here for the users convenience.
  // It is not used internally.  The intent was to let an "assignment" be made
  // when the translator/first source is created.  The translator/assignment
  // can be used for any new filter that uses the original source as output.
  // Branches will then have the same assignment.
  vtkSetMacro(AssignedPiece, int);
  vtkGetMacro(AssignedPiece, int);
  vtkSetMacro(AssignedNumberOfPieces, int);
  vtkGetMacro(AssignedNumberOfPieces, int);

protected:
  vtkBranchExtentTranslator();
  ~vtkBranchExtentTranslator();
  vtkBranchExtentTranslator(const vtkBranchExtentTranslator&);
  void operator=(const vtkBranchExtentTranslator&);

  vtkImageData *OriginalSource;
  int AssignedPiece;
  int AssignedNumberOfPieces;
};

#endif

