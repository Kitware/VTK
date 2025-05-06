//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <viskores/cont/Algorithm.h>

#include <viskores/exec/CellEdge.h>
#include <viskores/filter/entity_extraction/ExternalFaces.h>
#include <viskores/rendering/CanvasRayTracer.h>
#include <viskores/rendering/MapperRayTracer.h>
#include <viskores/rendering/MapperWireframer.h>
#include <viskores/rendering/Wireframer.h>
#include <viskores/worklet/DispatcherMapField.h>
#include <viskores/worklet/DispatcherMapTopology.h>
#include <viskores/worklet/ScatterCounting.h>
#include <viskores/worklet/WorkletMapField.h>
#include <viskores/worklet/WorkletMapTopology.h>

namespace viskores
{
namespace rendering
{
namespace
{

class CreateConnectivity : public viskores::worklet::WorkletMapField
{
public:
  VISKORES_CONT
  CreateConnectivity() {}

  using ControlSignature = void(FieldIn, WholeArrayOut);

  using ExecutionSignature = void(_1, _2);

  template <typename ConnPortalType>
  VISKORES_EXEC void operator()(const viskores::Id& i, ConnPortalType& connPortal) const
  {
    connPortal.Set(i * 2 + 0, i);
    connPortal.Set(i * 2 + 1, i + 1);
  }
}; // conn

class Convert1DCoordinates : public viskores::worklet::WorkletMapField
{
private:
  bool LogY;
  bool LogX;

public:
  VISKORES_CONT
  Convert1DCoordinates(bool logY, bool logX)
    : LogY(logY)
    , LogX(logX)
  {
  }

  using ControlSignature = void(FieldIn, FieldIn, FieldOut, FieldOut);

  using ExecutionSignature = void(_1, _2, _3, _4);
  template <typename ScalarType>
  VISKORES_EXEC void operator()(const viskores::Vec3f_32& inCoord,
                                const ScalarType& scalar,
                                viskores::Vec3f_32& outCoord,
                                viskores::Float32& fieldOut) const
  {
    //
    // Rendering supports lines based on a cellSetStructured<1>
    // where only the x coord matters. It creates a y based on
    // the scalar values and connects all the points with lines.
    // So, we need to convert it back to something that can
    // actually be rendered.
    //
    outCoord[0] = inCoord[0];
    outCoord[1] = static_cast<viskores::Float32>(scalar);
    outCoord[2] = 0.f;
    if (LogY)
    {
      outCoord[1] = viskores::Log10(outCoord[1]);
    }
    if (LogX)
    {
      outCoord[0] = viskores::Log10(outCoord[0]);
    }
    // all lines have the same color
    fieldOut = 1.f;
  }
}; // convert coords

#if defined(VISKORES_MSVC)
#pragma warning(push)
#pragma warning(disable : 4127) //conditional expression is constant
#endif
struct EdgesCounter : public viskores::worklet::WorkletVisitCellsWithPoints
{
  using ControlSignature = void(CellSetIn cellSet, FieldOutCell numEdges);
  using ExecutionSignature = _2(CellShape shape, PointCount numPoints);
  using InputDomain = _1;

  template <typename CellShapeTag>
  VISKORES_EXEC viskores::IdComponent operator()(CellShapeTag shape,
                                                 viskores::IdComponent numPoints) const
  {
    //TODO: Remove the if/then with templates.
    if (shape.Id == viskores::CELL_SHAPE_LINE)
    {
      return 1;
    }
    else
    {
      viskores::IdComponent numEdges;
      viskores::exec::CellEdgeNumberOfEdges(numPoints, shape, numEdges);
      return numEdges;
    }
  }
}; // struct EdgesCounter

struct EdgesExtracter : public viskores::worklet::WorkletVisitCellsWithPoints
{
  using ControlSignature = void(CellSetIn cellSet, FieldOutCell edgeIndices);
  using ExecutionSignature = void(CellShape, PointIndices, VisitIndex, _2);
  using InputDomain = _1;
  using ScatterType = viskores::worklet::ScatterCounting;

  VISKORES_CONT
  template <typename CountArrayType>
  static ScatterType MakeScatter(const CountArrayType& counts)
  {
    return ScatterType(counts);
  }

  template <typename CellShapeTag, typename PointIndexVecType, typename EdgeIndexVecType>
  VISKORES_EXEC void operator()(CellShapeTag shape,
                                const PointIndexVecType& pointIndices,
                                viskores::IdComponent visitIndex,
                                EdgeIndexVecType& edgeIndices) const
  {
    //TODO: Remove the if/then with templates.
    viskores::Id p1, p2;
    if (shape.Id == viskores::CELL_SHAPE_LINE)
    {
      p1 = pointIndices[0];
      p2 = pointIndices[1];
    }
    else
    {
      viskores::IdComponent localEdgeIndex;
      viskores::exec::CellEdgeLocalIndex(
        pointIndices.GetNumberOfComponents(), 0, visitIndex, shape, localEdgeIndex);
      p1 = pointIndices[localEdgeIndex];
      viskores::exec::CellEdgeLocalIndex(
        pointIndices.GetNumberOfComponents(), 1, visitIndex, shape, localEdgeIndex);
      p2 = pointIndices[localEdgeIndex];
    }
    // These indices need to be arranged in a definite order, as they will later be sorted to
    // detect duplicates
    edgeIndices[0] = p1 < p2 ? p1 : p2;
    edgeIndices[1] = p1 < p2 ? p2 : p1;
  }
}; // struct EdgesExtracter

#if defined(VISKORES_MSVC)
#pragma warning(pop)
#endif
} // namespace

struct MapperWireframer::InternalsType
{
  InternalsType()
    : InternalsType(nullptr, false, false)
  {
  }

  InternalsType(viskores::rendering::Canvas* canvas, bool showInternalZones, bool isOverlay)
    : Canvas(canvas)
    , ShowInternalZones(showInternalZones)
    , IsOverlay(isOverlay)
    , CompositeBackground(true)
  {
  }

  viskores::rendering::Canvas* Canvas;
  bool ShowInternalZones;
  bool IsOverlay;
  bool CompositeBackground;
}; // struct MapperWireframer::InternalsType

MapperWireframer::MapperWireframer()
  : Internals(new InternalsType(nullptr, false, false))
{
}

MapperWireframer::~MapperWireframer() {}

viskores::rendering::Canvas* MapperWireframer::GetCanvas() const
{
  return this->Internals->Canvas;
}

void MapperWireframer::SetCanvas(viskores::rendering::Canvas* canvas)
{
  this->Internals->Canvas = canvas;
}

bool MapperWireframer::GetShowInternalZones() const
{
  return this->Internals->ShowInternalZones;
}

void MapperWireframer::SetShowInternalZones(bool showInternalZones)
{
  this->Internals->ShowInternalZones = showInternalZones;
}

bool MapperWireframer::GetIsOverlay() const
{
  return this->Internals->IsOverlay;
}

void MapperWireframer::SetIsOverlay(bool isOverlay)
{
  this->Internals->IsOverlay = isOverlay;
}

void MapperWireframer::RenderCellsImpl(const viskores::cont::UnknownCellSet& inCellSet,
                                       const viskores::cont::CoordinateSystem& coords,
                                       const viskores::cont::Field& inScalarField,
                                       const viskores::cont::ColorTable& colorTable,
                                       const viskores::rendering::Camera& camera,
                                       const viskores::Range& scalarRange,
                                       const viskores::cont::Field& ghostField)
{
  viskores::cont::UnknownCellSet cellSet = inCellSet;

  bool is1D = cellSet.CanConvert<viskores::cont::CellSetStructured<1>>();
  bool is2D = cellSet.CanConvert<viskores::cont::CellSetStructured<2>>();

  viskores::cont::CoordinateSystem actualCoords = coords;
  viskores::cont::Field actualField = inScalarField;
  viskores::cont::Field actualGhostField = ghostField;

  if (is1D)
  {

    const bool isSupportedField = inScalarField.IsPointField();
    if (!isSupportedField)
    {
      throw viskores::cont::ErrorBadValue(
        "WireFramer: field must be associated with points for 1D cell set");
    }
    viskores::cont::ArrayHandle<viskores::Vec3f_32> newCoords;
    viskores::cont::ArrayHandle<viskores::Float32> newScalars;
    //
    // Convert the cell set into something we can draw
    //
    viskores::worklet::DispatcherMapField<Convert1DCoordinates>(
      Convert1DCoordinates(this->LogarithmY, this->LogarithmX))
      .Invoke(coords.GetData(),
              viskores::rendering::raytracing::GetScalarFieldArray(inScalarField),
              newCoords,
              newScalars);

    actualCoords = viskores::cont::CoordinateSystem("coords", newCoords);
    actualField = viskores::cont::Field(
      inScalarField.GetName(), viskores::cont::Field::Association::Points, newScalars);

    viskores::Id numCells = cellSet.GetNumberOfCells();
    viskores::cont::ArrayHandle<viskores::Id> conn;
    viskores::cont::ArrayHandleCounting<viskores::Id> iter =
      viskores::cont::make_ArrayHandleCounting(viskores::Id(0), viskores::Id(1), numCells);
    conn.Allocate(numCells * 2);
    viskores::worklet::DispatcherMapField<CreateConnectivity>(CreateConnectivity())
      .Invoke(iter, conn);

    viskores::cont::CellSetSingleType<> newCellSet;
    newCellSet.Fill(newCoords.GetNumberOfValues(), viskores::CELL_SHAPE_LINE, 2, conn);

    cellSet = viskores::cont::UnknownCellSet(newCellSet);
  }
  bool isLines = false;
  // Check for a cell set that is already lines
  // Since there is no need to de external faces or
  // render the depth of the mesh to hide internal zones
  if (cellSet.CanConvert<viskores::cont::CellSetSingleType<>>())
  {
    auto singleType = cellSet.AsCellSet<viskores::cont::CellSetSingleType<>>();
    isLines = singleType.GetCellShape(0) == viskores::CELL_SHAPE_LINE;
  }

  bool doExternalFaces = !(this->Internals->ShowInternalZones) && !isLines && !is1D && !is2D;
  if (doExternalFaces)
  {
    // If internal zones are to be hidden, the number of edges processed can be reduced by
    // running the external faces filter on the input cell set.
    viskores::cont::DataSet dataSet;
    dataSet.AddCoordinateSystem(actualCoords);
    dataSet.SetCellSet(inCellSet);
    dataSet.AddField(inScalarField);
    dataSet.AddField(ghostField);
    viskores::filter::entity_extraction::ExternalFaces externalFaces;
    externalFaces.SetCompactPoints(false);
    externalFaces.SetPassPolyData(true);
    viskores::cont::DataSet output = externalFaces.Execute(dataSet);
    cellSet = output.GetCellSet();
    actualField = output.GetField(inScalarField.GetName());
    actualGhostField = output.GetGhostCellField();
  }

  // Extract unique edges from the cell set.
  viskores::cont::ArrayHandle<viskores::IdComponent> counts;
  viskores::cont::ArrayHandle<viskores::Id2> edgeIndices;
  viskores::worklet::DispatcherMapTopology<EdgesCounter>().Invoke(cellSet, counts);
  viskores::worklet::DispatcherMapTopology<EdgesExtracter> extractDispatcher(
    EdgesExtracter::MakeScatter(counts));
  extractDispatcher.Invoke(cellSet, edgeIndices);
  viskores::cont::Algorithm::Sort(edgeIndices);
  viskores::cont::Algorithm::Unique(edgeIndices);

  Wireframer renderer(
    this->Internals->Canvas, this->Internals->ShowInternalZones, this->Internals->IsOverlay);
  // Render the cell set using a raytracer, on a separate canvas, and use the generated depth
  // buffer, which represents the solid mesh, to avoid drawing on the internal zones
  bool renderDepth =
    !(this->Internals->ShowInternalZones) && !(this->Internals->IsOverlay) && !isLines && !is1D;
  if (renderDepth)
  {
    CanvasRayTracer canvas(this->Internals->Canvas->GetWidth(),
                           this->Internals->Canvas->GetHeight());
    canvas.SetBackgroundColor(viskores::rendering::Color::white);
    canvas.Clear();
    MapperRayTracer raytracer;
    raytracer.SetCanvas(&canvas);
    raytracer.SetActiveColorTable(colorTable);
    raytracer.RenderCells(
      cellSet, actualCoords, actualField, colorTable, camera, scalarRange, actualGhostField);
    renderer.SetSolidDepthBuffer(canvas.GetDepthBuffer());
  }
  else
  {
    renderer.SetSolidDepthBuffer(this->Internals->Canvas->GetDepthBuffer());
  }

  renderer.SetCamera(camera);
  renderer.SetColorMap(this->ColorMap);
  renderer.SetData(actualCoords, edgeIndices, actualField, scalarRange);
  renderer.Render();

  if (this->Internals->CompositeBackground)
  {
    this->Internals->Canvas->BlendBackground();
  }
}

void MapperWireframer::SetCompositeBackground(bool on)
{
  this->Internals->CompositeBackground = on;
}

viskores::rendering::Mapper* MapperWireframer::NewCopy() const
{
  return new viskores::rendering::MapperWireframer(*this);
}
}
} // namespace viskores::rendering
