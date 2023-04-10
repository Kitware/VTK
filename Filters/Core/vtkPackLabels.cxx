/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPackLabels.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPackLabels.h"

#include "vtkArrayDispatch.h"
#include "vtkCellData.h"
#include "vtkDataArrayRange.h"
#include "vtkImageData.h"
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

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkPackLabels);

// TODO:
// This is a work in progress. There is much else that could be done:
// + The filter is really a vtkDataSet to vtkDataSet filter since it operates
//   scalars independent of vtkDataSet type.
// + Use the data array to process to select the appropriate array to pack
// + Additional threading
// + Use of ArrayTuple type access
// + Output an array of counts of each label (GetLabelCounts)
// + Specify a desired output number of labels M, which would output the top M
//   most used labels. Any label outside of the top M would be set to a specified
//   background label.

//------------------------------------------------------------------------------
// Internal classes and methods for packing.
namespace
{ // anonymous

// Sort scalars identify unique labels.
struct BuildLabels
{
  template <typename ArrayT>
  void operator()(ArrayT* sortScalars, vtkDataArray* labelsArray)
  {
    vtkIdType numScalars = sortScalars->GetNumberOfTuples();
    using T = vtk::GetAPIType<ArrayT>;

    // We'll be sorting the data.
    T* data = sortScalars->GetPointer(0);

    // The labels are the same type as the input scalars.
    ArrayT* labels = static_cast<ArrayT*>(labelsArray);

    // Sort the input array to identify unique labels
    vtkSMPTools::Sort(data, data + numScalars);

    // Now run through the labels and identify unique. Insert the
    // unique labels into the labels array.
    T label = data[0];
    T nextLabel;
    labels->InsertNextValue(label);
    for (auto i = 1; i < numScalars; ++i)
    {
      nextLabel = data[i];
      if (nextLabel != label)
      {
        labels->InsertNextValue(nextLabel);
        label = nextLabel;
      }
    }
  } // operator()
};  // BuildLabels

// Map the input labels to the output labels
struct MapLabels
{
  template <typename Array0T, typename Array1T>
  void operator()(Array0T* inScalars, Array1T* outScalars, vtkDataArray* labelsArray)
  {
    vtkIdType numScalars = inScalars->GetNumberOfTuples();
    using T0 = vtk::GetAPIType<Array0T>;
    using T1 = vtk::GetAPIType<Array1T>;

    // We'll ba mapping the input scalars to output scalars.
    T0* data0 = inScalars->GetPointer(0);
    T1* data1 = outScalars->GetPointer(0);

    // The labels are the same type as the input scalars.
    Array0T* labels = static_cast<Array0T*>(labelsArray);

    // To create a packed image, a map must be created that maps from the
    // original label value to the new packed label.
    std::map<T0, T1> labelMap;
    vtkIdType numLabels = labels->GetNumberOfTuples();
    for (vtkIdType i = 0; i < numLabels; ++i)
    {
      labelMap[labels->GetValue(i)] = i;
    }

    // Now loop over the input image, map the data values into the new
    // labels. This could be sped up with a templated lambda within
    // vtkSMPTools (exercise left for the user).
    for (auto id = 0; id < numScalars; ++id)
    {
      typename std::map<T0, T1>::iterator it = labelMap.find(data0[id]);
      data1[id] = it->second;
    }
  } // operator()
};  // MapLabels

} // end anonymous

//------------------------------------------------------------------------------
// Okay define the VTK class proper
vtkPackLabels::vtkPackLabels()
{
  this->OutputScalarType = VTK_DEFAULT_TYPE;
  this->PassPointData = true;
  this->PassCellData = true;
}

//------------------------------------------------------------------------------
bool vtkPackLabels::IsInputPacked()
{
  if (this->LabelsArray == nullptr)
  {
    return false;
  }

  vtkIdType numLabels = this->LabelsArray->GetNumberOfTuples();
  vtkIdType startId = this->LabelsArray->GetTuple1(0);
  vtkIdType endId = this->LabelsArray->GetTuple1(numLabels - 1);
  return ((endId - startId + 1) == numLabels ? true : false);
}

//------------------------------------------------------------------------------
// Find all the labels in the input.
int vtkPackLabels::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDebugMacro(<< "Executing Pack Labels");

  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkImageData* input = vtkImageData::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkImageData* output = vtkImageData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Copy the input data to the output. The temporary sortScalars
  // and labels array must be the same type as the input scalars.
  vtkDataArray* inScalars = input->GetPointData()->GetScalars();
  vtkSmartPointer<vtkDataArray> sortScalars;
  sortScalars.TakeReference(inScalars->NewInstance());
  sortScalars->DeepCopy(inScalars);
  this->LabelsArray.TakeReference(inScalars->NewInstance());

  // Now populate the labels array.  We could use a std::set or std::map. But
  // these are not threaded. So instead we use a threaded sort, and then
  // build the array of labels from that.  We'll have to use a typed
  // dispatch.
  using AllTypes = vtkArrayDispatch::AllTypes;
  using BuildDispatch = vtkArrayDispatch::DispatchByValueType<AllTypes>;
  BuildLabels buildLabels;
  if (!BuildDispatch::Execute(sortScalars, buildLabels, this->LabelsArray.Get()))
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
    if (N < std::numeric_limits<unsigned char>::max())
    {
      outScalars.TakeReference(vtkDataArray::CreateDataArray(VTK_UNSIGNED_CHAR));
    }
    else if (N < std::numeric_limits<unsigned short>::max())
    {
      outScalars.TakeReference(vtkDataArray::CreateDataArray(VTK_UNSIGNED_SHORT));
    }
    else if (N < std::numeric_limits<unsigned int>::max())
    {
      outScalars.TakeReference(vtkDataArray::CreateDataArray(VTK_UNSIGNED_INT));
    }
    else // if ( N < std::numeric_limits<unsigned long>::max() )
    {
      outScalars.TakeReference(vtkDataArray::CreateDataArray(VTK_UNSIGNED_LONG));
    }
  }
  vtkDebugMacro("Create packed scalars of type:" << outScalars->GetDataType());
  outScalars->SetNumberOfTuples(inScalars->GetNumberOfTuples());

  // Map the input data to output data using the new labels.
  using LabelTypes =
    vtkTypeList::Create<unsigned char, unsigned short, unsigned int, unsigned long>;
  using MapDispatch = vtkArrayDispatch::Dispatch2ByValueType<AllTypes, LabelTypes>;
  MapLabels mapLabels;
  MapDispatch::Execute(inScalars, outScalars.Get(), mapLabels, this->LabelsArray.Get());

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

  // Replace scalar array with packed array.
  output->GetPointData()->SetScalars(outScalars);

  return 1;
}

//------------------------------------------------------------------------------
int vtkPackLabels::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  return 1;
}

//------------------------------------------------------------------------------
void vtkPackLabels::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Labels Array: " << this->LabelsArray.Get() << "\n";
  os << indent << "Output Scalar Type: " << this->OutputScalarType << "\n";
  os << indent << "Pass Point Data: " << (this->PassPointData ? "On\n" : "Off\n");
  os << indent << "Pass Cell Data: " << (this->PassCellData ? "On\n" : "Off\n");
  os << indent << "Pass Field Data: " << (this->PassFieldData ? "On\n" : "Off\n");
}

VTK_ABI_NAMESPACE_END
