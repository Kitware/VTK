/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkThreshold.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include <stdlib.h>
#include "vtkThreshold.h"

// Construct with lower threshold=0, upper threshold=1, and threshold 
// function=upper AllScalars=1.
vtkThreshold::vtkThreshold()
{
  this->LowerThreshold = 0.0;
  this->UpperThreshold = 1.0;
  this->AllScalars = 1;
  this->Connectivity = 0;
  this->ThresholdFunction = &vtkThreshold::Upper;
}

// Description:
// Criterion is cells whose scalars are less or equal to lower threshold.
void vtkThreshold::ThresholdByLower(float lower) 
{
  if ( this->LowerThreshold != lower )
    {
    this->LowerThreshold = lower; 
    this->ThresholdFunction = &vtkThreshold::Lower;
    this->Modified();
    }
}
                           
// Description:
// Criterion is cells whose scalars are greater or equal to upper threshold.
void vtkThreshold::ThresholdByUpper(float upper)
{
  if ( this->UpperThreshold != upper )
    {
    this->UpperThreshold = upper; 
    this->ThresholdFunction = &vtkThreshold::Upper;
    this->Modified();
    }
}
                           
// Description:
// Criterion is cells whose scalars are between lower and upper thresholds.
void vtkThreshold::ThresholdBetween(float lower, float upper)
{
  if ( this->LowerThreshold != lower || this->UpperThreshold != upper )
    {
    this->LowerThreshold = lower; 
    this->UpperThreshold = upper;
    this->ThresholdFunction = &vtkThreshold::Between;
    this->Modified();
    }
}
  
void vtkThreshold::Execute()
{
  vtkThresholdLinkedList *cellsToKeep, *temp;
  vtkIdList *cellPts, *pointMap;
  vtkIdList *newCellPts = new vtkIdList;
  vtkCell *cell;
  vtkFloatPoints *newPoints;
  vtkPointData *pd, *outPD;
  int i, ptId, newId, numPts, numCellPts;
  float *x;
  vtkUnstructuredGrid *output= this->GetOutput();
  

  vtkDebugMacro(<< "Executing threshold filter");

  numPts = this->Input->GetNumberOfPoints();
  output->Allocate(this->Input->GetNumberOfCells());
  newPoints = new vtkFloatPoints(numPts);
  pd = this->Input->GetPointData();
  outPD = output->GetPointData();
  outPD->CopyAllocate(pd);

  pointMap = new vtkIdList(numPts); // maps old point ids into new
  for (i=0; i < numPts; i++) pointMap->SetId(i,-1);
  
  // Determine which cells to keep
  cellsToKeep = this->ComputeCellsToKeep();
  // Copy the cells and their points.
  while (cellsToKeep)
    {
    cell = this->Input->GetCell(cellsToKeep->Id);
    cellPts = cell->GetPointIds();
    numCellPts = cell->GetNumberOfPoints();
    // Copy the points if they have not been copied already
    for (i=0; i < numCellPts; i++)
      {
      ptId = cellPts->GetId(i);
      if ( (newId = pointMap->GetId(ptId)) < 0 )
	{
	x = this->Input->GetPoint(ptId);
	newId = newPoints->InsertNextPoint(x);
	pointMap->SetId(ptId,newId);
	outPD->CopyData(pd,ptId,newId);
	}
      newCellPts->InsertId(i,newId);
      }
    // Copy the point ids to the new cell.
    output->InsertNextCell(cell->GetCellType(),*newCellPts);
    newCellPts->Reset();
    temp = cellsToKeep;
    cellsToKeep = cellsToKeep->Next;
    delete temp;
    }

  vtkDebugMacro(<< "Extracted " << output->GetNumberOfCells() 
       << " number of cells.");

  // now clean up / update ourselves
  pointMap->Delete();
  newCellPts->Delete();
  
  output->SetPoints(newPoints);
  newPoints->Delete();

  output->Squeeze();
}

//----------------------------------------------------------------------------
// This function returns a linked list of cell ids to keep.
vtkThresholdLinkedList *vtkThreshold::ComputeCellsToKeep()
{
  vtkThresholdLinkedList *temp, *cellsToKeep = NULL;
  int cellId;
  vtkIdList *cellPts;
  vtkScalars *inScalars;
  vtkCell *cell;
  int i, ptId, numCells, numCellPts;
  int keepCell;
  

  if ( ! (inScalars = this->Input->GetPointData()->GetScalars()) )
    {
    vtkErrorMacro(<<"No scalar data to threshold");
    return NULL;
    }
     
  numCells = this->Input->GetNumberOfCells();
  // Check that the scalars of each cell satisfy the threshold criterion
  for (cellId=0; cellId < numCells; cellId++)
    {
    cell = this->Input->GetCell(cellId);
    cellPts = cell->GetPointIds();
    numCellPts = cell->GetNumberOfPoints();
    
    if (this->AllScalars)
      {
      keepCell = 1;
      for ( i=0; keepCell && (i < numCellPts); i++)
	{
	ptId = cellPts->GetId(i);
	keepCell = 
	  (this->*(this->ThresholdFunction))(inScalars->GetScalar(ptId));
	}
      }
    else
      {
      keepCell = 0;
      for ( i=0; (!keepCell) && (i < numCellPts); i++)
	{
	ptId = cellPts->GetId(i);
	keepCell = 
	  (this->*(this->ThresholdFunction))(inScalars->GetScalar(ptId));
	}
      }
    
    if ( keepCell ) // satisfied thresholding
      {
      // Add to the linked list
      temp = (vtkThresholdLinkedList *)malloc(sizeof(vtkThresholdLinkedList));
      temp->Id = cellId;
      temp->Next = cellsToKeep;
      cellsToKeep = temp;
      } // satisfied thresholding
    } // for all cells

  if (this->Connectivity)
    {
    return this->ComputeConnectedCells(cellsToKeep);
    }
  
  return cellsToKeep;
}


//----------------------------------------------------------------------------
// This function takes a list of seeds (cell ids)  and returns a list
// of cells connected to the seeds.
vtkThresholdLinkedList *
vtkThreshold::ComputeConnectedCells(vtkThresholdLinkedList *seeds)
{
  vtkIdList *visitedList, *cellPts;
  vtkIdList neighborIds(VTK_CELL_SIZE);
  vtkThresholdLinkedList *returnList, *temp, *end;
  int idx, k, neighborId, ptId, numCells, numCellPts;
  vtkCell *cell;
  

  if (seeds == NULL)
    {
    return NULL;
    }
  
  // list to show which cells have been visited.
  visitedList = new vtkIdList(this->Input->GetNumberOfCells());
  for (idx = 0 ; idx < this->Input->GetNumberOfCells(); ++idx) 
    {
    visitedList->SetId(idx, -1);
    }

  // Find the end of the list (marking seeds as visited along the way.
  end = seeds;
  visitedList->SetId(end->Id, 0);
  while (end->Next)
    {
    end = end->Next;
    visitedList->SetId(end->Id, 0);
    }
  
  // breadth first search of connected cells, starting with the seeds.
  returnList = NULL;
  while(seeds)
    {
    // Get the first cell seed
    cell = this->Input->GetCell(seeds->Id);
    cellPts = cell->GetPointIds();
    numCellPts = cell->GetNumberOfPoints();
    // for each point of the cell
    for (idx = 0; idx < numCellPts; ++idx) 
      {
      ptId = cellPts->GetId(idx);
      // Find neighboring cells.
      this->Input->GetPointCells(ptId,neighborIds); 
      numCells = neighborIds.GetNumberOfIds();
      // For each neighboring cell
      for (k=0; k < numCells; k++) 
	{
	neighborId = neighborIds.GetId(k);
	// If the neighbor has not been visited
	if (visitedList->GetId(neighborId) == -1)
	  {
	  // Add neighbor to the end of the seed list
	  end->Next = (vtkThresholdLinkedList *)
	    malloc(sizeof(vtkThresholdLinkedList));
	  end->Next->Next = NULL;
	  end->Next->Id = neighborId;
	  end = end->Next;
	  visitedList->SetId(neighborId, 0);
	  }
	}
      }
    // Transfer cell from seed list to output list.
    temp = seeds;
    seeds = seeds->Next;
    temp->Next = returnList;
    returnList = temp;
    }
  
  return returnList;
}

    
	  
	    
  
  
  
  

void vtkThreshold::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToUnstructuredGridFilter::PrintSelf(os,indent);

  os << indent << "All Scalars: " << this->AllScalars << "\n";;
  if ( this->ThresholdFunction == &vtkThreshold::Upper )
    os << indent << "Threshold By Upper\n";

  else if ( this->ThresholdFunction == &vtkThreshold::Lower )
    os << indent << "Threshold By Lower\n";

  else if ( this->ThresholdFunction == &vtkThreshold::Between )
    os << indent << "Threshold Between\n";

  os << indent << "Lower Threshold: " << this->LowerThreshold << "\n";;
  os << indent << "Upper Threshold: " << this->UpperThreshold << "\n";;
  if (this->AllScalars)
    {
    os << indent << "AllScalars On\n";
    }
  else
    {
    os << indent << "AllScalars Off\n";
    }
  if (this->Connectivity)
    {
    os << indent << "Connectivity On\n";
    }
  else
    {
    os << indent << "Connectivity Off\n";
    }
}
