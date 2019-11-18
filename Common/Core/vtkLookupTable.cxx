/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLookupTable.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkLookupTable.h"

#include "vtkAbstractArray.h"
#include "vtkBitArray.h"
#include "vtkMath.h"
#include "vtkMathConfigure.h"
#include "vtkObjectFactory.h"
#include "vtkStringArray.h"
#include "vtkVariantArray.h"

#include <cassert>

const vtkIdType vtkLookupTable::REPEATED_LAST_COLOR_INDEX = 0;
const vtkIdType vtkLookupTable::BELOW_RANGE_COLOR_INDEX = 1;
const vtkIdType vtkLookupTable::ABOVE_RANGE_COLOR_INDEX = 2;
const vtkIdType vtkLookupTable::NAN_COLOR_INDEX = 3;
const vtkIdType vtkLookupTable::NUMBER_OF_SPECIAL_COLORS = NAN_COLOR_INDEX + 1;

vtkStandardNewMacro(vtkLookupTable);

// Construct with range=(0,1); and hsv ranges set up for rainbow color table
// (from red to blue).
vtkLookupTable::vtkLookupTable(int sze, int ext)
{
  this->NumberOfColors = sze;
  this->Table = vtkUnsignedCharArray::New();
  this->Table->Register(this);
  this->Table->Delete();
  this->Table->SetNumberOfComponents(4);
  this->Table->Allocate(4 * (sze + NUMBER_OF_SPECIAL_COLORS), 4 * ext);

  this->HueRange[0] = 0.0;
  this->HueRange[1] = 0.66667;

  this->SaturationRange[0] = 1.0;
  this->SaturationRange[1] = 1.0;

  this->ValueRange[0] = 1.0;
  this->ValueRange[1] = 1.0;

  this->AlphaRange[0] = 1.0;
  this->AlphaRange[1] = 1.0;
  this->Alpha = 1.0;

  this->NanColor[0] = 0.5;
  this->NanColor[1] = 0.0;
  this->NanColor[2] = 0.0;
  this->NanColor[3] = 1.0;

  this->BelowRangeColor[0] = 0.0;
  this->BelowRangeColor[1] = 0.0;
  this->BelowRangeColor[2] = 0.0;
  this->BelowRangeColor[3] = 1.0;

  this->UseBelowRangeColor = 0;

  this->AboveRangeColor[0] = 1.0;
  this->AboveRangeColor[1] = 1.0;
  this->AboveRangeColor[2] = 1.0;
  this->AboveRangeColor[3] = 1.0;

  this->UseAboveRangeColor = 0;

  this->TableRange[0] = 0.0;
  this->TableRange[1] = 1.0;

  this->Ramp = VTK_RAMP_SCURVE;
  this->Scale = VTK_SCALE_LINEAR;

  this->OpaqueFlag = 1;
}

//----------------------------------------------------------------------------
vtkLookupTable::~vtkLookupTable()
{
  this->Table->UnRegister(this);
  this->Table = nullptr;
}

//----------------------------------------------------------------------------
// Description:
// Return true if all of the values defining the mapping have an opacity
// equal to 1. Default implementation returns true.
int vtkLookupTable::IsOpaque()
{
  if (this->OpaqueFlagBuildTime < this->GetMTime())
  {
    int opaque = 1;
    if (this->NanColor[3] < 1.0)
    {
      opaque = 0;
    }
    if (this->UseBelowRangeColor && this->BelowRangeColor[3] < 1.0)
    {
      opaque = 0;
    }
    if (this->UseAboveRangeColor && this->AboveRangeColor[3] < 1.0)
    {
      opaque = 0;
    }
    vtkIdType size = this->Table->GetNumberOfTuples();
    vtkIdType i = 0;
    const unsigned char* ptr = this->Table->GetPointer(0);
    while (opaque && i < size)
    {
      opaque = ptr[3] == 255;
      ptr += 4;
      ++i;
    }
    this->OpaqueFlag = opaque;
    this->OpaqueFlagBuildTime.Modified();
  }

  return this->OpaqueFlag;
}

int vtkLookupTable::IsOpaque(vtkAbstractArray* scalars, int colorMode, int component)
{
  // use superclass logic?
  vtkDataArray* dataArray = vtkArrayDownCast<vtkDataArray>(scalars);
  if ((colorMode == VTK_COLOR_MODE_DEFAULT &&
        vtkArrayDownCast<vtkUnsignedCharArray>(dataArray) != nullptr) ||
    (colorMode == VTK_COLOR_MODE_DIRECT_SCALARS && dataArray))
  {
    return this->Superclass::IsOpaque(scalars, colorMode, component);
  }
  // otherwise look at our table
  return this->IsOpaque();
}

//----------------------------------------------------------------------------
void vtkLookupTable::SetTableRange(const double r[2])
{
  this->SetTableRange(r[0], r[1]);
}

//----------------------------------------------------------------------------
// Set the minimum/maximum scalar values for scalar mapping. Scalar values
// less than minimum range value are clamped to minimum range value.
// Scalar values greater than maximum range value are clamped to maximum
// range value.
void vtkLookupTable::SetTableRange(double rmin, double rmax)
{
  if (this->Scale == VTK_SCALE_LOG10 && ((rmin > 0 && rmax < 0) || (rmin < 0 && rmax > 0)))
  {
    vtkErrorMacro("Bad table range for log scale: [" << rmin << ", " << rmax << "]");
    return;
  }
  if (rmax < rmin)
  {
    vtkErrorMacro("Bad table range: [" << rmin << ", " << rmax << "]");
    return;
  }

  if (this->TableRange[0] == rmin && this->TableRange[1] == rmax)
  {
    return;
  }

  this->TableRange[0] = rmin;
  this->TableRange[1] = rmax;

  this->Modified();
}

//----------------------------------------------------------------------------
// Have to be careful about the range if scale is logarithmic
void vtkLookupTable::SetScale(int scale)
{
  if (this->Scale == scale)
  {
    return;
  }
  this->Scale = scale;
  this->Modified();

  double rmin = this->TableRange[0];
  double rmax = this->TableRange[1];

  if (this->Scale == VTK_SCALE_LOG10 && ((rmin > 0 && rmax < 0) || (rmin < 0 && rmax > 0)))
  {
    this->TableRange[0] = 1.0;
    this->TableRange[1] = 10.0;
    vtkErrorMacro("Bad table range for log scale: [" << rmin << ", " << rmax
                                                     << "], "
                                                        "adjusting to [1, 10]");
    return;
  }
}

//----------------------------------------------------------------------------
// Allocate a color table of specified size.
int vtkLookupTable::Allocate(int sz, int ext)
{
  this->NumberOfColors = sz;
  int a = this->Table->Allocate(4 * (this->NumberOfColors + NUMBER_OF_SPECIAL_COLORS), 4 * ext);
  this->Modified();
  return a;
}

//----------------------------------------------------------------------------
// Force the lookup table to rebuild
void vtkLookupTable::ForceBuild()
{
  vtkIdType maxIndex = this->NumberOfColors - 1;

  double hinc, sinc, vinc, ainc;
  if (maxIndex > 0)
  {
    hinc = (this->HueRange[1] - this->HueRange[0]) / maxIndex;
    sinc = (this->SaturationRange[1] - this->SaturationRange[0]) / maxIndex;
    vinc = (this->ValueRange[1] - this->ValueRange[0]) / maxIndex;
    ainc = (this->AlphaRange[1] - this->AlphaRange[0]) / maxIndex;
  }
  else
  {
    hinc = sinc = vinc = ainc = 0.0;
  }

  double rgba[4];
  for (vtkIdType i = 0; i <= maxIndex; i++)
  {
    double hue = this->HueRange[0] + i * hinc;
    double sat = this->SaturationRange[0] + i * sinc;
    double val = this->ValueRange[0] + i * vinc;
    double alpha = this->AlphaRange[0] + i * ainc;

    vtkMath::HSVToRGB(hue, sat, val, &rgba[0], &rgba[1], &rgba[2]);
    rgba[3] = alpha;

    unsigned char* c_rgba = this->Table->WritePointer(4 * i, 4);

    switch (this->Ramp)
    {
      case VTK_RAMP_SCURVE:
      {
        c_rgba[0] = static_cast<unsigned char>(
          127.5 * (1.0 + cos((1.0 - static_cast<double>(rgba[0])) * vtkMath::Pi())));
        c_rgba[1] = static_cast<unsigned char>(
          127.5 * (1.0 + cos((1.0 - static_cast<double>(rgba[1])) * vtkMath::Pi())));
        c_rgba[2] = static_cast<unsigned char>(
          127.5 * (1.0 + cos((1.0 - static_cast<double>(rgba[2])) * vtkMath::Pi())));
        c_rgba[3] = static_cast<unsigned char>(alpha * 255.0);
        /* same code, but with rounding for correctness
        c_rgba[0] = static_cast<unsigned char>
          (127.5*(1.0 + cos((1.0 - rgba[0])*vtkMath::Pi())) + 0.5);
        c_rgba[1] = static_cast<unsigned char>
          (127.5*(1.0 + cos((1.0 - rgba[1])*vtkMath::Pi())) + 0.5);
        c_rgba[2] = static_cast<unsigned char>
          (127.5*(1.0 + cos((1.0 - rgba[2])*vtkMath::Pi())) + 0.5);
        c_rgba[3] = static_cast<unsigned char>(alpha*255.0 + 0.5);
        */
      }
      break;
      case VTK_RAMP_LINEAR:
      {
        c_rgba[0] = static_cast<unsigned char>(rgba[0] * 255.0 + 0.5);
        c_rgba[1] = static_cast<unsigned char>(rgba[1] * 255.0 + 0.5);
        c_rgba[2] = static_cast<unsigned char>(rgba[2] * 255.0 + 0.5);
        c_rgba[3] = static_cast<unsigned char>(rgba[3] * 255.0 + 0.5);
      }
      break;
      case VTK_RAMP_SQRT:
      {
        c_rgba[0] = static_cast<unsigned char>(sqrt(rgba[0]) * 255.0 + 0.5);
        c_rgba[1] = static_cast<unsigned char>(sqrt(rgba[1]) * 255.0 + 0.5);
        c_rgba[2] = static_cast<unsigned char>(sqrt(rgba[2]) * 255.0 + 0.5);
        c_rgba[3] = static_cast<unsigned char>(sqrt(rgba[3]) * 255.0 + 0.5);
      }
      break;
      default:
        assert("check: impossible case." && 0); // reaching this line is a bug.
        break;
    }
  }

  this->BuildSpecialColors();

  this->BuildTime.Modified();
}

//----------------------------------------------------------------------------
// Generate lookup table from hue, saturation, value, alpha min/max values.
// Table is built from linear ramp of each value.
void vtkLookupTable::Build()
{
  vtkMTimeType mtime = this->GetMTime();

  if ((mtime > this->BuildTime && this->InsertTime <= this->BuildTime) ||
    this->Table->GetNumberOfTuples() < 1)
  {
    this->ForceBuild();
  }
  else if (mtime > this->SpecialColorsBuildTime)
  {
    this->BuildSpecialColors();
  }
}

//----------------------------------------------------------------------------
void vtkLookupTable::BuildSpecialColors()
{
  // Append 4 "special" colors (repeated last, below range, above range, NaN) to table here.
  vtkIdType numberOfColors = this->GetTable()->GetNumberOfTuples();
  this->ResizeTableForSpecialColors();
  unsigned char* table = this->GetTable()->GetPointer(0);
  unsigned char color[4];

  // Repeat the last color. This is done to improve performance later on.
  // Floating point math in vtkLinearIndexLookupMain may result in an off-by-one,
  // and having an extra copy of the last color lets us avoid a test in that very hot function.
  unsigned char* tptr = table + 4 * (numberOfColors + vtkLookupTable::REPEATED_LAST_COLOR_INDEX);
  if (numberOfColors > 0)
  {
    // Duplicate the last color in the table.
    tptr[0] = table[4 * (numberOfColors - 1) + 0];
    tptr[1] = table[4 * (numberOfColors - 1) + 1];
    tptr[2] = table[4 * (numberOfColors - 1) + 2];
    tptr[3] = table[4 * (numberOfColors - 1) + 3];
  }
  else if (this->GetUseAboveRangeColor())
  {
    vtkLookupTable::GetColorAsUnsignedChars(this->GetAboveRangeColor(), color);
    tptr[0] = color[0];
    tptr[1] = color[1];
    tptr[2] = color[2];
    tptr[3] = color[3];
  }
  else
  {
    tptr[0] = 0;
    tptr[1] = 0;
    tptr[2] = 0;
    tptr[3] = 0;
  }

  // Below range color
  tptr = table + 4 * (numberOfColors + vtkLookupTable::BELOW_RANGE_COLOR_INDEX);
  if (this->GetUseBelowRangeColor() || numberOfColors == 0)
  {
    vtkLookupTable::GetColorAsUnsignedChars(this->GetBelowRangeColor(), color);
    tptr[0] = color[0];
    tptr[1] = color[1];
    tptr[2] = color[2];
    tptr[3] = color[3];
  }
  else
  {
    // Duplicate the first color in the table.
    tptr[0] = table[0];
    tptr[1] = table[1];
    tptr[2] = table[2];
    tptr[3] = table[3];
  }

  // Above range color
  tptr = table + 4 * (numberOfColors + vtkLookupTable::ABOVE_RANGE_COLOR_INDEX);
  if (this->GetUseAboveRangeColor() || numberOfColors == 0)
  {
    vtkLookupTable::GetColorAsUnsignedChars(this->GetAboveRangeColor(), color);
    tptr[0] = color[0];
    tptr[1] = color[1];
    tptr[2] = color[2];
    tptr[3] = color[3];
  }
  else
  {
    // Duplicate the last color in the table.
    tptr[0] = table[4 * (numberOfColors - 1) + 0];
    tptr[1] = table[4 * (numberOfColors - 1) + 1];
    tptr[2] = table[4 * (numberOfColors - 1) + 2];
    tptr[3] = table[4 * (numberOfColors - 1) + 3];
  }

  // Always use NanColor
  vtkLookupTable::GetColorAsUnsignedChars(this->GetNanColor(), color);
  tptr = table + 4 * (numberOfColors + vtkLookupTable::NAN_COLOR_INDEX);
  tptr[0] = color[0];
  tptr[1] = color[1];
  tptr[2] = color[2];
  tptr[3] = color[3];

  this->SpecialColorsBuildTime.Modified();
}

//----------------------------------------------------------------------------
// get the color for a scalar value
void vtkLookupTable::GetColor(double v, double rgb[3])
{
  const unsigned char* rgb8 = this->MapValue(v);

  rgb[0] = rgb8[0] / 255.0;
  rgb[1] = rgb8[1] / 255.0;
  rgb[2] = rgb8[2] / 255.0;
}

//----------------------------------------------------------------------------
// get the opacity (alpha) for a scalar value
double vtkLookupTable::GetOpacity(double v)
{
  const unsigned char* rgb8 = this->MapValue(v);

  return rgb8[3] / 255.0;
}

namespace
{

//----------------------------------------------------------------------------
// There is a little more to this than simply taking the log10 of the
// two range values: we do conversion of negative ranges to positive
// ranges, and conversion of zero to a 'very small number'
inline void vtkLookupTableLogRange(const double range[2], double logRange[2])
{
  double rmin = range[0];
  double rmax = range[1];

  // does the range include zero?
  if ((rmin <= 0 && rmax >= 0) || (rmin >= 0 && rmax <= 0))
  {
    // clamp the smaller value to 1e-6 times the larger
    if (fabs(rmax) >= fabs(rmin))
    {
      rmin = rmax * 1e-6;
    }
    else
    {
      rmax = rmin * 1e-6;
    }

    // ensure values are not zero
    if (rmax == 0)
    {
      rmax = (rmin < 0 ? -VTK_DBL_MIN : VTK_DBL_MIN);
    }
    if (rmin == 0)
    {
      rmin = (rmax < 0 ? -VTK_DBL_MIN : VTK_DBL_MIN);
    }
  }

  if (rmax < 0) // rmin and rmax have same sign now
  {
    logRange[0] = -log10(-rmin);
    logRange[1] = -log10(-rmax);
  }
  else
  {
    logRange[0] = log10(rmin);
    logRange[1] = log10(rmax);
  }
}

//----------------------------------------------------------------------------
// Apply log to value, with appropriate constraints.
static double vtkApplyLogScaleMain(double v, const double range[2], const double logRange[2])
{
  // is the range set for negative numbers?
  if (range[0] < 0)
  {
    if (v < 0)
    {
      v = -log10(-v);
    }
    else if (range[0] > range[1])
    {
      v = logRange[0];
    }
    else
    {
      v = logRange[1];
    }
  }
  else
  {
    if (v > 0)
    {
      v = log10(v);
    }
    else if (range[0] <= range[1])
    {
      v = logRange[0];
    }
    else
    {
      v = logRange[1];
    }
  }
  return v;
}

//----------------------------------------------------------------------------
template <class T>
double vtkApplyLogScale(T v, const double range[2], const double logRange[2])
{
  return vtkApplyLogScaleMain(v, range, logRange);
}

//----------------------------------------------------------------------------
// Apply log to a float value (NaN values pass through)
inline double vtkApplyLogScale(float v, const double range[2], const double logRange[2])
{
  if (vtkMath::IsNan(v))
  {
    return v;
  }

  return vtkApplyLogScaleMain(v, range, logRange);
}

//----------------------------------------------------------------------------
// Apply log to a double value (NaN values pass through)
inline double vtkApplyLogScale(double v, const double range[2], const double logRange[2])
{
  if (vtkMath::IsNan(v))
  {
    return v;
  }

  return vtkApplyLogScaleMain(v, range, logRange);
}

//----------------------------------------------------------------------------
// Private structure for passing data between various internal functions
struct TableParameters
{
  vtkIdType NumColors;
  double Range[2];
  double Shift;
  double Scale;
};

//----------------------------------------------------------------------------
// Apply shift/scale to the scalar value v and return the index.
inline vtkIdType vtkLinearIndexLookupMain(double v, const TableParameters& p)
{
  vtkIdType index;

  // This is a very hot function.
  // Be very careful changing it, as it affects performance greatly.

  if (v < p.Range[0])
  {
    index = p.NumColors + vtkLookupTable::BELOW_RANGE_COLOR_INDEX;
  }
  else if (v > p.Range[1])
  {
    index = p.NumColors + vtkLookupTable::ABOVE_RANGE_COLOR_INDEX;
  }
  else
  {
    double dIndex = (v + p.Shift) * p.Scale;

    // When v is very close to p.Range[1], the floating point calculation giving
    // dIndex may map above the highest value in the lut (at index p.NumColors-1)
    // in the linear mapping above. This is why we keep an extra copy of the last
    // lut value, to avoid extra work in this very hot function.
    // It should never be more than 1 off, assert to be sure.
    index = static_cast<vtkIdType>(dIndex);
    assert(index >= 0 && index <= p.NumColors);
  }

  return index;
}

//----------------------------------------------------------------------------
// integer variant
template <class T>
inline vtkIdType vtkLinearLookup(T v, const TableParameters& p)
{
  // First convert from integer to double.
  double dv = static_cast<double>(v);
  return vtkLinearIndexLookupMain(dv, p);
}

//----------------------------------------------------------------------------
// double variant
inline vtkIdType vtkLinearLookup(double v, const TableParameters& p)
{
  // If Nan, use the special NaN color.
  if (vtkMath::IsNan(v))
  {
    vtkIdType maxIndex = p.NumColors + vtkLookupTable::NAN_COLOR_INDEX;
    return maxIndex;
  }

  return vtkLinearIndexLookupMain(v, p);
}

//----------------------------------------------------------------------------
// float variant
inline vtkIdType vtkLinearLookup(float v, const TableParameters& p)
{
  // Convert from float to double, then call the double variant.
  double dv = static_cast<double>(v);
  return vtkLinearLookup(dv, p);
}

//----------------------------------------------------------------------------
inline void vtkLookupShiftAndScale(
  const double range[2], double numColors, double& shift, double& scale)
{
  shift = -range[0];
  double rangeDelta = range[1] - range[0];
  if (rangeDelta < VTK_DBL_MIN * numColors)
  {
    // if the range is tiny, anything within the range will map to the bottom
    // of the color scale.
    scale = 0.0;
  }
  else
  {
    scale = numColors / rangeDelta;
  }
  assert(scale >= 0.0);
}

} // end anonymous namespace

//----------------------------------------------------------------------------
void vtkLookupTable::GetLogRange(const double range[2], double log_range[2])
{
  vtkLookupTableLogRange(range, log_range);
}

//----------------------------------------------------------------------------
double vtkLookupTable::ApplyLogScale(double v, const double range[2], const double log_range[2])
{
  return vtkApplyLogScale(v, range, log_range);
}

//----------------------------------------------------------------------------
// Given a scalar value v, return an index into the lookup table
vtkIdType vtkLookupTable::GetIndex(double v)
{
  if (this->IndexedLookup)
  {
    if (this->NumberOfColors > 0)
    {
      return this->GetAnnotatedValueIndex(v) % this->NumberOfColors;
    }
    else
    {
      // Treat as a NaN
      return -1;
    }
  }

  // Map to an index:
  //   First, check whether we have a number...
  if (vtkMath::IsNan(v))
  {
    // For backwards compatibility
    return -1;
  }

  TableParameters p;
  p.NumColors = this->NumberOfColors;

  if (this->Scale == VTK_SCALE_LOG10)
  { // handle logarithmic scale
    double logRange[2];
    vtkLookupTableLogRange(this->TableRange, logRange);
    vtkLookupShiftAndScale(logRange, p.NumColors, p.Shift, p.Scale);
    v = vtkApplyLogScale(v, this->TableRange, logRange);
    p.Range[0] = logRange[0];
    p.Range[1] = logRange[1];
  }
  else
  { // plain old linear
    vtkLookupShiftAndScale(this->TableRange, p.NumColors, p.Shift, p.Scale);
    p.Range[0] = this->TableRange[0];
    p.Range[1] = this->TableRange[1];
  }

  vtkIdType index = vtkLinearIndexLookupMain(v, p);

  // For backwards compatibility, if the index indicates an
  // out-of-range value, truncate to index range for in-range colors.
  if (index == this->NumberOfColors + BELOW_RANGE_COLOR_INDEX)
  {
    index = 0;
  }
  else if ((index == this->NumberOfColors + REPEATED_LAST_COLOR_INDEX) ||
    (index == this->NumberOfColors + ABOVE_RANGE_COLOR_INDEX))
  {
    index = this->NumberOfColors - 1;
  }

  return index;
}

//----------------------------------------------------------------------------
// Given a table, set the internal table and set the number of colors.
void vtkLookupTable::SetTable(vtkUnsignedCharArray* table)
{
  if (table != this->Table && table != nullptr)
  {
    // Check for incorrect arrays.
    if (table->GetNumberOfComponents() != this->Table->GetNumberOfComponents())
    {
      vtkErrorMacro(<< "Number of components in given table (" << table->GetNumberOfComponents()
                    << ") is incorrect, it should have " << this->Table->GetNumberOfComponents()
                    << ".");
      return;
    }
    this->Table->UnRegister(this);
    this->Table = table;
    this->Table->Register(this);
    this->NumberOfColors = this->Table->GetNumberOfTuples();
    this->BuildSpecialColors();

    // If InsertTime is not modified the array will be rebuilt.  So we
    // use the same approach that the SetTableValue function does.
    this->InsertTime.Modified();
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkLookupTable::GetColorAsUnsignedChars(const double colorIn[4], unsigned char colorOut[4])
{
  assert(colorIn && colorOut);
  if (!colorIn || !colorOut)
  {
    return;
  }

  for (int c = 0; c < 4; ++c)
  {
    double v = colorIn[c];
    if (v < 0.0)
    {
      v = 0.0;
    }
    else if (v > 1.0)
    {
      v = 1.0;
    }
    colorOut[c] = static_cast<unsigned char>(v * 255.0 + 0.5);
  }
}

//----------------------------------------------------------------------------
unsigned char* vtkLookupTable::GetNanColorAsUnsignedChars()
{
  this->GetColorAsUnsignedChars(this->GetNanColor(), this->NanColorChar);
  return this->NanColorChar;
}

//----------------------------------------------------------------------------
// Given a scalar value v, return an RGBA color value from lookup table.
const unsigned char* vtkLookupTable::MapValue(double v)
{
  vtkIdType index = this->GetIndex(v);
  if (index < 0)
  {
    return this->GetNanColorAsUnsignedChars();
  }
  else if (index == 0)
  {
    if (this->UseBelowRangeColor && v < this->TableRange[0])
    {
      this->GetColorAsUnsignedChars(this->GetBelowRangeColor(), this->RGBABytes);
      return this->RGBABytes;
    }
  }
  else if (index == this->NumberOfColors - 1)
  {
    if (this->UseAboveRangeColor && v > this->TableRange[1])
    {
      this->GetColorAsUnsignedChars(this->GetAboveRangeColor(), this->RGBABytes);
      return this->RGBABytes;
    }
  }

  return this->Table->GetPointer(0) + 4 * index;
}

namespace
{

//----------------------------------------------------------------------------
template <class T>
void vtkLookupTableMapData(vtkLookupTable* self, T* input, unsigned char* output, int length,
  int inIncr, int outFormat, TableParameters& p)
{
  int i = length;
  const double* range = self->GetTableRange();
  const unsigned char* cptr;

  // Resize the internal table to hold the special colors at the
  // end. When this function is called repeatedly with the same size
  // lookup table, memory reallocation will be done only one the first
  // call if at all.

  vtkUnsignedCharArray* lookupTable = self->GetTable();

  const unsigned char* table = lookupTable->GetPointer(0);

  double alpha = self->GetAlpha();
  if (alpha >= 1.0) // no blending required
  {
    if (self->GetScale() == VTK_SCALE_LOG10)
    {
      double val;
      double logRange[2];
      vtkLookupTableLogRange(range, logRange);
      vtkLookupShiftAndScale(logRange, p.NumColors, p.Shift, p.Scale);
      p.Range[0] = logRange[0];
      p.Range[1] = logRange[1];

      if (outFormat == VTK_RGBA)
      {
        while (--i >= 0)
        {
          val = vtkApplyLogScale(*input, range, logRange);
          vtkIdType idx = vtkLinearLookup(val, p);
          cptr = table + 4 * idx;
          memcpy(output, cptr, 4);
          input += inIncr;
          output += 4;
        }
      }
      else if (outFormat == VTK_RGB)
      {
        while (--i >= 0)
        {
          val = vtkApplyLogScale(*input, range, logRange);
          vtkIdType idx = vtkLinearLookup(val, p);
          cptr = table + 4 * idx;
          memcpy(output, cptr, 3);
          input += inIncr;
          output += 3;
        }
      }
      else if (outFormat == VTK_LUMINANCE_ALPHA)
      {
        while (--i >= 0)
        {
          val = vtkApplyLogScale(*input, range, logRange);
          vtkIdType idx = vtkLinearLookup(val, p);
          cptr = table + 4 * idx;
          output[0] =
            static_cast<unsigned char>(cptr[0] * 0.30 + cptr[1] * 0.59 + cptr[2] * 0.11 + 0.5);
          output[1] = cptr[3];
          input += inIncr;
          output += 2;
        }
      }
      else // outFormat == VTK_LUMINANCE
      {
        while (--i >= 0)
        {
          val = vtkApplyLogScale(*input, range, logRange);
          vtkIdType idx = vtkLinearLookup(val, p);
          cptr = table + 4 * idx;
          *output++ =
            static_cast<unsigned char>(cptr[0] * 0.30 + cptr[1] * 0.59 + cptr[2] * 0.11 + 0.5);
          input += inIncr;
        }
      }
    } // if log scale

    else // not log scale
    {
      vtkLookupShiftAndScale(range, p.NumColors, p.Shift, p.Scale);
      p.Range[0] = range[0];
      p.Range[1] = range[1];
      if (outFormat == VTK_RGBA)
      {
        while (--i >= 0)
        {
          vtkIdType idx = vtkLinearLookup(*input, p);
          cptr = table + 4 * idx;
          memcpy(output, cptr, 4);
          input += inIncr;
          output += 4;
        }
      }
      else if (outFormat == VTK_RGB)
      {
        while (--i >= 0)
        {
          vtkIdType idx = vtkLinearLookup(*input, p);
          cptr = table + 4 * idx;
          memcpy(output, cptr, 3);
          input += inIncr;
          output += 3;
        }
      }
      else if (outFormat == VTK_LUMINANCE_ALPHA)
      {
        while (--i >= 0)
        {
          vtkIdType idx = vtkLinearLookup(*input, p);
          cptr = table + 4 * idx;
          output[0] =
            static_cast<unsigned char>(cptr[0] * 0.30 + cptr[1] * 0.59 + cptr[2] * 0.11 + 0.5);
          output[1] = cptr[3];
          input += inIncr;
          output += 2;
        }
      }
      else // outFormat == VTK_LUMINANCE
      {
        while (--i >= 0)
        {
          vtkIdType idx = vtkLinearLookup(*input, p);
          cptr = table + 4 * idx;
          *output++ =
            static_cast<unsigned char>(cptr[0] * 0.30 + cptr[1] * 0.59 + cptr[2] * 0.11 + 0.5);
          input += inIncr;
        }
      }
    } // if not log lookup
  }   // if blending not needed

  else // blend with the specified alpha
  {
    if (self->GetScale() == VTK_SCALE_LOG10)
    {
      double val;
      double logRange[2];
      vtkLookupTableLogRange(range, logRange);
      vtkLookupShiftAndScale(logRange, p.NumColors, p.Shift, p.Scale);
      p.Range[0] = logRange[0];
      p.Range[1] = logRange[1];

      if (outFormat == VTK_RGBA)
      {
        while (--i >= 0)
        {
          val = vtkApplyLogScale(*input, range, logRange);
          vtkIdType idx = vtkLinearLookup(val, p);
          cptr = table + 4 * idx;
          output[0] = cptr[0];
          output[1] = cptr[1];
          output[2] = cptr[2];
          output[3] = static_cast<unsigned char>(cptr[3] * alpha + 0.5);
          input += inIncr;
          output += 4;
        }
      }
      else if (outFormat == VTK_RGB)
      {
        while (--i >= 0)
        {
          val = vtkApplyLogScale(*input, range, logRange);
          vtkIdType idx = vtkLinearLookup(val, p);
          cptr = table + 4 * idx;
          memcpy(output, cptr, 3);
          input += inIncr;
          output += 3;
        }
      }
      else if (outFormat == VTK_LUMINANCE_ALPHA)
      {
        while (--i >= 0)
        {
          val = vtkApplyLogScale(*input, range, logRange);
          vtkIdType idx = vtkLinearLookup(val, p);
          cptr = table + 4 * idx;
          output[0] =
            static_cast<unsigned char>(cptr[0] * 0.30 + cptr[1] * 0.59 + cptr[2] * 0.11 + 0.5);
          output[1] = static_cast<unsigned char>(alpha * cptr[3] + 0.5);
          input += inIncr;
          output += 2;
        }
      }
      else // outFormat == VTK_LUMINANCE
      {
        while (--i >= 0)
        {
          val = vtkApplyLogScale(*input, range, logRange);
          vtkIdType idx = vtkLinearLookup(val, p);
          cptr = table + 4 * idx;
          *output++ =
            static_cast<unsigned char>(cptr[0] * 0.30 + cptr[1] * 0.59 + cptr[2] * 0.11 + 0.5);
          input += inIncr;
        }
      }
    } // log scale with blending

    else // no log scale with blending
    {
      vtkLookupShiftAndScale(range, p.NumColors, p.Shift, p.Scale);
      p.Range[0] = range[0];
      p.Range[1] = range[1];

      if (outFormat == VTK_RGBA)
      {
        while (--i >= 0)
        {
          vtkIdType idx = vtkLinearLookup(*input, p);
          cptr = table + 4 * idx;
          output[0] = cptr[0];
          output[1] = cptr[1];
          output[2] = cptr[2];
          output[3] = static_cast<unsigned char>(cptr[3] * alpha + 0.5);
          input += inIncr;
          output += 4;
        }
      }
      else if (outFormat == VTK_RGB)
      {
        while (--i >= 0)
        {
          vtkIdType idx = vtkLinearLookup(*input, p);
          cptr = table + 4 * idx;
          memcpy(output, cptr, 3);
          input += inIncr;
          output += 3;
        }
      }
      else if (outFormat == VTK_LUMINANCE_ALPHA)
      {
        while (--i >= 0)
        {
          vtkIdType idx = vtkLinearLookup(*input, p);
          cptr = table + 4 * idx;
          output[0] =
            static_cast<unsigned char>(cptr[0] * 0.30 + cptr[1] * 0.59 + cptr[2] * 0.11 + 0.5);
          output[1] = static_cast<unsigned char>(cptr[3] * alpha + 0.5);
          input += inIncr;
          output += 2;
        }
      }
      else // outFormat == VTK_LUMINANCE
      {
        while (--i >= 0)
        {
          vtkIdType idx = vtkLinearLookup(*input, p);
          cptr = table + 4 * idx;
          *output++ =
            static_cast<unsigned char>(cptr[0] * 0.30 + cptr[1] * 0.59 + cptr[2] * 0.11 + 0.5);
          input += inIncr;
        }
      }
    } // no log scale
  }   // alpha blending
}

//----------------------------------------------------------------------------
template <class T>
void vtkLookupTableIndexedMapData(vtkLookupTable* self, const T* input, unsigned char* output,
  int length, int inIncr, int outFormat)
{
  int i = length;
  unsigned char* cptr;

  unsigned char nanColor[4];
  vtkLookupTable::GetColorAsUnsignedChars(self->GetNanColor(), nanColor);

  vtkVariant vin;
  double alpha = self->GetAlpha();
  if (alpha >= 1.0) // no blending required
  {
    if (outFormat == VTK_RGBA)
    {
      while (--i >= 0)
      {
        vin = *input;
        vtkIdType idx = self->GetAnnotatedValueIndexInternal(vin);
        cptr = idx < 0 ? nanColor : self->GetPointer(idx);

        memcpy(output, cptr, 4);
        input += inIncr;
        output += 4;
      }
    }
    else if (outFormat == VTK_RGB)
    {
      while (--i >= 0)
      {
        vin = *input;
        vtkIdType idx = self->GetAnnotatedValueIndexInternal(vin);
        cptr = idx < 0 ? nanColor : self->GetPointer(idx);

        memcpy(output, cptr, 3);
        input += inIncr;
        output += 3;
      }
    }
    else if (outFormat == VTK_LUMINANCE_ALPHA)
    {
      while (--i >= 0)
      {
        vin = *input;
        vtkIdType idx = self->GetAnnotatedValueIndexInternal(vin);
        cptr = idx < 0 ? nanColor : self->GetPointer(idx);
        output[0] =
          static_cast<unsigned char>(cptr[0] * 0.30 + cptr[1] * 0.59 + cptr[2] * 0.11 + 0.5);
        output[1] = cptr[3];
        input += inIncr;
        output += 2;
      }
    }
    else // outFormat == VTK_LUMINANCE
    {
      while (--i >= 0)
      {
        vin = *input;
        vtkIdType idx = self->GetAnnotatedValueIndexInternal(vin);
        cptr = idx < 0 ? nanColor : self->GetPointer(idx);
        *output++ =
          static_cast<unsigned char>(cptr[0] * 0.30 + cptr[1] * 0.59 + cptr[2] * 0.11 + 0.5);
        input += inIncr;
      }
    }
  } // if blending not needed

  else // blend with the specified alpha
  {
    if (outFormat == VTK_RGBA)
    {
      while (--i >= 0)
      {
        vin = *input;
        vtkIdType idx = self->GetAnnotatedValueIndexInternal(vin);
        cptr = idx < 0 ? nanColor : self->GetPointer(idx);
        memcpy(output, cptr, 3);
        output[3] = static_cast<unsigned char>(cptr[3] * alpha + 0.5);
        input += inIncr;
        output += 4;
      }
    }
    else if (outFormat == VTK_RGB)
    {
      while (--i >= 0)
      {
        vin = *input;
        vtkIdType idx = self->GetAnnotatedValueIndexInternal(vin);
        cptr = idx < 0 ? nanColor : self->GetPointer(idx);
        memcpy(output, cptr, 3);
        input += inIncr;
        output += 3;
      }
    }
    else if (outFormat == VTK_LUMINANCE_ALPHA)
    {
      while (--i >= 0)
      {
        vin = *input;
        vtkIdType idx = self->GetAnnotatedValueIndexInternal(vin);
        cptr = idx < 0 ? nanColor : self->GetPointer(idx);
        output[0] =
          static_cast<unsigned char>(cptr[0] * 0.30 + cptr[1] * 0.59 + cptr[2] * 0.11 + 0.5);
        output[1] = static_cast<unsigned char>(cptr[3] * alpha + 0.5);
        input += inIncr;
        output += 2;
      }
    }
    else // outFormat == VTK_LUMINANCE
    {
      while (--i >= 0)
      {
        vin = *input;
        vtkIdType idx = self->GetAnnotatedValueIndexInternal(vin);
        cptr = idx < 0 ? nanColor : self->GetPointer(idx);
        *output++ =
          static_cast<unsigned char>(cptr[0] * 0.30 + cptr[1] * 0.59 + cptr[2] * 0.11 + 0.5);
        input += inIncr;
      }
    }
  } // alpha blending
}

} // end anonymous namespace

//----------------------------------------------------------------------------
void vtkLookupTable::MapScalarsThroughTable2(void* input, unsigned char* output, int inputDataType,
  int numberOfValues, int inputIncrement, int outputFormat)
{
  if (this->IndexedLookup)
  {
    switch (inputDataType)
    {
      case VTK_BIT:
      {
        vtkIdType i, id;
        vtkBitArray* bitArray = vtkBitArray::New();
        bitArray->SetVoidArray(input, numberOfValues, 1);
        vtkUnsignedCharArray* newInput = vtkUnsignedCharArray::New();
        newInput->SetNumberOfValues(numberOfValues);
        for (id = i = 0; i < numberOfValues; i++, id += inputIncrement)
        {
          newInput->SetValue(i, bitArray->GetValue(id));
        }
        vtkLookupTableIndexedMapData(
          this, newInput->GetPointer(0), output, numberOfValues, inputIncrement, outputFormat);
        newInput->Delete();
        bitArray->Delete();
      }
      break;

        vtkTemplateMacro(vtkLookupTableIndexedMapData(
          this, static_cast<VTK_TT*>(input), output, numberOfValues, inputIncrement, outputFormat));

      case VTK_STRING:
        vtkLookupTableIndexedMapData(this, static_cast<vtkStdString*>(input), output,
          numberOfValues, inputIncrement, outputFormat);
        break;

      default:
        vtkErrorMacro(<< "MapScalarsThroughTable2: Unknown input ScalarType");
        return;
    }
  }
  else
  {
    TableParameters p;
    p.NumColors = this->GetNumberOfColors();

    switch (inputDataType)
    {
      case VTK_BIT:
      {
        vtkIdType i, id;
        vtkBitArray* bitArray = vtkBitArray::New();
        bitArray->SetVoidArray(input, numberOfValues, 1);
        vtkUnsignedCharArray* newInput = vtkUnsignedCharArray::New();
        newInput->SetNumberOfValues(numberOfValues);
        for (id = i = 0; i < numberOfValues; i++, id += inputIncrement)
        {
          newInput->SetValue(i, bitArray->GetValue(id));
        }
        vtkLookupTableMapData(
          this, newInput->GetPointer(0), output, numberOfValues, inputIncrement, outputFormat, p);
        newInput->Delete();
        bitArray->Delete();
      }
      break;

        vtkTemplateMacro(vtkLookupTableMapData(this, static_cast<VTK_TT*>(input), output,
          numberOfValues, inputIncrement, outputFormat, p));
      default:
        vtkErrorMacro(<< "MapScalarsThroughTable2: Unknown input ScalarType");
        return;
    }
  }
}

//----------------------------------------------------------------------------
// Specify the number of values (i.e., colors) in the lookup
// table. This method simply allocates memory and prepares the table
// for use with SetTableValue(). It differs from Build() method in
// that the allocated memory is not initialized according to HSVA ramps.
void vtkLookupTable::SetNumberOfTableValues(vtkIdType number)
{
  if (this->NumberOfColors == number)
  {
    return;
  }
  this->Modified();
  this->NumberOfColors = number;
  this->ResizeTableForSpecialColors();
  this->Table->SetNumberOfTuples(number);
}

//----------------------------------------------------------------------------
// Directly load color into lookup table. Use [0,1] double values for color
// component specification. Make sure that you've either used the
// Build() method or used SetNumberOfTableValues() prior to using this method.
void vtkLookupTable::SetTableValue(vtkIdType indx, const double rgba[4])
{
  // Check the index to make sure it is valid
  if (indx < 0)
  {
    vtkErrorMacro("Can't set the table value for negative index " << indx);
    return;
  }
  if (indx >= this->NumberOfColors)
  {
    vtkErrorMacro(
      "Index " << indx << " is greater than the number of colors " << this->NumberOfColors);
    return;
  }

  unsigned char* _rgba = this->Table->WritePointer(4 * indx, 4);

  _rgba[0] = static_cast<unsigned char>(rgba[0] * 255.0 + 0.5);
  _rgba[1] = static_cast<unsigned char>(rgba[1] * 255.0 + 0.5);
  _rgba[2] = static_cast<unsigned char>(rgba[2] * 255.0 + 0.5);
  _rgba[3] = static_cast<unsigned char>(rgba[3] * 255.0 + 0.5);

  if (indx == 0 || indx == this->NumberOfColors - 1)
  {
    // This is needed due to the way the special colors are stored in
    // the internal table. If Above/BelowRangeColors are not used and
    // the min/max colors are changed in the table with this member
    // function, then the colors used for values outside the range may
    // be incorrect. Calling this here ensures the out-of-range colors
    // are set correctly.
    this->BuildSpecialColors();
  }

  this->InsertTime.Modified();
  this->Modified();
}

//----------------------------------------------------------------------------
// Directly load color into lookup table. Use [0,1] double values for color
// component specification.
void vtkLookupTable::SetTableValue(vtkIdType indx, double r, double g, double b, double a)
{
  const double rgba[4] = { r, g, b, a };
  this->SetTableValue(indx, rgba);
}

//----------------------------------------------------------------------------
// Return an RGBA color value for the given index into the lookup Table. Color
// components are expressed as [0,1] double values.
void vtkLookupTable::GetTableValue(vtkIdType indx, double rgba[4])
{
  indx = (indx < 0 ? 0 : (indx >= this->NumberOfColors ? this->NumberOfColors - 1 : indx));

  const unsigned char* _rgba = this->Table->GetPointer(indx * 4);

  rgba[0] = _rgba[0] / 255.0;
  rgba[1] = _rgba[1] / 255.0;
  rgba[2] = _rgba[2] / 255.0;
  rgba[3] = _rgba[3] / 255.0;
}

// Return an RGBA color value for the given index into the lookup table. Color
// components are expressed as [0,1] double values.
double* vtkLookupTable::GetTableValue(vtkIdType indx)
{
  this->GetTableValue(indx, this->RGBA);
  return this->RGBA;
}

//----------------------------------------------------------------------------
void vtkLookupTable::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "TableRange: (" << this->TableRange[0] << ", " << this->TableRange[1] << ")\n";
  os << indent << "Scale: " << (this->Scale == VTK_SCALE_LOG10 ? "Log10\n" : "Linear\n");
  os << indent << "HueRange: (" << this->HueRange[0] << ", " << this->HueRange[1] << ")\n";
  os << indent << "SaturationRange: (" << this->SaturationRange[0] << ", "
     << this->SaturationRange[1] << ")\n";
  os << indent << "ValueRange: (" << this->ValueRange[0] << ", " << this->ValueRange[1] << ")\n";
  os << indent << "AlphaRange: (" << this->AlphaRange[0] << ", " << this->AlphaRange[1] << ")\n";

  os << indent << "NanColor: (" << this->NanColor[0] << ", " << this->NanColor[1] << ", "
     << this->NanColor[2] << ", " << this->NanColor[3] << ")\n";

  os << indent << "BelowRangeColor: (" << this->BelowRangeColor[0] << ", "
     << this->BelowRangeColor[1] << ", " << this->BelowRangeColor[2] << ", "
     << this->BelowRangeColor[3] << ")\n";
  os << indent << "UseBelowRangeColor: " << (this->UseBelowRangeColor != 0 ? "ON" : "OFF") << "\n";

  os << indent << "AboveRangeColor: (" << this->AboveRangeColor[0] << ", "
     << this->AboveRangeColor[1] << ", " << this->AboveRangeColor[2] << ", "
     << this->AboveRangeColor[3] << ")\n";
  os << indent << "UseAboveRangeColor: " << (this->UseAboveRangeColor != 0 ? "ON" : "OFF") << "\n";

  os << indent << "NumberOfTableValues: " << this->GetNumberOfTableValues() << "\n";
  os << indent << "NumberOfColors: " << this->NumberOfColors << "\n";
  os << indent << "Ramp: " << (this->Ramp == VTK_RAMP_SCURVE ? "SCurve\n" : "Linear\n");
  os << indent << "InsertTime: " << this->InsertTime.GetMTime() << "\n";
  os << indent << "BuildTime: " << this->BuildTime.GetMTime() << "\n";
  os << indent << "Table: ";
  if (this->Table)
  {
    this->Table->PrintSelf(os << "\n", indent.GetNextIndent());
  }
  else
  {
    // Should not happen
    os << "(none)\n";
  }
}

//----------------------------------------------------------------------------
void vtkLookupTable::DeepCopy(vtkScalarsToColors* obj)
{
  if (!obj)
  {
    return;
  }

  vtkLookupTable* lut = vtkLookupTable::SafeDownCast(obj);

  if (!lut)
  {
    vtkErrorMacro("Cannot DeepCopy a " << obj->GetClassName() << " into a vtkLookupTable.");
    return;
  }

  this->Scale = lut->Scale;
  this->TableRange[0] = lut->TableRange[0];
  this->TableRange[1] = lut->TableRange[1];
  this->HueRange[0] = lut->HueRange[0];
  this->HueRange[1] = lut->HueRange[1];
  this->SaturationRange[0] = lut->SaturationRange[0];
  this->SaturationRange[1] = lut->SaturationRange[1];
  this->ValueRange[0] = lut->ValueRange[0];
  this->ValueRange[1] = lut->ValueRange[1];
  this->AlphaRange[0] = lut->AlphaRange[0];
  this->AlphaRange[1] = lut->AlphaRange[1];
  this->NumberOfColors = lut->NumberOfColors;
  this->Ramp = lut->Ramp;
  this->InsertTime = lut->InsertTime;
  this->BuildTime = lut->BuildTime;

  for (int i = 0; i < 4; ++i)
  {
    this->NanColor[i] = lut->NanColor[i];
  }
  this->Table->DeepCopy(lut->Table);
  this->ResizeTableForSpecialColors();

  this->Superclass::DeepCopy(obj);
}

//----------------------------------------------------------------------------
vtkIdType vtkLookupTable::GetNumberOfAvailableColors()
{
  return this->Table->GetNumberOfTuples();
}

//----------------------------------------------------------------------------
void vtkLookupTable::GetIndexedColor(vtkIdType idx, double rgba[4])
{
  vtkIdType n = this->GetNumberOfAvailableColors();
  if (n > 0 && idx >= 0)
  {
    this->GetTableValue(idx % n, rgba);
    return;
  }
  this->GetNanColor(rgba);
}

//----------------------------------------------------------------------------
void vtkLookupTable::ResizeTableForSpecialColors()
{
  vtkIdType neededColors = this->NumberOfColors + vtkLookupTable::NUMBER_OF_SPECIAL_COLORS;
  if (this->Table->GetSize() < neededColors * this->Table->GetNumberOfComponents())
  {
    this->Table->Resize(neededColors);
  }
}
