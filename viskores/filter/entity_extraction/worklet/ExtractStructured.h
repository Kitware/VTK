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
#ifndef viskores_worklet_ExtractStructured_h
#define viskores_worklet_ExtractStructured_h

#include <viskores/RangeId3.h>
#include <viskores/cont/ArrayCopyDevice.h>
#include <viskores/cont/ArrayGetValues.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleCartesianProduct.h>
#include <viskores/cont/ArrayHandleCounting.h>
#include <viskores/cont/ArrayHandleImplicit.h>
#include <viskores/cont/ArrayHandlePermutation.h>
#include <viskores/cont/ArrayHandleTransform.h>
#include <viskores/cont/ArrayHandleUniformPointCoordinates.h>
#include <viskores/cont/CellSetList.h>
#include <viskores/cont/CellSetStructured.h>
#include <viskores/cont/CoordinateSystem.h>
#include <viskores/cont/Invoker.h>
#include <viskores/cont/UncertainCellSet.h>
#include <viskores/cont/UnknownArrayHandle.h>
#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace worklet
{

namespace extractstructured
{
namespace internal
{

class SubArrayPermutePoints
{
public:
  SubArrayPermutePoints() = default;

  SubArrayPermutePoints(viskores::Id size,
                        viskores::Id first,
                        viskores::Id last,
                        viskores::Id stride,
                        bool includeBoundary)
    : MaxIdx(size - 1)
    , First(first)
    , Last(last)
    , Stride(stride)
    , IncludeBoundary(includeBoundary)
  {
  }

  VISKORES_EXEC_CONT
  viskores::Id operator()(viskores::Id idx) const
  {
    return (this->IncludeBoundary && (idx == this->MaxIdx)) ? (this->Last)
                                                            : (this->First + (idx * this->Stride));
  }

private:
  viskores::Id MaxIdx;
  viskores::Id First, Last;
  viskores::Id Stride;
  bool IncludeBoundary;
};

struct ExtractCopy : public viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, FieldOut, WholeArrayIn);

  explicit ExtractCopy(const viskores::Id3& dim)
    : XDim(dim[0])
    , XYDim(dim[0] * dim[1])
  {
  }
  VISKORES_EXEC_CONT
  inline viskores::Id ToFlat(const viskores::Id3& index) const
  {
    return index[0] + index[1] * this->XDim + index[2] * this->XYDim;
  }

  template <typename ScalarType, typename WholeFieldIn>
  VISKORES_EXEC void operator()(viskores::Id3& index,
                                ScalarType& output,
                                const WholeFieldIn& inputField) const
  {
    output = inputField.Get(this->ToFlat(index));
  }

  viskores::Id XDim;
  viskores::Id XYDim;
};
}
} // extractstructured::internal

class ExtractStructured
{
public:
  using UncertainCellSetStructured =
    viskores::cont::UncertainCellSet<viskores::cont::CellSetListStructured>;

private:
  using AxisIndexArrayPoints =
    viskores::cont::ArrayHandleImplicit<extractstructured::internal::SubArrayPermutePoints>;
  using PointIndexArray = viskores::cont::
    ArrayHandleCartesianProduct<AxisIndexArrayPoints, AxisIndexArrayPoints, AxisIndexArrayPoints>;

  using AxisIndexArrayCells = viskores::cont::ArrayHandleCounting<viskores::Id>;
  using CellIndexArray = viskores::cont::
    ArrayHandleCartesianProduct<AxisIndexArrayCells, AxisIndexArrayCells, AxisIndexArrayCells>;

  static inline AxisIndexArrayPoints MakeAxisIndexArrayPoints(viskores::Id count,
                                                              viskores::Id first,
                                                              viskores::Id last,
                                                              viskores::Id stride,
                                                              bool includeBoundary)
  {
    auto fnctr = extractstructured::internal::SubArrayPermutePoints(
      count, first, last, stride, includeBoundary);
    return viskores::cont::make_ArrayHandleImplicit(fnctr, count);
  }

  static inline AxisIndexArrayCells MakeAxisIndexArrayCells(viskores::Id count,
                                                            viskores::Id start,
                                                            viskores::Id stride)
  {
    return viskores::cont::make_ArrayHandleCounting(start, stride, count);
  }

  static UncertainCellSetStructured MakeCellSetStructured(
    const viskores::Id3& inputPointDims,
    const viskores::Id3& inputOffsets,
    const viskores::Id3& inputGlobalPointDims,
    viskores::IdComponent forcedDimensionality = 0)
  {
    // when the point dimension for a given axis is 1 we
    // need to lower the dimensonality by 1. So a Plane
    // in XZ space would have a dimensonality of 2.
    // likewise the global offsets need to also
    // be updated when this occurs
    viskores::IdComponent dimensionality = forcedDimensionality;
    viskores::Id3 dimensions = inputPointDims;
    viskores::Id3 offset = inputOffsets;
    viskores::Id3 globalDimensions = inputGlobalPointDims;
    for (int i = 0; i < 3 && (forcedDimensionality == 0); ++i)
    {
      if (inputPointDims[i] > 1)
      {
        dimensions[dimensionality] = inputPointDims[i];
        offset[dimensionality] = inputOffsets[i];
        // TODO/FIXME: This may not be the correct way to handle global point dims.
        // E.g., if we preserve the input global point dims (default) then they may
        // have a higher dimensionility than the returned data set. In that case,
        // the approach here will result in an incorrect value for GlobalPointDimensions.
        // This is the simplest approach, which should work in most use cases for this
        // filter, but if this choice causes further problems down the way, we may need
        // to rethink it.
        globalDimensions[dimensionality] = inputGlobalPointDims[i];

        ++dimensionality;
      }
    }

    switch (dimensionality)
    {
      case 1:
      {
        viskores::cont::CellSetStructured<1> outCs;
        outCs.SetPointDimensions(dimensions[0]);
        outCs.SetGlobalPointIndexStart(offset[0]);
        outCs.SetGlobalPointDimensions(globalDimensions[0]);
        return outCs;
      }
      case 2:
      {
        viskores::cont::CellSetStructured<2> outCs;
        outCs.SetPointDimensions(viskores::Id2(dimensions[0], dimensions[1]));
        outCs.SetGlobalPointIndexStart(viskores::Id2(offset[0], offset[1]));
        outCs.SetGlobalPointDimensions(viskores::Id2(globalDimensions[0], globalDimensions[1]));
        return outCs;
      }
      case 3:
      {
        viskores::cont::CellSetStructured<3> outCs;
        outCs.SetPointDimensions(dimensions);
        outCs.SetGlobalPointIndexStart(offset);
        outCs.SetGlobalPointDimensions(globalDimensions);
        return outCs;
      }
      default:
        return {};
    }
  }

public:
  inline UncertainCellSetStructured Run(const viskores::cont::CellSetStructured<1>& cellset,
                                        const viskores::RangeId3& voi,
                                        const viskores::Id3& sampleRate,
                                        bool includeBoundary,
                                        bool includeOffset)
  {
    viskores::Id pdims = cellset.GetPointDimensions();
    viskores::Id offsets = cellset.GetGlobalPointIndexStart();
    viskores::Id gpdims = cellset.GetGlobalPointDimensions();
    return this->Compute(1,
                         viskores::Id3{ pdims, 1, 1 },
                         viskores::Id3{ offsets, 0, 0 },
                         viskores::Id3{ gpdims, 1, 1 },
                         voi,
                         sampleRate,
                         includeBoundary,
                         includeOffset);
  }

  inline UncertainCellSetStructured Run(const viskores::cont::CellSetStructured<2>& cellset,
                                        const viskores::RangeId3& voi,
                                        const viskores::Id3& sampleRate,
                                        bool includeBoundary,
                                        bool includeOffset)
  {
    viskores::Id2 pdims = cellset.GetPointDimensions();
    viskores::Id2 offsets = cellset.GetGlobalPointIndexStart();
    viskores::Id2 gpdims = cellset.GetGlobalPointDimensions();
    return this->Compute(2,
                         viskores::Id3{ pdims[0], pdims[1], 1 },
                         viskores::Id3{ offsets[0], offsets[1], 0 },
                         viskores::Id3{ gpdims[0], gpdims[1], 1 },
                         voi,
                         sampleRate,
                         includeBoundary,
                         includeOffset);
  }

  inline UncertainCellSetStructured Run(const viskores::cont::CellSetStructured<3>& cellset,
                                        const viskores::RangeId3& voi,
                                        const viskores::Id3& sampleRate,
                                        bool includeBoundary,
                                        bool includeOffset)
  {
    viskores::Id3 pdims = cellset.GetPointDimensions();
    viskores::Id3 offsets = cellset.GetGlobalPointIndexStart();
    viskores::Id3 gpdims = cellset.GetGlobalPointDimensions();
    return this->Compute(
      3, pdims, offsets, gpdims, voi, sampleRate, includeBoundary, includeOffset);
  }

  UncertainCellSetStructured Compute(const int dimensionality,
                                     const viskores::Id3& ptdim,
                                     const viskores::Id3& offsets,
                                     const viskores::Id3& gpdims,
                                     const viskores::RangeId3& voi,
                                     const viskores::Id3& sampleRate,
                                     bool includeBoundary,
                                     bool includeOffset)
  {
    // Verify input parameters
    viskores::Id3 offset_vec(0, 0, 0);
    viskores::Id3 globalOffset(0, 0, 0);
    viskores::Id3 globalPointDimensions = gpdims;

    this->InputDimensions = ptdim;
    this->InputDimensionality = dimensionality;
    this->SampleRate = sampleRate;

    if (sampleRate[0] < 1 || sampleRate[1] < 1 || sampleRate[2] < 1)
    {
      throw viskores::cont::ErrorBadValue("Bad sampling rate");
    }
    if (includeOffset)
    {
      viskores::Id3 tmpDims = ptdim;
      offset_vec = offsets;
      for (int i = 0; i < dimensionality; ++i)
      {
        if (dimensionality > i)
        {
          if (offset_vec[i] >= voi[i].Min)
          {
            globalOffset[i] = offset_vec[i];
            this->VOI[i].Min = offset_vec[i];
            if (globalOffset[i] + ptdim[i] < voi[i].Max)
            {
              // Start from our GPIS (start point) up to the length of the
              // dimensions (if that is within VOI)
              this->VOI[i].Max = globalOffset[i] + ptdim[i];
            }
            else
            {
              // If it isn't within the voi we set our dimensions from the
              // GPIS up to the VOI.
              tmpDims[i] = voi[i].Max - globalOffset[i];
            }
          }
          else if (offset_vec[i] < voi[i].Min)
          {
            if (offset_vec[i] + ptdim[i] < voi[i].Min)
            {
              // If we're out of bounds we set the dimensions to 0. This
              // causes a return of UncertainCellSetStructured
              tmpDims[i] = 0;
            }
            else
            {
              // If our GPIS is less than VOI min, but our dimensions
              // include the VOI we go from the minimal value that we
              // can up to how far has been specified.
              globalOffset[i] = voi[i].Min;
              this->VOI[i].Min = voi[i].Min;
              if (globalOffset[i] + ptdim[i] < voi[i].Max)
              {
                this->VOI[i].Max = globalOffset[i] + ptdim[i];
              }
              else
              {
                tmpDims[i] = voi[i].Max - globalOffset[i];
              }
            }
          }
        }
      }
      this->OutputDimensions = viskores::Id3(tmpDims[0], tmpDims[1], tmpDims[2]);
    }

    this->VOI.X.Min = viskores::Max(viskores::Id(0), voi.X.Min);
    this->VOI.X.Max = viskores::Min(this->InputDimensions[0] + globalOffset[0], voi.X.Max);
    this->VOI.Y.Min = viskores::Max(viskores::Id(0), voi.Y.Min);
    this->VOI.Y.Max = viskores::Min(this->InputDimensions[1] + globalOffset[1], voi.Y.Max);
    this->VOI.Z.Min = viskores::Max(viskores::Id(0), voi.Z.Min);
    this->VOI.Z.Max = viskores::Min(this->InputDimensions[2] + globalOffset[2], voi.Z.Max);

    if (!this->VOI.IsNonEmpty())
    {
      viskores::Id3 empty = { 0, 0, 0 };
      return MakeCellSetStructured(empty, empty, globalPointDimensions, dimensionality);
    }
    if (!includeOffset)
    {
      // compute output dimensions
      this->OutputDimensions = viskores::Id3(1, 1, 1);
      viskores::Id3 voiDims = this->VOI.Dimensions();
      for (int i = 0; i < dimensionality; ++i)
      {
        this->OutputDimensions[i] = ((voiDims[i] + this->SampleRate[i] - 1) / this->SampleRate[i]) +
          ((includeBoundary && ((voiDims[i] - 1) % this->SampleRate[i])) ? 1 : 0);
      }
      this->ValidPoints = viskores::cont::make_ArrayHandleCartesianProduct(
        MakeAxisIndexArrayPoints(this->OutputDimensions[0],
                                 this->VOI.X.Min,
                                 this->VOI.X.Max - 1,
                                 this->SampleRate[0],
                                 includeBoundary),
        MakeAxisIndexArrayPoints(this->OutputDimensions[1],
                                 this->VOI.Y.Min,
                                 this->VOI.Y.Max - 1,
                                 this->SampleRate[1],
                                 includeBoundary),
        MakeAxisIndexArrayPoints(this->OutputDimensions[2],
                                 this->VOI.Z.Min,
                                 this->VOI.Z.Max - 1,
                                 this->SampleRate[2],
                                 includeBoundary));

      this->ValidCells = viskores::cont::make_ArrayHandleCartesianProduct(
        MakeAxisIndexArrayCells(viskores::Max(viskores::Id(1), this->OutputDimensions[0] - 1),
                                this->VOI.X.Min,
                                this->SampleRate[0]),
        MakeAxisIndexArrayCells(viskores::Max(viskores::Id(1), this->OutputDimensions[1] - 1),
                                this->VOI.Y.Min,
                                this->SampleRate[1]),
        MakeAxisIndexArrayCells(viskores::Max(viskores::Id(1), this->OutputDimensions[2] - 1),
                                this->VOI.Z.Min,
                                this->SampleRate[2]));

      // compute global point origin
      for (int i = 0; i < dimensionality; ++i)
      {
        globalOffset[i] = offsets[i] + this->VOI[i].Min;
      }
    }

    return MakeCellSetStructured(this->OutputDimensions, globalOffset, globalPointDimensions);
  }


private:
  class CallRun
  {
  public:
    CallRun(ExtractStructured* worklet,
            const viskores::RangeId3& voi,
            const viskores::Id3& sampleRate,
            bool includeBoundary,
            bool includeOffset,
            UncertainCellSetStructured& output)
      : Worklet(worklet)
      , VOI(&voi)
      , SampleRate(&sampleRate)
      , IncludeBoundary(includeBoundary)
      , IncludeOffset(includeOffset)
      , Output(&output)
    {
    }

    template <int N>
    void operator()(const viskores::cont::CellSetStructured<N>& cellset) const
    {
      *this->Output = this->Worklet->Run(
        cellset, *this->VOI, *this->SampleRate, this->IncludeBoundary, this->IncludeOffset);
    }

    template <typename CellSetType>
    void operator()(const CellSetType&) const
    {
      throw viskores::cont::ErrorBadType("ExtractStructured only works with structured datasets");
    }

  private:
    ExtractStructured* Worklet;
    const viskores::RangeId3* VOI;
    const viskores::Id3* SampleRate;
    bool IncludeBoundary;
    bool IncludeOffset;
    UncertainCellSetStructured* Output;
  };

public:
  template <typename CellSetList>
  UncertainCellSetStructured Run(const viskores::cont::UncertainCellSet<CellSetList>& cellset,
                                 const viskores::RangeId3& voi,
                                 const viskores::Id3& sampleRate,
                                 bool includeBoundary,
                                 bool includeOffset)
  {
    UncertainCellSetStructured output;
    CallRun cr(this, voi, sampleRate, includeBoundary, includeOffset, output);
    viskores::cont::CastAndCall(cellset, cr);
    return output;
  }

  using UniformCoordinatesArrayHandle = viskores::cont::ArrayHandleUniformPointCoordinates;

  using RectilinearCoordinatesArrayHandle = viskores::cont::ArrayHandleCartesianProduct<
    viskores::cont::ArrayHandle<viskores::FloatDefault>,
    viskores::cont::ArrayHandle<viskores::FloatDefault>,
    viskores::cont::ArrayHandle<viskores::FloatDefault>>;


  UniformCoordinatesArrayHandle MapCoordinatesUniform(
    const UniformCoordinatesArrayHandle& coords) const
  {
    using CoordsArray = viskores::cont::ArrayHandleUniformPointCoordinates;
    using CoordType = CoordsArray::ValueType;
    using ValueType = CoordType::ComponentType;

    const auto& portal = coords.ReadPortal();
    CoordType inOrigin = portal.GetOrigin();
    CoordType inSpacing = portal.GetSpacing();

    CoordType outOrigin =
      viskores::make_Vec(inOrigin[0] + static_cast<ValueType>(this->VOI.X.Min) * inSpacing[0],
                         inOrigin[1] + static_cast<ValueType>(this->VOI.Y.Min) * inSpacing[1],
                         inOrigin[2] + static_cast<ValueType>(this->VOI.Z.Min) * inSpacing[2]);
    CoordType outSpacing = inSpacing * static_cast<CoordType>(this->SampleRate);

    return { this->OutputDimensions, outOrigin, outSpacing };
  }

  RectilinearCoordinatesArrayHandle MapCoordinatesRectilinear(
    const RectilinearCoordinatesArrayHandle& coords) const
  {
    // For structured datasets, the cellsets are of different types based on
    // its dimensionality, but the coordinates are always 3 dimensional.
    // We can map the axis of the cellset to the coordinates by looking at the
    // length of a coordinate axis array.
    AxisIndexArrayPoints validIds[3] = { this->ValidPoints.GetFirstArray(),
                                         this->ValidPoints.GetSecondArray(),
                                         this->ValidPoints.GetThirdArray() };

    viskores::cont::ArrayHandle<viskores::FloatDefault> arrays[3] = { coords.GetFirstArray(),
                                                                      coords.GetSecondArray(),
                                                                      coords.GetThirdArray() };

    viskores::cont::ArrayHandle<viskores::FloatDefault> xyzs[3];
    int dim = 0;
    for (int i = 0; i < 3; ++i)
    {
      if (arrays[i].GetNumberOfValues() == 1)
      {
        xyzs[i].Allocate(1);
        xyzs[i].WritePortal().Set(0, viskores::cont::ArrayGetValue(0, arrays[i]));
      }
      else
      {
        viskores::cont::ArrayCopyDevice(
          viskores::cont::make_ArrayHandlePermutation(validIds[i], arrays[i]), xyzs[i]);
        ++dim;
      }
    }
    VISKORES_ASSERT(dim == this->InputDimensionality);

    return viskores::cont::make_ArrayHandleCartesianProduct(xyzs[0], xyzs[1], xyzs[2]);
  }

public:
  template <typename T, typename Storage>
  viskores::cont::ArrayHandle<T> ProcessPointField(
    const viskores::cont::ArrayHandle<T, Storage>& field) const
  {
    using namespace extractstructured::internal;
    viskores::cont::ArrayHandle<T> result;
    result.Allocate(this->ValidPoints.GetNumberOfValues());

    ExtractCopy worklet(this->InputDimensions);
    viskores::cont::Invoker invoke;
    invoke(worklet, this->ValidPoints, result, field);

    return result;
  }

  template <typename T, typename Storage>
  viskores::cont::ArrayHandle<T> ProcessCellField(
    const viskores::cont::ArrayHandle<T, Storage>& field) const
  {
    using namespace extractstructured::internal;
    viskores::cont::ArrayHandle<T> result;
    result.Allocate(this->ValidCells.GetNumberOfValues());

    auto inputCellDimensions = this->InputDimensions - viskores::Id3(1);
    ExtractCopy worklet(inputCellDimensions);
    viskores::cont::Invoker invoke;
    invoke(worklet, this->ValidCells, result, field);

    return result;
  }

private:
  viskores::RangeId3 VOI;
  viskores::Id3 SampleRate = { 1, 1, 1 };

  int InputDimensionality;
  viskores::Id3 InputDimensions;
  viskores::Id3 OutputDimensions;

  PointIndexArray ValidPoints;
  CellIndexArray ValidCells;
};
}
} // namespace viskores::worklet

#endif // viskores_worklet_ExtractStructured_h
