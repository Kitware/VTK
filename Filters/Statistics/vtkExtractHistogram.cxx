/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractHistogram.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExtractHistogram.h"

#include "vtkArrayDispatch.h"
#include "vtkCellData.h"
#include "vtkCharArray.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataArrayRange.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkGraph.h"
#include "vtkHyperTreeGrid.h"
#include "vtkIOStream.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSMPThreadLocal.h"
#include "vtkSMPTools.h"
#include "vtkShortArray.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTable.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkUnsignedShortArray.h"

#include <array>
#include <map>
#include <string>
#include <vector>

class vtkExtractHistogramInternal
{
public:
  struct ArrayValuesType
  {
    // The total of the values per bin - the second vector
    // is for arrays with multiple components
    std::vector<std::vector<double>> TotalValues;
  };
  using ArrayMapType = std::map<std::string, ArrayValuesType>;
  ArrayMapType ArrayValues;
  int FieldAssociation = -1;
};

vtkStandardNewMacro(vtkExtractHistogram);
//-----------------------------------------------------------------------------
vtkExtractHistogram::vtkExtractHistogram()
  : Internal(new vtkExtractHistogramInternal())
{
  this->SetBinExtentsArrayName("bin_extents");
  this->SetBinValuesArrayName("bin_values");
  this->SetBinAccumulationArrayName("bin_accumulation");

  this->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS, vtkDataSetAttributes::SCALARS);
}

//-----------------------------------------------------------------------------
vtkExtractHistogram::~vtkExtractHistogram()
{
  this->SetBinExtentsArrayName(nullptr);
  this->SetBinValuesArrayName(nullptr);
  this->SetBinAccumulationArrayName(nullptr);
}

//-----------------------------------------------------------------------------
void vtkExtractHistogram::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Component: " << this->Component << "\n";
  os << indent << "BinCount: " << this->BinCount << "\n";
  os << indent << "CenterBinsAroundMinAndMax: " << this->CenterBinsAroundMinAndMax << "\n";
  os << indent << "UseCustomBinRanges: " << this->UseCustomBinRanges << "\n";
  os << indent << "CustomBinRanges: " << this->CustomBinRanges[0] << ", "
     << this->CustomBinRanges[1] << "\n";
  os << indent << "BinExtentsArrayName" << this->BinExtentsArrayName << "\n";
  os << indent << "BinValuesArrayName" << this->BinValuesArrayName << "\n";
  os << indent << "BinAccumulationArrayName" << this->BinAccumulationArrayName << "\n";
  os << indent << "Normalize: " << this->Normalize << "\n";
  os << indent << "Accumulation: " << this->Accumulation << "\n";
  os << indent << endl;
}

//-----------------------------------------------------------------------------
int vtkExtractHistogram::FillInputPortInformation(int port, vtkInformation* info)
{
  this->Superclass::FillInputPortInformation(port, info);

  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  return 1;
}

//-----------------------------------------------------------------------------
int vtkExtractHistogram::GetInputFieldAssociation()
{
  vtkInformationVector* inArrayVec = this->Information->Get(INPUT_ARRAYS_TO_PROCESS());
  vtkInformation* inArrayInfo = inArrayVec->GetInformationObject(0);
  return inArrayInfo->Get(vtkDataObject::FIELD_ASSOCIATION());
}

//-----------------------------------------------------------------------------
vtkFieldData* vtkExtractHistogram::GetInputFieldData(vtkDataObject* input)
{
  if (this->Internal->FieldAssociation < 0)
  {
    this->Internal->FieldAssociation = this->GetInputFieldAssociation();
  }

  switch (this->Internal->FieldAssociation)
  {
    case vtkDataObject::FIELD_ASSOCIATION_POINTS:
      VTK_FALLTHROUGH;
    case vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS:
      return vtkDataSet::SafeDownCast(input)->GetPointData();
    case vtkDataObject::FIELD_ASSOCIATION_CELLS:
    {
      if (auto ds = vtkDataSet::SafeDownCast(input))
      {
        return ds->GetCellData();
      }
      else if (auto htg = vtkHyperTreeGrid::SafeDownCast(input))
      {
        return htg->GetCellData();
      }
      else
      {
        vtkErrorMacro("Unsupported input type: " << input->GetClassName());
        return nullptr;
      }
    }
    case vtkDataObject::FIELD_ASSOCIATION_NONE:
      return input->GetFieldData();
    case vtkDataObject::FIELD_ASSOCIATION_VERTICES:
      return vtkGraph::SafeDownCast(input)->GetVertexData();
    case vtkDataObject::FIELD_ASSOCIATION_EDGES:
      return vtkGraph::SafeDownCast(input)->GetEdgeData();
    case vtkDataObject::FIELD_ASSOCIATION_ROWS:
      return vtkTable::SafeDownCast(input)->GetRowData();
  }
  return nullptr;
}

//-----------------------------------------------------------------------------
namespace
{
template <typename ArrayT>
class FiniteMinAndMaxWithBlankingFunctor
{
protected:
  ArrayT* Array;
  vtkUnsignedCharArray* GhostArray = nullptr;
  unsigned char HiddenFlag;
  double ReducedRange[2];
  int Component;
  vtkSMPThreadLocal<std::array<double, 2>> TLRange;

public:
  FiniteMinAndMaxWithBlankingFunctor(
    ArrayT* array, int component, vtkUnsignedCharArray* ghostArray, unsigned char hiddenFlag)
    : Array(array)
    , GhostArray(ghostArray)
    , HiddenFlag(hiddenFlag)
    , Component(component)
  {
  }

  void Initialize()
  {
    auto& range = this->TLRange.Local();
    range[0] = vtkTypeTraits<double>::Max();
    range[1] = vtkTypeTraits<double>::Min();
    this->ReducedRange[0] = vtkTypeTraits<double>::Max();
    this->ReducedRange[1] = vtkTypeTraits<double>::Min();
  }

  void Reduce()
  {
    for (auto& range : this->TLRange)
    {
      this->ReducedRange[0] = vtkMath::Min(this->ReducedRange[0], range[0]);
      this->ReducedRange[1] = vtkMath::Max(this->ReducedRange[1], range[1]);
    }
  }

  void CopyRange(double* range)
  {
    range[0] = this->ReducedRange[0];
    range[1] = this->ReducedRange[1];
  }

  void operator()(vtkIdType begin, vtkIdType end)
  {
    auto& range = this->TLRange.Local();
    const auto valueRange = vtk::DataArrayTupleRange(this->Array);
    bool computeMagnitude = this->Component == this->Array->GetNumberOfComponents();

    if (this->GhostArray)
    {
      const auto ghostRange = vtk::DataArrayValueRange(this->GhostArray);
      for (vtkIdType idx = begin; idx < end; ++idx)
      {
        // Skip if the array value is blanked.
        if (ghostRange[idx] & this->HiddenFlag)
        {
          continue;
        }

        double value = 0.0;
        if (computeMagnitude)
        {
          double sum = static_cast<double>(vtkMath::SquaredNorm(valueRange[idx]));
          value = std::sqrt(sum);
        }
        else
        {
          value = static_cast<double>(valueRange[idx][this->Component]);
        }

        if (vtkMath::IsFinite(value))
        {
          range[0] = vtkMath::Min(range[0], value);
          range[1] = vtkMath::Max(range[1], value);
        }
      }
    }
    else
    {
      for (vtkIdType idx = begin; idx < end; ++idx)
      {
        double value = 0.0;
        if (computeMagnitude)
        {
          double sum = static_cast<double>(vtkMath::SquaredNorm(valueRange[idx]));
          value = std::sqrt(sum);
        }
        else
        {
          auto tuple = valueRange[idx];
          value = static_cast<double>(tuple[this->Component]);
        }

        if (vtkMath::IsFinite(value))
        {
          range[0] = vtkMath::Min(range[0], value);
          range[1] = vtkMath::Max(range[1], value);
        }
      }
    }
  }
};

// Functor for array dispatch
struct GetRangeWithBlankingWorker
{
  template <typename ArrayT>
  void operator()(ArrayT* array, int component, vtkUnsignedCharArray* ghostArray,
    unsigned char hiddenFlag, double tRange[2])
  {
    FiniteMinAndMaxWithBlankingFunctor<ArrayT> functor(array, component, ghostArray, hiddenFlag);
    vtkSMPTools::For(0, array->GetNumberOfTuples(), functor);
    functor.CopyRange(tRange);
  }
};

// Local version of GetRange that respects point/cell Blanking
void GetRangeWithBlanking(
  vtkDataArray* array, vtkFieldData* fieldData, double tRange[2], int component)
{
  vtkUnsignedCharArray* ghostArray = nullptr;
  vtkDataSetAttributes* attributes = vtkDataSetAttributes::SafeDownCast(fieldData);
  if (attributes)
  {
    auto ghostDataArray = attributes->GetArray(vtkDataSetAttributes::GhostArrayName());
    ghostArray = vtkUnsignedCharArray::SafeDownCast(ghostDataArray);
  }
  const unsigned char hiddenFlag = attributes->IsA("vtkPointData")
    ? (vtkDataSetAttributes::HIDDENPOINT | vtkDataSetAttributes::DUPLICATEPOINT)
    : (vtkDataSetAttributes::HIDDENCELL | vtkDataSetAttributes::DUPLICATECELL);

  using FastArrayTypes = vtkTypeList::Unique<
    vtkTypeList::Create<vtkCharArray, vtkShortArray, vtkIntArray, vtkUnsignedCharArray,
      vtkUnsignedShortArray, vtkUnsignedIntArray, vtkFloatArray, vtkDoubleArray>>::Result;
  using GetRangeWithBlankingWorkerDispatch = vtkArrayDispatch::DispatchByArray<FastArrayTypes>;

  if (!GetRangeWithBlankingWorkerDispatch::Execute(
        array, GetRangeWithBlankingWorker{}, component, ghostArray, hiddenFlag, tRange))
  {
    GetRangeWithBlankingWorker{}(array, component, ghostArray, hiddenFlag, tRange);
  }
}
}

//-----------------------------------------------------------------------------
bool vtkExtractHistogram::GetInputArrayRange(vtkInformationVector** inputVector, double range[2])
{
  range[0] = VTK_DOUBLE_MAX;
  range[1] = VTK_DOUBLE_MIN;

  // obtain a pointer to the name of the vtkDataArray to bin up
  // and find the range of the data values within it
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkDataObject* input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkCompositeDataSet* cdin = vtkCompositeDataSet::SafeDownCast(input);
  if (cdin)
  {
    // for composite datasets, visit each leaf data set and compute the total
    // range
    vtkCompositeDataIterator* cdit = cdin->NewIterator();
    cdit->InitTraversal();
    bool foundOne = false;
    while (!cdit->IsDoneWithTraversal())
    {
      vtkDataObject* dObj = cdit->GetCurrentDataObject();
      vtkDataArray* data_array = this->GetInputArrayToProcess(0, dObj);
      if (data_array && this->Component >= 0 &&
        this->Component <= data_array->GetNumberOfComponents())
      {
        foundOne = true;
        double tRange[2];
        GetRangeWithBlanking(data_array, this->GetInputFieldData(dObj), tRange, this->Component);
        range[0] = (tRange[0] < range[0]) ? tRange[0] : range[0];
        range[1] = (tRange[1] > range[1]) ? tRange[1] : range[1];
      }
      cdit->GoToNextItem();
    }
    cdit->Delete();

    if (!foundOne)
    {
      return false;
    }
  }
  else
  {
    vtkDataArray* data_array = this->GetInputArrayToProcess(0, inputVector);
    if (!data_array)
    {
      return false;
    }
    // If the requested component is out-of-range for the input, we return an
    // empty dataset
    if (this->Component < 0 && this->Component > data_array->GetNumberOfComponents())
    {
      vtkWarningMacro("Requested component " << this->Component << " is not available.");
      return false;
    }
    vtkFieldData* fieldData = this->GetInputFieldData(input);
    GetRangeWithBlanking(data_array, fieldData, range, this->Component);
  }

  return true;
}

//-----------------------------------------------------------------------------
bool vtkExtractHistogram::InitializeBinExtents(
  vtkInformationVector** inputVector, vtkDoubleArray* binExtents)
{
  this->BinRange[0] = VTK_DOUBLE_MAX;
  this->BinRange[1] = -VTK_DOUBLE_MAX;

  // Keeping the column name constant causes less issues in the GUI.
  binExtents->SetName(this->BinExtentsArrayName);

  if (this->UseCustomBinRanges)
  {
    this->BinRange[0] = this->CustomBinRanges[0];
    this->BinRange[1] = this->CustomBinRanges[1];
  }
  else if (!this->GetInputArrayRange(inputVector, this->BinRange) ||
    (this->BinRange[0] > this->BinRange[1]))
  {
    //
    // We don't flag this as error since the array may just be missing for
    // current time-step, see
    // https://www.paraview.org/Bug/print_bug_page.php?bug_id=12290
    //
    vtkDebugMacro("Could not determine array range. "
                  "The chosen array or component may not be available or "
                  "has invalid range");
    return false;
  }

  // Enforce a minimum bin width of +/- 0.5
  if (this->BinRange[0] == this->BinRange[1])
  {
    this->BinRange[0] -= 0.5;
    this->BinRange[1] += 0.5;
  }

  this->FillBinExtents(binExtents);
  return true;
}

//-----------------------------------------------------------------------------
void vtkExtractHistogram::FillBinExtents(vtkDoubleArray* binExtents)
{
  binExtents->SetNumberOfComponents(1);
  binExtents->SetNumberOfTuples(this->BinCount);
  double bin_delta = (this->BinRange[1] - this->BinRange[0]) /
    (this->CenterBinsAroundMinAndMax ? (this->BinCount - 1) : this->BinCount);
  double half_delta = bin_delta / 2.0;
  for (int i = 0; i < this->BinCount; ++i)
  {
    binExtents->SetValue(
      i, this->BinRange[0] + (i * bin_delta) + (this->CenterBinsAroundMinAndMax ? 0. : half_delta));
  }
}

//-----------------------------------------------------------------------------
void vtkExtractHistogram::NormalizeBins(vtkTable* outputData)
{
  vtkDataArray* bin_values =
    vtkDataArray::SafeDownCast(outputData->GetColumnByName(this->BinValuesArrayName));
  vtkNew<vtkDoubleArray> normalized_values;
  normalized_values->SetName(this->BinValuesArrayName);
  normalized_values->SetNumberOfComponents(1);
  normalized_values->SetNumberOfTuples(bin_values->GetNumberOfTuples());

  auto bin_range = vtk::DataArrayValueRange<1>(bin_values);
  int sum = 0;
  for (auto bin_val : bin_range)
  {
    sum += bin_val;
  }

  auto normalized_range = vtk::DataArrayValueRange<1>(normalized_values);
  auto bin_iter = bin_range.begin();
  auto normalized_iter = normalized_range.begin();
  for (; bin_iter != bin_range.end(); ++bin_iter, ++normalized_iter)
  {
    *normalized_iter = static_cast<double>(bin_iter[0]) / sum;
  }

  // Replace the previous bin values array with the normalized version.
  outputData->GetRowData()->AddArray(normalized_values);
}

//-----------------------------------------------------------------------------
void vtkExtractHistogram::AccumulateBins(vtkTable* outputData)
{
  vtkDataArray* bin_values =
    vtkDataArray::SafeDownCast(outputData->GetColumnByName(this->BinValuesArrayName));
  vtkDataArray* bin_accum = bin_values->NewInstance();
  bin_accum->SetName(this->BinAccumulationArrayName);
  bin_accum->SetNumberOfComponents(1);
  bin_accum->SetNumberOfTuples(bin_values->GetNumberOfTuples());

  auto bin_range = vtk::DataArrayValueRange<1>(bin_values);
  auto accum_range = vtk::DataArrayValueRange<1>(bin_accum);
  auto bin_iter = bin_range.begin();
  auto accum_iter = accum_range.begin();
  double sum = 0;
  for (; bin_iter != bin_range.end(); ++bin_iter, ++accum_iter)
  {
    sum += static_cast<double>(bin_iter[0]);
    *accum_iter = sum;
  }
  // Add in the new or replace the previous accumulation array.
  outputData->GetRowData()->AddArray(bin_accum);
  // Delete object created with NewInstance()
  bin_accum->Delete();
}

//-----------------------------------------------------------------------------
inline int vtkExtractHistogramClamp(int value, int min, int max)
{
  value = value < min ? min : value;
  value = value > max ? max : value;
  return value;
}

//-----------------------------------------------------------------------------
namespace
{
template <typename ArrayT>
class BinAnArrayFunctor
{
private:
  ArrayT* DataArray;
  vtkFieldData* Field;
  vtkIntArray* BinValues;
  vtkExtractHistogramInternal::ArrayMapType& ArrayValues;
  const char* BinValuesArrayName;
  int BinCount;
  int Component;
  double Min;
  double Max;
  int CalculateAverages;
  bool CenterBinsAroundMinAndMax;

  double BinDelta;
  double HalfDelta;
  vtkUnsignedCharArray* Blanking;
  unsigned char GhostIndicator;

  vtkSMPThreadLocal<vtkSmartPointer<vtkIntArray>> TLBinValues;
  vtkSMPThreadLocal<vtkExtractHistogramInternal::ArrayMapType> TLArrayValues;

public:
  BinAnArrayFunctor(ArrayT* dataArray, vtkFieldData* field, vtkIntArray* binValues,
    vtkExtractHistogramInternal::ArrayMapType& arrayValues, const char* binValuesArrayName,
    int binCount, int component, double min, double max, int calculateAverages,
    bool centerBinsAroundMinAndMax)
    : DataArray(dataArray)
    , Field(field)
    , BinValues(binValues)
    , ArrayValues(arrayValues)
    , BinValuesArrayName(binValuesArrayName)
    , BinCount(binCount)
    , Component(component)
    , Min(min)
    , Max(max)
    , CalculateAverages(calculateAverages)
    , CenterBinsAroundMinAndMax(centerBinsAroundMinAndMax)
  {
    this->BinDelta = (this->Max - this->Min) /
      (this->CenterBinsAroundMinAndMax ? (this->BinCount - 1) : this->BinCount);
    this->HalfDelta = this->BinDelta / 2.0;

    // Get blanking array
    this->Blanking = nullptr;
    vtkDataSetAttributes* attributes = vtkDataSetAttributes::SafeDownCast(this->Field);
    if (attributes)
    {
      auto ghostArray = attributes->GetArray(vtkDataSetAttributes::GhostArrayName());
      this->Blanking = vtkUnsignedCharArray::SafeDownCast(ghostArray);
    }
    this->GhostIndicator = field->IsA("vtkPointData")
      ? (vtkDataSetAttributes::HIDDENPOINT | vtkDataSetAttributes::DUPLICATEPOINT)
      : (vtkDataSetAttributes::HIDDENCELL | vtkDataSetAttributes::DUPLICATECELL);
  }

  void Initialize()
  {
    // initialize thread copies
    auto& tlBinValues = this->TLBinValues.Local();
    tlBinValues = vtkSmartPointer<vtkIntArray>::New();
    tlBinValues->SetNumberOfComponents(1);
    tlBinValues->SetNumberOfTuples(this->BinCount);
    tlBinValues->SetName(this->BinValuesArrayName);
    tlBinValues->FillComponent(0, 0.0);

    this->TLArrayValues.Local();
  }

  void operator()(vtkIdType begin, vtkIdType end)
  {
    auto& tlBinValues = this->TLBinValues.Local();
    auto& tlArrayValues = this->TLArrayValues.Local();
    const auto valueRange = vtk::DataArrayTupleRange(this->DataArray);
    const int numComponents = this->DataArray->GetNumberOfComponents();

    for (int i = begin; i < end; ++i)
    {
      // Skip if the array value is blanked.
      if (this->Blanking && this->Blanking->GetTypedComponent(i, 0) & this->GhostIndicator)
      {
        continue;
      }

      double value;
      // if component is equal to the number of components, then the magnitude was requested.
      if (this->Component == numComponents)
      {
        value = 0;
        double comp;
        for (int j = 0; j < numComponents; ++j)
        {
          comp = static_cast<double>(valueRange[i][j]);
          value += comp * comp;
        }
        value = std::sqrt(value);
      }
      else
      {
        value = static_cast<double>(valueRange[i][this->Component]);
      }
      int index = static_cast<int>(
        (value - this->Min + (this->CenterBinsAroundMinAndMax ? this->HalfDelta : 0.)) /
        this->BinDelta);

      // If the value is equal to max, include it in the last bin.
      index = ::vtkExtractHistogramClamp(index, 0, this->BinCount - 1);
      tlBinValues->SetValue(index, tlBinValues->GetValue(index) + 1);

      if (this->CalculateAverages)
      {
        // Get all other arrays, add their value to the bin
        // For each bin, we will need 2 values per array ->
        // total, num. elements
        // at the end, divide each total by num. elements
        int numberOfArrays = this->Field->GetNumberOfArrays();
        for (int idx = 0; idx < numberOfArrays; idx++)
        {
          vtkDataArray* array = this->Field->GetArray(idx);
          if (array && array != this->DataArray && array->GetName())
          {
            vtkExtractHistogramInternal::ArrayValuesType& arrayValues =
              tlArrayValues[array->GetName()];
            arrayValues.TotalValues.resize(this->BinCount);
            int numComps = array->GetNumberOfComponents();
            arrayValues.TotalValues[index].resize(numComps);
            for (int comp = 0; comp < numComps; comp++)
            {
              arrayValues.TotalValues[index][comp] += array->GetComponent(i, comp);
            }
          }
        }
      }
    }
  }

  void Reduce()
  {
    // collect binValues results
    for (auto& tlBinValues : this->TLBinValues)
    {
      for (int i = 0; i < this->BinCount; ++i)
      {
        this->BinValues->SetValue(i, this->BinValues->GetValue(i) + tlBinValues->GetValue(i));
      }
    }

    // collect arrayValues results
    if (this->CalculateAverages)
    {
      int numberOfArrays = this->Field->GetNumberOfArrays();
      for (int idx = 0; idx < numberOfArrays; idx++)
      {
        vtkDataArray* array = this->Field->GetArray(idx);
        // if array is not valid move to the next one
        if (!(array && array != this->DataArray && array->GetName()))
        {
          continue;
        }

        int numComps = this->Field->GetArray(array->GetName())->GetNumberOfComponents();
        // iterate over all thread instances
        for (auto& tlArrayValues : this->TLArrayValues)
        {
          vtkExtractHistogramInternal::ArrayValuesType& arrayValuesOfArray =
            this->ArrayValues[array->GetName()];
          vtkExtractHistogramInternal::ArrayValuesType& tlArrayValuesOfArray =
            tlArrayValues[array->GetName()];

          // resizing will occur only if it's needed
          arrayValuesOfArray.TotalValues.resize(this->BinCount);

          for (int i = 0; i < this->BinCount; ++i)
          {
            if (!tlArrayValuesOfArray.TotalValues[i].empty())
            {
              // resizing will occur only if it's needed
              arrayValuesOfArray.TotalValues[i].resize(tlArrayValuesOfArray.TotalValues[i].size());

              for (int comp = 0; comp < numComps; comp++)
              {
                arrayValuesOfArray.TotalValues[i][comp] +=
                  tlArrayValuesOfArray.TotalValues[i][comp];
              }
            }
          }
        }
      }
    }
  }
};

// Worker for array dispatch
struct BinAnArrayWorker
{
  template <typename ArrayT>
  void operator()(ArrayT* dataArray, vtkFieldData* field, vtkIntArray* binValues,
    vtkExtractHistogramInternal::ArrayMapType& arrayValues, const char* binValuesArrayName,
    int binCount, int component, double min, double max, int calculateAverages,
    bool centerBinsAroundMinAndMax)
  {
    BinAnArrayFunctor<ArrayT> functor(dataArray, field, binValues, arrayValues, binValuesArrayName,
      binCount, component, min, max, calculateAverages, centerBinsAroundMinAndMax);
    vtkSMPTools::For(0, dataArray->GetNumberOfTuples(), functor);
  }
};
}

//-----------------------------------------------------------------------------
void vtkExtractHistogram::BinAnArray(
  vtkDataArray* dataArray, vtkIntArray* binValues, double min, double max, vtkFieldData* field)
{
  // If the requested component is out-of-range for the input,
  // the bin_values will be 0, so no need to do any actual counting.
  if (dataArray == nullptr || this->Component < 0 ||
    this->Component > dataArray->GetNumberOfComponents())
  {
    return;
  }

  using FastArrayTypes = vtkTypeList::Unique<
    vtkTypeList::Create<vtkCharArray, vtkShortArray, vtkIntArray, vtkUnsignedCharArray,
      vtkUnsignedShortArray, vtkUnsignedIntArray, vtkFloatArray, vtkDoubleArray>>::Result;
  using BinAnArrayWorkerDispatch = vtkArrayDispatch::DispatchByArray<FastArrayTypes>;

  if (!BinAnArrayWorkerDispatch::Execute(dataArray, BinAnArrayWorker{}, field, binValues,
        this->Internal->ArrayValues, this->BinValuesArrayName, this->BinCount, this->Component, min,
        max, this->CalculateAverages, this->CenterBinsAroundMinAndMax))
  {
    BinAnArrayWorker{}(dataArray, field, binValues, this->Internal->ArrayValues,
      this->BinValuesArrayName, this->BinCount, this->Component, min, max, this->CalculateAverages,
      this->CenterBinsAroundMinAndMax);
  }
}

//-----------------------------------------------------------------------------
int vtkExtractHistogram::RequestData(vtkInformation* /*request*/,
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Build an empty output grid in advance, so we can bail-out if we
  // encounter any problems
  vtkTable* const outputData = vtkTable::GetData(outputVector, 0);
  outputData->Initialize();

  if (this->UseCustomBinRanges && this->CustomBinRanges[1] < this->CustomBinRanges[0])
  {
    double min = this->CustomBinRanges[1];
    double max = this->CustomBinRanges[0];
    this->CustomBinRanges[0] = min;
    this->CustomBinRanges[1] = max;
    vtkWarningMacro("Custom bin range adjusted to keep min <= max value");
  }

  // These are the mid-points for each of the bins
  auto binExtents = vtkSmartPointer<vtkDoubleArray>::New();
  binExtents->SetNumberOfComponents(1);
  binExtents->SetNumberOfTuples(this->BinCount);
  binExtents->SetName(this->BinExtentsArrayName);
  binExtents->FillComponent(0, 0.0);

  // Insert values into bins ...
  auto binValues = vtkSmartPointer<vtkIntArray>::New();
  binValues->SetNumberOfComponents(1);
  binValues->SetNumberOfTuples(this->BinCount);
  binValues->SetName(this->BinValuesArrayName);
  binValues->FillComponent(0, 0.0);

  // Initializes the bin extents array.
  if (!this->InitializeBinExtents(inputVector, binExtents))
  {
    this->Internal->ArrayValues.clear();
    return 1;
  }

  outputData->GetRowData()->AddArray(binExtents);
  outputData->GetRowData()->AddArray(binValues);

  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkDataObject* input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkCompositeDataSet* cdin = vtkCompositeDataSet::SafeDownCast(input);
  if (cdin)
  {
    // for composite datasets visit each leaf dataset and add in its counts
    vtkCompositeDataIterator* cdit = cdin->NewIterator();
    cdit->InitTraversal();
    while (!cdit->IsDoneWithTraversal())
    {
      vtkDataObject* dObj = cdit->GetCurrentDataObject();
      vtkDataArray* dataArray = this->GetInputArrayToProcess(0, dObj);
      this->BinAnArray(
        dataArray, binValues, this->BinRange[0], this->BinRange[1], this->GetInputFieldData(dObj));
      cdit->GoToNextItem();
    }
    cdit->Delete();
  }
  else
  {
    vtkDataArray* dataArray = this->GetInputArrayToProcess(0, inputVector);
    this->BinAnArray(
      dataArray, binValues, this->BinRange[0], this->BinRange[1], this->GetInputFieldData(input));
  }

  if (this->CalculateAverages)
  {
    for (auto& iter : this->Internal->ArrayValues)
    {
      auto da = vtkSmartPointer<vtkDoubleArray>::New();
      std::string newName = iter.first + "_total";
      da->SetName(newName.c_str());
      auto aa = vtkSmartPointer<vtkDoubleArray>::New();
      std::string newName2 = iter.first + "_average";
      aa->SetName(newName2.c_str());
      int numComps = static_cast<int>(iter.second.TotalValues[0].size());
      da->SetNumberOfComponents(numComps);
      da->SetNumberOfTuples(this->BinCount);
      aa->SetNumberOfComponents(numComps);
      aa->SetNumberOfTuples(this->BinCount);
      for (vtkIdType i = 0; i < this->BinCount; i++)
      {
        for (int j = 0; j < numComps; j++)
        {
          if (iter.second.TotalValues[i].size() == (unsigned int)numComps)
          {
            da->SetValue(i * numComps + j, iter.second.TotalValues[i][j]);
            if (binValues->GetValue(i))
            {
              aa->SetValue(
                i * numComps + j, iter.second.TotalValues[i][j] / binValues->GetValue(i));
            }
            else
            {
              aa->SetValue(i * numComps + j, 0);
            }
          }
          else
          {
            da->SetValue(i * numComps + j, 0);
            aa->SetValue(i * numComps + j, 0);
          }
        }
      }
      outputData->GetRowData()->AddArray(da);
      outputData->GetRowData()->AddArray(aa);
    }

    this->Internal->ArrayValues.clear();
  }

  if (this->Normalize)
  {
    NormalizeBins(outputData);
  }

  if (this->Accumulation)
  {
    AccumulateBins(outputData);
  }

  return 1;
}
