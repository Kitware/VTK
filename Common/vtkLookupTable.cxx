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
#include "vtkBitArray.h"
#include "vtkObjectFactory.h"
#include "vtkMath.h"
#include <assert.h>

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
  this->Table->Allocate(4*sze,4*ext);

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

  this->TableRange[0] = 0.0;
  this->TableRange[1] = 1.0;

  this->Ramp = VTK_RAMP_SCURVE;
  this->Scale = VTK_SCALE_LINEAR;
  
  this->OpaqueFlag=1;
}

//----------------------------------------------------------------------------
vtkLookupTable::~vtkLookupTable()
{
  this->Table->UnRegister(this);
  this->Table = NULL;
}

//----------------------------------------------------------------------------
// Description:
// Return true if all of the values defining the mapping have an opacity
// equal to 1. Default implementation return true.
int vtkLookupTable::IsOpaque()
{
  if(this->OpaqueFlagBuildTime<this->GetMTime())
    {
    int opaque=1;
    if (this->NanColor[3] < 1.0) { opaque = 0; }
    int size=this->Table->GetNumberOfTuples();
    int i=0;
    unsigned char *ptr=this->Table->GetPointer(0);
    while(opaque && i<size)
      {
      opaque=ptr[3]==255;
      ptr+=4;
      ++i;
      }
    this->OpaqueFlag=opaque;
    this->OpaqueFlagBuildTime.Modified();
    }
  return this->OpaqueFlag;
}

//----------------------------------------------------------------------------
// Scalar values greater than maximum range value are clamped to maximum
// range value.
void vtkLookupTable::SetTableRange(double r[2])
{
  this->SetTableRange(r[0],r[1]);
}

//----------------------------------------------------------------------------
// Set the minimum/maximum scalar values for scalar mapping. Scalar values
// less than minimum range value are clamped to minimum range value.
// Scalar values greater than maximum range value are clamped to maximum
// range value.
void vtkLookupTable::SetTableRange(double rmin, double rmax)
{
  if (this->Scale == VTK_SCALE_LOG10 && 
      ((rmin > 0 && rmax < 0) || (rmin < 0 && rmax > 0)))
    {
    vtkErrorMacro("Bad table range for log scale: ["<<rmin<<", "<<rmax<<"]");
    return;
    }
  if (rmax < rmin)
    {
    vtkErrorMacro("Bad table range: ["<<rmin<<", "<<rmax<<"]");
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

  if (this->Scale == VTK_SCALE_LOG10 && 
      ((rmin > 0 && rmax < 0) || (rmin < 0 && rmax > 0)))
    {
    this->TableRange[0] = 1.0;
    this->TableRange[1] = 10.0;
    vtkErrorMacro("Bad table range for log scale: ["<<rmin<<", "<<rmax<<"], "
                  "adjusting to [1, 10]");
    return;
    }
}

//----------------------------------------------------------------------------
// Allocate a color table of specified size.
int vtkLookupTable::Allocate(int sz, int ext)
{
  this->NumberOfColors = sz;
  int a = this->Table->Allocate(4*this->NumberOfColors,4*ext);
  this->Modified();
  return a;
}

//----------------------------------------------------------------------------
// Force the lookup table to rebuild
void vtkLookupTable::ForceBuild()
{
  int i;
  double hue, sat, val, hinc, sinc, vinc, ainc;
  double rgba[4], alpha;
  unsigned char *c_rgba;

  int maxIndex = this->NumberOfColors - 1;

  if( maxIndex )
    {
    hinc = (this->HueRange[1] - this->HueRange[0])/maxIndex;
    sinc = (this->SaturationRange[1] - this->SaturationRange[0])/maxIndex;
    vinc = (this->ValueRange[1] - this->ValueRange[0])/maxIndex;
    ainc = (this->AlphaRange[1] - this->AlphaRange[0])/maxIndex;
    }
  else
    {
    hinc = sinc = vinc = ainc = 0.0; 
    }

  for (i = 0; i <= maxIndex; i++) 
    {
    hue = this->HueRange[0] + i*hinc;
    sat = this->SaturationRange[0] + i*sinc;
    val = this->ValueRange[0] + i*vinc;
    alpha = this->AlphaRange[0] + i*ainc;

    vtkMath::HSVToRGB(hue, sat, val, &rgba[0], &rgba[1], &rgba[2]);
    rgba[3] = alpha;

    c_rgba = this->Table->WritePointer(4*i,4);

    switch(this->Ramp)
      {
      case VTK_RAMP_SCURVE:
        {
        c_rgba[0] = static_cast<unsigned char> 
          (127.5*(1.0+cos((1.0-static_cast<double>(rgba[0]))*3.141593)));
        c_rgba[1] = static_cast<unsigned char> 
          (127.5*(1.0+cos((1.0-static_cast<double>(rgba[1]))*3.141593)));
        c_rgba[2] = static_cast<unsigned char> 
          (127.5*(1.0+cos((1.0-static_cast<double>(rgba[2]))*3.141593)));
        c_rgba[3] = static_cast<unsigned char> (alpha*255.0);
        /* same code, but with rounding 
           c_rgba[0] = static_cast<unsigned char> 
           (127.5f*(1.0f + (double)cos(double((1.0f-rgba[0])*3.141593f)))+0.5f);
           c_rgba[1] = static_cast<unsigned char> 
           (127.5f*(1.0f + (double)cos(double((1.0f-rgba[1])*3.141593f)))+0.5f);
           c_rgba[2] = static_cast<unsigned char> 
           (127.5f*(1.0f + (double)cos(double((1.0f-rgba[2])*3.141593f)))+0.5f);
           c_rgba[3] = static_cast<unsigned char>(rgba[3]*255.0f + 0.5f);
        */
        }
        break;
      case VTK_RAMP_LINEAR:
        {
        c_rgba[0] = static_cast<unsigned char>((rgba[0])*255.0f + 0.5f);
        c_rgba[1] = static_cast<unsigned char>((rgba[1])*255.0f + 0.5f);
        c_rgba[2] = static_cast<unsigned char>((rgba[2])*255.0f + 0.5f);
        c_rgba[3] = static_cast<unsigned char>(rgba[3]*255.0f + 0.5f);
        }
        break;
      case VTK_RAMP_SQRT:
        {
        c_rgba[0] = static_cast<unsigned char>((sqrt(rgba[0]))*255.0f + 0.5f);
        c_rgba[1] = static_cast<unsigned char>((sqrt(rgba[1]))*255.0f + 0.5f);
        c_rgba[2] = static_cast<unsigned char>((sqrt(rgba[2]))*255.0f + 0.5f);
        c_rgba[3] = static_cast<unsigned char>((sqrt(rgba[3]))*255.0f + 0.5f);
        }
        break;
      default:
        assert("check: impossible case." && 0); // reaching this line is a bug.
        break;
      }
    }
  this->BuildTime.Modified();
}

//----------------------------------------------------------------------------
// Generate lookup table from hue, saturation, value, alpha min/max values. 
// Table is built from linear ramp of each value.
void vtkLookupTable::Build()
{
  if (this->Table->GetNumberOfTuples() < 1 ||
      (this->GetMTime() > this->BuildTime && 
       this->InsertTime <= this->BuildTime))
    {
    this->ForceBuild();
    }
}

//----------------------------------------------------------------------------
// get the color for a scalar value
void vtkLookupTable::GetColor(double v, double rgb[3])
{
  unsigned char *rgb8 = this->MapValue(v);

  rgb[0] = rgb8[0]/255.0;
  rgb[1] = rgb8[1]/255.0;
  rgb[2] = rgb8[2]/255.0;
}

//----------------------------------------------------------------------------
// get the opacity (alpha) for a scalar value
double vtkLookupTable::GetOpacity(double v)
{
  unsigned char *rgb8 = this->MapValue(v);

  return rgb8[3]/255.0;
}
 
//----------------------------------------------------------------------------
// There is a little more to this than simply taking the log10 of the
// two range values: we do conversion of negative ranges to positive
// ranges, and conversion of zero to a 'very small number'
void vtkLookupTableLogRange(const double range[2], double logRange[2])
{
  double rmin = range[0];
  double rmax = range[1];

  if (rmin == 0)
    {
    rmin = 1.0e-6*(rmax - rmin);
    if (rmax < 0)
      {
      rmin = -rmin;
      }
    }
  if (rmax == 0)
    {
    rmax = 1.0e-6*(rmin - rmax);
    if (rmin < 0)
      {
      rmax = -rmax;
      }
    }
  if (rmin < 0 && rmax < 0)
    {
    logRange[0] = log10(-static_cast<double>(rmin));
    logRange[1] = log10(-static_cast<double>(rmax));
    }
  else if (rmin > 0 && rmax > 0)
    {
    logRange[0] = log10(static_cast<double>(rmin));
    logRange[1] = log10(static_cast<double>(rmax));
    }
}

//----------------------------------------------------------------------------
// Apply log to value, with appropriate constraints.
inline double vtkApplyLogScale(double v, const double range[2], 
                               const double logRange[2])
{
  // is the range set for negative numbers?
  if (range[0] < 0)
    {
    if (v < 0)
      {
      v = log10(-static_cast<double>(v));
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
      v = log10(static_cast<double>(v));
      }
    else if (range[0] < range[1])
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
// Apply shift/scale to the scalar value v and do table lookup.
inline unsigned char *vtkLinearLookup(double v,
                                      unsigned char *table,
                                      double maxIndex,
                                      double shift, double scale,
                                      unsigned char *nanColor)
{
  if (vtkMath::IsNan(v))
    {
    return nanColor;
    }

  double findx = (v + shift)*scale;
  if (findx < 0.0)
    {
    findx = 0.0;
    }
  else if (findx > maxIndex)
    {
    findx = maxIndex;
    }
  return &table[4*static_cast<unsigned int>(findx)];
  /* round
  return &table[4*(unsigned int)(findx + 0.5f)];
  */
}

//----------------------------------------------------------------------------
void vtkLookupTable::GetLogRange(const double range[2], double log_range[2])
{
  vtkLookupTableLogRange(range, log_range);
}

//----------------------------------------------------------------------------
double vtkLookupTable::ApplyLogScale(double v, const double range[2],
  const double log_range[2])
{
  return vtkApplyLogScale(v, range, log_range);
}

//----------------------------------------------------------------------------
// Given a scalar value v, return an index into the lookup table
vtkIdType vtkLookupTable::GetIndex(double v)
{
  double maxIndex = this->NumberOfColors - 1;
  double shift, scale;

  if (this->Scale == VTK_SCALE_LOG10)
    {   // handle logarithmic scale
    double logRange[2];
    vtkLookupTableLogRange(this->TableRange, logRange);
    shift = -logRange[0];
    if (logRange[1] <= logRange[0])
      {
      scale = VTK_DOUBLE_MAX;
      }
    else
      {
      /* while this looks like the wrong scale, it is the correct scale
       * taking into account the truncation to int that happens below. */
      scale = (maxIndex + 1)/(logRange[1] - logRange[0]);
      }
    v = vtkApplyLogScale(v, this->TableRange, logRange);
    }
  else
    {   // plain old linear
    shift = -this->TableRange[0];
    if (this->TableRange[1] <= this->TableRange[0])
      {
      scale = VTK_DOUBLE_MAX;
      }
    else
      {
      /* while this looks like the wrong scale, it is the correct scale
       * taking into account the truncation to int that happens below. */
      scale = (maxIndex + 1)/(this->TableRange[1] - this->TableRange[0]);
      }
    }

  // map to an index
  double findx = (v + shift)*scale;
  if (findx < 0)
    {
    findx = 0;
    }
  if (findx > maxIndex)
    {
    findx = maxIndex;
    }
  return static_cast<int>(findx);
}

//----------------------------------------------------------------------------
// Given a table, set the internal table and set the number of colors.
void vtkLookupTable::SetTable(vtkUnsignedCharArray *table)
{
  if (table != this->Table && table != NULL)
    {
    // Check for incorrect arrays.
    if (table->GetNumberOfComponents() != this->Table->GetNumberOfComponents())
      {
      vtkErrorMacro(<<"Number of components in given table (" 
                    << table->GetNumberOfComponents()
                    << ") is incorrect, it should have "
                    << this->Table->GetNumberOfComponents() 
                    << "." );
      return;
      }
    this->Table->UnRegister(this);
    this->Table = table;
    this->Table->Register(this);
    this->NumberOfColors = this->Table->GetNumberOfTuples();
    // If InsertTime is not modified the array will be rebuilt.  So we
    // use the same approach that the SetTableValue function does.
    this->InsertTime.Modified();
    this->Modified();
    }
}

//----------------------------------------------------------------------------
// Given a scalar value v, return an rgba color value from lookup table.
unsigned char *vtkLookupTable::MapValue(double v)
{
  int idx = this->GetIndex(v);
  return (this->Table->GetPointer(0) + 4*idx);
}

//----------------------------------------------------------------------------
// accelerate the mapping by copying the data in 32-bit chunks instead
// of 8-bit chunks
template<class T>
void vtkLookupTableMapData(vtkLookupTable *self, T *input, 
                           unsigned char *output, int length, 
                           int inIncr, int outFormat)
{
  int i = length;
  double *range = self->GetTableRange();
  double maxIndex = self->GetNumberOfColors() - 1;
  double shift, scale;
  unsigned char *table = self->GetPointer(0);
  unsigned char *cptr;
  double alpha;

  unsigned char nanColor[4];
  for (int c = 0; c < 4; c++)
    {
    nanColor[c] = static_cast<unsigned char>(self->GetNanColor()[c]*255.0);
    }

  if ( (alpha=self->GetAlpha()) >= 1.0 ) //no blending required 
    {
    if (self->GetScale() == VTK_SCALE_LOG10)
      {
      double val;
      double logRange[2];
      vtkLookupTableLogRange(range, logRange);
      shift = -logRange[0];
      if (logRange[1] <= logRange[0])
        {
        scale = VTK_DOUBLE_MAX;
        }
      else
        {
        /* while this looks like the wrong scale, it is the correct scale
         * taking into account the truncation to int that happens below. */
        scale = (maxIndex + 1)/(logRange[1] - logRange[0]);
        }
      if (outFormat == VTK_RGBA)
        {
        while (--i >= 0) 
          {
          val = vtkApplyLogScale(*input, range, logRange);
          cptr = vtkLinearLookup(val, table, maxIndex, shift, scale, nanColor);
          *output++ = *cptr++;
          *output++ = *cptr++;
          *output++ = *cptr++;
          *output++ = *cptr++;     
          input += inIncr;
          }
        }
      else if (outFormat == VTK_RGB)
        {
        while (--i >= 0) 
          {
          val = vtkApplyLogScale(*input, range, logRange);
          cptr = vtkLinearLookup(val, table, maxIndex, shift, scale, nanColor);
          *output++ = *cptr++;
          *output++ = *cptr++;
          *output++ = *cptr++;
          input += inIncr;
          }
        }
      else if (outFormat == VTK_LUMINANCE_ALPHA)
        {
        while (--i >= 0) 
          {
          val = vtkApplyLogScale(*input, range, logRange);
          cptr = vtkLinearLookup(val, table, maxIndex, shift, scale, nanColor);
          *output++ = static_cast<unsigned char>(cptr[0]*0.30 + cptr[1]*0.59 + 
                                                 cptr[2]*0.11 + 0.5);
          *output++ = cptr[3];
          input += inIncr;
          }
        }
      else // outFormat == VTK_LUMINANCE
        {
        while (--i >= 0) 
          {
          val = vtkApplyLogScale(*input, range, logRange);
          cptr = vtkLinearLookup(val, table, maxIndex, shift, scale, nanColor);
          *output++ = static_cast<unsigned char>(cptr[0]*0.30 + cptr[1]*0.59 + 
                                                 cptr[2]*0.11 + 0.5);
          input += inIncr;
          }
        }
      }//if log scale

    else //not log scale
      {
      shift = -range[0];
      if (range[1] <= range[0])
        {
        scale = VTK_DOUBLE_MAX;
        }
      else
        {
        /* while this looks like the wrong scale, it is the correct scale
         * taking into account the truncation to int that happens below. */
        scale = (maxIndex + 1)/(range[1] - range[0]);
        }

      if (outFormat == VTK_RGBA)
        {
        while (--i >= 0) 
          {
          cptr = vtkLinearLookup(*input, table, maxIndex, shift, scale,
                                 nanColor);
          *output++ = *cptr++;
          *output++ = *cptr++;
          *output++ = *cptr++;
          *output++ = *cptr++;     
          input += inIncr;
          }
        }
      else if (outFormat == VTK_RGB)
        {
        while (--i >= 0) 
          {
          cptr = vtkLinearLookup(*input, table, maxIndex, shift, scale,
                                 nanColor);
          *output++ = *cptr++;
          *output++ = *cptr++;
          *output++ = *cptr++;
          input += inIncr;
          }
        }
      else if (outFormat == VTK_LUMINANCE_ALPHA)
        {
        while (--i >= 0) 
          {
          cptr = vtkLinearLookup(*input, table, maxIndex, shift, scale,
                                 nanColor);
          *output++ = static_cast<unsigned char>(cptr[0]*0.30 + cptr[1]*0.59 + 
                                                 cptr[2]*0.11 + 0.5);
          *output++ = cptr[3];
          input += inIncr;
          }
        }
      else // outFormat == VTK_LUMINANCE
        {
        while (--i >= 0) 
          {
          cptr = vtkLinearLookup(*input, table, maxIndex, shift, scale,
                                 nanColor);
          *output++ = static_cast<unsigned char>(cptr[0]*0.30 + cptr[1]*0.59 + 
                                                 cptr[2]*0.11 + 0.5);
          input += inIncr;
          }
        }
      }//if not log lookup
    }//if blending not needed

  else //blend with the specified alpha
    {
    if (self->GetScale() == VTK_SCALE_LOG10)
      {
      double val;
      double logRange[2];
      vtkLookupTableLogRange(range, logRange);
      shift = -logRange[0];
      if (logRange[1] <= logRange[0])
        {
        scale = VTK_DOUBLE_MAX;
        }
      else
        {
        /* while this looks like the wrong scale, it is the correct scale
         * taking into account the truncation to int that happens below. */
        scale = (maxIndex + 1)/(logRange[1] - logRange[0]);
        }
      if (outFormat == VTK_RGBA)
        {
        while (--i >= 0) 
          {
          val = vtkApplyLogScale(*input, range, logRange);
          cptr = vtkLinearLookup(val, table, maxIndex, shift, scale, nanColor);
          *output++ = *cptr++;
          *output++ = *cptr++;
          *output++ = *cptr++;
          *output++ = static_cast<unsigned char>((*cptr)*alpha); cptr++;
          input += inIncr;
          }
        }
      else if (outFormat == VTK_RGB)
        {
        while (--i >= 0) 
          {
          val = vtkApplyLogScale(*input, range, logRange);
          cptr = vtkLinearLookup(val, table, maxIndex, shift, scale, nanColor);
          *output++ = *cptr++;
          *output++ = *cptr++;
          *output++ = *cptr++;
          input += inIncr;
          }
        }
      else if (outFormat == VTK_LUMINANCE_ALPHA)
        {
        while (--i >= 0) 
          {
          val = vtkApplyLogScale(*input, range, logRange);
          cptr = vtkLinearLookup(val, table, maxIndex, shift, scale, nanColor);
          *output++ = static_cast<unsigned char>(cptr[0]*0.30 + cptr[1]*0.59 + 
                                                 cptr[2]*0.11 + 0.5);
          *output++ = static_cast<unsigned char>(alpha*cptr[3]);
          input += inIncr;
          }
        }
      else // outFormat == VTK_LUMINANCE
        {
        while (--i >= 0) 
          {
          val = vtkApplyLogScale(*input, range, logRange);
          cptr = vtkLinearLookup(val, table, maxIndex, shift, scale, nanColor);
          *output++ = static_cast<unsigned char>(cptr[0]*0.30 + cptr[1]*0.59 + 
                                                 cptr[2]*0.11 + 0.5);
          input += inIncr;
          }
        }
      }//log scale with blending

    else //no log scale with blending
      {
      shift = -range[0];
      if (range[1] <= range[0])
        {
        scale = VTK_DOUBLE_MAX;
        }
      else
        {
        /* while this looks like the wrong scale, it is the correct scale
         * taking into account the truncation to int that happens below. */
        scale = (maxIndex + 1)/(range[1] - range[0]);
        }

      if (outFormat == VTK_RGBA)
        {
        while (--i >= 0) 
          {
          cptr = vtkLinearLookup(*input, table, maxIndex, shift, scale,
                                 nanColor);
          *output++ = *cptr++;
          *output++ = *cptr++;
          *output++ = *cptr++;
          *output++ = static_cast<unsigned char>((*cptr)*alpha); cptr++;
          input += inIncr;
          }
        }
      else if (outFormat == VTK_RGB)
        {
        while (--i >= 0) 
          {
          cptr = vtkLinearLookup(*input, table, maxIndex, shift, scale,
                                 nanColor);
          *output++ = *cptr++;
          *output++ = *cptr++;
          *output++ = *cptr++;
          input += inIncr;
          }
        }
      else if (outFormat == VTK_LUMINANCE_ALPHA)
        {
        while (--i >= 0) 
          {
          cptr = vtkLinearLookup(*input, table, maxIndex, shift, scale,
                                 nanColor);
          *output++ = static_cast<unsigned char>(cptr[0]*0.30 + cptr[1]*0.59 + 
                                                 cptr[2]*0.11 + 0.5);
          *output++ = static_cast<unsigned char>(cptr[3]*alpha);
          input += inIncr;
          }
        }
      else // outFormat == VTK_LUMINANCE
        {
        while (--i >= 0) 
          {
          cptr = vtkLinearLookup(*input, table, maxIndex, shift, scale,
                                 nanColor);
          *output++ = static_cast<unsigned char>(cptr[0]*0.30 + cptr[1]*0.59 + 
                                                 cptr[2]*0.11 + 0.5);
          input += inIncr;
          }
        }
      }//no log scale
    }//alpha blending
}

//----------------------------------------------------------------------------
// Although this is a relatively expensive calculation,
// it is only done on the first render. Colors are cached
// for subsequent renders.
template<class T>
void vtkLookupTableMapMag(vtkLookupTable *self, T *input, 
                          unsigned char *output, int length, 
                          int inIncr, int outFormat)
{
  double tmp, sum;
  double *mag;
  int i, j;

  mag = new double[length];
  for (i = 0; i < length; ++i)
    {
    sum = 0;
    for (j = 0; j < inIncr; ++j)
      {
      tmp = static_cast<double>(*input);  
      sum += (tmp * tmp);
      ++input;
      }
    mag[i] = sqrt(sum);
    }

  vtkLookupTableMapData(self, mag, output, length, 1, outFormat);

  delete [] mag;
}


//----------------------------------------------------------------------------
void vtkLookupTable::MapScalarsThroughTable2(void *input, 
                                             unsigned char *output,
                                             int inputDataType, 
                                             int numberOfValues,
                                             int inputIncrement,
                                             int outputFormat)
{
  if (this->UseMagnitude && inputIncrement > 1)
    {
    switch (inputDataType)
      {
      vtkTemplateMacro(
        vtkLookupTableMapMag(this,static_cast<VTK_TT*>(input),output,
                             numberOfValues,inputIncrement,outputFormat);
        return
        );
      case VTK_BIT:
        vtkErrorMacro("Cannot comput magnitude of bit array.");
        break;
      default:
        vtkErrorMacro(<< "MapImageThroughTable: Unknown input ScalarType");
      }
    }

  switch (inputDataType)
    {
    case VTK_BIT:
      {
      vtkIdType i, id;
      vtkBitArray *bitArray = vtkBitArray::New();
      bitArray->SetVoidArray(input,numberOfValues,1);
      vtkUnsignedCharArray *newInput = vtkUnsignedCharArray::New();
      newInput->SetNumberOfValues(numberOfValues);
      for (id=i=0; i<numberOfValues; i++, id+=inputIncrement)
        {
        newInput->SetValue(i, bitArray->GetValue(id));
        }
      vtkLookupTableMapData(this,
                            static_cast<unsigned char*>(newInput->GetPointer(0)),
                            output,numberOfValues,
                            inputIncrement,outputFormat);
      newInput->Delete();
      bitArray->Delete();
      }
      break;

    vtkTemplateMacro(
      vtkLookupTableMapData(this,static_cast<VTK_TT*>(input),output,
                            numberOfValues,inputIncrement,outputFormat)
      );
    default:
      vtkErrorMacro(<< "MapImageThroughTable: Unknown input ScalarType");
      return;
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
  this->Table->SetNumberOfTuples(number);
}

//----------------------------------------------------------------------------
// Directly load color into lookup table. Use [0,1] double values for color
// component specification. Make sure that you've either used the
// Build() method or used SetNumberOfTableValues() prior to using this method.
void vtkLookupTable::SetTableValue(vtkIdType indx, double rgba[4])
{
  // Check the index to make sure it is valid
  if (indx < 0)
    {
    vtkErrorMacro("Can't set the table value for negative index " << indx);
    return;
    }
  if (indx >= this->NumberOfColors)
    {
    vtkErrorMacro("Index " << indx << 
                  " is greater than the number of colors " << 
                  this->NumberOfColors);
    return;
    }

  unsigned char *_rgba = this->Table->WritePointer(4*indx,4);

  _rgba[0] = static_cast<unsigned char>(rgba[0]*255.0f + 0.5f);
  _rgba[1] = static_cast<unsigned char>(rgba[1]*255.0f + 0.5f);
  _rgba[2] = static_cast<unsigned char>(rgba[2]*255.0f + 0.5f);
  _rgba[3] = static_cast<unsigned char>(rgba[3]*255.0f + 0.5f);

  this->InsertTime.Modified();
  this->Modified();
}

//----------------------------------------------------------------------------
// Directly load color into lookup table. Use [0,1] double values for color 
// component specification.
void vtkLookupTable::SetTableValue(vtkIdType indx, double r, double g, double b, 
                                   double a)
{
  double rgba[4];
  rgba[0] = r; rgba[1] = g; rgba[2] = b; rgba[3] = a;
  this->SetTableValue(indx,rgba);
}

//----------------------------------------------------------------------------
// Return a rgba color value for the given index into the lookup Table. Color
// components are expressed as [0,1] double values.
void vtkLookupTable::GetTableValue(vtkIdType indx, double rgba[4])
{
  unsigned char *_rgba;

  indx = (indx < 0 ? 0 : (indx >= this->NumberOfColors ? 
                          this->NumberOfColors-1 : indx));

  _rgba = this->Table->GetPointer(indx*4);

  rgba[0] = _rgba[0]/255.0;
  rgba[1] = _rgba[1]/255.0;
  rgba[2] = _rgba[2]/255.0;
  rgba[3] = _rgba[3]/255.0;
}

// Return a rgba color value for the given index into the lookup table. Color
// components are expressed as [0,1] double values.
double *vtkLookupTable::GetTableValue(vtkIdType indx)
{
  this->GetTableValue(indx, this->RGBA);
  return this->RGBA;
}

//----------------------------------------------------------------------------
void vtkLookupTable::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "TableRange: (" << this->TableRange[0] << ", "
     << this->TableRange[1] << ")\n";
  os << indent << "Scale: "
     << (this->Scale == VTK_SCALE_LOG10 ? "Log10\n" : "Linear\n");
  os << indent << "HueRange: (" << this->HueRange[0] << ", "
     << this->HueRange[1] << ")\n";
  os << indent << "SaturationRange: (" << this->SaturationRange[0] << ", "
     << this->SaturationRange[1] << ")\n";
  os << indent << "ValueRange: (" << this->ValueRange[0] << ", "
     << this->ValueRange[1] << ")\n";
  os << indent << "AlphaRange: (" << this->AlphaRange[0] << ", "
     << this->AlphaRange[1] << ")\n";
  os << indent << "NanColor: (" << this->NanColor[0] << ", "
     << this->NanColor[1] << ", " << this->NanColor[2] << ", "
     << this->NanColor[3] << ")\n";
  os << indent << "NumberOfTableValues: "
     << this->GetNumberOfTableValues() << "\n";
  os << indent << "NumberOfColors: " << this->NumberOfColors << "\n";
  os << indent << "Ramp: "
     << (this->Ramp == VTK_RAMP_SCURVE ? "SCurve\n" : "Linear\n");
  os << indent << "InsertTime: " <<this->InsertTime.GetMTime() << "\n";
  os << indent << "BuildTime: " <<this->BuildTime.GetMTime() << "\n";
  os << indent << "Table: ";
  if( this->Table )
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
void vtkLookupTable::DeepCopy(vtkLookupTable *lut)
{
  if (!lut)
    {
    return;
    }
  this->Alpha               = lut->Alpha;
  this->UseMagnitude        = lut->UseMagnitude;
  this->VectorMode          = lut->VectorMode;
  this->VectorComponent     = lut->VectorComponent;
  this->Scale               = lut->Scale;
  this->TableRange[0]       = lut->TableRange[0];
  this->TableRange[1]       = lut->TableRange[1];
  this->HueRange[0]         = lut->HueRange[0];
  this->HueRange[1]         = lut->HueRange[1];
  this->SaturationRange[0]  = lut->SaturationRange[0];
  this->SaturationRange[1]  = lut->SaturationRange[1];
  this->ValueRange[0]       = lut->ValueRange[0];
  this->ValueRange[1]       = lut->ValueRange[1];
  this->AlphaRange[0]       = lut->AlphaRange[0];
  this->AlphaRange[1]       = lut->AlphaRange[1];
  this->NumberOfColors      = lut->NumberOfColors;
  this->Ramp                = lut->Ramp;
  this->InsertTime          = lut->InsertTime;
  this->BuildTime           = lut->BuildTime;
  this->Table->DeepCopy(lut->Table);
}

//----------------------------------------------------------------------------
vtkIdType vtkLookupTable::GetNumberOfAvailableColors()
{
  return this->Table->GetNumberOfTuples();
}
