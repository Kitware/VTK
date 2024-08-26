// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkStaticCleanUnstructuredGrid.h"

#include "vtkArrayDispatch.h"
#include "vtkArrayListTemplate.h" // For processing attribute data
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDataArrayRange.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSMPTools.h"
#include "vtkStaticPointLocator.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnstructuredGrid.h"

#include <vector>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkStaticCleanUnstructuredGrid);

namespace
{ // anonymous

using PointUses = unsigned char;

// Helper functions to mark points used by cells taking into account
// the point merging information.
template <typename UType>
void MarkUses(vtkIdType numIds, UType* connArray, vtkIdType* mergeMap, PointUses* ptUses)
{
  for (auto i = 0; i < numIds; ++i)
  {
    ptUses[mergeMap[connArray->GetValue(i)]] = 1;
  }
}

//------------------------------------------------------------------------------
// Fast, threaded method to copy new points and attribute data to the output.
template <typename InArrayT, typename OutArrayT>
struct CopyPointsAlgorithm
{
  InArrayT* InPts;
  OutArrayT* OutPts;
  ArrayList Arrays;
  std::vector<vtkIdType> ReversePtMap;

  CopyPointsAlgorithm(vtkIdType* ptMap, InArrayT* inPts, vtkPointData* inPD, vtkIdType numNewPts,
    OutArrayT* outPts, vtkPointData* outPD)
    : InPts(inPts)
    , OutPts(outPts)
  {
    // Prepare for threaded copying
    this->Arrays.AddArrays(numNewPts, inPD, outPD, 0.0, /*promote=*/false);

    // Need to define a reverse point map (which maps the new/output points
    // to the input points from which they were merged). This could be
    // threaded for minimal performance gain - it's probably not worth it.
    this->ReversePtMap.resize(numNewPts);
    std::fill(this->ReversePtMap.begin(), this->ReversePtMap.end(), -1);

    // Since we are copying (not averaging), just find the first point from
    // the input which merged to the output point.
    vtkIdType numInPts = inPts->GetNumberOfTuples();
    for (auto inPtId = 0; inPtId < numInPts; ++inPtId)
    {
      if (ptMap[inPtId] != -1 && this->ReversePtMap[ptMap[inPtId]] == -1)
      {
        this->ReversePtMap[ptMap[inPtId]] = inPtId;
      }
    }
  }

  // Threaded copy point coordinates and attribute data
  void operator()(vtkIdType outPtId, vtkIdType endPtId)
  {
    using OutValueT = vtk::GetAPIType<OutArrayT>;
    const auto inPoints = vtk::DataArrayTupleRange<3>(this->InPts);
    auto outPoints = vtk::DataArrayTupleRange<3>(this->OutPts);

    // Loop over all new (output) points and copy data from the input
    for (; outPtId < endPtId; ++outPtId)
    {
      const vtkIdType inPtId = this->ReversePtMap[outPtId];
      const auto inP = inPoints[inPtId];
      auto outP = outPoints[outPtId];
      outP[0] = static_cast<OutValueT>(inP[0]);
      outP[1] = static_cast<OutValueT>(inP[1]);
      outP[2] = static_cast<OutValueT>(inP[2]);
      this->Arrays.Copy(inPtId, outPtId);
    }
  }
};

// Copy point data from input to output, taking into
// account point merging.
struct CopyPointsWorklet
{
  template <typename InArrayT, typename OutArrayT>
  void operator()(
    InArrayT* inPts, OutArrayT* outPts, vtkIdType* ptMap, vtkPointData* inPD, vtkPointData* outPD)
  {
    const vtkIdType numNewPts = outPts->GetNumberOfTuples();

    CopyPointsAlgorithm<InArrayT, OutArrayT> algo{ ptMap, inPts, inPD, numNewPts, outPts, outPD };
    vtkSMPTools::For(0, numNewPts, algo);
  }
};

using FastValueTypes = vtkArrayDispatch::Reals;
using Dispatcher = vtkArrayDispatch::Dispatch2ByValueType<FastValueTypes, FastValueTypes>;

//------------------------------------------------------------------------------
// Fast, threaded method to average the point coordinates and point attribute
// data that are merged to produce an output point.

// Count the number of time each output point is used by a merged input point.
// Being threaded, atomics are necessary to avoid data races.
struct CountUses
{
  vtkIdType* PtMap;
  std::atomic<vtkIdType>* Counts; // initialized to zero

  CountUses(vtkIdType* ptMap, std::atomic<vtkIdType>* counts)
    : PtMap(ptMap)
    , Counts(counts)
  {
  }

  void operator()(vtkIdType ptId, vtkIdType endPtId)
  {
    for (; ptId < endPtId; ++ptId)
    {
      if (this->PtMap[ptId] != -1)
      {
        this->Counts[this->PtMap[ptId]]++;
      }
    }
  }
};

// For each new/output point, create a list of all the input points
// that were merged to it.
struct InsertLinks
{
  vtkIdType* PtMap;
  std::atomic<vtkIdType>* Counts;
  vtkIdType* Links;
  vtkIdType* Offsets;

  InsertLinks(
    vtkIdType* ptMap, std::atomic<vtkIdType>* counts, vtkIdType* links, vtkIdType* offsets)
    : PtMap(ptMap)
    , Counts(counts)
    , Links(links)
    , Offsets(offsets)
  {
  }

  void operator()(vtkIdType ptId, vtkIdType endPtId)
  {
    std::atomic<vtkIdType>* counts = this->Counts;
    vtkIdType* links = this->Links;
    vtkIdType* offsets = this->Offsets;
    vtkIdType offset;

    for (; ptId < endPtId; ++ptId)
    {
      vtkIdType outPtId = this->PtMap[ptId];
      if (outPtId != -1)
      {
        offset = offsets[outPtId] + counts[outPtId].fetch_sub(1, std::memory_order_relaxed) - 1;
        links[offset] = ptId;
      }
    }
  }
};

// Actually do the work of averaging the point coordinates and point
// attribute data. The links are processed to average the input points merged
// to the new/output points.
template <typename InArrayT, typename OutArrayT>
struct AverageAlgorithm
{
  InArrayT* InPts;
  OutArrayT* OutPts;
  vtkIdType* Links;
  vtkIdType* Offsets;
  bool AverageCoords;
  ArrayList Arrays;

  AverageAlgorithm(InArrayT* inPts, vtkPointData* inPD, vtkIdType numNewPts, OutArrayT* outPts,
    vtkPointData* outPD, vtkIdType* links, vtkIdType* offsets, double tol)
    : InPts(inPts)
    , OutPts(outPts)
    , Links(links)
    , Offsets(offsets)
  {
    this->AverageCoords = (tol == 0.0 ? false : true);
    this->Arrays.AddArrays(numNewPts, inPD, outPD);
  }

  // Returns the list of input points merged to create an output point
  void GetMergedPoints(vtkIdType ptId, vtkIdType& num, const vtkIdType*& ids)
  {
    num = this->Offsets[ptId + 1] - this->Offsets[ptId];
    ids = this->Links + this->Offsets[ptId];
  }

  // The ptId is the id of a new/output point.
  void operator()(vtkIdType ptId, vtkIdType endPtId)
  {
    using OutValueT = vtk::GetAPIType<OutArrayT>;
    vtkIdType num;
    const vtkIdType* ids;
    const auto inPoints = vtk::DataArrayTupleRange<3>(this->InPts);
    auto outPoints = vtk::DataArrayTupleRange<3>(this->OutPts);

    // Loop over all output points
    for (; ptId < endPtId; ++ptId)
    {
      // Grab the list of merged input points. Depending on the
      // number of merged points, the attribute data can be copied
      // or averaged.
      this->GetMergedPoints(ptId, num, ids);
      if (num == 1)
      {
        this->Arrays.Copy(ids[0], ptId);
      }
      else
      {
        this->Arrays.Average(num, ids, ptId);
      }

      // If point coordinates don't need averaging (i.e., tolerance==0.0
      // or the number of merged points==1), then just copy the point.
      if (!this->AverageCoords || num == 1)
      {
        const auto inP = inPoints[ids[0]];
        auto outP = outPoints[ptId];
        outP[0] = static_cast<OutValueT>(inP[0]);
        outP[1] = static_cast<OutValueT>(inP[1]);
        outP[2] = static_cast<OutValueT>(inP[2]);
      }
      else // need to average the coordinates
      {
        double x[3] = { 0.0, 0.0, 0.0 };
        double n = static_cast<double>(num);
        for (auto i = 0; i < num; ++i)
        {
          const auto inP = inPoints[ids[i]];
          x[0] += static_cast<double>(inP[0]);
          x[1] += static_cast<double>(inP[1]);
          x[2] += static_cast<double>(inP[2]);
        }
        auto outP = outPoints[ptId];
        outP[0] = static_cast<OutValueT>(x[0] / n);
        outP[1] = static_cast<OutValueT>(x[1] / n);
        outP[2] = static_cast<OutValueT>(x[2] / n);
      } // averaging point coordinates
    }
  }
};

// Thread the averaging of point coordinates and attributes
struct AverageWorklet
{
  template <typename InArrayT, typename OutArrayT>
  void operator()(InArrayT* inPts, OutArrayT* outPts, vtkPointData* inPD, vtkPointData* outPD,
    vtkIdType* links, vtkIdType* offsets, double tol)
  {
    const vtkIdType numNewPts = outPts->GetNumberOfTuples();

    AverageAlgorithm<InArrayT, OutArrayT> algo(
      inPts, inPD, numNewPts, outPts, outPD, links, offsets, tol);
    vtkSMPTools::For(0, numNewPts, algo);
  }
};

//  Update the cell connectivity array.
void UpdateCellArrayConnectivity(vtkCellArray* ca, vtkIdType* ptMap)
{
  vtkIdType numConn = ca->GetNumberOfConnectivityIds();

  if (ca->IsStorage64Bit())
  {
    vtkTypeInt64* c = ca->GetConnectivityArray64()->GetPointer(0);
    vtkSMPTools::For(0, numConn, [&, c, ptMap](vtkIdType id, vtkIdType endId) {
      for (; id < endId; ++id)
      {
        c[id] = ptMap[c[id]];
      }
    }); // end lambda
  }
  else
  {
    vtkTypeInt32* c = ca->GetConnectivityArray32()->GetPointer(0);
    vtkSMPTools::For(0, numConn, [&, c, ptMap](vtkIdType id, vtkIdType endId) {
      for (; id < endId; ++id)
      {
        c[id] = ptMap[c[id]];
      }
    }); // end lambda
  }
}

//  Update the polyhedra face connectivity array.
void UpdatePolyhedraFaces(vtkCellArray* a, vtkIdType* ptMap)
{
  UpdateCellArrayConnectivity(a, ptMap);
}

} // anonymous namespace

//------------------------------------------------------------------------------
// Construct object with initial Tolerance of 0.0
vtkStaticCleanUnstructuredGrid::vtkStaticCleanUnstructuredGrid()
{
  this->ToleranceIsAbsolute = false;
  this->Tolerance = 0.0;
  this->AbsoluteTolerance = 0.0;

  this->MergingArray = nullptr;
  this->SetMergingArray("");

  this->RemoveUnusedPoints = true;
  this->ProduceMergeMap = false;
  this->AveragePointData = false;
  this->OutputPointsPrecision = vtkAlgorithm::DEFAULT_PRECISION;

  this->Locator = vtkSmartPointer<vtkStaticPointLocator>::New();

  this->PieceInvariant = true;
}

//------------------------------------------------------------------------------
int vtkStaticCleanUnstructuredGrid::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  if (this->PieceInvariant)
  {
    // Although piece > 1 is handled by superclass, we should be thorough.
    if (outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()) == 0)
    {
      inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), 0);
      inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), 1);
    }
    else
    {
      inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), 0);
      inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), 0);
    }
  }
  else
  {
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()));
  }

  return 1;
}

//------------------------------------------------------------------------------
int vtkStaticCleanUnstructuredGrid::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkUnstructuredGrid* input =
    vtkUnstructuredGrid::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkUnstructuredGrid* output =
    vtkUnstructuredGrid::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkPoints* inPts = input->GetPoints();
  vtkIdType numPts = input->GetNumberOfPoints();
  vtkIdType numCells = input->GetNumberOfCells();

  vtkDebugMacro(<< "Beginning unstructured grid clean");
  if ((numPts < 1) || (inPts == nullptr) || (numCells < 1))
  {
    vtkDebugMacro(<< "No data to operate on!");
    return 1;
  }

  vtkCellArray* inCells = input->GetCells();
  vtkPointData* inPD = input->GetPointData();
  vtkCellData* inCD = input->GetCellData();

  // The output cell data remains the same since the input cells are not
  // deleted nor reordered.
  output->GetCellData()->PassData(inCD);

  // Build the locator, this is needed for all execution paths.
  this->Locator->SetDataSet(input);
  this->Locator->BuildLocator();

  // Compute the tolerance
  double tol =
    (this->ToleranceIsAbsolute ? this->AbsoluteTolerance : this->Tolerance * input->GetLength());

  // Now merge the points to create a merge map.
  std::vector<vtkIdType> mergeMap(numPts);
  vtkDataArray* mergingData = nullptr;
  if (this->MergingArray)
  {
    if ((mergingData = inPD->GetArray(this->MergingArray)))
    {
      this->Locator->MergePointsWithData(mergingData, mergeMap.data());
    }
  }
  if (!mergingData)
  {
    this->Locator->MergePoints(tol, mergeMap.data());
  }

  // If removing unused points, traverse the connectivity array to mark the
  // points that are used by one or more cells.
  std::unique_ptr<PointUses[]> uPtUses; // reference counted to prevent leakage
  PointUses* ptUses = nullptr;
  if (this->RemoveUnusedPoints)
  {
    uPtUses = std::unique_ptr<PointUses[]>(new PointUses[numPts]);
    ptUses = uPtUses.get();
    std::fill_n(ptUses, numPts, 0);
    vtkStaticCleanUnstructuredGrid::MarkPointUses(inCells, mergeMap.data(), ptUses);
  }

  // Create a map that maps old point ids into new, renumbered point
  // ids.
  vtkNew<vtkIdTypeArray> ptMap;
  ptMap->SetNumberOfTuples(numPts);
  ptMap->SetName("PointMergeMap");
  vtkIdType* pmap = ptMap->GetPointer(0);
  if (this->ProduceMergeMap)
  {
    output->GetFieldData()->AddArray(ptMap);
  }

  // Build the map from old points to new points.
  vtkIdType numNewPts =
    vtkStaticCleanUnstructuredGrid::BuildPointMap(numPts, pmap, ptUses, mergeMap);

  // Create new points of the appropriate type
  vtkNew<vtkPoints> newPts;
  // Set the desired precision for the points in the output.
  if (this->OutputPointsPrecision == vtkAlgorithm::DEFAULT_PRECISION)
  {
    newPts->SetDataType(inPts->GetDataType());
  }
  else if (this->OutputPointsPrecision == vtkAlgorithm::SINGLE_PRECISION)
  {
    newPts->SetDataType(VTK_FLOAT);
  }
  else if (this->OutputPointsPrecision == vtkAlgorithm::DOUBLE_PRECISION)
  {
    newPts->SetDataType(VTK_DOUBLE);
  }
  newPts->SetNumberOfPoints(numNewPts);
  output->SetPoints(newPts);

  // Produce output points and associated point data. If point averaging is
  // requested, then point coordinates and point attribute values must be
  // combined - a relatively compute intensive process.
  vtkPointData* outPD = output->GetPointData();
  outPD->CopyAllocate(inPD);
  if (this->AveragePointData)
  {
    vtkStaticCleanUnstructuredGrid::AveragePoints(inPts, inPD, newPts, outPD, pmap, tol);
  }
  else
  {
    vtkStaticCleanUnstructuredGrid::CopyPoints(inPts, inPD, newPts, outPD, pmap);
  }

  // At this point, we need to construct the unstructured grid topology using
  // the point map. This means updating the connectivity arrays (including
  // possibly face connectivity for any polyhedra). Since the types of the
  // cells are not changing, offsets and type arrays do not need
  // modification.
  //
  // Update the cell connectivity using the point map.
  vtkNew<vtkCellArray> outCells;
  // The deep copy copies offsets connectivity as well as offsets. This
  // could be made more efficient by a combination of shallow copies, and
  // creating and copying into a new connectivity array.
  outCells->DeepCopy(inCells);
  UpdateCellArrayConnectivity(outCells, pmap);

  // If the unstructured grid contains polyhedra, the face connectivity needs
  // to be updated as well.
  vtkCellArray* faceLocations = input->GetPolyhedronFaceLocations();
  vtkCellArray* faces = input->GetPolyhedronFaces();
  if (faces != nullptr)
  {
    UpdatePolyhedraFaces(faces, pmap);
  }

  // Finally, assemble the filter output.
  output->SetPolyhedralCells(input->GetCellTypesArray(), outCells, faceLocations, faces);

  // Free unneeded memory
  this->Locator->Initialize();

  this->CheckAbort();

  return 1;
}

// The following static methods are used by outside classes such as
// vtkStaticCleanPolyData.

//------------------------------------------------------------------------------
// Helper function to dispatch to point marking function based on type
// of connectivity array storage.
void vtkStaticCleanUnstructuredGrid::MarkPointUses(
  vtkCellArray* ca, vtkIdType* mergeMap, PointUses* ptUses)
{
  vtkIdType numConn = ca->GetNumberOfConnectivityIds();
  if (ca->IsStorage64Bit())
  {
    vtkTypeInt64Array* conn = ca->GetConnectivityArray64();
    MarkUses<vtkTypeInt64Array>(numConn, conn, mergeMap, ptUses);
  }
  else
  {
    vtkTypeInt32Array* conn = ca->GetConnectivityArray32();
    MarkUses<vtkTypeInt32Array>(numConn, conn, mergeMap, ptUses);
  }
}

//------------------------------------------------------------------------------
// Build the final point map from input points to output points
vtkIdType vtkStaticCleanUnstructuredGrid::BuildPointMap(
  vtkIdType numPts, vtkIdType* pmap, unsigned char* ptUses, std::vector<vtkIdType>& mergeMap)
{
  // Count and map points to new points, taking into account
  // point uses (if requested).
  vtkIdType id, numNewPts = 0;
  // Perform a prefix sum to count the number of new points.
  std::fill_n(pmap, numPts, (-1));
  for (id = 0; id < numPts; ++id)
  {
    if (mergeMap[id] == id && (ptUses == nullptr || ptUses[id] != 0))
    {
      pmap[id] = numNewPts++;
    }
  }
  // Now map old merged points to new points
  for (id = 0; id < numPts; ++id)
  {
    if (mergeMap[id] != id)
    {
      pmap[id] = pmap[mergeMap[id]];
    }
  }
  return numNewPts;
}

//------------------------------------------------------------------------------
// Copy the input point coordinates and data attributes to the output merged points.
void vtkStaticCleanUnstructuredGrid::CopyPoints(
  vtkPoints* inPts, vtkPointData* inPD, vtkPoints* outPts, vtkPointData* outPD, vtkIdType* ptMap)
{
  vtkDataArray* inArray = inPts->GetData();
  vtkDataArray* outArray = outPts->GetData();

  CopyPointsWorklet worklet;
  if (!Dispatcher::Execute(inArray, outArray, worklet, ptMap, inPD, outPD))
  { // Fallback to slow path for unusual types:
    worklet(inArray, outArray, ptMap, inPD, outPD);
  }
}

//------------------------------------------------------------------------------
// Average the input points and attribute data to merged points. This
// requires counting the input points that are to be averaged to produce the
// output point coordinates and attributes. Also create a data structure
// (i.e., links, offsets), that for each output point, lists the input points
// that have been merged to it. Note that if the merge tolerance==0.0, then
// there is no need to average the point coordinates.
void vtkStaticCleanUnstructuredGrid::AveragePoints(vtkPoints* inPts, vtkPointData* inPD,
  vtkPoints* outPts, vtkPointData* outPD, vtkIdType* ptMap, double tol)
{
  vtkDataArray* inArray = inPts->GetData();
  vtkDataArray* outArray = outPts->GetData();

  // Basic information about the points
  vtkIdType numInPts = inPts->GetNumberOfPoints();
  vtkIdType numOutPts = outPts->GetNumberOfPoints();

  // Create an array of atomics with initial count=0. This will keep
  // track of point merges. Count them in parallel.
  std::unique_ptr<std::atomic<vtkIdType>[]> uCounts(new std::atomic<vtkIdType>[numOutPts]());
  std::atomic<vtkIdType>* counts = uCounts.get();
  CountUses count(ptMap, counts);
  vtkSMPTools::For(0, numInPts, count);

  // Perform a prefix sum to determine the offsets.
  vtkIdType ptId, npts;
  std::unique_ptr<vtkIdType[]> uOffsets(new vtkIdType[numOutPts + 1]); // extra +1 for convenience
  vtkIdType* offsets = uOffsets.get();
  offsets[0] = 0;
  for (ptId = 1; ptId <= numOutPts; ++ptId)
  {
    npts = counts[ptId - 1];
    offsets[ptId] = offsets[ptId - 1] + npts;
  }

  // Configure the "links" which are, for each output point, lists
  // the input points merged to that output point. The offsets point into
  // the links.
  std::unique_ptr<vtkIdType[]> uLinks(new vtkIdType[offsets[numOutPts]]);
  vtkIdType* links = uLinks.get();

  // Now insert cell ids into cell links.
  InsertLinks insertLinks(ptMap, counts, links, offsets);
  vtkSMPTools::For(0, numInPts, insertLinks);

  // Okay, now we can actually average the point coordinates and
  // point attribute data.
  AverageWorklet average;
  if (!Dispatcher::Execute(inArray, outArray, average, inPD, outPD, links, offsets, tol))
  { // Fallback to slow path for unusual types:
    average(inArray, outArray, inPD, outPD, links, offsets, tol);
  }
}

//------------------------------------------------------------------------------
vtkMTimeType vtkStaticCleanUnstructuredGrid::GetMTime()
{
  vtkMTimeType mTime = this->vtkObject::GetMTime();
  vtkMTimeType time = this->Locator->GetMTime();
  return (time > mTime ? time : mTime);
}

//------------------------------------------------------------------------------
void vtkStaticCleanUnstructuredGrid::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Tolerance Is Absolute: " << (this->ToleranceIsAbsolute ? "On\n" : "Off\n");
  os << indent << "Tolerance: " << (this->Tolerance ? "On\n" : "Off\n");
  os << indent << "Absolute Tolerance: " << (this->AbsoluteTolerance ? "On\n" : "Off\n");

  if (this->MergingArray)
  {
    os << indent << "Merging Array: " << this->MergingArray << "\n";
  }
  else
  {
    os << indent << "Merging Array: (none)\n";
  }

  if (this->Locator)
  {
    os << indent << "Locator: " << this->Locator << "\n";
  }
  else
  {
    os << indent << "Locator: (none)\n";
  }

  os << indent << "Remove Unused Points: " << (this->RemoveUnusedPoints ? "On\n" : "Off\n");
  os << indent << "Produce Merge Map: " << (this->ProduceMergeMap ? "On\n" : "Off\n");
  os << indent << "Average Point Data: " << (this->AveragePointData ? "On\n" : "Off\n");
  os << indent << "Output Points Precision: " << this->OutputPointsPrecision << "\n";
}
VTK_ABI_NAMESPACE_END
