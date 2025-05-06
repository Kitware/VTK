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
#ifndef viskores_worklet_colorconversion_LookupTable_h
#define viskores_worklet_colorconversion_LookupTable_h

#include <viskores/cont/ColorTableSamples.h>

#include <viskores/exec/ColorTable.h>
#include <viskores/worklet/WorkletMapField.h>
#include <viskores/worklet/colorconversion/Conversions.h>

#include <float.h>

namespace viskores
{
namespace worklet
{
namespace colorconversion
{

using LookupTableTypes =
  viskores::List<viskores::Vec3ui_8, viskores::Vec4ui_8, viskores::Vec3f_32, viskores::Vec4f_64>;

struct LookupTable : public viskores::worklet::WorkletMapField
{
  viskores::Float32 Shift;
  viskores::Float32 Scale;
  viskores::Range TableRange;
  viskores::Int32 NumberOfSamples;

  //needs to support Nan, Above, Below Range colors
  VISKORES_CONT
  template <typename T>
  LookupTable(const T& colorTableSamples)
  {
    this->Shift = static_cast<viskores::Float32>(-colorTableSamples.SampleRange.Min);
    double rangeDelta = colorTableSamples.SampleRange.Length();
    if (rangeDelta < DBL_MIN * colorTableSamples.NumberOfSamples)
    {
      // if the range is tiny, anything within the range will map to the bottom
      // of the color scale.
      this->Scale = 0.0;
    }
    else
    {
      this->Scale = static_cast<viskores::Float32>(colorTableSamples.NumberOfSamples / rangeDelta);
    }
    this->TableRange = colorTableSamples.SampleRange;
    this->NumberOfSamples = colorTableSamples.NumberOfSamples;
  }

  using ControlSignature = void(FieldIn in, WholeArrayIn lookup, FieldOut color);
  using ExecutionSignature = void(_1, _2, _3);

  template <typename T, typename WholeFieldIn, typename U, int N>
  VISKORES_EXEC void operator()(const T& in,
                                const WholeFieldIn lookupTable,
                                viskores::Vec<U, N>& output) const
  {
    viskores::Float64 v = (static_cast<viskores::Float64>(in));
    viskores::Int32 idx = 1;

    //This logic uses how ColorTableSamples is constructed. See
    //viskores/cont/ColorTableSamples to see why we use these magic offset values
    if (viskores::IsNan(v))
    {
      idx = this->NumberOfSamples + 3;
    }
    else if (v < this->TableRange.Min)
    { //If we are below the color range
      idx = 0;
    }
    else if (v == this->TableRange.Min)
    { //If we are at the ranges min value
      idx = 1;
    }
    else if (v > this->TableRange.Max)
    { //If we are above the ranges max value
      idx = this->NumberOfSamples + 2;
    }
    else if (v == this->TableRange.Max)
    { //If we are at the ranges min value
      idx = this->NumberOfSamples;
    }
    else
    {
      v = (v + this->Shift) * this->Scale;
      // When v is very close to p.Range[1], the floating point calculation giving
      // idx may map above the highest value in the lookup table. That is why it
      // is padded
      idx = 1 + static_cast<viskores::Int32>(v);
    }
    output = lookupTable.Get(idx);
  }
};
}
}
}
#endif
