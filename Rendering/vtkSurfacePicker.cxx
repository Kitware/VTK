/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSurfacePicker.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSurfacePicker.h"
#include "vtkObjectFactory.h"

#include "vtkMath.h"
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
#include "vtkVolume.h"
#include "vtkAbstractVolumeMapper.h"
#include "vtkVolumeProperty.h"
#include "vtkImageActor.h"
#include "vtkRenderer.h"
#include "vtkCamera.h"
#include "vtkAbstractCellLocator.h"

vtkCxxRevisionMacro(vtkSurfacePicker, "1.8");
vtkStandardNewMacro(vtkSurfacePicker);

//----------------------------------------------------------------------------
vtkSurfacePicker::vtkSurfacePicker()
{
  this->Locators = vtkCollection::New();

  this->Cell = vtkGenericCell::New();
  this->Gradients = vtkDoubleArray::New();
  this->Gradients->SetNumberOfComponents(3);
  this->Gradients->SetNumberOfTuples(8);
 
  this->Tolerance = 1e-6;
  this->VolumeOpacityIsovalue = 0.05;
  this->IgnoreGradientOpacity = 1;
  this->PickClippingPlanes = 0;

  this->ClippingPlaneId = -1;

  this->PointId = -1;
  this->CellId = -1;
  this->SubId = -1;

  this->PCoords[0] = 0.0;
  this->PCoords[1] = 0.0;
  this->PCoords[2] = 0.0;

  this->CellIJK[0] = 0;
  this->CellIJK[1] = 0;
  this->CellIJK[2] = 0;

  this->PointIJK[0] = 0;
  this->PointIJK[1] = 0;
  this->PointIJK[2] = 0;

  this->MapperNormal[0] = 0.0;
  this->MapperNormal[1] = 0.0;
  this->MapperNormal[2] = 1.0;

  this->PickNormal[0] = 0.0;
  this->PickNormal[1] = 0.0;
  this->PickNormal[2] = 1.0;
}

//----------------------------------------------------------------------------
vtkSurfacePicker::~vtkSurfacePicker()
{
  this->Gradients->Delete();
  this->Cell->Delete();
  this->Locators->Delete();
}

//----------------------------------------------------------------------------
void vtkSurfacePicker::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "VolumeOpacityIsovalue: " << this->VolumeOpacityIsovalue
     << "\n";
  os << indent << "IgnoreGradientOpacity: "
     << (this->IgnoreGradientOpacity ? "On" : "Off") << "\n";

  os << indent << "MapperNormal: (" <<  this->MapperNormal[0] << ","
     << this->MapperNormal[1] << "," << this->MapperNormal[2] << ")\n";
  
  os << indent << "PickNormal: (" <<  this->PickNormal[0] << ","
     << this->PickNormal[1] << "," << this->PickNormal[2] << ")\n";

  os << indent << "PointId: " << this->PointId << "\n";
  os << indent << "CellId: " << this->CellId << "\n";
  os << indent << "SubId: " << this->SubId << "\n";
  os << indent << "PCoords: (" << this->PCoords[0] << ", " 
     << this->PCoords[1] << ", " << this->PCoords[2] << ")\n";

  os << indent << "PointIJK: (" << this->PointIJK[0] << ", "
     << this->PointIJK[1] << ", " << this->PointIJK[2] << ")\n";
  os << indent << "CellIJK: (" << this->CellIJK[0] << ", "
     << this->CellIJK[1] << ", " << this->CellIJK[2] << ")\n";

  os << indent << "PickClippingPlanes: "
     << (this->PickClippingPlanes ? "On" : "Off") << "\n";

  os << indent << "ClippingPlaneId: " << this->ClippingPlaneId << "\n";
}
//----------------------------------------------------------------------------
void vtkSurfacePicker::Initialize()
{
  this->ClippingPlaneId = -1;

  this->PointId = -1;
  this->CellId = -1;
  this->SubId = -1;

  this->PCoords[0] = 0.0;
  this->PCoords[1] = 0.0;
  this->PCoords[2] = 0.0;

  this->CellIJK[0] = 0;
  this->CellIJK[1] = 0;
  this->CellIJK[2] = 0;

  this->PointIJK[0] = 0;
  this->PointIJK[1] = 0;
  this->PointIJK[2] = 0;

  this->MapperNormal[0] = 0.0;
  this->MapperNormal[1] = 0.0;
  this->MapperNormal[2] = 1.0;

  this->PickNormal[0] = 0.0;
  this->PickNormal[1] = 0.0;
  this->PickNormal[2] = 1.0;

  this->Superclass::Initialize();
}

//----------------------------------------------------------------------------
void vtkSurfacePicker::AddLocator(vtkAbstractCellLocator *locator)
{
  if (!this->Locators->IsItemPresent(locator))
    {
    this->Locators->AddItem(locator);
    }
}

//----------------------------------------------------------------------------
void vtkSurfacePicker::RemoveLocator(vtkAbstractCellLocator *locator)
{
  this->Locators->RemoveItem(locator);
}

//----------------------------------------------------------------------------
void vtkSurfacePicker::RemoveAllLocators()
{
  this->Locators->RemoveAllItems();
}

//----------------------------------------------------------------------------
int vtkSurfacePicker::Pick(double selectionX, double selectionY,
                           double selectionZ, vtkRenderer *renderer)
{
  int pickResult = 0;

  if ( (pickResult = this->Superclass::Pick(selectionX, selectionY, selectionZ,
                                            renderer)) == 0)
    {
    // If no pick, set the PickNormal so that it points at the camera
    double cameraPos[3];
    renderer->GetActiveCamera()->GetPosition(cameraPos);
    this->PickNormal[0] = cameraPos[0] - this->PickPosition[0];
    this->PickNormal[1] = cameraPos[1] - this->PickPosition[1];
    this->PickNormal[2] = cameraPos[2] - this->PickPosition[2];
    vtkMath::Normalize(this->PickNormal);
    }

  return pickResult;
}  

//----------------------------------------------------------------------------
double vtkSurfacePicker::IntersectWithLine(double p1[3], double p2[3],
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

  // Clip the ray with the mapper's ClippingPlanes and adjust t1, t2.
  int clippingPlaneId = -1;
  if (m && (planes = m->GetClippingPlanes())
      && (planes->GetNumberOfItems() > 0))
    {
    // This is a bit ugly: need to transform back to world coordinates
    double q1[3], q2[3];
    this->Transform->TransformPoint(p1, q1);
    this->Transform->TransformPoint(p2, q2);

    if (!this->ClipLineWithPlanes(planes, q1, q2, t1, t2, clippingPlaneId))
      {
      return VTK_DOUBLE_MAX;
      }
    }

  // Set the pick to the frontmost clipping plane
  if (this->PickClippingPlanes && clippingPlaneId >= 0)
    {
    tMin = t1;
    }

  // Actor
  else if ( (mapper = vtkMapper::SafeDownCast(m)) &&
       (actor = vtkActor::SafeDownCast(prop)) )
    {
    tMin = this->IntersectActorWithLine(p1, p2, t1, t2, tol, actor, mapper);
    }

  // Volume
  else if ( (volumeMapper = vtkAbstractVolumeMapper::SafeDownCast(m)) &&
            (volume = vtkVolume::SafeDownCast(prop)) )
    {
    tMin = this->IntersectVolumeWithLine(p1, p2, t1, t2,
                                         volume, volumeMapper);

    if (clippingPlaneId >= 0 && tMin == t1)
      {
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
    // If tMin == t1, the pick didn't go past the first clipping plane,
    // so the position and normal will be set from the clipping plane.
    if (tMin == t1 && clippingPlaneId >= 0)
      {
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
      }
    else
      {
      // Use the normal generated from the mapper's input data
      this->Transform->TransformNormal(this->MapperNormal, this->PickNormal);
      }

    this->ClippingPlaneId = clippingPlaneId;
    this->MarkPicked(path, prop, m, t1, this->MapperPosition);
    }

  return tMin;
}

//----------------------------------------------------------------------------
double vtkSurfacePicker::IntersectActorWithLine(const double p1[3],
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

  vtkCollectionSimpleIterator iter;
  vtkAbstractCellLocator *locator = 0;
  this->Locators->InitTraversal(iter);
  while ( (locator = static_cast<vtkAbstractCellLocator *>(
           this->Locators->GetNextItemAsObject(iter))) )
    {
    if (locator->GetDataSet() == data)
      {
      break;
      }
    }

  if (locator)
    {
    if (!locator->IntersectWithLine(const_cast<double *>(p1),
                                    const_cast<double *>(p2),
                                    tol, tMin, minXYZ,
                                    minPCoords, minSubId, minCellId))
      {
      return VTK_DOUBLE_MAX;
      }
    }
  else
    {
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
    }
  
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
double vtkSurfacePicker::IntersectVolumeWithLine(const double p1[3],
                                                const double p2[3],
                                                double t1, double t2,
                                                vtkVolume *volume, 
                                                vtkAbstractVolumeMapper *mapper)
{
  vtkImageData *data = vtkImageData::SafeDownCast(mapper->GetDataSetInput());
  
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

  // Clip the ray with the extent, results go in s1 and s2
  int planeId;
  double s1, s2;
  if (!this->ClipLineWithExtent(extent, x1, x2, s1, s2, planeId))
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

  // Get the theshold for the opacity
  double opacityThreshold = this->VolumeOpacityIsovalue;

  // Compute the length of the line intersecting the volume
  double rayLength = sqrt(vtkMath::Distance2BetweenPoints(x1, x2))*(t2 - t1); 

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
    if (!disableGradientOpacity && !this->IgnoreGradientOpacity)
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

    // Do a ray cast with linear interpolation.
    double opacity = 0.0;
    double lastOpacity = 0.0;
    double lastT = t1;
    double x[3];
    double pcoords[3];
    int xi[3];

    // Ray cast loop
    double t = t1;
    while (t <= t2)
      {
      for (int j = 0; j < 3; j++)
        {
        // "t" is the fractional distance between endpoints x1 and x2
        x[j] = x1[j]*(1.0 - t) + x2[j]*t;

        // Paranoia bounds check
        if (x[j] < extent[2*j]) { x[j] = extent[2*j]; }
        else if (x[j] > extent[2*j + 1]) { x[j] = extent[2*j+1]; }

        xi[j] = int(floor(x[j]));
        pcoords[j] = x[j] - xi[j];
        }

      opacity = this->ComputeVolumeOpacity(xi, pcoords, data, scalars,
                                           scalarOpacity, gradientOpacity);

      // If the ray has crossed the isosurface, then terminate the loop
      if (opacity > opacityThreshold)
        {
        break;
        }

      lastT = t;
      lastOpacity = opacity;

      // Compute the next "t" value that crosses a voxel boundary
      t = 1.0;
      for (int k = 0; k < 3; k++)
        {
        if (fabs((x2[k] - x1[k])/rayLength) > 1e-6)
          {
          double lastX = x1[k]*(1.0 - lastT) + x2[k]*lastT;
          // Increment to next slice boundary along dimension "k"
          double nextX = ((x2[k] > x1[k]) ? floor(lastX)+1 : ceil(lastX)-1);
          // Compute the "t" value for this slice boundary
          double ttry = lastT + (nextX - lastX)/(x2[k] - x1[k]);
          if (ttry > lastT && ttry < t)
            {
            t = ttry;
            }
          }
        } 
      } // End of "while (t <= t2)"

    // If the ray hit the isosurface, compute the isosurface position
    if (opacity > opacityThreshold)
      {
      // Backtrack to the actual surface position unless this was first step
      if (t > t1)
        {
        double f = (opacityThreshold - lastOpacity)/(opacity - lastOpacity);
        t = lastT*(1.0 - f) + t*f;
        for (int j = 0; j < 3; j++)
          {
          x[j] = x1[j]*(1.0 - t) + x2[j]*t;
          if (x[j] < extent[2*j]) { x[j] = extent[2*j]; }
          else if (x[j] > extent[2*j + 1]) { x[j] = extent[2*j+1]; }
          xi[j] = int(floor(x[j]));
          pcoords[j] = x[j] - xi[j];
          }
        }

      // Check to see if this is the new global minimum
      if (t < tMin && t < this->GlobalTMin)
        {
        tMin = t;

        for (int j = 0; j < 3; j++)
          {
          this->MapperPosition[j] = x[j]*spacing[j] + origin[j];
          this->CellIJK[j] = xi[j];
          this->PCoords[j] = pcoords[j];
          // Make sure cell is within bounds
          if (xi[j] == extent[2*j + 1])
            {
            this->CellIJK[j] = xi[j] - 1;
            this->PCoords[j] = 1.0;
            }
          this->PointIJK[j] = this->CellIJK[j] + (this->PCoords[j] >= 0.5);
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
        // normal of the boundary.
        if (t == t1 && planeId >= 0 && xi[planeId/2] == extent[planeId])
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
        } // End of "if (opacity > opacityThreshold)"
      } // End of "if (t < tMin && t < this->GlobalTMin)"
    } // End of loop over volume components

  scalars->Delete();

  return tMin;
}

//----------------------------------------------------------------------------
double vtkSurfacePicker::IntersectImageActorWithLine(const double p1[3],
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
      this->CellIJK[j] = int(floor(xj));
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
    }

  return tMin;
}

//----------------------------------------------------------------------------
// Clip a line with a collection of clipping planes, or return zero if
// the line does not intersect the volume enclosed by the planes.
// The result of the clipping is retured in t1 and t2, which will have
// values between 0 and 1.  The index of the frontmost intersected plane is
// returned in planeId.

int vtkSurfacePicker::ClipLineWithPlanes(vtkPlaneCollection *planes,
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

int vtkSurfacePicker::ClipLineWithExtent(const int extent[6],
                                        const double x1[3], const double x2[3],
                                        double &t1, double &t2, int &planeId)
{
  double bounds[6];
  bounds[0] = extent[0]; bounds[1] = extent[1]; bounds[2] = extent[2];
  bounds[3] = extent[3]; bounds[4] = extent[4]; bounds[5] = extent[5];

  return vtkSurfacePicker::ClipLineWithBounds(bounds, x1, x2, t1, t2, planeId);
}
 
//----------------------------------------------------------------------------
// Clip a line defined by endpoints p1 and p2 by a bounding box aligned with
// the x, y and z axes.  If the line does not intersect the bounds, the
// return value will be zero.  The parametric positions of the new endpoints
// are returned in t1 and t2, and the index of the plane corresponding to t1
// is returned in tMin, and the index of the frontmost intersected plane
// is returned in planeId.  The planes are ordered as follows:
// xmin, xmax, ymin, ymax, zmin, zmax.

int vtkSurfacePicker::ClipLineWithBounds(const double bounds[6],
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

int vtkSurfacePicker::ComputeSurfaceNormal(vtkDataSet *data, vtkCell *cell,
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

void vtkSurfacePicker::TriangleFromStrip(vtkGenericCell *cell, int subId)
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

double vtkSurfacePicker::ComputeVolumeOpacity(
  const int xi[3], const double pcoords[3],
  vtkImageData *data, vtkDataArray *scalars,
  vtkPiecewiseFunction *scalarOpacity, vtkPiecewiseFunction *gradientOpacity)
{
  double opacity = 1.0;

  // Get interpolation weights from the pcoords
  double weights[8];
  vtkVoxel::InterpolationFunctions(const_cast<double *>(pcoords), weights);

  // Get the volume extent to avoid out-of-bounds
  int extent[6];
  data->GetExtent(extent);
  int scalarType = data->GetScalarType();

  // Compute the increments for the three directions, checking the bounds
  vtkIdType xInc = 1;
  vtkIdType yInc = extent[1] - extent[0] + 1;
  vtkIdType zInc = yInc*(extent[3] - extent[2] + 1);
  if (xi[0] == extent[1]) { xInc = 0; }
  if (xi[1] == extent[3]) { yInc = 0; }
  if (xi[2] == extent[5]) { zInc = 0; }

  // Use the increments and weights to interpolate the data
  vtkIdType ptId = data->ComputePointId(const_cast<int *>(xi));
  double val = 0.0;
  for (int j = 0; j < 8; j++)
    {
    vtkIdType ptInc = (j & 1)*xInc + ((j>>1) & 1)*yInc + ((j>>2) & 1)*zInc;
    val += weights[j]*scalars->GetComponent(ptId + ptInc, 0);
    }

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
    data->GetVoxelGradient(xi[0], xi[1], xi[2], scalars, this->Gradients);
    double v[3]; v[0] = v[1] = v[2] = 0.0;
    for (int k = 0; k < 8; k++)
      {
      double *pg = this->Gradients->GetTuple(k);
      v[0] += pg[0]*weights[k]; 
      v[1] += pg[1]*weights[k]; 
      v[2] += pg[2]*weights[k]; 
      }
    double grad = sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
    opacity *= gradientOpacity->GetValue(grad);
    }  

  return opacity;
}


