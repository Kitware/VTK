/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellDataToPointData.cxx
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
#include "vtkCellDataToPointData.h"
#include "vtkObjectFactory.h"

//--------------------------------------------------------------------------
vtkCellDataToPointData* vtkCellDataToPointData::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkCellDataToPointData");
  if(ret)
    {
    return (vtkCellDataToPointData*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkCellDataToPointData;
}

// Instantiate object so that cell data is not passed to output.
vtkCellDataToPointData::vtkCellDataToPointData()
{
  this->PassCellData = 0;
}

#define VTK_MAX_CELLS_PER_POINT 4096

void vtkCellDataToPointData::Execute()
{
  vtkIdType cellId, ptId;
  vtkIdType numCells, numPts;
  vtkDataSet *input= this->GetInput();
  vtkDataSet *output= this->GetOutput();
  vtkCellData *inPD=input->GetCellData();
  vtkPointData *outPD=output->GetPointData();
  vtkIdList *cellIds;
  float weight, *weights=new float[VTK_MAX_CELLS_PER_POINT];

  vtkDebugMacro(<<"Mapping cell data to point data");

  // First, copy the input to the output as a starting point
  output->CopyStructure( input );

  cellIds = vtkIdList::New();
  cellIds->Allocate(VTK_MAX_CELLS_PER_POINT);

  if ( (numPts=input->GetNumberOfPoints()) < 1 )
    {
    vtkErrorMacro(<<"No input point data!");
    cellIds->Delete();
    return;
    }
  
  // Pass the point data first. The fields and attributes
  // which also exist in the cell data of the input will
  // be over-written during CopyAllocate
  output->GetPointData()->PassData(input->GetPointData());

  // notice that inPD and outPD are vtkCellData and vtkPointData; respectively.
  // It's weird, but it works.
  outPD->CopyAllocate(inPD,numPts);

  int abort=0;
  vtkIdType progressInterval=numPts/20 + 1;
  for (ptId=0; ptId < numPts && !abort; ptId++)
    {
    if ( !(ptId % progressInterval) )
      {
      this->UpdateProgress((float)ptId/numPts);
      abort = GetAbortExecute();
      }

    input->GetPointCells(ptId, cellIds);
    numCells = cellIds->GetNumberOfIds();
    if ( numCells > 0 )
      {
      weight = 1.0 / numCells;
      for (cellId=0; cellId < numCells; cellId++)
	{
	weights[cellId] = weight;
	}
      outPD->InterpolatePoint(inPD, ptId, cellIds, weights);
      }
    else
      {
      outPD->NullPoint(ptId);
      }
    }

  if ( this->PassCellData )
    {
    output->GetCellData()->PassData(input->GetCellData());
    }

  cellIds->Delete();
  delete [] weights;
}

void vtkCellDataToPointData::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToDataSetFilter::PrintSelf(os,indent);

  os << indent << "Pass Cell Data: " << (this->PassCellData ? "On\n" : "Off\n");
}
