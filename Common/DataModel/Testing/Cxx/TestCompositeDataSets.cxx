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
#include "vtkDataObjectTree.h"
#include "vtkUniformGridAMR.h"
#include "vtkSmartPointer.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkUniformGrid.h"
#include "vtkCompositeDataIterator.h"
#include "vtkDataObjectTreeIterator.h"

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
  bIter.TakeReference( vtkDataObjectTreeIterator::SafeDownCast(b->NewIterator()));
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
  errors+= !TestAMRToMultiBlock();
  return( errors );
}
