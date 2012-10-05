/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCompositeDataSets.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCompositeDataIterator.h"
#include "vtkDataObjectTree.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkInformation.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkSmartPointer.h"
#include "vtkStdString.h"
#include "vtkUniformGrid.h"
#include "vtkUniformGridAMR.h"

#include <iostream>
#include <vector>

// Test DataObjectTreeIterator-specific methods
bool TestDataObjectTreeIterator()
{
  bool ok = true;
  vtkNew<vtkMultiBlockDataSet> data;
  int blocksPerLevel[3] = {1,4,9};
  std::vector<vtkSmartPointer<vtkMultiBlockDataSet> > blocks;
  blocks.push_back(data.GetPointer());
  unsigned levelStart = 0;
  unsigned levelEnd = 1;
  int numLevels = sizeof(blocksPerLevel) / sizeof(blocksPerLevel[0]);
  int numLeaves = 0;
  int numNodes = 0;
  vtkStdString blockName("Rolf");
  for (int level = 1; level < numLevels; ++level)
    {
    int nblocks=blocksPerLevel[level];
    for (unsigned parent = levelStart; parent < levelEnd; ++parent)
      {
      blocks[parent]->SetNumberOfBlocks(nblocks);
      for (int block=0; block < nblocks; ++block, ++numNodes)
        {
        if (level == numLevels - 1)
          {
          vtkNew<vtkUniformGrid> child;
          blocks[parent]->SetBlock(
            block, block % 2 ? NULL : child.GetPointer());
          blocks[parent]->GetMetaData(block)->Set(
            vtkCompositeDataSet::NAME(), blockName.c_str());
          ++numLeaves;
          }
        else
          {
          vtkNew<vtkMultiBlockDataSet> child;
          blocks[parent]->SetBlock(block, child.GetPointer());
          blocks.push_back(child.GetPointer());
          }
        }
      }
    levelStart = levelEnd;
    levelEnd = static_cast<unsigned>(blocks.size());
    }

  vtkSmartPointer<vtkDataObjectTreeIterator> it;
  it.TakeReference(data->NewTreeIterator());
  int counter;

  it->VisitOnlyLeavesOn();
  it->SkipEmptyNodesOff();
  counter = 0;
  for (it->InitTraversal(); !it->IsDoneWithTraversal(); it->GoToNextItem())
    {
    ++counter;
    if (blockName != it->GetCurrentMetaData()->Get(vtkCompositeDataSet::NAME()))
      {
      cerr << "Unnamed leaf node!\n";
      ok = false;
      }
    }
  cout << "Expecting " << numLeaves << " leaf nodes got " <<  counter << endl;;
  ok |= counter == numLeaves;

  it->VisitOnlyLeavesOff();
  it->SkipEmptyNodesOff();
  counter = 0;
  for (it->InitTraversal(); !it->IsDoneWithTraversal(); it->GoToNextItem())
    {
    ++counter;
    }
  cout << "Expecting " << numNodes << " total nodes got " <<  counter << endl;;
  ok |= counter == numNodes;

  it->VisitOnlyLeavesOff();
  it->TraverseSubTreeOff();
  it->SkipEmptyNodesOff();
  counter = 0;
  for (it->InitTraversal(); !it->IsDoneWithTraversal(); it->GoToNextItem())
    {
    ++counter;
    }
  cout << "Expecting " << blocksPerLevel[1] << " top-level nodes got " <<  counter << endl;;
  ok |= counter == blocksPerLevel[1];

  return ok;
}

bool TestEmptyAMRIterator()
{
  for(int init=0; init<2; init++)
    {
    for(int op=0; op<2; op++)
      {
      vtkSmartPointer<vtkUniformGridAMR> a = vtkSmartPointer<vtkUniformGridAMR>::New();
      if(init==1)
        {
        a->Initialize();
        }
      vtkSmartPointer<vtkCompositeDataIterator> iter;
      iter.TakeReference(a->NewIterator());
      iter->SetSkipEmptyNodes(op);
      int numIteration=0;
      for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
        {
        numIteration++;
        }
      if(numIteration!=0)
        {
        return false;
        }
      }
    }

  return true;
}

//Test converting AMR to a multiblock data structure
//and associated APIs
bool TestAMRToMultiBlock()
{
  vtkSmartPointer<vtkUniformGridAMR> a = vtkSmartPointer<vtkUniformGridAMR>::New();
  int blocksPerLevel[3] = {1,4,9};
  a->Initialize(3, blocksPerLevel);
  for(unsigned int i=0; i<a->GetNumberOfLevels();i++)
    {
    for(unsigned int j=0; j<a->GetNumberOfDataSets(i); j++)
      {
      vtkSmartPointer<vtkUniformGrid> grid = vtkSmartPointer<vtkUniformGrid>::New();
      a->SetDataSet(i,j, grid);
      }
    }

  vtkSmartPointer<vtkMultiBlockDataSet> b= vtkSmartPointer<vtkMultiBlockDataSet>::New();
  b->CopyStructure(a);

  vtkSmartPointer<vtkCompositeDataIterator> aIter;
  aIter.TakeReference(a->NewIterator());
  aIter->SkipEmptyNodesOff();

  for (aIter->InitTraversal(); !aIter->IsDoneWithTraversal(); aIter->GoToNextItem())
    {
    b->SetDataSet(aIter, aIter->GetCurrentDataObject());
    vtkDataObject* obj = b->GetDataSet(aIter);
    vtkUniformGrid* grid = vtkUniformGrid::SafeDownCast(obj);
    if(!grid)
      {
      return false;
      }
    }

  unsigned int numBlocks = 0;
  vtkSmartPointer<vtkCompositeDataIterator> bIter;
  bIter.TakeReference(b->NewIterator());
  aIter->SkipEmptyNodesOff();
  for (aIter->InitTraversal(); !aIter->IsDoneWithTraversal(); aIter->GoToNextItem())
    {
    numBlocks++;
    }

  if(numBlocks!=a->GetTotalNumberOfBlocks())
    {
    return false;
    }
  return true;
}

//------------------------------------------------------------------------------
int TestCompositeDataSets(int , char *[])
{
  int errors = 0;
  errors+= !TestDataObjectTreeIterator();
  errors+= !TestAMRToMultiBlock();
  errors+= !TestEmptyAMRIterator();
  return( errors );
}
