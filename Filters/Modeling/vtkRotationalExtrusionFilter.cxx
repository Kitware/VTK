// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkRotationalExtrusionFilter.h"

#include "vtkAMRUtilities.h"
#include "vtkAppendFilter.h"
#include "vtkCartesianGrid.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCellIterator.h"
#include "vtkCompositeDataIterator.h"
#include "vtkGenericCell.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkOverlappingAMR.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUniformGrid.h"
#include "vtkUniformGridAMR.h"
#include "vtkUnstructuredGrid.h"

#include <array>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkRotationalExtrusionFilter);

namespace
{
struct AxisOfRevolution
{
  std::array<double, 3> Position;
  std::array<double, 3> Direction;
};

vtkSmartPointer<vtkUnstructuredGrid> AMRToUnstructuredGrid(vtkSmartPointer<vtkUniformGridAMR> inAMR)
{
  vtkNew<vtkAppendFilter> append;
  append->MergePointsOn();

  vtkSmartPointer<vtkCompositeDataIterator> it{ nullptr };

  // Insuring cell blanking to remove coarse refined cells later
  if (auto overAMR = vtkOverlappingAMR::SafeDownCast(inAMR))
  {
    vtkAMRUtilities::BlankCells(overAMR);

    it.TakeReference(vtkCompositeDataIterator::SafeDownCast(overAMR->NewIterator()));
  }

  else
  {
    it.TakeReference(vtkCompositeDataIterator::SafeDownCast(inAMR->NewIterator()));
  }

  for (it->InitTraversal(); !it->IsDoneWithTraversal(); it->GoToNextItem())
  {
    if (auto cartesianGrid = vtkCartesianGrid::SafeDownCast(it->GetCurrentDataObject()))
    {
      append->AddInputData(cartesianGrid);
    }
    else
    {
      vtkGenericWarningMacro(
        "Not a valid input data, only accepting UniformGrid or RectilinearGrid in AMR case");
      return nullptr;
    }
  }

  append->Update();

  vtkSmartPointer<vtkUnstructuredGrid> outUg = vtkSmartPointer<vtkUnstructuredGrid>::New();
  outUg->ShallowCopy(append->GetOutputDataObject(0));

  // Removing GhostCells with Attribute HIDDENCELL, REFINEDCELL, or DUPLICATECELL
  // REFINEDCELL is not mentioned in RemoveGhostCell documentation
  outUg->RemoveGhostCells();

  return outUg;
}

void RevolvePoint(std::array<double, 3> inPt, const AxisOfRevolution* axis, double angleIncr,
  double radIncr, double transIncr, std::array<double, 3>& outPt)
{
  std::array<double, 4> rotationAngleAndAxis{ angleIncr, axis->Direction[0], axis->Direction[1],
    axis->Direction[2] };

  // offsetting the points to the actual rotation axis coordinate system
  // (because RotateVectorByWXYZ rotates around origin)
  // position is set back at the end of the transformation
  inPt[0] -= axis->Position[0];
  inPt[1] -= axis->Position[1];
  inPt[2] -= axis->Position[2];

  // rotating point along axis
  vtkMath::RotateVectorByWXYZ(inPt.data(), rotationAngleAndAxis.data(), outPt.data());

  // translating point along axis
  outPt[0] += axis->Direction[0] * transIncr;
  outPt[1] += axis->Direction[1] * transIncr;
  outPt[2] += axis->Direction[2] * transIncr;

  // incrementing radius along rotation
  std::array<double, 3> projection;
  std::array<double, 3> radialVector;
  vtkMath::ProjectVector(outPt.data(), axis->Direction.data(), projection.data());
  vtkMath::Subtract(outPt.data(), projection.data(), radialVector.data());

  outPt[0] += radialVector[0] * radIncr;
  outPt[1] += radialVector[1] * radIncr;
  outPt[2] += radialVector[2] * radIncr;

  // setting back the points in the correct coordinate system
  outPt[0] += axis->Position[0];
  outPt[1] += axis->Position[1];
  outPt[2] += axis->Position[2];
}

void RevolvePoints(vtkDataSet* input, vtkPoints* newPts, AxisOfRevolution* axis, double angle,
  double deltaRadius, double translation, int resolution, vtkPointData* outPd, bool partialSweep)
{
  double angleInRadians = vtkMath::RadiansFromDegrees(angle) / resolution;
  double radIncr = deltaRadius / resolution;
  double transIncr = translation / resolution;

  vtkIdType n2DPoints = input->GetNumberOfPoints();
  vtkIdType counter{ 0 };
  std::array<double, 3> p2d;
  std::array<double, 3> p3d;

  // add one resolution step if partialSweep is true to close the surface
  for (int i = 0; i < resolution + static_cast<int>(partialSweep); i++)
  {
    for (int id = 0; id < n2DPoints; id++)
    {
      input->GetPoint(id, p2d.data());
      RevolvePoint(p2d, axis, i * angleInRadians, i * radIncr, i * transIncr, p3d);
      newPts->SetPoint(counter, p3d.data());
      outPd->CopyData(input->GetPointData(), id, counter);
      counter++;
    }
  }
}

template <int CellType>
void Revolve(vtkIdList* pointIds, vtkIdType n2DPoints, int resolution, vtkCellArray* connectivity,
  vtkUnsignedCharArray* types, vtkCellData* inCd, vtkIdType cellId, vtkCellData* outCd,
  bool partialSweep);

template <>
void Revolve<VTK_VERTEX>(vtkIdList* pointIds, vtkIdType n2DPoints, int resolution,
  vtkCellArray* connectivity, vtkUnsignedCharArray* types, vtkCellData* inCd, vtkIdType cellId,
  vtkCellData* outCd, bool partialSweep)
{
  std::array<vtkIdType, 2> newPtIds;
  vtkIdType newCellId;

  newPtIds[0] = pointIds->GetId(0);
  for (int i = 0; i < resolution; i++)
  {
    // add one resolution step if partialSweep is true to close the surface
    newPtIds[1] =
      (pointIds->GetId(0) + ((i + 1) % (resolution + static_cast<int>(partialSweep))) * n2DPoints);
    newCellId = connectivity->InsertNextCell(2, newPtIds.data());
    types->InsertNextValue(VTK_LINE);
    outCd->CopyData(inCd, cellId, newCellId);
    newPtIds[0] = newPtIds[1];
  }
}

template <>
void Revolve<VTK_POLY_VERTEX>(vtkIdList* pointIds, vtkIdType n2DPoints, int resolution,
  vtkCellArray* connectivity, vtkUnsignedCharArray* types, vtkCellData* inCd, vtkIdType cellId,
  vtkCellData* outCd, bool partialSweep)
{
  vtkNew<vtkIdList> pointId;
  pointId->SetNumberOfIds(1);
  for (vtkIdType i = 0; i < pointIds->GetNumberOfIds(); i++)
  {
    pointId->SetId(0, pointIds->GetId(i));
    Revolve<VTK_VERTEX>(
      pointId, n2DPoints, resolution, connectivity, types, inCd, cellId, outCd, partialSweep);
  }
}

template <>
void Revolve<VTK_LINE>(vtkIdList* pointIds, vtkIdType n2DPoints, int resolution,
  vtkCellArray* connectivity, vtkUnsignedCharArray* types, vtkCellData* inCd, vtkIdType cellId,
  vtkCellData* outCd, bool partialSweep)
{
  static const int nPoints = 2;

  std::array<vtkIdType, 2 * nPoints> newPtIds;
  vtkIdType newCellId;

  for (vtkIdType i = 0; i < nPoints; i++)
  {
    newPtIds[i] = pointIds->GetId(i);
  }

  for (int i = 0; i < resolution; i++)
  {
    for (vtkIdType j = 0; j < nPoints; j++)
    {
      // add one resolution step if partialSweep is true to close the surface
      newPtIds[2 * nPoints - 1 - j] =
        pointIds->GetId(j) + ((i + 1) % (resolution + static_cast<int>(partialSweep))) * n2DPoints;
    }
    newCellId = connectivity->InsertNextCell(2 * nPoints, newPtIds.data());
    types->InsertNextValue(VTK_QUAD);
    outCd->CopyData(inCd, cellId, newCellId);
    for (vtkIdType j = 0; j < nPoints; j++)
    {
      newPtIds[nPoints - 1 - j] = newPtIds[j + nPoints];
    }
  }
}

template <>
void Revolve<VTK_POLY_LINE>(vtkIdList* pointIds, vtkIdType n2DPoints, int resolution,
  vtkCellArray* connectivity, vtkUnsignedCharArray* types, vtkCellData* inCd, vtkIdType cellId,
  vtkCellData* outCd, bool partialSweep)
{
  vtkNew<vtkIdList> newPointIds;
  newPointIds->SetNumberOfIds(2);
  newPointIds->SetId(0, pointIds->GetId(0));
  for (vtkIdType i = 1; i < pointIds->GetNumberOfIds(); i++)
  {
    newPointIds->SetId(1, pointIds->GetId(i));
    Revolve<VTK_LINE>(
      newPointIds, n2DPoints, resolution, connectivity, types, inCd, cellId, outCd, partialSweep);
    newPointIds->SetId(0, pointIds->GetId(i));
  }
}

template <>
void Revolve<VTK_TRIANGLE>(vtkIdList* pointIds, vtkIdType n2DPoints, int resolution,
  vtkCellArray* connectivity, vtkUnsignedCharArray* types, vtkCellData* inCd, vtkIdType cellId,
  vtkCellData* outCd, bool partialSweep)
{
  static const int nPoints = 3;

  std::array<vtkIdType, 2 * nPoints> newPtIds;
  vtkIdType newCellId;

  for (vtkIdType i = 0; i < nPoints; i++)
  {
    newPtIds[i] = pointIds->GetId(i);
  }

  for (int i = 0; i < resolution; i++)
  {
    for (vtkIdType j = 0; j < nPoints; j++)
    {
      // add one resolution step if partialSweep is true to close the surface
      newPtIds[j + nPoints] =
        pointIds->GetId(j) + ((i + 1) % (resolution + static_cast<int>(partialSweep))) * n2DPoints;
    }
    newCellId = connectivity->InsertNextCell(2 * nPoints, newPtIds.data());
    types->InsertNextValue(VTK_WEDGE);
    outCd->CopyData(inCd, cellId, newCellId);
    for (vtkIdType j = 0; j < nPoints; j++)
    {
      newPtIds[j] = newPtIds[j + nPoints];
    }
  }
}

template <>
void Revolve<VTK_TRIANGLE_STRIP>(vtkIdList* pointIds, vtkIdType n2DPoints, int resolution,
  vtkCellArray* connectivity, vtkUnsignedCharArray* types, vtkCellData* inCd, vtkIdType cellId,
  vtkCellData* outCd, bool partialSweep)
{
  vtkNew<vtkIdList> newPointIds;
  newPointIds->SetNumberOfIds(3);
  newPointIds->SetId(0, pointIds->GetId(0));
  newPointIds->SetId(1, pointIds->GetId(1));
  for (vtkIdType i = 2; i < pointIds->GetNumberOfIds(); i++)
  {
    newPointIds->SetId(2, pointIds->GetId(i));
    Revolve<VTK_TRIANGLE>(
      newPointIds, n2DPoints, resolution, connectivity, types, inCd, cellId, outCd, partialSweep);
    newPointIds->SetId(0, pointIds->GetId(i));
    newPointIds->SetId(1, pointIds->GetId(i - 1));
  }
}

template <>
void Revolve<VTK_QUAD>(vtkIdList* pointIds, vtkIdType n2DPoints, int resolution,
  vtkCellArray* connectivity, vtkUnsignedCharArray* types, vtkCellData* inCd, vtkIdType cellId,
  vtkCellData* outCd, bool partialSweep)
{
  static const int nPoints = 4;

  std::array<vtkIdType, 2 * nPoints> newPtIds;
  vtkIdType newCellId;

  for (vtkIdType i = 0; i < nPoints; i++)
  {
    newPtIds[i] = pointIds->GetId(i);
  }

  for (int i = 0; i < resolution; i++)
  {
    for (vtkIdType j = 0; j < nPoints; j++)
    {
      // add one resolution step if partialSweep is true to close the surface
      newPtIds[j + nPoints] =
        pointIds->GetId(j) + ((i + 1) % (resolution + static_cast<int>(partialSweep))) * n2DPoints;
    }
    newCellId = connectivity->InsertNextCell(2 * nPoints, newPtIds.data());
    types->InsertNextValue(VTK_HEXAHEDRON);
    outCd->CopyData(inCd, cellId, newCellId);
    for (vtkIdType j = 0; j < nPoints; j++)
    {
      newPtIds[j] = newPtIds[j + nPoints];
    }
  }
}

template <>
void Revolve<VTK_PIXEL>(vtkIdList* pointIds, vtkIdType n2DPoints, int resolution,
  vtkCellArray* connectivity, vtkUnsignedCharArray* types, vtkCellData* inCd, vtkIdType cellId,
  vtkCellData* outCd, bool partialSweep)
{
  static const int nPoints = 4;

  std::array<vtkIdType, 2 * nPoints> newPtIds;
  vtkIdType newCellId;

  // NOT OK FOR A HEXAHEDRON!
  // Pixel is :
  //
  //     2--3
  //     |  |
  //     0--1
  //
  // Polyhedron needs :
  //
  //     3--2
  //     |  |
  //     0--1

  newPtIds[0] = pointIds->GetId(0);
  newPtIds[1] = pointIds->GetId(1);
  newPtIds[2] = pointIds->GetId(3);
  newPtIds[3] = pointIds->GetId(2);

  for (int i = 0; i < resolution; i++)
  {
    // add one resolution step if partialSweep is true to close the surface
    newPtIds[0 + nPoints] =
      pointIds->GetId(0) + ((i + 1) % (resolution + static_cast<int>(partialSweep))) * n2DPoints;
    newPtIds[1 + nPoints] =
      pointIds->GetId(1) + ((i + 1) % (resolution + static_cast<int>(partialSweep))) * n2DPoints;
    newPtIds[2 + nPoints] =
      pointIds->GetId(3) + ((i + 1) % (resolution + static_cast<int>(partialSweep))) * n2DPoints;
    newPtIds[3 + nPoints] =
      pointIds->GetId(2) + ((i + 1) % (resolution + static_cast<int>(partialSweep))) * n2DPoints;

    newCellId = connectivity->InsertNextCell(2 * nPoints, newPtIds.data());
    types->InsertNextValue(VTK_HEXAHEDRON);
    outCd->CopyData(inCd, cellId, newCellId);
    for (vtkIdType j = 0; j < nPoints; j++)
    {
      newPtIds[j] = newPtIds[j + nPoints];
    }
  }
}

template <>
void Revolve<VTK_POLYGON>(vtkIdList* pointIds, vtkIdType n2DPoints, int resolution,
  vtkCellArray* connectivity, vtkUnsignedCharArray* types, vtkCellData* inCd, vtkIdType cellId,
  vtkCellData* outCd, bool partialSweep)
{
  // A swept polygon creates a polyhedron with two polygon faces and <nPoly>
  // quad faces, comprised from 2*<nPoly> points. Because polyhedra have a
  // special connectivity format, the length of the connectivity array is
  // 1 + (<nPoly>+2) + 2*<nPoly> + 4*<nPoly> = 7*<nPoly> + 3.
  // ^        ^           ^           ^
  // integer describing # of faces (<nPoly> + 2)
  //          ^           ^           ^
  //          integers describing # of points per face
  //                      ^           ^
  //                      point ids for the two polygon faces
  //                                  ^
  //                                    point ids for the 4 quad faces

  int nPoly = pointIds->GetNumberOfIds();
  std::vector<vtkIdType> newPtIds(7 * nPoly + 3);
  // newFacePtIds are pointers to the point arrays describing each face
  std::vector<vtkIdType*> newFacePtIds(nPoly + 2);
  vtkIdType newCellId;

  newPtIds[0] = nPoly + 2;
  newPtIds[1] = nPoly;
  newPtIds[nPoly + 2] = nPoly;
  newFacePtIds[0] = &newPtIds[2];
  newFacePtIds[1] = &newPtIds[nPoly + 3];
  for (vtkIdType i = 0; i < nPoly; i++)
  {
    // All of the subsequent faces have four point ids
    newPtIds[3 + 2 * nPoly + 5 * i] = 4;
    newFacePtIds[2 + i] = &newPtIds[4 + 2 * nPoly + 5 * i];
    newFacePtIds[0][i] = pointIds->GetId(i);
  }

  for (int i = 0; i < resolution; i++)
  {
    for (vtkIdType j = 0; j < nPoly; j++)
    {
      // add one resolution step if partialSweep is true to close the surface
      newFacePtIds[1][nPoly - 1 - j] =
        pointIds->GetId(j) + ((i + 1) % (resolution + static_cast<int>(partialSweep))) * n2DPoints;
    }
    for (vtkIdType j = 0; j < nPoly; j++)
    {
      newFacePtIds[j + 2][0] = newFacePtIds[0][j];
      newFacePtIds[j + 2][1] = newFacePtIds[0][(j + 1) % nPoly];
      newFacePtIds[j + 2][2] = newFacePtIds[1][(2 * nPoly - 2 - j) % nPoly];
      newFacePtIds[j + 2][3] = newFacePtIds[1][nPoly - 1 - j];
    }
    newCellId = connectivity->InsertNextCell(7 * nPoly + 3, newPtIds.data());
    types->InsertNextValue(VTK_POLYHEDRON);
    outCd->CopyData(inCd, cellId, newCellId);
    for (vtkIdType j = 0; j < nPoly; j++)
    {
      newFacePtIds[0][j] = newFacePtIds[1][nPoly - 1 - j];
    }
  }
}

int RevolveCell(int cellType, vtkIdList* pointIds, vtkIdType n2DPoints, int resolution,
  vtkCellArray* connectivity, vtkUnsignedCharArray* types, vtkCellData* inCd, vtkIdType cellId,
  vtkCellData* outCd, bool partialSweep)
{
  int returnValue{ 0 };
#define RevolveCellCase(CellType)                                                                  \
  case CellType:                                                                                   \
    Revolve<CellType>(                                                                             \
      pointIds, n2DPoints, resolution, connectivity, types, inCd, cellId, outCd, partialSweep);    \
    break

  switch (cellType)
  {
    RevolveCellCase(VTK_VERTEX);
    RevolveCellCase(VTK_POLY_VERTEX);
    RevolveCellCase(VTK_LINE);
    RevolveCellCase(VTK_POLY_LINE);
    RevolveCellCase(VTK_TRIANGLE);
    RevolveCellCase(VTK_TRIANGLE_STRIP);
    RevolveCellCase(VTK_POLYGON);
    RevolveCellCase(VTK_PIXEL);
    RevolveCellCase(VTK_QUAD);
    default:
      returnValue = 1;
  }
  return returnValue;
#undef RevolveCellCase
}
}

//------------------------------------------------------------------------------
int vtkRotationalExtrusionFilter::RequestDataObject(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{

  auto inputDS = vtkDataObject::GetData(inputVector[0], 0);
  int outputType{ -1 };
  if (vtkDataSet::SafeDownCast(inputDS) || vtkUniformGridAMR::SafeDownCast(inputDS))
  {
    if (this->Volumetric)
      outputType = VTK_UNSTRUCTURED_GRID;
    else
      outputType = VTK_POLY_DATA;
  }

  else
  {
    vtkErrorMacro("Unsupported input type: " << inputDS->GetClassName());
    return 0;
  }

  return vtkDataObjectAlgorithm::SetOutputDataObject(
           outputType, outputVector->GetInformationObject(0), /*exact*/ true)
    ? 1
    : 0;
}

//------------------------------------------------------------------------------
int vtkRotationalExtrusionFilter::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* vtkNotUsed(outputVector))
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);
  return 1;
}

//------------------------------------------------------------------------------
int vtkRotationalExtrusionFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input data
  auto input = vtkDataObject::GetData(inputVector[0], 0);

  if (this->Volumetric)
  {
    auto output = vtkUnstructuredGrid::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
    assert(output != nullptr);

    // Handles AMR Data
    if (auto inputAMR = vtkUniformGridAMR::SafeDownCast(input))
    {
      vtkSmartPointer<vtkUnstructuredGrid> inputDS = AMRToUnstructuredGrid(inputAMR);

      return static_cast<int>(this->ExecuteVolumetric(inputDS, output));
    }

    // Handles Datasets
    else if (vtkDataSet::SafeDownCast(input))
    {
      auto inputDS = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
      assert(inputDS != nullptr);
      return static_cast<int>(this->ExecuteVolumetric(inputDS, output));
    }
    return 0;
  }
  else
  {
    vtkSmartPointer<vtkPolyData> inputPD = vtkPolyData::GetData(inputVector[0]);
    if (inputPD == nullptr)
    {
      vtkErrorMacro(<< "Not a valid input data, only accepting PolyData in surfacic mode");
      return 0;
    }
    vtkSmartPointer<vtkPolyData> output = vtkPolyData::GetData(outputVector, 0);
    assert(output != nullptr);
    return static_cast<int>(this->ExecuteSurfacic(inputPD, output));
  }
}

//------------------------------------------------------------------------------
// This method delegates to the appropriate algorithm
bool vtkRotationalExtrusionFilter::ExecuteSurfacic(
  vtkSmartPointer<vtkPolyData> input, vtkSmartPointer<vtkPolyData> output)
{
  AxisOfRevolution axis;
  for (vtkIdType i = 0; i < 3; i++)
  {
    axis.Position[i] = this->AxisPosition[i];
    axis.Direction[i] = this->RotationAxis[i];
  }

  std::array<double, 3> rotationAxisNormalized = { axis.Direction[0], axis.Direction[1],
    axis.Direction[2] };

  double norm = vtkMath::Normalize(rotationAxisNormalized.data());

  // if norm is equal to zero, the extrusion cannot be done
  if (norm == 0.0)
  {
    vtkErrorMacro(<< "Cannot perform extrusion around an axis with a norm of 0.");
    return false;
  }

  vtkIdType cellId;
  vtkIdType npts;
  const vtkIdType* pts;

  bool abort{ false };

  vtkPointData* pd{ input->GetPointData() };
  vtkCellData* cd{ input->GetCellData() };
  vtkPointData* outPD{ output->GetPointData() };
  vtkCellData* outCD{ output->GetCellData() };

  vtkSmartPointer<vtkPolyData> mesh = vtkSmartPointer<vtkPolyData>::New();
  vtkSmartPointer<vtkPoints> newPts = vtkSmartPointer<vtkPoints>::New();
  vtkSmartPointer<vtkCellArray> newLines = vtkSmartPointer<vtkCellArray>::New();
  vtkSmartPointer<vtkCellArray> newPolys = vtkSmartPointer<vtkCellArray>::New();
  vtkSmartPointer<vtkCellArray> newStrips = vtkSmartPointer<vtkCellArray>::New();
  vtkSmartPointer<vtkIdList> cellIds = vtkSmartPointer<vtkIdList>::New();
  vtkSmartPointer<vtkGenericCell> cell = vtkSmartPointer<vtkGenericCell>::New();
  vtkIdType newCellId{ 0 };

  // Initialize / check input
  //
  vtkDebugMacro(<< "Rotationally extruding data");
  vtkIdType numPts = input->GetNumberOfElements(0);
  vtkIdType numCells = input->GetNumberOfElements(1);
  if (numPts < 1 || numCells < 1)
  {
    vtkErrorMacro(<< "No data to extrude!");
    return false;
  }

  // Build cell data structure.
  //

  vtkPoints* inPts = input->GetPoints();
  vtkCellArray* inVerts = input->GetVerts();
  vtkCellArray* inLines = input->GetLines();
  vtkCellArray* inPolys = input->GetPolys();
  vtkCellArray* inStrips = input->GetStrips();
  mesh->SetPoints(inPts);
  mesh->SetVerts(inVerts);
  mesh->SetLines(inLines);
  mesh->SetPolys(inPolys);
  mesh->SetStrips(inStrips);
  if (inPolys || inStrips)
  {
    mesh->BuildLinks();
  }

  // Determine whether or not the sweep angle is a full 2*pi
  // or if the radius or translation is changing.
  // If so, the extrusion is partial (does not connect back to the initial surface).
  // Thus, we need to add a set of points at the end of the sweep to close the surface.
  // We also need to add a set of points
  bool partialSweep = ((fabs(360. - fabs(this->Angle)) > 1024 * VTK_DBL_EPSILON) ||
    (this->DeltaRadius != 0.0) || (this->Translation != 0.0));

  // Allocate memory for output. We don't copy normals because surface geometry
  // is modified.
  // add one resolution step if partialSweep is true to close the surface
  outPD->CopyNormalsOff();
  outPD->CopyAllocate(pd, (this->Resolution + static_cast<int>(partialSweep)) * numPts);
  // Set up output points and point data

  newPts->SetNumberOfPoints(
    input->GetNumberOfPoints() * (this->Resolution + static_cast<int>(partialSweep)));

  vtkIdType ncells;
  if ((ncells = inVerts->GetNumberOfCells()) > 0)
  {
    newLines->AllocateEstimate(ncells, this->Resolution + 1);
  }
  // arbitrary initial allocation size
  ncells = inLines->GetNumberOfCells() + inPolys->GetNumberOfCells() / 10 +
    inStrips->GetNumberOfCells() / 10;
  ncells = (ncells < 100 ? 100 : ncells);
  newStrips->AllocateEstimate(ncells, 2 * (this->Resolution + 1));

  outCD->CopyNormalsOff();
  outCD->CopyAllocate(cd, ncells);

  RevolvePoints(input, newPts, &axis, this->Angle, this->DeltaRadius, this->Translation,
    this->Resolution, outPD, partialSweep);

  // To ensure that cell attributes are in consistent order with the
  // cellId's, we process the verts, lines, polys and strips in order.
  int type{ 0 };
  if (newLines) // there are verts which produce lines
  {
    vtkIdType ptId;
    for (cellId = 0; cellId < numCells && !abort; cellId++)
    {
      type = mesh->GetCellType(cellId);
      if (type == VTK_VERTEX || type == VTK_POLY_VERTEX)
      {
        mesh->GetCellPoints(cellId, npts, pts);
        for (int i = 0; i < npts; i++)
        {
          ptId = pts[i];
          newLines->InsertNextCell(this->Resolution + 1);
          for (int j = 0; j <= this->Resolution; j++)
          {
            newLines->InsertCellPoint(ptId + j * numPts);
          }
          outCD->CopyData(cd, cellId, newCellId++);
        }
      } // if a vertex or polyVertex
    } // for all cells
  } // if there are verts generating lines
  this->UpdateProgress(0.25);
  abort = this->CheckAbort();

  // If capping is on and if the points don't end up aligning,
  // copy 2D cells to output (plus create cap). Notice
  // that polygons are done first, then strips.
  //
  if (this->Capping &&
    (this->Angle != 360.0 || this->DeltaRadius != 0.0 || this->Translation != 0.0))
  {
    if (inPolys->GetNumberOfCells() > 0)
    {

      newPolys->AllocateCopy(inPolys);

      for (cellId = 0; cellId < numCells && !abort; cellId++)
      {
        type = mesh->GetCellType(cellId);
        if (type == VTK_TRIANGLE || type == VTK_QUAD || type == VTK_POLYGON)
        {
          mesh->GetCellPoints(cellId, npts, pts);
          newPolys->InsertNextCell(npts, pts);
          outCD->CopyData(cd, cellId, newCellId++);
          newPolys->InsertNextCell(npts);
          for (int i = 0; i < npts; i++)
          {
            newPolys->InsertCellPoint(pts[i] + this->Resolution * numPts);
          }
          outCD->CopyData(cd, cellId, newCellId++);
        }
      }
    }

    for (cellId = 0; cellId < numCells && !abort; cellId++)
    {
      type = mesh->GetCellType(cellId);
      if (type == VTK_TRIANGLE_STRIP)
      {
        mesh->GetCellPoints(cellId, npts, pts);
        newStrips->InsertNextCell(npts, pts);
        outCD->CopyData(cd, cellId, newCellId++);
        newStrips->InsertNextCell(npts);
        for (int i = 0; i < npts; i++)
        {
          newStrips->InsertCellPoint(pts[i] + this->Resolution * numPts);
        }
        outCD->CopyData(cd, cellId, newCellId++);
      }
    }
  } // if capping
  this->UpdateProgress(0.5);
  abort = this->CheckAbort();

  // Now process lines, polys and/or strips to produce strips
  //
  if (inLines->GetNumberOfCells() || inPolys->GetNumberOfCells() || inStrips->GetNumberOfCells())
  {
    vtkIdType p1, p2;
    vtkCell* edge;

    cellIds->Reserve(VTK_CELL_SIZE);

    for (cellId = 0; cellId < numCells && !abort; cellId++)
    {
      type = mesh->GetCellType(cellId);
      if (type == VTK_LINE || type == VTK_POLY_LINE)
      {
        mesh->GetCellPoints(cellId, npts, pts);
        for (int i = 0; i < (npts - 1); i++)
        {
          p1 = pts[i];
          p2 = pts[i + 1];
          newStrips->InsertNextCell(2 * (this->Resolution + 1));
          for (int j = 0; j <= this->Resolution; j++)
          {
            newStrips->InsertCellPoint(p2 + j * numPts);
            newStrips->InsertCellPoint(p1 + j * numPts);
          }
          outCD->CopyData(cd, cellId, newCellId++);
        }
      } // if a line

      else if (type == VTK_TRIANGLE || type == VTK_QUAD || type == VTK_POLYGON ||
        type == VTK_TRIANGLE_STRIP)
      { // create strips from boundary edges
        mesh->GetCell(cellId, cell);
        int numEdges = cell->GetNumberOfEdges();
        for (int i = 0; i < numEdges; i++)
        {
          edge = cell->GetEdge(i);
          for (int j = 0; j < (edge->GetNumberOfPoints() - 1); j++)
          {
            p1 = edge->PointIds->GetId(j);
            p2 = edge->PointIds->GetId(j + 1);
            mesh->GetCellEdgeNeighbors(cellId, p1, p2, cellIds);

            if (cellIds->GetNumberOfIds() < 1) // generate strip
            {
              newStrips->InsertNextCell(2 * (this->Resolution + 1));
              for (int k = 0; k <= this->Resolution; k++)
              {
                newStrips->InsertCellPoint(p2 + k * numPts);
                newStrips->InsertCellPoint(p1 + k * numPts);
              }
              outCD->CopyData(cd, cellId, newCellId++);
            } // if boundary edge
          } // for each sub-edge
        } // for each edge
      } // for each polygon or triangle strip
    } // for all cells

  } // if strips are being generated
  this->UpdateProgress(1.00);

  // Update ourselves and release memory
  //
  output->SetPoints(newPts);
  output->SetLines(newLines);
  output->SetPolys(newPolys);
  output->SetStrips(newStrips);

  output->Squeeze();
  return true;
}

bool vtkRotationalExtrusionFilter::ExecuteVolumetric(
  vtkSmartPointer<vtkDataSet> input, vtkSmartPointer<vtkUnstructuredGrid> output)
{

  AxisOfRevolution axis;
  for (vtkIdType i = 0; i < 3; i++)
  {
    axis.Position[i] = this->AxisPosition[i];
    axis.Direction[i] = this->RotationAxis[i];
  }

  vtkPointData* inPd{ input->GetPointData() };
  vtkCellData* inCd{ input->GetCellData() };
  vtkPointData* outPd{ output->GetPointData() };
  vtkCellData* outCd{ output->GetCellData() };

  vtkSmartPointer<vtkUnsignedCharArray> outTypes = vtkSmartPointer<vtkUnsignedCharArray>::New();
  vtkSmartPointer<vtkCellArray> outCells = vtkSmartPointer<vtkCellArray>::New();
  vtkSmartPointer<vtkPoints> outPts = vtkSmartPointer<vtkPoints>::New();

  // Check to see that the input data is amenable to this operation
  vtkSmartPointer<vtkCellIterator> it;
  it.TakeReference(input->NewCellIterator());

  for (it->InitTraversal(); !it->IsDoneWithTraversal(); it->GoToNextCell())
  {
    int cellDimension{ it->GetCellDimension() };
    if (cellDimension > 2)
    {
      vtkErrorMacro(<< "All cells must have a topological dimension < 2.");
      return false;
    }
  }

  // Set up output points
  if (this->OutputPointsPrecision == vtkAlgorithm::DEFAULT_PRECISION)
  {
    if (auto inputPointSet = vtkPointSet::SafeDownCast(input))
    {
      outPts->SetDataType(inputPointSet->GetPoints()->GetDataType());
    }
    else
    {
      outPts->SetDataType(VTK_FLOAT);
    }
  }
  else if (this->OutputPointsPrecision == vtkAlgorithm::SINGLE_PRECISION)
  {
    outPts->SetDataType(VTK_FLOAT);
  }
  else if (this->OutputPointsPrecision == vtkAlgorithm::DOUBLE_PRECISION)
  {
    outPts->SetDataType(VTK_DOUBLE);
  }

  // Determine whether or not the sweep angle is a full 2*pi
  // or if the radius or translation is changing.
  // If so, the extrusion is partial (does not connect back to the initial surface).
  // Thus, we need to add a set of points at the end of the sweep to close the surface.
  // We also need to add a set of points
  bool partialSweep = ((fabs(360. - fabs(this->Angle)) > 1024 * VTK_DBL_EPSILON) ||
    (this->DeltaRadius != 0.0) || (this->Translation != 0.0));

  // Set up output points and point data
  // add one resolution step if partialSweep is true to close the surface
  outPts->SetNumberOfPoints(
    input->GetNumberOfPoints() * (this->Resolution + static_cast<int>(partialSweep)));
  outPd->CopyAllocate(
    inPd, input->GetNumberOfPoints() * (this->Resolution + static_cast<int>(partialSweep)));

  // Set up output cell data
  vtkIdType nNewCells{ input->GetNumberOfCells() * this->Resolution };
  outCd->CopyAllocate(inCd, nNewCells);

  RevolvePoints(input, outPts, &axis, this->Angle, this->DeltaRadius, this->Translation,
    this->Resolution, outPd, partialSweep);

  it.TakeReference(input->NewCellIterator());
  for (it->InitTraversal(); !it->IsDoneWithTraversal(); it->GoToNextCell())
  {
    if (this->CheckAbort())
    {
      break;
    }
    if (RevolveCell(it->GetCellType(), it->GetPointIds(), input->GetNumberOfPoints(),
          this->Resolution, outCells, outTypes, inCd, it->GetCellId(), outCd, partialSweep) == 1)
    {
      vtkWarningMacro(<< "No method for revolving cell type " << it->GetCellType()
                      << ". Skipping.");
    }
  }

  output->SetPoints(outPts);
  output->SetCells(outTypes, outCells);
  output->Squeeze();

  return true;
}

//------------------------------------------------------------------------------
void vtkRotationalExtrusionFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Resolution: " << this->Resolution << "\n";
  os << indent << "Capping: " << (this->Capping ? "On\n" : "Off\n");
  os << indent << "Angle: " << this->Angle << "\n";
  os << indent << "Translation: " << this->Translation << "\n";
  os << indent << "Delta Radius: " << this->DeltaRadius << "\n";
  os << indent << "Rotation axis: (" << this->RotationAxis[0] << ", " << this->RotationAxis[1]
     << ", " << this->RotationAxis[2] << ")\n";
  os << indent << "AxisPosition: (" << this->AxisPosition[0] << ", " << this->AxisPosition[1]
     << ", " << this->AxisPosition[2] << ")\n";
  os << indent << "Volumetric: " << this->Volumetric << "\n";
  os << indent << "OutputPointsPrecision: " << this->OutputPointsPrecision << "\n";
}
VTK_ABI_NAMESPACE_END
