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
#ifndef viskores_cont_internal_ReverseConnectivityBuilder_h
#define viskores_cont_internal_ReverseConnectivityBuilder_h

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleCast.h>
#include <viskores/cont/ArrayHandleConstant.h>
#include <viskores/cont/CellSetExplicit.h>

#include <viskores/cont/AtomicArray.h>
#include <viskores/exec/FunctorBase.h>

#include <utility>

namespace viskores
{
namespace cont
{
namespace internal
{

namespace rcb
{

template <typename AtomicHistogram, typename ConnInPortal, typename RConnToConnIdxCalc>
struct BuildHistogram : public viskores::exec::FunctorBase
{
  AtomicHistogram Histo;
  ConnInPortal Conn;
  RConnToConnIdxCalc IdxCalc;

  VISKORES_CONT
  BuildHistogram(const AtomicHistogram& histo,
                 const ConnInPortal& conn,
                 const RConnToConnIdxCalc& idxCalc)
    : Histo(histo)
    , Conn(conn)
    , IdxCalc(idxCalc)
  {
  }

  VISKORES_EXEC
  void operator()(viskores::Id rconnIdx) const
  {
    // Compute the connectivity array index (skipping cell length entries)
    const viskores::Id connIdx = this->IdxCalc(rconnIdx);
    const viskores::Id ptId = this->Conn.Get(connIdx);
    this->Histo.Add(ptId, 1);
  }
};

template <typename AtomicHistogram,
          typename ConnInPortal,
          typename ROffsetInPortal,
          typename RConnOutPortal,
          typename RConnToConnIdxCalc,
          typename ConnIdxToCellIdxCalc>
struct GenerateRConn : public viskores::exec::FunctorBase
{
  AtomicHistogram Histo;
  ConnInPortal Conn;
  ROffsetInPortal ROffsets;
  RConnOutPortal RConn;
  RConnToConnIdxCalc IdxCalc;
  ConnIdxToCellIdxCalc CellIdCalc;

  VISKORES_CONT
  GenerateRConn(const AtomicHistogram& histo,
                const ConnInPortal& conn,
                const ROffsetInPortal& rOffsets,
                const RConnOutPortal& rconn,
                const RConnToConnIdxCalc& idxCalc,
                const ConnIdxToCellIdxCalc& cellIdCalc)
    : Histo(histo)
    , Conn(conn)
    , ROffsets(rOffsets)
    , RConn(rconn)
    , IdxCalc(idxCalc)
    , CellIdCalc(cellIdCalc)
  {
  }

  VISKORES_EXEC
  void operator()(viskores::Id inputIdx) const
  {
    // Compute the connectivity array index (skipping cell length entries)
    const viskores::Id connIdx = this->IdxCalc(inputIdx);
    const viskores::Id ptId = this->Conn.Get(connIdx);

    // Compute the cell id:
    const viskores::Id cellId = this->CellIdCalc(connIdx);

    // Find the base offset for this point id:
    const viskores::Id baseOffset = this->ROffsets.Get(ptId);

    // Find the next unused index for this point id
    const viskores::Id nextAvailable = this->Histo.Add(ptId, 1);

    // Update the final location in the RConn table with the cellId
    const viskores::Id rconnIdx = baseOffset + nextAvailable;
    this->RConn.Set(rconnIdx, cellId);
  }
};
}
/// Takes a connectivity array handle (conn) and constructs a reverse
/// connectivity table suitable for use by Viskores (rconn).
///
/// This code is generalized for use by VTK and Viskores.
///
/// The Run(...) method is the main entry point. The template parameters are:
/// @param RConnToConnIdxCalc defines `viskores::Id operator()(viskores::Id in) const`
/// which computes the index of the in'th point id in conn. This is necessary
/// for VTK-style cell arrays that need to skip the cell length entries. In
/// viskores, this is a no-op passthrough.
/// @param ConnIdxToCellIdxCalc Functor that computes the cell id from an
/// index into conn.
/// @param ConnTag is the StorageTag for the input connectivity array.
///
/// See usages in viskoresCellSetExplicit and viskoresCellSetSingleType for examples.
class ReverseConnectivityBuilder
{
public:
  VISKORES_CONT
  template <typename ConnArray,
            typename RConnArray,
            typename ROffsetsArray,
            typename RConnToConnIdxCalc,
            typename ConnIdxToCellIdxCalc>
  inline void Run(const ConnArray& conn,
                  RConnArray& rConn,
                  ROffsetsArray& rOffsets,
                  const RConnToConnIdxCalc& rConnToConnCalc,
                  const ConnIdxToCellIdxCalc& cellIdCalc,
                  viskores::Id numberOfPoints,
                  viskores::Id rConnSize,
                  viskores::cont::DeviceAdapterId device)
  {
    viskores::cont::Token connToken;
    auto connPortal = conn.PrepareForInput(device, connToken);
    auto zeros =
      viskores::cont::make_ArrayHandleConstant(viskores::IdComponent{ 0 }, numberOfPoints);

    // Compute RConn offsets by atomically building a histogram and doing an
    // extended scan.
    //
    // Example:
    // (in)  Conn:  | 3  0  1  2  |  3  0  1  3  |  3  0  3  4  |  3  3  4  5  |
    // (out) RNumIndices:  3  2  1  3  2  1
    // (out) RIdxOffsets:  0  3  5  6  9 11 12
    viskores::cont::ArrayHandle<viskores::IdComponent> rNumIndices;
    { // allocate and zero the numIndices array:
      viskores::cont::Algorithm::Copy(device, zeros, rNumIndices);
    }

    { // Build histogram:
      viskores::cont::AtomicArray<viskores::IdComponent> atomicCounter{ rNumIndices };
      viskores::cont::Token token;
      auto ac = atomicCounter.PrepareForExecution(device, token);
      using BuildHisto =
        rcb::BuildHistogram<decltype(ac), decltype(connPortal), RConnToConnIdxCalc>;
      BuildHisto histoGen{ ac, connPortal, rConnToConnCalc };

      viskores::cont::Algorithm::Schedule(device, histoGen, rConnSize);
    }

    { // Compute offsets:
      viskores::cont::Algorithm::ScanExtended(
        device, viskores::cont::make_ArrayHandleCast<viskores::Id>(rNumIndices), rOffsets);
    }

    { // Reset the numIndices array to 0's:
      viskores::cont::Algorithm::Copy(device, zeros, rNumIndices);
    }

    // Fill the connectivity table:
    // 1) Lookup each point idx base offset.
    // 2) Use the atomic histogram to find the next available slot for this
    //    pt id in RConn.
    // 3) Compute the cell id from the connectivity index
    // 4) Update RConn[nextSlot] = cellId
    //
    // Example:
    // (in)    Conn:  | 3  0  1  2  |  3  0  1  3  |  3  0  3  4  |  3  3  4  5  |
    // (inout) RNumIndices:  0  0  0  0  0  0  (Initial)
    // (inout) RNumIndices:  3  2  1  3  2  1  (Final)
    // (in)    RIdxOffsets:  0  3  5  6  9  11
    // (out)   RConn: | 0  1  2  |  0  1  |  0  |  1  2  3  |  2  3  |  3  |
    {
      viskores::cont::AtomicArray<viskores::IdComponent> atomicCounter{ rNumIndices };
      viskores::cont::Token token;
      auto ac = atomicCounter.PrepareForExecution(device, token);
      auto rOffsetPortal = rOffsets.PrepareForInput(device, token);
      auto rConnPortal = rConn.PrepareForOutput(rConnSize, device, token);

      using GenRConnT = rcb::GenerateRConn<decltype(ac),
                                           decltype(connPortal),
                                           decltype(rOffsetPortal),
                                           decltype(rConnPortal),
                                           RConnToConnIdxCalc,
                                           ConnIdxToCellIdxCalc>;
      GenRConnT rConnGen{ ac, connPortal, rOffsetPortal, rConnPortal, rConnToConnCalc, cellIdCalc };

      viskores::cont::Algorithm::Schedule(device, rConnGen, rConnSize);
    }
  }
};

// Pass through (needed for ReverseConnectivityBuilder)
struct PassThrough
{
  VISKORES_EXEC viskores::Id operator()(const viskores::Id& val) const { return val; }
};

// Compute cell id from input connectivity:
// Find the upper bound of the conn idx in the offsets table and subtract 1
//
// Example:
// Offsets: |  0        |  3        |  6           |  10       |
// Conn:    |  0  1  2  |  0  1  3  |  2  4  5  6  |  1  3  5  |
// ConnIdx: |  0  1  2  |  3  4  5  |  6  7  8  9  |  10 11 12 |
// UpprBnd: |  1  1  1  |  2  2  2  |  3  3  3  3  |  4  4  4  |
// CellIdx: |  0  0  0  |  1  1  1  |  2  2  2  2  |  3  3  3  |
template <typename OffsetsPortalType>
struct ConnIdxToCellIdCalc
{
  OffsetsPortalType Offsets;

  VISKORES_CONT
  ConnIdxToCellIdCalc(const OffsetsPortalType& offsets)
    : Offsets(offsets)
  {
  }

  VISKORES_EXEC
  viskores::Id operator()(viskores::Id inIdx) const
  {
    // Compute the upper bound index:
    viskores::Id upperBoundIdx;
    {
      viskores::Id first = 0;
      viskores::Id length = this->Offsets.GetNumberOfValues();

      while (length > 0)
      {
        viskores::Id halfway = length / 2;
        viskores::Id pos = first + halfway;
        viskores::Id val = this->Offsets.Get(pos);
        if (val <= inIdx)
        {
          first = pos + 1;
          length -= halfway + 1;
        }
        else
        {
          length = halfway;
        }
      }

      upperBoundIdx = first;
    }

    return upperBoundIdx - 1;
  }
};

// Much easier for CellSetSingleType:
struct ConnIdxToCellIdCalcSingleType
{
  viskores::IdComponent CellSize;

  VISKORES_CONT
  ConnIdxToCellIdCalcSingleType(viskores::IdComponent cellSize)
    : CellSize(cellSize)
  {
  }

  VISKORES_EXEC
  viskores::Id operator()(viskores::Id inIdx) const { return inIdx / this->CellSize; }
};

template <typename ConnTableT, typename RConnTableT>
void ComputeRConnTable(RConnTableT& rConnTable,
                       const ConnTableT& connTable,
                       viskores::Id numberOfPoints,
                       viskores::cont::DeviceAdapterId device)
{
  if (rConnTable.ElementsValid)
  {
    return;
  }

  const auto& conn = connTable.Connectivity;
  auto& rConn = rConnTable.Connectivity;
  auto& rOffsets = rConnTable.Offsets;
  const viskores::Id rConnSize = conn.GetNumberOfValues();

  {
    viskores::cont::Token token;
    const auto offInPortal = connTable.Offsets.PrepareForInput(device, token);

    PassThrough idxCalc{};
    ConnIdxToCellIdCalc<decltype(offInPortal)> cellIdCalc{ offInPortal };

    viskores::cont::internal::ReverseConnectivityBuilder builder;
    builder.Run(conn, rConn, rOffsets, idxCalc, cellIdCalc, numberOfPoints, rConnSize, device);
  }

  rConnTable.Shapes = viskores::cont::make_ArrayHandleConstant(
    static_cast<viskores::UInt8>(CELL_SHAPE_VERTEX), numberOfPoints);
  rConnTable.ElementsValid = true;
}

// Specialize for CellSetSingleType:
template <typename RConnTableT, typename ConnectivityStorageTag>
void ComputeRConnTable(
  RConnTableT& rConnTable,
  const ConnectivityExplicitInternals< // SingleType specialization types:
    typename viskores::cont::ArrayHandleConstant<viskores::UInt8>::StorageTag,
    ConnectivityStorageTag,
    typename viskores::cont::ArrayHandleCounting<viskores::Id>::StorageTag>& connTable,
  viskores::Id numberOfPoints,
  viskores::cont::DeviceAdapterId device)
{
  if (rConnTable.ElementsValid)
  {
    return;
  }

  const auto& conn = connTable.Connectivity;
  auto& rConn = rConnTable.Connectivity;
  auto& rOffsets = rConnTable.Offsets;
  const viskores::Id rConnSize = conn.GetNumberOfValues();

  const viskores::IdComponent cellSize = [&]() -> viskores::IdComponent
  {
    if (connTable.Offsets.GetNumberOfValues() >= 2)
    {
      const auto firstTwo = viskores::cont::ArrayGetValues({ 0, 1 }, connTable.Offsets);
      return static_cast<viskores::IdComponent>(firstTwo[1] - firstTwo[0]);
    }
    return 0;
  }();

  PassThrough idxCalc{};
  ConnIdxToCellIdCalcSingleType cellIdCalc{ cellSize };

  viskores::cont::internal::ReverseConnectivityBuilder builder;
  builder.Run(conn, rConn, rOffsets, idxCalc, cellIdCalc, numberOfPoints, rConnSize, device);

  rConnTable.Shapes = viskores::cont::make_ArrayHandleConstant(
    static_cast<viskores::UInt8>(CELL_SHAPE_VERTEX), numberOfPoints);
  rConnTable.ElementsValid = true;
}

}
}
} // end namespace viskores::cont::internal

#endif // ReverseConnectivityBuilder_h
