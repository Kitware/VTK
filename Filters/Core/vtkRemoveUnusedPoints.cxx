// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkRemoveUnusedPoints.h"

#include "vtkArrayDispatch.h"
#include "vtkCellData.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkSMPTools.h"
#include "vtkTypeInt32Array.h"
#include "vtkTypeInt64Array.h"
#include "vtkUnstructuredGrid.h"

#include <algorithm>
#include <memory>
#include <numeric>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
namespace
{
template <typename ArrayT>
class RemapPointIdsFunctor
{
private:
  ArrayT* Input;
  ArrayT* Output;
  const std::vector<vtkIdType>& PointMap;

  using ValueType = typename ArrayT::ValueType;
  vtkSMPThreadLocal<std::vector<ValueType>> Tuple;
  vtkRemoveUnusedPoints* Filter;

public:
  RemapPointIdsFunctor(ArrayT* input, vtkDataArray* output, const std::vector<vtkIdType>& pointMap,
    vtkRemoveUnusedPoints* filter)
    : Input(input)
    , Output(vtkArrayDownCast<ArrayT>(output))
    , PointMap(pointMap)
    , Filter(filter)
  {
  }

  void Initialize()
  {
    auto& tuple = this->Tuple.Local();
    tuple.resize(this->Input->GetNumberOfComponents());
  }

  void operator()(vtkIdType begin, vtkIdType end)
  {
    auto& tuple = this->Tuple.Local();
    bool isFirst = vtkSMPTools::GetSingleThread();
    vtkIdType checkAbortInterval = std::min((end - begin) / 10 + 1, (vtkIdType)1000);
    for (vtkIdType cc = begin; cc < end; ++cc)
    {
      if (cc % checkAbortInterval == 0)
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

      this->Input->GetTypedTuple(cc, tuple.data());
      std::transform(tuple.begin(), tuple.end(), tuple.begin(),
        [this](vtkIdType id) { return this->PointMap[id]; });
      this->Output->SetTypedTuple(cc, tuple.data());
    }
  }

  void Reduce() {}
};

struct RemapPointIdsWorker
{
  template <typename ArrayT>
  void operator()(ArrayT* input, vtkDataArray* output, const std::vector<vtkIdType>& pointMap,
    vtkRemoveUnusedPoints* filter)
  {
    RemapPointIdsFunctor<ArrayT> functor(input, output, pointMap, filter);
    vtkSMPTools::For(0, input->GetNumberOfTuples(), functor);
  }
};

// Copies cell connectivity and other related information from input to output
// while mapping point ids using the pointMap.
bool CopyConnectivity(vtkUnstructuredGrid* input, vtkUnstructuredGrid* output,
  const std::vector<vtkIdType>& pointMap, vtkRemoveUnusedPoints* filter)
{
  auto inCellArray = input->GetCells();
  auto inConnectivity = inCellArray->GetConnectivityArray();
  auto inOffsets = inCellArray->GetOffsetsArray();
  auto inFaces = input->GetFaces();
  auto inFaceLocations = input->GetFaceLocations();

  vtkSmartPointer<vtkDataArray> outConnectivity;
  outConnectivity.TakeReference(inConnectivity->NewInstance());
  outConnectivity->SetNumberOfComponents(inConnectivity->GetNumberOfComponents());
  outConnectivity->SetNumberOfTuples(inConnectivity->GetNumberOfTuples());

  RemapPointIdsWorker worker;
  using SupportedArrays = vtkCellArray::StorageArrayList;
  using Dispatch = vtkArrayDispatch::DispatchByArray<SupportedArrays>;
  if (!Dispatch::Execute(inConnectivity, worker, outConnectivity, pointMap, filter))
  {
    return false;
  }

  vtkSmartPointer<vtkIdTypeArray> outFaces;
  if (inFaces)
  {
    using SupportedFacesArrays = vtkTypeList::Create<vtkIdTypeArray>;
    outFaces.TakeReference(vtkIdTypeArray::New());
    outFaces->SetNumberOfComponents(inFaces->GetNumberOfComponents());
    outFaces->SetNumberOfTuples(inFaces->GetNumberOfTuples());
    using DispatchFaces = vtkArrayDispatch::DispatchByArray<SupportedFacesArrays>;
    if (!DispatchFaces::Execute(inFaces, worker, outFaces, pointMap, filter))
    {
      return false;
    }
  }

  vtkNew<vtkCellArray> outCellArray;
  outCellArray->SetData(inOffsets, outConnectivity);
  output->SetCells(input->GetCellTypesArray(), outCellArray, inFaceLocations, outFaces);
  return true;
}
}

vtkStandardNewMacro(vtkRemoveUnusedPoints);
//----------------------------------------------------------------------------
vtkRemoveUnusedPoints::vtkRemoveUnusedPoints()
  : GenerateOriginalPointIds(true)
  , OriginalPointIdsArrayName(nullptr)
{
  this->SetOriginalPointIdsArrayName("vtkOriginalPointIds");
}

//----------------------------------------------------------------------------
vtkRemoveUnusedPoints::~vtkRemoveUnusedPoints()
{
  this->SetOriginalPointIdsArrayName(nullptr);
}

//----------------------------------------------------------------------------
int vtkRemoveUnusedPoints::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  auto input = vtkUnstructuredGrid::GetData(inputVector[0], 0);
  auto output = vtkUnstructuredGrid::GetData(outputVector, 0);
  const auto numPoints = input->GetNumberOfPoints();
  const auto numCells = input->GetNumberOfCells();
  output->GetPointData()->CopyAllOn();
  output->GetCellData()->CopyAllOn();

  if (numPoints == 0)
  {
    output->ShallowCopy(input);
    return 1;
  }

  if (numCells == 0)
  {
    output->CopyStructure(input);
    output->GetCellData()->ShallowCopy(input->GetCellData());
    output->GetPointData()->CopyAllocate(input->GetPointData(), 0);

    vtkNew<vtkPoints> pts;
    pts->SetDataType(input->GetPoints()->GetDataType());
    pts->SetNumberOfPoints(0);
    output->SetPoints(pts);
    return 1;
  }

  vtkNew<vtkIdList> originalIds;
  originalIds->Allocate(numPoints);

  std::vector<vtkIdType> pointMap(numPoints, -1);
  vtkIdType nextPtId = 0;
  for (vtkIdType cellId = 0; cellId < numCells; ++cellId)
  {
    vtkIdType npts;
    vtkIdType const* pts;
    input->GetCellPoints(cellId, npts, pts);
    for (vtkIdType ptIdx = 0; ptIdx < npts; ++ptIdx)
    {
      const auto oldid = pts[ptIdx];
      if (oldid < 0 || oldid >= numPoints)
      {
        vtkErrorMacro("Invalid point id '" << oldid << "' in cell '" << cellId
                                           << "'. Data maybe corrupt or incorrect.");
        output->Initialize();
        return 0;
      }
      auto& newid = pointMap[oldid];
      if (newid == -1)
      {
        newid = nextPtId++;
        originalIds->InsertId(newid, oldid);
      }
    }
  }

  if (nextPtId == numPoints)
  {
    // if all point are used, then skip extracting points.
    output->ShallowCopy(input);
    if (this->GenerateOriginalPointIds)
    {
      // reset original ids to be 0..n
      std::iota(originalIds->GetPointer(0), originalIds->GetPointer(0) + numPoints, 0);
      vtkNew<vtkIdTypeArray> opids;
      opids->SetName(this->OriginalPointIdsArrayName);
      opids->SetArray(
        originalIds->Release(), nextPtId, /*save=*/0, vtkIdTypeArray::VTK_DATA_ARRAY_DELETE);
      output->GetPointData()->AddArray(opids);
    }
    return 1;
  }

  if (!::CopyConnectivity(input, output, pointMap, this))
  {
    vtkErrorMacro("Error copy connectivity!");
    return 0;
  }

  // release extra memory
  originalIds->Squeeze();
  pointMap.clear();

  // copy cell data.
  output->GetCellData()->ShallowCopy(input->GetCellData());

  vtkNew<vtkPoints> pts;
  pts->SetDataType(input->GetPoints()->GetDataType());
  pts->SetNumberOfPoints(nextPtId);
  output->SetPoints(pts);

  // copy points.
  input->GetPoints()->GetData()->GetTuples(originalIds, pts->GetData());
  output->GetPointData()->CopyAllocate(input->GetPointData(), nextPtId);
  output->GetPointData()->SetNumberOfTuples(nextPtId);

  // copy point data.
  vtkNew<vtkIdList> destIds;
  destIds->SetNumberOfIds(nextPtId);
  std::iota(destIds->GetPointer(0), destIds->GetPointer(0) + nextPtId, 0);
  output->GetPointData()->CopyData(input->GetPointData(), originalIds, destIds);

  if (this->GenerateOriginalPointIds)
  {
    vtkNew<vtkIdTypeArray> opids;
    opids->SetName(this->OriginalPointIdsArrayName);
    opids->SetArray(
      originalIds->Release(), nextPtId, /*save=*/0, vtkIdTypeArray::VTK_DATA_ARRAY_DELETE);
    output->GetPointData()->AddArray(opids);
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkRemoveUnusedPoints::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "GenerateOriginalPointIds: " << this->GenerateOriginalPointIds << endl;
  os << indent << "OriginalPointIdsArrayName: "
     << (this->OriginalPointIdsArrayName ? this->OriginalPointIdsArrayName : "(null)") << endl;
}
VTK_ABI_NAMESPACE_END
