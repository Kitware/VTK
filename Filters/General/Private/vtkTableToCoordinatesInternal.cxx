// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkTableToCoordinatesInternal.h"

#include "vtkDataArray.h"
#include "vtkDataArrayRange.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkFieldData.h"
#include "vtkIdTypeArray.h"
#include "vtkMathUtilities.h"

#include <algorithm>

VTK_ABI_NAMESPACE_BEGIN
// ----------------------------------------------------------------------------
bool vtkTableToCoordinatesInternal::ComputeUniqueAndSorted(
  vtkDataArray* input, vtkDataArray* output)
{
  if (!input)
  {
    output->InsertNextTuple1(0.);
    return false;
  }

  for (const auto inputCoord : vtk::DataArrayTupleRange(input))
  {
    auto outIter = vtk::DataArrayValueRange(output);
    auto pos = std::find(outIter.begin(), outIter.end(), inputCoord[0]);
    if (pos == outIter.end())
    {
      output->InsertNextTuple1(inputCoord[0]);
    }
  }

  auto outIter = vtk::DataArrayValueRange(output);
  std::sort(outIter.begin(), outIter.end());

  return true;
}

//------------------------------------------------------------------------------
void vtkTableToCoordinatesInternal::ConstructSpace(
  vtkFieldData* nSpace, vtkDataSetAttributes* inputPoints)
{
  nSpace->CopyStructure(inputPoints);

  for (int idx = 0; idx < inputPoints->GetNumberOfArrays(); idx++)
  {
    auto col = inputPoints->GetArray(idx);
    if (!col)
    {
      continue;
    }

    auto coordinates = nSpace->GetArray(col->GetName());
    vtkTableToCoordinatesInternal::ComputeUniqueAndSorted(col, coordinates);
  }
}

//------------------------------------------------------------------------------
vtkIdType vtkTableToCoordinatesInternal::GetPointIdxInSpace(
  vtkFieldData* space, vtkDataSetAttributes* inputPoints, vtkIdType inputPointId)
{
  vtkIdType pointIdx = 0;
  vtkIdType idFactor = 1;
  for (int dimensionIdx = 0; dimensionIdx < space->GetNumberOfArrays(); dimensionIdx++)
  {
    auto coordArray = space->GetArray(dimensionIdx);
    auto inputArray = inputPoints->GetArray(coordArray->GetName());

    // this really does allocation at each call ??
    double target = inputArray->GetComponent(inputPointId, 0);
    auto coordRange = vtk::DataArrayValueRange(coordArray);
    auto coord = std::lower_bound(coordRange.begin(), coordRange.end(), target);
    vtkIdType indexOnCurrentDim = std::distance(coordRange.begin(), coord);

    pointIdx += indexOnCurrentDim * idFactor;
    idFactor *= coordArray->GetNumberOfTuples();
  }

  return pointIdx;
}

//------------------------------------------------------------------------------
void vtkTableToCoordinatesInternal::MapSpaceToInput(vtkFieldData* space,
  vtkDataSetAttributes* inputPoints, vtkIdTypeArray* spaceToInput, vtkIdType invalidMapping)
{
  spaceToInput->Initialize();
  vtkIdType inputNumberOfTuples = inputPoints->GetNumberOfTuples();
  vtkIdType spaceSize = 1;
  for (int axis = 0; axis < space->GetNumberOfArrays(); axis++)
  {
    spaceSize *= space->GetArray(axis)->GetNumberOfTuples();
  }
  spaceToInput->SetNumberOfTuples(spaceSize);
  spaceToInput->Fill(invalidMapping);

  for (vtkIdType inputIdx = 0; inputIdx < inputNumberOfTuples; inputIdx++)
  {
    vtkIdType pointIdx =
      vtkTableToCoordinatesInternal::GetPointIdxInSpace(space, inputPoints, inputIdx);
    spaceToInput->SetValue(pointIdx, inputIdx);
  }
}

//------------------------------------------------------------------------------
void vtkTableToCoordinatesInternal::GetSurroundingPoints(vtkFieldData* space,
  const std::vector<double>& pickedPoint, vtkIdTypeArray* pointIds, vtkDoubleArray* weights)
{
  weights->Initialize();
  pointIds->Initialize();
  if (space->GetNumberOfArrays() != static_cast<int>(pickedPoint.size()))
  {
    return;
  }

  weights->SetNumberOfTuples(std::pow(2, space->GetNumberOfArrays()));
  weights->Fill(1);
  pointIds->SetNumberOfTuples(std::pow(2, space->GetNumberOfArrays()));
  pointIds->Fill(0);

  vtkIdType idFactor = 1;
  int nbOfGeneratedPoints = 1;
  for (int dimensionIdx = 0; dimensionIdx < space->GetNumberOfArrays(); dimensionIdx++)
  {
    auto coordArray = space->GetArray(dimensionIdx);
    auto target = pickedPoint[dimensionIdx];

    auto coordRange = vtk::DataArrayValueRange(coordArray);
    auto coord = std::upper_bound(coordRange.begin(), coordRange.end(), target);
    vtkIdType upStructuredCoord =
      std::min(std::distance(coordRange.begin(), coord), coordArray->GetNumberOfValues() - 1);
    vtkIdType lowStructuredCoord = std::max(upStructuredCoord - 1, vtkIdType(0));

    double param = (target - coordRange[lowStructuredCoord]) /
      (coordRange[upStructuredCoord] - coordRange[lowStructuredCoord]);

    param = vtkMathUtilities::NearlyEqual(param, 0., 1.e-6) ? 0
      : vtkMathUtilities::NearlyEqual(param, 1., 1.e-6)     ? 1
                                                            : param;
    const double lowWeight = 1 - param;
    const double upWeight = param;

    // iteratively fill the indices. For each axis of the input space, get two indices: nearest
    // upper and lower bounds for the input point. Duplicates previously computed indices for both
    // those up and low values.
    for (auto prevId = 0; prevId < nbOfGeneratedPoints; prevId++)
    {
      auto prev = pointIds->GetValue(prevId);
      // we duplicate previous partial space at up and low coordinates
      pointIds->SetValue(prevId, prev + lowStructuredCoord * idFactor);
      pointIds->SetValue(prevId + nbOfGeneratedPoints, prev + upStructuredCoord * idFactor);

      auto prevWeight = weights->GetValue(prevId);
      weights->SetValue(prevId, prevWeight * lowWeight);
      weights->SetValue(prevId + nbOfGeneratedPoints, prevWeight * upWeight);
    }
    nbOfGeneratedPoints *= 2;
    idFactor *= coordArray->GetNumberOfTuples();
  }
}
VTK_ABI_NAMESPACE_END
