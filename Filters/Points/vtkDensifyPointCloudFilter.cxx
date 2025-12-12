// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDensifyPointCloudFilter.h"

#include "vtkArrayDispatch.h"
#include "vtkArrayDispatchDataSetArrayList.h"
#include "vtkArrayListTemplate.h" // For processing attribute data
#include "vtkDataArrayRange.h"
#include "vtkFloatArray.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkPoints.h"
#include "vtkSMPThreadLocalObject.h"
#include "vtkSMPTools.h"
#include "vtkStaticPointLocator.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkDensifyPointCloudFilter);

//------------------------------------------------------------------------------
// Helper classes to support efficient computing, and threaded execution.
namespace
{

//------------------------------------------------------------------------------
// Count the number of points that need generation
template <typename TArray>
struct CountPointsFunctor
{
  TArray* InPoints;
  vtkStaticPointLocator* Locator;
  vtkIdType* Count;
  int NeighborhoodType;
  int NClosest;
  double Radius;
  double Distance;

  // Don't want to allocate working arrays on every thread invocation. Thread local
  // storage prevents lots of new/delete.
  vtkSMPThreadLocalObject<vtkIdList> PIds;

  CountPointsFunctor(TArray* inPts, vtkStaticPointLocator* loc, vtkIdType* count, int ntype,
    int nclose, double r, double d)
    : InPoints(inPts)
    , Locator(loc)
    , Count(count)
    , NeighborhoodType(ntype)
    , NClosest(nclose)
    , Radius(r)
    , Distance(d)
  {
  }

  // Just allocate a little bit of memory to get started.
  void Initialize()
  {
    vtkIdList*& pIds = this->PIds.Local();
    pIds->Allocate(128); // allocate some memory
  }

  void operator()(vtkIdType pointId, vtkIdType endPointId)
  {
    auto points = vtk::DataArrayTupleRange<3>(this->InPoints);
    auto p = points.begin() + pointId;
    vtkStaticPointLocator* loc = this->Locator;
    vtkIdType* count = this->Count + pointId;
    vtkIdList*& pIds = this->PIds.Local();
    vtkIdType i, id, numIds, numNewPts;
    double px[3], py[3];
    int ntype = this->NeighborhoodType;
    double radius = this->Radius;
    int nclose = this->NClosest;
    double d2 = this->Distance * this->Distance;

    for (; pointId < endPointId; ++pointId, ++p)
    {
      numNewPts = 0;
      p->GetTuple(px);
      if (ntype == vtkDensifyPointCloudFilter::N_CLOSEST)
      {
        // use nclose+1 because we want to discount ourselves
        loc->FindClosestNPoints(nclose + 1, px, pIds);
      }
      else // ntype == vtkDensifyPointCloudFilter::RADIUS
      {
        loc->FindPointsWithinRadius(radius, px, pIds);
      }
      numIds = pIds->GetNumberOfIds();

      for (i = 0; i < numIds; ++i)
      {
        id = pIds->GetId(i);
        if (id > pointId) // only process points of larger id
        {
          points.GetTuple(id, py);

          if (vtkMath::Distance2BetweenPoints(px, py) >= d2)
          {
            numNewPts++;
          }
        } // larger id
      }   // for all neighbors
      *count++ = numNewPts;
    } // for all points in this batch
  }

  void Reduce() {}
}; // CountPoints

struct CountPointsWorker
{
  template <class TArray>
  void operator()(TArray* pts, vtkStaticPointLocator* loc, vtkIdType* count, int ntype, int nclose,
    double r, double d)
  {
    CountPointsFunctor<TArray> counter(pts, loc, count, ntype, nclose, r, d);
    vtkSMPTools::For(0, pts->GetNumberOfTuples(), counter);
  }
};

//------------------------------------------------------------------------------
// Count the number of points that need generation
template <typename TArray>
struct GeneratePointsFunctor
{
  TArray* OutPoints;
  vtkStaticPointLocator* Locator;
  const vtkIdType* Offsets;
  int NeighborhoodType;
  int NClosest;
  double Radius;
  double Distance;
  ArrayList Arrays;

  using T = vtk::GetAPIType<TArray>;
  // Don't want to allocate working arrays on every thread invocation. Thread local
  // storage prevents lots of new/delete.
  vtkSMPThreadLocalObject<vtkIdList> PIds;

  GeneratePointsFunctor(TArray* outPts, vtkStaticPointLocator* loc, const vtkIdType* offset,
    int ntype, int nclose, double r, double d, vtkPointData* attr)
    : OutPoints(outPts)
    , Locator(loc)
    , Offsets(offset)
    , NeighborhoodType(ntype)
    , NClosest(nclose)
    , Radius(r)
    , Distance(d)
  {
    this->Arrays.AddSelfInterpolatingArrays(outPts->GetNumberOfTuples(), attr);
  }

  // Just allocate a little bit of memory to get started.
  void Initialize()
  {
    vtkIdList*& pIds = this->PIds.Local();
    pIds->Allocate(128); // allocate some memory
  }

  void operator()(vtkIdType pointId, vtkIdType endPointId)
  {
    auto points = vtk::DataArrayTupleRange<3>(this->OutPoints);
    auto p = points.begin() + pointId;
    vtkStaticPointLocator* loc = this->Locator;
    vtkIdList*& pIds = this->PIds.Local();
    vtkIdType i, id, numIds;
    vtkIdType outPtId = this->Offsets[pointId];
    double px[3], py[3];
    int ntype = this->NeighborhoodType;
    double radius = this->Radius;
    int nclose = this->NClosest;
    double d2 = this->Distance * this->Distance;

    for (; pointId < endPointId; ++pointId, ++p)
    {
      p->GetTuple(px);
      if (ntype == vtkDensifyPointCloudFilter::N_CLOSEST)
      {
        // use nclose+1 because we want to discount ourselves
        loc->FindClosestNPoints(nclose + 1, px, pIds);
      }
      else // ntype == vtkDensifyPointCloudFilter::RADIUS
      {
        loc->FindPointsWithinRadius(radius, px, pIds);
      }
      numIds = pIds->GetNumberOfIds();

      for (i = 0; i < numIds; ++i)
      {
        id = pIds->GetId(i);
        if (id > pointId) // only process points of larger id
        {
          points.GetTuple(id, py);

          if (vtkMath::Distance2BetweenPoints(px, py) >= d2)
          {
            auto newX = points[outPtId];
            newX[0] = static_cast<T>(0.5 * (px[0] + py[0]));
            newX[1] = static_cast<T>(0.5 * (px[1] + py[1]));
            newX[2] = static_cast<T>(0.5 * (px[2] + py[2]));
            this->Arrays.InterpolateEdge(pointId, id, 0.5, outPtId);
            outPtId++;
          }
        } // larger id
      }   // for all neighbor points
    }     // for all points in this batch
  }

  void Reduce() {}
}; // GeneratePoints

struct GeneratePointsWorker
{
  template <class TArray>
  void operator()(TArray* pts, vtkIdType numInPts, vtkStaticPointLocator* loc, vtkIdType* offsets,
    int ntype, int nclose, double r, double d, vtkPointData* pd)
  {
    GeneratePointsFunctor<TArray> generator(pts, loc, offsets, ntype, nclose, r, d, pd);
    vtkSMPTools::For(0, numInPts, generator);
  }
};

} // anonymous namespace

//================= Begin VTK class proper =======================================
//------------------------------------------------------------------------------
vtkDensifyPointCloudFilter::vtkDensifyPointCloudFilter()
{

  this->NeighborhoodType = vtkDensifyPointCloudFilter::N_CLOSEST;
  this->Radius = 1.0;
  this->NumberOfClosestPoints = 6;
  this->TargetDistance = 0.5;
  this->MaximumNumberOfIterations = 3;
  this->InterpolateAttributeData = true;
  this->MaximumNumberOfPoints = VTK_ID_MAX;
}

//------------------------------------------------------------------------------
vtkDensifyPointCloudFilter::~vtkDensifyPointCloudFilter() = default;

//------------------------------------------------------------------------------
// Produce the output data
int vtkDensifyPointCloudFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPointSet* input = vtkPointSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Check the input
  if (!input || !output)
  {
    return 1;
  }
  vtkIdType numPts = input->GetNumberOfPoints();
  if (numPts < 1)
  {
    return 1;
  }

  // Start by building the locator, creating the output points and otherwise
  // and prepare for iteration.
  int iterNum;
  vtkStaticPointLocator* locator = vtkStaticPointLocator::New();

  vtkPoints* inPts = input->GetPoints();
  vtkPoints* newPts = inPts->NewInstance();
  newPts->DeepCopy(inPts);
  output->SetPoints(newPts);
  vtkPointData* outPD = nullptr;
  if (this->InterpolateAttributeData)
  {
    outPD = output->GetPointData();
    outPD->DeepCopy(input->GetPointData());
    outPD->InterpolateAllocate(outPD, numPts);
  }

  vtkIdType ptId, numInPts, numNewPts;
  vtkIdType npts, offset, *offsets;
  double d = this->TargetDistance;

  // Loop over the data, bisecting connecting edges as required.
  for (iterNum = 0; iterNum < this->MaximumNumberOfIterations; ++iterNum)
  {
    // Prepare to process
    locator->SetDataSet(output);
    locator->Modified();
    locator->BuildLocator();

    // Count the number of points to create
    numInPts = output->GetNumberOfPoints();
    offsets = new vtkIdType[numInPts];
    CountPointsWorker countWorker;
    if (!vtkArrayDispatch::DispatchByArray<vtkArrayDispatch::PointArrays>::Execute(
          output->GetPoints()->GetData(), countWorker, locator, offsets, this->NeighborhoodType,
          this->NumberOfClosestPoints, this->Radius, d))
    {
      countWorker(output->GetPoints()->GetData(), locator, offsets, this->NeighborhoodType,
        this->NumberOfClosestPoints, this->Radius, d);
    }

    // Prefix sum to count the number of points created and build offsets
    offset = numInPts;
    for (ptId = 0; ptId < numInPts; ++ptId)
    {
      npts = offsets[ptId];
      offsets[ptId] = offset;
      offset += npts;
    }
    numNewPts = offset - numInPts;

    // Check convergence
    if (numNewPts == 0 || offset > this->MaximumNumberOfPoints)
    {
      delete[] offsets;
      break;
    }

    // Now add points and attribute data if requested. Allocate memory
    // for points and attributes.
    newPts->InsertPoint(offset, 0.0, 0.0, 0.0); // side effect reallocs memory

    GeneratePointsWorker genWorker;
    if (!vtkArrayDispatch::DispatchByArray<vtkArrayDispatch::PointArrays>::Execute(
          output->GetPoints()->GetData(), genWorker, numInPts, locator, offsets,
          this->NeighborhoodType, this->NumberOfClosestPoints, this->Radius, d, outPD))
    {
      genWorker(output->GetPoints()->GetData(), numInPts, locator, offsets, this->NeighborhoodType,
        this->NumberOfClosestPoints, this->Radius, d, outPD);
    }

    delete[] offsets;
  } // while max num of iterations not exceeded

  // Clean up
  locator->Delete();
  newPts->Delete();

  return 1;
}

//------------------------------------------------------------------------------
int vtkDensifyPointCloudFilter::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPointSet");
  return 1;
}

//------------------------------------------------------------------------------
void vtkDensifyPointCloudFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Neighborhood Type: " << this->GetNeighborhoodType() << "\n";
  os << indent << "Radius: " << this->Radius << "\n";
  os << indent << "Number Of Closest Points: " << this->NumberOfClosestPoints << "\n";
  os << indent << "Target Distance: " << this->TargetDistance << endl;
  os << indent << "Maximum Number of Iterations: " << this->MaximumNumberOfIterations << "\n";
  os << indent
     << "Interpolate Attribute Data: " << (this->InterpolateAttributeData ? "On\n" : "Off\n");
  os << indent << "Maximum Number Of Points: " << this->MaximumNumberOfPoints << "\n";
}
VTK_ABI_NAMESPACE_END
