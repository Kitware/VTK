/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOTUtilities.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOTUtilities.h"

#include "vtkDataArrayCollection.h"
#include "vtkDoubleArray.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"

#include "openturns/NumericalPoint.hxx"
#include "openturns/NumericalSample.hxx"

using namespace OT;

//-----------------------------------------------------------------------------
NumericalSample* vtkOTUtilities::SingleDimArraysToNumericalSample(vtkDataArrayCollection* arrays)
{
  if (arrays == NULL)
  {
    return NULL;
  }

  int numComp = arrays->GetNumberOfItems();
  if (numComp == 0)
  {
    vtkWarningWithObjectMacro(arrays, "Collection is empty");
    return NULL;
  }
  int numTuples = arrays->GetItem(0)->GetNumberOfTuples();
  NumericalSample* ns = new NumericalSample(numTuples, numComp);

  int j = 0;
  arrays->InitTraversal();
  vtkDataArray* array = arrays->GetNextItem();
  while (array != NULL)
  {
    if (numTuples != array->GetNumberOfTuples())
    {
      // TODO NULL Object
      vtkErrorWithObjectMacro(arrays,
        "An array has not the expected number of tuples. Expecting: " << numTuples << " , got: "
                                                                      << array->GetNumberOfTuples()
                                                                      << " , dropping it");
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

//-----------------------------------------------------------------------------
NumericalSample* vtkOTUtilities::ArrayToNumericalSample(vtkDataArray* arr)
{
  if (arr == NULL)
  {
    return NULL;
  }

  int numTuples = arr->GetNumberOfTuples();
  int numComp = arr->GetNumberOfComponents();
  NumericalSample* ns = new NumericalSample(numTuples, numComp);

  for (int i = 0; i < numTuples; ++i)
  {
    for (int j = 0; j < ns->getDimension(); j++)
    {
      ns->at(i, j) = arr->GetComponent(i, j);
    }
  }
  return ns;
}

//-----------------------------------------------------------------------------
vtkDataArray* vtkOTUtilities::NumericalSampleToArray(NumericalSample* ns)
{
  if (ns == NULL)
  {
    return NULL;
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
