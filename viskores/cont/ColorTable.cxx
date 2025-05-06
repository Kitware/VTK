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
#include <algorithm>
#include <cctype>
#include <memory>

#include <viskores/cont/ColorTable.h>
#include <viskores/cont/ColorTableMap.h>
#include <viskores/cont/ErrorBadType.h>


namespace
{

template <typename T>
struct MinDelta
{
};
// This value seems to work well for viskores::Float32 ranges we have tested
template <>
struct MinDelta<viskores::Float32>
{
  static constexpr int value = 2048;
};
template <>
struct MinDelta<viskores::Float64>
{
  static constexpr viskores::Int64 value = 2048L;
};

// Reperesents the following:
// T m = std::numeric_limits<T>::min();
// EquivSizeIntT im;
// std::memcpy(&im, &m, sizeof(T));
//
template <typename EquivSizeIntT>
struct MinRepresentable
{
};
template <>
struct MinRepresentable<viskores::Float32>
{
  static constexpr int value = 8388608;
};
template <>
struct MinRepresentable<viskores::Float64>
{
  static constexpr viskores::Int64 value = 4503599627370496L;
};

inline bool rangeAlmostEqual(const viskores::Range& r)
{
  viskores::Int64 irange[2];
  // needs to be a memcpy to avoid strict aliasing issues, doing a count
  // of 2*sizeof(T) to couple both values at the same time
  std::memcpy(irange, &r.Min, sizeof(viskores::Int64));
  std::memcpy(irange + 1, &r.Max, sizeof(viskores::Int64));
  // determine the absolute delta between these two numbers.
  const viskores::Int64 delta = std::abs(irange[1] - irange[0]);
  // If the numbers are not nearly equal, we don't touch them. This avoids running into
  // pitfalls like BUG PV #17152.
  return (delta < 1024) ? true : false;
}

template <typename T>
inline viskores::Float64 expandRange(T r[2])
{
  constexpr bool is_float32_type = std::is_same<T, viskores::Float32>::value;
  using IRange = typename std::conditional<is_float32_type, viskores::Int32, viskores::Int64>::type;
  IRange irange[2];
  // needs to be a memcpy to avoid strict aliasing issues, doing a count
  // of 2*sizeof(T) to couple both values at the same time
  std::memcpy(irange, r, sizeof(T) * 2);

  const bool denormal = !std::isnormal(r[0]);
  const IRange minInt = MinRepresentable<T>::value;
  const IRange minDelta = denormal ? minInt + MinDelta<T>::value : MinDelta<T>::value;

  // determine the absolute delta between these two numbers.
  const viskores::Int64 delta = std::abs(irange[1] - irange[0]);

  // if our delta is smaller than the min delta push out the max value
  // so that it is equal to minRange + minDelta. When our range is entirely
  // negative we should instead subtract from our max, to max a larger negative
  // value
  if (delta < minDelta)
  {
    if (irange[0] < 0)
    {
      irange[1] = irange[0] - minDelta;
    }
    else
    {
      irange[1] = irange[0] + minDelta;
    }

    T result;
    std::memcpy(&result, irange + 1, sizeof(T));
    return static_cast<viskores::Float64>(result);
  }
  return static_cast<viskores::Float64>(r[1]);
}

inline viskores::Range adjustRange(const viskores::Range& r)
{
  const bool spans_zero_boundary = r.Min < 0 && r.Max > 0;
  if (spans_zero_boundary)
  { // nothing needs to be done, but this check is required.
    // if we convert into integer space the delta difference will overflow
    // an integer
    return r;
  }
  if (rangeAlmostEqual(r))
  {
    return r;
  }

  // range should be left untouched as much as possible to
  // to avoid loss of precision whenever possible. That is why
  // we only modify the Max value
  viskores::Range result = r;
  if (r.Min > static_cast<viskores::Float64>(std::numeric_limits<viskores::Float32>::lowest()) &&
      r.Max < static_cast<viskores::Float64>(std::numeric_limits<viskores::Float32>::max()))
  { //We've found it best to offset it in viskores::Float32 space if the numbers
    //lay inside that representable range
    viskores::Float32 frange[2] = { static_cast<viskores::Float32>(r.Min),
                                    static_cast<viskores::Float32>(r.Max) };
    result.Max = expandRange(frange);
  }
  else
  {
    viskores::Float64 drange[2] = { r.Min, r.Max };
    result.Max = expandRange(drange);
  }
  return result;
}


inline viskores::Vec3f_32 hsvTorgb(const viskores::Vec3f_32& hsv)
{
  viskores::Vec3f_32 rgb;
  constexpr viskores::Float32 onethird = 1.0f / 3.0f;
  constexpr viskores::Float32 onesixth = 1.0f / 6.0f;
  constexpr viskores::Float32 twothird = 2.0f / 3.0f;
  constexpr viskores::Float32 fivesixth = 5.0f / 6.0f;

  // compute RGB from HSV
  if (hsv[0] > onesixth && hsv[0] <= onethird) // green/red
  {
    rgb[1] = 1.0f;
    rgb[0] = (onethird - hsv[0]) * 6.0f;
    rgb[2] = 0.0f;
  }
  else if (hsv[0] > onethird && hsv[0] <= 0.5f) // green/blue
  {
    rgb[1] = 1.0f;
    rgb[2] = (hsv[0] - onethird) * 6.0f;
    rgb[0] = 0.0f;
  }
  else if (hsv[0] > 0.5 && hsv[0] <= twothird) // blue/green
  {
    rgb[2] = 1.0f;
    rgb[1] = (twothird - hsv[0]) * 6.0f;
    rgb[0] = 0.0f;
  }
  else if (hsv[0] > twothird && hsv[0] <= fivesixth) // blue/red
  {
    rgb[2] = 1.0f;
    rgb[0] = (hsv[0] - twothird) * 6.0f;
    rgb[1] = 0.0f;
  }
  else if (hsv[0] > fivesixth && hsv[0] <= 1.0) // red/blue
  {
    rgb[0] = 1.0f;
    rgb[2] = (1.0f - hsv[0]) * 6.0f;
    rgb[1] = 0.0f;
  }
  else // red/green
  {
    rgb[0] = 1.0f;
    rgb[1] = hsv[0] * 6;
    rgb[2] = 0.0f;
  }

  // add Saturation to the equation.
  rgb[0] = (hsv[1] * rgb[0] + (1.0f - hsv[1]));
  rgb[1] = (hsv[1] * rgb[1] + (1.0f - hsv[1]));
  rgb[2] = (hsv[1] * rgb[2] + (1.0f - hsv[1]));

  rgb[0] *= hsv[2];
  rgb[1] *= hsv[2];
  rgb[2] *= hsv[2];
  return rgb;
}

inline bool outside_vrange(viskores::Float64 x)
{
  return x < 0.0 || x > 1.0;
}
inline bool outside_vrange(viskores::Float32 x)
{
  return x < 0.0f || x > 1.0f;
}
template <typename T>
inline bool outside_vrange(const viskores::Vec<T, 2>& x)
{
  return outside_vrange(x[0]) || outside_vrange(x[1]);
}
template <typename T>
inline bool outside_vrange(const viskores::Vec<T, 3>& x)
{
  return outside_vrange(x[0]) || outside_vrange(x[1]) || outside_vrange(x[2]);
}

inline bool outside_range()
{
  return false;
}

template <typename T>
inline bool outside_range(T&& t)
{
  return outside_vrange(t);
}

template <typename T, typename U>
inline bool outside_range(T&& t, U&& u)
{
  return outside_vrange(t) || outside_vrange(u);
}

template <typename T, typename U, typename V, typename... Args>
inline bool outside_range(T&& t, U&& u, V&& v, Args&&... args)
{
  return outside_vrange(t) || outside_vrange(u) || outside_vrange(v) ||
    outside_range(std::forward<Args>(args)...);
}

template <typename T>
inline viskores::cont::ArrayHandle<T> buildSampleHandle(viskores::Int32 numSamples,
                                                        T start,
                                                        T end,
                                                        T inc,
                                                        bool appendNanAndRangeColors)
{

  //number of samples + end + appendNanAndRangeColors
  viskores::Int32 allocationSize = (appendNanAndRangeColors) ? numSamples + 5 : numSamples + 1;

  viskores::cont::ArrayHandle<T> handle;
  handle.Allocate(allocationSize);

  auto portal = handle.WritePortal();
  viskores::Id index = 0;

  //Insert the below range first
  if (appendNanAndRangeColors)
  {
    portal.Set(index++, std::numeric_limits<T>::lowest()); //below
  }

  //add number of samples which doesn't account for the end
  T value = start;
  for (viskores::Int32 i = 0; i < numSamples; ++i, ++index, value += inc)
  {
    portal.Set(index, value);
  }
  portal.Set(index++, end);

  if (appendNanAndRangeColors)
  {
    //push back the last value again so that when lookups near the max value
    //occur we don't need to clamp as if they are out-of-bounds they will
    //land in the extra 'end' color
    portal.Set(index++, end);
    portal.Set(index++, std::numeric_limits<T>::max()); //above
    portal.Set(index++, viskores::Nan<T>());            //nan
  }

  return handle;
}

template <typename OutputColors>
inline bool sampleColorTable(const viskores::cont::ColorTable* self,
                             viskores::Int32 numSamples,
                             OutputColors& colors,
                             viskores::Float64 tolerance,
                             bool appendNanAndRangeColors)
{
  viskores::Range r = self->GetRange();
  //We want the samples to start at Min, and end at Max so that means
  //we want actually to interpolate numSample - 1 values. For example
  //for range 0 - 1, we want the values 0, 0.5, and 1.
  const viskores::Float64 d_samples = static_cast<viskores::Float64>(numSamples - 1);
  const viskores::Float64 d_delta = r.Length() / d_samples;

  if (r.Min > static_cast<viskores::Float64>(std::numeric_limits<viskores::Float32>::lowest()) &&
      r.Max < static_cast<viskores::Float64>(std::numeric_limits<viskores::Float32>::max()))
  {
    //we can try and see if Float32 space has enough resolution
    const viskores::Float32 f_samples = static_cast<viskores::Float32>(numSamples - 1);
    const viskores::Float32 f_start = static_cast<viskores::Float32>(r.Min);
    const viskores::Float32 f_delta = static_cast<viskores::Float32>(r.Length()) / f_samples;
    const viskores::Float32 f_end = f_start + (f_delta * f_samples);

    if (viskores::Abs(static_cast<viskores::Float64>(f_end) - r.Max) <= tolerance &&
        viskores::Abs(static_cast<viskores::Float64>(f_delta) - d_delta) <= tolerance)
    {
      auto handle =
        buildSampleHandle((numSamples - 1), f_start, f_end, f_delta, appendNanAndRangeColors);
      return viskores::cont::ColorTableMap(handle, *self, colors);
    }
  }

  //otherwise we need to use Float64 space
  auto handle = buildSampleHandle((numSamples - 1), r.Min, r.Max, d_delta, appendNanAndRangeColors);
  return viskores::cont::ColorTableMap(handle, *self, colors);
}
} // anonymous namespace

namespace viskores
{
namespace cont
{

namespace detail
{

struct ColorTableInternals
{
  std::string Name;

  viskores::ColorSpace Space = viskores::ColorSpace::Lab;
  viskores::Range TableRange = { 1.0, 0.0 };

  viskores::Vec3f_32 NaNColor = { 0.5f, 0.0f, 0.0f };
  viskores::Vec3f_32 BelowRangeColor = { 0.0f, 0.0f, 0.0f };
  viskores::Vec3f_32 AboveRangeColor = { 0.0f, 0.0f, 0.0f };

  bool UseClamping = true;

  std::vector<viskores::Float64> ColorNodePos;
  std::vector<viskores::Vec3f_32> ColorRGB;

  std::vector<viskores::Float64> OpacityNodePos;
  std::vector<viskores::Float32> OpacityAlpha;
  std::vector<viskores::Vec2f_32> OpacityMidSharp;

  viskores::cont::ArrayHandle<viskores::Float64> ColorPosHandle;
  viskores::cont::ArrayHandle<viskores::Vec3f_32> ColorRGBHandle;
  viskores::cont::ArrayHandle<viskores::Float64> OpacityPosHandle;
  viskores::cont::ArrayHandle<viskores::Float32> OpacityAlphaHandle;
  viskores::cont::ArrayHandle<viskores::Vec2f_32> OpacityMidSharpHandle;
  bool ColorArraysChanged = true;
  bool OpacityArraysChanged = true;

  viskores::Id ModifiedCount = 1;
  void Modified() { ++this->ModifiedCount; }

  void RecalculateRange()
  {
    viskores::Range r;
    if (this->ColorNodePos.size() > 0)
    {
      r.Include(this->ColorNodePos.front());
      r.Include(this->ColorNodePos.back());
    }

    if (this->OpacityNodePos.size() > 0)
    {
      r.Include(this->OpacityNodePos.front());
      r.Include(this->OpacityNodePos.back());
    }

    this->TableRange = r;
  }
};

} // namespace detail

namespace internal
{
std::set<std::string> GetPresetNames();
bool LoadColorTablePreset(viskores::cont::ColorTable::Preset preset,
                          viskores::cont::ColorTable& table);
bool LoadColorTablePreset(std::string name, viskores::cont::ColorTable& table);
} // namespace internal

//----------------------------------------------------------------------------
ColorTable::ColorTable(viskores::cont::ColorTable::Preset preset)
  : Internals(std::make_shared<detail::ColorTableInternals>())
{
  const bool loaded = this->LoadPreset(preset);
  if (!loaded)
  { //if we failed to load the requested color table, call SetColorSpace
    //so that the internal host side cache is constructed and we leave
    //the constructor in a valid state. We use LAB as it is the default
    //when the no parameter constructor is called
    this->SetColorSpace(viskores::ColorSpace::Lab);
  }
  this->AddSegmentAlpha(
    this->Internals->TableRange.Min, 1.0f, this->Internals->TableRange.Max, 1.0f);
}

//----------------------------------------------------------------------------
ColorTable::ColorTable(const std::string& name)
  : Internals(std::make_shared<detail::ColorTableInternals>())
{
  const bool loaded = this->LoadPreset(name);
  if (!loaded)
  { //if we failed to load the requested color table, call SetColorSpace
    //so that the internal host side cache is constructed and we leave
    //the constructor in a valid state. We use LAB as it is the default
    //when the no parameter constructor is called
    this->SetColorSpace(viskores::ColorSpace::Lab);
  }
  this->AddSegmentAlpha(
    this->Internals->TableRange.Min, 1.0f, this->Internals->TableRange.Max, 1.0f);
}

//----------------------------------------------------------------------------
ColorTable::ColorTable(viskores::ColorSpace space)
  : Internals(std::make_shared<detail::ColorTableInternals>())
{
  this->SetColorSpace(space);
}

//----------------------------------------------------------------------------
ColorTable::ColorTable(const viskores::Range& range,
                       const viskores::Vec3f_32& rgb1,
                       const viskores::Vec3f_32& rgb2,
                       viskores::ColorSpace space)
  : Internals(std::make_shared<detail::ColorTableInternals>())
{
  this->AddSegment(range.Min, rgb1, range.Max, rgb2);
  this->AddSegmentAlpha(range.Min, 1.0f, range.Max, 1.0f);
  this->SetColorSpace(space);
}

//----------------------------------------------------------------------------
ColorTable::ColorTable(const viskores::Range& range,
                       const viskores::Vec4f_32& rgba1,
                       const viskores::Vec4f_32& rgba2,
                       viskores::ColorSpace space)
  : Internals(std::make_shared<detail::ColorTableInternals>())
{
  viskores::Vec3f_32 rgb1(rgba1[0], rgba1[1], rgba1[2]);
  viskores::Vec3f_32 rgb2(rgba2[0], rgba2[1], rgba2[2]);
  this->AddSegment(range.Min, rgb1, range.Max, rgb2);
  this->AddSegmentAlpha(range.Min, rgba1[3], range.Max, rgba2[3]);
  this->SetColorSpace(space);
}

//----------------------------------------------------------------------------
ColorTable::ColorTable(const std::string& name,
                       viskores::ColorSpace colorSpace,
                       const viskores::Vec3f_64& nanColor,
                       const std::vector<viskores::Float64>& rgbPoints,
                       const std::vector<viskores::Float64>& alphaPoints)
  : Internals(std::make_shared<detail::ColorTableInternals>())
{
  this->SetName(name);
  this->SetColorSpace(colorSpace);
  this->SetNaNColor(nanColor);
  this->FillColorTableFromDataPointer(static_cast<viskores::Int32>(rgbPoints.size()),
                                      rgbPoints.data());
  this->FillOpacityTableFromDataPointer(static_cast<viskores::Int32>(alphaPoints.size()),
                                        alphaPoints.data());
}

//----------------------------------------------------------------------------
ColorTable::~ColorTable() {}

//----------------------------------------------------------------------------
const std::string& ColorTable::GetName() const
{
  return this->Internals->Name;
}

//----------------------------------------------------------------------------
void ColorTable::SetName(const std::string& name)
{
  this->Internals->Name = name;
}

//----------------------------------------------------------------------------
bool ColorTable::LoadPreset(viskores::cont::ColorTable::Preset preset)
{
  return internal::LoadColorTablePreset(preset, *this);
}

//----------------------------------------------------------------------------
std::set<std::string> ColorTable::GetPresets()
{
  return internal::GetPresetNames();
}

//----------------------------------------------------------------------------
bool ColorTable::LoadPreset(const std::string& name)
{
  return internal::LoadColorTablePreset(name, *this);
}

//----------------------------------------------------------------------------
ColorTable ColorTable::MakeDeepCopy()
{
  ColorTable dcopy(this->Internals->Space);

  dcopy.Internals->TableRange = this->Internals->TableRange;

  dcopy.Internals->NaNColor = this->Internals->NaNColor;
  dcopy.Internals->BelowRangeColor = this->Internals->BelowRangeColor;
  dcopy.Internals->AboveRangeColor = this->Internals->AboveRangeColor;

  dcopy.Internals->UseClamping = this->Internals->UseClamping;

  dcopy.Internals->ColorNodePos = this->Internals->ColorNodePos;
  dcopy.Internals->ColorRGB = this->Internals->ColorRGB;

  dcopy.Internals->OpacityNodePos = this->Internals->OpacityNodePos;
  dcopy.Internals->OpacityAlpha = this->Internals->OpacityAlpha;
  dcopy.Internals->OpacityMidSharp = this->Internals->OpacityMidSharp;
  return dcopy;
}

//----------------------------------------------------------------------------
viskores::ColorSpace ColorTable::GetColorSpace() const
{
  return this->Internals->Space;
}

//----------------------------------------------------------------------------
void ColorTable::SetColorSpace(viskores::ColorSpace space)
{
  this->Internals->Space = space;
  this->Internals->Modified();
}

//----------------------------------------------------------------------------
void ColorTable::SetClamping(bool state)
{
  this->Internals->UseClamping = state;
  this->Internals->Modified();
}

//----------------------------------------------------------------------------
bool ColorTable::GetClamping() const
{
  return this->Internals->UseClamping;
}

//----------------------------------------------------------------------------
void ColorTable::SetBelowRangeColor(const viskores::Vec3f_32& c)
{
  this->Internals->BelowRangeColor = c;
  this->Internals->Modified();
}

//----------------------------------------------------------------------------
const viskores::Vec3f_32& ColorTable::GetBelowRangeColor() const
{
  return this->Internals->BelowRangeColor;
}

//----------------------------------------------------------------------------
void ColorTable::SetAboveRangeColor(const viskores::Vec3f_32& c)
{
  this->Internals->AboveRangeColor = c;
  this->Internals->Modified();
}

//----------------------------------------------------------------------------
const viskores::Vec3f_32& ColorTable::GetAboveRangeColor() const
{
  return this->Internals->AboveRangeColor;
}

//----------------------------------------------------------------------------
void ColorTable::SetNaNColor(const viskores::Vec3f_32& c)
{
  this->Internals->NaNColor = c;
  this->Internals->Modified();
}

//----------------------------------------------------------------------------
const viskores::Vec3f_32& ColorTable::GetNaNColor() const
{
  return this->Internals->NaNColor;
}

//----------------------------------------------------------------------------
void ColorTable::Clear()
{
  this->ClearColors();
  this->ClearAlpha();
}

//---------------------------------------------------------------------------
void ColorTable::ClearColors()
{
  this->Internals->ColorNodePos.clear();
  this->Internals->ColorRGB.clear();
  this->Internals->ColorArraysChanged = true;
  this->Internals->Modified();
}

//---------------------------------------------------------------------------
void ColorTable::ClearAlpha()
{
  this->Internals->OpacityNodePos.clear();
  this->Internals->OpacityAlpha.clear();
  this->Internals->OpacityMidSharp.clear();
  this->Internals->OpacityArraysChanged = true;
  this->Internals->Modified();
}

//---------------------------------------------------------------------------
void ColorTable::ReverseColors()
{
  std::reverse(this->Internals->ColorRGB.begin(), this->Internals->ColorRGB.end());
  this->Internals->ColorArraysChanged = true;
  this->Internals->Modified();
}

//---------------------------------------------------------------------------
void ColorTable::ReverseAlpha()
{
  std::reverse(this->Internals->OpacityAlpha.begin(), this->Internals->OpacityAlpha.end());
  //To keep the shape correct the mid and sharp values of the last node are not included in the reversal
  std::reverse(this->Internals->OpacityMidSharp.begin(),
               this->Internals->OpacityMidSharp.end() - 1);
  this->Internals->OpacityArraysChanged = true;
  this->Internals->Modified();
}

//---------------------------------------------------------------------------
const viskores::Range& ColorTable::GetRange() const
{
  return this->Internals->TableRange;
}

//---------------------------------------------------------------------------
void ColorTable::RescaleToRange(const viskores::Range& r)
{
  if (r == this->GetRange())
  {
    return;
  }
  //make sure range has space.
  auto newRange = adjustRange(r);

  //slam control points down to 0.0 - 1.0, and than rescale to new range
  const viskores::Float64 minv = this->GetRange().Min;
  const viskores::Float64 oldScale = this->GetRange().Length();
  const viskores::Float64 newScale = newRange.Length();
  VISKORES_ASSERT(oldScale > 0);
  VISKORES_ASSERT(newScale > 0);
  for (auto i = this->Internals->ColorNodePos.begin(); i != this->Internals->ColorNodePos.end();
       ++i)
  {
    const auto t = (*i - minv) / oldScale;
    *i = (t * newScale) + newRange.Min;
  }
  for (auto i = this->Internals->OpacityNodePos.begin(); i != this->Internals->OpacityNodePos.end();
       ++i)
  {
    const auto t = (*i - minv) / oldScale;
    *i = (t * newScale) + newRange.Min;
  }

  this->Internals->ColorArraysChanged = true;
  this->Internals->OpacityArraysChanged = true;
  this->Internals->TableRange = newRange;
  this->Internals->Modified();
}

//---------------------------------------------------------------------------
viskores::Int32 ColorTable::AddPoint(viskores::Float64 x, const viskores::Vec3f_32& rgb)
{
  if (outside_range(rgb))
  {
    return -1;
  }

  std::size_t index = 0;
  if (this->Internals->ColorNodePos.size() == 0 || this->Internals->ColorNodePos.back() < x)
  {
    this->Internals->ColorNodePos.emplace_back(x);
    this->Internals->ColorRGB.emplace_back(rgb);
    index = this->Internals->ColorNodePos.size();
  }
  else
  {
    auto begin = this->Internals->ColorNodePos.begin();
    auto pos = std::lower_bound(begin, this->Internals->ColorNodePos.end(), x);
    index = static_cast<std::size_t>(std::distance(begin, pos));

    if (*pos == x)
    {
      this->Internals->ColorRGB[index] = rgb;
    }
    else
    {
      this->Internals->ColorRGB.emplace(
        this->Internals->ColorRGB.begin() + std::distance(begin, pos), rgb);
      this->Internals->ColorNodePos.emplace(pos, x);
    }
  }
  this->Internals->TableRange.Include(x); //update range to include x
  this->Internals->ColorArraysChanged = true;
  this->Internals->Modified();
  return static_cast<viskores::Int32>(index);
}

//---------------------------------------------------------------------------
viskores::Int32 ColorTable::AddPointHSV(viskores::Float64 x, const viskores::Vec3f_32& hsv)
{
  return this->AddPoint(x, hsvTorgb(hsv));
}

//---------------------------------------------------------------------------
viskores::Int32 ColorTable::AddSegment(viskores::Float64 x1,
                                       const viskores::Vec3f_32& rgb1,
                                       viskores::Float64 x2,
                                       const viskores::Vec3f_32& rgb2)
{
  if (outside_range(rgb1, rgb2))
  {
    return -1;
  }
  if (this->Internals->ColorNodePos.size() > 0)
  {
    //Todo:
    // - This could be optimized so we do 2 less lower_bound calls when
    // the table already exists

    //When we add a segment we remove all points that are inside the line

    auto nodeBegin = this->Internals->ColorNodePos.begin();
    auto nodeEnd = this->Internals->ColorNodePos.end();

    auto rgbBegin = this->Internals->ColorRGB.begin();

    auto nodeStart = std::lower_bound(nodeBegin, nodeEnd, x1);
    auto nodeStop = std::lower_bound(nodeBegin, nodeEnd, x2);

    auto rgbStart = rgbBegin + std::distance(nodeBegin, nodeStart);
    auto rgbStop = rgbBegin + std::distance(nodeBegin, nodeStop);

    //erase is exclusive so if end->x == x2 it will be kept around, and
    //than we will update it in AddPoint
    this->Internals->ColorNodePos.erase(nodeStart, nodeStop);
    this->Internals->ColorRGB.erase(rgbStart, rgbStop);
  }
  viskores::Int32 pos = this->AddPoint(x1, rgb1);
  this->AddPoint(x2, rgb2);
  return pos;
}

//---------------------------------------------------------------------------
viskores::Int32 ColorTable::AddSegmentHSV(viskores::Float64 x1,
                                          const viskores::Vec3f_32& hsv1,
                                          viskores::Float64 x2,
                                          const viskores::Vec3f_32& hsv2)
{
  return this->AddSegment(x1, hsvTorgb(hsv1), x2, hsvTorgb(hsv2));
}

//---------------------------------------------------------------------------
bool ColorTable::GetPoint(viskores::Int32 index, viskores::Vec4f_64& data) const
{
  std::size_t i = static_cast<std::size_t>(index);
  const std::size_t size = this->Internals->ColorNodePos.size();
  if (index < 0 || i >= size)
  {
    return false;
  }

  const auto& pos = this->Internals->ColorNodePos[i];
  const auto& rgb = this->Internals->ColorRGB[i];

  data[0] = pos;
  data[1] = rgb[0];
  data[2] = rgb[1];
  data[3] = rgb[2];
  return true;
}

//---------------------------------------------------------------------------
viskores::Int32 ColorTable::UpdatePoint(viskores::Int32 index, const viskores::Vec4f_64& data)
{
  //skip data[0] as we don't care about position
  if (outside_range(data[1], data[2], data[3]))
  {
    return -1;
  }

  std::size_t i = static_cast<std::size_t>(index);
  const std::size_t size = this->Internals->ColorNodePos.size();
  if (index < 0 || i >= size)
  {
    return -1;
  }

  //When updating the first question is has the relative position of the point changed?
  //If it hasn't we can quickly just update the RGB value
  auto oldPos = this->Internals->ColorNodePos.begin() + index;
  auto newPos = std::lower_bound(
    this->Internals->ColorNodePos.begin(), this->Internals->ColorNodePos.end(), data[0]);
  if (oldPos == newPos)
  { //node's relative location hasn't changed
    this->Internals->ColorArraysChanged = true;
    auto& rgb = this->Internals->ColorRGB[i];
    *newPos = data[0];
    rgb[0] = static_cast<viskores::Float32>(data[1]);
    rgb[1] = static_cast<viskores::Float32>(data[2]);
    rgb[2] = static_cast<viskores::Float32>(data[3]);
    this->Internals->Modified();
    return index;
  }
  else
  { //remove the point, and add the new values as the relative location is different
    this->RemovePoint(index);
    viskores::Vec3f_32 newrgb(static_cast<viskores::Float32>(data[1]),
                              static_cast<viskores::Float32>(data[2]),
                              static_cast<viskores::Float32>(data[3]));
    return this->AddPoint(data[0], newrgb);
  }
}

//---------------------------------------------------------------------------
bool ColorTable::RemovePoint(viskores::Float64 x)
{
  auto begin = this->Internals->ColorNodePos.begin();
  auto pos = std::lower_bound(begin, this->Internals->ColorNodePos.end(), x);
  return this->RemovePoint(static_cast<viskores::Int32>(std::distance(begin, pos)));
}

//---------------------------------------------------------------------------
bool ColorTable::RemovePoint(viskores::Int32 index)
{
  std::size_t i = static_cast<std::size_t>(index);
  const std::size_t size = this->Internals->ColorNodePos.size();
  if (index < 0 || i >= size)
  {
    return false;
  }

  this->Internals->ColorNodePos.erase(this->Internals->ColorNodePos.begin() + index);
  this->Internals->ColorRGB.erase(this->Internals->ColorRGB.begin() + index);
  this->Internals->ColorArraysChanged = true;
  this->Internals->RecalculateRange();
  this->Internals->Modified();
  return true;
}

//---------------------------------------------------------------------------
viskores::Int32 ColorTable::GetNumberOfPoints() const
{
  return static_cast<viskores::Int32>(this->Internals->ColorNodePos.size());
}

//---------------------------------------------------------------------------
viskores::Int32 ColorTable::AddPointAlpha(viskores::Float64 x,
                                          viskores::Float32 alpha,
                                          viskores::Float32 midpoint,
                                          viskores::Float32 sharpness)
{
  if (outside_range(alpha, midpoint, sharpness))
  {
    return -1;
  }

  const viskores::Vec2f_32 midsharp(midpoint, sharpness);
  std::size_t index = 0;
  if (this->Internals->OpacityNodePos.size() == 0 || this->Internals->OpacityNodePos.back() < x)
  {
    this->Internals->OpacityNodePos.emplace_back(x);
    this->Internals->OpacityAlpha.emplace_back(alpha);
    this->Internals->OpacityMidSharp.emplace_back(midsharp);
    index = this->Internals->OpacityNodePos.size();
  }
  else
  {
    auto begin = this->Internals->OpacityNodePos.begin();
    auto pos = std::lower_bound(begin, this->Internals->OpacityNodePos.end(), x);
    index = static_cast<std::size_t>(std::distance(begin, pos));
    if (*pos == x)
    {
      this->Internals->OpacityAlpha[index] = alpha;
      this->Internals->OpacityMidSharp[index] = midsharp;
    }
    else
    {
      this->Internals->OpacityAlpha.emplace(
        this->Internals->OpacityAlpha.begin() + std::distance(begin, pos), alpha);
      this->Internals->OpacityMidSharp.emplace(
        this->Internals->OpacityMidSharp.begin() + std::distance(begin, pos), midsharp);
      this->Internals->OpacityNodePos.emplace(pos, x);
    }
  }
  this->Internals->OpacityArraysChanged = true;
  this->Internals->TableRange.Include(x); //update range to include x
  this->Internals->Modified();
  return static_cast<viskores::Int32>(index);
}

//---------------------------------------------------------------------------
viskores::Int32 ColorTable::AddSegmentAlpha(viskores::Float64 x1,
                                            viskores::Float32 alpha1,
                                            viskores::Float64 x2,
                                            viskores::Float32 alpha2,
                                            const viskores::Vec2f_32& mid_sharp1,
                                            const viskores::Vec2f_32& mid_sharp2)
{
  if (outside_range(alpha1, alpha2, mid_sharp1, mid_sharp2))
  {
    return -1;
  }

  if (this->Internals->OpacityNodePos.size() > 0)
  {
    //Todo:
    // - This could be optimized so we do 2 less lower_bound calls when
    // the table already exists

    //When we add a segment we remove all points that are inside the line

    auto nodeBegin = this->Internals->OpacityNodePos.begin();
    auto nodeEnd = this->Internals->OpacityNodePos.end();

    auto alphaBegin = this->Internals->OpacityAlpha.begin();
    auto midBegin = this->Internals->OpacityMidSharp.begin();

    auto nodeStart = std::lower_bound(nodeBegin, nodeEnd, x1);
    auto nodeStop = std::lower_bound(nodeBegin, nodeEnd, x2);

    auto alphaStart = alphaBegin + std::distance(nodeBegin, nodeStart);
    auto alphaStop = alphaBegin + std::distance(nodeBegin, nodeStop);
    auto midStart = midBegin + std::distance(nodeBegin, nodeStart);
    auto midStop = midBegin + std::distance(nodeBegin, nodeStop);

    //erase is exclusive so if end->x == x2 it will be kept around, and
    //than we will update it in AddPoint
    this->Internals->OpacityNodePos.erase(nodeStart, nodeStop);
    this->Internals->OpacityAlpha.erase(alphaStart, alphaStop);
    this->Internals->OpacityMidSharp.erase(midStart, midStop);
  }

  viskores::Int32 pos = this->AddPointAlpha(x1, alpha1, mid_sharp1[0], mid_sharp1[1]);
  this->AddPointAlpha(x2, alpha2, mid_sharp2[0], mid_sharp2[1]);
  return pos;
}

//---------------------------------------------------------------------------
bool ColorTable::GetPointAlpha(viskores::Int32 index, viskores::Vec4f_64& data) const
{
  std::size_t i = static_cast<std::size_t>(index);
  const std::size_t size = this->Internals->OpacityNodePos.size();
  if (index < 0 || i >= size)
  {
    return false;
  }

  const auto& pos = this->Internals->OpacityNodePos[i];
  const auto& alpha = this->Internals->OpacityAlpha[i];
  const auto& midsharp = this->Internals->OpacityMidSharp[i];

  data[0] = pos;
  data[1] = alpha;
  data[2] = midsharp[0];
  data[3] = midsharp[1];
  return true;
}

//---------------------------------------------------------------------------
viskores::Int32 ColorTable::UpdatePointAlpha(viskores::Int32 index, const viskores::Vec4f_64& data)
{
  //skip data[0] as we don't care about position
  if (outside_range(data[1], data[2], data[3]))
  {
    return -1;
  }

  std::size_t i = static_cast<std::size_t>(index);
  const std::size_t size = this->Internals->OpacityNodePos.size();
  if (index < 0 || i >= size)
  {
    return -1;
  }
  //When updating the first question is has the relative position of the point changed?
  //If it hasn't we can quickly just update the RGB value
  auto oldPos = this->Internals->OpacityNodePos.begin() + index;
  auto newPos = std::lower_bound(
    this->Internals->OpacityNodePos.begin(), this->Internals->OpacityNodePos.end(), data[0]);
  if (oldPos == newPos)
  { //node's relative location hasn't changed
    this->Internals->OpacityArraysChanged = true;
    auto& alpha = this->Internals->OpacityAlpha[i];
    auto& midsharp = this->Internals->OpacityMidSharp[i];
    *newPos = data[0];
    alpha = static_cast<viskores::Float32>(data[1]);
    midsharp[0] = static_cast<viskores::Float32>(data[2]);
    midsharp[1] = static_cast<viskores::Float32>(data[3]);
    this->Internals->Modified();
    return index;
  }
  else
  { //remove the point, and add the new values as the relative location is different
    this->RemovePointAlpha(index);
    return this->AddPointAlpha(data[0],
                               static_cast<viskores::Float32>(data[1]),
                               static_cast<viskores::Float32>(data[2]),
                               static_cast<viskores::Float32>(data[3]));
  }
}

//---------------------------------------------------------------------------
bool ColorTable::RemovePointAlpha(viskores::Float64 x)
{
  auto begin = this->Internals->OpacityNodePos.begin();
  auto pos = std::lower_bound(begin, this->Internals->OpacityNodePos.end(), x);
  return this->RemovePointAlpha(static_cast<viskores::Int32>(std::distance(begin, pos)));
}

//---------------------------------------------------------------------------
bool ColorTable::RemovePointAlpha(viskores::Int32 index)
{
  std::size_t i = static_cast<std::size_t>(index);
  const std::size_t size = this->Internals->OpacityNodePos.size();
  if (index < 0 || i >= size)
  {
    return false;
  }

  this->Internals->OpacityNodePos.erase(this->Internals->OpacityNodePos.begin() + index);
  this->Internals->OpacityAlpha.erase(this->Internals->OpacityAlpha.begin() + index);
  this->Internals->OpacityMidSharp.erase(this->Internals->OpacityMidSharp.begin() + index);
  this->Internals->OpacityArraysChanged = true;
  this->Internals->RecalculateRange();
  this->Internals->Modified();
  return true;
}

//---------------------------------------------------------------------------
viskores::Int32 ColorTable::GetNumberOfPointsAlpha() const
{
  return static_cast<viskores::Int32>(this->Internals->OpacityNodePos.size());
}

//---------------------------------------------------------------------------
bool ColorTable::FillColorTableFromDataPointer(viskores::Int32 n, const viskores::Float64* ptr)
{
  if (n <= 0 || ptr == nullptr)
  {
    return false;
  }
  this->ClearColors();

  std::size_t size = static_cast<std::size_t>(n / 4);
  this->Internals->ColorNodePos.reserve(size);
  this->Internals->ColorRGB.reserve(size);
  for (std::size_t i = 0; i < size; ++i)
  { //allows us to support unsorted arrays
    viskores::Vec3f_32 rgb(static_cast<viskores::Float32>(ptr[1]),
                           static_cast<viskores::Float32>(ptr[2]),
                           static_cast<viskores::Float32>(ptr[3]));
    this->AddPoint(ptr[0], rgb);
    ptr += 4;
  }
  this->Internals->ColorArraysChanged = true;
  this->Internals->Modified();

  return true;
}

//---------------------------------------------------------------------------
bool ColorTable::FillColorTableFromDataPointer(viskores::Int32 n, const viskores::Float32* ptr)
{
  if (n <= 0 || ptr == nullptr)
  {
    return false;
  }
  this->ClearColors();

  std::size_t size = static_cast<std::size_t>(n / 4);
  this->Internals->ColorNodePos.reserve(size);
  this->Internals->ColorRGB.reserve(size);
  for (std::size_t i = 0; i < size; ++i)
  { //allows us to support unsorted arrays
    viskores::Vec3f_32 rgb(ptr[1], ptr[2], ptr[3]);
    this->AddPoint(ptr[0], rgb);
    ptr += 4;
  }
  this->Internals->ColorArraysChanged = true;
  this->Internals->Modified();
  return true;
}

//---------------------------------------------------------------------------
bool ColorTable::FillOpacityTableFromDataPointer(viskores::Int32 n, const viskores::Float64* ptr)
{
  if (n <= 0 || ptr == nullptr)
  {
    return false;
  }
  this->ClearAlpha();

  std::size_t size = static_cast<std::size_t>(n / 4);
  this->Internals->OpacityNodePos.reserve(size);
  this->Internals->OpacityAlpha.reserve(size);
  this->Internals->OpacityMidSharp.reserve(size);
  for (std::size_t i = 0; i < size; ++i)
  { //allows us to support unsorted arrays
    this->AddPointAlpha(ptr[0],
                        static_cast<viskores::Float32>(ptr[1]),
                        static_cast<viskores::Float32>(ptr[2]),
                        static_cast<viskores::Float32>(ptr[3]));
    ptr += 4;
  }

  this->Internals->OpacityArraysChanged = true;
  this->Internals->Modified();
  return true;
}

//---------------------------------------------------------------------------
bool ColorTable::FillOpacityTableFromDataPointer(viskores::Int32 n, const viskores::Float32* ptr)
{
  if (n <= 0 || ptr == nullptr)
  {
    return false;
  }
  this->ClearAlpha();


  std::size_t size = static_cast<std::size_t>(n / 4);
  this->Internals->OpacityNodePos.reserve(size);
  this->Internals->OpacityAlpha.reserve(size);
  this->Internals->OpacityMidSharp.reserve(size);
  for (std::size_t i = 0; i < size; ++i)
  { //allows us to support unsorted arrays
    this->AddPointAlpha(ptr[0], ptr[1], ptr[2], ptr[3]);
    ptr += 4;
  }
  this->Internals->OpacityArraysChanged = true;
  this->Internals->Modified();
  return true;
}

//---------------------------------------------------------------------------
bool ColorTable::Sample(viskores::Int32 numSamples,
                        viskores::cont::ColorTableSamplesRGBA& samples,
                        viskores::Float64 tolerance) const
{
  if (numSamples <= 1)
  {
    return false;
  }
  samples.NumberOfSamples = numSamples;
  samples.SampleRange = this->GetRange();
  return sampleColorTable(this, numSamples, samples.Samples, tolerance, true);
}

//---------------------------------------------------------------------------
bool ColorTable::Sample(viskores::Int32 numSamples,
                        viskores::cont::ColorTableSamplesRGB& samples,
                        viskores::Float64 tolerance) const
{
  if (numSamples <= 1)
  {
    return false;
  }
  samples.NumberOfSamples = numSamples;
  samples.SampleRange = this->GetRange();
  return sampleColorTable(this, numSamples, samples.Samples, tolerance, true);
}

//---------------------------------------------------------------------------
bool ColorTable::Sample(viskores::Int32 numSamples,
                        viskores::cont::ArrayHandle<viskores::Vec4ui_8>& colors,
                        viskores::Float64 tolerance) const
{
  if (numSamples <= 1)
  {
    return false;
  }
  return sampleColorTable(this, numSamples, colors, tolerance, false);
}

//---------------------------------------------------------------------------
bool ColorTable::Sample(viskores::Int32 numSamples,
                        viskores::cont::ArrayHandle<viskores::Vec3ui_8>& colors,
                        viskores::Float64 tolerance) const
{
  if (numSamples <= 1)
  {
    return false;
  }
  return sampleColorTable(this, numSamples, colors, tolerance, false);
}

//----------------------------------------------------------------------------
void ColorTable::UpdateArrayHandles() const
{
  //Only rebuild the array handles that have changed since the last time
  //we have modified or color / opacity information

  if (this->Internals->ColorArraysChanged)
  {
    this->Internals->ColorPosHandle =
      viskores::cont::make_ArrayHandle(this->Internals->ColorNodePos, viskores::CopyFlag::Off);
    this->Internals->ColorRGBHandle =
      viskores::cont::make_ArrayHandle(this->Internals->ColorRGB, viskores::CopyFlag::Off);
    this->Internals->ColorArraysChanged = false;
  }

  if (this->Internals->OpacityArraysChanged)
  {
    this->Internals->OpacityPosHandle =
      viskores::cont::make_ArrayHandle(this->Internals->OpacityNodePos, viskores::CopyFlag::Off);
    this->Internals->OpacityAlphaHandle =
      viskores::cont::make_ArrayHandle(this->Internals->OpacityAlpha, viskores::CopyFlag::Off);
    this->Internals->OpacityMidSharpHandle =
      viskores::cont::make_ArrayHandle(this->Internals->OpacityMidSharp, viskores::CopyFlag::Off);
    this->Internals->OpacityArraysChanged = false;
  }
}

//---------------------------------------------------------------------------
viskores::exec::ColorTable ColorTable::PrepareForExecution(viskores::cont::DeviceAdapterId device,
                                                           viskores::cont::Token& token) const
{
  this->UpdateArrayHandles();

  viskores::exec::ColorTable execTable;

  execTable.Space = this->Internals->Space;
  execTable.NaNColor = this->Internals->NaNColor;
  execTable.BelowRangeColor = this->Internals->BelowRangeColor;
  execTable.AboveRangeColor = this->Internals->AboveRangeColor;
  execTable.UseClamping = this->Internals->UseClamping;

  VISKORES_ASSERT(static_cast<viskores::Id>(this->Internals->ColorNodePos.size()) ==
                  this->Internals->ColorPosHandle.GetNumberOfValues());
  execTable.ColorSize =
    static_cast<viskores::Int32>(this->Internals->ColorPosHandle.GetNumberOfValues());
  VISKORES_ASSERT(static_cast<viskores::Id>(execTable.ColorSize) ==
                  this->Internals->ColorRGBHandle.GetNumberOfValues());
  execTable.ColorNodes = this->Internals->ColorPosHandle.PrepareForInput(device, token).GetArray();
  execTable.RGB = this->Internals->ColorRGBHandle.PrepareForInput(device, token).GetArray();

  VISKORES_ASSERT(static_cast<viskores::Id>(this->Internals->OpacityNodePos.size()) ==
                  this->Internals->OpacityPosHandle.GetNumberOfValues());
  execTable.OpacitySize =
    static_cast<viskores::Int32>(this->Internals->OpacityPosHandle.GetNumberOfValues());
  VISKORES_ASSERT(static_cast<viskores::Id>(execTable.OpacitySize) ==
                  this->Internals->OpacityAlphaHandle.GetNumberOfValues());
  VISKORES_ASSERT(static_cast<viskores::Id>(execTable.OpacitySize) ==
                  this->Internals->OpacityMidSharpHandle.GetNumberOfValues());
  execTable.ONodes = this->Internals->OpacityPosHandle.PrepareForInput(device, token).GetArray();
  execTable.Alpha = this->Internals->OpacityAlphaHandle.PrepareForInput(device, token).GetArray();
  execTable.MidSharp =
    this->Internals->OpacityMidSharpHandle.PrepareForInput(device, token).GetArray();

  return execTable;
}

//---------------------------------------------------------------------------
viskores::Id ColorTable::GetModifiedCount() const
{
  return this->Internals->ModifiedCount;
}
}
} //namespace viskores::cont
