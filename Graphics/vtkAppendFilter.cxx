/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAppendFilter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAppendFilter.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkAppendFilter, "1.58");
vtkStandardNewMacro(vtkAppendFilter);

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
  vtkIdType numPts, numCells, ptOffset, cellOffset;
  int   tenth, count, abort=0;
  float decimal;
  vtkPoints *newPts;
  vtkPointData *pd = NULL;
  vtkCellData *cd = NULL;
  vtkIdList *ptIds, *newPtIds;
  int i, idx;
  vtkDataSet *ds;
  vtkIdType ptId, cellId, newCellId;
  vtkUnstructuredGrid *output = this->GetOutput();
  vtkPointData *outputPD = output->GetPointData();
  vtkCellData *outputCD = output->GetCellData();
  
  vtkDebugMacro(<<"Appending data together");

  // Loop over all data sets, checking to see what data is common to 
  // all inputs. Note that data is common if 1) it is the same attribute 
  // type (scalar, vector, etc.), 2) it is the same native type (int, 
  // float, etc.), and 3) if a data array in a field, if it has the same name.
  count   = 0;
  decimal = 0.0;

  numPts = 0;
  numCells = 0;

  vtkDataSetAttributes::FieldList ptList(this->NumberOfInputs);
  vtkDataSetAttributes::FieldList cellList(this->NumberOfInputs);
  int firstPD=1;
  int firstCD=1;
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
      if ( firstPD )
        {
        ptList.InitializeFieldList(pd);
        firstPD = 0;
        }
      else
        {
        ptList.IntersectFieldList(pd);
        }
      
      cd = ds->GetCellData();
      if ( firstCD )
        {
        cellList.InitializeFieldList(cd);
        firstCD = 0;
        }
      else
        {
        cellList.IntersectFieldList(cd);
        }
      }//if non-empty dataset
    }//for all inputs

  if ( numPts < 1 || numCells < 1 )
    {
    vtkErrorMacro(<<"No data to append!");
    return;
    }
  
  // Now can allocate memory
  output->Allocate(numCells); //allocate storage for geometry/topology
  outputPD->CopyAllocate(ptList,numPts);
  outputCD->CopyAllocate(cellList,numCells);

  newPts = vtkPoints::New();
  newPts->SetNumberOfPoints(numPts);
  ptIds = vtkIdList::New(); ptIds->Allocate(VTK_CELL_SIZE);
  newPtIds = vtkIdList::New(); newPtIds->Allocate(VTK_CELL_SIZE);
  
  // Append each input dataset together
  //
  tenth = (numPts + numCells)/10 + 1;
  ptOffset=0;
  cellOffset=0;
  for (idx = 0; idx < this->NumberOfInputs && !abort; ++idx)
    {
    ds = (vtkDataSet *)(this->Inputs[idx]);
    if (ds != NULL)
      {
      numPts = ds->GetNumberOfPoints();
      numCells = ds->GetNumberOfCells();
      pd = ds->GetPointData();
      
      // copy points and point data
      for (ptId=0; ptId < numPts && !abort; ptId++)
        {
        newPts->SetPoint(ptId+ptOffset,ds->GetPoint(ptId));
        outputPD->CopyData(ptList,pd,idx,ptId,ptId+ptOffset);

        // Update progress
        count++;
        if ( !(count % tenth) ) 
          {
          decimal += 0.1;
          this->UpdateProgress (decimal);
          abort = this->GetAbortExecute();
          }
        }
      
      cd = ds->GetCellData();
      // copy cell and cell data
      for (cellId=0; cellId < numCells && !abort; cellId++)
        {
        ds->GetCellPoints(cellId, ptIds);
        newPtIds->Reset ();
        for (i=0; i < ptIds->GetNumberOfIds(); i++)
          {
          newPtIds->InsertId(i,ptIds->GetId(i)+ptOffset);
          }
        newCellId = output->InsertNextCell(ds->GetCellType(cellId),newPtIds);
        outputCD->CopyData(cellList,cd,idx,cellId,newCellId);

        // Update progress
        count++;
        if ( !(count % tenth) ) 
          {
          decimal += 0.1;
          this->UpdateProgress (decimal);
          abort = this->GetAbortExecute();
          }
        }
      ptOffset+=numPts;
      cellOffset+=numCells;
      }
    }
  
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
  this->Superclass::PrintSelf(os,indent);
}
