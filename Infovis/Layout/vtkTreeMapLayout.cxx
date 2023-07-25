// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "vtkTreeMapLayout.h"

#include "vtkAdjacentVertexIterator.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkTree.h"
#include "vtkTreeMapLayoutStrategy.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkTreeMapLayout);

vtkTreeMapLayout::vtkTreeMapLayout()
{
  this->RectanglesFieldName = nullptr;
  this->LayoutStrategy = nullptr;
  this->SetRectanglesFieldName("area");
  this->SetSizeArrayName("size");
}

vtkTreeMapLayout::~vtkTreeMapLayout()
{
  this->SetRectanglesFieldName(nullptr);
  if (this->LayoutStrategy)
  {
    this->LayoutStrategy->Delete();
  }
}

vtkCxxSetObjectMacro(vtkTreeMapLayout, LayoutStrategy, vtkTreeMapLayoutStrategy);

int vtkTreeMapLayout::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (this->LayoutStrategy == nullptr)
  {
    vtkErrorMacro(<< "Layout strategy must be non-null.");
    return 0;
  }
  if (this->RectanglesFieldName == nullptr)
  {
    vtkErrorMacro(<< "Rectangles field name must be non-null.");
    return 0;
  }
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // Storing the inputTree and outputTree handles
  vtkTree* inputTree = vtkTree::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkTree* outputTree = vtkTree::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Copy the input into the output
  outputTree->ShallowCopy(inputTree);

  // Add the 4-tuple array that will store the min,max xy coords
  vtkFloatArray* coordsArray = vtkFloatArray::New();
  coordsArray->SetName(this->RectanglesFieldName);
  coordsArray->SetNumberOfComponents(4);
  coordsArray->SetNumberOfTuples(inputTree->GetNumberOfVertices());
  vtkDataSetAttributes* data = outputTree->GetVertexData();
  data->AddArray(coordsArray);
  coordsArray->Delete();

  // Add the 4-tuple array that will store the min,max xy coords
  vtkDataArray* sizeArray = this->GetInputArrayToProcess(0, inputTree);
  if (!sizeArray)
  {
    vtkErrorMacro("Size array not found.");
    return 0;
  }

  // Okay now layout the tree :)
  this->LayoutStrategy->Layout(inputTree, coordsArray, sizeArray);

  return 1;
}

void vtkTreeMapLayout::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "RectanglesFieldName: "
     << (this->RectanglesFieldName ? this->RectanglesFieldName : "(none)") << endl;
  os << indent << "LayoutStrategy: " << (this->LayoutStrategy ? "" : "(none)") << endl;
  if (this->LayoutStrategy)
  {
    this->LayoutStrategy->PrintSelf(os, indent.GetNextIndent());
  }
}

vtkIdType vtkTreeMapLayout::FindVertex(float pnt[2], float* binfo)
{
  // Do we have an output?
  vtkTree* otree = this->GetOutput();
  if (!otree)
  {
    vtkErrorMacro(<< "Could not get output tree.");
    return -1;
  }

  // Get the four tuple array for the points
  vtkDataArray* array = otree->GetVertexData()->GetArray(this->RectanglesFieldName);
  if (!array)
  {
    // vtkErrorMacro(<< "Output Tree does not have box information.");
    return -1;
  }

  // Check to see that we are in the dataset at all
  float blimits[4];

  vtkIdType vertex = otree->GetRoot();
  vtkFloatArray* boxInfo = vtkArrayDownCast<vtkFloatArray>(array);
  // Now try to find the vertex that contains the point
  boxInfo->GetTypedTuple(vertex, blimits); // Get the extents of the root
  if ((pnt[0] < blimits[0]) || (pnt[0] > blimits[1]) || (pnt[1] < blimits[2]) ||
    (pnt[1] > blimits[3]))
  {
    // Point is not in the tree at all
    return -1;
  }

  // Now traverse the children to try and find
  // the vertex that contains the point
  vtkIdType child;
  if (binfo)
  {
    binfo[0] = blimits[0];
    binfo[1] = blimits[1];
    binfo[2] = blimits[2];
    binfo[3] = blimits[3];
  }

  vtkAdjacentVertexIterator* it = vtkAdjacentVertexIterator::New();
  otree->GetAdjacentVertices(vertex, it);
  while (it->HasNext())
  {
    child = it->Next();
    boxInfo->GetTypedTuple(child, blimits); // Get the extents of the child
    if ((pnt[0] < blimits[0]) || (pnt[0] > blimits[1]) || (pnt[1] < blimits[2]) ||
      (pnt[1] > blimits[3]))
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

void vtkTreeMapLayout::GetBoundingBox(vtkIdType id, float* binfo)
{
  // Do we have an output?
  vtkTree* otree = this->GetOutput();
  if (!otree)
  {
    vtkErrorMacro(<< "Could not get output tree.");
    return;
  }

  // Get the four tuple array for the points
  vtkDataArray* array = otree->GetVertexData()->GetArray(this->RectanglesFieldName);
  if (!array)
  {
    // vtkErrorMacro(<< "Output Tree does not have box information.");
    return;
  }

  vtkFloatArray* boxInfo = vtkArrayDownCast<vtkFloatArray>(array);
  boxInfo->GetTypedTuple(id, binfo);
}

vtkMTimeType vtkTreeMapLayout::GetMTime()
{
  vtkMTimeType mTime = this->Superclass::GetMTime();
  vtkMTimeType time;

  if (this->LayoutStrategy != nullptr)
  {
    time = this->LayoutStrategy->GetMTime();
    mTime = (time > mTime ? time : mTime);
  }
  return mTime;
}
VTK_ABI_NAMESPACE_END
