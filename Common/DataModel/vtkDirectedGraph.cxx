/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDirectedGraph.cxx

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

#include "vtkDirectedGraph.h"

#include "vtkInEdgeIterator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkOutEdgeIterator.h"
#include "vtkSmartPointer.h"

#include <vector>

//----------------------------------------------------------------------------
// class vtkDirectedGraph
//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkDirectedGraph);
//----------------------------------------------------------------------------
vtkDirectedGraph::vtkDirectedGraph()
{
}

//----------------------------------------------------------------------------
vtkDirectedGraph::~vtkDirectedGraph()
{
}

//----------------------------------------------------------------------------
vtkDirectedGraph *vtkDirectedGraph::GetData(vtkInformation *info)
{
  return info? vtkDirectedGraph::SafeDownCast(info->Get(DATA_OBJECT())) : 0;
}

//----------------------------------------------------------------------------
vtkDirectedGraph *vtkDirectedGraph::GetData(vtkInformationVector *v, int i)
{
  return vtkDirectedGraph::GetData(v->GetInformationObject(i));
}

//----------------------------------------------------------------------------
bool vtkDirectedGraph::IsStructureValid(vtkGraph *g)
{
  if (!g)
  {
    return false;
  }
  if (vtkDirectedGraph::SafeDownCast(g))
  {
    return true;
  }

  // Verify that each edge appears in exactly one in and one out edge list.
  std::vector<bool> in(g->GetNumberOfEdges(), false);
  std::vector<bool> out(g->GetNumberOfEdges(), false);
  vtkSmartPointer<vtkInEdgeIterator> inIter =
    vtkSmartPointer<vtkInEdgeIterator>::New();
  vtkSmartPointer<vtkOutEdgeIterator> outIter =
    vtkSmartPointer<vtkOutEdgeIterator>::New();
  for (vtkIdType v = 0; v < g->GetNumberOfVertices(); ++v)
  {
    g->GetInEdges(v, inIter);
    while (inIter->HasNext())
    {
      vtkIdType id = inIter->Next().Id;
      if (in[id])
      {
        return false;
      }
      in[id] = true;
    }
    g->GetOutEdges(v, outIter);
    while (outIter->HasNext())
    {
      vtkIdType id = outIter->Next().Id;
      if (out[id])
      {
        return false;
      }
      out[id] = true;
    }
  }
  for (vtkIdType i = 0; i < g->GetNumberOfEdges(); ++i)
  {
    if (in[i] == false || out[i] == false)
    {
      return false;
    }
  }

  return true;
}

//----------------------------------------------------------------------------
void vtkDirectedGraph::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
