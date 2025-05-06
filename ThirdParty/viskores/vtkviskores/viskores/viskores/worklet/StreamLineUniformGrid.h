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

#ifndef viskores_worklet_StreamLineUniformGrid_h
#define viskores_worklet_StreamLineUniformGrid_h

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleCounting.h>
#include <viskores/cont/CellSetExplicit.h>
#include <viskores/cont/CellSetStructured.h>
#include <viskores/cont/ConvertNumComponentsToOffsets.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/DeviceAdapter.h>
#include <viskores/cont/Field.h>
#include <viskores/cont/Invoker.h>

#include <viskores/worklet/ScatterUniform.h>
#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace worklet
{
namespace streamline
{
// Take this out when defined in CellShape.h
const viskores::UInt8 CELL_SHAPE_POLY_LINE = 4;

enum StreamLineMode
{
  FORWARD = 0,
  BACKWARD = 1,
  BOTH = 2
};

// Trilinear interpolation to calculate vector data at position
template <typename FieldType, typename PortalType>
VISKORES_EXEC viskores::Vec<FieldType, 3> VecDataAtPos(viskores::Vec<FieldType, 3> pos,
                                                       const viskores::Id3& vdims,
                                                       const viskores::Id& planesize,
                                                       const viskores::Id& rowsize,
                                                       const PortalType& vecdata)
{
  // Adjust initial position to be within bounding box of grid
  for (viskores::IdComponent d = 0; d < 3; d++)
  {
    if (pos[d] < 0.0f)
      pos[d] = 0.0f;
    if (pos[d] > static_cast<FieldType>(vdims[d] - 1))
      pos[d] = static_cast<FieldType>(vdims[d] - 1);
  }

  // Set the eight corner indices with no wraparound
  viskores::Id3 idx000, idx001, idx010, idx011, idx100, idx101, idx110, idx111;
  idx000[0] = static_cast<viskores::Id>(floor(pos[0]));
  idx000[1] = static_cast<viskores::Id>(floor(pos[1]));
  idx000[2] = static_cast<viskores::Id>(floor(pos[2]));

  idx001 = idx000;
  idx001[0] = (idx001[0] + 1) <= vdims[0] - 1 ? idx001[0] + 1 : vdims[0] - 1;
  idx010 = idx000;
  idx010[1] = (idx010[1] + 1) <= vdims[1] - 1 ? idx010[1] + 1 : vdims[1] - 1;
  idx011 = idx010;
  idx011[0] = (idx011[0] + 1) <= vdims[0] - 1 ? idx011[0] + 1 : vdims[0] - 1;
  idx100 = idx000;
  idx100[2] = (idx100[2] + 1) <= vdims[2] - 1 ? idx100[2] + 1 : vdims[2] - 1;
  idx101 = idx100;
  idx101[0] = (idx101[0] + 1) <= vdims[0] - 1 ? idx101[0] + 1 : vdims[0] - 1;
  idx110 = idx100;
  idx110[1] = (idx110[1] + 1) <= vdims[1] - 1 ? idx110[1] + 1 : vdims[1] - 1;
  idx111 = idx110;
  idx111[0] = (idx111[0] + 1) <= vdims[0] - 1 ? idx111[0] + 1 : vdims[0] - 1;

  // Get the vecdata at the eight corners
  viskores::Vec<FieldType, 3> v000, v001, v010, v011, v100, v101, v110, v111;
  v000 = vecdata.Get(idx000[2] * planesize + idx000[1] * rowsize + idx000[0]);
  v001 = vecdata.Get(idx001[2] * planesize + idx001[1] * rowsize + idx001[0]);
  v010 = vecdata.Get(idx010[2] * planesize + idx010[1] * rowsize + idx010[0]);
  v011 = vecdata.Get(idx011[2] * planesize + idx011[1] * rowsize + idx011[0]);
  v100 = vecdata.Get(idx100[2] * planesize + idx100[1] * rowsize + idx100[0]);
  v101 = vecdata.Get(idx101[2] * planesize + idx101[1] * rowsize + idx101[0]);
  v110 = vecdata.Get(idx110[2] * planesize + idx110[1] * rowsize + idx110[0]);
  v111 = vecdata.Get(idx111[2] * planesize + idx111[1] * rowsize + idx111[0]);

  // Interpolation in X
  viskores::Vec<FieldType, 3> v00, v01, v10, v11;
  FieldType a = pos[0] - static_cast<FieldType>(floor(pos[0]));
  v00[0] = (1.0f - a) * v000[0] + a * v001[0];
  v00[1] = (1.0f - a) * v000[1] + a * v001[1];
  v00[2] = (1.0f - a) * v000[2] + a * v001[2];

  v01[0] = (1.0f - a) * v010[0] + a * v011[0];
  v01[1] = (1.0f - a) * v010[1] + a * v011[1];
  v01[2] = (1.0f - a) * v010[2] + a * v011[2];

  v10[0] = (1.0f - a) * v100[0] + a * v101[0];
  v10[1] = (1.0f - a) * v100[1] + a * v101[1];
  v10[2] = (1.0f - a) * v100[2] + a * v101[2];

  v11[0] = (1.0f - a) * v110[0] + a * v111[0];
  v11[1] = (1.0f - a) * v110[1] + a * v111[1];
  v11[2] = (1.0f - a) * v110[2] + a * v111[2];

  // Interpolation in Y
  viskores::Vec<FieldType, 3> v0, v1;
  a = pos[1] - static_cast<FieldType>(floor(pos[1]));
  v0[0] = (1.0f - a) * v00[0] + a * v01[0];
  v0[1] = (1.0f - a) * v00[1] + a * v01[1];
  v0[2] = (1.0f - a) * v00[2] + a * v01[2];

  v1[0] = (1.0f - a) * v10[0] + a * v11[0];
  v1[1] = (1.0f - a) * v10[1] + a * v11[1];
  v1[2] = (1.0f - a) * v10[2] + a * v11[2];

  // Interpolation in Z
  viskores::Vec<FieldType, 3> v;
  a = pos[2] - static_cast<FieldType>(floor(pos[2]));
  v[0] = (1.0f - a) * v0[0] + v1[0];
  v[1] = (1.0f - a) * v0[1] + v1[1];
  v[2] = (1.0f - a) * v0[2] + v1[2];
  return v;
}

struct IsUnity
{
  template <typename T>
  VISKORES_EXEC_CONT bool operator()(const T& x) const
  {
    return x == T(1);
  }
};

template <typename FieldType>
class MakeStreamLines : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(WholeArrayIn field,
                                FieldIn seedId,
                                FieldIn position,
                                WholeArrayOut numIndices,
                                WholeArrayOut validPoint,
                                WholeArrayOut streamLines);
  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, VisitIndex);
  using InputDomain = _2;

  using ScatterType = viskores::worklet::ScatterUniform<2>;

  const viskores::Id3 vdims;
  const viskores::Id maxsteps;
  const FieldType timestep;
  const viskores::Id planesize;
  const viskores::Id rowsize;
  const viskores::Id streammode;

  VISKORES_CONT
  MakeStreamLines() {}

  VISKORES_CONT
  MakeStreamLines(const FieldType tStep,
                  const viskores::Id sMode,
                  const viskores::Id nSteps,
                  const viskores::Id3 dims)
    : vdims(dims)
    , maxsteps(nSteps)
    , timestep(tStep)
    , planesize(dims[0] * dims[1])
    , rowsize(dims[0])
    , streammode(sMode)
  {
  }

  template <typename FieldPortalType, typename IdComponentPortalType, typename FieldVec3PortalType>
  VISKORES_EXEC void operator()(const FieldPortalType& field,
                                viskores::Id& seedId,
                                viskores::Vec<FieldType, 3>& seedPos,
                                IdComponentPortalType& numIndices,
                                IdComponentPortalType& validPoint,
                                FieldVec3PortalType& slLists,
                                viskores::IdComponent visitIndex) const
  {
    // Set initial offset into the output streams array
    viskores::Vec<FieldType, 3> pos = seedPos;
    viskores::Vec<FieldType, 3> pre_pos = seedPos;

    // Forward tracing
    if (visitIndex == 0 && (streammode == FORWARD || streammode == BOTH))
    {
      viskores::Id index = (seedId * 2) * maxsteps;
      bool done = false;
      viskores::Id step = 0;
      validPoint.Set(index, 1);
      slLists.Set(index++, pos);

      while (done != true && step < maxsteps)
      {
        viskores::Vec<FieldType, 3> vdata, adata, bdata, cdata, ddata;
        vdata = VecDataAtPos(pos, vdims, planesize, rowsize, field);
        for (viskores::IdComponent d = 0; d < 3; d++)
        {
          adata[d] = timestep * vdata[d];
          pos[d] += adata[d] / 2.0f;
        }

        vdata = VecDataAtPos(pos, vdims, planesize, rowsize, field);
        for (viskores::IdComponent d = 0; d < 3; d++)
        {
          bdata[d] = timestep * vdata[d];
          pos[d] += bdata[d] / 2.0f;
        }

        vdata = VecDataAtPos(pos, vdims, planesize, rowsize, field);
        for (viskores::IdComponent d = 0; d < 3; d++)
        {
          cdata[d] = timestep * vdata[d];
          pos[d] += cdata[d] / 2.0f;
        }

        vdata = VecDataAtPos(pos, vdims, planesize, rowsize, field);
        for (viskores::IdComponent d = 0; d < 3; d++)
        {
          ddata[d] = timestep * vdata[d];
          pos[d] += (adata[d] + (2.0f * bdata[d]) + (2.0f * cdata[d]) + ddata[d]) / 6.0f;
        }

        if (pos[0] < 0.0f || pos[0] > static_cast<FieldType>(vdims[0]) || pos[1] < 0.0f ||
            pos[1] > static_cast<FieldType>(vdims[1]) || pos[2] < 0.0f ||
            pos[2] > static_cast<FieldType>(vdims[2]))
        {
          pos = pre_pos;
          done = true;
        }
        else
        {
          validPoint.Set(index, 1);
          slLists.Set(index++, pos);
          pre_pos = pos;
        }
        step++;
      }
      numIndices.Set(seedId * 2, static_cast<viskores::IdComponent>(step));
    }

    // Backward tracing
    if (visitIndex == 1 && (streammode == BACKWARD || streammode == BOTH))
    {
      viskores::Id index = (seedId * 2 + 1) * maxsteps;
      bool done = false;
      viskores::Id step = 0;
      validPoint.Set(index, 1);
      slLists.Set(index++, pos);

      while (done != true && step < maxsteps)
      {
        viskores::Vec<FieldType, 3> vdata, adata, bdata, cdata, ddata;
        vdata = VecDataAtPos(pos, vdims, planesize, rowsize, field);
        for (viskores::IdComponent d = 0; d < 3; d++)
        {
          adata[d] = timestep * (0.0f - vdata[d]);
          pos[d] += adata[d] / 2.0f;
        }

        vdata = VecDataAtPos(pos, vdims, planesize, rowsize, field);
        for (viskores::IdComponent d = 0; d < 3; d++)
        {
          bdata[d] = timestep * (0.0f - vdata[d]);
          pos[d] += bdata[d] / 2.0f;
        }

        vdata = VecDataAtPos(pos, vdims, planesize, rowsize, field);
        for (viskores::IdComponent d = 0; d < 3; d++)
        {
          cdata[d] = timestep * (0.0f - vdata[d]);
          pos[d] += cdata[d] / 2.0f;
        }

        vdata = VecDataAtPos(pos, vdims, planesize, rowsize, field);
        for (viskores::IdComponent d = 0; d < 3; d++)
        {
          ddata[d] = timestep * (0.0f - vdata[d]);
          pos[d] += (adata[d] + (2.0f * bdata[d]) + (2.0f * cdata[d]) + ddata[d]) / 6.0f;
        }

        if (pos[0] < 0.0f || pos[0] > static_cast<FieldType>(vdims[0]) || pos[1] < 0.0f ||
            pos[1] > static_cast<FieldType>(vdims[1]) || pos[2] < 0.0f ||
            pos[2] > static_cast<FieldType>(vdims[2]))
        {
          pos = pre_pos;
          done = true;
        }
        else
        {
          validPoint.Set(index, 1);
          slLists.Set(index++, pos);
          pre_pos = pos;
        }
        step++;
      }
      numIndices.Set((seedId * 2) + 1, static_cast<viskores::IdComponent>(step));
    }
  }
};


} // namespace streamline

/// \brief Compute the streamline
template <typename FieldType>
class StreamLineFilterUniformGrid
{
public:
  StreamLineFilterUniformGrid() {}

  viskores::cont::DataSet Run(const viskores::cont::DataSet& InDataSet,
                              viskores::Id streamMode,
                              viskores::Id numSeeds,
                              viskores::Id maxSteps,
                              FieldType timeStep)
  {
    using Algorithm = viskores::cont::Algorithm;

    // Get information from input dataset
    viskores::cont::CellSetStructured<3> inCellSet;
    InDataSet.GetCellSet().AsCellSet(inCellSet);
    viskores::Id3 vdims = inCellSet.GetSchedulingRange(viskores::TopologyElementTagPoint());

    viskores::cont::ArrayHandle<viskores::Vec<FieldType, 3>> fieldArray;
    InDataSet.GetField("vecData").GetData().AsArrayHandle(fieldArray);

    // Generate random seeds for starting streamlines
    viskores::cont::ArrayHandle<viskores::Vec<FieldType, 3>> seedPosArray;
    seedPosArray.Allocate(numSeeds);
    {
      auto seedPosPortal = seedPosArray.WritePortal();
      for (viskores::Id i = 0; i < numSeeds; i++)
      {
        viskores::Vec<FieldType, 3> seed;
        seed[0] = static_cast<FieldType>(rand() % vdims[0]);
        seed[1] = static_cast<FieldType>(rand() % vdims[1]);
        seed[2] = static_cast<FieldType>(rand() % vdims[2]);
        seedPosPortal.Set(i, seed);
      }
    }
    viskores::cont::ArrayHandleIndex seedIdArray(numSeeds);

    // Number of streams * number of steps * [forward, backward]
    viskores::Id numCells = numSeeds * 2;
    viskores::Id maxConnectivityLen = numCells * maxSteps;

    // Stream array at max size will be filled with stream coordinates
    viskores::cont::ArrayHandle<viskores::Vec<FieldType, 3>> streamArray;
    streamArray.Allocate(maxConnectivityLen);

    // NumIndices per polyline cell filled in by MakeStreamLines
    viskores::cont::ArrayHandle<viskores::IdComponent> numIndices;
    numIndices.Allocate(numCells);

    // All cells are polylines
    viskores::cont::ArrayHandle<viskores::UInt8> cellTypes;
    cellTypes.Allocate(numCells);
    viskores::cont::ArrayHandleConstant<viskores::UInt8> polyLineShape(
      streamline::CELL_SHAPE_POLY_LINE, numCells);
    Algorithm::Copy(polyLineShape, cellTypes);

    // Possible maxSteps points but if less use stencil
    viskores::cont::ArrayHandle<viskores::IdComponent> validPoint;
    viskores::cont::ArrayHandleConstant<viskores::Id> zeros(0, maxConnectivityLen);
    validPoint.Allocate(maxConnectivityLen);
    Algorithm::Copy(zeros, validPoint);

    // Worklet to make the streamlines
    streamline::MakeStreamLines<FieldType> makeStreamLines(timeStep, streamMode, maxSteps, vdims);

    viskores::cont::Invoker{}(
      makeStreamLines, fieldArray, seedIdArray, seedPosArray, numIndices, validPoint, streamArray);

    // Size of connectivity based on size of returned streamlines
    viskores::Id connectivityLen;
    auto offsets = viskores::cont::ConvertNumComponentsToOffsets(numIndices, connectivityLen);

    // Connectivity is sequential
    viskores::cont::ArrayHandleCounting<viskores::Id> connCount(0, 1, connectivityLen);
    viskores::cont::ArrayHandle<viskores::Id> connectivity;
    Algorithm::Copy(connCount, connectivity);

    // Compact the stream array so it only has valid points
    viskores::cont::ArrayHandle<viskores::Vec<FieldType, 3>> coordinates;
    Algorithm::CopyIf(streamArray, validPoint, coordinates, streamline::IsUnity());

    // Create the output data set
    viskores::cont::DataSet OutDataSet;
    viskores::cont::CellSetExplicit<> outCellSet;

    outCellSet.Fill(coordinates.GetNumberOfValues(), cellTypes, connectivity, offsets);
    OutDataSet.SetCellSet(outCellSet);
    OutDataSet.AddCoordinateSystem(viskores::cont::CoordinateSystem("coordinates", coordinates));

    return OutDataSet;
  }
};
}
}

#endif // viskores_worklet_StreamLineUniformGrid_h
