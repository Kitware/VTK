/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSortDataArray.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*
 * Copyright 2003 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "vtkSortDataArray.h"

#include "vtkAbstractArray.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkIdList.h"
#include "vtkStdString.h"
#include "vtkStringArray.h"
#include "vtkVariant.h"
#include "vtkVariantArray.h"
#include "vtkSMPTools.h"
#include <functional>  //std::greater

//-------------------------------------------------------------------------

vtkStandardNewMacro(vtkSortDataArray);

//-------------------------------------------------------------------------
vtkSortDataArray::vtkSortDataArray()
{
}

//---------------------------------------------------------------------------
vtkSortDataArray::~vtkSortDataArray()
{
}

//---------------------------------------------------------------------------
void vtkSortDataArray::Sort(vtkIdList *keys, int dir)
{
  if ( keys == NULL )
  {
    return;
  }
  vtkIdType *data = keys->GetPointer(0);
  vtkIdType numKeys = keys->GetNumberOfIds();
  if ( dir == 0 )
  {
    vtkSMPTools::Sort(data, data + numKeys);
  }
  else
  {
    vtkSMPTools::Sort(data, data + numKeys, std::greater<vtkIdType>());
  }
}


//---------------------------------------------------------------------------
void vtkSortDataArray::Sort(vtkAbstractArray *keys, int dir)
{
  if ( keys == NULL )
  {
    return;
  }

  if (keys->GetNumberOfComponents() != 1)
  {
    vtkGenericWarningMacro("Can only sort keys that are 1-tuples.");
    return;
  }

  void *data = keys->GetVoidPointer(0);
  vtkIdType numKeys = keys->GetNumberOfTuples();

  if ( dir == 0 )
  {
    switch (keys->GetDataType())
    {
      vtkExtendedTemplateMacro(vtkSMPTools::Sort(static_cast<VTK_TT *>(data),
                                                 static_cast<VTK_TT *>(data) + numKeys));
    }
  }
  else
  {
    switch (keys->GetDataType())
    {
      vtkExtendedTemplateMacro(vtkSMPTools::Sort(static_cast<VTK_TT *>(data),
                                                 static_cast<VTK_TT *>(data) + numKeys,
                                                 std::greater<VTK_TT>()));
    }
  }
}

//---------------------------------------------------------------------------
// Hide some stuff; mostly things plugged into templated functions
namespace {

//---------------------------------------------------------------------------
// We sort the indices based on a key value in another array. Produces sort
// in ascending direction. Note that sort comparison operator is for single
// component arrays.
template <typename T>
struct KeyComp
{
  const T *Array;
  KeyComp(T *array) : Array(array) {};
  bool operator() (vtkIdType idx0, vtkIdType idx1) const
  {
    return ( Array[idx0] < Array[idx1] );
  }
};

//-----------------------------------------------------------------------------
// Special comparison functor using tuple component as a key. Note that this
// comparison function is for general arrays of n components.
template <typename T>
struct TupleComp
{
  const T *Array;
  int NumComp;
  int K;
  TupleComp(T *array, int n, int k) : Array(array), NumComp(n), K(k) {};
  bool operator() (vtkIdType idx0, vtkIdType idx1) const
    {return Array[idx0*NumComp+K] < Array[idx1*NumComp+K];}
};

//---------------------------------------------------------------------------
// Given a set of indices (after sorting), copy the data from a pre-sorted
// array to a final, post-sorted array, Implementation note: the direction of
// sort (dir) is treated here rather than in the std::sort() function to
// reduce object file .obj size; e.g., running std::sort with a different
// comporator function causes inline expansion to produce very large object
// files.
template <typename T>
void Shuffle1Tuples(vtkIdType *idx, vtkIdType sze, vtkAbstractArray *arrayIn,
                    T *preSort, int dir)
{
  T *postSort = new T [sze];

  if ( dir == 0 ) //ascending
  {
    for (vtkIdType i=0; i<sze; ++i)
    {
      postSort[i] = preSort[idx[i]];
    }
  }
  else
  {
    vtkIdType end=sze-1;
    for (vtkIdType i=0; i<sze; ++i)
    {
      postSort[i] = preSort[idx[end-i]];
    }
  }

  arrayIn->SetVoidArray(postSort, sze, 0,
                        vtkAbstractArray::VTK_DATA_ARRAY_DELETE);
}

//---------------------------------------------------------------------------
// Given a set of indices (after sorting), copy the data from a pre-sorted
// data array to a final, post-sorted array. Note that the data array is
// assumed to have arbitrary sized components.
template <typename T>
void ShuffleTuples(vtkIdType *idx, vtkIdType sze, int numComp,
                       vtkAbstractArray *arrayIn, T *preSort, int dir)
{
  T *postSort = new T [sze*numComp];

  int k;
  vtkIdType i;
  if ( dir == 0 ) //ascending
  {
    for (i=0; i<sze; ++i)
    {
      for (k=0; k<numComp; ++k)
      {
        postSort[i*numComp+k] = preSort[idx[i]*numComp+k];
      }
    }
  }
  else
  {
    vtkIdType end=sze-1;
    for (i=0; i<sze; ++i)
    {
      for (k=0; k<numComp; ++k)
      {
        postSort[i*numComp+k] = preSort[idx[end-i]*numComp+k];
      }
    }
  }

  arrayIn->SetVoidArray(postSort, sze*numComp, 0,
                        vtkAbstractArray::VTK_DATA_ARRAY_DELETE);
}

}//anonymous namespace

//---------------------------------------------------------------------------
// Allocate and initialize sort indices
vtkIdType* vtkSortDataArray::InitializeSortIndices(vtkIdType num)
{
  vtkIdType *idx = new vtkIdType[num];
  for (vtkIdType i=0; i < num; ++i)
  {
    idx[i] = i;
  }
  return idx;
}


//---------------------------------------------------------------------------
// Efficent function for generating sort ordering specialized to single
// component arrays.
void vtkSortDataArray::
GenerateSort1Indices(int dataType, void *dataIn, vtkIdType numKeys,
                     vtkIdType *idx)
{
  if ( dataType == VTK_VARIANT)
  {
    vtkSMPTools::Sort(idx, idx+numKeys,
                      KeyComp<vtkVariant>(static_cast<vtkVariant*>(dataIn)));
  }
  else
  {
    switch ( dataType )
    {
      vtkExtendedTemplateMacro(vtkSMPTools::Sort(idx, idx+numKeys,
                               KeyComp<VTK_TT>(static_cast<VTK_TT *>(dataIn))));
    }
  }
}

//---------------------------------------------------------------------------
// Function for generating sort ordering for general arrays.
void vtkSortDataArray::
GenerateSortIndices(int dataType, void *dataIn, vtkIdType numKeys,
                    int numComp, int k, vtkIdType *idx)
{
  // Specialized and faster for single component arrays
  if ( numComp == 1)
  {
    return vtkSortDataArray::GenerateSort1Indices(dataType, dataIn, numKeys, idx);
  }

  if ( dataType == VTK_VARIANT)
  {
    vtkSMPTools::Sort(idx, idx+numKeys,
                 TupleComp<vtkVariant>(static_cast<vtkVariant*>(dataIn),numComp,k));
  }
  else
  {
    switch (dataType)
    {
      vtkExtendedTemplateMacro(vtkSMPTools::Sort(idx, idx+numKeys,
                               TupleComp<VTK_TT>(static_cast<VTK_TT *>(dataIn),numComp,k)));
    }
  }
}

//-------------------------------------------------------------------------
// Set up the actual templated shuffling operation. This method is for
// VTK arrays that are precsisely one component.
void vtkSortDataArray::
Shuffle1Array(vtkIdType *idx, int dataType, vtkIdType numKeys,
              vtkAbstractArray *arr, void *dataIn, int dir)
{
  if ( dataType == VTK_VARIANT)
  {
    Shuffle1Tuples(idx, numKeys, arr, static_cast<vtkVariant*>(dataIn), dir);
  }
  else
  {
    switch (arr->GetDataType())
    {
      vtkExtendedTemplateMacro(Shuffle1Tuples(idx, numKeys, arr,
                               static_cast<VTK_TT *>(dataIn), dir));
    }
  }
}

//-------------------------------------------------------------------------
// Set up the actual templated shuffling operation
void vtkSortDataArray::
ShuffleArray(vtkIdType *idx, int dataType, vtkIdType numKeys, int numComp,
             vtkAbstractArray *arr, void *dataIn, int dir)
{
  // Specialized for single component arrays
  if ( numComp == 1)
  {
    return vtkSortDataArray::Shuffle1Array(idx, dataType, numKeys, arr,
                                           dataIn, dir);
  }

  if ( dataType == VTK_VARIANT)
  {
    ShuffleTuples(idx, numKeys, numComp, arr,
                      static_cast<vtkVariant*>(dataIn), dir);
  }
  else
  {
    switch (arr->GetDataType())
    {
      vtkExtendedTemplateMacro(ShuffleTuples(idx, numKeys, numComp, arr,
                               static_cast<VTK_TT *>(dataIn), dir));
    }
  }
}

//---------------------------------------------------------------------------
// Given a set of indices (after sorting), copy the ids from a pre-sorted
// id array to a final, post-sorted array.
void vtkSortDataArray::
ShuffleIdList(vtkIdType *idx, vtkIdType sze, vtkIdList *arrayIn,
              vtkIdType *preSort, int dir)
{
  vtkIdType *postSort = new vtkIdType [sze];

  if ( dir == 0 ) //ascending
  {
    for (vtkIdType i=0; i<sze; ++i)
    {
      postSort[i] = preSort[idx[i]];
    }
  }
  else
  {
    vtkIdType end=sze-1;
    for (vtkIdType i=0; i<sze; ++i)
    {
      postSort[i] = preSort[idx[end-i]];
    }
  }

  arrayIn->SetArray(postSort, sze);
}

//---------------------------------------------------------------------------
// Sort a position index based on the values in the abstract array. Once
// sorted, then shuffle the keys and values around into new arrays.
void vtkSortDataArray::
Sort(vtkAbstractArray *keys, vtkAbstractArray *values, int dir)
{
  // Check input
  if ( keys == NULL || values == NULL )
  {
    return;
  }
  if (keys->GetNumberOfComponents() != 1)
  {
    vtkGenericWarningMacro("Can only sort keys that are 1-tuples.");
    return;
  }
  vtkIdType numKeys = keys->GetNumberOfTuples();
  vtkIdType numValues = values->GetNumberOfTuples();
  if ( numKeys != numValues )
  {
    vtkGenericWarningMacro("Could not sort arrays.  Key and value arrays have different sizes.");
    return;
  }

  // Sort the index array
  vtkIdType *idx = vtkSortDataArray::InitializeSortIndices(numKeys);

  // Generate the sorting index array
  void *dataIn = keys->GetVoidPointer(0);
  int numComp = 1;
  int dataType = keys->GetDataType();
  vtkSortDataArray::GenerateSortIndices(dataType, dataIn, numKeys,
                                        numComp, 0, idx);

  // Now shuffle data around based on sorted indices
  vtkSortDataArray::ShuffleArray(idx, dataType, numKeys, numComp,
                                 keys, dataIn, dir);

  dataIn = values->GetVoidPointer(0);
  numComp = values->GetNumberOfComponents();
  dataType = values->GetDataType();
  vtkSortDataArray::ShuffleArray(idx, dataType, numKeys, numComp,
                                 values, dataIn, dir);

  // Clean up
  delete [] idx;
}


//---------------------------------------------------------------------------
void vtkSortDataArray::
Sort(vtkAbstractArray *keys, vtkIdList *values, int  dir)
{
  // Check input
  if ( keys == NULL || values == NULL )
  {
    return;
  }
  if (keys->GetNumberOfComponents() != 1)
  {
    vtkGenericWarningMacro("Can only sort keys that are 1-tuples.");
    return;
  }
  vtkIdType numKeys = keys->GetNumberOfTuples();
  vtkIdType numIds = values->GetNumberOfIds();
  if ( numKeys != numIds )
  {
    vtkGenericWarningMacro("Could not sort arrays.  Key and id arrays have different sizes.");
    return;
  }

  // Sort the index array
  vtkIdType *idx = vtkSortDataArray::InitializeSortIndices(numKeys);

  // Generate the sorting index array
  void *dataIn = keys->GetVoidPointer(0);
  int numComp = 1;
  int dataType = keys->GetDataType();
  vtkSortDataArray::GenerateSortIndices(dataType, dataIn, numKeys,
                                        numComp, 0, idx);

  // Shuffle the keys
  vtkSortDataArray::ShuffleArray(idx, dataType, numKeys, numComp,
                                 keys, dataIn, dir);

  // Now shuffle the ids to match the sort
  vtkIdType *ids = values->GetPointer(0);
  ShuffleIdList(idx, numKeys, values, ids, dir);

  // Clean up
  delete [] idx;
}

//---------------------------------------------------------------------------
void vtkSortDataArray::
SortArrayByComponent( vtkAbstractArray* arr, int k, int dir)
{
  // Check input
  if ( arr == NULL )
  {
    return;
  }
  vtkIdType numKeys = arr->GetNumberOfTuples();
  int nc = arr->GetNumberOfComponents();

  if ( k < 0 || k >= nc )
  {
    vtkGenericWarningMacro( "Cannot sort by column " << k <<
      " since the array only has columns 0 through " << (nc-1) );
    return;
  }

  // Perform the sort
  vtkIdType *idx = vtkSortDataArray::InitializeSortIndices(numKeys);

  void *dataIn = arr->GetVoidPointer(0);
  int dataType = arr->GetDataType();
  vtkSortDataArray::GenerateSortIndices(dataType, dataIn, numKeys, nc, k, idx);

  vtkSortDataArray::ShuffleArray(idx, dataType, numKeys, nc,
                                 arr, dataIn, dir);

  // Clean up
  delete [] idx;
}


//-------------------------------------------------------------------------
void vtkSortDataArray::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


// vtkSortDataArray methods -------------------------------------------------------
