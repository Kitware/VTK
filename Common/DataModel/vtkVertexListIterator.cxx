/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVertexListIterator.cxx

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

#include "vtkVertexListIterator.h"

#include "vtkDataObject.h"
#include "vtkDistributedGraphHelper.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkGraph.h"

vtkStandardNewMacro(vtkVertexListIterator);
//----------------------------------------------------------------------------
vtkVertexListIterator::vtkVertexListIterator()
{
  this->Current = 0;
  this->End = 0;
  this->Graph = 0;
}

//----------------------------------------------------------------------------
vtkVertexListIterator::~vtkVertexListIterator()
{
  if (this->Graph)
  {
    this->Graph->Delete();
  }
}

//----------------------------------------------------------------------------
void vtkVertexListIterator::SetGraph(vtkGraph *graph)
{
  vtkSetObjectBodyMacro(Graph, vtkGraph, graph);
  if (this->Graph)
  {
    this->Current = 0;
    this->End = this->Graph->GetNumberOfVertices();

    // For a distributed graph, shift the iteration space to cover
    // local vertices
    vtkDistributedGraphHelper *helper
      = this->Graph->GetDistributedGraphHelper();
    if (helper)
    {
      int myRank
        = this->Graph->GetInformation()->Get(vtkDataObject::DATA_PIECE_NUMBER());
      this->Current = helper->MakeDistributedId(myRank, this->Current);
      this->End = helper->MakeDistributedId(myRank, this->End);
    }
  }
}

//----------------------------------------------------------------------------
void vtkVertexListIterator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Graph: " << (this->Graph ? "" : "(null)") << endl;
  if (this->Graph)
  {
    this->Graph->PrintSelf(os, indent.GetNextIndent());
  }
}
