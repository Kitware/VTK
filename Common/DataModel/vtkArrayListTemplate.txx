// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkArrayDispatch.h"
#include "vtkArrayListTemplate.h"
#include "vtkFloatArray.h"

#include <cassert>

#ifndef vtkArrayListTemplate_txx
#define vtkArrayListTemplate_txx

//----------------------------------------------------------------------------
// Sort of a little object factory
VTK_ABI_NAMESPACE_BEGIN
struct ArrayPairCreator
{
  template <typename TInputArray, typename TOutputArray,
    typename T = std::conditional_t<std::is_same<TOutputArray, vtkStringArray>::value, vtkStdString,
      vtk::GetAPIType<TOutputArray>>>
  void operator()(TInputArray* inArray, TOutputArray* outArray, ArrayList* list,
    vtkIdType numTuples, int numComp, T nullValue)
  {
    auto* pair =
      new ArrayPair<TInputArray, TOutputArray, T>(inArray, outArray, numTuples, numComp, nullValue);
    list->Arrays.push_back(pair);
  }
};

//----------------------------------------------------------------------------
// Indicate arrays not to process
inline void ArrayList::ExcludeArray(vtkAbstractArray* da)
{
  this->ExcludedArrays.push_back(da);
}

//----------------------------------------------------------------------------
// Has the specified array been excluded?
inline vtkTypeBool ArrayList::IsExcluded(vtkAbstractArray* da)
{
  return std::find(ExcludedArrays.begin(), ExcludedArrays.end(), da) != ExcludedArrays.end();
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
    ArrayPairCreator creator;
    if (!vtkArrayDispatch::Dispatch::Execute(inArray, creator,
          vtkAOSDataArrayTemplate<float>::FastDownCast(outArray), this, numTuples,
          inArray->GetNumberOfComponents(), static_cast<float>(nullValue)))
    {
      creator(vtkDataArray::FastDownCast(inArray),
        vtkAOSDataArrayTemplate<float>::FastDownCast(outArray), this, numTuples,
        inArray->GetNumberOfComponents(), static_cast<float>(nullValue));
    }
  }
  else
  {
    outArray = inArray->NewInstance();
    outArray->SetNumberOfComponents(inArray->GetNumberOfComponents());
    outArray->SetNumberOfTuples(numTuples);
    outArray->SetName(outArrayName.c_str());
    ArrayPairCreator creator;
    if (!vtkArrayDispatch::Dispatch2SameValueType::Execute(
          inArray, outArray, creator, this, numTuples, inArray->GetNumberOfComponents(), nullValue))
    {
      if (inArray->IsA("vtkDataArray") && outArray->IsA("vtkDataArray"))
      {
        creator(vtkDataArray::FastDownCast(inArray), vtkDataArray::FastDownCast(outArray), this,
          numTuples, inArray->GetNumberOfComponents(), nullValue);
      }
      else if (inArray->IsA("vtkStringArray") && outArray->IsA("vtkStringArray"))
      {
        creator(vtkStringArray::FastDownCast(inArray), vtkStringArray::FastDownCast(outArray), this,
          numTuples, inArray->GetNumberOfComponents(), vtkStdString(vtk::to_string(nullValue)));
      }
    }
  } // promote integral types

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
  for (auto& i : outPD->RequiredArrays)
  {
    vtkAbstractArray* iArray = inPD->Data[i];
    vtkAbstractArray* oArray = outPD->Data[outPD->TargetIndices[i]];

    if (iArray && oArray && !this->IsExcluded(oArray) && !this->IsExcluded(iArray))
    {
      int iType = iArray->GetDataType();
      int oType = oArray->GetDataType();
      if (promote && oType != VTK_FLOAT && oType != VTK_DOUBLE)
      {
        oType = VTK_FLOAT;
        vtkFloatArray* fArray = vtkFloatArray::New();
        fArray->SetName(oArray->GetName());
        fArray->SetNumberOfComponents(oArray->GetNumberOfComponents());
        outPD->AddArray(fArray); // nasty side effect will replace current array in the same spot
        oArray = fArray;
        fArray->Delete();
      }
      oArray->SetNumberOfTuples(numOutPts);

      assert(iArray->GetNumberOfComponents() == oArray->GetNumberOfComponents());
      ArrayPairCreator creator;
      if (iType == oType)
      {
        if (!vtkArrayDispatch::Dispatch2SameValueType::Execute(
              iArray, oArray, creator, this, numOutPts, iArray->GetNumberOfComponents(), nullValue))
        {
          if (iArray->IsA("vtkDataArray") && oArray->IsA("vtkDataArray"))
          {
            creator(vtkDataArray::FastDownCast(iArray), vtkDataArray::FastDownCast(oArray), this,
              numOutPts, iArray->GetNumberOfComponents(), nullValue);
          }
          else if (iArray->IsA("vtkStringArray") && iArray->IsA("vtkStringArray"))
          {
            creator(vtkStringArray::FastDownCast(iArray), vtkStringArray::FastDownCast(oArray),
              this, numOutPts, iArray->GetNumberOfComponents(),
              vtkStdString(vtk::to_string(nullValue)));
          }
        }
      }    // if matching types
      else // promoted type
      {
        if (!vtkArrayDispatch::Dispatch::Execute(iArray, creator,
              vtkAOSDataArrayTemplate<float>::FastDownCast(oArray), this, numOutPts,
              iArray->GetNumberOfComponents(), static_cast<float>(nullValue)))
        {
          creator(vtkDataArray::FastDownCast(iArray),
            vtkAOSDataArrayTemplate<float>::FastDownCast(oArray), this, numOutPts,
            iArray->GetNumberOfComponents(), static_cast<float>(nullValue));
        }
      } // if promoted pair
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
  for (int i = 0; i < attr->GetNumberOfArrays(); ++i)
  {
    vtkAbstractArray* iArray = attr->GetArray(i);
    if (iArray && !this->IsExcluded(iArray))
    {
      iArray->Resize(numOutPts);
      ArrayPairCreator creator;
      if (!vtkArrayDispatch::Dispatch2SameValueType::Execute(
            iArray, iArray, creator, this, numOutPts, iArray->GetNumberOfComponents(), nullValue))
      {
        if (iArray->IsA("vtkDataArray"))
        {
          creator(vtkDataArray::FastDownCast(iArray), vtkDataArray::FastDownCast(iArray), this,
            numOutPts, iArray->GetNumberOfComponents(), nullValue);
        }
        else if (iArray->IsA("vtkStringArray"))
        {
          creator(vtkStringArray::FastDownCast(iArray), vtkStringArray::FastDownCast(iArray), this,
            numOutPts, iArray->GetNumberOfComponents(), vtkStdString(vtk::to_string(nullValue)));
        }
      }
    } // if not excluded
  }   // for each candidate array
}

VTK_ABI_NAMESPACE_END
#endif
