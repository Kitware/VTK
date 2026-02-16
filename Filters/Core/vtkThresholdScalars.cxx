// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkThresholdScalars.h"

#include "vtkArrayDispatch.h"
#include "vtkDataArrayRange.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSMPTools.h"

#include <algorithm>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkThresholdScalars);

namespace // anonymous
{
// Represent a threshold interval.
struct ThresholdInterval
{
  double SMin;
  double SMax;
  int Label;
  vtkIdType IntervalId;
  ThresholdInterval(double smin, double smax, int label, vtkIdType id)
    : SMin(smin)
    , SMax(smax)
    , Label(label)
    , IntervalId(id)
  {
    // Make the threshold interval is ordered properly.
    if (this->SMin > this->SMax)
    {
      std::swap(this->SMin, this->SMax);
    }
  }

  // For sorting the threshold intervals (based on Smin)
  bool operator<(const ThresholdInterval& ti) const { return this->SMin < ti.SMin; }

  // For comparing against specified scalar value
  bool operator<(double s) const { return this->SMin < s; }
}; // ThresholdInterval

using IntervalSet = std::vector<ThresholdInterval>;

} // end anonymous namespace

// A relatively inefficient class to keep track of intervals,
// and assign region labels given a scalar value. It assumes that
// the number of intervals is a few dozen; if the scale is increased,
// the class may need performance improvements.
class vtkIntervalSet
{
public:
  vtkIdType IntervalId = 0;
  int BackgroundLabel;
  IntervalSet Intervals;

  void SetBackgroundLabel(int background) { this->BackgroundLabel = background; }
  vtkIdType GetNumberOfIntervals() { return this->Intervals.size(); }
  vtkIdType AddInterval(double s0, double s1, int label)
  {
    this->Intervals.emplace_back(s0, s1, label, this->IntervalId);
    return this->IntervalId++;
  }
  void RemoveInterval(vtkIdType intervalId)
  {
    for (auto lItr = this->Intervals.begin(); lItr != this->Intervals.end(); ++lItr)
    {
      if (lItr->IntervalId == intervalId)
      {
        this->Intervals.erase(lItr);
        break;
      }
    }
  }
  void RemoveAllIntervals()
  {
    this->Intervals.clear();
    this->IntervalId = 0;
  }
  void Update() { std::sort(this->Intervals.begin(), this->Intervals.end()); }
  // Thread-safe method to determine which interval a scalar
  // value is in. If none, then the background label is returned.
  // This should only be called after the Update() method is invoked.
  // This assumes that the threshold intervals are disjoint. The
  // approach searches for the beginning of the interval, and then
  // checks to see which +/- interval s lies in.
  int GetRegionLabel(double s) const
  {
    // Do not modify the interval set
    const IntervalSet& intervals = this->Intervals;

    // Make sure there is something to search for
    if (intervals.empty() || s < intervals.front().SMin || s >= intervals.back().SMax)
    {
      return this->BackgroundLabel;
    }

    // Search for the containing interval. This will quickly identify
    // the approximate position - but need to check boundary conditions.
    auto itr = std::lower_bound(intervals.begin(), intervals.end(), s);
    if (itr != intervals.end())
    {
      // If s == SMin, then this works
      if (itr->SMin <= s && s < itr->SMax)
      {
        return itr->Label;
      }
      // More often, s is in the previous interval to the behavior of
      // std::lower_bound()
      else if (itr > intervals.begin())
      {
        --itr;
        if (itr->SMin <= s && s < itr->SMax)
        {
          return itr->Label;
        }
      }
    }
    else // The last interval needs special treatment.
    {
      if (intervals.back().SMin <= s && s < intervals.back().SMax)
      {
        return intervals.back().Label;
      }
    }

    return this->BackgroundLabel;
  }
};

namespace // anonymous
{
// Threaded perform threshold lookup
struct ThresholdWorker
{
  template <typename ST>
  void operator()(ST* inScalars, vtkIdType numScalars, int* sPtr, const vtkIntervalSet& intervals)
  {
    vtkSMPTools::For(0, numScalars,
      [&inScalars, sPtr, &intervals](vtkIdType id, vtkIdType endId)
      {
        const auto inS = vtk::DataArrayTupleRange(inScalars);
        for (; id < endId; ++id)
        {
          const auto sTuple = inS[id];
          double s = static_cast<double>(sTuple[0]);
          sPtr[id] = intervals.GetRegionLabel(s);
        }
      });
  }
}; // ThresholdWorker

} // end anonymous namespace

//=================================Begin class proper=========================
//------------------------------------------------------------------------------
// Construct object with background label = (-100) and no threshd intervals
vtkThresholdScalars::vtkThresholdScalars()
{
  this->BackgroundLabel = (-100);
  this->Intervals = new vtkIntervalSet;
  this->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::SCALARS);
}

//------------------------------------------------------------------------------
// Destroy the allocated interval set
vtkThresholdScalars::~vtkThresholdScalars()
{
  delete this->Intervals;
}

//------------------------------------------------------------------------------
// Segment the input scalar field
//
int vtkThresholdScalars::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataSet* input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet* output = vtkDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkPointData *pd = input->GetPointData(), *outPD = output->GetPointData();
  vtkDataArray* inScalars = this->GetInputArrayToProcess(0, inputVector);

  // Initialize
  vtkDebugMacro(<< "Thresholding scalars");

  // First, copy the input to the output as a starting point.
  output->CopyStructure(input);
  outPD->PassData(pd);

  if (!(inScalars = pd->GetScalars()))
  {
    vtkErrorMacro(<< "No scalars defined!");
    return 1;
  }
  vtkIdType numScalars = inScalars->GetNumberOfTuples();
  if (numScalars < 1)
  {
    vtkErrorMacro(<< "Input scalars are empty!");
    return 1;
  }
  if (this->Intervals->GetNumberOfIntervals() < 1)
  {
    vtkErrorMacro(<< "No thresholding intervals defined!");
    return 1;
  }

  // Allocate the output scalars
  //
  vtkNew<vtkIntArray> newScalars;
  newScalars->SetName("Thresholded Scalars");
  newScalars->SetNumberOfTuples(numScalars);
  int* sPtr = newScalars->GetPointer(0);

  // Threaded loop over all input scalars, assigning region ids
  // based on the defined set of intervals. Depending on the type
  // of input scalars, dispatch to the appropriate functor.
  this->Intervals->SetBackgroundLabel(this->BackgroundLabel);
  this->Intervals->Update();

  ThresholdWorker thWorker;
  vtkArrayDispatch::Dispatch::Execute(inScalars, thWorker, numScalars, sPtr, *this->Intervals);

  // Populate the output with the new scalars
  int idx = outPD->AddArray(newScalars);
  outPD->SetActiveAttribute(idx, vtkDataSetAttributes::SCALARS);

  return 1;
}

//------------------------------------------------------------------------------
// Return an interval id
vtkIdType vtkThresholdScalars::AddInterval(double s0, double s1, int labelId)
{
  vtkIdType id = this->Intervals->AddInterval(s0, s1, labelId);
  this->Modified();
  return id;
}

//------------------------------------------------------------------------------
// Remove the interval specified by intervalId
void vtkThresholdScalars::RemoveInterval(vtkIdType intervalId)
{
  this->Intervals->RemoveInterval(intervalId);
  this->Modified();
}

//------------------------------------------------------------------------------
// Remove all intervals
void vtkThresholdScalars::RemoveAllIntervals()
{
  this->Intervals->RemoveAllIntervals();
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkThresholdScalars::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Number of Intervals: " << this->Intervals->GetNumberOfIntervals() << endl;
  os << indent << "Background Label: " << this->BackgroundLabel << endl;
}
VTK_ABI_NAMESPACE_END
