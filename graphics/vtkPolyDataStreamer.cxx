/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataStreamer.cxx
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
#include "vtkPolyDataStreamer.h"
#include "vtkAppendPolyData.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkPolyDataStreamer* vtkPolyDataStreamer::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPolyDataStreamer");
  if(ret)
    {
    return (vtkPolyDataStreamer*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPolyDataStreamer;
}




//----------------------------------------------------------------------------
vtkPolyDataStreamer::vtkPolyDataStreamer()
{
  this->MemoryLimit = 1000000;
  this->UseMemoryLimit = 0;
  this->NumberOfStreamDivisions = 2;
  this->ColorByPiece = 0;
}

//----------------------------------------------------------------------------
vtkPolyDataStreamer::~vtkPolyDataStreamer()
{
}

//----------------------------------------------------------------------------
void vtkPolyDataStreamer::ComputeNumberOfStreamDivisionsFromMemoryLimit()
{
  int num, max;
  vtkPolyData *input = this->GetInput();
  vtkPolyData *output = this->GetOutput();

  if (this->UseMemoryLimit == 0)
    { // This method should never have been called.
    return;
    }

  if (input == NULL)
    {
    this->NumberOfStreamDivisions = 1;
    return;
    }
  
  max = input->GetMaximumNumberOfPieces();
  num = output->GetEstimatedMemorySize();
  num = 1 + num / this->MemoryLimit;
  
  if (num > max)
    {
    num = max;
    }
  
  this->NumberOfStreamDivisions = num;
}

//----------------------------------------------------------------------------
void vtkPolyDataStreamer::SetNumberOfStreamDivisions(int num)
{
  if (this->UseMemoryLimit == 0 && this->NumberOfStreamDivisions == num)
    {
    return;
    }

  this->Modified();
  this->UseMemoryLimit = 0;
  this->NumberOfStreamDivisions = num;
}

//----------------------------------------------------------------------------
int vtkPolyDataStreamer::GetNumberOfStreamDivisions()
{
  if (this->UseMemoryLimit)
    {
    this->ComputeNumberOfStreamDivisionsFromMemoryLimit();
    }
  return this->NumberOfStreamDivisions;
}

//----------------------------------------------------------------------------
void vtkPolyDataStreamer::ComputeInputUpdateExtents(vtkDataObject *output)
{  
  if (this->GetInput() == NULL)
    {
    vtkErrorMacro("Missing input");
    return;
    }

  this->vtkPolyDataSource::ComputeInputUpdateExtents(output);

  // If we are actually streaming, then bypass the normal update process.
  this->ComputeNumberOfStreamDivisionsFromMemoryLimit();
  if (this->NumberOfStreamDivisions > 1)
    {
    this->GetInput()->SetUpdateExtent(-1, 0, 0);
    }
}

//----------------------------------------------------------------------------
// Append data sets into single unstructured grid
void vtkPolyDataStreamer::Execute()
{
  vtkPolyData *input = this->GetInput();
  vtkPolyData *output = this->GetOutput();
  vtkPolyData *copy;
  vtkAppendPolyData *append = vtkAppendPolyData::New();
  int outPiece, outNumPieces, outGhost;
  int i, j, inPiece;
  vtkScalars *pieceColors = NULL;

  if (this->ColorByPiece)
    {
    pieceColors = vtkScalars::New();
    }

  outGhost = output->GetUpdateGhostLevel();
  outPiece = output->GetUpdatePiece();
  outNumPieces = output->GetUpdateNumberOfPieces();
  for (i = 0; i < this->NumberOfStreamDivisions; ++i)
    {
    inPiece = outPiece * this->NumberOfStreamDivisions + i;
    input->SetUpdateExtent(inPiece,outNumPieces *this->NumberOfStreamDivisions);
    input->Update();
    copy = vtkPolyData::New();
    copy->ShallowCopy(input);
    append->AddInput(copy);
    copy->Delete();
    copy = NULL;
    if (pieceColors)
      {
      for (j = 0; j < input->GetNumberOfCells(); ++j)
        {
        pieceColors->InsertNextScalar((float)inPiece);
        }
      }
    }

  append->Update();
  output->ShallowCopy(append->GetOutput());
  // set the piece and number of pieces back to the correct value
  // since the shallow copy of the append filter has overwritten them.
  output->SetUpdateNumberOfPieces(outNumPieces );
  output->SetUpdatePiece(outPiece);
  output->SetUpdateGhostLevel(outGhost);
  if (pieceColors)
    {
    output->GetCellData()->SetScalars(pieceColors);
    pieceColors->Delete();
    }
  append->Delete();
}


//----------------------------------------------------------------------------
void vtkPolyDataStreamer::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyDataToPolyDataFilter::PrintSelf(os,indent);

  os << indent << "NumberOfStreamDivisions: " << this->NumberOfStreamDivisions << endl;
  if (this->UseMemoryLimit)
    {
    os << indent << "MemoryLimit: " << this->MemoryLimit << endl;
    }
  os << indent << "ColorByPiece: " << this->ColorByPiece << endl;
}



