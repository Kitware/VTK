/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkReverseSense.cxx
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
#include "vtkReverseSense.h"
#include "vtkObjectFactory.h"

//---------------------------------------------------------------------------
vtkReverseSense* vtkReverseSense::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkReverseSense");
  if(ret)
    {
    return (vtkReverseSense*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkReverseSense;
}

// Construct object so that behavior is to reverse cell ordering and
// leave normal orientation as is.
vtkReverseSense::vtkReverseSense()
{
  this->ReverseCells = 1;
  this->ReverseNormals = 0;
}

void vtkReverseSense::Execute()
{
  vtkPolyData *input= this->GetInput();
  vtkPolyData *output= this->GetOutput();
  vtkDataArray *normals=input->GetPointData()->GetActiveNormals();
  vtkDataArray *cellNormals=input->GetCellData()->GetActiveNormals();

  vtkDebugMacro(<<"Reversing sense of poly data");

  output->CopyStructure(input);
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->PassData(input->GetCellData());

  //If specified, traverse all cells and reverse them
  int abort=0;
  vtkIdType progressInterval;
  
  if ( this->ReverseCells )
    {
    vtkIdType numCells=input->GetNumberOfCells();
    vtkCellArray *verts, *lines, *polys, *strips;

    //Instantiate necessary topology arrays
    verts = vtkCellArray::New();
    verts->DeepCopy(input->GetVerts());
    lines = vtkCellArray::New();
    lines->DeepCopy(input->GetLines());
    polys = vtkCellArray::New();
    polys->DeepCopy(input->GetPolys());
    strips = vtkCellArray::New();
    strips->DeepCopy(input->GetStrips());

    output->SetVerts(verts); verts->Delete();
    output->SetLines(lines); lines->Delete();
    output->SetPolys(polys);  polys->Delete();
    output->SetStrips(strips);  strips->Delete();

    progressInterval=numCells/10+1;
    for (vtkIdType cellId=0; cellId < numCells && !abort; cellId++ )
      {
      if ( ! (cellId % progressInterval) ) //manage progress / early abort
        {
        this->UpdateProgress (0.6*cellId/numCells);
        abort = this->GetAbortExecute();
        }
      output->ReverseCell(cellId);
      }
    }

  //If specified and normals available, reverse orientation of normals.
  // Using MakeObject() creates normals of the same data type.
  if ( this->ReverseNormals && normals )
    {
    //first do point normals
    vtkIdType numPoints=input->GetNumberOfPoints();
    vtkDataArray *outNormals=normals->MakeObject();
    outNormals->SetNumberOfTuples(numPoints);
    float n[3];

    progressInterval=numPoints/5+1;
    for ( int ptId=0; ptId < numPoints; ptId++ )
      {
      if ( ! (ptId % progressInterval) ) //manage progress / early abort
        {
        this->UpdateProgress (0.6 + 0.2*ptId/numPoints);
        abort = this->GetAbortExecute();
        }
      normals->GetTuple(ptId,n);
      n[0] = -n[0]; n[1] = -n[1]; n[2] = -n[2];
      outNormals->SetTuple(ptId,n);
      }

    output->GetPointData()->SetNormals(outNormals);
    outNormals->Delete();
    }
  
  //now do cell normals
  if ( this->ReverseNormals && cellNormals )
    {
    vtkIdType numCells=input->GetNumberOfCells();
    vtkDataArray *outNormals=cellNormals->MakeObject();
    outNormals->SetNumberOfTuples(numCells);
    float n[3];

    progressInterval=numCells/5+1;
    for (vtkIdType cellId=0; cellId < numCells; cellId++ )
      {
      if ( ! (cellId % progressInterval) ) //manage progress / early abort
        {
        this->UpdateProgress (0.8 + 0.2*cellId/numCells);
        abort = this->GetAbortExecute();
        }

      cellNormals->GetTuple(cellId,n);
      n[0] = -n[0]; n[1] = -n[1]; n[2] = -n[2];
      outNormals->SetTuple(cellId,n);
      }

    output->GetCellData()->SetNormals(outNormals);
    outNormals->Delete();
    }
}


void vtkReverseSense::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyDataToPolyDataFilter::PrintSelf(os,indent);

  os << indent << "Reverse Cells: " 
     << (this->ReverseCells ? "On\n" : "Off\n");
  os << indent << "Reverse Normals: " 
     << (this->ReverseNormals ? "On\n" : "Off\n");
}

