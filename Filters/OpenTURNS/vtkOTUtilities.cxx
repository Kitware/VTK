// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkOTUtilities.h"

#include "vtkDataArrayCollection.h"
#include "vtkDoubleArray.h"
#include "vtkOTIncludes.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"

using namespace OT;

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
Sample* vtkOTUtilities::SingleDimArraysToSample(vtkDataArrayCollection* arrays)
{
  if (arrays == nullptr)
  {
    return nullptr;
  }

  int numComp = arrays->GetNumberOfItems();
  if (numComp == 0)
  {
    vtkWarningWithObjectMacro(arrays, "Collection is empty");
    return nullptr;
  }
  int numTuples = arrays->GetItem(0)->GetNumberOfTuples();
  Sample* ns = new Sample(numTuples, numComp);

  int j = 0;
  arrays->InitTraversal();
  vtkDataArray* array = arrays->GetNextItem();
  while (array != nullptr)
  {
    if (numTuples != array->GetNumberOfTuples())
    {
      // TODO nullptr Object
      vtkErrorWithObjectMacro(arrays,
        "An array has not the expected number of tuples. Expecting: "
          << numTuples << " , got: " << array->GetNumberOfTuples() << " , dropping it");
      continue;
    }
    for (int i = 0; i < numTuples; ++i)
    {
      ns->at(i, j) = array->GetComponent(i, 0);
    }
    array = arrays->GetNextItem();
    j++;
  }
  return ns;
}

//------------------------------------------------------------------------------
Sample* vtkOTUtilities::ArrayToSample(vtkDataArray* arr)
{
  if (arr == nullptr)
  {
    return nullptr;
  }

  vtkIdType numTuples = arr->GetNumberOfTuples();
  int numComp = arr->GetNumberOfComponents();
  Sample* ns = new Sample(numTuples, numComp);

  for (vtkIdType i = 0; i < numTuples; ++i)
  {
    for (unsigned int j = 0; j < ns->getDimension(); j++)
    {
      ns->at(i, j) = arr->GetComponent(i, j);
    }
  }
  return ns;
}

//------------------------------------------------------------------------------
vtkDataArray* vtkOTUtilities::SampleToArray(Sample* ns)
{
  if (ns == nullptr)
  {
    return nullptr;
  }

  int numTuples = ns->getSize();
  int numComp = ns->getDimension();

  vtkDoubleArray* arr = vtkDoubleArray::New();
  arr->SetNumberOfTuples(numTuples);
  arr->SetNumberOfComponents(numComp);

  for (int i = 0; i < numTuples; i++)
  {
    for (int j = 0; j < numComp; j++)
    {
      arr->SetComponent(i, j, ns->at(i, j));
    }
  }
  return arr;
}
VTK_ABI_NAMESPACE_END
