/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageSinusoidSource.cxx
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
#include <math.h>
#include "vtkImageRegion.h"
#include "vtkImageCache.h"
#include "vtkImageSinusoidSource.h"

//----------------------------------------------------------------------------
vtkImageSinusoidSource::vtkImageSinusoidSource()
{
  this->Maximum = 10.0;
  this->Center[0] = 0.0;
  this->Center[1] = 0.0;
  this->Center[2] = 0.0;
  this->Center[3] = 0.0;

  this->WholeExtent[0] = 0;  this->WholeExtent[1] = 255;
  this->WholeExtent[2] = 0;  this->WholeExtent[3] = 255;
  this->WholeExtent[4] = 0;  this->WholeExtent[5] = 0;
  this->WholeExtent[6] = 0;  this->WholeExtent[7] = 0;
  this->StandardDeviation = 100.0;
  
  this->SetOutputScalarType(VTK_FLOAT);
  this->SetExecutionAxes(VTK_IMAGE_X_AXIS);
}


//----------------------------------------------------------------------------
void vtkImageSinusoidSource::SetWholeExtent(int dim, int *extent)
{
  int idx;
  
  if (dim > 4)
    {
    vtkWarningMacro("SetWholeExtent: Too many axes");
    dim = 4;
    }
  
  for (idx = 0; idx < dim*2; ++idx)
    {
    if (this->WholeExtent[idx] != extent[idx])
      {
      this->WholeExtent[idx] = extent[idx];
      this->Modified();
      }
    }
}

//----------------------------------------------------------------------------
void vtkImageSinusoidSource::UpdateImageInformation()
{
  this->CheckCache();
  this->Output->SetWholeExtent(this->WholeExtent);
}

//----------------------------------------------------------------------------
void vtkImageSinusoidSource::Execute(vtkImageRegion *region)
{
  int min, max;
  float *ptr;
  int idx, inc, extent[8];
  int idx2;
  float sum, temp, temp2;

  if (region->GetScalarType() != VTK_FLOAT)
    {
    vtkErrorMacro("Execute: This source only outputs floats");
    }

  region->GetExtent(4, extent);
  min = extent[0];
  max = extent[1];
  region->GetIncrements(inc);
  ptr = (float *)(region->GetScalarPointer());
  
  temp2 = 1.0 / (2.0 * this->StandardDeviation * this->StandardDeviation);
  for (idx = min; idx <= max; ++idx)
    {
    //
    sum = 0.0;
    extent[0] = idx;
    for (idx2 = 0; idx2 < 4; ++idx2)
      {
      temp = this->Center[idx2] - (float)(extent[idx2*2]);
      sum += temp * temp;
      }
    *ptr = this->Maximum * exp(-sum * temp2);
    ptr += inc;
    }
}







