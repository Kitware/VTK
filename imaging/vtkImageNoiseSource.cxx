/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageNoiseSource.cxx
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
#include <stdlib.h>
#include "vtkMath.h"
#include "vtkImageRegion.h"
#include "vtkImageCache.h"
#include "vtkImageNoiseSource.h"


//----------------------------------------------------------------------------
vtkImageNoiseSource::vtkImageNoiseSource()
{
  this->Minimum = 0.0;
  this->Maximum = 10.0;
  this->WholeExtent[0] = 0;  this->WholeExtent[1] = 255;
  this->WholeExtent[2] = 0;  this->WholeExtent[3] = 255;
  this->WholeExtent[4] = 0;  this->WholeExtent[5] = 0;
  this->WholeExtent[6] = 0;  this->WholeExtent[7] = 0;
  this->SetOutputScalarType(VTK_FLOAT);
  this->SetExecutionAxes(VTK_IMAGE_X_AXIS);
}


//----------------------------------------------------------------------------
void vtkImageNoiseSource::SetWholeExtent(int xMin, int xMax, 
					 int yMin, int yMax,
					 int zMin, int zMax, 
					 int tMin, int tMax)
{
  int modified = 0;
  
  if (this->WholeExtent[0] != xMin)
    {
    modified = 1;
    this->WholeExtent[0] = xMin ;
    }
  if (this->WholeExtent[1] != xMax)
    {
    modified = 1;
    this->WholeExtent[1] = xMax ;
    }
  if (this->WholeExtent[2] != yMin)
    {
    modified = 1;
    this->WholeExtent[2] = yMin ;
    }
  if (this->WholeExtent[3] != yMax)
    {
    modified = 1;
    this->WholeExtent[3] = yMax ;
    }
  if (this->WholeExtent[4] != zMin)
    {
    modified = 1;
    this->WholeExtent[4] = zMin ;
    }
  if (this->WholeExtent[5] != zMax)
    {
    modified = 1;
    this->WholeExtent[5] = zMax ;
    }
  if (this->WholeExtent[6] != tMin)
    {
    modified = 1;
    this->WholeExtent[6] = tMin ;
    }
  if (this->WholeExtent[7] != tMax)
    {
    modified = 1;
    this->WholeExtent[7] = tMax ;
    }
  if (modified)
    {
    this->Modified();
    }
}
//----------------------------------------------------------------------------
void vtkImageNoiseSource::UpdateImageInformation()
{
  this->CheckCache();
  this->Output->SetWholeExtent(this->WholeExtent);
}

void vtkImageNoiseSource::Execute(vtkImageRegion *region)
{
  int min, max;
  float *ptr;
  int idx, inc;
  
  if (region->GetScalarType() != VTK_FLOAT)
    {
    vtkErrorMacro("Execute: This source only outputs floats");
    }
  
  region->GetExtent(min, max);
  region->GetIncrements(inc);
  ptr = (float *)(region->GetScalarPointer());
  
  for (idx = min; idx <= max; ++idx)
    {
    *ptr = this->Minimum +
      (this->Maximum - this->Minimum) * vtkMath::Random();
    ptr += inc;
    }
}







