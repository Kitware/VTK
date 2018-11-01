/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStaticEdgeLocatorTemplate.txx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE file for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkStaticEdgeLocatorTemplate.h"

#include "vtkSMPTools.h"

#ifndef vtkStaticEdgeLocatorTemplate_txx
#define vtkStaticEdgeLocatorTemplate_txx

//----------------------------------------------------------------------------
// Gather coincident edges into contiguous runs. Use this for merging edges.
template <typename IDType, typename EdgeData>
const IDType* vtkStaticEdgeLocatorTemplate<IDType,EdgeData>::
MergeEdges(vtkIdType numEdges, MergeTupleType *mergeArray,vtkIdType &numUniqueEdges)
{
  // Sort the edges. Note that the sort is first on V0, then V1. So both
  // V0 and V1 are sorted in ascending order.
  this->NumEdges = numEdges;
  this->MergeArray = mergeArray;
  vtkSMPTools::Sort(this->MergeArray, this->MergeArray + numEdges);

  // Now build offsets, i.e., determine the number of unique edges and determine
  // the offsets into each identical group of edges.
  this->MergeOffsets.push_back(0);
  IDType curOffset=0;

  for ( IDType eId=1; eId < numEdges; ++eId )
  {
    if ( this->MergeArray[curOffset] != this->MergeArray[eId] )
    {
      this->MergeOffsets.push_back(eId);
      curOffset = eId;
    }
  }

  // This makes traversal a little easier
  numUniqueEdges = static_cast<vtkIdType>(this->MergeOffsets.size());
  this->MergeOffsets.push_back(numEdges);

  return this->MergeOffsets.data();
}

//----------------------------------------------------------------------------
// Build the locator from the edge array provided. The edgeArray is sorted,
// then offsets into the array provide rapid access to edges.
template <typename IDType, typename EdgeData>
vtkIdType vtkStaticEdgeLocatorTemplate<IDType,EdgeData>::
BuildLocator(vtkIdType numEdges, EdgeTupleType *edgeArray)
{
  // Sort the edges. Note that the sort is first on V0, then V1. So both
  // V0 and V1 are sorted in ascending order.
  this->EdgeArray = edgeArray;
  vtkSMPTools::Sort(this->EdgeArray, this->EdgeArray + numEdges);

  // Remove duplicates. What's left is a list of unique edges, with their
  // position in the edge array corresponding to their edge id.
  EdgeTupleType *end =
    std::unique(this->EdgeArray, this->EdgeArray + numEdges);
  this->NumEdges = end - this->EdgeArray;

  // Create an offset array to accelerate finding edges (v0,v1). Basically
  // this is a 1D binning based on v0, with a quick search is then made to
  // find v1. Recall at this point the edges have been sorted so that given
  // two edges (a,b) & (c,d) (with (a,b)id < (c,d,)id,
  // then a<=c; and if a==c, then b<d.
  this->MinV0 = this->EdgeArray[0].V0;
  this->MaxV0 = this->EdgeArray[this->NumEdges-1].V0;
  this->V0Range = this->MaxV0 - this->MinV0 + 1;
  this->NDivs = (this->V0Range / this->NumEdgesPerBin) + 1;
  this->EdgeOffsets = new IDType[this->NDivs+1]; //one extra simplifies math

  IDType pos, curPos = 0;
  IDType num, idx = 0;
  this->EdgeOffsets[idx++] = curPos;
  for ( IDType eId=0; eId < this->NumEdges; ++eId )
  {
    pos = this->HashBin(this->EdgeArray[eId].V0);
    if ( pos > curPos )
    {
      num = pos - curPos;
      for ( IDType i=0; i < num; ++i )
      {
        this->EdgeOffsets[idx++] = eId;
      }
      curPos = pos;
    }
  }
  while ( idx <= this->NDivs )
  {
    this->EdgeOffsets[idx++] = this->NumEdges; //mark the end
  }

  return this->NumEdges;
}


#endif
