/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAppendFilter.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include "vtkAppendFilter.hh"

vtkAppendFilter::vtkAppendFilter()
{
}

// Description:
// Add a dataset to the list of data to append.
void vtkAppendFilter::AddInput(vtkDataSet *ds)
{
  if ( ! this->InputList.IsItemPresent(ds) )
    {
    this->Modified();
    this->InputList.AddItem(ds);
    }
}

// Description:
// Remove a dataset from the list of data to append.
void vtkAppendFilter::RemoveInput(vtkDataSet *ds)
{
  if ( this->InputList.IsItemPresent(ds) )
    {
    this->Modified();
    this->InputList.RemoveItem(ds);
    }
}

void vtkAppendFilter::Update()
{
  unsigned long int mtime, dsMtime;
  vtkDataSet *ds;

  // make sure input is available
  if ( this->InputList.GetNumberOfItems() < 1 )
    {
    vtkErrorMacro(<< "No input...can't execute!");
    return;
    }

  // prevent chasing our tail
  if (this->Updating) return;

  this->Updating = 1;
  for (mtime=0, this->InputList.InitTraversal(); (ds = this->InputList.GetNextItem()); )
    {
    ds->Update();
    dsMtime = ds->GetMTime();
    if ( dsMtime > mtime ) mtime = dsMtime;
    }
  this->Updating = 0;

  if ( mtime > this->ExecuteTime || this->GetMTime() > this->ExecuteTime )
    {
    for ( this->InputList.InitTraversal(); ds=this->InputList.GetNextItem(); )
      {
      if ( ds->GetDataReleased() ) ds->ForceUpdate();
      }

    if ( this->StartMethod ) (*this->StartMethod)(this->StartMethodArg);
    this->Output->Initialize(); //clear output
    this->Execute();
    this->ExecuteTime.Modified();
    this->SetDataReleased(0);
    if ( this->EndMethod ) (*this->EndMethod)(this->EndMethodArg);
    }

  for (this->InputList.InitTraversal(); (ds = this->InputList.GetNextItem()); )
    if ( ds->ShouldIReleaseData() ) ds->ReleaseData();
}

// Append data sets into single unstructured grid
void vtkAppendFilter::Execute()
{
  int scalarsPresent, vectorsPresent, normalsPresent, tcoordsPresent;
  int tensorsPresent, userDefinedPresent;
  int numPts, numCells, ptOffset;
  vtkFloatPoints *newPts;
  vtkPointData *pd;
  vtkIdList ptIds(VTK_CELL_SIZE), newPtIds(VTK_CELL_SIZE);
  int i;
  vtkDataSet *ds;
  int ptId, cellId;
  vtkUnstructuredGrid *output = (vtkUnstructuredGrid *)this->Output;
  vtkPointData *outputPD = output->GetPointData();
  
  vtkDebugMacro(<<"Appending data together");

  // loop over all data sets, checking to see what point data is available.
  numPts = 0;
  numCells = 0;
  scalarsPresent = 1;
  vectorsPresent = 1;
  normalsPresent = 1;
  tcoordsPresent = 1;
  tensorsPresent = 1;
  userDefinedPresent = 1;

  for (this->InputList.InitTraversal(); (ds = this->InputList.GetNextItem()); )
    {
    numPts += ds->GetNumberOfPoints();
    numCells += ds->GetNumberOfCells();
    pd = ds->GetPointData();
    if ( pd->GetScalars() == NULL ) scalarsPresent &= 0;
    if ( pd->GetVectors() == NULL ) vectorsPresent &= 0;
    if ( pd->GetNormals() == NULL ) normalsPresent &= 0;
    if ( pd->GetTCoords() == NULL ) tcoordsPresent &= 0;
    if ( pd->GetTensors() == NULL ) tensorsPresent &= 0;
    if ( pd->GetUserDefined() == NULL ) userDefinedPresent &= 0;
    }

  if ( numPts < 1 || numCells < 1 )
    {
    vtkErrorMacro(<<"No data to append!");
    return;
    }
  
  // Now can allocate memory
  output->Allocate(numCells); //allocate storage for geometry/topology
  if ( !scalarsPresent ) outputPD->CopyScalarsOff();
  if ( !vectorsPresent ) outputPD->CopyVectorsOff();
  if ( !normalsPresent ) outputPD->CopyNormalsOff();
  if ( !tcoordsPresent ) outputPD->CopyTCoordsOff();
  if ( !tensorsPresent ) outputPD->CopyTensorsOff();
  if ( !userDefinedPresent ) outputPD->CopyUserDefinedOff();
  outputPD->CopyAllocate(pd,numPts);

  newPts = new vtkFloatPoints(numPts);

  for (ptOffset=0, this->InputList.InitTraversal(); 
       (ds = this->InputList.GetNextItem()); ptOffset+=numPts)
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

    // copy cells
    for (cellId=0; cellId < numCells; cellId++)
      {
      ds->GetCellPoints(cellId,ptIds);
      for (i=0; i < ptIds.GetNumberOfIds(); i++)
        newPtIds.InsertId(i,ptIds.GetId(i)+ptOffset);
      output->InsertNextCell(ds->GetCellType(cellId),newPtIds);
      }
    }
//
// Update ourselves and release memory
//
  output->SetPoints(newPts);
  newPts->Delete();
}

void vtkAppendFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkFilter::PrintSelf(os,indent);

  os << indent << "Input DataSets:\n";
  this->InputList.PrintSelf(os,indent.GetNextIndent());
}



