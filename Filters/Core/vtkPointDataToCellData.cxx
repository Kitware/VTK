/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointDataToCellData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPointDataToCellData.h"

#include <algorithm>
#include <cassert>
#include <limits>
#include <vector>

#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"

#define VTK_EPSILON 1.e-6

namespace
{
class Histogram
{
public:
  struct Bin
  {
    // A histogram bin is comprised of the following:
    // index: the point index associated with the bin
    // count: the number of elements in the bin
    // value: the point data value associated with the bin
    Bin(vtkIdType index, vtkIdType count, double value) :
      Index(index), Count(count), Value(value) {}

    friend bool operator<(const Bin &b1, const Bin &b2)
    { return b1.Value < b2.Value; }

    bool Assigned() const { return this->Index != -1; }

    vtkIdType Index;
    vtkIdType Count;
    double Value;
  };

  typedef std::vector<Bin> HistogramBins;
  typedef HistogramBins::iterator BinIt;

  Histogram(vtkIdType size)
  {
    // Construct the array of bins.
    this->Bins.assign(size + 1, this->Init);
  }

  // Reset the fields of the bins in the histogram.
  void Reset(vtkIdType size)
  {
    for (vtkIdType i=0; i < size + 1; i++)
    {
      this->Bins[i] = this->Init;
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

Histogram::Bin Histogram::Init(-1, 1, std::numeric_limits<double>::max());

bool BinCountCmp(const Histogram::Bin &b1, const Histogram::Bin &b2)
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
    if (fabs((*it1).Value - (*it2).Value) < VTK_EPSILON)
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

}


vtkStandardNewMacro(vtkPointDataToCellData);

//----------------------------------------------------------------------------
// Instantiate object so that point data is not passed to output.
vtkPointDataToCellData::vtkPointDataToCellData()
{
  this->PassPointData = 0;
  this->CategoricalData = 0;
}

//----------------------------------------------------------------------------
int vtkPointDataToCellData::RequestData(
  vtkInformation*,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkInformation* info = outputVector->GetInformationObject(0);
  vtkDataSet *output = vtkDataSet::SafeDownCast(
    info->Get(vtkDataObject::DATA_OBJECT()));

  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkIdType cellId, ptId, pointId;
  vtkIdType numCells, numPts;
  vtkPointData *inPD=input->GetPointData();
  vtkCellData *outCD=output->GetCellData();
  int maxCellSize=input->GetMaxCellSize();
  vtkIdList *cellPts;
  double weight;
  double *weights;

  vtkDebugMacro(<<"Mapping point data to cell data");

  // First, copy the input to the output as a starting point
  output->CopyStructure( input );

  if ( (numCells=input->GetNumberOfCells()) < 1 )
  {
    vtkDebugMacro(<<"No input cells!");
    return 1;
  }
  weights=new double[maxCellSize];

  Histogram hist(maxCellSize);

  if (this->CategoricalData == 1)
  {
    // If the categorical data flag is enabled, then a) there must be scalars
    // to treat as categorical data, and b) the scalars must have one component.
    if (!input->GetPointData()->GetScalars())
    {
      vtkDebugMacro(<<"No input scalars!");
      delete [] weights;
      return 1;
    }
    if (input->GetPointData()->GetScalars()->GetNumberOfComponents() != 1)
    {
      vtkDebugMacro(<<"Input scalars have more than one component! Cannot categorize!");
      delete [] weights;
      return 1;
    }
  }

  cellPts = vtkIdList::New();
  cellPts->Allocate(maxCellSize);

  // Pass the cell data first. The fields and attributes
  // which also exist in the point data of the input will
  // be over-written during CopyAllocate
  output->GetCellData()->CopyGlobalIdsOff();
  output->GetCellData()->PassData(input->GetCellData());
  output->GetCellData()->CopyFieldOff(vtkDataSetAttributes::GhostArrayName());

  // notice that inPD and outCD are vtkPointData and vtkCellData; respectively.
  // It's weird, but it works.
  outCD->InterpolateAllocate(inPD,numCells);

  int abort=0;
  vtkIdType progressInterval=numCells/20 + 1;
  for (cellId=0; cellId < numCells && !abort; cellId++)
  {
    if ( !(cellId % progressInterval) )
    {
      this->UpdateProgress((double)cellId/numCells);
      abort = GetAbortExecute();
    }

    input->GetCellPoints(cellId, cellPts);
    numPts = cellPts->GetNumberOfIds();

    if (numPts == 0)
    {
      continue;
    }

    // If we aren't dealing with categorical data...
    if (!(this->CategoricalData))
    {
      // ...then we simply provide each point with an equal weight value and
      // interpolate.
      weight = 1.0 / numPts;
      for (ptId=0; ptId < numPts; ptId++)
      {
        weights[ptId] = weight;
      }
      outCD->InterpolatePoint(inPD, cellId, cellPts, weights);
    }
    else
    {
      // ...otherwise, we populate a histogram from the scalar values at each
      // point, and then select the bin with the most elements.
      hist.Reset(numPts);
      for (ptId=0; ptId < numPts; ptId++)
      {
        pointId = cellPts->GetId(ptId);
        hist.Fill(pointId,
                  input->GetPointData()->GetScalars()->GetTuple1(pointId));
      }

      outCD->CopyData(inPD, hist.IndexOfLargestBin(), cellId);
    }
  }

  if ( !this->PassPointData )
  {
    output->GetPointData()->CopyAllOff();
    output->GetPointData()->CopyFieldOn(vtkDataSetAttributes::GhostArrayName());
  }
  output->GetPointData()->PassData(input->GetPointData());

  cellPts->Delete();
  delete [] weights;

  return 1;
}

//----------------------------------------------------------------------------
void vtkPointDataToCellData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Pass Point Data: " << (this->PassPointData ? "On\n" : "Off\n");
}
