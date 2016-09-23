/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSortFieldData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSortFieldData.h"

#include "vtkAbstractArray.h"
#include "vtkFieldData.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

//-------------------------------------------------------------------------
vtkStandardNewMacro(vtkSortFieldData);

//-------------------------------------------------------------------------
vtkSortFieldData::vtkSortFieldData()
{
}

//---------------------------------------------------------------------------
vtkSortFieldData::~vtkSortFieldData()
{
}

//-------------------------------------------------------------------------
// Using vtkSortDataArray, it's easy to loop over all of the arrays in the
// field data and sort them. Initially we just need to generate the sort
// indices which are then applied to each array in turn.
vtkIdType* vtkSortFieldData::
Sort( vtkFieldData *fd, const char *arrayName, int k, int retIndices, int dir)
{
  // Verify the input
  if ( fd == NULL || arrayName == NULL )
  {
    vtkGenericWarningMacro("SortFieldData needs valid input");
    return NULL;
  }
  int pos;
  vtkAbstractArray *array = fd->GetAbstractArray(arrayName, pos);
  if ( pos < 0 )
  {
    vtkGenericWarningMacro("Sorting array not found.");
    return NULL;
  }
  int numComp = array->GetNumberOfComponents();
  if ( k < 0 || k >= numComp )
  {
    vtkGenericWarningMacro( "Cannot sort by column " << k <<
      " since the array only has columns 0 through " << (numComp-1) );
    return NULL;
  }
  vtkIdType numKeys = array->GetNumberOfTuples();
  if ( numKeys <= 0 )
  {
    return NULL;
  }

  // Create and initialize the sorting indices
  vtkIdType *idx = vtkSortDataArray::InitializeSortIndices(numKeys);

  // Sort and generate the sorting indices
  void *dataIn = array->GetVoidPointer(0);
  int dataType = array->GetDataType();
  vtkSortDataArray::GenerateSortIndices(dataType, dataIn, numKeys,
                                        numComp, k, idx);

  // Now loop over all arrays in the field data. Those that are the
  // same length as the sorting indices are processed. Otherwise they
  // are skipped and remain unchanged.
  int nc, numArrays = fd->GetNumberOfArrays();
  for (int arrayNum=0; arrayNum < numArrays; ++arrayNum)
  {
    array = fd->GetAbstractArray(arrayNum);
    if ( array != NULL && array->GetNumberOfTuples() == numKeys )
    {//process the array
      dataIn = array->GetVoidPointer(0);
      dataType = array->GetDataType();
      nc = array->GetNumberOfComponents();
      vtkSortDataArray::ShuffleArray(idx, dataType, numKeys, nc,
                                     array, dataIn, dir);
    }
  }

  // Clean up
  if ( retIndices )
  {
    return idx;
  }
  else
  {
    delete [] idx;
    return NULL;
  }
}

//-------------------------------------------------------------------------
void vtkSortFieldData::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


// vtkSortFieldData methods -------------------------------------------------------
