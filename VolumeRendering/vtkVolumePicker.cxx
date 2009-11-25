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

#include "vtkMath.h"
#include "vtkFastNumericConversion.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPlaneCollection.h"
#include "vtkTransform.h"
#include "vtkDoubleArray.h"
#include "vtkPoints.h"
#include "vtkPolygon.h"
#include "vtkVoxel.h"
#include "vtkGenericCell.h"
#include "vtkPointData.h"
#include "vtkImageData.h"
#include "vtkActor.h"
#include "vtkMapper.h"
#include "vtkProperty.h"
#include "vtkVolume.h"
#include "vtkAbstractVolumeMapper.h"
#include "vtkVolumeMapper.h"
#include "vtkVolumeProperty.h"
#include "vtkImageActor.h"

vtkCxxRevisionMacro(vtkVolumePicker, "1.2");
vtkStandardNewMacro(vtkVolumePicker);

//----------------------------------------------------------------------------
vtkVolumePicker::vtkVolumePicker()
{
  this->Cell = vtkGenericCell::New();
  this->Gradients = vtkDoubleArray::New();
  this->Gradients->SetNumberOfComponents(3);
  this->Gradients->SetNumberOfTuples(8);
 
  this->Tolerance = 1e-6;
  this->VolumeOpacityIsovalue = 0.01;
  this->PickClippingPlanes = 0;

  this->ClippingPlaneId = -1;
  this->CroppingPlaneId = -1;

  this->PointId = -1;
  this->CellId = -1;
  this->SubId = -1;

  this->PCoords[0] = 0.0;
  this->PCoords[1] = 0.0;
  this->PCoords[2] = 0.0;

  this->CellIJK[0] = 0.0;
  this->CellIJK[1] = 0.0;
  this->CellIJK[2] = 0.0;

  this->PointIJK[0] = 0.0;
  this->PointIJK[1] = 0.0;
  this->PointIJK[2] = 0.0;

  this->MapperNormal[0] = 0.0;
  this->MapperNormal[1] = 0.0;
  this->MapperNormal[2] = 1.0;

  this->PickNormal[0] = 0.0;
  this->PickNormal[1] = 0.0;
  this->PickNormal[2] = 1.0;
}

//----------------------------------------------------------------------------
vtkVolumePicker::~vtkVolumePicker()
{
  this->Gradients->Delete();
  this->Cell->Delete();
}

//----------------------------------------------------------------------------
void vtkVolumePicker::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "PointId: " << this->PointId << "\n";
  os << indent << "CellId: " << this->CellId << "\n";
  os << indent << "SubId: " << this->SubId << "\n";
  os << indent << "PCoords: (" << this->PCoords[0] << ", " 
     << this->PCoords[1] << ", " << this->PCoords[2] << ")\n";

  os << indent << "PointIJK: (" << this->PointIJK[0] << ", "
     << this->PointIJK[1] << ", " << this->PointIJK[2] << ")\n";
  os << indent << "CellIJK: (" << this->CellIJK[0] << ", "
     << this->CellIJK[1] << ", " << this->CellIJK[2] << ")\n";

  os << indent << "ClippingPlaneId: " << this->ClippingPlaneId << "\n";

  os << indent << "CroppingPlaneId: " << this->CroppingPlaneId << "\n";

  os << indent << "MapperNormal: (" <<  this->MapperNormal[0] << ","
     << this->MapperNormal[1] << "," << this->MapperNormal[2] << ")\n";
  
  os << indent << "PickNormal: (" <<  this->PickNormal[0] << ","
     << this->PickNormal[1] << "," << this->PickNormal[2] << ")\n";

  os << indent << "PickClippingPlanes: "
     << (this->PickClippingPlanes ? "On" : "Off") << "\n";

  os << indent << "VolumeOpacityIsovalue: " << this->VolumeOpacityIsovalue
     << "\n";
}
//----------------------------------------------------------------------------
void vtkVolumePicker::Initialize()
{
  this->ClippingPlaneId = -1;
  this->CroppingPlaneId = -1;

  this->PointId = -1;
  this->CellId = -1;
  this->SubId = -1;

  this->PCoords[0] = 0.0;
  this->PCoords[1] = 0.0;
  this->PCoords[2] = 0.0;

  this->CellIJK[0] = 0.0;
  this->CellIJK[1] = 0.0;
  this->CellIJK[2] = 0.0;

  this->PointIJK[0] = 0.0;
  this->PointIJK[1] = 0.0;
  this->PointIJK[2] = 0.0;

  this->MapperNormal[0] = 0.0;
  this->MapperNormal[1] = 0.0;
  this->MapperNormal[2] = 1.0;

  this->PickNormal[0] = 0.0;
  this->PickNormal[1] = 0.0;
  this->PickNormal[2] = 1.0;

  this->vtkPicker::Initialize();
}

//----------------------------------------------------------------------------
double vtkVolumePicker::IntersectWithLine(double p1[3], double p2[3],
                                          double tol, 
                                          vtkAssemblyPath *path, 
                                          vtkProp3D *prop, 
                                          vtkAbstractMapper3D *m)
{ 
  // This method will be called for vtkVolume and vtkActor but not
  // for vtkImageActor, since ImageActor has no mapper.
  vtkActor *actor = 0;
  vtkMapper *mapper = 0;
  vtkVolume *volume = 0;
  vtkAbstractVolumeMapper *volumeMapper = 0;
  vtkImageActor *imageActor = 0;
  vtkPlaneCollection *planes = 0;

  double tMin = VTK_DOUBLE_MAX;
  double t1 = 0.0;
  double t2 = 1.0;

  // Clip the ray with the mapper's ClippingPlanes.  This will
  // require the "t" values to be adjusted later.
  int clippingPlaneId = -1;
  if (m)
    {
    planes = m->GetClippingPlanes();
    if (planes && planes->GetNumberOfItems() > 0)
      {
      // This is a bit ugly: need to transform back to world coordinates
      double q1[3], q2[3];
      this->Transform->TransformPoint(p1, q1);
      this->Transform->TransformPoint(p2, q2);

      if (!this->ClipLineWithPlanes(planes, q1, q2, t1, t2, clippingPlaneId))
        {
        return VTK_DOUBLE_MAX;
        }
      else if (this->PickClippingPlanes && t1 < this->GlobalTMin)
        {
        // Do the pick on the planes, rather than on the data
        this->ClippingPlaneId = clippingPlaneId;
        this->MapperPosition[0] = p1[0]*(1.0-t1) + p2[0]*t1;
        this->MapperPosition[1] = p1[1]*(1.0-t1) + p2[1]*t1;
        this->MapperPosition[2] = p1[2]*(1.0-t1) + p2[2]*t1;
        planes->GetItem(clippingPlaneId)->GetNormal(this->PickNormal);
        // We want the "out" direction
        this->PickNormal[0] = -this->PickNormal[0];
        this->PickNormal[1] = -this->PickNormal[1];
        this->PickNormal[2] = -this->PickNormal[2];

        // This code is a little crazy: transforming a normal involves
        // matrix inversion and transposal, but since the normal
        // is to be transform from world -> mapper coords, only the
        // transpose is needed.
        vtkMatrix4x4 *matrix = vtkMatrix4x4::New();
        double hvec[4];
        hvec[0] = this->PickNormal[0];
        hvec[1] = this->PickNormal[1];
        hvec[2] = this->PickNormal[2];
        hvec[3] = 0.0;
        this->Transform->GetTranspose(matrix);
        matrix->MultiplyPoint(hvec, hvec);
        this->MapperNormal[0] = hvec[0];
        this->MapperNormal[1] = hvec[1];
        this->MapperNormal[2] = hvec[2];
        matrix->Delete();

        this->MarkPicked(path, prop, m, t1, this->MapperPosition);

        return t1;
        }
      }
    }

  // Actor
  if ( (mapper = vtkMapper::SafeDownCast(m)) &&
       (actor = vtkActor::SafeDownCast(prop)) )
    {
    tMin = this->IntersectActorWithLine(p1, p2, t1, t2, tol, actor, mapper);
    }

  // Volume
  else if ( (volumeMapper = vtkAbstractVolumeMapper::SafeDownCast(m)) &&
            (volume = vtkVolume::SafeDownCast(prop)) )
    {
    tMin = this->IntersectVolumeWithLine(p1, p2, t1, t2, clippingPlaneId,
                                         volume, volumeMapper);
    }

  // ImageActor
  else if ( (imageActor = vtkImageActor::SafeDownCast(prop)) )
    {
    tMin = this->IntersectImageActorWithLine(p1, p2, t1, t2, imageActor);
    }

  // Unidentified Prop3D
  else
    {
    return VTK_DOUBLE_MAX;
    }

  if (tMin < this->GlobalTMin)
    {
    this->ClippingPlaneId = clippingPlaneId;
    this->Transform->TransformNormal(this->MapperNormal, this->PickNormal);
    this->MarkPicked(path, prop, m, tMin, this->MapperPosition);
    }

  return tMin;
}

//----------------------------------------------------------------------------
double vtkVolumePicker::IntersectActorWithLine(const double p1[3],
                                               const double p2[3],
                                               double t1, double t2,
                                               double tol, 
                                               vtkActor *vtkNotUsed(actor), 
                                               vtkMapper *mapper)
{
  // This code was taken from CellPicker with almost no modification.
  // Intersect each cell with ray.  Keep track of one closest to
  // the eye (within the tolerance tol) and within the clipping range). 
  // Note that we fudge the "closest to" (tMin+this->Tolerance) a little and
  // keep track of the cell with the best pick based on parametric
  // coordinate (pick the minimum, maximum parametric distance). This 
  // breaks ties in a reasonable way when cells are the same distance 
  // from the eye (like cells lying on a 2D plane).

  vtkDataSet *data = mapper->GetInput();
  double tMin = VTK_DOUBLE_MAX;
  double minPCoords[3];
  double pDistMin = VTK_DOUBLE_MAX;
  vtkIdType minCellId = -1;
  int minSubId = -1;
  double minXYZ[3];
  minXYZ[0] = minXYZ[1] = minXYZ[2] = 0.0;

  vtkIdType numCells = data->GetNumberOfCells();
  for (vtkIdType cellId = 0; cellId < numCells; cellId++) 
    {
    double t;
    double x[3];
    double pcoords[3];
    pcoords[0] = pcoords[1] = pcoords[2] = 0;
    int subId = -1;

    data->GetCell(cellId, this->Cell);
    if (this->Cell->IntersectWithLine(const_cast<double *>(p1),
                                      const_cast<double *>(p2),
                                      tol, t, x, pcoords, subId) 
        && t <= (tMin + this->Tolerance) && t >= t1 && t <= t2)
      {
      double pDist = this->Cell->GetParametricDistance(pcoords);
      if (pDist < pDistMin || (pDist == pDistMin && t < tMin))
        {
        tMin = t;
        pDistMin = pDist;
        // save all of these
        minCellId = cellId;
        minSubId = subId;
        minXYZ[0] = x[0];
        minXYZ[1] = x[1];
        minXYZ[2] = x[2];
        minPCoords[0] = pcoords[0];
        minPCoords[1] = pcoords[1];
        minPCoords[2] = pcoords[2];
        } // if minimum, maximum
      } // if a close cell
    } // for all cells
  
  // Do this if a cell was intersected
  if (minCellId >= 0 && tMin < this->GlobalTMin)
    {
    // Don't call MarkPicked here like vtkCellPicker does,
    // that needs to be done at the very end.
    this->CellId = minCellId;
    this->SubId = minSubId;
    this->PCoords[0] = minPCoords[0];
    this->PCoords[1] = minPCoords[1];
    this->PCoords[2] = minPCoords[2];
    this->MapperPosition[0] = minXYZ[0];
    this->MapperPosition[1] = minXYZ[1];
    this->MapperPosition[2] = minXYZ[2];

    // Get the cell, convert to triangle if it is a strip
    vtkGenericCell *cell = this->Cell;
    data->GetCell(minCellId, cell);
    if (cell->GetCellType() == VTK_TRIANGLE_STRIP)
      {
      this->TriangleFromStrip(cell, minSubId);
      }

    // Use weights to find the closest point in the cell
    vtkIdType numPoints = cell->GetNumberOfPoints();
    double *weights = new double[numPoints];
    double maxWeight = VTK_DOUBLE_MIN;
    vtkIdType iMaxWeight = 0;
    cell->InterpolateFunctions(minPCoords, weights);
    for (vtkIdType i = 0; i < numPoints; i++)
      {
      if (weights[i] > maxWeight)
        {
        iMaxWeight = i;
        }
      }
    this->PointId = cell->PointIds->GetId(iMaxWeight);

    // Compute the normal
    if (!this->ComputeSurfaceNormal(data, cell, weights, this->MapperNormal))
      {
      // By default, the normal points back along view ray
      this->MapperNormal[0] = p1[0] - p2[0];
      this->MapperNormal[1] = p1[1] - p2[1];
      this->MapperNormal[2] = p1[2] - p2[2];
      vtkMath::Normalize(this->MapperNormal);
      }

    delete [] weights;
    }

  return tMin;
}

//----------------------------------------------------------------------------
// Intersect a vtkVolume with a line by ray casting.
double vtkVolumePicker::IntersectVolumeWithLine(const double p1[3],
                                                const double p2[3],
                                                double t1, double t2,
                                                int clippingPlaneId,
                                                vtkVolume *volume, 
                                                vtkAbstractVolumeMapper *mapper)
{
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
    }
  else
    {
    // Just use the extent as the crop region
    for (int j = 0; j < 6; j++)
      {
      bounds[j] = extent[j];
      }
    }

  // Clip the ray with the volume cropping, results go in s1 and s2
  int planeId;
  double s1, s2;
  if (!this->ClipLineWithBounds(bounds, x1, x2, s1, s2, planeId))
    {
    return VTK_DOUBLE_MAX;
    }

  // Check to see if the clipping planes are tighter than the crop
  int useCropPlane = 0;
  if (s1 >= t1)
    {
    useCropPlane = 1;
    t1 = s1;
    }
  if (s2 <= t2)
    {
    t2 = s2;
    }
  if (t2 < t1)
    {
    return VTK_DOUBLE_MAX;
    }

  // Get the theshold for the opacity
  double opacityThreshold = this->VolumeOpacityIsovalue;

  // Compute the number of steps, using a step size of 1
  int n = int(sqrt(vtkMath::Distance2BetweenPoints(x1, x2))*(t2 - t1) + 1);

  // Find out whether there are multiple components in the volume
  int numComponents = data->GetNumberOfScalarComponents();
  vtkVolumeProperty *property = volume->GetProperty();
  int independentComponents = property->GetIndependentComponents();
  int numIndependentComponents = 1;
  if (independentComponents)
    {
    numIndependentComponents = numComponents;
    }

  // Create a scalar array, it will be needed later
  vtkDataArray *scalars = vtkDataArray::CreateDataArray(data->GetScalarType());
  scalars->SetNumberOfComponents(numComponents);
  vtkIdType scalarArraySize = numComponents*data->GetNumberOfPoints();
  int scalarSize = data->GetScalarSize();
  void *scalarPtr = data->GetScalarPointer();

  // Go through each volume component separately
  double tMin = VTK_DOUBLE_MAX;
  for (int component = 0; component < numIndependentComponents; component++)
    { 
    vtkPiecewiseFunction *scalarOpacity =
      property->GetScalarOpacity(component);
    int disableGradientOpacity = 
      property->GetDisableGradientOpacity(component);
    vtkPiecewiseFunction *gradientOpacity = 0;
    if (!disableGradientOpacity)
      {
      gradientOpacity = property->GetGradientOpacity(component);
      }

    // This is the component used to compute the opacity
    int oComponent = component;
    if (!independentComponents)
      {
      oComponent = numComponents - 1;
      }

    // Make a new array, shifted to the desired component
    scalars->SetVoidArray(static_cast<void *>(static_cast<char *>(scalarPtr)
                                              + scalarSize*oComponent),
                          scalarArraySize, 1);

    // Do a ray cast with nearest-neighbor interpolation.  This code should
    // be changed to use linear interpolation instead, and should visit all
    // voxel faces along the ray in order to achieve maximum precision.
    double x[3];
    int xi[3];
    int imax = (( n > 1 ) ? (n - 1) : 1);
    for (int i = 0; i < n; i++)
      {
      // "f" is the current fractional distance between t1 and t2
      double f = i*1.0/imax;
      double t = t1*(1.0 - f) + t2*f;

      for (int j = 0; j < 3; j++)
        {
        // "t" is the fractional distance between endpoints x1 and x2
        x[j] = x1[j]*(1.0 - t) + x2[j]*t;

        // Paranoia bounds check
        if (x[j] < extent[2*j]) { x[j] = extent[2*j]; }
        else if (x[j] > extent[2*j + 1]) { x[j] = extent[2*j+1]; }

        // Round in order to do nearest-neighbor interpolation
        xi[j] = floor(x[j]); if (x[j] - xi[j] >= 0.5) { xi[j]++; }
        }

      double opacity = this->ComputeVolumeOpacity(xi, data, scalars,
                                                  scalarOpacity,
                                                  gradientOpacity);

      if (opacity > opacityThreshold)
        {
        if (t < tMin && t < this->GlobalTMin)
          {
          tMin = t;

          for (int j = 0; j < 3; j++)
            {
            this->MapperPosition[j] = x[j]*spacing[j] + origin[j];
            this->PointIJK[j] = xi[j];
            if (x[j] >= xi[j] && xi[j] != extent[2*j+1])
              {
              this->CellIJK[j] = xi[j];
              this->PCoords[j] = x[j] - xi[j];
              }
            else
              {
              this->CellIJK[j] = xi[j] - 1;
              this->PCoords[j] = x[j] - xi[j] + 1.0;
              }
            }
  
          this->PointId = data->ComputePointId(this->PointIJK);
          this->CellId = data->ComputeCellId(this->CellIJK);
          this->SubId = 0;

          // Default the normal to the view-plane normal.  This default
          // will be used if the gradient cannot be computed any other way.
          this->MapperNormal[0] = p1[0] - p2[0];
          this->MapperNormal[1] = p1[1] - p2[1];
          this->MapperNormal[2] = p1[2] - p2[2];
          vtkMath::Normalize(this->MapperNormal);

          // Check to see if this is the first step, which means that this
          // is the boundary of the volume.  If this is the case, use the
          // normal of whatever boundary this is: the extent boundary,
          // the crop boundary, or the clipping plane boundary.
          if (i == 0)
            {
            if (useCropPlane)
              {
              if (planeId >= 0)
                {
                this->MapperNormal[0] = 0.0;
                this->MapperNormal[1] = 0.0;
                this->MapperNormal[2] = 0.0;
                this->MapperNormal[planeId/2] = 2.0*(planeId%2) - 1.0;
                }
              }
            else if (clippingPlaneId >= 0)
              {
              vtkPlaneCollection *planes = mapper->GetClippingPlanes();
              vtkPlane *plane = planes->GetItem(clippingPlaneId);
              // normal is in world coords, so transform to mapper coords
              double hvec[4];
              plane->GetNormal(hvec);
              hvec[0] = -hvec[0];
              hvec[1] = -hvec[1];
              hvec[2] = -hvec[2];
              hvec[3] = 0.0;
              vtkMatrix4x4 *matrix = vtkMatrix4x4::New();
              this->Transform->GetTranspose(matrix);
              matrix->MultiplyPoint(hvec, hvec);
              matrix->Delete();
              this->MapperNormal[0] = hvec[0];
              this->MapperNormal[1] = hvec[1];
              this->MapperNormal[2] = hvec[2];
              }
            }
          else
            {
            // Set the normal from the direction of the gradient
            int *ci = this->CellIJK;
            double weights[8];
            vtkVoxel::InterpolationFunctions(this->PCoords, weights);
            data->GetVoxelGradient(ci[0], ci[1], ci[2], scalars,
                                   this->Gradients);
            double v[3]; v[0] = v[1] = v[2] = 0.0;
            for (int k = 0; k < 8; k++)
              {
              double *pg = this->Gradients->GetTuple(k);
              v[0] += pg[0]*weights[k]; 
              v[1] += pg[1]*weights[k]; 
              v[2] += pg[2]*weights[k]; 
              }

            double norm = vtkMath::Norm(v);
            if (norm > 0)
              {
              this->MapperNormal[0] = v[0]/norm;
              this->MapperNormal[1] = v[1]/norm;
              this->MapperNormal[2] = v[2]/norm;
              }
            }
          }
        break; // This break matches the opacity check
        }
      }
    }

  scalars->Delete();

  return tMin;
}

//----------------------------------------------------------------------------
double vtkVolumePicker::IntersectImageActorWithLine(const double p1[3],
                                                    const double p2[3],
                                                    double t1, double t2,
                                                    vtkImageActor *imageActor)
{
  // Convert ray to structured coordinates
  vtkImageData *data = imageActor->GetInput();
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

  // Clip the ray with the extent
  int planeId, displayExtent[6];
  double tMin, tMax;
  imageActor->GetDisplayExtent(displayExtent);
  if (!this->ClipLineWithExtent(displayExtent, x1, x2, tMin, tMax, planeId)
      || tMin < t1 || tMin > t2)
    {
    return VTK_DOUBLE_MAX;
    }

  if (tMin < this->GlobalTMin)
    {
    // Compute all the pick values
    for (int j = 0; j < 3; j++)
      {
      double xj = x1[j]*(1.0 - tMin) + x2[j]*tMin;
      // Avoid out-of-bounds due to roundoff error
      if (xj < displayExtent[2*j])
        {
        xj = displayExtent[2*j];
        } 
      else if (xj > displayExtent[2*j+1])
        {
        xj = displayExtent[2*j + 1];
        }
      this->MapperPosition[j] = origin[j] + xj*spacing[j];
      this->CellIJK[j] = floor(xj);
      this->PCoords[j] = xj - this->CellIJK[j];
      // Keep the cell in-bounds if it is on the edge
      if (this->CellIJK[j] == extent[2*j+1])
        {
        this->CellIJK[j] -= 1;
        this->PCoords[j] = 1.0;
        }
      this->PointIJK[j] = this->CellIJK[j] + (this->PCoords[j] >= 0.5);
      }

    this->PointId = data->ComputePointId(this->PointIJK);
    this->CellId = data->ComputeCellId(this->CellIJK);
    this->SubId = 0;

    // Set the normal in mapper coordinates
    this->MapperNormal[0] = 0.0;
    this->MapperNormal[1] = 0.0;
    this->MapperNormal[2] = 0.0;
    this->MapperNormal[planeId/2] = 2.0*(planeId%2) - 1.0;

    // Set the bounding plane id, take spacing sign into account
    if (spacing[planeId/2] < 0)
      {
      this->CroppingPlaneId = 2*(planeId/2) + (1-planeId%2);
      }
    else
      {
      this->CroppingPlaneId = planeId;
      }
    }

  return tMin;
}

//----------------------------------------------------------------------------
// Clip a line with a collection of clipping planes, or return zero if
// the line does not intersect the volume enclosed by the planes.
// The result of the clipping is retured in t1 and t2, which will have
// values between 0 and 1.  The index of the frontmost intersected plane is
// returned in planeId.

int vtkVolumePicker::ClipLineWithPlanes(vtkPlaneCollection *planes,
                                        const double p1[3], const double p2[3],
                                        double &t1, double &t2, int& planeId)
{
  // The minPlaneId is the index of the plane that t1 lies on
  planeId = -1;
  t1 = 0.0;
  t2 = 1.0;

  vtkCollectionSimpleIterator iter;
  planes->InitTraversal(iter);
  vtkPlane *plane;
  for (int i = 0; (plane = planes->GetNextPlane(iter)); i++)
    {
    // This uses EvaluateFunction instead of FunctionValue because,
    // like the mapper, we want to ignore any transform on the planes.
    double d1 = -plane->EvaluateFunction(const_cast<double *>(p1));
    double d2 = -plane->EvaluateFunction(const_cast<double *>(p2));

    // If both distances are positive, both points are outside
    if (d1 > 0 && d2 > 0)
      {
      return 0;
      }
    // If one of the distances is positive, the line crosses the plane
    else if (d1 > 0 || d2 > 0)
      {
      // Compute fractional distance "t" of the crossing between p1 & p2
      double t = 0.0;
      if (d1 != 0)
        {
        t = d1/(d1 - d2);
        }

      // If point p1 was clipped, adjust t1
      if (d1 > 0)
        {
        if (t >= t1)
          {
          t1 = t;
          planeId = i;
          }
        }
      // else point p2 was clipped, so adjust t2
      else
        {
        if (t <= t2)
          {
          t2 = t;
          } 
        }

      // If this happens, there's no line left
      if (t1 > t2)
        {
        return 0;
        }
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
// Clip a line in structured coordinates with an extent.  If the line
// does not intersect the extent, the return value will be zero.
// The fractional position of the new x1 with respect to the original line
// is returned in tMin, and the index of the frontmost intersected plane
// is returned in planeId.  The planes are ordered as follows:
// xmin, xmax, ymin, ymax, zmin, zmax.

int vtkVolumePicker::ClipLineWithExtent(const int extent[6],
                                        const double x1[3], const double x2[3],
                                        double &t1, double &t2, int &planeId)
{
  double bounds[6];
  bounds[0] = extent[0]; bounds[1] = extent[1]; bounds[2] = extent[2];
  bounds[3] = extent[3]; bounds[4] = extent[4]; bounds[5] = extent[5];

  return vtkVolumePicker::ClipLineWithBounds(bounds, x1, x2, t1, t2, planeId);
}
 
//----------------------------------------------------------------------------
// Clip a line defined by endpoints p1 and p2 by a bounding box aligned with
// the x, y and z axes.  If the line does not intersect the bounds, the
// return value will be zero.  The parametric positions of the new endpoints
// are returned in t1 and t2, and the index of the plane corresponding to t1
// is returned in tMin, and the index of the frontmost intersected plane
// is returned in planeId.  The planes are ordered as follows:
// xmin, xmax, ymin, ymax, zmin, zmax.

int vtkVolumePicker::ClipLineWithBounds(const double bounds[6],
                                        const double p1[3], const double p2[3],
                                        double &t1, double &t2, int &planeId)
{
  planeId = -1;
  t1 = 0.0;
  t2 = 1.0;

  for (int j = 0; j < 3; j++)
    {
    for (int k = 0; k < 2; k++)
      {
      // Compute distances of p1 and p2 from the plane along the plane normal
      double d1 = (bounds[2*j + k] - p1[j])*(1 - 2*k);
      double d2 = (bounds[2*j + k] - p2[j])*(1 - 2*k);

      // If both distances are positive, both points are outside
      if (d1 > 0 && d2 > 0)
        {
        return 0;
        }
      // If one of the distances is positive, the line crosses the plane
      else if (d1 > 0 || d2 > 0)
        {
        // Compute fractional distance "t" of the crossing between p1 & p2
        double t = 0.0;
        if (d1 != 0)
          {
          t = d1/(d1 - d2);
          }

        // If point p1 was clipped, adjust t1
        if (d1 > 0)
          {
          if (t >= t1)
            {
            t1 = t;
            planeId = 2*j + k;
            }
          }
        // else point p2 was clipped, so adjust t2
        else
          {
          if (t <= t2)
            {
            t2 = t;
            } 
          }

        // If this happens, there's no line left
        if (t1 > t2)
          {
          return 0;
          }
        }      
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
// Compute the cell normal either by interpolating the point normals,
// or by computing the plane normal for 2D cells.

int vtkVolumePicker::ComputeSurfaceNormal(vtkDataSet *data, vtkCell *cell,
                                          const double *weights,
                                          double normal[3])
{
  vtkDataArray *normals = data->GetPointData()->GetNormals();

  if (normals)
    {
    normal[0] = normal[1] = normal[2] = 0.0;
    double pointNormal[3];
    vtkIdType numPoints = cell->GetNumberOfPoints();
    for (vtkIdType k = 0; k < numPoints; k++)
      {
      normals->GetTuple(cell->PointIds->GetId(k), pointNormal);
      normal[0] += pointNormal[0]*weights[k];
      normal[1] += pointNormal[1]*weights[k];
      normal[2] += pointNormal[2]*weights[k];
      }
    vtkMath::Normalize(normal);
    }
  else if (cell->GetCellDimension() == 2)
    {
    vtkPolygon::ComputeNormal(cell->Points, normal);
    }
  else
    {
    return 0;
    }

  return 1;
}

//----------------------------------------------------------------------------
// Do an in-place replacement of a triangle strip with a single triangle.

void vtkVolumePicker::TriangleFromStrip(vtkGenericCell *cell, int subId)
{
  static int idx[2][3]={{0,1,2},{1,0,2}};
  int order = subId % 2;
  vtkIdType pointIds[3];
  double points[3][3];

  pointIds[0] = cell->PointIds->GetId(subId + idx[order][0]);
  pointIds[1] = cell->PointIds->GetId(subId + idx[order][1]);
  pointIds[2] = cell->PointIds->GetId(subId + idx[order][2]);

  cell->Points->GetPoint(subId + idx[order][0], points[0]);
  cell->Points->GetPoint(subId + idx[order][1], points[1]);
  cell->Points->GetPoint(subId + idx[order][2], points[2]);

  cell->SetCellTypeToTriangle();

  cell->PointIds->SetId(0, pointIds[0]);
  cell->PointIds->SetId(1, pointIds[1]);
  cell->PointIds->SetId(2, pointIds[2]);

  cell->Points->SetPoint(0, points[0]);
  cell->Points->SetPoint(1, points[1]);
  cell->Points->SetPoint(2, points[2]);
}

//----------------------------------------------------------------------------
// Given a structured position within the volume, and the point scalars,
// compute the local opacity of the volume.

double vtkVolumePicker::ComputeVolumeOpacity(
  const int xi[3], vtkImageData *data, vtkDataArray *scalars,
  vtkPiecewiseFunction *scalarOpacity, vtkPiecewiseFunction *gradientOpacity)
{
  double opacity = 1.0;

  // Sample the volume using the scalars
  vtkIdType ptId = data->ComputePointId(const_cast<int *>(xi));
  double val = scalars->GetComponent(ptId, 0);      
  int scalarType = data->GetScalarType();

  // Compute the ScalarOpacity
  if (scalarOpacity)
    {
    opacity *= scalarOpacity->GetValue(val);
    }
  else if (scalarType == VTK_FLOAT || scalarType == VTK_DOUBLE)
    {
    opacity *= val;
    }
  else
    {
    // Assume unsigned char
    opacity *= val/255.0;
    }

  // Compute gradient and GradientOpacity
  if (gradientOpacity)
    {
    double g[3];
    data->GetPointGradient(xi[0], xi[1], xi[2], scalars, g); 
    double grad = sqrt(g[0]*g[0] + g[1]*g[1] + g[2]*g[2]);
    opacity *= gradientOpacity->GetValue(grad);
    }

  return opacity;
}


