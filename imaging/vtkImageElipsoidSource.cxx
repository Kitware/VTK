/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageElipsoidSource.cxx
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
#include "vtkImageRegion.h"
#include "vtkImageCache.h"
#include "vtkImageElipsoidSource.h"

//----------------------------------------------------------------------------
vtkImageElipsoidSource::vtkImageElipsoidSource()
{
  this->WholeExtent[0] = 0;
  this->WholeExtent[1] = 255;
  this->WholeExtent[2] = 0;
  this->WholeExtent[3] = 255;
  this->WholeExtent[4] = 0;
  this->WholeExtent[5] = 0;
  this->WholeExtent[6] = 0;
  this->WholeExtent[7] = 0;
  this->Center[0] = 128.0;
  this->Center[1] = 128.0;
  this->Center[2] = 0.0;
  this->Center[3] = 0.0;
  this->Radius[0] = 70.0;
  this->Radius[1] = 70.0;
  this->Radius[2] = 70.0;
  this->Radius[3] = 70.0;
  this->InValue = 255.0;
  this->OutValue = 0.0;
  
  // This can be overridden.
  this->SetOutputScalarType(VTK_UNSIGNED_CHAR);

  // simplest execute method possible (but slow)
  this->NumberOfExecutionAxes = 0;
}

//----------------------------------------------------------------------------
vtkImageElipsoidSource::~vtkImageElipsoidSource()
{
}

//----------------------------------------------------------------------------
void vtkImageElipsoidSource::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "Center: (" << this->Center[0] << ", "
     << this->Center[1] << ", " << this->Center[2] << ", " 
     << this->Center[3] << ")\n";
  
  os << indent << "Radius: (" << this->Radius[0] << ", "
     << this->Radius[1] << ", " << this->Radius[2] << ", " 
     << this->Radius[3] << ")\n";
  
  os << indent << "InValue: " << this->InValue << "\n";
  os << indent << "OutValue: " << this->OutValue << "\n";

  vtkImageSource::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkImageElipsoidSource::SetWholeExtent(int dim, int *extent)
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
void vtkImageElipsoidSource::GetWholeExtent(int dim, int *extent)
{
  int idx;
  
  if (dim > 4)
    {
    vtkWarningMacro("SetWholeExtent: Too many axes");
    dim = 4;
    }
  
  for (idx = 0; idx < dim*2; ++idx)
    {
    extent[idx] = this->WholeExtent[idx];
    }
}

//----------------------------------------------------------------------------
void vtkImageElipsoidSource::SetCenter(int dim, float *center)
{
  int idx;
  
  if (dim > 4)
    {
    vtkWarningMacro("SetCenter: Too many axes");
    dim = 4;
    }
  
  for (idx = 0; idx < dim; ++idx)
    {
    if (this->Center[idx] != center[idx])
      {
      this->Center[idx] = center[idx];
      this->Modified();
      }
    }
}
//----------------------------------------------------------------------------
void vtkImageElipsoidSource::GetCenter(int dim, float *center)
{
  int idx;
  
  if (dim > 4)
    {
    vtkWarningMacro("SetCenter: Too many axes");
    dim = 4;
    }
  
  for (idx = 0; idx < dim; ++idx)
    {
    center[idx] = this->Center[idx];
    }
}


//----------------------------------------------------------------------------
void vtkImageElipsoidSource::SetRadius(int dim, float *radius)
{
  int idx;
  
  if (dim > 4)
    {
    vtkWarningMacro("SetRadius: Too many axes");
    dim = 4;
    }
  
  for (idx = 0; idx < dim; ++idx)
    {
    if (this->Radius[idx] != radius[idx])
      {
      this->Radius[idx] = radius[idx];
      this->Modified();
      }
    }
}
//----------------------------------------------------------------------------
void vtkImageElipsoidSource::GetRadius(int dim, float *radius)
{
  int idx;
  
  if (dim > 4)
    {
    vtkWarningMacro("SetRadius: Too many axes");
    dim = 4;
    }
  
  for (idx = 0; idx < dim; ++idx)
    {
    radius[idx] = this->Radius[idx];
    }
}


//----------------------------------------------------------------------------
void vtkImageElipsoidSource::UpdateImageInformation()
{
  this->CheckCache();
  this->Output->SetSpacing(1.0, 1.0, 1.0, 1.0);
  this->Output->SetWholeExtent(this->WholeExtent);
  this->Output->SetNumberOfScalarComponents(1);
}


//----------------------------------------------------------------------------
void vtkImageElipsoidSource::Execute(vtkImageRegion *region)
{
  float d2, temp;
  float val;
  int *extent;
  
  extent = region->GetExtent();
  temp = ((float)(extent[0]) - this->Center[0]) / this->Radius[0];
  d2 = temp * temp;
  temp = ((float)(extent[2]) - this->Center[1]) / this->Radius[1];
  d2 += temp * temp;
  temp = ((float)(extent[4]) - this->Center[2]) / this->Radius[2];
  d2 += temp * temp;
  temp = ((float)(extent[6]) - this->Center[3]) / this->Radius[3];
  d2 += temp * temp;
  
  if (d2 < 1.0)
    {
    val = this->InValue;
    }
  else
    {
    val = this->OutValue;
    }
  
  switch (region->GetScalarType())
    {
    case VTK_FLOAT:
      *((float *)(region->GetScalarPointer())) = val;
      break;
    case VTK_INT:
      *((int *)(region->GetScalarPointer())) = (int)val;
      break;
    case VTK_SHORT:
      *((short *)(region->GetScalarPointer())) = (short)val;
      break;
    case VTK_UNSIGNED_SHORT:
      *((unsigned short *)(region->GetScalarPointer())) = (unsigned short)val;
      break;
    case VTK_UNSIGNED_CHAR:
      *((unsigned char *)(region->GetScalarPointer())) = (unsigned char)val;
      break;
    default:
      vtkErrorMacro("Execute: Unknown output ScalarType");
    }
}

