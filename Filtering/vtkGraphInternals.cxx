/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGraphInternals.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
#include "vtkGraphInternals.h"

#include "vtkDistributedGraphHelper.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkGraphInternals);

//----------------------------------------------------------------------------
vtkGraphInternals::vtkGraphInternals()
{ 
  this->NumberOfEdges = 0; 
  this->LastRemoteEdgeId = -1;
  this->UsingPedigreeIds = false;
}

//----------------------------------------------------------------------------
vtkGraphInternals::~vtkGraphInternals()
{
}

//----------------------------------------------------------------------------
void vtkGraphInternals::RemoveEdgeFromOutList(vtkIdType e, vtksys_stl::vector<vtkOutEdgeType>& outEdges)
{
  size_t outSize = outEdges.size();
  size_t i = 0;
  for (; i < outSize; ++i)
    {
    if (outEdges[i].Id == e)
      {
      break;
      }
    }
  if (i == outSize)
    {
    vtkErrorMacro("Could not find edge in source edge list.");
    return;
    }
  outEdges[i] = outEdges[outSize-1];
  outEdges.pop_back();
}

//----------------------------------------------------------------------------
void vtkGraphInternals::RemoveEdgeFromInList(vtkIdType e, vtksys_stl::vector<vtkInEdgeType>& inEdges)
{
  size_t inSize = inEdges.size();
  size_t i = 0;
  for (; i < inSize; ++i)
    {
    if (inEdges[i].Id == e)
      {
      break;
      }
    }
  if (i == inSize)
    {
    vtkErrorMacro("Could not find edge in source edge list.");
    return;
    }
  inEdges[i] = inEdges[inSize-1];
  inEdges.pop_back();
}

//----------------------------------------------------------------------------
void vtkGraphInternals::ReplaceEdgeFromOutList(vtkIdType from, vtkIdType to, vtksys_stl::vector<vtkOutEdgeType>& outEdges)
{
  size_t outSize = outEdges.size();
  for (size_t i = 0; i < outSize; ++i)
    {
    if (outEdges[i].Id == from)
      {
      outEdges[i].Id = to;
      }
    }
}

//----------------------------------------------------------------------------
void vtkGraphInternals::ReplaceEdgeFromInList(vtkIdType from, vtkIdType to, vtksys_stl::vector<vtkInEdgeType>& inEdges)
{
  size_t inSize = inEdges.size();
  for (size_t i = 0; i < inSize; ++i)
    {
    if (inEdges[i].Id == from)
      {
      inEdges[i].Id = to;
      }
    }
}
