/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageEllipsoidSource.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageEllipsoidSource.h"
#include "vtkObjectFactory.h"

#include "vtkImageData.h"

vtkCxxRevisionMacro(vtkImageEllipsoidSource, "1.25");
vtkStandardNewMacro(vtkImageEllipsoidSource);

//----------------------------------------------------------------------------
vtkImageEllipsoidSource::vtkImageEllipsoidSource()
{
  this->WholeExtent[0] = 0;
  this->WholeExtent[1] = 255;
  this->WholeExtent[2] = 0;
  this->WholeExtent[3] = 255;
  this->WholeExtent[4] = 0;
  this->WholeExtent[5] = 0;
  this->Center[0] = 128.0;
  this->Center[1] = 128.0;
  this->Center[2] = 0.0;
  this->Radius[0] = 70.0;
  this->Radius[1] = 70.0;
  this->Radius[2] = 70.0;
  this->InValue = 255.0;
  this->OutValue = 0.0;
  
  this->OutputScalarType = VTK_UNSIGNED_CHAR;
}

//----------------------------------------------------------------------------
vtkImageEllipsoidSource::~vtkImageEllipsoidSource()
{
}

//----------------------------------------------------------------------------
void vtkImageEllipsoidSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Center: (" << this->Center[0] << ", "
     << this->Center[1] << ", " << this->Center[2] << ")\n";
  
  os << indent << "Radius: (" << this->Radius[0] << ", "
     << this->Radius[1] << ", " << this->Radius[2] << ")\n";
  
  os << indent << "InValue: " << this->InValue << "\n";
  os << indent << "OutValue: " << this->OutValue << "\n";
  os << indent << "OutputScalarType: " << this->OutputScalarType << "\n";
}
//----------------------------------------------------------------------------
void vtkImageEllipsoidSource::SetWholeExtent(int extent[6])
{
  int idx;
  
  for (idx = 0; idx < 6; ++idx)
    {
    if (this->WholeExtent[idx] != extent[idx])
      {
      this->WholeExtent[idx] = extent[idx];
      this->Modified();
      }
    }
}

//----------------------------------------------------------------------------
void vtkImageEllipsoidSource::SetWholeExtent(int minX, int maxX, 
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
void vtkImageEllipsoidSource::GetWholeExtent(int extent[6])
{
  int idx;
  
  for (idx = 0; idx < 6; ++idx)
    {
    extent[idx] = this->WholeExtent[idx];
    }
}

//----------------------------------------------------------------------------
void vtkImageEllipsoidSource::ExecuteInformation()
{
  vtkImageData *data = this->GetOutput();
  
  data->SetSpacing(1.0, 1.0, 1.0);
  data->SetWholeExtent(this->WholeExtent);
  data->SetNumberOfScalarComponents(1);
  data->SetScalarType(this->OutputScalarType);
}





template <class T>
void vtkImageEllipsoidSourceExecute(vtkImageEllipsoidSource *self,
                                    vtkImageData *data, int ext[6], T *ptr)
{
  int min0, max0;
  int idx0, idx1, idx2;
  int inc0, inc1, inc2;
  double s0, s1, s2, temp;
  T outVal, inVal;
  float *center, *radius;
  unsigned long count = 0;
  unsigned long target;

  outVal = (T)(self->GetOutValue());
  inVal = (T)(self->GetInValue());
  center = self->GetCenter();
  radius = self->GetRadius();

  min0 = ext[0];
  max0 = ext[1];
  data->GetContinuousIncrements(ext, inc0, inc1, inc2);

  target = (unsigned long)((ext[5]-ext[4]+1)*(ext[3]-ext[2]+1)/50.0);
  target++;

  for (idx2 = ext[4]; idx2 <= ext[5]; ++idx2)
    {
    // handle divide by zero
    if (radius[2] != 0.0)
      {
      temp = ((double)idx2 - (double)(center[2])) / (double)(radius[2]);
      }
    else
      {
      if ((float)idx2 - center[2] == 0.0)
        {
        temp = 0.0;
        }
      else
        {
        temp = VTK_LARGE_FLOAT;
        }  
      }
    
    
    s2 = temp * temp;
    for (idx1 = ext[2]; !self->AbortExecute && idx1 <= ext[3]; ++idx1)
      {
      if (!(count%target))
        {
        self->UpdateProgress(count/(50.0*target));
        }
      count++;
      
      // handle divide by zero
      if (radius[1] != 0.0)
        {
        temp = ((double)idx1 - (double)(center[1])) / (double)(radius[1]);
        }
      else
        {
        if ((float)idx1 - center[1] == 0.0)
          {
          temp = 0.0;
          }
        else
          {
          temp = VTK_LARGE_FLOAT;
          }  
        }
      
      s1 = temp * temp;
      for (idx0 = min0; idx0 <= max0; ++idx0)
        {
        // handle divide by zero
        if (radius[0] != 0.0)
          {
          temp = ((double)idx0 - (double)(center[0])) / (double)(radius[0]);
          }
        else
          {
          if ((float)idx0 - center[0] == 0.0)
            {
            temp = 0.0;
            }
          else
            {
            temp = VTK_LARGE_FLOAT;
            }  
          }

        s0 = temp * temp;
        if (s0 + s1 + s2 > 1.0)
          {
          *ptr = outVal;
          }
        else
          {
          *ptr = inVal;
          }
        ++ptr;
        // inc0 is 0
        }
      ptr += inc1;
      }
    ptr += inc2;
    }
}

//----------------------------------------------------------------------------
void vtkImageEllipsoidSource::ExecuteData(vtkDataObject *output)
{
  int *extent;
  void *ptr;
  
  vtkImageData *data = this->AllocateOutputData(output);
  
  extent = this->GetOutput()->GetUpdateExtent();
  ptr = data->GetScalarPointerForExtent(extent);
  
  switch (data->GetScalarType())
    {
    vtkTemplateMacro4(vtkImageEllipsoidSourceExecute, this, data, 
                      extent, (VTK_TT *)ptr);
    default:
      vtkErrorMacro("Execute: Unknown output ScalarType");
    }
}







