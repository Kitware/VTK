/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMaskPoints.cxx
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
#include "vtkMaskPoints.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkMaskPoints* vtkMaskPoints::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkMaskPoints");
  if(ret)
    {
    return (vtkMaskPoints*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkMaskPoints;
}

//----------------------------------------------------------------------------
vtkMaskPoints::vtkMaskPoints()
{
  this->OnRatio = 2;
  this->Offset = 0;
  this->RandomMode = 0;
  this->MaximumNumberOfPoints = VTK_LARGE_ID;
  this->GenerateVertices = 0;
}

//----------------------------------------------------------------------------
void vtkMaskPoints::Execute()
{
  vtkPoints *newPts;
  vtkPointData *pd;
  vtkIdType numNewPts;
  float *x;
  vtkIdType ptId, id;
  vtkPolyData *output = this->GetOutput();
  vtkPointData *outputPD = output->GetPointData();
  vtkDataSet *input= this->GetInput();
  vtkIdType numPts=input->GetNumberOfPoints();
  
  // Check input
  //
  vtkDebugMacro(<<"Masking points");

  if ( numPts < 1 )
    {
    vtkErrorMacro(<<"No data to mask!");
    return;
    }

  pd = input->GetPointData();
  id = 0;
  
  // Allocate space
  //
  numNewPts = numPts / this->OnRatio;
  if (numNewPts > this->MaximumNumberOfPoints)
    {
    numNewPts = this->MaximumNumberOfPoints;
    }
  newPts = vtkPoints::New();
  newPts->Allocate(numNewPts);
  outputPD->CopyAllocate(pd);

  // Traverse points and copy
  //
  int abort=0;
  vtkIdType progressInterval=numPts/20 +1;
  if ( this->RandomMode ) // retro mode
    {
    float cap;
    
    if (((float)numPts/this->OnRatio) > this->MaximumNumberOfPoints)
      {
      cap = 2.0*numPts/this->MaximumNumberOfPoints - 1;
      }
    else 
      {
      cap = 2.0*this->OnRatio - 1;
      }

    for (ptId = this->Offset; 
    (ptId < numPts) && (id < this->MaximumNumberOfPoints) && !abort;  
    ptId += (1 + (int)((float)vtkMath::Random()*cap)) )
      {
      x =  input->GetPoint(ptId);
      id = newPts->InsertNextPoint(x);
      outputPD->CopyData(pd,ptId,id);
      if ( ! (id % progressInterval) ) //abort/progress
        {
        this->UpdateProgress (0.5*id/numPts);
        abort = this->GetAbortExecute();
        }
      }
    }
  else // a.r. mode
    {
    for ( ptId = this->Offset; 
    (ptId < numPts) && (id < (this->MaximumNumberOfPoints-1)) && !abort;
    ptId += this->OnRatio )
      {
      x =  input->GetPoint(ptId);
      id = newPts->InsertNextPoint(x);
      outputPD->CopyData(pd,ptId,id);
      if ( ! (id % progressInterval) ) //abort/progress
        {
        this->UpdateProgress (0.5*id/numPts);
        abort = this->GetAbortExecute();
        }
      }
    }

  // Generate vertices if requested
  //
  if ( this->GenerateVertices )
    {
    vtkCellArray *verts = vtkCellArray::New();
    verts->Allocate(verts->EstimateSize(1,id+1));
    verts->InsertNextCell(id+1);
    for ( ptId=0; ptId<(id+1) && !abort; ptId++)
      {
      if ( ! (ptId % progressInterval) ) //abort/progress
        {
        this->UpdateProgress (0.5+0.5*ptId/(id+1));
        abort = this->GetAbortExecute();
        }
      verts->InsertCellPoint(ptId);
      }
    output->SetVerts(verts);
    verts->Delete();
    }

  // Update ourselves
  //
  output->SetPoints(newPts);
  newPts->Delete();
  
  output->Squeeze();

  vtkDebugMacro(<<"Masked " << numPts << " original points to " 
                << id+1 << " points");
}


//----------------------------------------------------------------------------
void vtkMaskPoints::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToPolyDataFilter::PrintSelf(os,indent);

  os << indent << "Generate Vertices: " 
     << (this->GenerateVertices ? "On\n" : "Off\n");
  os << indent << "MaximumNumberOfPoints: " 
     << this->MaximumNumberOfPoints << "\n";
  os << indent << "On Ratio: " << this->OnRatio << "\n";
  os << indent << "Offset: " << this->Offset << "\n";
  os << indent << "Random Mode: " << (this->RandomMode ? "On\n" : "Off\n");
}
