/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiVolume.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMultiVolume.h"
#include "vtkBoundingBox.h"
#include "vtkGPUVolumeRayCastMapper.h"
#include "vtkImageData.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkVector.h"
#include "vtkVectorOperators.h"
#include "vtkVolumeProperty.h"

vtkMultiVolume::vtkMultiVolume()
  : Superclass()
  , TexToBBox(vtkSmartPointer<vtkMatrix4x4>::New())
{
  vtkMath::UninitializeBounds(this->Bounds);
  vtkMath::UninitializeBounds(this->DataBounds.data());
}

vtkMultiVolume::~vtkMultiVolume()
{
  for (auto& pair : this->Volumes)
  {
    vtkVolume* vol = pair.second;
    if (vol)
    {
      vol->UnRegister(this);
    }
  }
}

vtkStandardNewMacro(vtkMultiVolume);

void vtkMultiVolume::SetVolume(vtkVolume* vol, int port)
{
  auto currentVol = this->FindVolume(port);
  if (currentVol != vol)
  {
    if (currentVol)
    {
      currentVol->UnRegister(this);
      this->Volumes.erase(port);
    }

    if (vol)
    {
      this->Volumes[port] = vol;
      vol->Register(this);
    }

    this->Modified();
  }
}

vtkVolume* vtkMultiVolume::GetVolume(int port)
{
  auto vol = this->FindVolume(port);
  if (!vol)
  {
    vtkErrorMacro(<< "Failed to query vtkVolume instance for port " << port);
    return nullptr;
  }

  return vol;
}

vtkVolume* vtkMultiVolume::FindVolume(int port)
{
  vtkVolume* vol = nullptr;
  const auto it = this->Volumes.find(port);
  if (it != this->Volumes.end())
  {
    vol = it->second;
  }
  return vol;
}

double* vtkMultiVolume::GetBounds()
{
  if (!this->VolumesChanged() && vtkMath::AreBoundsInitialized(this->Bounds))
  {
    return this->Bounds;
  }

  vtkMath::UninitializeBounds(this->Bounds);

  // Transform the bounds of each input to world coordinates and compute the
  // total bounds (T_total = T_vol * T_tex (vtkImageData))
  for (auto& item : this->Volumes)
  {
    // Transform to world coordinates (ensure the matrix is
    // up-to-date).
    const int port = item.first;
    auto mapper = vtkGPUVolumeRayCastMapper::SafeDownCast(this->Mapper);
    if (!mapper)
    {
      vtkErrorMacro(<< "vtkMultiVolume is currently only supported by"
                       " vtkGPUVolumeRayCastMapper.");
      return this->Bounds;
    }
    double* bnd = mapper->GetBoundsFromPort(port);

    vtkVolume* vol = item.second;
    vol->ComputeMatrix();
    auto rBoundsWorld = this->ComputeAABounds(bnd, vol->GetMatrix());

    if (vtkMath::AreBoundsInitialized(this->Bounds))
    {
      // Expand current bounds
      for (size_t i = 0; i < 3; i++)
      {
        const size_t c = i * 2;
        this->Bounds[c] = std::min(rBoundsWorld[c], this->Bounds[c]);
        this->Bounds[c + 1] = std::max(rBoundsWorld[c + 1], this->Bounds[c + 1]);
      }
    }
    else
    {
      // Init bounds
      this->Bounds[0] = rBoundsWorld[0];
      this->Bounds[1] = rBoundsWorld[1];
      this->Bounds[2] = rBoundsWorld[2];
      this->Bounds[3] = rBoundsWorld[3];
      this->Bounds[4] = rBoundsWorld[4];
      this->Bounds[5] = rBoundsWorld[5];
    }
  }

  vtkVector3d minPoint(this->Bounds[0], this->Bounds[2], this->Bounds[4]);
  vtkVector3d maxPoint(this->Bounds[1], this->Bounds[3], this->Bounds[5]);

  // The bounding-box coordinate system is axis-aligned with the world
  // coordinate system, so only the translation vector is needed for
  // the bboxDatasetToWorld transformation (unlike other volume-matrices,
  // this one does not include any scaling or rotation, those are only
  // defined in the contained volumes).

  // T_bboxToWorld = T_translation
  // Translation vector is its actual position in world coordinates.
  // (Min-point as origin).
  this->Matrix->Identity();
  this->Matrix->SetElement(0, 3, minPoint[0]);
  this->Matrix->SetElement(1, 3, minPoint[1]);
  this->Matrix->SetElement(2, 3, minPoint[2]);

  // Compute bbox dimensions (world)
  auto scale = maxPoint - minPoint;

  // T_texToBbox = T_scaling
  TexToBBox->Identity();
  TexToBBox->SetElement(0, 0, scale[0]);
  TexToBBox->SetElement(1, 1, scale[1]);
  TexToBBox->SetElement(2, 2, scale[2]);

  // Transform bounds back to data-coords (range [0, scale]). Data-coords
  // is what the mapper expects.
  auto minPointData = minPoint - minPoint;
  auto maxPointData = maxPoint - minPoint;
  this->DataBounds[0] = minPointData[0];
  this->DataBounds[2] = minPointData[1];
  this->DataBounds[4] = minPointData[2];
  this->DataBounds[1] = maxPointData[0];
  this->DataBounds[3] = maxPointData[1];
  this->DataBounds[5] = maxPointData[2];

  this->DataGeometry[0] = minPointData[0];
  this->DataGeometry[1] = minPointData[1];
  this->DataGeometry[2] = minPointData[2];
  this->DataGeometry[3] = maxPointData[0];
  this->DataGeometry[4] = minPointData[1];
  this->DataGeometry[5] = minPointData[2];

  this->DataGeometry[6] = minPointData[0];
  this->DataGeometry[7] = maxPointData[1];
  this->DataGeometry[8] = minPointData[2];
  this->DataGeometry[9] = maxPointData[0];
  this->DataGeometry[10] = maxPointData[1];
  this->DataGeometry[11] = minPointData[2];

  this->DataGeometry[12] = minPointData[0];
  this->DataGeometry[13] = minPointData[1];
  this->DataGeometry[14] = maxPointData[2];
  this->DataGeometry[15] = maxPointData[0];
  this->DataGeometry[16] = minPointData[1];
  this->DataGeometry[17] = maxPointData[2];

  this->DataGeometry[18] = minPointData[0];
  this->DataGeometry[19] = maxPointData[1];
  this->DataGeometry[20] = maxPointData[2];
  this->DataGeometry[21] = maxPointData[0];
  this->DataGeometry[22] = maxPointData[1];
  this->DataGeometry[23] = maxPointData[2];

  this->BoundsComputeTime.Modified();
  return this->Bounds;
}

std::array<double, 6> vtkMultiVolume::ComputeAABounds(double bounds[6], vtkMatrix4x4* T) const
{
  using Point = vtkVector4d;
  using PointVec = std::vector<Point>;

  // Create all corner poiints of the bounding box
  vtkVector3d dim(bounds[1] - bounds[0], bounds[3] - bounds[2], bounds[5] - bounds[4]);

  Point minPoint(bounds[0], bounds[2], bounds[4], 1.0);
  PointVec pointsDataCoords;
  pointsDataCoords.reserve(8);
  pointsDataCoords.push_back(minPoint);
  pointsDataCoords.push_back(std::move(minPoint + Point(dim[0], 0., 0., 0.)));
  pointsDataCoords.push_back(std::move(minPoint + Point(dim[0], dim[1], 0., 0.)));
  pointsDataCoords.push_back(std::move(minPoint + Point(0., dim[1], 0., 0.)));
  pointsDataCoords.push_back(std::move(minPoint + Point(0., 0., dim[2], 0.)));
  pointsDataCoords.push_back(std::move(minPoint + Point(dim[0], 0., dim[2], 0.)));
  pointsDataCoords.push_back(Point(bounds[1], bounds[3], bounds[5], 0.));
  pointsDataCoords.push_back(std::move(minPoint + Point(0., dim[1], dim[2], 0.)));

  // Transform all points from data to world coordinates
  vtkBoundingBox bBoxWorld;
  for (const Point& pData : pointsDataCoords)
  {
    Point pWorld;
    T->MultiplyPoint(pData.GetData(), pWorld.GetData());
    bBoxWorld.AddPoint(pWorld.GetX(), pWorld.GetY(), pWorld.GetZ());
  }
  std::array<double, 6> boundsWorld;
  bBoxWorld.GetBounds(boundsWorld.data());

  return boundsWorld;
}

bool vtkMultiVolume::VolumesChanged()
{
  auto mapper = vtkGPUVolumeRayCastMapper::SafeDownCast(this->Mapper);
  if (!mapper)
  {
    vtkErrorMacro(<< "vtkMultiVolume is currently only supported by"
                     " vtkGPUVolumeRayCastMapper.");
    return false;
  }

  for (auto& item : this->Volumes)
  {
    auto vol = item.second;
    vol->ComputeMatrix();
    const bool moved = this->BoundsComputeTime < vol->GetMatrix()->GetMTime();
    auto data = mapper->GetTransformedInput(item.first);
    const bool changed = data ? this->BoundsComputeTime < data->GetMTime() : true;

    if (moved || changed)
      return true;
  }

  return false;
}

vtkMTimeType vtkMultiVolume::GetMTime()
{
  auto mTime = this->vtkObject::GetMTime();

  mTime = this->BoundsComputeTime > mTime ? this->BoundsComputeTime.GetMTime() : mTime;

  return mTime;
}

void vtkMultiVolume::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
  os << indent << "Num. volumes: " << this->Volumes.size() << "\n";
  os << indent << "BoundsComputeTime: " << this->BoundsComputeTime.GetMTime() << "\n";
  os << indent << "Texture-To-Data: \n";
  this->TexToBBox->PrintSelf(os, indent);
  os << indent << "Data-To-World: \n ";
  this->Matrix->PrintSelf(os, indent);
}

void vtkMultiVolume::ShallowCopy(vtkProp* prop)
{
  auto multiVol = vtkMultiVolume::SafeDownCast(prop);
  if (multiVol)
  {
    for (auto& item : multiVol->Volumes)
    {
      this->SetVolume(item.second, item.first);
    }
    this->DataBounds = multiVol->DataBounds;
    this->DataGeometry = multiVol->DataGeometry;
    this->BoundsComputeTime = multiVol->BoundsComputeTime;
    this->TexToBBox->DeepCopy(multiVol->TexToBBox);
    return;
  }
  Superclass::ShallowCopy(prop);
}

int vtkMultiVolume::RenderVolumetricGeometry(vtkViewport* vp)
{
  this->Update();

  if (!this->Mapper)
  {
    vtkErrorMacro(<< "Invalid Mapper!\n");
    return 0;
  }

  if (!this->Mapper->GetDataObjectInput())
  {
    return 0;
  }

  this->Mapper->Render(static_cast<vtkRenderer*>(vp), this);
  this->EstimatedRenderTime += this->Mapper->GetTimeToDraw();

  return 1;
}

void vtkMultiVolume::SetProperty(vtkVolumeProperty* vtkNotUsed(property))
{
  vtkWarningMacro(<< "This vtkVolumeProperty will not be used during"
                  << " rendering. Volume properties should be specified through registered"
                  << " vtkVolume instances.");
}

vtkVolumeProperty* vtkMultiVolume::GetProperty()
{
  auto volume = this->FindVolume(0);
  if (volume)
    return volume->GetProperty();

  return nullptr;
}
