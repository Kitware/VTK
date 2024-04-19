// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPackLabels.h"

#include "vtkArrayDispatch.h"
#include "vtkCellData.h"
#include "vtkDataArrayRange.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSMPTools.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTypeList.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkUnsignedShortArray.h"

#include <map>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkPackLabels);

// TODO:
// This is a work in progress. There is much else that could be done:
// + Additional threading
// + Use of ArrayTuple type access

//------------------------------------------------------------------------------
// Internal classes and methods for packing.
namespace
{ // anonymous

// This struct is used to sort labels by the frequency of occurrence (i.e.,
// its count).
template <typename T>
struct LabelTuple
{
  T Label;
  vtkIdType Count;
  LabelTuple()
    : Label(0)
    , Count(0)
  {
  }

  // This comparison produces a stable sort because it also
  // considers the label value (it the label count is a tie).
  bool operator>(const LabelTuple& tuple) const
  {
    if (Count > tuple.Count)
      return true;
    if (tuple.Count > Count)
      return false;
    if (Label > tuple.Label)
      return true;
    return false;
  }
}; // LabelTuple

// This helper function performs the sorting of labels by count.
// Used when SortByLabelCount is on.
template <typename T>
void SortLabelsByCount(vtkIdType numLabels, T* labels, vtkIdType* counts)
{
  // Insert tuples into pre-allocated vector
  std::vector<LabelTuple<T>> labelTuples(numLabels);
  for (vtkIdType i = 0; i < numLabels; ++i)
  {
    labelTuples[i].Label = labels[i];
    labelTuples[i].Count = counts[i];
  }

  // Sort the vector of tuples.
  vtkSMPTools::Sort(labelTuples.begin(), labelTuples.end(), std::greater<LabelTuple<T>>());

  // Now repopulate the LabelsArray and LabelsCount arrays.
  for (vtkIdType i = 0; i < numLabels; ++i)
  {
    labels[i] = labelTuples[i].Label;
    counts[i] = labelTuples[i].Count;
  }
} // SortLabelsByCount

// Sort input scalars to identify unique labels (labels array). Also extract
// the frequency of occurrence of each label (labels count). Finally if
// requested, sort the labels array based on labels count in descending order
// of occurrence.
struct BuildLabels
{
  template <typename ArrayT>
  void operator()(
    ArrayT* sortScalars, vtkDataArray* labelsArray, vtkIdTypeArray* labelsCount, int sortBy)
  {
    vtkIdType numScalars = sortScalars->GetNumberOfTuples();
    using T = vtk::GetAPIType<ArrayT>;

    // We'll be sorting the data.
    T* data = sortScalars->GetPointer(0);

    // The labels are the same type as the input scalars.
    ArrayT* labels = static_cast<ArrayT*>(labelsArray);

    // Sort the input array to identify unique labels.
    vtkSMPTools::Sort(data, data + numScalars);

    // Now run through the labels and identify unique. Insert the unique
    // labels into the labels array. The result of this process is a sorted
    // list of labels by the label value (SORT_BY_LABEL_VALUE).
    T label = data[0];
    T nextLabel;
    labels->InsertNextValue(label);
    vtkIdType count = 1;
    for (vtkIdType i = 1; i < numScalars; ++i)
    {
      nextLabel = data[i];
      if (nextLabel != label)
      {
        labels->InsertNextValue(nextLabel);
        label = nextLabel;
        labelsCount->InsertNextValue(count);
        count = 1;
      }
      else
      {
        count++;
      }
    }
    labelsCount->InsertNextValue(count);

    // If sorting by label counts is enabled, do it now.
    if (sortBy == vtkPackLabels::SORT_BY_LABEL_COUNT)
    {
      vtkIdType numLabels = labels->GetNumberOfTuples();
      T* labelsPtr = labels->GetPointer(0);
      vtkIdType* countsPtr = labelsCount->GetPointer(0);
      SortLabelsByCount(numLabels, labelsPtr, countsPtr);
    } // if SORT_BY_LABEL_COUNT

  } // operator()
};  // BuildLabels

// Map the input labels to the output labels
struct MapLabels
{
  template <typename Array0T, typename Array1T>
  void operator()(Array0T* inScalars, Array1T* outScalars, vtkDataArray* labelsArray,
    vtkIdType maxLabels, unsigned long backgroundValue)
  {
    vtkIdType numScalars = inScalars->GetNumberOfTuples();
    using T0 = vtk::GetAPIType<Array0T>;
    using T1 = vtk::GetAPIType<Array1T>;

    // We'll ba mapping the input scalars to output scalars.
    T0* data0 = inScalars->GetPointer(0);
    T1* data1 = outScalars->GetPointer(0);

    // The labels are the same type as the input scalars.
    Array0T* labels = static_cast<Array0T*>(labelsArray);

    // To create a packed array, a map must be created that maps from the
    // original label value to the new packed label.
    std::map<T0, T1> labelMap;
    vtkIdType numLabels = labels->GetNumberOfTuples();
    numLabels = (numLabels > maxLabels ? maxLabels : numLabels);
    for (vtkIdType i = 0; i < numLabels; ++i)
    {
      labelMap[labels->GetValue(i)] = i;
    }

    // Now loop over the input scalar array, and map the data values into the
    // new labels. This could be sped up with a templated lambda within
    // vtkSMPTools (exercise left for the user).
    for (vtkIdType id = 0; id < numScalars; ++id)
    {
      typename std::map<T0, T1>::iterator it = labelMap.find(data0[id]);
      if (it != labelMap.end())
      {
        data1[id] = it->second;
      }
      else
      {
        data1[id] = backgroundValue;
      }
    }
  } // operator()
};  // MapLabels

// Give a VTK data type, determine the maximum number of
// labels that can be represented.
unsigned long GetMaxLabels(int dataType)
{
  unsigned long maxLabels;
  if (dataType == VTK_UNSIGNED_CHAR)
  {
    maxLabels = std::numeric_limits<unsigned char>::max();
  }
  else if (dataType == VTK_UNSIGNED_SHORT)
  {
    maxLabels = std::numeric_limits<unsigned short>::max();
  }
  else if (dataType == VTK_UNSIGNED_INT)
  {
    maxLabels = std::numeric_limits<unsigned int>::max();
  }
  else // if ( dataType == VTK_UNSIGNED_LONG )
  {
    maxLabels = std::numeric_limits<unsigned long>::max();
  }

  return maxLabels;
} // GetMaxLabels

} // end anonymous

//------------------------------------------------------------------------------
// Define the VTK class proper
vtkPackLabels::vtkPackLabels()
{
  this->SortBy = SORT_BY_LABEL_VALUE;
  this->OutputScalarType = VTK_DEFAULT_TYPE;
  this->BackgroundValue = 0;
  this->PassPointData = true;
  this->PassCellData = true;
  this->PassFieldData = true;

  // By default process point scalars, then cell scalars
  this->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS, vtkDataSetAttributes::SCALARS);
}

//------------------------------------------------------------------------------
// Find all the labels in the input.
int vtkPackLabels::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDebugMacro(<< "Executing Pack Labels");

  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataSet* input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet* output = vtkDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Copy the input scalar data to the output. The temporary sortScalars
  // and labels array must be the same type as the input scalars.
  vtkDataArray* inScalars = this->GetInputArrayToProcess(0, inputVector);
  int fieldAssociation = this->GetInputArrayAssociation(0, inputVector);
  if (!inScalars)
  {
    vtkErrorMacro("No input scalars");
    return 1;
  }
  vtkSmartPointer<vtkDataArray> sortScalars;
  sortScalars.TakeReference(inScalars->NewInstance());
  sortScalars->DeepCopy(inScalars);
  this->LabelsArray.TakeReference(inScalars->NewInstance());
  this->LabelsCount.TakeReference(vtkIdTypeArray::New());

  // Now populate the labels array which requires sorting the scalars.  We
  // could use a std::set or std::map. But these are not threaded. So instead
  // we use a threaded sort, and then build the array of labels from that.
  // We'll have to use a typed dispatch.
  using AllTypes = vtkArrayDispatch::AllTypes;
  using BuildDispatch = vtkArrayDispatch::DispatchByValueType<AllTypes>;
  BuildLabels buildLabels;
  if (!BuildDispatch::Execute(
        sortScalars, buildLabels, this->LabelsArray.Get(), this->LabelsCount.Get(), this->SortBy))
  { // Fallback should never happen
    vtkErrorMacro("Data array not supported");
    return 1;
  }

  // We have the labels, now delete the temporary array using smart pointer magic.
  sortScalars = nullptr;

  // Now let's determine the output scalars type. If manually specified, we use
  // that type. Otherwise, find the smallest integral type that can represent the
  // N packed labels discovered.
  vtkIdType N = this->LabelsArray->GetNumberOfTuples();
  vtkSmartPointer<vtkDataArray> outScalars;
  if (this->OutputScalarType != VTK_DEFAULT_TYPE)
  {
    // Create specified type
    outScalars.TakeReference(vtkDataArray::CreateDataArray(this->OutputScalarType));
  }
  else
  {
    // Create smallest type that can represent N labels
    if (N < static_cast<vtkIdType>(std::numeric_limits<unsigned char>::max()))
    {
      outScalars.TakeReference(vtkDataArray::CreateDataArray(VTK_UNSIGNED_CHAR));
    }
    else if (N < static_cast<vtkIdType>(std::numeric_limits<unsigned short>::max()))
    {
      outScalars.TakeReference(vtkDataArray::CreateDataArray(VTK_UNSIGNED_SHORT));
    }
    else if (N < static_cast<vtkIdType>(std::numeric_limits<unsigned int>::max()))
    {
      outScalars.TakeReference(vtkDataArray::CreateDataArray(VTK_UNSIGNED_INT));
    }
    else // if ( N < std::numeric_limits<unsigned long>::max() )
    {
      outScalars.TakeReference(vtkDataArray::CreateDataArray(VTK_UNSIGNED_LONG));
    }
  }
  vtkIdType maxLabels = GetMaxLabels(outScalars->GetDataType());
  if (N > maxLabels)
  {
    vtkWarningMacro(
      "Due to specified output data type, truncating number of labels to: " << maxLabels);
  }

  outScalars->SetName("PackedLabels");
  vtkDebugMacro("Create packed scalars of type:" << outScalars->GetDataType());
  outScalars->SetNumberOfTuples(inScalars->GetNumberOfTuples());

  // Map the input data to output data using the new labels.
  using LabelTypes =
    vtkTypeList::Create<unsigned char, unsigned short, unsigned int, unsigned long>;
  using MapDispatch = vtkArrayDispatch::Dispatch2ByValueType<AllTypes, LabelTypes>;
  MapLabels mapLabels;
  MapDispatch::Execute(inScalars, outScalars.Get(), mapLabels, this->LabelsArray.Get(), maxLabels,
    this->BackgroundValue);

  // Finally, create the output scalars. If passData is set, then the
  // input is simply passed to the output. Otherwise, the output scalars
  // are specified as the output. Note that passData is set when the
  // input and output data types are the same, and the input labels
  // where found to be packed.
  // Start by passing data, replace the output scalars if necessary.
  output->CopyStructure(input);
  if (this->PassPointData)
  {
    output->GetPointData()->PassData(input->GetPointData());
  }
  if (this->PassCellData)
  {
    output->GetCellData()->PassData(input->GetCellData());
  }
  if (this->PassFieldData)
  {
    output->GetFieldData()->PassData(input->GetFieldData());
  }

  // Replace scalar array with packed array. Depending on whether the
  // data origin is from point or cell data, update appropriately.
  if (fieldAssociation == vtkDataObject::FIELD_ASSOCIATION_POINTS)
  {
    output->GetPointData()->SetScalars(outScalars);
  }
  else
  {
    output->GetCellData()->SetScalars(outScalars);
  }

  return 1;
}

//------------------------------------------------------------------------------
void vtkPackLabels::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Labels Array: " << this->LabelsArray.Get() << "\n";
  os << indent << "Labels Count: " << this->LabelsCount.Get() << "\n";
  os << indent
     << "Sort By: " << (this->SortBy == SORT_BY_LABEL_VALUE ? "Label Value\n" : "Label Count\n");
  os << indent << "Output Scalar Type: " << this->OutputScalarType << "\n";
  os << indent << "Pass Point Data: " << (this->PassPointData ? "On\n" : "Off\n");
  os << indent << "Pass Cell Data: " << (this->PassCellData ? "On\n" : "Off\n");
  os << indent << "Pass Field Data: " << (this->PassFieldData ? "On\n" : "Off\n");
}

VTK_ABI_NAMESPACE_END
