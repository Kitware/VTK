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
  this->Origin[0] = -3.2;
  this->Origin[1] = -2.5;
  this->Origin[2] = 0.0;
  this->JuliaSet = 0;
}

//----------------------------------------------------------------------------
vtkImageMandelbrotSource::~vtkImageMandelbrotSource()
{
}

//----------------------------------------------------------------------------
void vtkImageMandelbrotSource::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageSource::PrintSelf(os,indent);

  os << indent << "Origin: (" << this->Origin[0] << ", "
     << this->Origin[1] << ", " << this->Origin[2] << ")\n";
  os << indent << "Spacing: " << this->Spacing << "\n";

  os << indent << "WholeExtent: (" << this->WholeExtent[0] << ", "
     << this->WholeExtent[1] << ", " << this->WholeExtent[2] << ", "
     << this->WholeExtent[3] << ", " << this->WholeExtent[4] << ", " 
     << this->WholeExtent[5] << ")\n";
  os << "MaximumNumberOfIterations: " << this->MaximumNumberOfIterations << endl;
  os << "JuliaSet: " << this->JuliaSet << endl;
}

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
  vtkImageData *output = this->GetOutput();

  output->SetSpacing(this->Spacing, this->Spacing, this->Spacing);
  output->SetWholeExtent(this->WholeExtent);
  output->SetOrigin(this->Origin[0], this->Origin[1], this->Origin[2]);
  output->SetNumberOfScalarComponents(1);
  output->SetScalarType(VTK_UNSIGNED_SHORT);
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
  if (x == 0.0 && y == 0.0 && z == 0.0)
    {
    return;
    }
  this->Modified();
  this->Origin[0] += this->Spacing * x;
  this->Origin[1] += this->Spacing * y;
  this->Origin[2] += this->Spacing * z;
}

//----------------------------------------------------------------------------
void vtkImageMandelbrotSource::Execute(vtkImageData *data)
{
  int *ext;
  unsigned short *ptr;
  int min0, max0;
  int idx0, idx1, idx2;
  int inc0, inc1, inc2;
  float *origin;
  double x, y, z;
  unsigned long count = 0;
  unsigned long target;
  
  ext = data->GetUpdateExtent();
  ptr = (unsigned short *)(data->GetScalarPointerForExtent(ext));
  origin = data->GetOrigin();

  vtkDebugMacro("Generating Extent: " << ext[0] << " -> " << ext[1] << ", "
          << ext[2] << " -> " << ext[3]);

  vtkDebugMacro("Generating bounds: " << origin[0] + ext[0]*this->Spacing << " -> "
          << origin[0] + ext[1]*this->Spacing << ", "
          << origin[1] + ext[2]*this->Spacing << " -> "
          << origin[1] + ext[3]*this->Spacing);
  // Get min and max of axis 0 because it is the innermost loop.
  min0 = ext[0];
  max0 = ext[1];
  data->GetContinuousIncrements(ext, inc0, inc1, inc2);

  target = (unsigned long)((ext[5]-ext[4]+1)*(ext[3]-ext[2]+1)/50.0);
  target++;

  for (idx2 = ext[4]; idx2 <= ext[5]; ++idx2)
    {
    z = (double)(origin[2]) + (double)(idx2)*(this->Spacing);
    for (idx1 = ext[2]; !this->AbortExecute && idx1 <= ext[3]; ++idx1)
      {
      if (!(count%target))
        {
        this->UpdateProgress(count/(50.0*target));
        }
      count++;
      y = (double)(origin[1]) + (double)(idx1)*(this->Spacing);
      for (idx0 = min0; idx0 <= max0; ++idx0)
	{
        x = (double)(origin[0]) + (double)(idx0)*this->Spacing;

        *ptr = this->EvaluateSet(x, y, z);

	++ptr;
	// inc0 is 0
	}
      ptr += inc1;
      }
    ptr += inc2;
    }
}



//----------------------------------------------------------------------------
unsigned short vtkImageMandelbrotSource::EvaluateSet(double x, double y, 
                                                     double z)
{
  unsigned short count = 0;
  double cReal, cImag, zReal, zImag;
  double zReal2, zImag2;

  if (this->JuliaSet)
    {
    zReal = x;
    zImag = y;
    cReal = -0.75;
    cImag = z;
    }
  else
    {
    zReal = z;
    zImag = 0.0;
    cReal = x;
    cImag = y;
  }

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







