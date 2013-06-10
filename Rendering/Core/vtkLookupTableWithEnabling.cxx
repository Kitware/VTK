/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLookupTableWithEnabling.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkLookupTableWithEnabling.h"
#include "vtkBitArray.h"
#include "vtkObjectFactory.h"
#include "vtkMath.h"
#include "vtkVariant.h"
#include <assert.h>

vtkStandardNewMacro(vtkLookupTableWithEnabling);

vtkCxxSetObjectMacro(vtkLookupTableWithEnabling,EnabledArray,vtkDataArray);

// Construct with range=(0,1); and hsv ranges set up for rainbow color table
// (from red to blue).
vtkLookupTableWithEnabling::vtkLookupTableWithEnabling(int sze, int ext) :
  vtkLookupTable(sze, ext)
{
  this->EnabledArray = 0;
}

//----------------------------------------------------------------------------
vtkLookupTableWithEnabling::~vtkLookupTableWithEnabling()
{
  if(this->EnabledArray)
    {
    this->EnabledArray->Delete();
    this->EnabledArray = 0;
    }
}

void vtkLookupTableWithEnabling::DisableColor(
          unsigned char r, unsigned char g, unsigned char b,
          unsigned char *rd, unsigned char *gd, unsigned char *bd)
{
  double rgb[3], hsv[3];
  rgb[0] = static_cast<double>(r);
  rgb[1] = static_cast<double>(g);
  rgb[2] = static_cast<double>(b);
  vtkMath::RGBToHSV(rgb,hsv);
  hsv[1] = 0;
  hsv[2] = 0;
  vtkMath::HSVToRGB(hsv,rgb);
  *rd = static_cast<unsigned char>(rgb[0]);
  *gd = static_cast<unsigned char>(rgb[1]);
  *bd = static_cast<unsigned char>(rgb[2]);
}


//----------------------------------------------------------------------------
// There is a little more to this than simply taking the log10 of the
// two range values: we do conversion of negative ranges to positive
// ranges, and conversion of zero to a 'very small number'
static void vtkLookupTableWithEnablingLogRange(double range[2], double logRange[2])
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
  else
    {
    logRange[0] = 0.;
    logRange[1] = 0.;
    }
}

//----------------------------------------------------------------------------
// Apply log to value, with appropriate constraints.
inline double vtkApplyLogScale(double v, double range[2],
                               double logRange[2])
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
                                      double shift, double scale)
{
  double findx = (v + shift)*scale;
  if (findx < 0)
    {
    findx = 0;
    }
  if (findx > maxIndex)
    {
    findx = maxIndex;
    }
  return &table[4*static_cast<int>(findx)];
  /* round
  return &table[4*(int)(findx + 0.5f)];
  */
}

//----------------------------------------------------------------------------
// accelerate the mapping by copying the data in 32-bit chunks instead
// of 8-bit chunks
template<class T>
void vtkLookupTableWithEnablingMapData(vtkLookupTableWithEnabling *self, T *input,
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
  unsigned char r, g, b;

  bool hasEnabledArray = false;
  if(self->GetEnabledArray() &&
     self->GetEnabledArray()->GetNumberOfTuples() == length)
    {
    hasEnabledArray = true;
    }

  if ( (alpha=self->GetAlpha()) >= 1.0 ) //no blending required
    {
    if (self->GetScale() == VTK_SCALE_LOG10)
      {
      double val;
      double logRange[2];
      vtkLookupTableWithEnablingLogRange(range, logRange);
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
          cptr = vtkLinearLookup(val, table, maxIndex, shift, scale);
          if(hasEnabledArray &&
            !self->GetEnabledArray()->GetTuple1(length-i-1))
            {
            self->DisableColor(cptr[0],cptr[1],cptr[2],&r,&g,&b);
            *output++ = r;
            *output++ = g;
            *output++ = b;
            cptr+=3;
            }
          else
            {
            *output++ = *cptr++;
            *output++ = *cptr++;
            *output++ = *cptr++;
            }
          *output++ = *cptr++;
          input += inIncr;
          }
        }
      else if (outFormat == VTK_RGB)
        {
        while (--i >= 0)
          {
          val = vtkApplyLogScale(*input, range, logRange);
          cptr = vtkLinearLookup(val, table, maxIndex, shift, scale);
          if(hasEnabledArray &&
             !self->GetEnabledArray()->GetTuple1(length-i-1))
            {
            self->DisableColor(cptr[0],cptr[1],cptr[2],&r,&g,&b);
            *output++ = r;
            *output++ = g;
            *output++ = b;
            cptr+=3;
            }
          else
            {
            *output++ = *cptr++;
            *output++ = *cptr++;
            *output++ = *cptr++;
            }
          input += inIncr;
          }
        }
      else if (outFormat == VTK_LUMINANCE_ALPHA)
        {
        while (--i >= 0)
          {
          val = vtkApplyLogScale(*input, range, logRange);
          cptr = vtkLinearLookup(val, table, maxIndex, shift, scale);
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
          cptr = vtkLinearLookup(val, table, maxIndex, shift, scale);
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
          cptr = vtkLinearLookup(*input, table, maxIndex, shift, scale);
          if(hasEnabledArray &&
            !self->GetEnabledArray()->GetTuple1(length-i-1))
            {
            self->DisableColor(cptr[0],cptr[1],cptr[2],&r,&g,&b);
            *output++ = r;
            *output++ = g;
            *output++ = b;
            cptr+=3;
            *output++ = static_cast<unsigned char>((*cptr)*0.2); cptr++;
            }
          else
            {
            *output++ = *cptr++;
            *output++ = *cptr++;
            *output++ = *cptr++;
            *output++ = *cptr++;
            }
          input += inIncr;
          }
        }
      else if (outFormat == VTK_RGB)
        {
        while (--i >= 0)
          {
          cptr = vtkLinearLookup(*input, table, maxIndex, shift, scale);
          if(hasEnabledArray &&
             !self->GetEnabledArray()->GetTuple1(length-i-1))
            {
            self->DisableColor(cptr[0],cptr[1],cptr[2],&r,&g,&b);
            *output++ = r;
            *output++ = g;
            *output++ = b;
            cptr+=3;
            }
          else
            {
            *output++ = *cptr++;
            *output++ = *cptr++;
            *output++ = *cptr++;
            }
          input += inIncr;
          }
        }
      else if (outFormat == VTK_LUMINANCE_ALPHA)
        {
        while (--i >= 0)
          {
          cptr = vtkLinearLookup(*input, table, maxIndex, shift, scale);
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
          cptr = vtkLinearLookup(*input, table, maxIndex, shift, scale);
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
      vtkLookupTableWithEnablingLogRange(range, logRange);
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
          cptr = vtkLinearLookup(val, table, maxIndex, shift, scale);
          if(hasEnabledArray &&
             !self->GetEnabledArray()->GetTuple1(length-i-1))
            {
            self->DisableColor(cptr[0],cptr[1],cptr[2],&r,&g,&b);
            *output++ = r;
            *output++ = g;
            *output++ = b;
            cptr+=3;
            }
          else
            {
            *output++ = *cptr++;
            *output++ = *cptr++;
            *output++ = *cptr++;
            }
          *output++ = static_cast<unsigned char>((*cptr)*alpha); cptr++;
          input += inIncr;
          }
        }
      else if (outFormat == VTK_RGB)
        {
        while (--i >= 0)
          {
          val = vtkApplyLogScale(*input, range, logRange);
          cptr = vtkLinearLookup(val, table, maxIndex, shift, scale);
          if(hasEnabledArray &&
             !self->GetEnabledArray()->GetTuple1(length-i-1))
            {
            self->DisableColor(cptr[0],cptr[1],cptr[2],&r,&g,&b);
            *output++ = r;
            *output++ = g;
            *output++ = b;
            cptr+=3;
            }
          else
            {
            *output++ = *cptr++;
            *output++ = *cptr++;
            *output++ = *cptr++;
            }
          input += inIncr;
          }
        }
      else if (outFormat == VTK_LUMINANCE_ALPHA)
        {
        while (--i >= 0)
          {
          val = vtkApplyLogScale(*input, range, logRange);
          cptr = vtkLinearLookup(val, table, maxIndex, shift, scale);
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
          cptr = vtkLinearLookup(val, table, maxIndex, shift, scale);
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
          cptr = vtkLinearLookup(*input, table, maxIndex, shift, scale);
          if(hasEnabledArray &&
             !self->GetEnabledArray()->GetTuple1(length-i-1))
            {
            self->DisableColor(cptr[0],cptr[1],cptr[2],&r,&g,&b);
            *output++ = r;
            *output++ = g;
            *output++ = b;
            cptr+=3;
            *output++ = static_cast<unsigned char>((*cptr)*alpha*0.2); cptr++;
            }
          else
            {
            *output++ = *cptr++;
            *output++ = *cptr++;
            *output++ = *cptr++;
            if(hasEnabledArray)
              {
              *output++ = *cptr++;
              }
            else
              {
              *output++ = static_cast<unsigned char>((*cptr)*alpha); cptr++;
              }
            }
          input += inIncr;
          }
        }
      else if (outFormat == VTK_RGB)
        {
        while (--i >= 0)
          {
          cptr = vtkLinearLookup(*input, table, maxIndex, shift, scale);
          if(hasEnabledArray &&
             !self->GetEnabledArray()->GetTuple1(length-i-1))
            {
            self->DisableColor(cptr[0],cptr[1],cptr[2],&r,&g,&b);
            *output++ = r;
            *output++ = g;
            *output++ = b;
            cptr+=3;
            }
          else
            {
            *output++ = *cptr++;
            *output++ = *cptr++;
            *output++ = *cptr++;
            }
          input += inIncr;
          }
        }
      else if (outFormat == VTK_LUMINANCE_ALPHA)
        {
        while (--i >= 0)
          {
          cptr = vtkLinearLookup(*input, table, maxIndex, shift, scale);
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
          cptr = vtkLinearLookup(*input, table, maxIndex, shift, scale);
          *output++ = static_cast<unsigned char>(cptr[0]*0.30 + cptr[1]*0.59 +
                                                 cptr[2]*0.11 + 0.5);
          input += inIncr;
          }
        }
      }//no log scale
    }//alpha blending
}

//----------------------------------------------------------------------------
void vtkLookupTableWithEnabling::MapScalarsThroughTable2(void *input,
                                             unsigned char *output,
                                             int inputDataType,
                                             int numberOfValues,
                                             int inputIncrement,
                                             int outputFormat)
{
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
      vtkLookupTableWithEnablingMapData(this,
                            static_cast<unsigned char*>(newInput->GetPointer(0)),
                            output,numberOfValues,
                            inputIncrement,outputFormat);
      newInput->Delete();
      bitArray->Delete();
      }
      break;

    vtkTemplateMacro(
      vtkLookupTableWithEnablingMapData(this,static_cast<VTK_TT*>(input),output,
                            numberOfValues,inputIncrement,outputFormat)
      );
    default:
      vtkErrorMacro(<< "MapImageThroughTable: Unknown input ScalarType");
      return;
    }
}

//----------------------------------------------------------------------------
void vtkLookupTableWithEnabling::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "EnabledArray: ";
  if( this->EnabledArray )
    {
    this->EnabledArray->PrintSelf(os << "\n", indent.GetNextIndent());
    }
  else
    {
    // Should not happen
    os << "(none)\n";
    }
}
