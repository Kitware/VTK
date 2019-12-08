/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellPicker.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCellPicker.h"
#include "vtkObjectFactory.h"

#include "vtkAbstractCellLocator.h"
#include "vtkAbstractHyperTreeGridMapper.h"
#include "vtkAbstractVolumeMapper.h"
#include "vtkActor.h"
#include "vtkBitArray.h"
#include "vtkBox.h"
#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkCompositeDataSet.h"
#include "vtkCompositeDataSetRange.h"
#include "vtkDoubleArray.h"
#include "vtkGenericCell.h"
#include "vtkHyperTreeGridNonOrientedGeometryCursor.h"
#include "vtkImageData.h"
#include "vtkImageMapper3D.h"
#include "vtkLODProp3D.h"
#include "vtkMapper.h"
#include "vtkMath.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPlaneCollection.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolygon.h"
#include "vtkRenderer.h"
#include "vtkTexture.h"
#include "vtkTransform.h"
#include "vtkUniformHyperTreeGrid.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"
#include "vtkVoxel.h"

#include <algorithm>

vtkStandardNewMacro(vtkCellPicker);

//----------------------------------------------------------------------------
vtkCellPicker::vtkCellPicker()
{
  // List of locators for accelerating polydata picking
  this->Locators = vtkCollection::New();

  // For polydata picking
  this->Cell = vtkGenericCell::New();
  this->PointIds = vtkIdList::New();

  // For interpolation of volume gradients
  this->Gradients = vtkDoubleArray::New();
  this->Gradients->SetNumberOfComponents(3);
  this->Gradients->SetNumberOfTuples(8);

  // Miscellaneous ivars
  this->Tolerance = 1e-6;
  this->VolumeOpacityIsovalue = 0.05;
  this->UseVolumeGradientOpacity = 0;
  this->PickClippingPlanes = 0;
  this->PickTextureData = 0;

  // Clear all info returned by the pick
  this->ResetCellPickerInfo();
}

//----------------------------------------------------------------------------
vtkCellPicker::~vtkCellPicker()
{
  this->Gradients->Delete();
  this->Cell->Delete();
  this->PointIds->Delete();
  this->Locators->Delete();
}

//----------------------------------------------------------------------------
void vtkCellPicker::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "MapperNormal: (" << this->MapperNormal[0] << "," << this->MapperNormal[1] << ","
     << this->MapperNormal[2] << ")\n";

  os << indent << "PickNormal: (" << this->PickNormal[0] << "," << this->PickNormal[1] << ","
     << this->PickNormal[2] << ")\n";

  if (this->Texture)
  {
    os << indent << "Texture: " << this->Texture << "\n";
  }
  else
  {
    os << indent << "Texture: (none)";
  }

  os << indent << "PickTextureData: " << (this->PickTextureData ? "On" : "Off") << "\n";

  os << indent << "PointId: " << this->PointId << "\n";
  os << indent << "CellId: " << this->CellId << "\n";
  os << indent << "SubId: " << this->SubId << "\n";
  os << indent << "PCoords: (" << this->PCoords[0] << ", " << this->PCoords[1] << ", "
     << this->PCoords[2] << ")\n";

  os << indent << "PointIJK: (" << this->PointIJK[0] << ", " << this->PointIJK[1] << ", "
     << this->PointIJK[2] << ")\n";
  os << indent << "CellIJK: (" << this->CellIJK[0] << ", " << this->CellIJK[1] << ", "
     << this->CellIJK[2] << ")\n";

  os << indent << "ClippingPlaneId: " << this->ClippingPlaneId << "\n";

  os << indent << "PickClippingPlanes: " << (this->PickClippingPlanes ? "On" : "Off") << "\n";

  os << indent << "VolumeOpacityIsovalue: " << this->VolumeOpacityIsovalue << "\n";
  os << indent << "UseVolumeGradientOpacity: " << (this->UseVolumeGradientOpacity ? "On" : "Off")
     << "\n";
}

//----------------------------------------------------------------------------
void vtkCellPicker::Initialize()
{
  this->ResetPickInfo();
  this->Superclass::Initialize();
}

//----------------------------------------------------------------------------
void vtkCellPicker::ResetPickInfo()
{
  // First, reset information from the superclass, since
  // vtkPicker does not have a ResetPickInfo method
  this->DataSet = nullptr;
  this->Mapper = nullptr;
  this->CompositeDataSet = nullptr;
  this->FlatBlockIndex = -1;

  // Reset all the information specific to this class
  this->ResetCellPickerInfo();
}

//----------------------------------------------------------------------------
void vtkCellPicker::ResetCellPickerInfo()
{
  this->Texture = nullptr;

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
void vtkCellPicker::AddLocator(vtkAbstractCellLocator* locator)
{
  if (!this->Locators->IsItemPresent(locator))
  {
    this->Locators->AddItem(locator);
  }
}

//----------------------------------------------------------------------------
void vtkCellPicker::RemoveLocator(vtkAbstractCellLocator* locator)
{
  this->Locators->RemoveItem(locator);
}

//----------------------------------------------------------------------------
void vtkCellPicker::RemoveAllLocators()
{
  this->Locators->RemoveAllItems();
}

//----------------------------------------------------------------------------
int vtkCellPicker::Pick(
  double selectionX, double selectionY, double selectionZ, vtkRenderer* renderer)
{
  int pickResult = 0;

  if ((pickResult = this->Superclass::Pick(selectionX, selectionY, selectionZ, renderer)) == 0)
  {
    // If no pick, set the PickNormal so that it points at the camera
    vtkCamera* camera = renderer->GetActiveCamera();
    double cameraPos[3];
    camera->GetPosition(cameraPos);

    if (camera->GetParallelProjection())
    {
      // For parallel projection, use -ve direction of projection
      double cameraFocus[3];
      camera->GetFocalPoint(cameraFocus);
      this->PickNormal[0] = cameraPos[0] - cameraFocus[0];
      this->PickNormal[1] = cameraPos[1] - cameraFocus[1];
      this->PickNormal[2] = cameraPos[2] - cameraFocus[2];
    }
    else
    {
      // Get the vector from pick position to the camera
      this->PickNormal[0] = cameraPos[0] - this->PickPosition[0];
      this->PickNormal[1] = cameraPos[1] - this->PickPosition[1];
      this->PickNormal[2] = cameraPos[2] - this->PickPosition[2];
    }

    vtkMath::Normalize(this->PickNormal);
  }

  return pickResult;
}

//----------------------------------------------------------------------------
int vtkCellPicker::Pick3DRay(double pos[3], double orient[4], vtkRenderer* renderer)
{
  int pickResult = 0;

  if ((pickResult = this->Superclass::Pick3DRay(pos, orient, renderer)) == 0)
  {
    // If no pick, set the PickNormal so that it points at the camera
    vtkCamera* camera = renderer->GetActiveCamera();
    double cameraPos[3];
    camera->GetPosition(cameraPos);

    if (camera->GetParallelProjection())
    {
      // For parallel projection, use -ve direction of projection
      double cameraFocus[3];
      camera->GetFocalPoint(cameraFocus);
      this->PickNormal[0] = cameraPos[0] - cameraFocus[0];
      this->PickNormal[1] = cameraPos[1] - cameraFocus[1];
      this->PickNormal[2] = cameraPos[2] - cameraFocus[2];
    }
    else
    {
      // Get the vector from pick position to the camera
      this->PickNormal[0] = cameraPos[0] - this->PickPosition[0];
      this->PickNormal[1] = cameraPos[1] - this->PickPosition[1];
      this->PickNormal[2] = cameraPos[2] - this->PickPosition[2];
    }

    vtkMath::Normalize(this->PickNormal);
  }

  return pickResult;
}

//----------------------------------------------------------------------------
// Tolerance for parametric coordinate matching an intersection with a plane
#define VTKCELLPICKER_PLANE_TOL 1e-14

double vtkCellPicker::IntersectWithLine(const double p1[3], const double p2[3], double tol,
  vtkAssemblyPath* path, vtkProp3D* prop, vtkAbstractMapper3D* m)
{
  vtkMapper* mapper = nullptr;
  vtkAbstractVolumeMapper* volumeMapper = nullptr;
  vtkImageMapper3D* imageMapper = nullptr;
  vtkAbstractHyperTreeGridMapper* htgMapper = nullptr;

  double tMin = VTK_DOUBLE_MAX;
  double t1 = 0.0;
  double t2 = 1.0;

  // Clip the ray with the mapper's ClippingPlanes and adjust t1, t2.
  // This limits the pick search to the inside of the clipped region.
  int clippingPlaneId = -1;
  if (m &&
    !this->ClipLineWithPlanes(m, this->Transform->GetMatrix(), p1, p2, t1, t2, clippingPlaneId))
  {
    return VTK_DOUBLE_MAX;
  }

  // Initialize the pick position to the frontmost clipping plane
  if (this->PickClippingPlanes && clippingPlaneId >= 0)
  {
    tMin = t1;
  }

  // HyperTreeGrid
  else if ((htgMapper = vtkAbstractHyperTreeGridMapper::SafeDownCast(m)))
  {
    tMin = this->IntersectHyperTreeGridWithLine(p1, p2, t1, t2, htgMapper);
  }

  // Volume
  else if ((volumeMapper = vtkAbstractVolumeMapper::SafeDownCast(m)))
  {
    tMin = this->IntersectVolumeWithLine(p1, p2, t1, t2, prop, volumeMapper);
  }

  // Image
  else if ((imageMapper = vtkImageMapper3D::SafeDownCast(m)))
  {
    tMin = this->IntersectImageWithLine(p1, p2, t1, t2, prop, imageMapper);
  }

  // Actor
  else if ((mapper = vtkMapper::SafeDownCast(m)))
  {
    tMin = this->IntersectActorWithLine(p1, p2, t1, t2, tol, prop, mapper);
  }

  // Unidentified Prop3D
  else
  {
    tMin = this->IntersectProp3DWithLine(p1, p2, t1, t2, tol, prop, m);
  }

  if (tMin < this->GlobalTMin)
  {
    this->GlobalTMin = tMin;
    this->SetPath(path);

    this->ClippingPlaneId = clippingPlaneId;

    // If tMin == t1, the pick didn't go past the first clipping plane,
    // so the position and normal will be set from the clipping plane.
    if (fabs(tMin - t1) < VTKCELLPICKER_PLANE_TOL && clippingPlaneId >= 0)
    {
      this->MapperPosition[0] = p1[0] * (1.0 - t1) + p2[0] * t1;
      this->MapperPosition[1] = p1[1] * (1.0 - t1) + p2[1] * t1;
      this->MapperPosition[2] = p1[2] * (1.0 - t1) + p2[2] * t1;

      double plane[4];
      m->GetClippingPlaneInDataCoords(this->Transform->GetMatrix(), clippingPlaneId, plane);
      vtkMath::Normalize(plane);
      // Want normal outward from the planes, not inward
      this->MapperNormal[0] = -plane[0];
      this->MapperNormal[1] = -plane[1];
      this->MapperNormal[2] = -plane[2];
    }

    // The position comes from the data, so put it into world coordinates
    this->Transform->TransformPoint(this->MapperPosition, this->PickPosition);
    this->Transform->TransformNormal(this->MapperNormal, this->PickNormal);
  }

  return tMin;
}

//----------------------------------------------------------------------------
double vtkCellPicker::IntersectActorWithLine(const double p1[3], const double p2[3], double t1,
  double t2, double tol, vtkProp3D* prop, vtkMapper* mapper)
{
  // Intersect each cell with ray. Keep track of the one closest to
  // the eye (within the tolerance tol) and within the clipping range).
  // Note that we fudge the "closest to" (tMin+this->Tolerance) a little and
  // keep track of the cell with the best pick based on parametric
  // coordinate (pick the minimum, maximum parametric distance). This
  // breaks ties in a reasonable way when cells are the same distance
  // from the eye (like cells laying on a 2D plane).

  double tMin = VTK_DOUBLE_MAX;

  double minPCoords[3];
  double pDistMin = VTK_DOUBLE_MAX;
  vtkIdType minCellId = -1;
  int minSubId = -1;
  double minXYZ[3];
  minXYZ[0] = minXYZ[1] = minXYZ[2] = 0.0;
  vtkAbstractCellLocator* locator = nullptr;
  vtkIdType flatIndex = -1;

  vtkDataSet* data = mapper->GetInput();
  if (data)
  {
    bool ok = this->IntersectDataSetWithLine(
      data, p1, p2, t1, t2, tol, locator, minCellId, minSubId, tMin, pDistMin, minXYZ, minPCoords);
    if (!ok)
    {
      return VTK_DOUBLE_MAX;
    }
  }
  else
  {
    // Construct the ray
    //
    double ray[3];
    ray[0] = p2[0] - p1[0];
    ray[1] = p2[1] - p1[1];
    ray[2] = p2[2] - p1[2];

    vtkCompositeDataSet* composite =
      vtkCompositeDataSet::SafeDownCast(mapper->GetInputDataObject(0, 0));
    if (composite)
    {
      using Opts = vtk::CompositeDataSetOptions;
      for (auto node : vtk::Range(composite, Opts::SkipEmptyNodes))
      {
        vtkDataSet* ds = vtkDataSet::SafeDownCast(node.GetDataObject());
        if (!ds)
        {
          vtkDebugMacro(<< "Skipping " << node.GetDataObject()->GetClassName() << " block at index "
                        << node.GetFlatIndex() << ": not a dataset");
          continue;
        }

        // First check if the bounding box of the data set is hit.
        double bounds[6];
        ds->GetBounds(bounds);
        bounds[0] -= tol;
        bounds[1] += tol;
        bounds[2] -= tol;
        bounds[3] += tol;
        bounds[4] -= tol;
        bounds[5] += tol;
        double tDummy;
        double xyzDummy[3];

        if (!vtkBox::IntersectBox(bounds, const_cast<double*>(p1), ray, xyzDummy, tDummy))
        {
          // box not hit: no need to intersect...
          continue;
        }

        double t = tMin;
        vtkAbstractCellLocator* loc = nullptr;
        vtkIdType cellId = -1;
        int subId = -1;
        double p = VTK_DOUBLE_MAX;
        double xyz[3] = { 0., 0., 0. };
        double pcoord[3] = { 0., 0., 0. };
        bool ok = this->IntersectDataSetWithLine(
          ds, p1, p2, t1, t2, tol, loc, cellId, subId, t, p, xyz, pcoord);
        if (ok)
        {
          tMin = t;

          flatIndex = node.GetFlatIndex();
          data = ds;
          locator = loc;

          minCellId = cellId;
          minSubId = subId;
          pDistMin = p;
          std::copy(xyz, xyz + 3, minXYZ);
          std::copy(pcoord, pcoord + 3, minPCoords);
        }
      }
    }
  }

  // Do this if a cell was intersected
  if (minCellId >= 0 && tMin < this->GlobalTMin)
  {
    this->ResetPickInfo();

    // Get the cell, convert to triangle if it is a strip
    vtkGenericCell* cell = this->Cell;

    // If we used a locator, we already have the picked cell
    if (!locator)
    {
      int cellType = data->GetCellType(minCellId);

      if (this->HasSubCells(cellType))
      {
        data->GetCellPoints(minCellId, this->PointIds);
        this->GetSubCell(data, this->PointIds, minSubId, cellType, cell);
      }
      else
      {
        data->GetCell(minCellId, cell);
      }
    }

    // Get the cell weights
    vtkIdType numPoints = cell->GetNumberOfPoints();
    double* weights = new double[numPoints];
    for (vtkIdType i = 0; i < numPoints; i++)
    {
      weights[i] = 0;
    }

    // Get the interpolation weights (point is thrown away)
    double point[3];
    cell->EvaluateLocation(minSubId, minPCoords, point, weights);

    this->Mapper = mapper;

    // Get the texture from the actor or the LOD
    vtkActor* actor = nullptr;
    vtkLODProp3D* lodActor = nullptr;
    if ((actor = vtkActor::SafeDownCast(prop)))
    {
      this->Texture = actor->GetTexture();
    }
    else if ((lodActor = vtkLODProp3D::SafeDownCast(prop)))
    {
      int lodId = lodActor->GetPickLODID();
      lodActor->GetLODTexture(lodId, &this->Texture);
    }

    if (this->PickTextureData && this->Texture)
    {
      // Return the texture's image data to the user
      vtkImageData* image = this->Texture->GetInput();
      this->DataSet = image;

      // Get and check the image dimensions
      int extent[6];
      image->GetExtent(extent);
      int dimensionsAreValid = 1;
      int dimensions[3];
      for (int i = 0; i < 3; i++)
      {
        dimensions[i] = extent[2 * i + 1] - extent[2 * i] + 1;
        dimensionsAreValid = (dimensionsAreValid && dimensions[i] > 0);
      }

      // Use the texture coord to set the information
      double tcoord[3];
      if (dimensionsAreValid && this->ComputeSurfaceTCoord(data, cell, weights, tcoord))
      {
        // Take the border into account when computing coordinates
        double x[3];
        x[0] = extent[0] + tcoord[0] * dimensions[0] - 0.5;
        x[1] = extent[2] + tcoord[1] * dimensions[1] - 0.5;
        x[2] = extent[4] + tcoord[2] * dimensions[2] - 0.5;

        this->SetImageDataPickInfo(x, extent);
      }
    }
    else
    {
      // Return the polydata to the user
      this->DataSet = data;
      this->FlatBlockIndex = flatIndex;
      this->CellId = minCellId;
      this->SubId = minSubId;
      this->PCoords[0] = minPCoords[0];
      this->PCoords[1] = minPCoords[1];
      this->PCoords[2] = minPCoords[2];

      // Find the point with the maximum weight
      double maxWeight = 0;
      vtkIdType iMaxWeight = -1;
      for (vtkIdType i = 0; i < numPoints; i++)
      {
        if (weights[i] > maxWeight)
        {
          iMaxWeight = i;
          maxWeight = weights[i];
        }
      }

      // If maximum weight is found, use it to get the PointId
      if (iMaxWeight != -1)
      {
        this->PointId = cell->PointIds->GetId(iMaxWeight);
      }
    }

    // Set the mapper position
    this->MapperPosition[0] = minXYZ[0];
    this->MapperPosition[1] = minXYZ[1];
    this->MapperPosition[2] = minXYZ[2];

    // Compute the normal
    if (!this->ComputeSurfaceNormal(data, cell, weights, this->MapperNormal))
    {
      // By default, the normal points back along view ray
      this->MapperNormal[0] = p1[0] - p2[0];
      this->MapperNormal[1] = p1[1] - p2[1];
      this->MapperNormal[2] = p1[2] - p2[2];
      vtkMath::Normalize(this->MapperNormal);
    }

    delete[] weights;
  }

  return tMin;
}

bool vtkCellPicker::IntersectDataSetWithLine(vtkDataSet* dataSet, const double p1[3],
  const double p2[3], double t1, double t2, double tol, vtkAbstractCellLocator*& locator,
  vtkIdType& minCellId, int& minSubId, double& tMin, double& pDistMin, double minXYZ[3],
  double minPCoords[3])
{
  bool cellWasPicked = false;
  // Polydata has no 3D cells
  vtkTypeBool isPolyData = dataSet->IsA("vtkPolyData");

  // Make a new p1 and p2 using the clipped t1 and t2
  double q1[3], q2[3];
  q1[0] = p1[0];
  q1[1] = p1[1];
  q1[2] = p1[2];
  q2[0] = p2[0];
  q2[1] = p2[1];
  q2[2] = p2[2];
  if (t1 != 0.0 || t2 != 1.0)
  {
    for (int j = 0; j < 3; j++)
    {
      q1[j] = p1[j] * (1.0 - t1) + p2[j] * t1;
      q2[j] = p1[j] * (1.0 - t2) + p2[j] * t2;
    }
  }

  // Use the locator if one exists for this data
  vtkCollectionSimpleIterator iter;
  locator = nullptr;
  this->Locators->InitTraversal(iter);
  while (
    (locator = static_cast<vtkAbstractCellLocator*>(this->Locators->GetNextItemAsObject(iter))))
  {
    if (locator->GetDataSet() == dataSet)
    {
      break;
    }
  }

  if (locator)
  {
    double t = tMin;
    double xyz[3];
    double pcoords[3];
    int subId;
    vtkIdType cellId;
    if (locator->IntersectWithLine(q1, q2, tol, t, xyz, pcoords, subId, cellId, this->Cell))
    {
      // Stretch t out to the original range
      if (t1 != 0.0 || t2 != 1.0)
      {
        t = t1 * (1.0 - t) + t2 * t;
      }

      // If cell is a strip, then replace cell with a sub-cell
      this->SubCellFromCell(this->Cell, subId);

      if (t <= (tMin + this->Tolerance) && t >= t1 && t <= t2)
      {
        cellWasPicked = true;
        minCellId = cellId;
        minSubId = subId;
        minXYZ[0] = xyz[0];
        minXYZ[1] = xyz[1];
        minXYZ[2] = xyz[2];
        minPCoords[0] = pcoords[0];
        minPCoords[1] = pcoords[1];
        minPCoords[2] = pcoords[2];
        tMin = t;
      }
    }
  }
  else
  {
    vtkIdList* pointIds = this->PointIds;
    vtkIdType numCells = dataSet->GetNumberOfCells();

    for (vtkIdType cellId = 0; cellId < numCells; cellId++)
    {
      double t;
      double x[3];
      double pcoords[3];
      pcoords[0] = pcoords[1] = pcoords[2] = 0;
      int newSubId = -1;
      int numSubIds = 1;

      // If it is a strip, we need to iterate over the subIds
      int cellType = dataSet->GetCellType(cellId);
      int useSubCells = this->HasSubCells(cellType);
      if (useSubCells)
      {
        // Get the pointIds for the strip and the length of the strip
        dataSet->GetCellPoints(cellId, pointIds);
        numSubIds = this->GetNumberOfSubCells(pointIds, cellType);
      }

      // This will only loop once unless we need to deal with a strip
      for (int subId = 0; subId < numSubIds; subId++)
      {
        if (useSubCells)
        {
          // Get a sub-cell from a the strip
          this->GetSubCell(dataSet, pointIds, subId, cellType, this->Cell);
        }
        else
        {
          dataSet->GetCell(cellId, this->Cell);
        }

        int cellPicked = 0;
        if (isPolyData)
        {
          // Polydata can always be picked with original endpoints
          cellPicked = this->Cell->IntersectWithLine(
            const_cast<double*>(p1), const_cast<double*>(p2), tol, t, x, pcoords, newSubId);
        }
        else
        {
          // Any 3D cells need to be intersected with a line segment that
          // has been clipped with the clipping planes, in case one end is
          // actually inside the cell.
          cellPicked = this->Cell->IntersectWithLine(q1, q2, tol, t, x, pcoords, newSubId);

          // Stretch t out to the original range
          if (t1 != 0.0 || t2 != 1.0)
          {
            t = t1 * (1.0 - t) + t2 * t;
          }
        }

        if (cellPicked && t <= (tMin + this->Tolerance) && t >= t1 && t <= t2)
        {
          cellWasPicked = true;
          double pDist = this->Cell->GetParametricDistance(pcoords);
          if (pDist < pDistMin || (pDist == pDistMin && t < tMin))
          {
            tMin = t;
            pDistMin = pDist;
            // save all of these
            minCellId = cellId;
            minSubId = newSubId;
            if (useSubCells)
            {
              minSubId = subId;
            }
            for (int k = 0; k < 3; k++)
            {
              minXYZ[k] = x[k];
              minPCoords[k] = pcoords[k];
            }
          } // for all subIds
        }   // if minimum, maximum
      }     // if a close cell
    }       // for all cells
  }
  return cellWasPicked;
}

//----------------------------------------------------------------------------
double vtkCellPicker::IntersectHyperTreeGridWithLine(const double p1[3], const double p2[3],
  double t1, double t2, vtkAbstractHyperTreeGridMapper* mapper)
{
  // Retrieve input grid
  vtkUniformHyperTreeGrid* grid = vtkUniformHyperTreeGrid::SafeDownCast(mapper->GetDataSetInput());
  if (grid == nullptr)
  {
    // This picker works only with uniform hypertree grid inputs
    return VTK_DOUBLE_MAX;
  }

  // Retrieve grid dimensionality
  unsigned int dimension = grid->GetDimension();
  if (dimension != 2)
  {
    // This picker works only with 2-dimensional uniform hypertree grids
    return VTK_DOUBLE_MAX;
  }

  // Retrieve grid topology and geometry
  double origin[3];
  grid->GetOrigin(origin);
  double scale[3];
  grid->GetGridScale(scale);
  int extent[6];
  grid->GetExtent(extent);

  // Determine normal vector of the grid
  double normal[] = { 0., 0., 0., 1. };
  unsigned int orientation = grid->GetOrientation();
  if (p1[orientation] - p2[orientation] > 0)
  {
    normal[orientation] = 1.;
    normal[3] = 1.;
  }
  else
  {
    normal[orientation] = -1.;
    normal[3] = -1.;
  }
  normal[3] += vtkMath::Dot(origin, normal);
  double norm = vtkMath::Norm(normal);
  normal[orientation] /= norm;
  normal[3] /= norm;

  // Determine grid principal axes
  unsigned axis1, axis2;
  if (orientation == 2)
  {
    axis1 = 0;
    axis2 = 1;
  }
  else if (orientation == 1)
  {
    axis1 = 0;
    axis2 = 2;
  }
  else
  {
    axis1 = 1;
    axis2 = 2;
  }

  // Compute ray intersection in grid coordinates
  double gridPoint[3];
  gridPoint[axis1] = (p1[axis1] - origin[axis1]) / scale[axis1];
  gridPoint[axis2] = (p1[axis2] - origin[axis2]) / scale[axis2];
  gridPoint[orientation] = origin[orientation];

  // Compute mapper bounds in grid coordinates
  double bounds[6];
  mapper->GetBounds(bounds);
  unsigned int da1 = 2 * axis1;
  bounds[da1] = (bounds[da1] - origin[axis1]) / scale[axis1];
  bounds[da1 + 1] = (bounds[da1 + 1] - origin[axis1]) / scale[axis1];
  unsigned int da2 = 2 * axis2;
  bounds[da2] = (bounds[da2] - origin[axis2]) / scale[axis2];
  bounds[da2 + 1] = (bounds[da2 + 1] - origin[axis2]) / scale[axis2];

  // Clip ray with view extent
  int plane1, plane2;
  double tMin, tMax;
  if (!vtkBox::IntersectWithLine(
        bounds, gridPoint, gridPoint, tMin, tMax, nullptr, nullptr, plane1, plane2))
  {
    return VTK_DOUBLE_MAX;
  }

  // Ensure that intersection is within clipping planes
  if (tMin < t1 || tMin > t2)
  {
    return VTK_DOUBLE_MAX;
  }

  // Compute actual pick when picking conditions are met
  if (tMin < this->GlobalTMin)
  {
    // Compute index of root cell intercepted by ray
    unsigned int i = 0, j = 0, k = 0;
    switch (orientation)
    {
      case 0:
        i = 0;
        j = static_cast<unsigned int>(floor(gridPoint[1]));
        k = static_cast<unsigned int>(floor(gridPoint[2]));
        break;
      case 1:
        i = static_cast<unsigned int>(floor(gridPoint[0]));
        j = 0;
        k = static_cast<unsigned int>(floor(gridPoint[2]));
        break;
      case 2:
        i = static_cast<unsigned int>(floor(gridPoint[0]));
        j = static_cast<unsigned int>(floor(gridPoint[1]));
        k = 0;
        break;
    } // switch ( orientation )
    vtkIdType index;
    grid->GetIndexFromLevelZeroCoordinates(index, i, j, k);

    // Retrieve material mask
    this->InMask = grid->HasMask() ? grid->GetMask() : nullptr;

    // Reset pick information
    this->ResetPickInfo();
    this->Mapper = mapper;

    // DDM
    this->DataSet = nullptr; // grid;
    // abstractHTGmapper is not in cmakelists, so this function is all dead code
    // when we do make start to exercise this we can add a new dataobject to vtkPicker::DataObject
    // and set that instead of dataset DDM

    this->SubId = 0;
    this->PointId = -1;

    // Convert grid into world coordinates
    this->WordlPoint[axis1] = origin[axis1] + gridPoint[axis1] * scale[axis1];
    this->WordlPoint[axis2] = origin[axis2] + gridPoint[axis2] * scale[axis2];
    this->WordlPoint[orientation] = origin[orientation];

    // Initialize new geometric cursor at tree of root cell
    vtkNew<vtkHyperTreeGridNonOrientedGeometryCursor> cursor;
    grid->InitializeNonOrientedGeometryCursor(cursor, index);
    // Descend into tree only if needed
    if (!cursor->IsLeaf())
    {
      // Compute intersection interactively
      this->RecursivelyProcessTree(cursor, 0);
    }
    else
    {
      // Root cell is the intercepted cell
      this->CellId = cursor->GetGlobalNodeIndex();
    }

    // If picked cell is masked then no picking occurred
    if (this->InMask && this->InMask->GetValue(this->CellId))
    {
      return VTK_DOUBLE_MAX;
    }

    // Set picked mapper position
    this->MapperPosition[axis1] = this->WordlPoint[axis1];
    this->MapperPosition[axis2] = this->WordlPoint[axis2];
    this->MapperPosition[orientation] = this->WordlPoint[orientation];

    // Set mapper normal coordinates
    this->MapperNormal[0] = normal[0];
    this->MapperNormal[1] = normal[1];
    this->MapperNormal[2] = normal[2];

    // Set picked items
  } // if ( tMin < this->GlobalTMin )

  // Return current tMin value
  return tMin;
}

//----------------------------------------------------------------------------
bool vtkCellPicker::RecursivelyProcessTree(
  vtkHyperTreeGridNonOrientedGeometryCursor* cursor, int level)
{
  // Retrieve cell geometry
  const double* origin = cursor->GetOrigin();
  const double* size = cursor->GetOrigin();
  // Check if point is inside cell
  for (unsigned int c = 0; c < 3; ++c)
  {
    if ((this->WordlPoint[c] < origin[c]) || (this->WordlPoint[c] > origin[c] + size[c]))
    {
      return false;
    }
  } // c

  // Pick cell only if it is a leaf
  if (cursor->IsLeaf())
  {
    // Cell at cursor center is a leaf, retrieve its global index
    vtkIdType id = cursor->GetGlobalNodeIndex();
    // Leaf cell contains point
    this->CellId = id;
    // Found it !
    return true;
  } // if ( cursor->IsLeaf() )

  // Cursor is not at leaf, recurse to all children
  int numChildren = vtkHyperTreeGrid::SafeDownCast(this->DataSet)->GetNumberOfChildren();
  for (int ichild = 0; ichild < numChildren; ++ichild)
  {
    cursor->ToChild(ichild);
    // Recurse
    if (this->RecursivelyProcessTree(cursor, level + 1))
    {
      return true;
    }
    cursor->ToParent();
  } // child

  return false;
}

//----------------------------------------------------------------------------
// Intersect a vtkVolume with a line by ray casting.

// For algorithm stability: choose a tolerance that is larger than
// the expected roundoff error in computing the voxel indices from "t"
#define VTKCELLPICKER_VOXEL_TOL 1e-6

double vtkCellPicker::IntersectVolumeWithLine(const double p1[3], const double p2[3], double t1,
  double t2, vtkProp3D* prop, vtkAbstractVolumeMapper* mapper)
{
  vtkImageData* data = vtkImageData::SafeDownCast(mapper->GetDataSetInput());

  if (data == nullptr)
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
    x1[i] = (p1[i] - origin[i]) / spacing[i];
    x2[i] = (p2[i] - origin[i]) / spacing[i];
  }

  // Clip the ray with the extent, results go in s1 and s2
  int planeId;
  double s1, s2;
  if (!this->ClipLineWithExtent(extent, x1, x2, s1, s2, planeId))
  {
    return VTK_DOUBLE_MAX;
  }
  if (s1 >= t1)
  {
    t1 = s1;
  }
  if (s2 <= t2)
  {
    t2 = s2;
  }

  // Sanity check
  if (t2 < t1)
  {
    return VTK_DOUBLE_MAX;
  }

  // Get the property from the volume or the LOD
  vtkVolumeProperty* property = nullptr;
  vtkVolume* volume = nullptr;
  vtkLODProp3D* lodVolume = nullptr;
  if ((volume = vtkVolume::SafeDownCast(prop)))
  {
    property = volume->GetProperty();
  }
  else if ((lodVolume = vtkLODProp3D::SafeDownCast(prop)))
  {
    int lodId = lodVolume->GetPickLODID();
    lodVolume->GetLODProperty(lodId, &property);
  }

  // Get the threshold for the opacity
  double opacityThreshold = this->VolumeOpacityIsovalue;

  // Compute the length of the line intersecting the volume
  double rayLength = sqrt(vtkMath::Distance2BetweenPoints(x1, x2)) * (t2 - t1);

  // This is the minimum increment that will be allowed
  double tTol = VTKCELLPICKER_VOXEL_TOL / rayLength * (t2 - t1);

  // Find out whether there are multiple components in the volume
  int numComponents = data->GetNumberOfScalarComponents();
  int independentComponents = 0;
  if (property)
  {
    independentComponents = property->GetIndependentComponents();
  }
  int numIndependentComponents = 1;
  if (independentComponents)
  {
    numIndependentComponents = numComponents;
  }

  // Create a scalar array, it will be needed later
  vtkDataArray* scalars = vtkDataArray::CreateDataArray(data->GetScalarType());
  scalars->SetNumberOfComponents(numComponents);
  vtkIdType scalarArraySize = numComponents * data->GetNumberOfPoints();
  int scalarSize = data->GetScalarSize();
  void* scalarPtr = data->GetScalarPointer();

  // Go through each volume component separately
  double tMin = VTK_DOUBLE_MAX;
  for (int component = 0; component < numIndependentComponents; component++)
  {
    vtkPiecewiseFunction* scalarOpacity =
      (property ? property->GetScalarOpacity(component) : nullptr);
    int disableGradientOpacity = (property ? property->GetDisableGradientOpacity(component) : 1);
    vtkPiecewiseFunction* gradientOpacity = nullptr;
    if (!disableGradientOpacity && this->UseVolumeGradientOpacity)
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
    scalars->SetVoidArray(
      static_cast<void*>(static_cast<char*>(scalarPtr) + scalarSize * oComponent), scalarArraySize,
      1);

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
        x[j] = x1[j] * (1.0 - t) + x2[j] * t;

        // Paranoia bounds check
        if (x[j] < extent[2 * j])
        {
          x[j] = extent[2 * j];
        }
        else if (x[j] > extent[2 * j + 1])
        {
          x[j] = extent[2 * j + 1];
        }

        xi[j] = vtkMath::Floor(x[j]);
        pcoords[j] = x[j] - xi[j];
      }

      opacity =
        this->ComputeVolumeOpacity(xi, pcoords, data, scalars, scalarOpacity, gradientOpacity);

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
        // Skip dimension "k" if it is perpendicular to ray
        if (fabs(x2[k] - x1[k]) > VTKCELLPICKER_VOXEL_TOL * rayLength)
        {
          // Compute the previous coord along dimension "k"
          double lastX = x1[k] * (1.0 - lastT) + x2[k] * lastT;

          // Increment to next slice boundary along dimension "k",
          // including a tolerance value for stability in cases
          // where lastX is just less than an integer value.
          double nextX = 0;
          if (x2[k] > x1[k])
          {
            nextX = vtkMath::Floor(lastX + VTKCELLPICKER_VOXEL_TOL) + 1;
          }
          else
          {
            nextX = vtkMath::Ceil(lastX - VTKCELLPICKER_VOXEL_TOL) - 1;
          }

          // Compute the "t" value for this slice boundary
          double ttry = lastT + (nextX - lastX) / (x2[k] - x1[k]);
          if (ttry > lastT + tTol && ttry < t)
          {
            t = ttry;
          }
        }
      }

      // Break if far clipping plane has been reached
      if (t >= 1.0)
      {
        t = 1.0;
        break;
      }
    } // End of "while (t <= t2)"

    // If the ray hit the isosurface, compute the isosurface position
    if (opacity > opacityThreshold)
    {
      // Backtrack to the actual surface position unless this was first step
      if (t > t1)
      {
        double f = (opacityThreshold - lastOpacity) / (opacity - lastOpacity);
        t = lastT * (1.0 - f) + t * f;
        for (int j = 0; j < 3; j++)
        {
          x[j] = x1[j] * (1.0 - t) + x2[j] * t;
          if (x[j] < extent[2 * j])
          {
            x[j] = extent[2 * j];
          }
          else if (x[j] > extent[2 * j + 1])
          {
            x[j] = extent[2 * j + 1];
          }
          xi[j] = vtkMath::Floor(x[j]);
          pcoords[j] = x[j] - xi[j];
        }
      }

      // Check to see if this is the new global minimum
      if (t < tMin && t < this->GlobalTMin)
      {
        this->ResetPickInfo();
        tMin = t;

        this->Mapper = mapper;
        this->DataSet = data;

        this->SetImageDataPickInfo(x, extent);

        this->MapperPosition[0] = x[0] * spacing[0] + origin[0];
        this->MapperPosition[1] = x[1] * spacing[1] + origin[1];
        this->MapperPosition[2] = x[2] * spacing[2] + origin[2];

        // Default the normal to the view-plane normal.  This default
        // will be used if the gradient cannot be computed any other way.
        this->MapperNormal[0] = p1[0] - p2[0];
        this->MapperNormal[1] = p1[1] - p2[1];
        this->MapperNormal[2] = p1[2] - p2[2];
        vtkMath::Normalize(this->MapperNormal);

        // Check to see if this is the first step, which means that this
        // is the boundary of the volume.  If this is the case, use the
        // normal of the boundary.
        if (t == t1 && planeId >= 0 && xi[planeId / 2] == extent[planeId])
        {
          this->MapperNormal[0] = 0.0;
          this->MapperNormal[1] = 0.0;
          this->MapperNormal[2] = 0.0;
          this->MapperNormal[planeId / 2] = 2.0 * (planeId % 2) - 1.0;
          if (spacing[planeId / 2] < 0)
          {
            this->MapperNormal[planeId / 2] = -this->MapperNormal[planeId / 2];
          }
        }
        else
        {
          // Set the normal from the direction of the gradient
          int* ci = this->CellIJK;
          double weights[8];
          vtkVoxel::InterpolationFunctions(this->PCoords, weights);
          data->GetVoxelGradient(ci[0], ci[1], ci[2], scalars, this->Gradients);
          double v[3];
          v[0] = v[1] = v[2] = 0.0;
          for (int k = 0; k < 8; k++)
          {
            double* pg = this->Gradients->GetTuple(k);
            v[0] += pg[0] * weights[k];
            v[1] += pg[1] * weights[k];
            v[2] += pg[2] * weights[k];
          }

          double norm = vtkMath::Norm(v);
          if (norm > 0)
          {
            this->MapperNormal[0] = v[0] / norm;
            this->MapperNormal[1] = v[1] / norm;
            this->MapperNormal[2] = v[2] / norm;
          }
        }
      } // End of "if (opacity > opacityThreshold)"
    }   // End of "if (t < tMin && t < this->GlobalTMin)"
  }     // End of loop over volume components

  scalars->Delete();

  return tMin;
}

//----------------------------------------------------------------------------
double vtkCellPicker::IntersectImageWithLine(const double p1[3], const double p2[3], double t1,
  double t2, vtkProp3D* prop, vtkImageMapper3D* imageMapper)
{
  // Get the image information
  vtkImageData* data = imageMapper->GetInput();
  int extent[6];
  data->GetExtent(extent);

  // Get the plane equation for the slice
  double normal[4];
  imageMapper->GetSlicePlaneInDataCoords(prop->GetMatrix(), normal);

  // Point the normal towards camera
  if (normal[0] * (p1[0] - p2[0]) + normal[1] * (p1[1] - p2[1]) + normal[2] * (p1[2] - p2[2]) < 0)
  {
    normal[0] = -normal[0];
    normal[1] = -normal[1];
    normal[2] = -normal[2];
    normal[3] = -normal[3];
  }

  // And convert plane eqn to structured coords
  double xnormal[4];
  data->TransformPhysicalPlaneToContinuousIndex(normal, xnormal);

  // Also convert ray to structured coords
  double x1[3], x2[3];
  data->TransformPhysicalPointToContinuousIndex(p1, x1);
  data->TransformPhysicalPointToContinuousIndex(p2, x2);

  // Get the bounds to discover any cropping that has been applied
  double bounds[6];
  imageMapper->GetIndexBounds(bounds);

  // Clip the ray with the extent
  int planeId, plane2Id;
  double tMin, tMax;
  if (!vtkBox::IntersectWithLine(bounds, x1, x2, tMin, tMax, nullptr, nullptr, planeId, plane2Id))
  {
    return VTK_DOUBLE_MAX;
  }

  if (tMin != tMax)
  {
    // Intersect the ray with the slice plane
    double w1 = vtkMath::Dot(x1, xnormal) + xnormal[3];
    double w2 = vtkMath::Dot(x2, xnormal) + xnormal[3];
    if (w1 * w2 > VTKCELLPICKER_VOXEL_TOL)
    {
      return VTK_DOUBLE_MAX;
    }
    if (w1 * w2 < 0)
    {
      tMin = w1 / (w1 - w2);
    }
  }

  // Make sure that intersection is within clipping planes
  if (tMin < t1 || tMin > t2)
  {
    return VTK_DOUBLE_MAX;
  }

  if (tMin < this->GlobalTMin)
  {
    // Compute the pick position in structured coords
    double x[3];
    for (int j = 0; j < 3; j++)
    {
      x[j] = x1[j] * (1.0 - tMin) + x2[j] * tMin;

      // Do a bounds check.  If beyond tolerance of bound, then
      // pick failed, but if within tolerance, clamp the coord
      // to the bound for robustness against roundoff errors.
      if (x[j] < bounds[2 * j])
      {
        if (x[j] < bounds[2 * j] - VTKCELLPICKER_VOXEL_TOL)
        {
          return VTK_DOUBLE_MAX;
        }
        x[j] = bounds[2 * j];
      }
      else if (x[j] > bounds[2 * j + 1])
      {
        if (x[j] > bounds[2 * j + 1] + VTKCELLPICKER_VOXEL_TOL)
        {
          return VTK_DOUBLE_MAX;
        }
        x[j] = bounds[2 * j + 1];
      }
    }

    this->ResetPickInfo();
    this->Mapper = imageMapper;
    this->DataSet = data;

    // Compute all the pick values
    this->SetImageDataPickInfo(x, extent);

    data->TransformContinuousIndexToPhysicalPoint(x, this->MapperPosition);

    // Set the normal in mapper coordinates
    this->MapperNormal[0] = normal[0];
    this->MapperNormal[1] = normal[1];
    this->MapperNormal[2] = normal[2];
  }

  return tMin;
}

//----------------------------------------------------------------------------
// This is a catch-all for Prop3D types that vtkCellPicker does not
// recognize.  It can be overridden in subclasses to provide support
// for picking new Prop3D types.

double vtkCellPicker::IntersectProp3DWithLine(
  const double*, const double*, double, double, double, vtkProp3D*, vtkAbstractMapper3D*)
{
  return VTK_DOUBLE_MAX;
}

//----------------------------------------------------------------------------
// Clip a line with a collection of clipping planes, or return zero if
// the line does not intersect the volume enclosed by the planes.
// The result of the clipping is returned in t1 and t2, which will have
// values between 0 and 1.  The index of the frontmost intersected plane is
// returned in planeId.

int vtkCellPicker::ClipLineWithPlanes(vtkAbstractMapper3D* mapper, vtkMatrix4x4* mat,
  const double p1[3], const double p2[3], double& t1, double& t2, int& planeId)
{
  // The minPlaneId is the index of the plane that t1 lies on
  planeId = -1;
  t1 = 0.0;
  t2 = 1.0;

  double plane[4];
  int numClipPlanes = mapper->GetNumberOfClippingPlanes();
  for (int i = 0; i < numClipPlanes; i++)
  {
    mapper->GetClippingPlaneInDataCoords(mat, i, plane);

    double d1 = plane[0] * p1[0] + plane[1] * p1[1] + plane[2] * p1[2] + plane[3];
    double d2 = plane[0] * p2[0] + plane[1] * p2[1] + plane[2] * p2[2] + plane[3];

    // If both distances are negative, both points are outside
    if (d1 < 0 && d2 < 0)
    {
      return 0;
    }
    // If only one of the distances is negative, the line crosses the plane
    else if (d1 < 0 || d2 < 0)
    {
      // Compute fractional distance "t" of the crossing between p1 & p2
      double t = 0.0;

      // The "if" here just avoids an expensive division when possible
      if (d1 != 0)
      {
        // We will never have d1==d2 since they have different signs
        t = d1 / (d1 - d2);
      }

      // If point p1 was clipped, adjust t1
      if (d1 < 0)
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

int vtkCellPicker::ClipLineWithExtent(
  const int extent[6], const double x1[3], const double x2[3], double& t1, double& t2, int& planeId)
{
  double bounds[6];
  bounds[0] = extent[0];
  bounds[1] = extent[1];
  bounds[2] = extent[2];
  bounds[3] = extent[3];
  bounds[4] = extent[4];
  bounds[5] = extent[5];

  int p2;
  return vtkBox::IntersectWithLine(bounds, x1, x2, t1, t2, nullptr, nullptr, planeId, p2);
}

//----------------------------------------------------------------------------
// Compute the cell normal either by interpolating the point normals,
// or by computing the plane normal for 2D cells.

int vtkCellPicker::ComputeSurfaceNormal(
  vtkDataSet* data, vtkCell* cell, const double* weights, double normal[3])
{
  vtkDataArray* normals = data->GetPointData()->GetNormals();

  if (normals)
  {
    normal[0] = normal[1] = normal[2] = 0.0;
    double pointNormal[3];
    vtkIdType numPoints = cell->GetNumberOfPoints();
    for (vtkIdType k = 0; k < numPoints; k++)
    {
      normals->GetTuple(cell->PointIds->GetId(k), pointNormal);
      normal[0] += pointNormal[0] * weights[k];
      normal[1] += pointNormal[1] * weights[k];
      normal[2] += pointNormal[2] * weights[k];
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
// Use weights to compute the texture coordinates of a point on the cell.

int vtkCellPicker::ComputeSurfaceTCoord(
  vtkDataSet* data, vtkCell* cell, const double* weights, double tcoord[3])
{
  vtkDataArray* tcoords = data->GetPointData()->GetTCoords();

  if (tcoords)
  {
    tcoord[0] = tcoord[1] = tcoord[2] = 0.0;
    double pointTCoord[3];

    int numComponents = tcoords->GetNumberOfComponents();
    vtkIdType numPoints = cell->GetNumberOfPoints();
    for (vtkIdType k = 0; k < numPoints; k++)
    {
      tcoords->GetTuple(cell->PointIds->GetId(k), pointTCoord);
      for (int i = 0; i < numComponents; i++)
      {
        tcoord[i] += pointTCoord[i] * weights[k];
      }
    }

    return 1;
  }

  return 0;
}

//----------------------------------------------------------------------------
// Do an in-place replacement of a cell with a subcell of that cell
void vtkCellPicker::SubCellFromCell(vtkGenericCell* cell, int subId)
{
  switch (cell->GetCellType())
  {
    case VTK_TRIANGLE_STRIP:
    {
      static int idx[2][3] = { { 0, 1, 2 }, { 1, 0, 2 } };
      int* order = idx[subId & 1];
      vtkIdType pointIds[3];
      double points[3][3];

      pointIds[0] = cell->PointIds->GetId(subId + order[0]);
      pointIds[1] = cell->PointIds->GetId(subId + order[1]);
      pointIds[2] = cell->PointIds->GetId(subId + order[2]);

      cell->Points->GetPoint(subId + order[0], points[0]);
      cell->Points->GetPoint(subId + order[1], points[1]);
      cell->Points->GetPoint(subId + order[2], points[2]);

      cell->SetCellTypeToTriangle();

      cell->PointIds->SetId(0, pointIds[0]);
      cell->PointIds->SetId(1, pointIds[1]);
      cell->PointIds->SetId(2, pointIds[2]);

      cell->Points->SetPoint(0, points[0]);
      cell->Points->SetPoint(1, points[1]);
      cell->Points->SetPoint(2, points[2]);
    }
    break;

    case VTK_POLY_LINE:
    {
      vtkIdType pointIds[2];
      double points[2][3];

      pointIds[0] = cell->PointIds->GetId(subId);
      pointIds[1] = cell->PointIds->GetId(subId + 1);

      cell->Points->GetPoint(subId, points[0]);
      cell->Points->GetPoint(subId + 1, points[1]);

      cell->SetCellTypeToLine();

      cell->PointIds->SetId(0, pointIds[0]);
      cell->PointIds->SetId(1, pointIds[1]);

      cell->Points->SetPoint(0, points[0]);
      cell->Points->SetPoint(1, points[1]);
    }
    break;

    case VTK_POLY_VERTEX:
    {
      double point[3];

      vtkIdType pointId = cell->PointIds->GetId(subId);
      cell->Points->GetPoint(subId, point);

      cell->SetCellTypeToVertex();

      cell->PointIds->SetId(0, pointId);
      cell->Points->SetPoint(0, point);
    }
    break;
  }
}

//----------------------------------------------------------------------------
int vtkCellPicker::HasSubCells(int cellType)
{
  switch (cellType)
  {
    case VTK_TRIANGLE_STRIP:
    case VTK_POLY_LINE:
    case VTK_POLY_VERTEX:
      return 1;
  }

  return 0;
}

//----------------------------------------------------------------------------
// Extract a single subcell from a cell in a data set
int vtkCellPicker::GetNumberOfSubCells(vtkIdList* pointIds, int cellType)
{
  switch (cellType)
  {
    case VTK_TRIANGLE_STRIP:
      return pointIds->GetNumberOfIds() - 2;

    case VTK_POLY_LINE:
      return pointIds->GetNumberOfIds() - 1;

    case VTK_POLY_VERTEX:
      return pointIds->GetNumberOfIds();
  }

  return 0;
}

//----------------------------------------------------------------------------
// Extract a single subcell from a cell in a data set.  This method
// requires a vtkIdList that contains the pointIds for the cell.
void vtkCellPicker::GetSubCell(
  vtkDataSet* data, vtkIdList* ptIds, int subId, int cellType, vtkGenericCell* cell)
{
  switch (cellType)
  {
    case VTK_TRIANGLE_STRIP:
    {
      static int idx[2][3] = { { 0, 1, 2 }, { 1, 0, 2 } };
      int* order = idx[subId & 1];
      vtkIdType pointIds[3];
      double points[3][3];

      pointIds[0] = ptIds->GetId(subId + order[0]);
      pointIds[1] = ptIds->GetId(subId + order[1]);
      pointIds[2] = ptIds->GetId(subId + order[2]);

      data->GetPoint(pointIds[0], points[0]);
      data->GetPoint(pointIds[1], points[1]);
      data->GetPoint(pointIds[2], points[2]);

      cell->SetCellTypeToTriangle();

      cell->PointIds->SetId(0, pointIds[0]);
      cell->PointIds->SetId(1, pointIds[1]);
      cell->PointIds->SetId(2, pointIds[2]);

      cell->Points->SetPoint(0, points[0]);
      cell->Points->SetPoint(1, points[1]);
      cell->Points->SetPoint(2, points[2]);
    }
    break;

    case VTK_POLY_LINE:
    {
      vtkIdType pointIds[2];
      double points[2][3];

      pointIds[0] = ptIds->GetId(subId);
      pointIds[1] = ptIds->GetId(subId + 1);

      data->GetPoint(pointIds[0], points[0]);
      data->GetPoint(pointIds[1], points[1]);

      cell->SetCellTypeToLine();

      cell->PointIds->SetId(0, pointIds[0]);
      cell->PointIds->SetId(1, pointIds[1]);

      cell->Points->SetPoint(0, points[0]);
      cell->Points->SetPoint(1, points[1]);
    }
    break;

    case VTK_POLY_VERTEX:
    {
      double point[3];

      vtkIdType pointId = ptIds->GetId(subId);
      data->GetPoint(pointId, point);

      cell->SetCellTypeToVertex();

      cell->PointIds->SetId(0, pointId);
      cell->Points->SetPoint(0, point);
    }
    break;
  }
}

//----------------------------------------------------------------------------
// Set all Cell and Point information, given a structured coordinate
// and the extent of the data.

void vtkCellPicker::SetImageDataPickInfo(const double x[3], const int extent[6])
{
  for (int j = 0; j < 3; j++)
  {
    double xj = x[j];
    if (xj < extent[2 * j])
    {
      xj = extent[2 * j];
    }
    if (xj > extent[2 * j + 1])
    {
      xj = extent[2 * j + 1];
    }

    this->CellIJK[j] = vtkMath::Floor(xj);
    this->PCoords[j] = xj - this->CellIJK[j];
    // Keep the cell in-bounds if it is on the edge
    if (this->CellIJK[j] == extent[2 * j + 1] && this->CellIJK[j] > extent[2 * j])
    {
      this->CellIJK[j] -= 1;
      this->PCoords[j] = 1.0;
    }
    this->PointIJK[j] = this->CellIJK[j] + (this->PCoords[j] >= 0.5);
  }

  // Stupid const/non-const, and I hate const_cast
  int ext[6];
  ext[0] = extent[0];
  ext[1] = extent[1];
  ext[2] = extent[2];
  ext[3] = extent[3];
  ext[4] = extent[4];
  ext[5] = extent[5];

  this->PointId = vtkStructuredData::ComputePointIdForExtent(ext, this->PointIJK);

  this->CellId = vtkStructuredData::ComputeCellIdForExtent(ext, this->CellIJK);

  this->SubId = 0;
}

//----------------------------------------------------------------------------
// Given a structured position within the volume, and the point scalars,
// compute the local opacity of the volume.

double vtkCellPicker::ComputeVolumeOpacity(const int xi[3], const double pcoords[3],
  vtkImageData* data, vtkDataArray* scalars, vtkPiecewiseFunction* scalarOpacity,
  vtkPiecewiseFunction* gradientOpacity)
{
  double opacity = 1.0;

  // Get interpolation weights from the pcoords
  double weights[8];
  vtkVoxel::InterpolationFunctions(const_cast<double*>(pcoords), weights);

  // Get the volume extent to avoid out-of-bounds
  int extent[6];
  data->GetExtent(extent);
  int scalarType = data->GetScalarType();

  // Compute the increments for the three directions, checking the bounds
  vtkIdType xInc = 1;
  vtkIdType yInc = extent[1] - extent[0] + 1;
  vtkIdType zInc = yInc * (extent[3] - extent[2] + 1);
  if (xi[0] == extent[1])
  {
    xInc = 0;
  }
  if (xi[1] == extent[3])
  {
    yInc = 0;
  }
  if (xi[2] == extent[5])
  {
    zInc = 0;
  }

  // Use the increments and weights to interpolate the data
  vtkIdType ptId = data->ComputePointId(const_cast<int*>(xi));
  double val = 0.0;
  for (int j = 0; j < 8; j++)
  {
    vtkIdType ptInc = (j & 1) * xInc + ((j >> 1) & 1) * yInc + ((j >> 2) & 1) * zInc;
    val += weights[j] * scalars->GetComponent(ptId + ptInc, 0);
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
    opacity *= val / 255.0;
  }

  // Compute gradient and GradientOpacity
  if (gradientOpacity)
  {
    data->GetVoxelGradient(xi[0], xi[1], xi[2], scalars, this->Gradients);
    double v[3];
    v[0] = v[1] = v[2] = 0.0;
    for (int k = 0; k < 8; k++)
    {
      double* pg = this->Gradients->GetTuple(k);
      v[0] += pg[0] * weights[k];
      v[1] += pg[1] * weights[k];
      v[2] += pg[2] * weights[k];
    }
    double grad = sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    opacity *= gradientOpacity->GetValue(grad);
  }

  return opacity;
}
