/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPPolyDataNormals.cxx
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
#include "vtkPPolyDataNormals.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkPPolyDataNormals* vtkPPolyDataNormals::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPPolyDataNormals");
  if(ret)
    {
    return (vtkPPolyDataNormals*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPPolyDataNormals;
}

//----------------------------------------------------------------------------
vtkPPolyDataNormals::vtkPPolyDataNormals()
{
  this->PieceInvariant = 1;
}

//----------------------------------------------------------------------------
void vtkPPolyDataNormals::Execute()
{
  vtkPolyData *output = this->GetOutput();

  this->vtkPolyDataNormals::Execute();
  
  if (this->PieceInvariant)
    {
    output->RemoveGhostCells(output->GetUpdateGhostLevel()+1);
    }
                                                                   
}

//--------------------------------------------------------------------------
void vtkPPolyDataNormals::ComputeInputUpdateExtents(vtkDataObject *output)
{
  vtkPolyData *input = this->GetInput();
  int piece = output->GetUpdatePiece();
  int numPieces = output->GetUpdateNumberOfPieces();
  int ghostLevel = output->GetUpdateGhostLevel();

  if (input == NULL)
    {
    return;
    }
  if (this->PieceInvariant)
    {
    input->SetUpdatePiece(piece);
    input->SetUpdateNumberOfPieces(numPieces);
    input->SetUpdateGhostLevel(ghostLevel + 1);
    }
  else
    {
    input->SetUpdatePiece(piece);
    input->SetUpdateNumberOfPieces(numPieces);
    input->SetUpdateGhostLevel(ghostLevel);
    }
}

//----------------------------------------------------------------------------
void vtkPPolyDataNormals::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyDataNormals::PrintSelf(os,indent);

  os << indent << "PieceInvariant: "
     << this->PieceInvariant << "\n";
}
