// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkStreaklineFilter.h"
#include "vtkCellArray.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"

#include <vector>

VTK_ABI_NAMESPACE_BEGIN

namespace
{
//------------------------------------------------------------------------------
template <class ArrayT>
void FillCellArrays(
  const std::vector<std::vector<vtkIdType>>& streaks, vtkCellArray* verts, vtkCellArray* lines)
{
  vtkNew<ArrayT> vertsConnectivity, vertsOffsets, linesConnectivity, linesOffsets;
  vertsOffsets->InsertNextValue(0);
  linesOffsets->InsertNextValue(0);
  vtkIdType nverts = 0, nlines = 0;

  for (const auto& streak : streaks)
  {
    auto insertNextCell = [&streak](ArrayT* connectivity, ArrayT* offsets, vtkIdType& n) {
      for (vtkIdType pointId : streak)
      {
        connectivity->InsertNextValue(pointId);
      }
      n += streak.size();
      offsets->InsertNextValue(n);
    };

    if (streak.size() == 1)
    {
      insertNextCell(vertsConnectivity, vertsOffsets, nverts);
    }
    else
    {
      insertNextCell(linesConnectivity, linesOffsets, nlines);
    }
  }

  verts->SetData(vertsOffsets, vertsConnectivity);
  lines->SetData(linesOffsets, linesConnectivity);
}
} // anonymous namespace

vtkObjectFactoryNewMacro(vtkStreaklineFilter);

//------------------------------------------------------------------------------
int vtkStreaklineFilter::Initialize(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  int retVal = this->Superclass::Initialize(request, inputVector, outputVector);
  this->ForceReinjectionEveryNSteps = 1;
  return retVal;
}

//------------------------------------------------------------------------------
int vtkStreaklineFilter::Finalize(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  int retVal = this->Superclass::Finalize(request, inputVector, outputVector);

  vtkNew<vtkPointData> pd;
  vtkNew<vtkPoints> points;

  pd->CopyAllocate(this->OutputPointData, this->OutputPointData->GetNumberOfTuples());
  pd->CopyData(this->OutputPointData, 0, this->OutputPointData->GetNumberOfTuples(), 0);
  points->DeepCopy(this->OutputCoordinates);

  // Strategy: we send all the particles to the root node
  if (this->Controller && this->Controller->GetNumberOfProcesses() > 1)
  {
    vtkNew<vtkPolyData> ps;
    if (this->Controller->GetLocalProcessId() != 0)
    {
      ps->GetPointData()->ShallowCopy(this->OutputPointData);
      ps->SetPoints(this->OutputCoordinates);
    }
    std::vector<vtkSmartPointer<vtkDataObject>> recvBuffer;
    this->Controller->Gather(ps, recvBuffer, 0);

    // Non root ranks have nothing to do
    if (this->Controller->GetLocalProcessId() != 0)
    {
      return retVal;
    }

    for (auto& recv : recvBuffer)
    {
      auto ds = vtkDataSet::SafeDownCast(recv);
      vtkIdType n = ds->GetNumberOfPoints();
      if (!n)
      {
        continue;
      }
      vtkIdType end = points->GetNumberOfPoints();
      // If root rank has no particles, it doesn't have arrays allocated in pd,
      // so let's allocate them here using what we received
      if (!pd->GetNumberOfTuples())
      {
        pd->CopyAllocate(ds->GetPointData(), n);
      }
      if (!pd->GetNumberOfTuples())
      {
        pd->CopyAllocate(ds->GetPointData(), ds->GetNumberOfPoints());
      }
      pd->CopyData(ds->GetPointData(), end, n, 0);
      points->InsertPoints(end, n, 0, ds->GetPoints());
    }
  }

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  auto output = vtkPolyData::SafeDownCast(vtkDataObject::GetData(outInfo));
  output->Initialize();

  vtkIdType n = points->GetNumberOfPoints();

  if (!n)
  {
    return retVal;
  }

  std::vector<std::vector<vtkIdType>> streaks(this->GetCurrentTimeIndex() + 1);

  auto ageArray = this->GetParticleAge(pd);

  // We sort streaks by age, addoing points as they come, indexed as seen by OutputPointData
  // and OutputCoordinates
  for (vtkIdType pointId = 0; pointId < ageArray->GetNumberOfValues(); ++pointId)
  {
    streaks[this->GetCurrentTimeIndex() - ageArray->GetValue(pointId)].emplace_back(pointId);
  }

  // We are going to map the output point ids to the point ids produced by Execute.
  vtkNew<vtkIdList> mapping;
  mapping->SetNumberOfIds(n);
  vtkIdType* ids = mapping->GetPointer(0);

  for (const auto& streak : streaks)
  {
    for (vtkIdType pointId : streak)
    {
      *(ids++) = pointId;
    }
  }

  vtkNew<vtkCellArray> verts;
  vtkNew<vtkCellArray> lines;
  vtkNew<vtkPoints> outPoints;

  output->GetPointData()->CopyAllocate(pd, n);
  output->GetPointData()->CopyData(pd, mapping);
  output->SetPoints(points);
  outPoints->GetData()->InsertTuplesStartingAt(0, mapping, points->GetData());

#ifdef VTK_USE_64BIT_IDS
  if (!(n >> 31))
  {
    verts->ConvertTo32BitStorage();
    lines->ConvertTo32BitStorage();
    FillCellArrays<vtkCellArray::ArrayType32>(streaks, verts, lines);
  }
#else
  if (false)
  {
  }
#endif
  else
  {
    FillCellArrays<vtkCellArray::ArrayType64>(streaks, verts, lines);
  }

  output->SetVerts(verts);
  output->SetLines(lines);

  return retVal;
}

VTK_ABI_NAMESPACE_END
