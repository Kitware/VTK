/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHierarchicalBoxDataSet.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHierarchicalBoxDataSet.h"

#include "vtkHierarchicalBoxDataSetInternal.h"
#include "vtkHierarchicalBoxVisitor.h"

#include "vtkObjectFactory.h"
#include "vtkUniformGrid.h"
#include "vtkUnsignedCharArray.h"

vtkCxxRevisionMacro(vtkHierarchicalBoxDataSet, "1.2");
vtkStandardNewMacro(vtkHierarchicalBoxDataSet);

//----------------------------------------------------------------------------
vtkHierarchicalBoxDataSet::vtkHierarchicalBoxDataSet()
{
  this->BoxInternal = new vtkHierarchicalBoxDataSetInternal;
}

//----------------------------------------------------------------------------
vtkHierarchicalBoxDataSet::~vtkHierarchicalBoxDataSet()
{
  delete this->BoxInternal;
}

//----------------------------------------------------------------------------
vtkCompositeDataVisitor* vtkHierarchicalBoxDataSet::NewVisitor()
{
  vtkHierarchicalBoxVisitor* vis = vtkHierarchicalBoxVisitor::New();
  vis->SetDataSet(this);
  return vis;
}

//----------------------------------------------------------------------------
vtkHDSNode* vtkHierarchicalBoxDataSet::NewNode()
{
  return new vtkHBDSNode;
}

//----------------------------------------------------------------------------
void vtkHierarchicalBoxDataSet::SetDataSet(
  unsigned int level, unsigned int id, vtkAMRBox& box, vtkUniformGrid* dataSet)
{
  this->Superclass::SetDataSet(level, id, dataSet);

  vtkHierarchicalDataSetInternal::LevelDataSetsType& ldataSets = 
    this->Internal->DataSets[level];
  static_cast<vtkHBDSNode*>(ldataSets[id])->Box = box;
}

//----------------------------------------------------------------------------
vtkUniformGrid* vtkHierarchicalBoxDataSet::GetDataSet(unsigned int level, 
                                                      unsigned int id,
                                                      vtkAMRBox& box)
{
  if (this->Internal->DataSets.size() <= level)
    {
    return 0;
    }

  vtkHierarchicalDataSetInternal::LevelDataSetsType& ldataSets = 
    this->Internal->DataSets[level];
  if (ldataSets.size() <= id)
    {
    return 0;
    }

  if (!ldataSets[id])
    {
    return 0;
    }

  box = static_cast<vtkHBDSNode*>(ldataSets[id])->Box;
  return static_cast<vtkUniformGrid*>(ldataSets[id]->DataSet.GetPointer());
}

//----------------------------------------------------------------------------
void vtkHierarchicalBoxDataSet::SetRefinementRatio(unsigned int level, 
                                                   int ratio)
{
  if (level >= this->BoxInternal->RefinementRatios.size())
    {
    this->BoxInternal->RefinementRatios.resize(level+1);
    }
  this->BoxInternal->RefinementRatios[level] = ratio;
}

//----------------------------------------------------------------------------
int vtkHierarchicalBoxDataSetIsInBoxes(vtkstd::vector<vtkAMRBox>& boxes,
                                       int i, int j, int k)
{
  vtkstd::vector<vtkAMRBox>::iterator it;
  for(it = boxes.begin(); it != boxes.end(); it++)
    {
    if (it->DoesContainCell(i, j, k))
      {
      return 1;
      }
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkHierarchicalBoxDataSet::GenerateVisibilityArrays()
{
  unsigned int numLevels = this->GetNumberOfLevels();
  for (unsigned int levelIdx=0; levelIdx<numLevels-1; levelIdx++)
    {

    // Copy boxes of higher level and coarsen to this level
    vtkstd::vector<vtkAMRBox> boxes;
    vtkHierarchicalDataSetInternal::LevelDataSetsType& ldataSets = 
      this->Internal->DataSets[levelIdx+1];
    vtkHierarchicalDataSetInternal::LevelDataSetsIterator ldx;
    for (ldx = ldataSets.begin(); ldx != ldataSets.end(); ldx++)
      {
      if (! *ldx )
        {
        continue;
        }
      vtkAMRBox coarsebox = 
        static_cast<vtkHBDSNode*>(*ldx)->Box;
      if (this->BoxInternal->RefinementRatios.size() <= levelIdx)
        {
        continue;
        }
      coarsebox.Coarsen(this->BoxInternal->RefinementRatios[levelIdx]);
      boxes.push_back(coarsebox);
      }

    unsigned int numDataSets = this->GetNumberOfDataSets(levelIdx);
    for (unsigned int dataSetIdx=0; dataSetIdx<numDataSets; dataSetIdx++)
      {
      vtkAMRBox box;
      vtkUniformGrid* grid = this->GetDataSet(levelIdx, dataSetIdx, box);
      if (grid)
        {
        int i;
        int cellDims[3];
        for (i=0; i<3; i++)
          {
          cellDims[i] = box.HiCorner[i] - box.LoCorner[i] + 1;
          }
        vtkUnsignedCharArray* vis = vtkUnsignedCharArray::New();
        vtkIdType numCells = box.GetNumberOfCells();
        vis->SetNumberOfTuples(numCells);
        for (i=0; i<numCells; i++)
          {
          vis->SetValue(i, 1);
          }

        
        for (int iz=0; iz<cellDims[2]; iz++)
          {
          for (int ix=0; ix<cellDims[0]; ix++)
            {
            for (int iy=0; iy<cellDims[1]; iy++)
              {
              // Blank if cell is covered by a box of higher level
              if (vtkHierarchicalBoxDataSetIsInBoxes(boxes, ix, iy, iz))
                {
                vtkIdType id = 
                  iz*cellDims[0]*cellDims[1] +
                  iy*cellDims[0] +
                  ix;
                vis->SetValue(id, 0);
                }
              }
            }
          }
        grid->SetCellVisibilityArray(vis);
        vis->Delete();
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkHierarchicalBoxDataSet::ShallowCopy(vtkDataObject *src)
{
  if (src == this)
    {
    return;
    }
  this->InitializeDataSets();
  this->Modified();

  vtkHierarchicalBoxDataSet* from = 
    vtkHierarchicalBoxDataSet::SafeDownCast(src);
  if (from)
    {
    // If the source is a vtkHierarchicalBoxDataSet, do not call
    // superclass' ShallowCopy, instead skip to vtkCompositeDataSet's
    // constructor
    this->vtkCompositeDataSet::ShallowCopy(src);

    unsigned int numLevels = from->GetNumberOfLevels();
    this->SetNumberOfLevels(numLevels);
    for (unsigned int i=0; i<numLevels; i++)
      {
      unsigned int numDataSets = from->GetNumberOfDataSets(i);
      this->SetNumberOfDataSets(i, numDataSets);
      for (unsigned int j=0; j<numDataSets; j++)
        {
        vtkAMRBox box;
        vtkUniformGrid* grid = from->GetDataSet(i, j, box);
        this->SetDataSet(i, j, box, grid);
        }
      }
    }
  else
    {
    this->Superclass::ShallowCopy(src);
    }
}

//----------------------------------------------------------------------------
void vtkHierarchicalBoxDataSet::DeepCopy(vtkDataObject *src)
{
  if (src == this)
    {
    return;
    }
  this->InitializeDataSets();
  this->Modified();

  vtkHierarchicalBoxDataSet* from = 
    vtkHierarchicalBoxDataSet::SafeDownCast(src);
  if (from)
    {
    // If the source is a vtkHierarchicalBoxDataSet, do not call
    // superclass' DeepCopy, instead skip to vtkCompositeDataSet's
    // constructor
    this->vtkCompositeDataSet::ShallowCopy(src);

    unsigned int numLevels = from->GetNumberOfLevels();
    this->SetNumberOfLevels(numLevels);
    for (unsigned int i=0; i<numLevels; i++)
      {
      unsigned int numDataSets = from->GetNumberOfDataSets(i);
      this->SetNumberOfDataSets(i, numDataSets);
      for (unsigned int j=0; j<numDataSets; j++)
        {
        vtkAMRBox box;
        vtkUniformGrid* ds = from->GetDataSet(i, j, box);
        if (ds)
          {
          vtkUniformGrid* copy = ds->NewInstance();
          copy->DeepCopy(ds);
          this->SetDataSet(i, j, box, copy);
          }
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkHierarchicalBoxDataSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

