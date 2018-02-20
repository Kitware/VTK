/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkArrayListTemplate.txx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE file for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkArrayListTemplate.h"
#include "vtkFloatArray.h"

#include <cassert>

#ifndef vtkArrayListTemplate_txx
#define vtkArrayListTemplate_txx

//----------------------------------------------------------------------------
// Sort of a little object factory (in conjunction w/ vtkTemplateMacro())
template <typename T>
void CreateArrayPair(ArrayList *list, T *inData, T *outData,
                     vtkIdType numTuples, int numComp, vtkDataArray *outArray,
                     T nullValue)
{
  ArrayPair<T> *pair = new ArrayPair<T>(inData,outData,numTuples,numComp,
                                        outArray,nullValue);
  list->Arrays.push_back(pair);
}

//----------------------------------------------------------------------------
// Sort of a little object factory (in conjunction w/ vtkTemplateMacro())
template <typename T>
void CreateRealArrayPair(ArrayList *list, T *inData, float *outData,
                         vtkIdType numTuples, int numComp, vtkDataArray *outArray,
                         float nullValue)
{
  RealArrayPair<T,float> *pair =
    new RealArrayPair<T,float>(inData,outData,numTuples,numComp,
                               outArray,nullValue);
  list->Arrays.push_back(pair);
}

//----------------------------------------------------------------------------
// Indicate arrays not to process
inline void ArrayList::
ExcludeArray(vtkDataArray *da)
{
  ExcludedArrays.push_back(da);
}

//----------------------------------------------------------------------------
// Has the specified array been excluded?
inline vtkTypeBool ArrayList::
IsExcluded(vtkDataArray *da)
{
  return (std::find(ExcludedArrays.begin(), ExcludedArrays.end(), da) != ExcludedArrays.end());
}

//----------------------------------------------------------------------------
// Add an array pair (input,output) using the name provided for the output. The
// numTuples is the number of output tuples allocated.
inline vtkDataArray* ArrayList::
AddArrayPair(vtkIdType numTuples, vtkDataArray *inArray,
             vtkStdString &outArrayName, double nullValue, vtkTypeBool promote)
{
  if (this->IsExcluded(inArray))
  {
    return nullptr;
  }

  int iType = inArray->GetDataType();
  vtkDataArray *outArray;
  if ( promote && iType != VTK_FLOAT && iType != VTK_DOUBLE )
  {
    outArray = vtkFloatArray::New();
    outArray->SetNumberOfComponents(inArray->GetNumberOfComponents());
    outArray->SetNumberOfTuples(numTuples);
    outArray->SetName(outArrayName);
    void *iD = inArray->GetVoidPointer(0);
    void *oD = outArray->GetVoidPointer(0);
    switch (iType)
    {
      vtkTemplateMacro(CreateRealArrayPair(this, static_cast<VTK_TT *>(iD),
                       static_cast<float*>(oD),numTuples,inArray->GetNumberOfComponents(),
                       outArray,static_cast<float>(nullValue)));
    }//over all VTK types
  }
  else
  {
    outArray = inArray->NewInstance();
    outArray->SetNumberOfComponents(inArray->GetNumberOfComponents());
    outArray->SetNumberOfTuples(numTuples);
    outArray->SetName(outArrayName);
    void *iD = inArray->GetVoidPointer(0);
    void *oD = outArray->GetVoidPointer(0);
    switch (iType)
    {
      vtkTemplateMacro(CreateArrayPair(this, static_cast<VTK_TT *>(iD),
                       static_cast<VTK_TT *>(oD),numTuples,inArray->GetNumberOfComponents(),
                       outArray,static_cast<VTK_TT>(nullValue)));
    }//over all VTK types
  }//promote integral types

  assert(outArray->GetReferenceCount() > 1);
  outArray->FastDelete();
  return outArray;
}

//----------------------------------------------------------------------------
// Add the arrays to interpolate here. This presumes that vtkDataSetAttributes::CopyData() or
// vtkDataSetAttributes::InterpolateData() has been called, and the input and output array
// names match.
inline void ArrayList::
AddArrays(vtkIdType numOutPts, vtkDataSetAttributes *inPD, vtkDataSetAttributes *outPD,
          double nullValue, vtkTypeBool promote)
{
  // Build the vector of interpolation pairs. Note that InterpolateAllocate should have
  // been called at this point (output arrays created and allocated).
  char *name;
  vtkDataArray *iArray, *oArray;
  int iType, oType;
  void *iD, *oD;
  int iNumComp, oNumComp;
  int i, numArrays = outPD->GetNumberOfArrays();

  for (i=0; i < numArrays; ++i)
  {
    oArray = outPD->GetArray(i);
    if ( oArray && ! this->IsExcluded(oArray) )
    {
      name = oArray->GetName();
      iArray = inPD->GetArray(name);
      if ( iArray && ! this->IsExcluded(iArray) )
      {
        iType = iArray->GetDataType();
        oType = oArray->GetDataType();
        iNumComp = iArray->GetNumberOfComponents();
        oNumComp = oArray->GetNumberOfComponents();
        if ( promote && oType != VTK_FLOAT && oType != VTK_DOUBLE )
        {
          oType = VTK_FLOAT;
          vtkFloatArray *fArray = vtkFloatArray::New();
          fArray->SetName(oArray->GetName());
          fArray->SetNumberOfComponents(oNumComp);
          outPD->AddArray(fArray); //nasty side effect will replace current array in the same spot
          oArray = fArray;
          fArray->Delete();
        }
        oArray->SetNumberOfTuples(numOutPts);

        assert( iNumComp == oNumComp );
        if ( iType == oType )
        {
          iD = iArray->GetVoidPointer(0);
          oD = oArray->GetVoidPointer(0);
          switch (iType)
          {
            vtkTemplateMacro(CreateArrayPair(this, static_cast<VTK_TT *>(iD),
                             static_cast<VTK_TT *>(oD),numOutPts,oNumComp,
                             oArray,static_cast<VTK_TT>(nullValue)));
          }//over all VTK types
        }//if matching types
        else //promoted type
        {
          iD = iArray->GetVoidPointer(0);
          oD = oArray->GetVoidPointer(0);
          switch (iType)
          {
            vtkTemplateMacro(CreateRealArrayPair(this, static_cast<VTK_TT *>(iD),
                             static_cast<float*>(oD),numOutPts,iNumComp,
                             oArray,static_cast<float>(nullValue)));
          }//over all VTK types
        }//if promoted pair
      }//if matching input array
    }//if output array
  }//for each candidate array
}

//----------------------------------------------------------------------------
// Add the arrays to interpolate here. This presumes that vtkDataSetAttributes::CopyData() or
// vtkDataSetAttributes::InterpolateData() has been called. This special version creates an
// array pair that interpolates from itself.
inline void ArrayList::
AddSelfInterpolatingArrays(vtkIdType numOutPts, vtkDataSetAttributes *attr, double nullValue)
{
  // Build the vector of interpolation pairs. Note that CopyAllocate/InterpolateAllocate should have
  // been called at this point (output arrays created and allocated).
  vtkDataArray *iArray;
  int iType, iNumComp;
  void *iD;
  int i, numArrays = attr->GetNumberOfArrays();

  for (i=0; i < numArrays; ++i)
  {
    iArray = attr->GetArray(i);
    if ( iArray && ! this->IsExcluded(iArray) )
    {
      iType = iArray->GetDataType();
      iNumComp = iArray->GetNumberOfComponents();
      iArray->WriteVoidPointer(0,numOutPts*iNumComp); //allocates memory, preserves data
      iD = iArray->GetVoidPointer(0);
      switch (iType)
      {
        vtkTemplateMacro(CreateArrayPair(this, static_cast<VTK_TT *>(iD),
                         static_cast<VTK_TT *>(iD),numOutPts,iNumComp,
                         iArray,static_cast<VTK_TT>(nullValue)));
      }//over all VTK types
    }//if not excluded
  }//for each candidate array
}

#endif
