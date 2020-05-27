/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRemoveUnusedPoints.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkRemoveUnusedPoints.h"

#include "vtkArrayDispatch.h"
#include "vtkCellData.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkTypeInt32Array.h"
#include "vtkTypeInt64Array.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

#include <algorithm>
#include <memory>
#include <numeric>
#include <vector>

namespace
{
struct RemapPointIds
{
  vtkDataArray* Output;
  const std::vector<vtkIdType>& PointMap;
  template <typename ArrayT>
  void operator()(const ArrayT* input)
  {
    using ValueType = typename ArrayT::ValueType;
    auto output = vtkArrayDownCast<ArrayT>(this->Output);
    const int numComps = input->GetNumberOfComponents();
    std::unique_ptr<ValueType[]> tuple{ new ValueType[numComps] };
    for (vtkIdType cc = 0, max = input->GetNumberOfTuples(); cc < max; ++cc)
    {
      input->GetTypedTuple(cc, tuple.get());
      std::transform(tuple.get(), tuple.get() + numComps, tuple.get(),
        [this](vtkIdType id) { return this->PointMap[id]; });
      output->SetTypedTuple(cc, tuple.get());
    }
  }
};

// Copies cell connectivity and other related information from input to output
// while mapping point ids using the pointMap.
bool CopyConnectivity(
  vtkUnstructuredGrid* input, vtkUnstructuredGrid* output, const std::vector<vtkIdType>& pointMap)
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

  RemapPointIds worker{ outConnectivity, pointMap };
  using SupportedArrays = vtkCellArray::StorageArrayList;
  using Dispatch = vtkArrayDispatch::DispatchByArray<SupportedArrays>;
  if (!Dispatch::Execute(inConnectivity, worker))
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
    worker.Output = outFaces;
    using DispatchFaces = vtkArrayDispatch::DispatchByArray<SupportedFacesArrays>;
    if (!DispatchFaces::Execute(inFaces, worker))
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
                                           << "'. "
                                              "Data maybe corrupt or incorrect.");
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

  if (!::CopyConnectivity(input, output, pointMap))
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
