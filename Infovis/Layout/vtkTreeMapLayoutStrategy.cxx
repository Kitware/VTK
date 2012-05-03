/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTreeMapLayoutStrategy.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkTreeMapLayoutStrategy.h"

#include "vtkAdjacentVertexIterator.h"
#include "vtkFloatArray.h"
#include "vtkTree.h"


vtkTreeMapLayoutStrategy::vtkTreeMapLayoutStrategy()
{
}

vtkTreeMapLayoutStrategy::~vtkTreeMapLayoutStrategy()
{
}

void vtkTreeMapLayoutStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

void vtkTreeMapLayoutStrategy::AddBorder(float *boxInfo)
{
  float dx, dy;
  dx = 0.5 * (boxInfo[1] - boxInfo[0]) * this->ShrinkPercentage;
  dy = 0.5 * (boxInfo[3] - boxInfo[2]) * this->ShrinkPercentage;
  boxInfo[0] += dx;
  boxInfo[1] -= dx;
  boxInfo[2] += dy;
  boxInfo[3] -= dy;
}

vtkIdType vtkTreeMapLayoutStrategy::FindVertex(
    vtkTree* otree, vtkDataArray* array, float pnt[2])
{
  // Check to see that we are in the dataset at all
  float blimits[4];

  vtkIdType vertex = otree->GetRoot();
  vtkFloatArray *boxInfo = vtkFloatArray::SafeDownCast(array);
  // Now try to find the vertex that contains the point
  boxInfo->GetTupleValue(vertex, blimits); // Get the extents of the root
  if ((pnt[0] < blimits[0]) || (pnt[0] > blimits[1]) ||
      (pnt[1] < blimits[2]) || (pnt[1] > blimits[3]))
    {
    // Point is not in the tree at all
    return -1;
    }

  // Now traverse the children to try and find
  // the vertex that contains the point
  vtkIdType child;
#if 0
  if (binfo)
    {
    binfo[0] = blimits[0];
    binfo[1] = blimits[1];
    binfo[2] = blimits[2];
    binfo[3] = blimits[3];
    }
#endif

  vtkAdjacentVertexIterator *it = vtkAdjacentVertexIterator::New();
  otree->GetAdjacentVertices(vertex, it);
  while (it->HasNext())
    {
    child = it->Next();
    boxInfo->GetTupleValue(child, blimits); // Get the extents of the child
    if ((pnt[0] < blimits[0]) || (pnt[0] > blimits[1]) ||
            (pnt[1] < blimits[2]) || (pnt[1] > blimits[3]))
      {
      continue;
      }
    // If we are here then the point is contained by the child
    // So recurse down the children of this vertex
    vertex = child;
    otree->GetAdjacentVertices(vertex, it);
    }
  it->Delete();

  return vertex;
}

