/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkAMRUtilities.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "vtkAMRBox.h"
#include "vtkAMRInformation.h"
#include "vtkParallelAMRUtilities.h"
#include "vtkMultiProcessController.h"
#include "vtkOverlappingAMR.h"
#include "vtkUniformGrid.h"
#include "vtkSmartPointer.h"
#include "vtkCompositeDataIterator.h"
#include <cmath>
#include <limits>
#include <cassert>

#define PRINT(x) cout<<"("<<myRank<<")"<<x<<endl;

//------------------------------------------------------------------------------
void vtkParallelAMRUtilities::PrintSelf( std::ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
}

//------------------------------------------------------------------------------
void vtkParallelAMRUtilities::DistributeProcessInformation(vtkOverlappingAMR* amr,
                                                   vtkMultiProcessController *controller,
                                                   std::vector<int>& processMap)
{
  processMap.resize(amr->GetTotalNumberOfBlocks(),-1);
  vtkSmartPointer<vtkCompositeDataIterator> iter;
  iter.TakeReference(amr->NewIterator());
  iter->SkipEmptyNodesOn();

  if(!controller || controller->GetNumberOfProcesses()==1)
    {
    for(iter->GoToFirstItem(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
      {
      unsigned int index = iter->GetCurrentFlatIndex();
      processMap[index] = 0;
      }
    return;
    }
  vtkAMRInformation* amrInfo = amr->GetAMRInfo();
  int myRank = controller->GetLocalProcessId();
  int numProcs   = controller->GetNumberOfProcesses();

  //get the active process ids
  std::vector<int> myBlocks;
  for(iter->GoToFirstItem(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    myBlocks.push_back(iter->GetCurrentFlatIndex());
    }

  vtkIdType myNumBlocks = myBlocks.size();
  std::vector<vtkIdType> numBlocks(numProcs,0);
  numBlocks[myRank] = myNumBlocks;

  //gather the active process counts
  controller->AllGather( &myNumBlocks, &numBlocks[0], 1);

  //gather the blocks each process owns into one array
  std::vector<vtkIdType> offsets(numProcs,0);
  vtkIdType currentOffset(0);
  for(int i=0; i<numProcs;i++)
    {
    offsets[i] = currentOffset;
    currentOffset+=numBlocks[i];
    }
  PRINT("total # of active blocks: "<<currentOffset<<" out of total "<<amrInfo->GetTotalNumberOfBlocks());
  std::vector<int> allBlocks(currentOffset,-1);
  controller->AllGatherV(&myBlocks[0], &allBlocks[0], (vtkIdType)myBlocks.size(), &numBlocks[0], &offsets[0] );

#ifdef DEBUG
  if(myRank==0)
    {
    for(int i=0; i<numProcs; i++)
      {
      vtkIdType offset= offsets[i];
      int n = numBlocks[i];
      cout<<"Rank "<<i<<" has: ";
      for(vtkIdType j=offset; j<offset+n; j++)
        {
        cout<<allBlocks[j]<<" ";
        }
      cout<<endl;
      }
    }
#endif
  for(int rank=0; rank<numProcs; rank++)
    {
    int offset= offsets[rank];
    int n = numBlocks[rank];
    for(int j=offset; j<offset+n; j++)
      {
      int index = allBlocks[j];
      assert(index>=0);
      processMap[index] =rank;
      }
    }
}

//------------------------------------------------------------------------------
void vtkParallelAMRUtilities::StripGhostLayers(
        vtkOverlappingAMR *ghostedAMRData,
        vtkOverlappingAMR *strippedAMRData,
        vtkMultiProcessController *controller)
{
  vtkAMRUtilities::StripGhostLayers(ghostedAMRData, strippedAMRData);

  if( controller != NULL )
    {
    controller->Barrier();
    }
}

//------------------------------------------------------------------------------
void vtkParallelAMRUtilities::BlankCells(vtkOverlappingAMR* amr,  vtkMultiProcessController *myController)
{
  vtkAMRInformation* info = amr->GetAMRInfo();
  if(!info->HasRefinementRatio())
    {
    info->GenerateRefinementRatio();
    }
  if(!info->HasChildrenInformation())
    {
    info->GenerateParentChildInformation();
    }

  std::vector<int> processorMap;
  vtkParallelAMRUtilities::DistributeProcessInformation(amr,myController,processorMap);
  unsigned int numLevels =info->GetNumberOfLevels();
  for(unsigned int i=0; i<numLevels; i++)
    {
    vtkAMRUtilities::BlankGridsAtLevel(amr, i, info->GetChildrenAtLevel(i),processorMap);
    }
}
