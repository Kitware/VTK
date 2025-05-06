//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//=============================================================================
#define viskores_filter_flow_worklet_Analysis_cxx

#include <viskores/filter/flow/worklet/Analysis.h>

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ConvertNumComponentsToOffsets.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/Invoker.h>
#include <viskores/cont/ParticleArrayCopy.h>
#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace worklet
{
namespace flow
{

template <typename ParticleType>
VISKORES_CONT bool NoAnalysis<ParticleType>::MakeDataSet(
  viskores::cont::DataSet& dataset,
  const std::vector<NoAnalysis<ParticleType>>& results)
{
  size_t nResults = results.size();
  std::vector<viskores::cont::ArrayHandle<ParticleType>> allParticles;
  allParticles.reserve(nResults);
  for (const auto& vres : results)
    allParticles.emplace_back(vres.Particles);

  viskores::cont::ArrayHandle<viskores::Vec3f> pts;
  viskores::cont::ParticleArrayCopy(allParticles, pts);

  viskores::Id numPoints = pts.GetNumberOfValues();
  if (numPoints > 0)
  {
    //Create coordinate system and vertex cell set.
    dataset.AddCoordinateSystem(viskores::cont::CoordinateSystem("coordinates", pts));

    viskores::cont::CellSetSingleType<> cells;
    viskores::cont::ArrayHandleIndex conn(numPoints);
    viskores::cont::ArrayHandle<viskores::Id> connectivity;

    viskores::cont::ArrayCopy(conn, connectivity);
    cells.Fill(numPoints, viskores::CELL_SHAPE_VERTEX, 1, connectivity);
    dataset.SetCellSet(cells);
  }
  return true;
}

namespace detail
{
class GetSteps : public viskores::worklet::WorkletMapField
{
public:
  VISKORES_CONT
  GetSteps() {}
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2);

  template <typename ParticleType>
  VISKORES_EXEC void operator()(const ParticleType& p, viskores::Id& numSteps) const
  {
    numSteps = p.GetNumberOfSteps();
  }
};

class ComputeNumPoints : public viskores::worklet::WorkletMapField
{
public:
  VISKORES_CONT
  ComputeNumPoints() {}
  using ControlSignature = void(FieldIn, FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2, _3);

  // Offset is number of points in streamline.
  // 1 (inital point) + number of steps taken (p.NumSteps - initalNumSteps)
  template <typename ParticleType>
  VISKORES_EXEC void operator()(const ParticleType& p,
                                const viskores::Id& initialNumSteps,
                                viskores::Id& diff) const
  {
    diff = 1 + p.GetNumberOfSteps() - initialNumSteps;
  }
};

} // namespace detail

template <typename ParticleType>
VISKORES_CONT void StreamlineAnalysis<ParticleType>::InitializeAnalysis(
  const viskores::cont::ArrayHandle<ParticleType>& particles)
{
  this->NumParticles = particles.GetNumberOfValues();

  //Create ValidPointArray initialized to zero.
  viskores::cont::ArrayHandleConstant<viskores::Id> validity(
    0, this->NumParticles * (this->MaxSteps + 1));
  viskores::cont::ArrayCopy(validity, this->Validity);
  //Create StepCountArray initialized to zero.
  viskores::cont::ArrayHandleConstant<viskores::Id> streamLengths(0, this->NumParticles);
  viskores::cont::ArrayCopy(streamLengths, this->StreamLengths);

  // Initialize InitLengths
  viskores::Id numSeeds = static_cast<viskores::Id>(particles.GetNumberOfValues());
  viskores::cont::ArrayHandleIndex idxArray(numSeeds);
  viskores::cont::Invoker invoker;
  invoker(detail::GetSteps{}, particles, this->InitialLengths);
}

template <typename ParticleType>
VISKORES_CONT void StreamlineAnalysis<ParticleType>::FinalizeAnalysis(
  viskores::cont::ArrayHandle<ParticleType>& particles)
{
  viskores::Id numSeeds = particles.GetNumberOfValues();
  viskores::cont::ArrayHandle<viskores::Vec3f> positions;
  viskores::cont::Algorithm::CopyIf(this->Streams, this->Validity, positions, IsOne());
  viskores::cont::Algorithm::Copy(positions, this->Streams);

  // Create the cells
  viskores::cont::ArrayHandle<viskores::Id> numPoints;
  viskores::cont::Invoker invoker;
  invoker(detail::ComputeNumPoints{}, particles, this->InitialLengths, numPoints);

  viskores::cont::ArrayHandle<viskores::Id> cellIndex;
  viskores::Id connectivityLen = viskores::cont::Algorithm::ScanExclusive(numPoints, cellIndex);
  viskores::cont::ArrayHandleIndex connCount(connectivityLen);
  viskores::cont::ArrayHandle<viskores::Id> connectivity;
  viskores::cont::ArrayCopy(connCount, connectivity);

  viskores::cont::ArrayHandle<viskores::UInt8> cellTypes;
  auto polyLineShape = viskores::cont::make_ArrayHandleConstant<viskores::UInt8>(
    viskores::CELL_SHAPE_POLY_LINE, numSeeds);
  viskores::cont::ArrayCopy(polyLineShape, cellTypes);

  auto offsets = viskores::cont::ConvertNumComponentsToOffsets(numPoints);

  this->PolyLines.Fill(this->Streams.GetNumberOfValues(), cellTypes, connectivity, offsets);
  this->Particles = particles;
}

template <typename ParticleType>
VISKORES_CONT bool StreamlineAnalysis<ParticleType>::MakeDataSet(
  viskores::cont::DataSet& dataset,
  const std::vector<StreamlineAnalysis<ParticleType>>& results)
{
  size_t nResults = results.size();
  if (nResults == 1)
  {
    const auto& res = results[0];
    dataset.AddCoordinateSystem(viskores::cont::CoordinateSystem("coordinates", res.Streams));
    dataset.SetCellSet(res.PolyLines);
  }
  else
  {
    std::vector<viskores::Id> posOffsets(nResults, 0);
    viskores::Id totalNumCells = 0, totalNumPts = 0;
    for (std::size_t i = 0; i < nResults; i++)
    {
      const auto& res = results[i];
      if (i == 0)
        posOffsets[i] = 0;
      else
        posOffsets[i] = totalNumPts;

      totalNumPts += res.Streams.GetNumberOfValues();
      totalNumCells += res.PolyLines.GetNumberOfCells();
    }

    //Append all the points together.
    viskores::cont::ArrayHandle<viskores::Vec3f> appendPts;
    appendPts.Allocate(totalNumPts);
    for (std::size_t i = 0; i < nResults; i++)
    {
      const auto& res = results[i];
      // copy all values into appendPts starting at offset.
      viskores::cont::Algorithm::CopySubRange(
        res.Streams, 0, res.Streams.GetNumberOfValues(), appendPts, posOffsets[i]);
    }
    dataset.AddCoordinateSystem(viskores::cont::CoordinateSystem("coordinates", appendPts));

    //Create polylines.
    std::vector<viskores::Id> numPtsPerCell(static_cast<std::size_t>(totalNumCells));
    std::size_t off = 0;
    for (std::size_t i = 0; i < nResults; i++)
    {
      const auto& res = results[i];
      viskores::Id nCells = res.PolyLines.GetNumberOfCells();
      for (viskores::Id j = 0; j < nCells; j++)
        numPtsPerCell[off++] = static_cast<viskores::Id>(res.PolyLines.GetNumberOfPointsInCell(j));
    }

    auto numPointsPerCellArray =
      viskores::cont::make_ArrayHandle(numPtsPerCell, viskores::CopyFlag::Off);

    viskores::cont::ArrayHandle<viskores::Id> cellIndex;
    viskores::Id connectivityLen =
      viskores::cont::Algorithm::ScanExclusive(numPointsPerCellArray, cellIndex);
    viskores::cont::ArrayHandleIndex connCount(connectivityLen);
    viskores::cont::ArrayHandle<viskores::Id> connectivity;
    viskores::cont::ArrayCopy(connCount, connectivity);

    viskores::cont::ArrayHandle<viskores::UInt8> cellTypes;
    auto polyLineShape = viskores::cont::make_ArrayHandleConstant<viskores::UInt8>(
      viskores::CELL_SHAPE_POLY_LINE, totalNumCells);
    viskores::cont::ArrayCopy(polyLineShape, cellTypes);
    auto offsets = viskores::cont::ConvertNumComponentsToOffsets(numPointsPerCellArray);

    viskores::cont::CellSetExplicit<> polyLines;
    polyLines.Fill(totalNumPts, cellTypes, connectivity, offsets);
    dataset.SetCellSet(polyLines);
  }
  return true;
}

template class VISKORES_FILTER_FLOW_EXPORT NoAnalysis<viskores::Particle>;
template class VISKORES_FILTER_FLOW_EXPORT NoAnalysis<viskores::ChargedParticle>;
template class VISKORES_FILTER_FLOW_EXPORT StreamlineAnalysis<viskores::Particle>;
template class VISKORES_FILTER_FLOW_EXPORT StreamlineAnalysis<viskores::ChargedParticle>;

} // namespace flow
} // namespace worklet
} // namespace viskores
