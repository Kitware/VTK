/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtentTranslator.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$



Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

    THIS CLASS IS PATENT PENDING.

    Application of this software for commercial purposes requires 
    a license grant from Kitware. Contact:
        Ken Martin
        Kitware
        469 Clifton Corporate Parkway,
        Clifton Park, NY 12065
        Phone:1-518-371-3971 
    for more information.

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
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkExtentTranslator - Generates a structured extent from unstructured.

// .SECTION Description
// vtkExtentTranslator generates a structured extent from an unstructured extent.
// It uses a recursive cheme that splits the largest axis.  A hard coded extent can
// be used fro a starting point.

// .SECTION Caveats
// This object is still under development.

#ifndef __vtkExtentTranslator_h
#define __vtkExtentTranslator_h

#include "vtkObject.h"


class VTK_EXPORT vtkExtentTranslator : public vtkObject
{
public:
  static vtkExtentTranslator *New();

  vtkTypeMacro(vtkExtentTranslator,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the Piece/NumPieces. Set the WholeExtent and then call PieceToExtent.
  // The result can be obtained from the Extent ivar.
  vtkSetVector6Macro(WholeExtent, int);
  vtkGetVector6Macro(WholeExtent, int);
  vtkSetVector6Macro(Extent, int);
  vtkGetVector6Macro(Extent, int);
  virtual int PieceToExtent();
  vtkSetMacro(Piece,int);
  vtkGetMacro(Piece,int);
  vtkSetMacro(NumberOfPieces,int);
  vtkGetMacro(NumberOfPieces,int);
  vtkSetMacro(GhostLevel, int);
  vtkGetMacro(GhostLevel, int);

protected:
  vtkExtentTranslator();
  ~vtkExtentTranslator();
  vtkExtentTranslator(const vtkExtentTranslator&) {};
  void operator=(const vtkExtentTranslator&) {};

  // Description:
  // Returns 0 if no data exist for a piece.
  // The whole extent SHould be passed in as the ext.
  // It is modified to return the result.
  int SplitExtent(int piece, int numPieces, int *ext);  


  int Piece;
  int NumberOfPieces;
  int GhostLevel;
  int Extent[6];
  int WholeExtent[6];

  int MinimumPieceSize[3];

};

#endif

