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
#ifndef viskores_worklet_tube_h
#define viskores_worklet_tube_h

#include <typeinfo>
#include <viskores/VectorAnalysis.h>
#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayHandleCast.h>
#include <viskores/cont/CellSetExplicit.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/UnknownCellSet.h>
#include <viskores/worklet/DispatcherMapTopology.h>
#include <viskores/worklet/ScatterCounting.h>
#include <viskores/worklet/WorkletMapField.h>
#include <viskores/worklet/WorkletMapTopology.h>

namespace viskores
{
namespace worklet
{

class Tube
{
public:
  //Helper worklet to count various things in each polyline.
  class CountSegments : public viskores::worklet::WorkletVisitCellsWithPoints
  {
  public:
    VISKORES_CONT
    CountSegments(const bool& capping, const viskores::Id& n)
      : Capping(capping)
      , NumSides(n)
      , NumVertsPerCell(3)
    {
    }

    using ControlSignature = void(CellSetIn cellset,
                                  WholeArrayIn pointCoords,
                                  FieldOut nonIncidentPtsPerPolyline,
                                  FieldOut ptsPerPolyline,
                                  FieldOut ptsPerTube,
                                  FieldOut numTubeConnIds,
                                  FieldOut validCell);
    using ExecutionSignature = void(CellShape shapeType,
                                    PointCount numPoints,
                                    PointIndices ptIndices,
                                    _2 inPts,
                                    _3 nonIncidentPtsPerPolyline,
                                    _4 ptsPerPolyline,
                                    _5 ptsPerTube,
                                    _6 numTubeConnIds,
                                    _7 validCell);
    using InputDomain = _1;

    template <typename CellShapeTag, typename PointIndexType, typename InPointsType>
    VISKORES_EXEC void operator()(const CellShapeTag& shapeType,
                                  const viskores::IdComponent& numPoints,
                                  const PointIndexType& ptIndices,
                                  const InPointsType& inPts,
                                  viskores::IdComponent& nonIncidentPtsPerPolyline,
                                  viskores::Id& ptsPerPolyline,
                                  viskores::Id& ptsPerTube,
                                  viskores::Id& numTubeConnIds,
                                  viskores::Id& validCell) const
    {
      // We only support polylines that contain 2 or more points.
      viskores::IdComponent numNonCoincidentPoints = 1;
      viskores::Vec3f p = inPts.Get(ptIndices[0]);

      validCell = 0;
      for (int i = 1; i < numPoints; ++i)
      {
        viskores::Vec3f pNext = inPts.Get(ptIndices[i]);
        if (viskores::Magnitude(pNext - p) > viskores::Epsilon<viskores::FloatDefault>())
        {
          numNonCoincidentPoints++;
          p = pNext;
          validCell = 1;
        }
      }

      if (shapeType.Id == viskores::CELL_SHAPE_POLY_LINE && numNonCoincidentPoints > 1)
      {
        ptsPerPolyline = numPoints;
        nonIncidentPtsPerPolyline = numNonCoincidentPoints;
        ptsPerTube = this->NumSides * numNonCoincidentPoints;
        // (two tris per segment) X (numSides) X numVertsPerCell
        numTubeConnIds = (numNonCoincidentPoints - 1) * 2 * this->NumSides * this->NumVertsPerCell;

        //Capping adds center vertex in middle of cap, plus NumSides triangles for cap.
        if (this->Capping)
        {
          ptsPerTube += 2;
          numTubeConnIds += (2 * this->NumSides * this->NumVertsPerCell);
        }
      }
      else
      {
        validCell = 0;
        ptsPerPolyline = 0;
        nonIncidentPtsPerPolyline = 0;
        ptsPerTube = 0;
        numTubeConnIds = 0;
      }
    }

  private:
    bool Capping;
    viskores::Id NumSides;
    viskores::Id NumVertsPerCell;
  };

  //Helper worklet to generate normals at each point in the polyline.
  class GenerateNormals : public viskores::worklet::WorkletVisitCellsWithPoints
  {
    static constexpr viskores::FloatDefault vecMagnitudeEps =
      static_cast<viskores::FloatDefault>(1e-3);

  public:
    VISKORES_CONT
    GenerateNormals()
      : DefaultNorm(0, 0, 1)
    {
    }

    using ControlSignature = void(CellSetIn cellset,
                                  WholeArrayIn pointCoords,
                                  FieldInCell polylineOffset,
                                  WholeArrayOut newNormals);
    using ExecutionSignature = void(CellShape shapeType,
                                    PointCount numPoints,
                                    PointIndices ptIndices,
                                    _2 inPts,
                                    _3 polylineOffset,
                                    _4 outNormals);
    using InputDomain = _1;
    using ScatterType = viskores::worklet::ScatterCounting;
    VISKORES_CONT
    static ScatterType MakeScatter(const viskores::cont::ArrayHandle<viskores::Id>& validCell)
    {
      return ScatterType(validCell);
    }

    template <typename InPointsType, typename PointIndexType>
    VISKORES_EXEC viskores::IdComponent FindValidSegment(const InPointsType& inPts,
                                                         const PointIndexType& ptIndices,
                                                         const viskores::IdComponent& numPoints,
                                                         viskores::IdComponent start) const
    {
      auto ps = inPts.Get(ptIndices[start]);
      viskores::IdComponent end = start + 1;
      while (end < numPoints)
      {
        auto pe = inPts.Get(ptIndices[end]);
        if (viskores::Magnitude(pe - ps) > viskores::Epsilon<viskores::FloatDefault>())
          return end - 1;
        end++;
      }

      return numPoints;
    }

    template <typename CellShapeTag,
              typename PointIndexType,
              typename InPointsType,
              typename OutNormalType>
    VISKORES_EXEC void operator()(const CellShapeTag& shapeType,
                                  const viskores::IdComponent& numPoints,
                                  const PointIndexType& ptIndices,
                                  const InPointsType& inPts,
                                  const viskores::Id& polylineOffset,
                                  OutNormalType& outNormals) const
    {
      //Ignore non-polyline and polyline with less than 2 points.
      if (shapeType.Id != viskores::CELL_SHAPE_POLY_LINE || numPoints < 2)
        return;
      else
      {
        //The following follows the VTK implementation in:
        //vtkPolyLine::GenerateSlidingNormals
        viskores::Vec3f sPrev, sNext, normal, p0, p1;
        viskores::IdComponent sNextId = FindValidSegment(inPts, ptIndices, numPoints, 0);

        if (sNextId != numPoints) // at least one valid segment
        {
          p0 = inPts.Get(ptIndices[sNextId]);
          p1 = inPts.Get(ptIndices[sNextId + 1]);
          sPrev = viskores::Normal(p1 - p0);
        }
        else // no valid segments. Set everything to the default normal.
        {
          for (viskores::Id i = 0; i < numPoints; i++)
            outNormals.Set(polylineOffset + i, this->DefaultNorm);
          return;
        }

        // find the next valid, non-parallel segment
        while (++sNextId < numPoints)
        {
          sNextId = FindValidSegment(inPts, ptIndices, numPoints, sNextId);
          if (sNextId != numPoints)
          {
            p0 = inPts.Get(ptIndices[sNextId]);
            p1 = inPts.Get(ptIndices[sNextId + 1]);
            sNext = viskores::Normal(p1 - p0);

            // now the starting normal should simply be the cross product
            // in the following if statement we check for the case where
            // the two segments are parallel, in which case, continue searching
            // for the next valid segment
            auto n = viskores::Cross(sPrev, sNext);
            if (viskores::Magnitude(n) > vecMagnitudeEps)
            {
              normal = n;
              sPrev = sNext;
              break;
            }
          }
        }

        //only one valid segment...
        if (sNextId >= numPoints)
        {
          for (viskores::IdComponent j = 0; j < 3; j++)
            if (sPrev[j] != 0)
            {
              normal[(j + 2) % 3] = 0;
              normal[(j + 1) % 3] = 1;
              normal[j] = -sPrev[(j + 1) % 3] / sPrev[j];
              break;
            }
        }

        viskores::Normalize(normal);
        viskores::Id lastNormalId = 0;
        while (++sNextId < numPoints)
        {
          sNextId = FindValidSegment(inPts, ptIndices, numPoints, sNextId);
          if (sNextId == numPoints)
            break;

          p0 = inPts.Get(ptIndices[sNextId]);
          p1 = inPts.Get(ptIndices[sNextId + 1]);
          sNext = viskores::Normal(p1 - p0);

          auto q = viskores::Cross(sNext, sPrev);

          if (viskores::Magnitude(q) <=
              viskores::Epsilon<viskores::FloatDefault>()) //can't use this segment
            continue;
          viskores::Normalize(q);

          viskores::FloatDefault f1 = viskores::Dot(q, normal);
          viskores::FloatDefault f2 = 1 - (f1 * f1);
          if (f2 > 0)
            f2 = viskores::Sqrt(f2);
          else
            f2 = 0;

          auto c = viskores::Normal(sNext + sPrev);
          auto w = viskores::Cross(c, q);
          c = viskores::Cross(sPrev, q);
          if ((viskores::Dot(normal, c) * viskores::Dot(w, c)) < 0)
            f2 = -f2;

          for (viskores::Id i = lastNormalId; i < sNextId; i++)
            outNormals.Set(polylineOffset + i, normal);
          lastNormalId = sNextId;
          sPrev = sNext;
          normal = (f1 * q) + (f2 * w);
        }

        for (viskores::Id i = lastNormalId; i < numPoints; i++)
          outNormals.Set(polylineOffset + i, normal);
      }
    }

  private:
    viskores::Vec3f DefaultNorm;
  };

  //Helper worklet to generate the tube points
  class GeneratePoints : public viskores::worklet::WorkletVisitCellsWithPoints
  {
  public:
    VISKORES_CONT
    GeneratePoints(const bool& capping, const viskores::Id& n, const viskores::FloatDefault& r)
      : Capping(capping)
      , NumSides(n)
      , Radius(r)
      , Theta(2 * static_cast<viskores::FloatDefault>(viskores::Pi()) /
              static_cast<viskores::FloatDefault>(n))
    {
    }

    using ControlSignature = void(CellSetIn cellset,
                                  WholeArrayIn pointCoords,
                                  WholeArrayIn normals,
                                  FieldInCell numNonCoincidentPts,
                                  FieldInCell tubePointOffsets,
                                  FieldInCell polylineOffset,
                                  WholeArrayOut newPointCoords,
                                  WholeArrayOut outPointSrcIdx);
    using ExecutionSignature = void(CellShape shapeType,
                                    PointCount numPoints,
                                    PointIndices ptIndices,
                                    _2 inPts,
                                    _3 inNormals,
                                    _4 numNonCoincidentPts,
                                    _5 tubePointOffsets,
                                    _6 polylineOffset,
                                    _7 outPts,
                                    _8 outPointSrcIdx);
    using InputDomain = _1;
    using ScatterType = viskores::worklet::ScatterCounting;
    VISKORES_CONT
    static ScatterType MakeScatter(const viskores::cont::ArrayHandle<viskores::Id>& validCell)
    {
      return ScatterType(validCell);
    }

    template <typename CellShapeTag,
              typename PointIndexType,
              typename InPointsType,
              typename InNormalsType,
              typename OutPointsType,
              typename OutPointSrcIdxType>
    VISKORES_EXEC void operator()(const CellShapeTag& shapeType,
                                  const viskores::IdComponent& numPoints,
                                  const PointIndexType& ptIndices,
                                  const InPointsType& inPts,
                                  const InNormalsType& inNormals,
                                  const viskores::Id& numNonCoincidentPts,
                                  const viskores::Id& tubePointOffsets,
                                  const viskores::Id& polylineOffset,
                                  OutPointsType& outPts,
                                  OutPointSrcIdxType& outPointSrcIdx) const
    {
      if (shapeType.Id != viskores::CELL_SHAPE_POLY_LINE || numNonCoincidentPts < 2)
        return;
      else
      {
        viskores::Id outIdx = tubePointOffsets;
        viskores::Id pIdx = ptIndices[0];
        viskores::Id pNextIdx =
          ptIndices[this->FindNextNonCoincidentPointIndex(ptIndices, inPts, 0, numPoints)];
        viskores::Vec3f p = inPts.Get(pIdx);
        viskores::Vec3f pNext = inPts.Get(pNextIdx);
        viskores::Vec3f sNext = pNext - p;
        viskores::Vec3f sPrev = sNext;
        viskores::FloatDefault eps = viskores::Epsilon<viskores::FloatDefault>();

        //Add the start cap vertex. This is just a point at the center of the tube (on the polyline).
        if (this->Capping)
        {
          outPts.Set(outIdx, p);
          outPointSrcIdx.Set(outIdx, pIdx);
          outIdx++;
        }

        viskores::IdComponent j = 0;
        while (j < numPoints)
        {
          viskores::IdComponent jNext =
            this->FindNextNonCoincidentPointIndex(ptIndices, inPts, j, numPoints);
          if (j == 0) //first point
          {
            //Variables initialized before loop started.
          }
          else if (jNext == numPoints) //last point
          {
            sPrev = sNext;
            p = pNext;
            pIdx = pNextIdx;
          }
          else
          {
            p = pNext;
            pIdx = pNextIdx;
            pNextIdx = ptIndices[jNext];
            pNext = inPts.Get(pNextIdx);
            sPrev = sNext;
            sNext = pNext - p;
          }
          viskores::Vec3f n = inNormals.Get(polylineOffset + j);

          viskores::Normalize(sNext);
          auto s = (sPrev + sNext) / 2.;
          if (viskores::Magnitude(s) <= eps)
            s = viskores::Cross(sPrev, n);
          viskores::Normalize(s);

          auto w = viskores::Cross(s, n);
          //Bad normal
          if (viskores::Magnitude(w) <= eps)
            this->RaiseError("Bad normal in Tube worklet.");
          viskores::Normalize(w);

          //create orthogonal coordinate system.
          auto nP = viskores::Cross(w, s);
          viskores::Normalize(nP);

          //this only implements the 'sides share vertices' line 476
          viskores::Vec3f normal;
          for (viskores::IdComponent k = 0; k < this->NumSides; k++)
          {
            viskores::FloatDefault angle = static_cast<viskores::FloatDefault>(k) * this->Theta;
            viskores::FloatDefault cosValue = viskores::Cos(angle);
            viskores::FloatDefault sinValue = viskores::Sin(angle);
            normal = w * cosValue + nP * sinValue;
            auto newPt = p + this->Radius * normal;
            outPts.Set(outIdx, newPt);
            outPointSrcIdx.Set(outIdx, pIdx);
            outIdx++;
          }

          j = jNext;
        }

        //Add the end cap vertex. This is just a point at the center of the tube (on the polyline).
        if (this->Capping)
        {
          outPts.Set(outIdx, p);
          outPointSrcIdx.Set(outIdx, pIdx);
          outIdx++;
        }
      }
    }

    template <typename PointIndexType, typename InPointsType>
    VISKORES_EXEC viskores::IdComponent FindNextNonCoincidentPointIndex(
      const PointIndexType& ptIndices,
      const InPointsType& inPts,
      viskores::IdComponent start,
      viskores::IdComponent numPoints) const
    {
      viskores::Id pIdx = ptIndices[start];
      viskores::Id pNextIdx;
      viskores::Float32 eps = viskores::Epsilon<viskores::FloatDefault>();
      for (viskores::IdComponent i = start + 1; i < numPoints; ++i)
      {
        pNextIdx = ptIndices[i];
        viskores::FloatDefault pNext = viskores::Magnitude(inPts.Get(pIdx) - inPts.Get(pNextIdx));
        if (pNext > eps)
        {
          return i;
        }
      }

      return numPoints;
    }

  private:
    bool Capping;
    viskores::Id NumSides;
    viskores::FloatDefault Radius;
    viskores::FloatDefault Theta;
  };

  //Helper worklet to generate the tube cells
  class GenerateCells : public viskores::worklet::WorkletVisitCellsWithPoints
  {
  public:
    VISKORES_CONT
    GenerateCells(const bool& capping, const viskores::Id& n)
      : Capping(capping)
      , NumSides(n)
    {
    }

    using ControlSignature = void(CellSetIn cellset,
                                  FieldInCell ptsPerPolyline,
                                  FieldInCell tubePointOffsets,
                                  FieldInCell tubeConnOffsets,
                                  WholeArrayOut outConnectivity,
                                  WholeArrayOut outCellSrcIdx);
    using ExecutionSignature = void(CellShape shapeType,
                                    InputIndex inCellIndex,
                                    _2 ptsPerPolyline,
                                    _3 tubePointOffset,
                                    _4 tubeConnOffsets,
                                    _5 outConn,
                                    _6 outCellSrcIdx);
    using InputDomain = _1;

    template <typename CellShapeTag, typename OutConnType, typename OutCellSrcIdxType>
    VISKORES_EXEC void operator()(const CellShapeTag& shapeType,
                                  viskores::Id inCellIndex,
                                  const viskores::IdComponent& numPoints,
                                  const viskores::Id& tubePointOffset,
                                  const viskores::Id& tubeConnOffset,
                                  OutConnType& outConn,
                                  OutCellSrcIdxType& outCellSrcIdx) const
    {
      if (shapeType.Id != viskores::CELL_SHAPE_POLY_LINE || numPoints < 2)
        return;
      else
      {
        viskores::Id outIdx = tubeConnOffset;
        viskores::Id tubePtOffset = (this->Capping ? tubePointOffset + 1 : tubePointOffset);
        for (viskores::IdComponent i = 0; i < numPoints - 1; i++)
        {
          for (viskores::Id j = 0; j < this->NumSides; j++)
          {
            //Triangle 1: verts 0,1,2
            outConn.Set(outIdx + 0, tubePtOffset + i * this->NumSides + j);
            outConn.Set(outIdx + 1, tubePtOffset + i * this->NumSides + (j + 1) % this->NumSides);
            outConn.Set(outIdx + 2,
                        tubePtOffset + (i + 1) * this->NumSides + (j + 1) % this->NumSides);
            outCellSrcIdx.Set(outIdx / 3, inCellIndex);
            outIdx += 3;

            //Triangle 2: verts 0,2,3
            outConn.Set(outIdx + 0, tubePtOffset + i * this->NumSides + j);
            outConn.Set(outIdx + 1,
                        tubePtOffset + (i + 1) * this->NumSides + (j + 1) % this->NumSides);
            outConn.Set(outIdx + 2, tubePtOffset + (i + 1) * this->NumSides + j);
            outCellSrcIdx.Set(outIdx / 3, inCellIndex);
            outIdx += 3;
          }
        }

        if (this->Capping)
        {
          //start cap triangles
          viskores::Id startCenterPt = 0 + tubePointOffset;
          for (viskores::Id j = 0; j < this->NumSides; j++)
          {
            outConn.Set(outIdx + 0, startCenterPt);
            outConn.Set(outIdx + 1, startCenterPt + 1 + j);
            outConn.Set(outIdx + 2, startCenterPt + 1 + ((j + 1) % this->NumSides));
            outCellSrcIdx.Set(outIdx / 3, inCellIndex);
            outIdx += 3;
          }

          //end cap triangles
          viskores::Id endCenterPt = (tubePointOffset + 1) + (numPoints * this->NumSides);
          viskores::Id endOffsetPt = endCenterPt - this->NumSides;

          for (viskores::Id j = 0; j < this->NumSides; j++)
          {
            outConn.Set(outIdx + 0, endCenterPt);
            outConn.Set(outIdx + 1, endOffsetPt + j);
            outConn.Set(outIdx + 2, endOffsetPt + ((j + 1) % this->NumSides));
            outCellSrcIdx.Set(outIdx / 3, inCellIndex);
            outIdx += 3;
          }
        }
      }
    }

  private:
    bool Capping;
    viskores::Id NumSides;
  };


  class MapField : public viskores::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn sourceIdx, WholeArrayIn sourceArray, FieldOut output);
    using ExecutionSignature = void(_1 sourceIdx, _2 sourceArray, _3 output);
    using InputDomain = _1;

    VISKORES_CONT
    MapField() {}

    template <typename SourceArrayType, typename T>
    VISKORES_EXEC void operator()(const viskores::Id& sourceIdx,
                                  const SourceArrayType& sourceArray,
                                  T& output) const
    {
      output = sourceArray.Get(sourceIdx);
    }
  };

  VISKORES_CONT
  Tube()
    : Capping(false)
    , NumSides(0)
    , Radius(0)
  {
  }

  VISKORES_CONT
  Tube(const bool& capping, const viskores::Id& n, const viskores::FloatDefault& r)
    : Capping(capping)
    , NumSides(n)
    , Radius(r)
  {
  }

  VISKORES_CONT
  void SetCapping(bool v) { this->Capping = v; }
  VISKORES_CONT
  void SetNumberOfSides(viskores::Id n) { this->NumSides = n; }
  VISKORES_CONT
  void SetRadius(viskores::FloatDefault r) { this->Radius = r; }

  template <typename Storage>
  VISKORES_CONT void Run(const viskores::cont::ArrayHandle<viskores::Vec3f, Storage>& coords,
                         const viskores::cont::UnknownCellSet& cellset,
                         viskores::cont::ArrayHandle<viskores::Vec3f>& newPoints,
                         viskores::cont::CellSetSingleType<>& newCells)
  {
    using NormalsType = viskores::cont::ArrayHandle<viskores::Vec3f>;

    if (!cellset.CanConvert<viskores::cont::CellSetExplicit<>>() &&
        !cellset.CanConvert<viskores::cont::CellSetSingleType<>>())
    {
      throw viskores::cont::ErrorBadValue("Tube filter only supported for polyline data.");
    }

    //Count number of polyline pts, tube pts and tube cells
    viskores::cont::ArrayHandle<viskores::Id> ptsPerPolyline, ptsPerTube, numTubeConnIds, validCell;
    viskores::cont::ArrayHandle<viskores::IdComponent> nonIncidentPtsPerPolyline;
    CountSegments countSegs(this->Capping, this->NumSides);
    viskores::worklet::DispatcherMapTopology<CountSegments> countInvoker(countSegs);
    countInvoker.Invoke(cellset,
                        coords,
                        nonIncidentPtsPerPolyline,
                        ptsPerPolyline,
                        ptsPerTube,
                        numTubeConnIds,
                        validCell);

    viskores::Id totalPolylinePts =
      viskores::cont::Algorithm::Reduce(ptsPerPolyline, viskores::Id(0));
    if (totalPolylinePts == 0)
      throw viskores::cont::ErrorBadValue("Tube filter only supported for polyline data.");
    viskores::Id totalTubePts = viskores::cont::Algorithm::Reduce(ptsPerTube, viskores::Id(0));
    viskores::Id totalTubeConnIds =
      viskores::cont::Algorithm::Reduce(numTubeConnIds, viskores::Id(0));
    //All cells are triangles, so cell count is simple to compute.
    viskores::Id totalTubeCells = totalTubeConnIds / 3;

    viskores::cont::ArrayHandle<viskores::Id> polylinePtOffset, nonIncidentPolylinePtOffset,
      tubePointOffsets, tubeConnOffsets;
    viskores::cont::Algorithm::ScanExclusive(ptsPerPolyline, polylinePtOffset);
    viskores::cont::Algorithm::ScanExclusive(
      viskores::cont::make_ArrayHandleCast<viskores::Id>(nonIncidentPtsPerPolyline),
      nonIncidentPolylinePtOffset);
    viskores::cont::Algorithm::ScanExclusive(ptsPerTube, tubePointOffsets);
    viskores::cont::Algorithm::ScanExclusive(numTubeConnIds, tubeConnOffsets);

    //Generate normals at each point on all polylines
    NormalsType normals;
    normals.Allocate(totalPolylinePts);
    viskores::worklet::DispatcherMapTopology<GenerateNormals> genNormalsDisp(
      GenerateNormals::MakeScatter(validCell));
    genNormalsDisp.Invoke(cellset, coords, polylinePtOffset, normals);

    //Generate the tube points
    newPoints.Allocate(totalTubePts);
    this->OutputPointSourceIndex.Allocate(totalTubePts);
    GeneratePoints genPts(this->Capping, this->NumSides, this->Radius);
    viskores::worklet::DispatcherMapTopology<GeneratePoints> genPtsDisp(
      genPts, GeneratePoints::MakeScatter(validCell));
    genPtsDisp.Invoke(cellset,
                      coords,
                      normals,
                      nonIncidentPtsPerPolyline,
                      tubePointOffsets,
                      polylinePtOffset,
                      newPoints,
                      this->OutputPointSourceIndex);

    //Generate tube cells
    viskores::cont::ArrayHandle<viskores::Id> newConnectivity;
    newConnectivity.Allocate(totalTubeConnIds);
    this->OutputCellSourceIndex.Allocate(totalTubeCells);
    GenerateCells genCells(this->Capping, this->NumSides);
    viskores::worklet::DispatcherMapTopology<GenerateCells> genCellsDisp(genCells);
    genCellsDisp.Invoke(cellset,
                        nonIncidentPtsPerPolyline,
                        tubePointOffsets,
                        tubeConnOffsets,
                        newConnectivity,
                        this->OutputCellSourceIndex);
    newCells.Fill(totalTubePts, viskores::CELL_SHAPE_TRIANGLE, 3, newConnectivity);
  }

  viskores::cont::ArrayHandle<viskores::Id> GetOutputCellSourceIndex() const
  {
    return this->OutputCellSourceIndex;
  }
  viskores::cont::ArrayHandle<viskores::Id> GetOutputPointSourceIndex() const
  {
    return this->OutputPointSourceIndex;
  }

private:
  bool Capping;
  viskores::Id NumSides;
  viskores::FloatDefault Radius;
  viskores::cont::ArrayHandle<viskores::Id> OutputCellSourceIndex;
  viskores::cont::ArrayHandle<viskores::Id> OutputPointSourceIndex;
};
}
}

#endif //  viskores_worklet_tube_h
