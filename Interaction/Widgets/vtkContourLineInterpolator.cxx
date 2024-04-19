// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkContourLineInterpolator.h"

#include "vtkContourRepresentation.h"
#include "vtkIntArray.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkContourLineInterpolator::vtkContourLineInterpolator() = default;

//------------------------------------------------------------------------------
vtkContourLineInterpolator::~vtkContourLineInterpolator() = default;

//------------------------------------------------------------------------------
int vtkContourLineInterpolator::UpdateNode(
  vtkRenderer*, vtkContourRepresentation*, double* vtkNotUsed(node), int vtkNotUsed(idx))
{
  return 0;
}

//------------------------------------------------------------------------------
void vtkContourLineInterpolator::GetSpan(
  int nodeIndex, vtkIntArray* nodeIndices, vtkContourRepresentation* rep)
{
  int start = nodeIndex - 1;
  int end = nodeIndex;
  int index[2];

  // Clear the array
  nodeIndices->Reset();
  nodeIndices->Squeeze();
  nodeIndices->SetNumberOfComponents(2);

  for (int i = 0; i < 3; i++)
  {
    index[0] = start++;
    index[1] = end++;

    if (rep->GetClosedLoop())
    {
      if (index[0] < 0)
      {
        index[0] += rep->GetNumberOfNodes();
      }
      if (index[1] < 0)
      {
        index[1] += rep->GetNumberOfNodes();
      }
      if (index[0] >= rep->GetNumberOfNodes())
      {
        index[0] -= rep->GetNumberOfNodes();
      }
      if (index[1] >= rep->GetNumberOfNodes())
      {
        index[1] -= rep->GetNumberOfNodes();
      }
    }

    if (index[0] >= 0 && index[0] < rep->GetNumberOfNodes() && index[1] >= 0 &&
      index[1] < rep->GetNumberOfNodes())
    {
      nodeIndices->InsertNextTypedTuple(index);
    }
  }
}

//------------------------------------------------------------------------------
void vtkContourLineInterpolator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
