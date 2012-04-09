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

#include "vtkBox.h"
#include "vtkImageData.h"
#include "vtkVolume.h"
#include "vtkVolumeMapper.h"

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
void vtkVolumePicker::ResetPickInfo()
{
  this->Superclass::ResetPickInfo();

  this->CroppingPlaneId = -1;
}

//----------------------------------------------------------------------------
// Intersect a vtkVolume with a line by ray casting.  Compared to the
// same method in the superclass, this method will look for cropping planes.

double vtkVolumePicker::IntersectVolumeWithLine(const double p1[3],
                                                const double p2[3],
                                                double t1, double t2,
                                                vtkProp3D *prop,
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

  // These are set to the plane that the ray enters through
  int planeId = -1;
  int extentPlaneId = -1;

  // There might be multiple regions, depending on cropping flags
  int numSegments = 1;
  double t1List[16], t2List[16], s1List[16];
  int planeIdList[16];
  t1List[0] = t1;
  t2List[0] = t2;
  // s1 is the cropping plane intersection, initialize to large value
  double s1 = s1List[0] = VTK_DOUBLE_MAX;
  planeIdList[0] = -1;

  // Find the cropping bounds in structured coordinates
  double bounds[6];
  for (int j = 0; j < 6; j++)
    {
    bounds[j] = extent[j];
    }

  if (vmapper && vmapper->GetCropping())
    {
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

    // Get all of the line segments that intersect the visible blocks
    int flags = vmapper->GetCroppingRegionFlags();
    if (!this->ClipLineWithCroppingRegion(bounds, extent, flags, x1, x2,
                                          t1, t2, extentPlaneId, numSegments,
                                          t1List, t2List, s1List, planeIdList))
      {
      return VTK_DOUBLE_MAX;
      }
    }
  else
    {
    // If no cropping, then use volume bounds
    double s2;
    if (!this->ClipLineWithExtent(extent, x1, x2, s1, s2, extentPlaneId))
      {
      return VTK_DOUBLE_MAX;
      }
    s1List[0] = s1;
    t1List[0] = ( (s1 > t1) ? s1 : t1 );
    t2List[0] = ( (s2 < t2) ? s2 : t2 );
    }

  if (this->PickCroppingPlanes && vmapper && vmapper->GetCropping())
    {
    // Only require information about the first intersection
    s1 = s1List[0];
    if (s1 > t1)
      {
      planeId = planeIdList[0];
      }

    // Set data values at the intersected cropping or clipping plane
    if ((tMin = t1List[0]) < this->GlobalTMin)
      {
      this->ResetPickInfo();
      this->DataSet = data;
      this->Mapper = vmapper;

      double x[3];
      for (int j = 0; j < 3; j++)
        {
        x[j] = x1[j]*(1.0 - tMin) + x2[j]*tMin;
        if (planeId >= 0 && j == planeId/2)
          {
          x[j] = bounds[planeId];
          }
        else if (planeId < 0 && extentPlaneId >= 0 && j == extentPlaneId/2)
          {
          x[j] = extent[extentPlaneId];
          }
        this->MapperPosition[j] = x[j]*spacing[j] + origin[j];
        }

      this->SetImageDataPickInfo(x, extent);
      }
    }
  else
    {
    // Go through the segments in order, until a hit occurs
    for (int segment = 0; segment < numSegments; segment++)
      {
      if ((tMin = this->Superclass::IntersectVolumeWithLine(
           p1, p2, t1List[segment], t2List[segment], prop, mapper))
           < VTK_DOUBLE_MAX)
        {
        s1 = s1List[segment];
        // Keep the first planeId that was set at the first intersection
        // that occurred after t1
        if (planeId < 0 && s1 > t1)
          {
          planeId = planeIdList[segment];
          }
        break;
        }
      }
    }

  if (tMin < this->GlobalTMin)
    {
    this->CroppingPlaneId = planeId;
    // If t1 is at a cropping or extent plane, use the plane normal
    if (planeId < 0)
      {
      planeId = extentPlaneId;
      }
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

//----------------------------------------------------------------------------
// This method does several things.  Given the volume CroppingRegionPlanes
// stored in bounds (in structured coords), and the volume extent, it
// casts a ray through the 27 "blocks" that the volume has been divided into.
// Each "block" is turned on or off by a bit in "flags".  The result
// of the ray cast is a collection of line segments: the parametric
// start and end of each segment is stored in t1List and t2List respectively.
// If the segment starts at a cropping plane, the planeIdList will store
// the Id of that plane, otherwise planeIdList will store -1 for that segment.

int vtkVolumePicker::ClipLineWithCroppingRegion(
  const double bounds[6], const int extent[6], int flags,
  const double x1[3], const double x2[3], double t1, double t2,
  int &extentPlaneId, int &numSegments,
  double *t1List, double *t2List, double *s1List, int *planeIdList)
{
  extentPlaneId = -1;
  numSegments = 0;
  double s1, s2;

  // Start by clipping the line with the volume extent
  if (!vtkVolumePicker::ClipLineWithExtent(extent, x1, x2, s1, s2,
                                           extentPlaneId))
    {
    return 0;
    }

  if (s1 >= t1) { t1 = s1; }
  if (s2 <= t2) { t2 = s2; }

  if (t2 < t1)
    {
    return 0;
    }

  // Compute the coordinates that correspond to t1
  double x[3];
  for (int i = 0; i < 3; i++)
    {
    x[i] = x1[i]*(1.0 - t1) + x2[i]*t1;
    // Watch for out-of-bounds due to numerical roundoff
    if (x[i] < extent[2*i]) { x[i] = extent[2*i]; }
    if (x[i] > extent[2*i+1]) { x[i] = extent[2*i+1]; }
    }
  if (t1 == s1 && extentPlaneId >= 0)
    {
    // If right on the boundary, set position exactly
    x[extentPlaneId/2] = extent[extentPlaneId];
    }

  // Find out which block is hit first, store indices and bounds
  int xi[3];
  double blockBounds[6];
  for (int j = 0; j < 3; j++)
    {
    xi[j] = 0;
    blockBounds[2*j] = extent[2*j];
    blockBounds[2*j+1] = bounds[2*j];
    // Be particular about the ray direction
    if (x[j] > bounds[2*j] || (x[j] == bounds[2*j] && x1[j] < x2[j]))
      {
      xi[j] = 1;
      blockBounds[2*j] = bounds[2*j];
      blockBounds[2*j+1] = bounds[2*j+1];
      }
    if (x[j] > bounds[2*j+1] || (x[j] == bounds[2*j+1] && x1[j] < x2[j]))
      {
      xi[j] = 2;
      blockBounds[2*j] = bounds[2*j+1];
      blockBounds[2*j+1] = extent[2*j+1];
      }
    }

  // Loop through the blocks along the ray path
  int plane1 = -1;
  int plane2 = -1;
  for (;;)
    {
    if (!vtkBox::IntersectWithLine(blockBounds, x1, x2,
                                   s1, s2, 0, 0, plane1, plane2))
      {
      // This should never happen, but if it does, stop here
      break;
      }

    int blockId = xi[0] + xi[1]*3 + xi[2]*9;
    if ((flags >> blockId) & 1)
      {
      t1List[numSegments] = (t1 > s1 ? t1 : s1);
      t2List[numSegments] = (t2 < s2 ? t2 : s2);
      s1List[numSegments] = s1;
      planeIdList[numSegments] = -1;
      if (plane1 >= 0)
        {
        // Compute  plane1/2  and  plane1%2
        int k = (plane1 >> 1);
        int l = (plane1 & 1);

        // Need to know if the ray is entering the volume, i.e. whether
        // the adjacent block that the ray is coming from is "off", because
        // we can't define a clip plane unless it is off.
        static int blockInc[3] = {1, 3, 9};
        int noPlane = 1;

        if (xi[k] == 1)
          {
          noPlane = (flags >> (blockId + blockInc[k]*(2*l - 1)) & 1);
          if (!noPlane)
            {
            planeIdList[numSegments] = plane1;
            }
          }
        else if (xi[k] == 0)
          {
          noPlane = (flags >> (blockId + blockInc[k]) & 1);
          if (!noPlane && l == 1)
            {
            planeIdList[numSegments] = 2*k;
            }
          }
        else if (xi[k] == 2)
          {
          noPlane = (flags >> (blockId - blockInc[k]) & 1);
          if (!noPlane && l == 0)
            {
            planeIdList[numSegments] = 2*k + 1;
            }
          }
        }

      // Sanity check: allow no segments with negative length
      if (t1List[numSegments] <= t2List[numSegments])
        {
        if (numSegments > 0 && t1List[numSegments] == t2List[numSegments-1])
          {
          // Concatenate this segment with the previous one
          t2List[numSegments-1] = t2List[numSegments];
          }
        else
          {
          // Add this segment as a new segment
          numSegments++;
          }
        }
      }

    // If there is no exit plane, the ray terminated and the search is over
    if (plane2 < 0)
      {
      break;
      }

    // Use the exit plane to choose the next block
    int k = plane2 / 2;
    xi[k] += 2*(plane2 - 2*k) - 1;

    if (xi[k] == 0)
      {
      blockBounds[2*k] = extent[2*k];
      blockBounds[2*k+1] = bounds[2*k];
      }
    else if (xi[k] == 1)
      {
      blockBounds[2*k] = bounds[2*k];
      blockBounds[2*k+1] = bounds[2*k+1];
      }
    else if (xi[k] == 2)
      {
      blockBounds[2*k] = bounds[2*k+1];
      blockBounds[2*k+1] = extent[2*k+1];
      }
    else
      {
      // Exit, stage right
      break;
      }
    }

  return numSegments;
}

