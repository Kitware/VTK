/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMandelbrotSource.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

Copyright (c) 1993-1995 Ken Martin, Will Schroeder,ill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

=========================================================================*/
#include "vtkImageData.h"
#include "vtkImageMandelbrotSource.h"

//----------------------------------------------------------------------------
vtkImageMandelbrotSource::vtkImageMandelbrotSource()
{
  this->MaximumNumberOfIterations = 100;
  this->WholeExtent[0] = 0;
  this->WholeExtent[1] = 500;
  this->WholeExtent[2] = 0;
  this->WholeExtent[3] = 500;
  this->WholeExtent[4] = 0;
  this->WholeExtent[5] = 0;
  this->Spacing = 0.005;
  this->OriginCX[0] = -3.2;
  this->OriginCX[1] = -2.5;
  this->OriginCX[2] = 0.0;
  this->OriginCX[3] = 0.0;

  this->ProjectionAxes[0] = 0;
  this->ProjectionAxes[1] = 1;
  this->ProjectionAxes[2] = 2;
}

//----------------------------------------------------------------------------
vtkImageMandelbrotSource::~vtkImageMandelbrotSource()
{
}

//----------------------------------------------------------------------------
void vtkImageMandelbrotSource::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageSource::PrintSelf(os,indent);

  os << indent << "OriginC: (" << this->OriginCX[0] << ", "
     << this->OriginCX[1] << ")\n";
  os << indent << "OriginX: (" << this->OriginCX[2] << ", "
     << this->OriginCX[3] << ")\n";
  os << indent << "Spacing: " << this->Spacing << "\n";

  os << indent << "WholeExtent: (" << this->WholeExtent[0] << ", "
     << this->WholeExtent[1] << ", " << this->WholeExtent[2] << ", "
     << this->WholeExtent[3] << ", " << this->WholeExtent[4] << ", " 
     << this->WholeExtent[5] << ")\n";
  os << "MaximumNumberOfIterations: " << this->MaximumNumberOfIterations << endl;

  os << indent << "ProjectionAxes: (" << this->ProjectionAxes[0] << ", "
     << this->ProjectionAxes[1] << this->ProjectionAxes[2] << ")\n";

}

//----------------------------------------------------------------------------
/*
void vtkImageMandelbrotSource::SetOriginCX(double cReal, double cImag, 
                                           double xReal, double xImag)
{
  int modified = 0;

  if (cReal != this->OriginCX[0])
    {
    this->OriginCX[0]  = cReal;
    modified = 1;
    }
  if (cImag != this->OriginCX[1])
    {
    this->OriginCX[1]  = cImag;
    modified = 1;
    }
  if (xReal != this->OriginCX[2])
    {
    this->OriginCX[2]  = xReal;
    modified = 1;
    }
  if (xImag != this->OriginCX[3])
    {
    this->OriginCX[3]  = xImag;
    modified = 1;
    }

  if (modified)
    {
    this->Modified();
    }
}
*/
//----------------------------------------------------------------------------
void vtkImageMandelbrotSource::SetWholeExtent(int extent[6])
{
  int idx, modified = 0;
  
  for (idx = 0; idx < 6; ++idx)
    {
    if (this->WholeExtent[idx] != extent[idx])
      {
      this->WholeExtent[idx] = extent[idx];
      this->Modified();
      }
    }
  if (modified)
    {
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkImageMandelbrotSource::SetWholeExtent(int minX, int maxX, 
					    int minY, int maxY,
					    int minZ, int maxZ)
{
  int extent[6];
  
  extent[0] = minX;  extent[1] = maxX;
  extent[2] = minY;  extent[3] = maxY;
  extent[4] = minZ;  extent[5] = maxZ;
  this->SetWholeExtent(extent);
}


//----------------------------------------------------------------------------
void vtkImageMandelbrotSource::GetWholeExtent(int extent[6])
{
  int idx;
  
  for (idx = 0; idx < 6; ++idx)
    {
    extent[idx] = this->WholeExtent[idx];
    }
}


//----------------------------------------------------------------------------
void vtkImageMandelbrotSource::ExecuteInformation()
{
  int idx, axis;
  float origin[3];
  vtkImageData *output = this->GetOutput();
  unsigned long mem;
  
  
  output->SetSpacing(this->Spacing, this->Spacing, this->Spacing);
  output->SetWholeExtent(this->WholeExtent);
  for (idx = 0; idx < 3; ++idx)
    {
    axis = this->ProjectionAxes[idx];
    if (axis >= 0 && axis < 4)
      {
      origin[idx] = this->OriginCX[axis];
      }
    else
      {
      vtkErrorMacro("Bad projection axis.");
      origin[idx] = 0.0;
      }
    }

  output->SetOrigin(origin);
  output->SetNumberOfScalarComponents(1);
  output->SetScalarType(VTK_FLOAT);
  
  // What if we are trying to process a VERY large 2D image?
  mem = output->GetScalarSize();
  mem = mem * (this->WholeExtent[1] - this->WholeExtent[0] + 1);
  mem = mem * (this->WholeExtent[3] - this->WholeExtent[2] + 1);
  mem = mem / 1000;
  mem = mem * (this->WholeExtent[5] - this->WholeExtent[4] + 1);
  if (mem < 1)
    {
    mem = 1;
    }
  
  output->SetEstimatedWholeMemorySize(mem);
}

//----------------------------------------------------------------------------
void vtkImageMandelbrotSource::Zoom(double factor)
{
  if (factor == 1.0)
    {
    return;
    }
  this->Modified();
  this->Spacing *= factor;
}

//----------------------------------------------------------------------------
void vtkImageMandelbrotSource::Pan(double x, double y, double z)
{
  int idx, axis;
  double pan[3];

  if (x == 0.0 && y == 0.0 && z == 0.0)
    {
    return;
    }

  this->Modified();
  pan[0]=x;    pan[1]=y;    pan[2]=z;
  for (idx = 0; idx < 3; ++idx)
    {
    axis = this->ProjectionAxes[idx];
    if (axis >= 0 && axis < 4)
      {
      this->OriginCX[axis] += this->Spacing * pan[idx];
      }
    }
}

//----------------------------------------------------------------------------
void vtkImageMandelbrotSource::CopyOriginAndSpacing(
                                            vtkImageMandelbrotSource *source)
{
  int idx;

  for (idx = 0; idx < 4; ++idx)
    {
    this->OriginCX[idx] = source->OriginCX[idx];
    }

  this->Spacing = source->Spacing;
  this->Modified();
}
//----------------------------------------------------------------------------
void vtkImageMandelbrotSource::Execute(vtkImageData *data)
{
  int *ext, a0, a1, a2;
  float *ptr;
  int min0, max0;
  int idx0, idx1, idx2;
  int inc0, inc1, inc2;
  double *origin;
  double p[4];
  unsigned long count = 0;
  unsigned long target;
  
  // Copy origin into pixel
  for (idx0 = 0; idx0 < 4; ++idx0)
    {
    p[idx0] = this->OriginCX[idx0];
    }

  ext = data->GetUpdateExtent();
  ptr = (float *)(data->GetScalarPointerForExtent(ext));

  vtkDebugMacro("Generating Extent: " << ext[0] << " -> " << ext[1] << ", "
          << ext[2] << " -> " << ext[3]);

  // Get min and max of axis 0 because it is the innermost loop.
  min0 = ext[0];
  max0 = ext[1];
  data->GetContinuousIncrements(ext, inc0, inc1, inc2);

  target = (unsigned long)((ext[5]-ext[4]+1)*(ext[3]-ext[2]+1)/50.0);
  target++;

  a0 = this->ProjectionAxes[0];
  a1 = this->ProjectionAxes[1];
  a2 = this->ProjectionAxes[2];
  origin = this->OriginCX;

  if (a0<0 || a1<0 || a2<0 || a0>3 || a1>3 || a2>3)
    {
    vtkErrorMacro("Bad projection axis");
    return;
    }
  for (idx2 = ext[4]; idx2 <= ext[5]; ++idx2)
    {
    p[a2] = (double)(origin[a2]) + (double)(idx2)*(this->Spacing);
    for (idx1 = ext[2]; !this->AbortExecute && idx1 <= ext[3]; ++idx1)
      {
      if (!(count%target))
        {
        this->UpdateProgress(count/(50.0*target));
        }
      count++;
      p[a1] = (double)(origin[a1]) + (double)(idx1)*(this->Spacing);
      for (idx0 = min0; idx0 <= max0; ++idx0)
	{
        p[a0] = (double)(origin[a0]) + (double)(idx0)*this->Spacing;

        *ptr = (float)(this->EvaluateSet(p));

	++ptr;
	// inc0 is 0
	}
      ptr += inc1;
      }
    ptr += inc2;
    }
}

//----------------------------------------------------------------------------
// This method selectively supersamples pixels closet to the mandelbrot set.
/*
void vtkImageMandelbrotSource::SuperSample(vtkImageData *data)
{
  int *ext;
  float *ptr0, *ptr1, *ptr2;
  int min0, max0;
  int idx0, idx1, idx2;
  int inc0, inc1, inc2;

  // Get min and max of axis 0 because it is the innermost loop.
  ext = data->GetUpdateExtent();
  min0 = ext[0];
  max0 = ext[1];
  data->GetIncrements(inc0, inc1, inc2);

  // loop through pixels ignoring the borders.
  ptr2 = (float *)(data->GetScalarPointer(min0+1, ext[2]+1, ext[4]+1));

  for (idx2 = ext[4]+1; idx2 < ext[5]; ++idx2)
    {
    ptr1 = ptr2;
    for (idx1 = ext[2]+1; idx1 < ext[3]; ++idx1)
      {
      ptr0 = ptr1;
      for (idx0 = min0+1; idx0 < max0; ++idx0)
	{
        *ptr0 = (float*)(this->EvaluateSet(p));

	++ptr0;
	// inc0 is 1
	}
      ptr1 += inc1;
      }
    ptr2 += inc2;
    }
}
*/


//----------------------------------------------------------------------------
unsigned short vtkImageMandelbrotSource::EvaluateSet(double p[4])
{
  unsigned short count = 0;
  double cReal, cImag, zReal, zImag;
  double zReal2, zImag2;

  cReal = p[0];
  cImag = p[1];
  zReal = p[2];
  zImag = p[3];

  zReal2 = zReal * zReal;
  zImag2 = zImag * zImag;
  while ((zReal2 + zImag2) < 4.0 && count < this->MaximumNumberOfIterations)
    {
    zImag = 2.0 * zReal * zImag + cImag;
    zReal = zReal2 - zImag2 + cReal;
    zReal2 = zReal * zReal;
    zImag2 = zImag * zImag;
    ++count;
    }

  return count;
}







