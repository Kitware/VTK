/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAppendFilter.cxx
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
#include "vtkAppendFilter.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkAppendFilter* vtkAppendFilter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkAppendFilter");
  if(ret)
    {
    return (vtkAppendFilter*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkAppendFilter;
}




//----------------------------------------------------------------------------
vtkAppendFilter::vtkAppendFilter()
{
  this->InputList = NULL;
}

//----------------------------------------------------------------------------
vtkAppendFilter::~vtkAppendFilter()
{
  if (this->InputList != NULL)
    {
    this->InputList->Delete();
    this->InputList = NULL;
    }
}

//----------------------------------------------------------------------------
// Add a dataset to the list of data to append.
void vtkAppendFilter::AddInput(vtkDataSet *ds)
{
  this->vtkProcessObject::AddInput(ds);
}

//----------------------------------------------------------------------------
vtkDataSet *vtkAppendFilter::GetInput(int idx)
{
  if (idx >= this->NumberOfInputs || idx < 0)
    {
    return NULL;
    }
  
  return (vtkDataSet *)(this->Inputs[idx]);
}

//----------------------------------------------------------------------------
// Remove a dataset from the list of data to append.
void vtkAppendFilter::RemoveInput(vtkDataSet *ds)
{
  this->vtkProcessObject::RemoveInput(ds);
}

//----------------------------------------------------------------------------
vtkDataSetCollection *vtkAppendFilter::GetInputList()
{
  int idx;
  
  if (this->InputList)
    {
    this->InputList->Delete();
    }
  this->InputList = vtkDataSetCollection::New();
  
  for (idx = 0; idx < this->NumberOfInputs; ++idx)
    {
    if (this->Inputs[idx] != NULL)
      {
      this->InputList->AddItem((vtkDataSet*)(this->Inputs[idx]));
      }
    }  
  
  return this->InputList;
}



//----------------------------------------------------------------------------
// Append data sets into single unstructured grid
void vtkAppendFilter::Execute()
{
  int scalarsPresentInPD, vectorsPresentInPD;
  int normalsPresentInPD, tcoordsPresentInPD;
  int tensorsPresentInPD, fieldPresentInPD;
  int scalarsPresentInCD, vectorsPresentInCD;
  int normalsPresentInCD, tcoordsPresentInCD;
  int tensorsPresentInCD, fieldPresentInCD;
  int numPts, numCells, ptOffset, cellOffset;
  vtkPoints *newPts;
  vtkPointData *pd = NULL;
  vtkCellData *cd = NULL;
  vtkIdList *ptIds, *newPtIds;
  int i, idx;
  vtkDataSet *ds;
  int ptId, cellId, newCellId;
  vtkUnstructuredGrid *output = this->GetOutput();
  vtkPointData *outputPD = output->GetPointData();
  vtkCellData *outputCD = output->GetCellData();
  
  vtkDebugMacro(<<"Appending data together");

  // loop over all data sets, checking to see what point data is available.
  numPts = 0;
  numCells = 0;
  scalarsPresentInPD = 1;
  vectorsPresentInPD = 1;
  normalsPresentInPD = 1;
  tcoordsPresentInPD = 1;
  tensorsPresentInPD = 1;
  fieldPresentInPD = 1;
  scalarsPresentInCD = 1;
  vectorsPresentInCD = 1;
  normalsPresentInCD = 1;
  tcoordsPresentInCD = 1;
  tensorsPresentInCD = 1;
  fieldPresentInCD = 1;

  for (idx = 0; idx < this->NumberOfInputs; ++idx)
    {
    ds = (vtkDataSet *)(this->Inputs[idx]);
    if (ds != NULL)
      {
      if ( ds->GetNumberOfPoints() <= 0 && ds->GetNumberOfCells() <= 0 )
        {
        continue; //no input, just skip
        }
      numPts += ds->GetNumberOfPoints();
      numCells += ds->GetNumberOfCells();
      pd = ds->GetPointData();
      
      if ( pd && pd->GetScalars() == NULL )
        {
        scalarsPresentInPD &= 0;
        }
      if ( pd && pd->GetVectors() == NULL )
        {
        vectorsPresentInPD &= 0;
        }
      if ( pd && pd->GetNormals() == NULL )
        {
        normalsPresentInPD &= 0;
        }
      if ( pd && pd->GetTCoords() == NULL )
        {
        tcoordsPresentInPD &= 0;
        }
      if ( pd && pd->GetTensors() == NULL )
        {
        tensorsPresentInPD &= 0;
        }
      if ( pd && pd->GetFieldData() == NULL )
        {
        fieldPresentInPD &= 0;
        }
      
      cd = ds->GetCellData();
      if ( cd && cd->GetScalars() == NULL )
        {
        scalarsPresentInCD &= 0;
        }
      if ( cd && cd->GetVectors() == NULL )
        {
        vectorsPresentInCD &= 0;
        }
      if ( cd && cd->GetNormals() == NULL )
        {
        normalsPresentInCD &= 0;
        }
      if ( cd && cd->GetTCoords() == NULL )
        {
        tcoordsPresentInCD &= 0;
        }
      if ( cd && cd->GetTensors() == NULL )
        {
        tensorsPresentInCD &= 0;
        }
      if ( cd && cd->GetFieldData() == NULL )
        {
        fieldPresentInCD &= 0;
        }
      }
    }

  if ( numPts < 1 || numCells < 1 )
    {
    vtkErrorMacro(<<"No data to append!");
    return;
    }
  
  // Now can allocate memory
  output->Allocate(numCells); //allocate storage for geometry/topology
  if ( !scalarsPresentInPD )
    {
    outputPD->CopyScalarsOff();
    }
  if ( !vectorsPresentInPD )
    {
    outputPD->CopyVectorsOff();
    }
  if ( !normalsPresentInPD )
    {
    outputPD->CopyNormalsOff();
    }
  if ( !tcoordsPresentInPD )
    {
    outputPD->CopyTCoordsOff();
    }
  if ( !tensorsPresentInPD )
    {
    outputPD->CopyTensorsOff();
    }
  if ( !fieldPresentInPD )
    {
    outputPD->CopyFieldDataOff();
    }
  outputPD->CopyAllocate(pd,numPts);

  // now do cell data
  if ( !scalarsPresentInCD )
    {
    outputCD->CopyScalarsOff();
    }
  if ( !vectorsPresentInCD )
    {
    outputCD->CopyVectorsOff();
    }
  if ( !normalsPresentInCD )
    {
    outputCD->CopyNormalsOff();
    }
  if ( !tcoordsPresentInCD )
    {
    outputCD->CopyTCoordsOff();
    }
  if ( !tensorsPresentInCD )
    {
    outputCD->CopyTensorsOff();
    }
  if ( !fieldPresentInCD )
    {
    outputCD->CopyFieldDataOff();
    }
  outputCD->CopyAllocate(cd,numCells);

  newPts = vtkPoints::New();
  newPts->SetNumberOfPoints(numPts);
  ptIds = vtkIdList::New(); ptIds->Allocate(VTK_CELL_SIZE);
  newPtIds = vtkIdList::New(); newPtIds->Allocate(VTK_CELL_SIZE);

  
  ptOffset=0;
  cellOffset=0;
  for (idx = 0; idx < this->NumberOfInputs; ++idx)
    {
    ds = (vtkDataSet *)(this->Inputs[idx]);
    if (ds != NULL)
      {
      numPts = ds->GetNumberOfPoints();
      numCells = ds->GetNumberOfCells();
      pd = ds->GetPointData();
      
      // copy points and point data
      for (ptId=0; ptId < numPts; ptId++)
        {
        newPts->SetPoint(ptId+ptOffset,ds->GetPoint(ptId));
        outputPD->CopyData(pd,ptId,ptId+ptOffset);
        }
      
      cd = ds->GetCellData();
      // copy cell and cell data
      for (cellId=0; cellId < numCells; cellId++)
        {
        ds->GetCellPoints(cellId, ptIds);
        newPtIds->Reset ();
        for (i=0; i < ptIds->GetNumberOfIds(); i++)
          {
          newPtIds->InsertId(i,ptIds->GetId(i)+ptOffset);
          }
        newCellId = output->InsertNextCell(ds->GetCellType(cellId),newPtIds);
        outputCD->CopyData(cd,cellId,newCellId);
        }
      ptOffset+=numPts;
      cellOffset+=numCells;
      }
    }
  
  //
  // Update ourselves and release memory
  //
  output->SetPoints(newPts);
  newPts->Delete();
  ptIds->Delete();
  newPtIds->Delete();
}

//----------------------------------------------------------------------------
void vtkAppendFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToUnstructuredGridFilter::PrintSelf(os,indent);
}








