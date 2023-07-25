// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkGraphInternals.h"

#include "vtkDistributedGraphHelper.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkGraphInternals);

//------------------------------------------------------------------------------
vtkGraphInternals::vtkGraphInternals()
{
  this->NumberOfEdges = 0;
  this->LastRemoteEdgeId = -1;
  this->UsingPedigreeIds = false;
}

//------------------------------------------------------------------------------
vtkGraphInternals::~vtkGraphInternals() = default;

//------------------------------------------------------------------------------
void vtkGraphInternals::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "NumberOfEdges: " << this->NumberOfEdges << endl;
  os << indent << "LastRemoteEdgeId: " << this->LastRemoteEdgeId << endl;
  os << indent << "LastRemoteEdgeSource: " << this->LastRemoteEdgeSource << endl;
  os << indent << "LastRemoteEdgeTarget: " << this->LastRemoteEdgeTarget << endl;
  os << indent << "UsingPedigreeIds: " << this->UsingPedigreeIds << endl;
}

//------------------------------------------------------------------------------
void vtkGraphInternals::RemoveEdgeFromOutList(vtkIdType e, std::vector<vtkOutEdgeType>& outEdges)
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
  outEdges[i] = outEdges[outSize - 1];
  outEdges.pop_back();
}

//------------------------------------------------------------------------------
void vtkGraphInternals::RemoveEdgeFromInList(vtkIdType e, std::vector<vtkInEdgeType>& inEdges)
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
  inEdges[i] = inEdges[inSize - 1];
  inEdges.pop_back();
}

//------------------------------------------------------------------------------
void vtkGraphInternals::ReplaceEdgeFromOutList(
  vtkIdType from, vtkIdType to, std::vector<vtkOutEdgeType>& outEdges)
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

//------------------------------------------------------------------------------
void vtkGraphInternals::ReplaceEdgeFromInList(
  vtkIdType from, vtkIdType to, std::vector<vtkInEdgeType>& inEdges)
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
VTK_ABI_NAMESPACE_END
