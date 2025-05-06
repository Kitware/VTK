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

#include <viskores/exec/CellEdge.h>
#include <viskores/exec/CellFace.h>

#include <viskores/CellShape.h>
#include <viskores/CellTraits.h>

#include <viskores/exec/FunctorBase.h>

#include <viskores/testing/Testing.h>

#include <set>
#include <vector>

#define CHECK_CALL(call) \
  VISKORES_TEST_ASSERT((call) == viskores::ErrorCode::Success, "Call resulted in error.")

namespace
{

using EdgeType = viskores::IdComponent2;

void MakeEdgeCanonical(EdgeType& edge)
{
  if (edge[1] < edge[0])
  {
    std::swap(edge[0], edge[1]);
  }
}

struct TestCellFacesFunctor
{
  template <typename CellShapeTag>
  void DoTest(viskores::IdComponent numPoints,
              CellShapeTag shape,
              viskores::CellTopologicalDimensionsTag<3>) const
  {
    std::vector<viskores::Id> pointIndexProxyBuffer(static_cast<std::size_t>(numPoints));
    for (std::size_t index = 0; index < pointIndexProxyBuffer.size(); ++index)
    {
      pointIndexProxyBuffer[index] = static_cast<viskores::Id>(1000000 - index);
    }
    viskores::VecCConst<viskores::Id> pointIndexProxy(&pointIndexProxyBuffer.at(0), numPoints);

    viskores::IdComponent numEdges;
    CHECK_CALL(viskores::exec::CellEdgeNumberOfEdges(numPoints, shape, numEdges));
    VISKORES_TEST_ASSERT(numEdges > 0, "No edges?");

    std::set<EdgeType> edgeSet;
    for (viskores::IdComponent edgeIndex = 0; edgeIndex < numEdges; edgeIndex++)
    {
      EdgeType edge;
      CHECK_CALL(viskores::exec::CellEdgeLocalIndex(numPoints, 0, edgeIndex, shape, edge[0]));
      CHECK_CALL(viskores::exec::CellEdgeLocalIndex(numPoints, 1, edgeIndex, shape, edge[1]));
      VISKORES_TEST_ASSERT(edge[0] >= 0, "Bad index in edge.");
      VISKORES_TEST_ASSERT(edge[0] < numPoints, "Bad index in edge.");
      VISKORES_TEST_ASSERT(edge[1] >= 0, "Bad index in edge.");
      VISKORES_TEST_ASSERT(edge[1] < numPoints, "Bad index in edge.");
      VISKORES_TEST_ASSERT(edge[0] != edge[1], "Degenerate edge.");
      MakeEdgeCanonical(edge);
      VISKORES_TEST_ASSERT(edge[0] < edge[1], "Internal test error: MakeEdgeCanonical failed");
      VISKORES_TEST_ASSERT(edgeSet.find(edge) == edgeSet.end(), "Found duplicate edge");
      edgeSet.insert(edge);

      viskores::Id2 canonicalEdgeId;
      CHECK_CALL(viskores::exec::CellEdgeCanonicalId(
        numPoints, edgeIndex, shape, pointIndexProxy, canonicalEdgeId));
      VISKORES_TEST_ASSERT(canonicalEdgeId[0] > 0, "Not using global ids?");
      VISKORES_TEST_ASSERT(canonicalEdgeId[0] < canonicalEdgeId[1], "Bad order.");
    }

    viskores::IdComponent numFaces;
    CHECK_CALL(viskores::exec::CellFaceNumberOfFaces(shape, numFaces));
    VISKORES_TEST_ASSERT(numFaces > 0, "No faces?");

    std::set<EdgeType> edgesFoundInFaces;
    for (viskores::IdComponent faceIndex = 0; faceIndex < numFaces; faceIndex++)
    {
      viskores::IdComponent numPointsInFace;
      CHECK_CALL(viskores::exec::CellFaceNumberOfPoints(faceIndex, shape, numPointsInFace));

      VISKORES_TEST_ASSERT(numPointsInFace >= 3, "Face has fewer points than a triangle.");

      for (viskores::IdComponent pointIndex = 0; pointIndex < numPointsInFace; pointIndex++)
      {
        viskores::IdComponent localFaceIndex;
        CHECK_CALL(
          viskores::exec::CellFaceLocalIndex(pointIndex, faceIndex, shape, localFaceIndex));
        VISKORES_TEST_ASSERT(localFaceIndex >= 0, "Invalid point index for face.");
        VISKORES_TEST_ASSERT(localFaceIndex < numPoints, "Invalid point index for face.");
        EdgeType edge;
        if (pointIndex < numPointsInFace - 1)
        {
          CHECK_CALL(viskores::exec::CellFaceLocalIndex(pointIndex, faceIndex, shape, edge[0]));
          CHECK_CALL(viskores::exec::CellFaceLocalIndex(pointIndex + 1, faceIndex, shape, edge[1]));
        }
        else
        {
          CHECK_CALL(viskores::exec::CellFaceLocalIndex(0, faceIndex, shape, edge[0]));
          CHECK_CALL(viskores::exec::CellFaceLocalIndex(pointIndex, faceIndex, shape, edge[1]));
        }
        MakeEdgeCanonical(edge);
        VISKORES_TEST_ASSERT(edgeSet.find(edge) != edgeSet.end(),
                             "Edge in face not in cell's edges");
        edgesFoundInFaces.insert(edge);
      }

      viskores::Id3 canonicalFaceId;
      CHECK_CALL(
        viskores::exec::CellFaceCanonicalId(faceIndex, shape, pointIndexProxy, canonicalFaceId));
      VISKORES_TEST_ASSERT(canonicalFaceId[0] > 0, "Not using global ids?");
      VISKORES_TEST_ASSERT(canonicalFaceId[0] < canonicalFaceId[1], "Bad order.");
      VISKORES_TEST_ASSERT(canonicalFaceId[1] < canonicalFaceId[2], "Bad order.");
    }
    VISKORES_TEST_ASSERT(edgesFoundInFaces.size() == edgeSet.size(),
                         "Faces did not contain all edges in cell");
  }

  // Case of cells that have 2 dimensions (no faces)
  template <typename CellShapeTag>
  void DoTest(viskores::IdComponent numPoints,
              CellShapeTag shape,
              viskores::CellTopologicalDimensionsTag<2>) const
  {
    std::vector<viskores::Id> pointIndexProxyBuffer(static_cast<std::size_t>(numPoints));
    for (std::size_t index = 0; index < pointIndexProxyBuffer.size(); ++index)
    {
      pointIndexProxyBuffer[index] = static_cast<viskores::Id>(1000000 - index);
    }
    viskores::VecCConst<viskores::Id> pointIndexProxy(&pointIndexProxyBuffer.at(0), numPoints);

    viskores::IdComponent numEdges;
    CHECK_CALL(viskores::exec::CellEdgeNumberOfEdges(numPoints, shape, numEdges));
    VISKORES_TEST_ASSERT(numEdges == numPoints,
                         "Polygons should have same number of points and edges");

    std::set<EdgeType> edgeSet;
    for (viskores::IdComponent edgeIndex = 0; edgeIndex < numEdges; edgeIndex++)
    {
      EdgeType edge;
      CHECK_CALL(viskores::exec::CellEdgeLocalIndex(numPoints, 0, edgeIndex, shape, edge[0]));
      CHECK_CALL(viskores::exec::CellEdgeLocalIndex(numPoints, 1, edgeIndex, shape, edge[1]));
      VISKORES_TEST_ASSERT(edge[0] >= 0, "Bad index in edge.");
      VISKORES_TEST_ASSERT(edge[0] < numPoints, "Bad index in edge.");
      VISKORES_TEST_ASSERT(edge[1] >= 0, "Bad index in edge.");
      VISKORES_TEST_ASSERT(edge[1] < numPoints, "Bad index in edge.");
      VISKORES_TEST_ASSERT(edge[0] != edge[1], "Degenerate edge.");
      MakeEdgeCanonical(edge);
      VISKORES_TEST_ASSERT(edge[0] < edge[1], "Internal test error: MakeEdgeCanonical failed");
      VISKORES_TEST_ASSERT(edgeSet.find(edge) == edgeSet.end(), "Found duplicate edge");
      edgeSet.insert(edge);

      viskores::Id2 canonicalEdgeId;
      CHECK_CALL(viskores::exec::CellEdgeCanonicalId(
        numPoints, edgeIndex, shape, pointIndexProxy, canonicalEdgeId));
      VISKORES_TEST_ASSERT(canonicalEdgeId[0] > 0, "Not using global ids?");
      VISKORES_TEST_ASSERT(canonicalEdgeId[0] < canonicalEdgeId[1], "Bad order.");
    }

    viskores::IdComponent numFaces;
    CHECK_CALL(viskores::exec::CellFaceNumberOfFaces(shape, numFaces));
    VISKORES_TEST_ASSERT(numFaces == 0, "Non 3D shape should have no faces");
  }

  // Less important case of cells that have less than 2 dimensions
  // (no faces or edges)
  template <typename CellShapeTag, viskores::IdComponent NumDimensions>
  void DoTest(viskores::IdComponent numPoints,
              CellShapeTag shape,
              viskores::CellTopologicalDimensionsTag<NumDimensions>) const
  {
    viskores::IdComponent numEdges;
    CHECK_CALL(viskores::exec::CellEdgeNumberOfEdges(numPoints, shape, numEdges));
    VISKORES_TEST_ASSERT(numEdges == 0, "0D or 1D shape should have no edges");

    viskores::IdComponent numFaces;
    CHECK_CALL(viskores::exec::CellFaceNumberOfFaces(shape, numFaces));
    VISKORES_TEST_ASSERT(numFaces == 0, "Non 3D shape should have no faces");
  }

  template <typename CellShapeTag>
  void TryShapeWithNumPoints(viskores::IdComponent numPoints, CellShapeTag) const
  {
    std::cout << "--- Test shape tag directly"
              << " (" << numPoints << " points)" << std::endl;
    this->DoTest(numPoints,
                 CellShapeTag(),
                 typename viskores::CellTraits<CellShapeTag>::TopologicalDimensionsTag());

    std::cout << "--- Test generic shape tag"
              << " (" << numPoints << " points)" << std::endl;
    this->DoTest(numPoints,
                 viskores::CellShapeTagGeneric(CellShapeTag::Id),
                 typename viskores::CellTraits<CellShapeTag>::TopologicalDimensionsTag());
  }

  template <typename CellShapeTag>
  void operator()(CellShapeTag) const
  {
    this->TryShapeWithNumPoints(viskores::CellTraits<CellShapeTag>::NUM_POINTS, CellShapeTag());
  }

  void operator()(viskores::CellShapeTagPolyLine) const
  {
    for (viskores::IdComponent numPoints = 3; numPoints < 7; numPoints++)
    {
      this->TryShapeWithNumPoints(numPoints, viskores::CellShapeTagPolyLine());
    }
  }

  void operator()(viskores::CellShapeTagPolygon) const
  {
    for (viskores::IdComponent numPoints = 3; numPoints < 7; numPoints++)
    {
      this->TryShapeWithNumPoints(numPoints, viskores::CellShapeTagPolygon());
    }
  }
};

void TestAllShapes()
{
  viskores::testing::Testing::TryAllCellShapes(TestCellFacesFunctor());
}

} // anonymous namespace

int UnitTestCellEdgeFace(int argc, char* argv[])
{
  return viskores::testing::Testing::Run(TestAllShapes, argc, argv);
}
