/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataMapper.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindow.h"
#include "vtkGraphicsFactory.h"

// return the correct type of PolyDataMapper 
vtkPolyDataMapper *vtkPolyDataMapper::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkGraphicsFactory::CreateInstance("vtkPolyDataMapper");
  return (vtkPolyDataMapper*)ret;
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
void vtkPolyDataMapper::SetInput(vtkPolyData *input)
{
  this->vtkProcessObject::SetNthInput(0, input);
  this->Piece = 0;
  this->NumberOfPieces = 1;
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
vtkPolyData *vtkPolyDataMapper::GetInput()
{
  if (this->NumberOfInputs < 1)
    {
    return NULL;
    }
  
  return (vtkPolyData *)(this->Inputs[0]);
}


// Request a piece before we get the bounds.
float *vtkPolyDataMapper::GetBounds()
{
  if (this->GetInput() ) 
    {
    this->GetInput()->SetUpdateExtent(this->Piece, this->NumberOfPieces);
    }

  return this->vtkMapper::GetBounds();
}


// Update the network connected to this mapper.
void vtkPolyDataMapper::Update()
{
  if (this->GetInput() ) 
    {
    this->GetInput()->SetUpdateExtent(this->Piece, this->NumberOfPieces);
    }
  this->vtkMapper::Update();
}


void vtkPolyDataMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkMapper::PrintSelf(os,indent);

  os << indent << "Piece : " << this->Piece << endl;
  os << indent << "NumberOfPieces : " << this->NumberOfPieces << endl;
}