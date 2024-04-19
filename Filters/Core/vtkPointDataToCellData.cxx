// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPointDataToCellData.h"

#include "vtkArrayDispatch.h"
#include "vtkArrayListTemplate.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSMPThreadLocalObject.h"
#include "vtkSMPTools.h"

#include <algorithm>
#include <cassert>
#include <limits>
#include <set>
#include <vector>

#define VTK_EPSILON 1.e-6

// Anonymous namespace
VTK_ABI_NAMESPACE_BEGIN
namespace
{
//------------------------------------------------------------------------------
class PointDataToCellDataFunctor
{
private:
  vtkDataSet* Input;
  ArrayList Arrays;
  vtkSMPThreadLocalObject<vtkIdList> TLCellPts; // scratch array
  vtkPointDataToCellData* Filter;

public:
  PointDataToCellDataFunctor(
    vtkDataSet* input, vtkPointData* inPD, vtkCellData* outCD, vtkPointDataToCellData* filter)
    : Input(input)
    , Filter(filter)
  {
    vtkIdType numCells = input->GetNumberOfCells();
    this->Arrays.AddArrays(numCells, inPD, outCD);

    // call everything we will call on the data object on the main thread first
    // so that it can build its caching structures
    vtkNew<vtkGenericCell> cell;
    this->Input->GetCell(0, cell);
  }

  void Initialize() { this->TLCellPts.Local()->Allocate(128); }

  void operator()(vtkIdType beginCellId, vtkIdType endCellId)
  {
    auto& cellPts = this->TLCellPts.Local();
    bool isFirst = vtkSMPTools::GetSingleThread();
    vtkIdType checkAbortInterval = std::min((endCellId - beginCellId) / 10 + 1, (vtkIdType)1000);
    for (vtkIdType cellId = beginCellId; cellId < endCellId; ++cellId)
    {
      if (cellId % checkAbortInterval == 0)
      {
        if (isFirst)
        {
          this->Filter->CheckAbort();
        }
        if (this->Filter->GetAbortOutput())
        {
          break;
        }
      }
      this->Input->GetCellPoints(cellId, cellPts);
      vtkIdType numPts = cellPts->GetNumberOfIds();

      if (numPts == 0)
      {
        continue;
      }

      // Non-categorical -> simply average the data.
      this->Arrays.Average(numPts, cellPts->GetPointer(0), cellId);
    }
  }

  void Reduce() {}
};

//------------------------------------------------------------------------------
// Used to process categorical data
class Histogram
{
public:
  struct Bin
  {
    // A histogram bin is comprised of the following:
    // index: the point index associated with the bin
    // count: the number of elements in the bin
    // value: the point data value associated with the bin
    Bin(vtkIdType index, vtkIdType count, double value)
      : Index(index)
      , Count(count)
      , Value(value)
    {
    }

    friend bool operator<(const Bin& b1, const Bin& b2) { return b1.Value < b2.Value; }

    bool Assigned() const { return this->Index != -1; }

    vtkIdType Index;
    vtkIdType Count;
    double Value;
  };

  typedef std::vector<Bin> HistogramBins;
  typedef HistogramBins::iterator BinIt;

  Histogram() = default;

  void Initialize(vtkIdType size) { this->Bins.assign(size + 1, Histogram::Init); }

  // Reset the fields of the bins in the histogram.
  void Reset(vtkIdType size)
  {
    for (vtkIdType i = 0; i < size + 1; i++)
    {
      this->Bins[i] = Histogram::Init;
    }
    this->Counter = 0;
  }

  // Simply populate the next bin in the histogram with the provided index
  // and value.
  void Fill(vtkIdType index, double value)
  {
    assert(static_cast<std::size_t>(this->Counter) < this->Bins.size());

    Bin& bin = this->Bins[this->Counter++];
    bin.Index = index;
    bin.Value = value;
  }

  // Return the index of the bin with the largest count (i.e. the majority
  // value).
  vtkIdType IndexOfLargestBin();

  static Bin Init;
  HistogramBins Bins;
  vtkIdType Counter;
};

//------------------------------------------------------------------------------
Histogram::Bin Histogram::Init(-1, 1, std::numeric_limits<double>::max());

//------------------------------------------------------------------------------
bool BinCountCmp(const Histogram::Bin& b1, const Histogram::Bin& b2)
{
  if (b1.Count < b2.Count)
  {
    return true;
  }
  else if (b1.Count > b2.Count)
  {
    return false;
  }
  else
  {
    return (b1.Value < b2.Value);
  }
}

//------------------------------------------------------------------------------
vtkIdType Histogram::IndexOfLargestBin()
{
  // If there is only one datapoint, return its index
  if (this->Counter == 1)
  {
    return this->Bins[0].Index;
  }

  // Sort the histogram bins by value, effectively grouping like bins
  std::sort(this->Bins.begin(), this->Bins.end());

  // Perform a single sweep, comparing adjacent bins.
  BinIt it2 = this->Bins.begin();
  BinIt it1 = it2++;
  for (; (*it2).Assigned() && it2 != this->Bins.end(); ++it2)
  {
    // If the adjacent bins are close enough to be merged...
    if (std::abs((*it1).Value - (*it2).Value) < VTK_EPSILON)
    {
      //...then increment the count of the first bin. We need not worry about
      // the count of the second bin, since it will remain 1 and will therefore
      // not be in contention for the largest bin.
      (*it1).Count++;
    }
    else
    {
      //...otherwise, move on to the next potential set of merges.
      it1 = it2;
    }
  }

  // Finally, return the index of the element with the largest count. If there
  // is more than one, the index of the element with the smallest value is
  // consistently chosen.
  return std::max_element(this->Bins.begin(), it2, BinCountCmp)->Index;
}

//------------------------------------------------------------------------------
template <typename ArrayType>
class PointDataToCellDataCategoricalFunctor
{
private:
  vtkDataSet* Input;
  ArrayType* Scalars;
  using ScalarsValueT = vtk::GetAPIType<ArrayType>;

  ArrayList Arrays;
  int MaxCellSize;
  vtkSMPThreadLocal<Histogram> TLHistogram;
  vtkSMPThreadLocalObject<vtkIdList> TLCellPts;
  vtkPointDataToCellData* Filter;

public:
  PointDataToCellDataCategoricalFunctor(vtkDataSet* input, vtkPointData* inPD, vtkCellData* outCD,
    ArrayType* scalars, vtkPointDataToCellData* filter)
    : Input(input)
    , Scalars(scalars)
    , Filter(filter)
  {
    vtkIdType numCells = input->GetNumberOfCells();
    this->Arrays.AddArrays(numCells, inPD, outCD);

    this->MaxCellSize = input->GetMaxCellSize();
    // call everything we will call on the data object on the main thread first
    // so that it can build its caching structures
    vtkNew<vtkGenericCell> cell;
    this->Input->GetCell(0, cell);
  }

  void Initialize()
  {
    this->TLHistogram.Local().Initialize(this->MaxCellSize);
    this->TLCellPts.Local()->Allocate(this->MaxCellSize);
  }

  void operator()(vtkIdType beginCellId, vtkIdType endCellId)
  {
    auto& cellPts = this->TLCellPts.Local();
    auto& histogram = this->TLHistogram.Local();
    const auto scalars = vtk::DataArrayValueRange<1>(this->Scalars);
    bool isFirst = vtkSMPTools::GetSingleThread();
    vtkIdType checkAbortInterval = std::min((endCellId - beginCellId) / 10 + 1, (vtkIdType)1000);
    for (vtkIdType cellId = beginCellId; cellId < endCellId; ++cellId)
    {
      if (cellId % checkAbortInterval == 0)
      {
        if (isFirst)
        {
          this->Filter->CheckAbort();
        }
        if (this->Filter->GetAbortOutput())
        {
          break;
        }
      }
      this->Input->GetCellPoints(cellId, cellPts);
      vtkIdType numPts = cellPts->GetNumberOfIds();

      if (numPts == 0)
      {
        continue;
      }

      // Populate a histogram from the scalar values at each
      // point, and then select the bin with the most elements.
      histogram.Reset(numPts);
      for (vtkIdType ptId = 0; ptId < numPts; ++ptId)
      {
        auto pointId = cellPts->GetId(ptId);
        histogram.Fill(pointId, static_cast<ScalarsValueT>(scalars[pointId]));
      }
      this->Arrays.Copy(histogram.IndexOfLargestBin(), cellId);
    }
  }

  void Reduce() {}
};

//------------------------------------------------------------------------------
struct PointDataToCellDataCategoricalWorker
{
  template <typename ArrayType>
  void operator()(ArrayType* scalars, vtkDataSet* input, vtkPointData* inPD, vtkCellData* outCD,
    vtkPointDataToCellData* filter)
  {
    PointDataToCellDataCategoricalFunctor<ArrayType> pd2cd(input, inPD, outCD, scalars, filter);
    vtkSMPTools::For(0, input->GetNumberOfCells(), pd2cd);
  }
};
} // End anonymous namespace

//------------------------------------------------------------------------------
class vtkPointDataToCellData::Internals
{
public:
  std::set<std::string> PointDataArrays;
};

//------------------------------------------------------------------------------
vtkObjectFactoryNewMacro(vtkPointDataToCellData);

//------------------------------------------------------------------------------
// Instantiate object so that point data is not passed to output.
vtkPointDataToCellData::vtkPointDataToCellData()
{
  this->PassPointData = false;
  this->CategoricalData = false;
  this->ProcessAllArrays = true;
  this->Implementation = new Internals();
}

//------------------------------------------------------------------------------
vtkPointDataToCellData::~vtkPointDataToCellData()
{
  delete this->Implementation;
}

//------------------------------------------------------------------------------
void vtkPointDataToCellData::AddPointDataArray(const char* name)
{
  if (!name)
  {
    vtkErrorMacro("name cannot be null.");
    return;
  }

  this->Implementation->PointDataArrays.insert(std::string(name));
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkPointDataToCellData::RemovePointDataArray(const char* name)
{
  if (!name)
  {
    vtkErrorMacro("name cannot be null.");
    return;
  }

  this->Implementation->PointDataArrays.erase(name);
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkPointDataToCellData::ClearPointDataArrays()
{
  if (!this->Implementation->PointDataArrays.empty())
  {
    this->Modified();
  }
  this->Implementation->PointDataArrays.clear();
}

//------------------------------------------------------------------------------
vtkIdType vtkPointDataToCellData::GetNumberOfPointArraysToProcess()
{
  return static_cast<vtkIdType>(this->Implementation->PointDataArrays.size());
}

//------------------------------------------------------------------------------
void vtkPointDataToCellData::GetPointArraysToProcess(const char* names[])
{
  for (const auto& n : this->Implementation->PointDataArrays)
  {
    *names = n.c_str();
    ++names;
  }
}

//------------------------------------------------------------------------------
int vtkPointDataToCellData::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* info = outputVector->GetInformationObject(0);
  vtkDataSet* output = vtkDataSet::SafeDownCast(info->Get(vtkDataObject::DATA_OBJECT()));

  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkDataSet* input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkIdType numCells;
  vtkPointData* inputInPD = input->GetPointData();
  vtkSmartPointer<vtkPointData> inPD;
  vtkCellData* outCD = output->GetCellData();

  if (!this->ProcessAllArrays)
  {
    inPD = vtkSmartPointer<vtkPointData>::New();

    for (const auto& name : this->Implementation->PointDataArrays)
    {
      vtkAbstractArray* arr = inputInPD->GetAbstractArray(name.c_str());
      if (arr == nullptr)
      {
        vtkWarningMacro("point data array name not found.");
        continue;
      }
      inPD->AddArray(arr);
    }
  }
  else
  {
    inPD = inputInPD;
  }

  vtkDebugMacro(<< "Mapping point data to cell data");

  // First, copy the input to the output as a starting point
  output->CopyStructure(input);

  if ((numCells = input->GetNumberOfCells()) < 1)
  {
    vtkDebugMacro(<< "No input cells!");
    return 1;
  }

  if (this->CategoricalData == 1)
  {
    // If the categorical data flag is enabled, then a) there must be scalars
    // to treat as categorical data, and b) the scalars must have one component.
    if (!input->GetPointData()->GetScalars())
    {
      vtkDebugMacro(<< "No input scalars!");
      return 1;
    }
    if (input->GetPointData()->GetScalars()->GetNumberOfComponents() != 1)
    {
      vtkDebugMacro(<< "Input scalars have more than one component! Cannot categorize!");
      return 1;
    }

    // Set the scalar to interpolate via nearest neighbor. That way, we won't
    // get any false values (for example, a zone 4 cell appearing on the
    // boundary of zone 3 and zone 5).
    output->GetPointData()->SetCopyAttribute(
      vtkDataSetAttributes::SCALARS, 2, vtkDataSetAttributes::INTERPOLATE);
  }

  // Pass the cell data first. The fields and attributes
  // which also exist in the point data of the input will
  // be over-written during CopyAllocate
  output->GetCellData()->PassData(input->GetCellData());
  output->GetCellData()->CopyFieldOff(vtkDataSetAttributes::GhostArrayName());

  // Notice that inPD and outCD are vtkPointData and vtkCellData; respectively.
  // It's weird, but it works.
  outCD->InterpolateAllocate(inPD, numCells);

  // Create a threaded fast path for non-categorical data.
  if (!this->CategoricalData)
  {
    // Thread the process
    PointDataToCellDataFunctor pd2cd(input, inPD, outCD, this);
    vtkSMPTools::For(0, numCells, pd2cd);
  }
  // Create a threaded fast path for categorical data.
  else
  {
    PointDataToCellDataCategoricalWorker worker;
    if (!vtkArrayDispatch::Dispatch::Execute(inPD->GetScalars(), worker, input, inPD, outCD, this))
    {
      worker(inPD->GetScalars(), input, inPD, outCD, this);
    }
  } // categorical data

  // Pass data if requested.
  if (!this->PassPointData)
  {
    output->GetPointData()->CopyAllOff();
    output->GetPointData()->CopyFieldOn(vtkDataSetAttributes::GhostArrayName());
  }
  output->GetPointData()->PassData(input->GetPointData());
  output->GetFieldData()->PassData(input->GetFieldData());

  return 1;
}

//------------------------------------------------------------------------------
void vtkPointDataToCellData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Categorical Data: " << (this->CategoricalData ? "On\n" : "Off\n");
  os << indent << "Pass Point Data: " << (this->PassPointData ? "On\n" : "Off\n");
}
VTK_ABI_NAMESPACE_END
