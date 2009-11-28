/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumePicker.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkVolumePicker.h"
#include "vtkObjectFactory.h"

#include "vtkImageData.h"
#include "vtkVolume.h"
#include "vtkVolumeMapper.h"

vtkCxxRevisionMacro(vtkVolumePicker, "1.4");
vtkStandardNewMacro(vtkVolumePicker);

//----------------------------------------------------------------------------
vtkVolumePicker::vtkVolumePicker()
{
  this->PickCroppingPlanes = 0;
  this->CroppingPlaneId = -1;
}

//----------------------------------------------------------------------------
vtkVolumePicker::~vtkVolumePicker()
{
}

//----------------------------------------------------------------------------
void vtkVolumePicker::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "PickCroppingPlanes: "
     << (this->PickCroppingPlanes ? "On" : "Off") << "\n";

  os << indent << "CroppingPlaneId: " << this->CroppingPlaneId << "\n";
}

//----------------------------------------------------------------------------
void vtkVolumePicker::Initialize()
{
  this->CroppingPlaneId = -1;

  this->Superclass::Initialize();
}

//----------------------------------------------------------------------------
// Intersect a vtkVolume with a line by ray casting.
double vtkVolumePicker::IntersectVolumeWithLine(const double p1[3],
                                                const double p2[3],
                                                double t1, double t2,
                                                vtkVolume *volume, 
                                                vtkAbstractVolumeMapper *mapper)
{
  double tMin = VTK_DOUBLE_MAX;

  vtkImageData *data = vtkImageData::SafeDownCast(mapper->GetDataSetInput());
  vtkVolumeMapper *vmapper = vtkVolumeMapper::SafeDownCast(mapper);
  
  if (data == 0)
    {
    // This picker only works with image inputs
    return VTK_DOUBLE_MAX;
    }

  // Convert ray to structured coordinates
  double spacing[3], origin[3];
  int extent[6];
  data->GetSpacing(spacing);
  data->GetOrigin(origin);
  data->GetExtent(extent);

  double x1[3], x2[3];
  for (int i = 0; i < 3; i++)
    {
    x1[i] = (p1[i] - origin[i])/spacing[i];
    x2[i] = (p2[i] - origin[i])/spacing[i];
    }

  // Find the cropping bounds in structured coordinates
  int planeId = -1;
  double s1 = t1;
  double s2 = t2;
  double bounds[6];
  if (vmapper && vmapper->GetCropping())
    {
    // The only cropping mode suppported here is "subvolume".
    vmapper->GetCroppingRegionPlanes(bounds);
    for (int j = 0; j < 3; j++)
      {
      double b1 = (bounds[2*j] - origin[j])/spacing[j]; 
      double b2 = (bounds[2*j+1] - origin[j])/spacing[j]; 
      bounds[2*j] = (b1 < b2 ? b1 : b2);
      bounds[2*j+1] = (b1 < b2 ? b2 : b1);
      if (bounds[2*j] < extent[2*j]) { bounds[2*j] = extent[2*j]; }
      if (bounds[2*j+1] > extent[2*j+1]) { bounds[2*j+1] = extent[2*j+1]; }
      if (bounds[2*j] > bounds[2*j+1])
        {
        return VTK_DOUBLE_MAX;
        }
      }

    // Clip the ray with the volume cropping, results go in s1 and s2
    if (!this->ClipLineWithBounds(bounds, x1, x2, s1, s2, planeId))
      {
      return VTK_DOUBLE_MAX;
      }
    if (s1 >= t1) { t1 = s1; }
    if (s2 <= t2) { t2 = s2; }

    // Sanity check
    if (t2 < t1)
      {
      return VTK_DOUBLE_MAX;
      }
    }

  if (this->PickCroppingPlanes)
    {
    // Set data values at the intersected cropping or clipping plane
    if ((tMin = t1) < this->GlobalTMin)
      {
      for (int j = 0; j < 3; j++)
        {
        double x = x1[j]*(1.0 - tMin) + x2[j]*tMin;
        if (j == planeId/2)
          {
          x = bounds[planeId];
          }
        this->MapperPosition[j] = x*spacing[j] + origin[j];
        int xi = int(floor(x));
        this->CellIJK[j] = xi;
        this->PCoords[j] = x - xi;
        if (xi == extent[2*j+1])
          { // Avoid out-of-bounds cell
          this->CellIJK[j]--;
          this->PCoords[j] = 1.0;
          }
        this->PointIJK[j] = this->CellIJK[j];
        if (this->PCoords[j] >= 0.5)
          {
          this->PointIJK[j]++;
          }
        }
      this->PointId = data->ComputePointId(this->PointIJK);
      this->CellId = data->ComputeCellId(this->CellIJK);
      this->SubId = 0;
      }
    }
  else
    {
    tMin = this->Superclass::IntersectVolumeWithLine(
      p1, p2, t1, t2, volume, mapper);
    }

  if (tMin < this->GlobalTMin)
    {
    this->CroppingPlaneId = planeId;
    // If t1 is at a cropping plane, reset the normal to the plane normal
    if (planeId >= 0 && tMin == s1)
      {
      this->MapperNormal[0] = 0.0;
      this->MapperNormal[1] = 0.0;
      this->MapperNormal[2] = 0.0;
      this->MapperNormal[planeId/2] = 2.0*(planeId%2) - 1.0;
      if (spacing[planeId/2] < 0)
        {
        this->MapperNormal[planeId/2] = - this->MapperNormal[planeId/2];
        }
      }
    }

  return tMin;
}

