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

#include <viskores/rendering/AxisAnnotation.h>

namespace viskores
{
namespace rendering
{

namespace
{

inline viskores::Float64 ffix(viskores::Float64 value)
{
  int ivalue = (int)(value);
  viskores::Float64 v = (value - ivalue);
  if (v > 0.9999)
  {
    ivalue++;
  }
  return static_cast<viskores::Float64>(ivalue);
}

} // anonymous namespace

void AxisAnnotation::CalculateTicks(const viskores::Range& range,
                                    bool minor,
                                    std::vector<viskores::Float64>& positions,
                                    std::vector<viskores::Float64>& proportions,
                                    int modifyTickQuantity)
{
  positions.clear();
  proportions.clear();

  if (!range.IsNonEmpty())
  {
    return;
  }

  viskores::Float64 length = range.Length();

  // Find the integral points.
  viskores::Float64 pow10 = log10(length);

  // Build in numerical tolerance
  viskores::Float64 eps = 10.0e-10;
  pow10 += eps;

  // ffix moves you in the wrong direction if pow10 is negative.
  if (pow10 < 0.)
  {
    pow10 = pow10 - 1.;
  }

  viskores::Float64 fxt = viskores::Pow(10., ffix(pow10));

  // Find the number of integral points in the interval.
  int numTicks = int(ffix(length / fxt) + 1);

  // We should get about major 10 ticks on a length that's near
  // the power of 10.  (e.g. length=1000).  If the length is small
  // enough we have less than 5 ticks (e.g. length=400), then
  // divide the step by 2, or if it's about 2 ticks (e.g. length=150)
  // or less, then divide the step by 5.  That gets us back to
  // about 10 major ticks.
  //
  // But we might want more or less.  To adjust this up by
  // approximately a factor of 2, instead of the default
  // 1/2/5 dividers, use 2/5/10, and to adjust it down by
  // about a factor of two, use .5/1/2 as the dividers.
  // (We constrain to 1s, 2s, and 5s, for the obvious reason
  // that only those values are factors of 10.....)
  viskores::Float64 divs[5] = { 0.5, 1, 2, 5, 10 };
  int divindex = (numTicks >= 5) ? 1 : (numTicks >= 3 ? 2 : 3);
  divindex += modifyTickQuantity;

  viskores::Float64 div = divs[divindex];

  // If there aren't enough major tick points in this decade, use the next
  // decade.
  viskores::Float64 majorStep = fxt / div;
  viskores::Float64 minorStep = (fxt / div) / 10.;

  // When we get too close, we lose the tickmarks. Run some special case code.
  if (numTicks <= 1)
  {
    if (minor)
    {
      // no minor ticks
      return;
    }
    else
    {
      positions.resize(3);
      proportions.resize(3);
      positions[0] = range.Min;
      positions[1] = range.Center();
      positions[2] = range.Max;
      proportions[0] = 0.0;
      proportions[1] = 0.5;
      proportions[2] = 1.0;
      return;
    }
  }

  // Figure out the first major and minor tick locations, relative to the
  // start of the axis.
  viskores::Float64 majorStart, minorStart;
  if (range.Min < 0.)
  {
    majorStart = majorStep * (ffix(range.Min * (1. / majorStep)));
    minorStart = minorStep * (ffix(range.Min * (1. / minorStep)));
  }
  else
  {
    majorStart = majorStep * (ffix(range.Min * (1. / majorStep) + .999));
    minorStart = minorStep * (ffix(range.Min * (1. / minorStep) + .999));
  }

  // Create all of the minor ticks
  const int max_count_cutoff = 1000;
  numTicks = 0;
  viskores::Float64 location = minor ? minorStart : majorStart;
  viskores::Float64 step = minor ? minorStep : majorStep;
  while (location <= range.Max && numTicks < max_count_cutoff)
  {
    positions.push_back(location);
    proportions.push_back((location - range.Min) / length);
    numTicks++;
    location += step;
  }
}

void AxisAnnotation::CalculateTicksLogarithmic(const viskores::Range& range,
                                               bool minor,
                                               std::vector<viskores::Float64>& positions,
                                               std::vector<viskores::Float64>& proportions)
{
  positions.clear();
  proportions.clear();

  // Sort the min and max range to account for range modification due to log functions
  viskores::Range sortedRange;
  sortedRange.Min = range.Min < range.Max ? range.Min : range.Max;
  sortedRange.Max = range.Min > range.Max ? range.Min : range.Max;

  if (!sortedRange.IsNonEmpty())
  {
    return;
  }

  viskores::Float64 first_log = viskores::Ceil(sortedRange.Min);
  viskores::Float64 last_log = viskores::Floor(sortedRange.Max);

  if (last_log <= first_log)
  {
    last_log = first_log + 1;
  }
  viskores::Float64 diff_log = last_log - first_log;
  auto step = viskores::Int32((diff_log + 9) / 10);

  if (minor)
  {
    first_log -= step;
    last_log += step;
  }

  for (auto i = viskores::Int32(first_log); i <= last_log; i += step)
  {
    viskores::Float64 logpos = i;
    viskores::Float64 pos = viskores::Pow(10, logpos);
    if (minor)
    {
      // If we're showing major tickmarks for every power of 10,
      // then show 2x10^n, 3x10^n, ..., 9x10^n for minor ticks.
      // If we're skipping some powers of 10, then use the minor
      // ticks to show where those skipped ones are.  (Beyond
      // a range of 100 orders of magnitude, we get more than 10
      // minor ticks per major tick, but that's awfully rare.)
      if (step == 1)
      {
        for (viskores::Int32 j = 1; j < 10; ++j)
        {
          viskores::Float64 minor_pos = viskores::Float64(j) * viskores::Float64(pos);
          viskores::Float64 minor_logpos = viskores::Log10(minor_pos);
          if (minor_logpos < sortedRange.Min || minor_logpos > sortedRange.Max)
          {
            continue;
          }
          positions.push_back(minor_pos);
          proportions.push_back((minor_logpos - sortedRange.Min) /
                                (sortedRange.Max - sortedRange.Max));
        }
      }
      else
      {
        for (viskores::Int32 j = 1; j < step; ++j)
        {
          viskores::Float64 minor_logpos = logpos + j;
          viskores::Float64 minor_pos = viskores::Pow(10., minor_logpos);
          if (minor_logpos < sortedRange.Min || minor_logpos > sortedRange.Max)
          {
            continue;
          }
          positions.push_back(minor_pos);
          proportions.push_back((minor_logpos - sortedRange.Min) /
                                (sortedRange.Max - sortedRange.Min));
        }
      }
    }
    else
    {
      if (logpos > sortedRange.Max)
      {
        break;
      }
      positions.push_back(pos);
      proportions.push_back((logpos - sortedRange.Min) / (sortedRange.Max - sortedRange.Min));
    }
  }
}
}
} // namespace viskores::rendering
