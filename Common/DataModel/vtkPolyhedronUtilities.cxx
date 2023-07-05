// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkPolyhedronUtilities.h"
#include "vtkArrayDispatch.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkPointData.h"
#include "vtkPolyhedron.h"
#include "vtkStringArray.h"
#include "vtkUnstructuredGrid.h"

namespace
{
//------------------------------------------------------------------------------
struct CopyWorker
{
  // Copy tuples from inArray (indexed on global ids) to outArray (indexed on local ids).
  // pointIdMap is used to map global ids to local ids.
  // WARNING: this worker only make sense in the vtkPolyhedron scope (where `globalIds` corresponds
  // to vtkPolyhedron::PointIds and `pointIdMap` corresponds to `vtkPolyhedron::PointIdMap`).
  // It should not be used in other cases, nothing guarantying that localPtId is in the
  // range of outArray.
  template <typename ArrayType1, typename ArrayType2>
  void operator()(ArrayType1* inArray, ArrayType2* outArray, vtkIdList* globalIds,
    vtkPolyhedron::vtkPointIdMap* pointIdMap)
  {
    // Number of components is already set by calling CopyStructure beforehand
    outArray->SetNumberOfTuples(globalIds->GetNumberOfIds());

    for (int i = 0; i < globalIds->GetNumberOfIds(); i++)
    {
      auto globalPtId = globalIds->GetId(i);
      auto localPtId = pointIdMap->at(globalPtId);
      outArray->SetTuple(localPtId, globalPtId, inArray);
    }
  }
};

//------------------------------------------------------------------------------
struct InitWorker
{
  // Insert new tuple to the array and initialize all its components to 0
  template <typename ArrayType>
  void operator()(ArrayType* outArray)
  {
    using T = vtk::GetAPIType<ArrayType>;

    std::vector<T> nextTuple(outArray->GetNumberOfComponents(), 0);
    outArray->InsertNextTypedTuple(nextTuple.data());
  }
};

//------------------------------------------------------------------------------
// Fallback for not dispatched data arrays
template <>
void InitWorker::operator()(vtkDataArray* outArray)
{
  std::vector<double> nextTuple(outArray->GetNumberOfComponents(), 0);
  outArray->InsertNextTuple(nextTuple.data());
}

//------------------------------------------------------------------------------
// Fallback for string array
template <>
void InitWorker::operator()(vtkStringArray* outArray)
{
  vtkIdType nbOfComponents = outArray->GetNumberOfComponents();
  vtkIdType nbOfValuesOld = outArray->GetNumberOfValues();
  outArray->SetNumberOfValues(nbOfValuesOld + nbOfComponents);

  for (int i = 0; i < nbOfComponents; i++)
  {
    outArray->SetValue(nbOfValuesOld + i, "");
  }
}

//------------------------------------------------------------------------------
// Fallback for all other arrays
template <>
void InitWorker::operator()(vtkAbstractArray* outArray)
{
  // Insert one "uninitialized" tuple
  vtkIdType nbOfComponents = outArray->GetNumberOfComponents();
  vtkIdType nbOfValuesOld = outArray->GetNumberOfValues();
  outArray->SetNumberOfValues(nbOfValuesOld + nbOfComponents);
}

//------------------------------------------------------------------------------
struct AccuWorker
{
  // Add the components of the given tuple of inArray to the components
  // of the last tuple of outArray
  template <typename ArrayType1, typename ArrayType2>
  void operator()(ArrayType1* inArray, ArrayType2* outArray, vtkIdType inPtId)
  {
    using T = vtk::GetAPIType<ArrayType1>;

    int nbOfComp = outArray->GetNumberOfComponents();
    vtkIdType nbOfValues = outArray->GetNumberOfValues();

    const auto inRange =
      vtk::DataArrayValueRange(inArray, inPtId * nbOfComp, (inPtId + 1) * nbOfComp);
    auto outRange = vtk::DataArrayValueRange(outArray, nbOfValues - nbOfComp, nbOfValues);

    std::transform(inRange.cbegin(), inRange.cend(), outRange.cbegin(), outRange.begin(),
      [=](T inValue, T outValue) { return outValue + inValue; });
  }
};

//------------------------------------------------------------------------------
struct DivWorker
{
  // Divide all the components of the last tuple of outArray with div
  template <typename ArrayType>
  void operator()(ArrayType* outArray, vtkIdType div)
  {
    using T = vtk::GetAPIType<ArrayType>;

    int nbOfComp = outArray->GetNumberOfComponents();
    vtkIdType nbOfValues = outArray->GetNumberOfValues();

    auto range = vtk::DataArrayValueRange(outArray, nbOfValues - nbOfComp, nbOfValues);

    std::transform(
      range.cbegin(), range.cend(), range.begin(), [=](T inValue) { return inValue / div; });
  }
};
}

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
vtkSmartPointer<vtkUnstructuredGrid> vtkPolyhedronUtilities::Decompose(
  vtkPolyhedron* polyhedron, vtkPointData* inPd, vtkIdType cellId, vtkCellData* inCd)
{
  if (!polyhedron->GetPoints() || !polyhedron->GetNumberOfPoints())
  {
    return nullptr;
  }

  vtkNew<vtkUnstructuredGrid> outputGrid;
  ::CopyWorker copyWorker;
  ::InitWorker initWorker;
  ::AccuWorker accuWorker;
  ::DivWorker divWorker;
  typedef vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::AllTypes> Dispatcher;
  typedef vtkArrayDispatch::Dispatch2BySameValueType<vtkArrayDispatch::AllTypes> Dispatcher2;

  vtkPolyhedron::vtkPointIdMap* pointIdMap = polyhedron->PointIdMap;
  vtkIdList* pointIds = polyhedron->GetPointIds();

  ////////// Copy point data to the output //////////
  // Output point data should follow the output unstructured grid indexation, that corresponds
  // initially to polyhedron's canonical ids (new point data will be added for barycenters).
  // Therefore, we can use polyhedron's pointIdMap to do the mapping between input ids and output
  // ids.
  vtkPointData* outPd = outputGrid->GetPointData();
  if (inPd)
  {
    outPd->CopyStructure(inPd);
    for (vtkIdType arrayId = 0; arrayId < outPd->GetNumberOfArrays(); arrayId++)
    {
      vtkAbstractArray* inAbstArray = inPd->GetAbstractArray(arrayId);
      vtkDataArray* inArray = vtkDataArray::SafeDownCast(inAbstArray);
      vtkAbstractArray* outAbstArray = outPd->GetAbstractArray(arrayId);
      vtkDataArray* outArray = vtkDataArray::SafeDownCast(outAbstArray);
      if (inArray && outArray)
      {
        if (!Dispatcher2::Execute(inArray, outArray, copyWorker, pointIds, pointIdMap))
        {
          copyWorker(inArray, outArray, pointIds, pointIdMap); // Fallback for vtkDataArray subtypes
        }
      }
      else
      {
        copyWorker(inAbstArray, outAbstArray, pointIds, pointIdMap); // Fallback for other arrays
      }
    }
  }

  ////////// Compute barycenters and barycenters data //////////
  // Here we iterate over each face and generate a new point (barycenter of the face).
  // We also add new point data for the barycenter, that is the mean value of the face points data.
  // XXX Consider rework this code in order to include the face and face points interations
  // inside the workers in order to reduce the number of dispatchs (that are costly)

  // Global faces are faces with global point indexes
  vtkIdType* globalFaces = polyhedron->GetFaces();
  vtkIdType facesNb = globalFaces[0];
  vtkIdType* globalFace = globalFaces + 1;
  vtkIdType numberOfNewCells = 0; // Account for the number of cells of the output UG

  vtkNew<vtkPoints> barycenters;
  // Iterate on each face to compute face barycenters and barycenters data (point data)
  for (vtkIdType faceCount = 0; faceCount < facesNb; faceCount++)
  {
    vtkIdType nbFacePts = globalFace[0];

    // Add a new value for each output array, init to 0.0
    for (vtkIdType arrayId = 0; arrayId < outPd->GetNumberOfArrays(); arrayId++)
    {
      vtkAbstractArray* abstArray = outPd->GetAbstractArray(arrayId);
      if (auto dataArray = vtkDataArray::SafeDownCast(abstArray))
      {
        if (!Dispatcher::Execute(dataArray, initWorker))
        {
          initWorker(dataArray); // Fallback for vtkDataArray subtypes
        }
      }
      else if (auto stringArray = vtkStringArray::SafeDownCast(abstArray))
      {
        initWorker(stringArray); // Fallback for vtkStringArray
      }
      else
      {
        vtkWarningWithObjectMacro(nullptr,
          "" << abstArray->GetName()
             << ": array type is not supported. Values on new points will be undefined.");
        initWorker(abstArray); // Fallback for all other types
      }
    }

    std::array<double, 3> barycenter = { 0.0, 0.0, 0.0 };

    for (vtkIdType i = 1; i <= nbFacePts; i++)
    {
      // Accumulate face points coordinates
      auto globalPtId = globalFace[i];
      auto localPtId = pointIdMap->at(globalPtId);

      std::array<double, 3> pt = { 0.0, 0.0, 0.0 };
      polyhedron->GetPoints()->GetPoint(localPtId, pt.data());
      barycenter[0] += pt[0];
      barycenter[1] += pt[1];
      barycenter[2] += pt[2];

      // Accumulate barycenter new point data
      for (vtkIdType arrayId = 0; arrayId < outPd->GetNumberOfArrays(); arrayId++)
      {
        vtkDataArray* inArray = vtkDataArray::SafeDownCast(inPd->GetAbstractArray(arrayId));
        vtkDataArray* outArray = vtkDataArray::SafeDownCast(outPd->GetAbstractArray(arrayId));
        if (inArray && outArray)
        {
          if (!Dispatcher2::Execute(inArray, outArray, accuWorker, globalPtId))
          {
            // We only need to fallback for vtkDataArray subtypes, other types
            // are just initialized and no mean value is computed.
            accuWorker(inArray, outArray, globalPtId);
          }
        }
      }
    }

    // Compute the barycenter of the face
    for (int i = 0; i < 3; i++)
    {
      barycenter[i] /= nbFacePts;
    }

    barycenters->InsertNextPoint(barycenter.data());

    // Compute barycenter point data
    for (vtkIdType arrayId = 0; arrayId < outPd->GetNumberOfArrays(); arrayId++)
    {
      if (auto array = vtkDataArray::SafeDownCast(outPd->GetAbstractArray(arrayId)))
      {
        if (!Dispatcher::Execute(array, divWorker, nbFacePts))
        {
          // Fallback for vtkDataArray subtypes only
          divWorker(array, nbFacePts);
        }
      }
    }

    numberOfNewCells += nbFacePts;
    globalFace += nbFacePts + 1; // Go to next face
  }

  // Compute polyhedron barycenter from faces barycenters
  std::array<double, 3> polyBarycenter = { 0.0, 0.0, 0.0 };
  for (vtkIdType ptId = 0; ptId < barycenters->GetNumberOfPoints(); ptId++)
  {
    std::array<double, 3> pt = { 0.0, 0.0, 0.0 };
    barycenters->GetPoint(ptId, pt.data());
    polyBarycenter[0] += pt[0];
    polyBarycenter[1] += pt[1];
    polyBarycenter[2] += pt[2];
  }
  for (int i = 0; i < 3; i++)
  {
    polyBarycenter[i] /= barycenters->GetNumberOfPoints();
  }

  // Compute polyhedron barycenter point data
  for (vtkIdType arrayId = 0; arrayId < outPd->GetNumberOfArrays(); arrayId++)
  {
    vtkAbstractArray* abstArray = outPd->GetAbstractArray(arrayId);
    vtkDataArray* array = vtkDataArray::SafeDownCast(abstArray);
    if (array)
    {
      if (!Dispatcher::Execute(array, initWorker))
      {
        initWorker(array); // Fallback for vtkDataArray subtypes
      }
    }
    else if (auto stringArray = vtkStringArray::SafeDownCast(abstArray))
    {
      initWorker(stringArray); // Fallback for vtkStringArray
    }
    else
    {
      vtkWarningWithObjectMacro(nullptr,
        "" << abstArray->GetName()
           << ": array type is not supported. Values on new points will be undefined.");
      initWorker(abstArray); // Fallback for all other types
    }

    for (vtkIdType pointId = polyhedron->GetNumberOfPoints();
         pointId < polyhedron->GetNumberOfPoints() + barycenters->GetNumberOfPoints(); pointId++)
    {
      if (array)
      {
        if (!Dispatcher2::Execute(array, array, accuWorker, pointId))
        {
          // Fallback for vtkDataArray subtypes only
          accuWorker(array, array, pointId);
        }
      }
    }
    if (array)
    {
      if (!Dispatcher::Execute(array, divWorker, barycenters->GetNumberOfPoints()))
      {
        // Fallback for vtkDataArray subtypes only
        divWorker(array, barycenters->GetNumberOfPoints());
      }
    }
  }

  /////////////////// Construct output UG  ///////////////////
  // Here we construct the output UG (geometry and topology).
  // For each concave face, we generate a tetrahedron for each face edge (2 edge points + face
  // barycenter + cell barycenter). We also fill the cell data here.

  // Copy the original points to the output UG
  vtkNew<vtkPoints> outputPoints;
  outputGrid->SetPoints(outputPoints);
  outputPoints->DeepCopy(polyhedron->GetPoints());

  // Add the new points (barycenters) to the output UG
  outputPoints->Resize(polyhedron->GetNumberOfPoints() + barycenters->GetNumberOfPoints() + 1);
  for (vtkIdType newPtId = 0; newPtId < barycenters->GetNumberOfPoints(); newPtId++)
  {
    outputPoints->InsertNextPoint(barycenters->GetPoint(newPtId));
  }
  outputPoints->InsertNextPoint(polyBarycenter.data());

  // Prepare output cell data
  vtkCellData* outCd = outputGrid->GetCellData();
  outCd->CopyAllocate(inCd, numberOfNewCells);

  vtkIdType barycenterId = polyhedron->GetNumberOfPoints();
  vtkIdType polyBarycenterId = polyhedron->GetNumberOfPoints() + barycenters->GetNumberOfPoints();

  // Insert to UG a new tetra. Tetra points:
  // ptId1, ptId2 (forming one face edge), face barycenter, polyhedron barycenter
  auto insertTetra = [&](vtkCell* face, vtkIdType ptId1, vtkIdType ptId2) {
    vtkIdType ptIds[4] = { 0 };
    ptIds[0] = pointIdMap->at(face->GetPointId(ptId1));
    ptIds[1] = barycenterId;
    ptIds[2] = pointIdMap->at(face->GetPointId(ptId2));
    ptIds[3] = polyBarycenterId;
    vtkIdType newCellId = outputGrid->InsertNextCell(VTK_TETRA, 4, ptIds);
    outCd->CopyData(inCd, cellId, newCellId);
  };

  // Add cells to output UG. Each new cell will contain the same data that the current polyhedron.
  // This can be potentially improved by finding a way to insert index at the same time we
  // create the barycenters, in order to avoid re-iterating over all the faces again
  for (vtkIdType faceId = 0; faceId < polyhedron->GetNumberOfFaces(); ++faceId)
  {
    vtkCell* face = polyhedron->GetFace(faceId);

    for (vtkIdType ptId = 0; ptId < face->GetNumberOfPoints() - 1; ptId++)
    {
      insertTetra(face, ptId, ptId + 1);
    }

    insertTetra(face, face->GetNumberOfPoints() - 1, 0);
    barycenterId++;
  }

  return outputGrid;
}

VTK_ABI_NAMESPACE_END
