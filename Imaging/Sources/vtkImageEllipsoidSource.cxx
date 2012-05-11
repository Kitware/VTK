/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageEllipsoidSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageEllipsoidSource.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include "vtkImageData.h"

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
  this->SetNumberOfInputPorts(0);
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
int vtkImageEllipsoidSource::RequestInformation (
  vtkInformation * vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed( inputVector ),
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  outInfo->Set(vtkDataObject::SPACING(), 1.0, 1.0, 1.0);
  outInfo->Set(vtkDataObject::ORIGIN(),  0.0, 0.0, 0.0);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
               this->WholeExtent, 6);
  vtkDataObject::SetPointDataActiveScalarInfo(outInfo, this->OutputScalarType, -1);
  return 1;
}

template <class T>
void vtkImageEllipsoidSourceExecute(vtkImageEllipsoidSource *self,
                                    vtkImageData *data, int ext[6], T *ptr)
{
  int min0, max0;
  int idx0, idx1, idx2;
  vtkIdType inc0, inc1, inc2;
  double s0, s1, s2, temp;
  T outVal, inVal;
  double *center, *radius;
  unsigned long count = 0;
  unsigned long target;

  outVal = static_cast<T>(self->GetOutValue());
  inVal = static_cast<T>(self->GetInValue());
  center = self->GetCenter();
  radius = self->GetRadius();

  min0 = ext[0];
  max0 = ext[1];
  data->GetContinuousIncrements(ext, inc0, inc1, inc2);

  target = static_cast<unsigned long>((ext[5]-ext[4]+1)*(ext[3]-ext[2]+1)/50.0);
  target++;

  for (idx2 = ext[4]; idx2 <= ext[5]; ++idx2)
    {
    // handle divide by zero
    if (radius[2] != 0.0)
      {
      temp = (static_cast<double>(idx2) - center[2]) / radius[2];
      }
    else
      {
      if (static_cast<double>(idx2) - center[2] == 0.0)
        {
        temp = 0.0;
        }
      else
        {
        temp = VTK_DOUBLE_MAX;
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
        temp = (static_cast<double>(idx1) - center[1]) / radius[1];
        }
      else
        {
        if (static_cast<double>(idx1) - center[1] == 0.0)
          {
          temp = 0.0;
          }
        else
          {
          temp = VTK_DOUBLE_MAX;
          }
        }

      s1 = temp * temp;
      for (idx0 = min0; idx0 <= max0; ++idx0)
        {
        // handle divide by zero
        if (radius[0] != 0.0)
          {
          temp = (static_cast<double>(idx0) - center[0]) / radius[0];
          }
        else
          {
          if (static_cast<double>(idx0) - center[0] == 0.0)
            {
            temp = 0.0;
            }
          else
            {
            temp = VTK_DOUBLE_MAX;
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
int vtkImageEllipsoidSource::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector ** vtkNotUsed( inputVector ),
  vtkInformationVector *outputVector)
{
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkImageData *data = vtkImageData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
  int extent[6];

  outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),extent);

  data->SetExtent(extent);
  data->AllocateScalars(outInfo);

  void *ptr;
  ptr = data->GetScalarPointerForExtent(extent);

  switch (data->GetScalarType())
    {
    vtkTemplateMacro(
      vtkImageEllipsoidSourceExecute(this, data, extent,
                                     static_cast<VTK_TT *>(ptr)));
    default:
      vtkErrorMacro("Execute: Unknown output ScalarType");
    }

  return 1;
}
