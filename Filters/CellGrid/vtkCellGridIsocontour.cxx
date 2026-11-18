// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCellGridIsocontour.h"

#include "vtkBoundingBox.h"
#include "vtkCellArray.h"
#include "vtkCellAttribute.h"
#include "vtkCellGrid.h"
#include "vtkCellGridChangeBasis.h"
#include "vtkCellGridEvaluator.h"
#include "vtkCellGridIsocontourPoints.h"
#include "vtkCellGridToUnstructuredGrid.h"
#include "vtkContourFilter.h"
#include "vtkDGEdge.h"
#include "vtkDGTri.h"
#include "vtkDGVert.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkFiltersCellGrid.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSMPTools.h"
#include "vtkStaticCleanPolyData.h"
#include "vtkStringToken.h"
#include "vtkTypeUInt32Array.h"
#include "vtkTypeUInt64Array.h"
#include "vtkUnstructuredGrid.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <unordered_map>
#include <utility>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN

using namespace vtk::literals;

vtkStandardNewMacro(vtkCellGridIsocontour);
vtkStandardNewMacro(vtkCellGridIsocontour::Query);
vtkCxxSetObjectMacro(vtkCellGridIsocontour::Query, CellAttribute, vtkCellAttribute);

vtkCellGridIsocontour::Query::~Query()
{
  this->SetCellAttribute(nullptr);
}

void vtkCellGridIsocontour::Query::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "CellAttribute: " << this->CellAttribute << "\n";
  os << indent << "Isovalue: " << this->Isovalue << "\n";
  os << indent << "Candidates: " << this->Candidates.size() << " cell types\n";
  os << indent << "MaximumOrder: " << this->MaximumOrder << "\n";
}

bool vtkCellGridIsocontour::Query::Initialize()
{
  if (!this->Superclass::Initialize())
  {
    return false;
  }
  this->Candidates.clear();
  this->MaximumOrder = 0;
  return true;
}

void vtkCellGridIsocontour::Query::AddCandidateCells(
  vtkStringToken cellType, vtkIdTypeArray* cellIds, int order)
{
  this->Candidates[cellType] = cellIds;
  this->MaximumOrder = std::max(this->MaximumOrder, order);
}

namespace
{

/// Return a copy of \a grid with \a attribute converted to the
/// Bernstein-Bezier basis, or \a grid itself if it already is. Returns null on
/// failure.
vtkSmartPointer<vtkCellGrid> ToBezier(vtkCellGrid* grid, vtkCellAttribute* attribute)
{
  for (const auto& cellTypeName : grid->CellTypeArray())
  {
    auto info = attribute->GetCellTypeInfo(cellTypeName);
    if (!info.FunctionSpace.IsValid())
    {
      continue;
    }
    if (info.FunctionSpace == "Bezier"_token)
    {
      return grid;
    }
    vtkNew<vtkCellGridChangeBasis> converter;
    converter->SetInputDataObject(grid);
    converter->SetCellAttribute(attribute);
    converter->SetSourceFunctionSpace(info.FunctionSpace.Data().c_str());
    converter->SetSourceBasis(info.Basis.Data().c_str());
    converter->SetSourceOrder(info.Order);
    converter->SetTargetFunctionSpace("Bezier");
    converter->SetTargetBasis("A");
    converter->SetTargetOrder(info.Order);
    converter->Update();
    return vtkCellGrid::SafeDownCast(converter->GetOutputDataObject(0));
  }
  return grid;
}

/// Transcribe the \a cells of \a grid into linear cells, each subdivided
/// \a subdivisionLevel times (see vtkCellGridToUnstructuredGrid).
///
/// The cells are transcribed apart: the linear cells of different input cells
/// share no points, so every point has the attributes of its own input cell
/// only, and it reports where in that cell it is ("CellType", "CellId" and
/// "RST").
vtkSmartPointer<vtkUnstructuredGrid> TranscribeCells(vtkCellGrid* grid,
  const vtkCellGridIsocontour::Query::CandidateCells& cells, int subdivisionLevel)
{
  vtkNew<vtkCellGridToUnstructuredGrid> transcriber;
  transcriber->SetInputDataObject(grid);
  transcriber->SetSubdivisionLevel(subdivisionLevel);
  transcriber->MergePointsOff();
  transcriber->GenerateInputCellArraysOn();
  for (const auto& entry : cells)
  {
    transcriber->SetCellsToTranscribe(entry.first, entry.second);
  }
  transcriber->Update();
  return transcriber->GetOutput();
}

/// Contour the point-data array \a attributeName of \a mesh (transcribed by
/// TranscribeCells()) at \a isovalue, and merge the points of the contour that are
/// within \a tolerance of each other.
///
/// The contour interpolates the point-data arrays along the edges it cuts, and an
/// edge joins two points of the same input cell (see TranscribeCells()), so the
/// contour's points have the "CellType" and "CellId" of that cell (the same value
/// at both ends) and "RST", the parametric coordinates, interpolated between
/// those of the ends. A point that several points merge into keeps the arrays of
/// one of them (they are not averaged), so it stays in one cell, at the
/// parametric coordinates of that cell.
///
/// The contour's cell data is not used to carry the input cell: vtkContourFilter
/// does not keep it in order where the contour has cells of more than one
/// dimension.
vtkSmartPointer<vtkPolyData> ContourMesh(
  vtkUnstructuredGrid* mesh, const char* attributeName, double isovalue, double tolerance)
{
  // The contour interpolates every point-data array along the edges it cuts, and
  // only the contoured one and those that say where the points are are needed.
  auto* pointData = mesh->GetPointData();
  for (int array = pointData->GetNumberOfArrays() - 1; array >= 0; --array)
  {
    const char* name = pointData->GetArrayName(array);
    if (std::strcmp(name, attributeName) != 0 && std::strcmp(name, "RST") != 0 &&
      std::strcmp(name, "CellType") != 0 && std::strcmp(name, "CellId") != 0)
    {
      pointData->RemoveArray(array);
    }
  }

  vtkNew<vtkContourFilter> contour;
  contour->SetInputData(mesh);
  contour->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, attributeName);
  contour->SetValue(0, isovalue);
  contour->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);
  contour->GenerateTrianglesOn();
  contour->ComputeNormalsOff();
  contour->ComputeScalarsOff();

  vtkNew<vtkStaticCleanPolyData> merge;
  merge->SetInputConnection(contour->GetOutputPort());
  merge->ToleranceIsAbsoluteOn();
  merge->SetAbsoluteTolerance(tolerance);
  merge->AveragePointDataOff();
  merge->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);
  merge->Update();
  return merge->GetOutput();
}

/// Where in the input each point of a contour lies.
///
/// vtkCellGridEvaluator interpolates at points whose cells and parametric
/// coordinates are known when the points of each cell type are contiguous (see
/// vtkCellGridEvaluator::InterpolateCellParameters()). The contour's points are:
/// the cells are transcribed one cell type after the other, so their points are,
/// and neither the contour nor the merge of its points reorders them.
class LocatedContourPoints
{
public:
  /// Read where each point of \a contour lies in \a grid from its "CellType",
  /// "CellId" and "RST" arrays (see ContourMesh()). Return false if an array is
  /// missing, a point is not in a cell of \a grid, or the points of a cell type
  /// are not contiguous.
  bool Initialize(vtkPolyData* contour, vtkCellGrid* grid)
  {
    auto* pointData = contour->GetPointData();
    this->CellTypes = vtkTypeUInt32Array::SafeDownCast(pointData->GetArray("CellType"));
    this->CellIds = vtkTypeUInt64Array::SafeDownCast(pointData->GetArray("CellId"));
    this->Parameters = pointData->GetArray("RST");
    if (!this->CellTypes || !this->CellIds || !this->Parameters)
    {
      return false;
    }

    std::unordered_map<vtkTypeUInt32, vtkIdType> numCellsOfType;
    for (const auto& cellTypeName : grid->CellTypeArray())
    {
      numCellsOfType[cellTypeName.GetId()] = grid->GetCellType(cellTypeName)->GetNumberOfCells();
    }

    // Every point must be in a cell of the input, and the points of a cell type
    // must follow each other. The evaluator's description of them has an entry
    // for each cell type, and a last one, of the invalid hash, that only marks the
    // end of the others.
    const vtkIdType numPoints = contour->GetNumberOfPoints();
    std::vector<vtkTypeUInt32> types;
    std::vector<vtkTypeUInt64> offsets;
    for (vtkIdType point = 0; point < numPoints; ++point)
    {
      const vtkTypeUInt32 type = this->CellTypes->GetValue(point);
      auto cellsOfType = numCellsOfType.find(type);
      if (cellsOfType == numCellsOfType.end() ||
        static_cast<vtkIdType>(this->CellIds->GetValue(point)) >= cellsOfType->second)
      {
        return false;
      }
      if (types.empty() || types.back() != type)
      {
        if (std::find(types.begin(), types.end(), type) != types.end())
        {
          return false;
        }
        types.push_back(type);
        offsets.push_back(point);
      }
    }
    types.push_back(vtkStringToken::InvalidHash());
    offsets.push_back(numPoints);
    this->RangeTypes->SetNumberOfValues(static_cast<vtkIdType>(types.size()));
    this->RangeOffsets->SetNumberOfValues(static_cast<vtkIdType>(types.size()));
    for (std::size_t range = 0; range < types.size(); ++range)
    {
      this->RangeTypes->SetValue(static_cast<vtkIdType>(range), types[range]);
      this->RangeOffsets->SetValue(static_cast<vtkIdType>(range), offsets[range]);
    }
    return true;
  }

  /// Evaluate \a attribute of \a grid at the points and return its values, or
  /// null if it cannot be evaluated. If \a gradients is given, it is set to the
  /// attribute's gradients there (see vtkCellGridEvaluator::ComputeGradients).
  vtkSmartPointer<vtkDataArray> Evaluate(vtkCellGrid* grid, vtkCellAttribute* attribute,
    vtkSmartPointer<vtkDataArray>* gradients = nullptr) const
  {
    vtkNew<vtkCellGridEvaluator> evaluator;
    evaluator->SetCellAttribute(attribute);
    evaluator->SetComputeGradients(gradients != nullptr);
    evaluator->InterpolateCellParameters(
      this->RangeTypes, this->RangeOffsets, this->CellIds, this->Parameters);
    if (!grid->Query(evaluator))
    {
      return nullptr;
    }
    if (gradients)
    {
      *gradients = evaluator->GetInterpolatedGradients();
    }
    return evaluator->GetInterpolatedValues();
  }

  /// The type of the cell of each point, as the hash of its name ("CellType").
  vtkTypeUInt32Array* GetCellTypes() const { return this->CellTypes; }
  /// The index of the cell of each point among the cells of its type ("CellId").
  vtkTypeUInt64Array* GetCellIds() const { return this->CellIds; }
  /// The parametric coordinates of each point in its cell ("RST").
  vtkDataArray* GetParameters() const { return this->Parameters; }

private:
  /// The cell types and where their points begin, as vtkCellGridEvaluator takes them.
  vtkNew<vtkTypeUInt32Array> RangeTypes;
  vtkNew<vtkTypeUInt64Array> RangeOffsets;
  /// Where each point lies: the contour's own arrays.
  vtkSmartPointer<vtkTypeUInt32Array> CellTypes;
  vtkSmartPointer<vtkTypeUInt64Array> CellIds;
  vtkSmartPointer<vtkDataArray> Parameters;
};

/// Order the \a corners (point IDs) of a triangle so that its normal points, as
/// in vtkContourFilter, toward the lower values: against the sum of the
/// \a gradients at its corners. \a coordinates are those of the points.
void OrientAgainstGradients(
  std::array<vtkIdType, 3>& corners, vtkDataArray* coordinates, vtkDataArray* gradients)
{
  double p0[3], p1[3], p2[3];
  coordinates->GetTuple(corners[0], p0);
  coordinates->GetTuple(corners[1], p1);
  coordinates->GetTuple(corners[2], p2);
  double edge1[3], edge2[3], normal[3];
  vtkMath::Subtract(p1, p0, edge1);
  vtkMath::Subtract(p2, p0, edge2);
  vtkMath::Cross(edge1, edge2, normal);

  double gradientSum[3] = { 0., 0., 0. };
  for (const vtkIdType corner : corners)
  {
    double gradient[3];
    gradients->GetTuple(corner, gradient);
    vtkMath::Add(gradientSum, gradient, gradientSum);
  }

  // Flip the triangle if its normal points along the gradients.
  if (vtkMath::Dot(normal, gradientSum) > 0.)
  {
    std::swap(corners[1], corners[2]);
  }
}

/// Orient every triangle of \a triangles (an array of tuples of three point IDs)
/// against the \a gradients (see OrientAgainstGradients()).
void OrientTriangles(vtkDataArray* triangles, vtkDataArray* coordinates, vtkDataArray* gradients)
{
  vtkSMPTools::For(0, triangles->GetNumberOfTuples(),
    [&](vtkIdType begin, vtkIdType end)
    {
      for (vtkIdType tri = begin; tri < end; ++tri)
      {
        std::array<vtkIdType, 3> corners;
        for (int cc = 0; cc < 3; ++cc)
        {
          corners[cc] = static_cast<vtkIdType>(triangles->GetComponent(tri, cc));
        }
        OrientAgainstGradients(corners, coordinates, gradients);
        for (int cc = 0; cc < 3; ++cc)
        {
          triangles->SetComponent(tri, cc, static_cast<double>(corners[cc]));
        }
      }
    });
}

/// Add the cells of the type \a CellMetadata, with the corners \a connectivity,
/// to \a output, and return their type.
template <typename CellMetadata>
vtkStringToken AddCells(vtkCellGrid* output, vtkDataArray* connectivity)
{
  auto* cells = output->AddCellMetadata<CellMetadata>();
  vtkStringToken type = cells->GetClassName();
  output->GetAttributes(type)->SetScalars(connectivity);
  cells->GetCellSpec().Connectivity = connectivity;
  return type;
}

/// Return \a attribute as an attribute of the \a cells (their connectivity, by
/// type) of a cell-grid, linear across each, with the \a pointValues at its points.
vtkSmartPointer<vtkCellAttribute> LinearAttribute(vtkStringToken name, vtkStringToken space,
  int numComponents, const std::unordered_map<vtkStringToken, vtkSmartPointer<vtkDataArray>>& cells,
  vtkDataArray* pointValues)
{
  auto attribute = vtkSmartPointer<vtkCellAttribute>::New();
  attribute->Initialize(name, space, numComponents);
  for (const auto& entry : cells)
  {
    vtkCellAttribute::CellTypeInfo info;
    info.DOFSharing = "points"_token;
    info.FunctionSpace = "HGRAD"_token;
    info.Basis = "C"_token;
    info.Order = 1;
    info.ArraysByRole["connectivity"_token] = entry.second;
    info.ArraysByRole["values"_token] = pointValues;
    attribute->SetCellTypeInfo(entry.first, info);
  }
  return attribute;
}

} // anonymous namespace

vtkCellGridIsocontour::vtkCellGridIsocontour()
{
  vtkFiltersCellGrid::RegisterCellsAndResponders();
  this->SetNumberOfOutputPorts(2);
}

int vtkCellGridIsocontour::FillOutputPortInformation(int port, vtkInformation* info)
{
  if (port == 1)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
    return 1;
  }
  return this->Superclass::FillOutputPortInformation(port, info);
}

vtkPolyData* vtkCellGridIsocontour::GetIsocontourPointsOutput()
{
  return vtkPolyData::SafeDownCast(this->GetOutputDataObject(1));
}

void vtkCellGridIsocontour::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "SubdivisionLevel: " << this->SubdivisionLevel << "\n";
  os << indent << "Tolerance: " << this->Tolerance << "\n";
  os << indent << "ParticleRelaxationIterations: " << this->ParticleRelaxationIterations << "\n";
  os << indent << "OutputIsocontourPoints: " << (this->OutputIsocontourPoints ? "On" : "Off")
     << "\n";
  os << indent << "InterpolateAttributes: " << (this->InterpolateAttributes ? "On" : "Off") << "\n";
  os << indent << "Request:\n";
  this->Request->PrintSelf(os, indent.GetNextIndent());
}

void vtkCellGridIsocontour::SetIsovalue(double isovalue)
{
  if (this->Request->GetIsovalue() == isovalue)
  {
    return;
  }
  this->Request->SetIsovalue(isovalue);
  this->Modified();
}

int vtkCellGridIsocontour::RequestData(
  vtkInformation* vtkNotUsed(request), vtkInformationVector** inInfo, vtkInformationVector* ouInfo)
{
  auto* input = vtkCellGrid::GetData(inInfo[0]);
  auto* output = vtkCellGrid::GetData(ouInfo);
  if (!input)
  {
    vtkWarningMacro("Empty input.");
    return 1;
  }
  if (!output)
  {
    vtkErrorMacro("Empty output.");
    return 0;
  }
  output->Initialize();

  auto* attribute = this->GetInputCellAttributeToProcess(0, input);
  if (!attribute)
  {
    vtkErrorMacro("No cell-attribute to contour.");
    return 0;
  }
  if (attribute->GetNumberOfComponents() != 1)
  {
    vtkErrorMacro("Only scalar cell-attributes can be contoured.");
    return 0;
  }

  auto bezierGrid = ToBezier(input, attribute);
  auto* bezierAttribute =
    bezierGrid ? bezierGrid->GetCellAttributeByName(attribute->GetName().Data()) : nullptr;
  if (!bezierAttribute)
  {
    vtkErrorMacro("Could not convert the cell-attribute to the Bernstein-Bezier basis.");
    return 0;
  }
  // The points on the isocontour, if requested, for the second output.
  if (vtkPolyData* pointsOutput = vtkPolyData::GetData(ouInfo, 1))
  {
    pointsOutput->Initialize();
    if (this->OutputIsocontourPoints)
    {
      vtkNew<vtkCellGridIsocontourPoints> pointsFilter;
      pointsFilter->SetInputDataObject(bezierGrid);
      pointsFilter->SetCellAttribute(bezierAttribute);
      pointsFilter->SetIsovalue(this->GetIsovalue());
      pointsFilter->SetParticleRelaxationIterations(this->ParticleRelaxationIterations);
      pointsFilter->SetInterpolateAttributes(this->InterpolateAttributes);
      pointsFilter->Update();
      pointsOutput->ShallowCopy(pointsFilter->GetOutput());
    }
  }

  // Step 1: the cells that may hold the isosurface, judged by their Bezier
  // control values.
  this->Request->SetCellAttribute(bezierAttribute);
  const bool found = bezierGrid->Query(this->Request);
  // The attribute was only needed to answer the query (and setting it must not
  // make the filter execute again).
  this->Request->SetCellAttribute(nullptr);
  if (!found)
  {
    vtkErrorMacro("Could not find the cells that may hold the isosurface.");
    return 0;
  }
  if (this->Request->GetCandidateCells().empty())
  {
    return 1;
  }

  // Step 2: contour those cells as linear cells. Unless the user chose a level,
  // subdivide once less than the order of the basis.
  const int subdivisionLevel = this->SubdivisionLevel >= 0
    ? this->SubdivisionLevel
    : std::max(this->Request->GetMaximumOrder() - 1, 0);
  auto mesh = TranscribeCells(input, this->Request->GetCandidateCells(), subdivisionLevel);
  const char* attributeName = attribute->GetName().Data().c_str();
  if (!mesh->GetPointData()->GetArray(attributeName))
  {
    vtkErrorMacro("The transcribed cells have no values of the cell-attribute.");
    return 0;
  }
  std::array<double, 6> bounds;
  input->GetBounds(bounds.data());
  const double tolerance = this->Tolerance * vtkBoundingBox(bounds.data()).GetDiagonalLength();
  auto contour = ContourMesh(mesh, attributeName, this->GetIsovalue(), tolerance);
  if (contour->GetNumberOfPoints() == 0 || contour->GetNumberOfCells() == 0)
  {
    return 1;
  }

  // Step 3: place every point at its parametric coordinates in its cell, and
  // evaluate the contoured attribute and its gradient there.
  LocatedContourPoints locatedPoints;
  if (!locatedPoints.Initialize(contour, input))
  {
    vtkErrorMacro("The contour does not say where its points are in the input.");
    return 0;
  }
  auto coordinates = locatedPoints.Evaluate(input, input->GetShapeAttribute());
  vtkSmartPointer<vtkDataArray> gradients;
  auto contouredValues = locatedPoints.Evaluate(input, attribute, &gradients);
  if (!coordinates || !contouredValues || !gradients)
  {
    vtkErrorMacro("Could not evaluate the input at the points of the contour.");
    return 0;
  }
  coordinates->SetName("coords");

  // Step 4: output the contour's triangles (oriented by the gradients), lines
  // and vertices. Each cell's corners are the tuples of the contour's
  // connectivity, read as many components at a time as it has corners.
  std::unordered_map<vtkStringToken, vtkSmartPointer<vtkDataArray>> outputCells;
  auto* triangleCells = contour->GetPolys();
  if (triangleCells->GetNumberOfCells() > 0)
  {
    auto* triangles = triangleCells->GetConnectivityArray();
    triangles->SetName("connectivity");
    triangles->SetNumberOfComponents(3);
    OrientTriangles(triangles, coordinates, gradients);
    outputCells[AddCells<vtkDGTri>(output, triangles)] = triangles;
  }
  auto* lineCells = contour->GetLines();
  if (lineCells->GetNumberOfCells() > 0)
  {
    auto* lines = lineCells->GetConnectivityArray();
    lines->SetName("connectivity");
    lines->SetNumberOfComponents(2);
    outputCells[AddCells<vtkDGEdge>(output, lines)] = lines;
  }
  auto* vertexCells = contour->GetVerts();
  if (vertexCells->GetNumberOfCells() > 0)
  {
    auto* vertices = vertexCells->GetConnectivityArray();
    vertices->SetName("connectivity");
    outputCells[AddCells<vtkDGVert>(output, vertices)] = vertices;
  }

  auto* pointGroup = output->GetAttributes("points"_token);
  pointGroup->SetScalars(coordinates);
  output->SetShapeAttribute(
    LinearAttribute("shape"_token, "ℝ³"_token, 3, outputCells, coordinates));

  // Step 5: where each point is in the input, and, if requested, every
  // attribute of the input but its shape evaluated there and interpolated
  // linearly across each output cell.
  pointGroup->AddArray(locatedPoints.GetCellTypes());
  pointGroup->AddArray(locatedPoints.GetCellIds());
  pointGroup->AddArray(locatedPoints.GetParameters());
  if (this->InterpolateAttributes)
  {
    for (const auto& inputAttribute : input->GetCellAttributeList())
    {
      if (inputAttribute == input->GetShapeAttribute())
      {
        continue;
      }
      vtkSmartPointer<vtkDataArray> pointValues = contouredValues;
      if (inputAttribute != attribute)
      {
        pointValues = locatedPoints.Evaluate(input, inputAttribute);
      }
      if (!pointValues)
      {
        vtkWarningMacro(
          "Could not evaluate \"" << inputAttribute->GetName().Data() << "\"; skipping it.");
        continue;
      }
      pointValues->SetName(inputAttribute->GetName().Data().c_str());
      pointGroup->AddArray(pointValues);
      output->AddCellAttribute(
        LinearAttribute(inputAttribute->GetName(), inputAttribute->GetSpace(),
          inputAttribute->GetNumberOfComponents(), outputCells, pointValues));
    }
  }
  return 1;
}

VTK_ABI_NAMESPACE_END
