/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGetGhostCells.cxx
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
#include "vtkGetGhostCells.h"
#include "vtkObjectFactory.h"
#include "vtkGhostLevels.h"


//------------------------------------------------------------------------------
vtkGetGhostCells* vtkGetGhostCells::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkGetGhostCells");
  if(ret)
    {
    return (vtkGetGhostCells*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkGetGhostCells;
}

//----------------------------------------------------------------------------
vtkGetGhostCells::vtkGetGhostCells()
{
  this->InputList = NULL;
}

//----------------------------------------------------------------------------
vtkGetGhostCells::~vtkGetGhostCells()
{
  if (this->InputList != NULL)
    {
    this->InputList->Delete();
    this->InputList = NULL;
    }
}

//----------------------------------------------------------------------------
// Add a piece of a dataset to the list of data to look for ghost cells in.
void vtkGetGhostCells::AddInput(vtkDataSet *ds)
{
  this->vtkProcessObject::AddInput(ds);
}

//----------------------------------------------------------------------------
vtkDataSet *vtkGetGhostCells::GetInput(int idx)
{
  if (idx >= this->NumberOfInputs || idx < 0)
    {
    return NULL;
    }
  
  return (vtkDataSet *)(this->Inputs[idx]);
}

//----------------------------------------------------------------------------
// Remove a piece of a dataset from the list to look for ghost cells in.
void vtkGetGhostCells::RemoveInput(vtkDataSet *ds)
{
  this->vtkProcessObject::RemoveInput(ds);
}

//----------------------------------------------------------------------------
vtkDataSetCollection *vtkGetGhostCells::GetInputList()
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
void vtkGetGhostCells::Execute()
{
  vtkDataSet *input = this->GetInput();
  vtkUnstructuredGrid *output = this->GetOutput();
  vtkGhostLevels *ghostLevels = vtkGhostLevels::New();
  int ghostLevel = output->GetUpdateGhostLevel();
  vtkPointLocator **locators;
  int numInputs = this->GetNumberOfInputs();
  int numCells = input->GetNumberOfCells();
  int numPoints;
  int i, j;
  vtkPoints *points = vtkPoints::New();
  vtkGenericCell *cell = vtkGenericCell::New();
  float point[3];
  float bounds[6], localBounds[6];
  
  locators = (vtkPointLocator**)malloc(numInputs * sizeof(vtkPointLocator));
  output->Initialize();
  output->Allocate();
  
  bounds[0] = 0.0;
  bounds[1] = 1.0;
  bounds[2] = 0.0;
  bounds[3] = 1.0;
  bounds[4] = 0.0;
  bounds[5] = 1.0;
  
  // Get the bounds for the entire data set
  for (i = 0; i < numInputs; i++)
    {
    this->GetInput(i)->GetBounds(localBounds);
    
    if (localBounds[0] < bounds[0])
      {
      bounds[0] = localBounds[0];
      }
    if (localBounds[1] > bounds[1])
      {
      bounds[1] = localBounds[1];
      }
    if (localBounds[2] < bounds[2])
      {
      bounds[2] = localBounds[2];
      }
    if (localBounds[3] > bounds[3])
      {
      bounds[3] = localBounds[3];
      }
    if (localBounds[4] < bounds[4])
      {
      bounds[4] = localBounds[4];
      }
    if (localBounds[5] > bounds[5])
      {
      bounds[5] = localBounds[5];
      }
    }
  
  for (i = 0; i < numInputs; i++)
    {
    locators[i] = vtkPointLocator::New();
    locators[i]->InitPointInsertion(vtkPoints::New(), bounds);
    numPoints = this->GetInput(i)->GetNumberOfPoints();
    for (j = 0; j < numPoints; j++)
      {
      this->GetInput(i)->GetPoint(j, point);
      locators[i]->InsertPoint(j, point);
      if (i == 0)
        {
        points->InsertPoint(j, point);
        }
      }
    }
  
  output->SetPoints(points);
  
  for (i = 0; i < numCells; i++)
    {
    input->GetCell(i, cell);
    output->InsertNextCell(cell->GetCellType(), cell->GetPointIds());
    ghostLevels->InsertNextGhostLevel(0);
    }
  
  for (i = 0; i < ghostLevel; i++)
    {
    this->AddGhostLevel(output, i+1, points, locators, numInputs, ghostLevels);
    }
  
  output->GetCellData()->SetGhostLevels(ghostLevels);
  
  points->Delete();
  ghostLevels->Delete();
  free(locators);
  cell->Delete();
}

//----------------------------------------------------------------------------
void vtkGetGhostCells::AddGhostLevel(vtkUnstructuredGrid *output,
				     int ghostLevel, vtkPoints *points,
				     vtkPointLocator **locators,
				     int numInputs,
				     vtkGhostLevels *ghostLevels)
{
  int i, j, k, l;
  vtkPoints *cellPoints = vtkPoints::New();
  vtkPoints *newCellPoints;
  vtkGenericCell *newCell = vtkGenericCell::New();
  int pointId, newPointId, *pointIds, numNewCellPoints;
  float point[3], newPoint[3];
  vtkIdList *cellIds = vtkIdList::New();
  int numPoints = points->GetNumberOfPoints();
  
  for (i = 0; i < numPoints; i++)
    {
    points->GetPoint(i, point);
    for (j = 1; j < numInputs; j++)
      {
      if ((pointId = locators[j]->IsInsertedPoint(point)) != -1)
        {
        this->GetInput(j)->GetPointCells(pointId, cellIds);
        for (k = 0; k < cellIds->GetNumberOfIds(); k++)
          {
          this->GetInput(j)->GetCell(cellIds->GetId(k), newCell);
          newCellPoints = newCell->GetPoints();
          numNewCellPoints = newCellPoints->GetNumberOfPoints();
          pointIds = (int*)malloc(numNewCellPoints * sizeof(int));
          for (l = 0; l < numNewCellPoints; l++)
            {
            newCellPoints->GetPoint(l, newPoint);
            if ((newPointId = locators[0]->IsInsertedPoint(newPoint)) == -1)
              {
              pointIds[l] = points->InsertNextPoint(newPoint);
              locators[0]->InsertPoint(pointIds[l], newPoint);
              output->SetPoints(points);
              } // end if point not found
            else
              {
              pointIds[l] = newPointId;
              } // end else
            } // end for num points in cell
          output->InsertNextCell(newCell->GetCellType(), numNewCellPoints,
                                 pointIds);
          ghostLevels->InsertNextGhostLevel(ghostLevel);
          free(pointIds);
          } // end for num cells using this point
        } // end if point located
      } // end for input > 0
    } // end for each point
  
  newCell->Delete();
  newCell = NULL;
  cellPoints->Delete();
  cellIds->Delete();
}

//----------------------------------------------------------------------------
void vtkGetGhostCells::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToUnstructuredGridFilter::PrintSelf(os,indent);
}
