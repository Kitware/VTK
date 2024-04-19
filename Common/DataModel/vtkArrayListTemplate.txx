// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkArrayListTemplate.h"
#include "vtkFloatArray.h"

#include <cassert>

#ifndef vtkArrayListTemplate_txx
#define vtkArrayListTemplate_txx

//----------------------------------------------------------------------------
// Sort of a little object factory (in conjunction w/ vtkTemplateMacro())
VTK_ABI_NAMESPACE_BEGIN
template <typename T>
void CreateArrayPair(ArrayList* list, T* inData, T* outData, vtkIdType numTuples, int numComp,
  vtkAbstractArray* outArray, double nullValue)
{
  ArrayPair<T>* pair = new ArrayPair<T>(inData, outData, numTuples, numComp, outArray, nullValue);
  list->Arrays.push_back(pair);
}

//----------------------------------------------------------------------------
// Sort of a little object factory (in conjunction w/ vtkTemplateMacro())
template <typename T>
void CreateRealArrayPair(ArrayList* list, T* inData, float* outData, vtkIdType numTuples,
  int numComp, vtkAbstractArray* outArray, float nullValue)
{
  RealArrayPair<T, float>* pair =
    new RealArrayPair<T, float>(inData, outData, numTuples, numComp, outArray, nullValue);
  list->Arrays.push_back(pair);
}

//----------------------------------------------------------------------------
// Indicate arrays not to process
inline void ArrayList::ExcludeArray(vtkAbstractArray* da)
{
  ExcludedArrays.push_back(da);
}

//----------------------------------------------------------------------------
// Has the specified array been excluded?
inline vtkTypeBool ArrayList::IsExcluded(vtkAbstractArray* da)
{
  return (std::find(ExcludedArrays.begin(), ExcludedArrays.end(), da) != ExcludedArrays.end());
}

//----------------------------------------------------------------------------
// Add an array pair (input,output) using the name provided for the output. The
// numTuples is the number of output tuples allocated.
inline vtkAbstractArray* ArrayList::AddArrayPair(vtkIdType numTuples, vtkAbstractArray* inArray,
  vtkStdString& outArrayName, double nullValue, vtkTypeBool promote)
{
  if (this->IsExcluded(inArray))
  {
    return nullptr;
  }

  int iType = inArray->GetDataType();
  vtkAbstractArray* outArray;
  if (promote && iType != VTK_FLOAT && iType != VTK_DOUBLE)
  {
    outArray = vtkFloatArray::New();
    outArray->SetNumberOfComponents(inArray->GetNumberOfComponents());
    outArray->SetNumberOfTuples(numTuples);
    outArray->SetName(outArrayName.c_str());
    void* iD = inArray->GetVoidPointer(0);
    void* oD = outArray->GetVoidPointer(0);
    switch (iType)
    {
      vtkTemplateMacro(CreateRealArrayPair(this, static_cast<VTK_TT*>(iD), static_cast<float*>(oD),
        numTuples, inArray->GetNumberOfComponents(), outArray, static_cast<float>(nullValue)));
    } // over all VTK types
  }
  else
  {
    outArray = inArray->NewInstance();
    outArray->SetNumberOfComponents(inArray->GetNumberOfComponents());
    outArray->SetNumberOfTuples(numTuples);
    outArray->SetName(outArrayName.c_str());
    void* iD = inArray->GetVoidPointer(0);
    void* oD = outArray->GetVoidPointer(0);
    switch (iType)
    {
      vtkTemplateMacro(CreateArrayPair(this, static_cast<VTK_TT*>(iD), static_cast<VTK_TT*>(oD),
        numTuples, inArray->GetNumberOfComponents(), outArray, static_cast<VTK_TT>(nullValue)));
    } // over all VTK types
  }   // promote integral types

  assert(outArray->GetReferenceCount() > 1);
  outArray->FastDelete();
  return outArray;
}

//----------------------------------------------------------------------------
// Add the arrays to interpolate here. This presumes that
// vtkDataSetAttributes::CopyAllocate() or vtkDataSetAttributes::InterpolateAllocate()
// has been called prior to invoking this method.
inline void ArrayList::AddArrays(vtkIdType numOutPts, vtkDataSetAttributes* inPD,
  vtkDataSetAttributes* outPD, double nullValue, vtkTypeBool promote)
{
  // Build the vector of interpolation pairs. Note that
  // vtkDataSetAttributes::InterpolateAllocate or CopyAllocate() should have
  // been called at this point (i.e., output arrays created and allocated).
  vtkAbstractArray *iArray, *oArray;
  int iType, oType;
  void *iD, *oD;
  int iNumComp, oNumComp;

  for (auto& i : outPD->RequiredArrays)
  {
    iArray = inPD->Data[i];
    oArray = outPD->Data[outPD->TargetIndices[i]];

    if (iArray && oArray && !this->IsExcluded(oArray) && !this->IsExcluded(iArray))
    {
      iType = iArray->GetDataType();
      oType = oArray->GetDataType();
      iNumComp = iArray->GetNumberOfComponents();
      oNumComp = oArray->GetNumberOfComponents();
      if (promote && oType != VTK_FLOAT && oType != VTK_DOUBLE)
      {
        oType = VTK_FLOAT;
        vtkFloatArray* fArray = vtkFloatArray::New();
        fArray->SetName(oArray->GetName());
        fArray->SetNumberOfComponents(oNumComp);
        outPD->AddArray(fArray); // nasty side effect will replace current array in the same spot
        oArray = fArray;
        fArray->Delete();
      }
      oArray->SetNumberOfTuples(numOutPts);

      assert(iNumComp == oNumComp);
      if (iType == oType)
      {
        iD = iArray->GetVoidPointer(0);
        oD = oArray->GetVoidPointer(0);
        switch (iType)
        {
          vtkExtendedTemplateMacro(CreateArrayPair(this, static_cast<VTK_TT*>(iD),
            static_cast<VTK_TT*>(oD), numOutPts, oNumComp, oArray, nullValue));
        }  // over all VTK types
      }    // if matching types
      else // promoted type
      {
        iD = iArray->GetVoidPointer(0);
        oD = oArray->GetVoidPointer(0);
        switch (iType)
        {
          vtkTemplateMacro(CreateRealArrayPair(this, static_cast<VTK_TT*>(iD),
            static_cast<float*>(oD), numOutPts, iNumComp, oArray, static_cast<float>(nullValue)));
        } // over all VTK types
      }   // if promoted pair
    }
  } // for each candidate array
}

//----------------------------------------------------------------------------
// Add the arrays to interpolate here. This presumes that vtkDataSetAttributes::CopyData() or
// vtkDataSetAttributes::InterpolateData() has been called. This special version creates an
// array pair that interpolates from itself.
inline void ArrayList::AddSelfInterpolatingArrays(
  vtkIdType numOutPts, vtkDataSetAttributes* attr, double nullValue)
{
  // Build the vector of interpolation pairs. Note that CopyAllocate/InterpolateAllocate should have
  // been called at this point (output arrays created and allocated).
  vtkAbstractArray* iArray;
  int iType, iNumComp;
  void* iD;
  int i, numArrays = attr->GetNumberOfArrays();

  for (i = 0; i < numArrays; ++i)
  {
    iArray = attr->GetArray(i);
    if (iArray && !this->IsExcluded(iArray))
    {
      iType = iArray->GetDataType();
      iNumComp = iArray->GetNumberOfComponents();
      iArray->Resize(numOutPts);
      iD = iArray->GetVoidPointer(0);
      switch (iType)
      {
        vtkTemplateMacro(CreateArrayPair(this, static_cast<VTK_TT*>(iD), static_cast<VTK_TT*>(iD),
          numOutPts, iNumComp, iArray, static_cast<VTK_TT>(nullValue)));
      } // over all VTK types
    }   // if not excluded
  }     // for each candidate array
}

VTK_ABI_NAMESPACE_END
#endif
