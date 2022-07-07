/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHardwarePicker.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHardwarePicker.h"

#include "vtkAbstractMapper3D.h"
#include "vtkAbstractVolumeMapper.h"
#include "vtkActor.h"
#include "vtkAssemblyPath.h"
#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkIdTypeArray.h"
#include "vtkImageMapper3D.h"
#include "vtkImageSlice.h"
#include "vtkInformation.h"
#include "vtkLODProp3D.h"
#include "vtkMapper.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolygon.h"
#include "vtkPropCollection.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkTriangle.h"
#include "vtkVolume.h"

#include <limits>

namespace
{
constexpr double DEFAULT_VALUE = std::numeric_limits<double>::quiet_NaN();
constexpr double IntersectionTolerance = 0.0000000001;
constexpr double PI_2 = vtkMath::Pi() / 2.0;
}

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkHardwarePicker);

//------------------------------------------------------------------------------
vtkHardwarePicker::vtkHardwarePicker()
{
  this->SnapToMeshPoint = false;
  this->PixelTolerance = 5;

  this->HardwareSelection = vtkSmartPointer<vtkSelection>::New();

  this->NearRayPoint[0] = DEFAULT_VALUE;
  this->NearRayPoint[1] = DEFAULT_VALUE;
  this->NearRayPoint[2] = DEFAULT_VALUE;

  this->FarRayPoint[0] = DEFAULT_VALUE;
  this->FarRayPoint[1] = DEFAULT_VALUE;
  this->FarRayPoint[2] = DEFAULT_VALUE;

  this->Mapper = nullptr;
  this->DataSet = nullptr;
  this->CompositeDataSet = nullptr;
  this->FlatBlockIndex = -1;
  this->PointId = -1;
  this->CellId = -1;
  this->SubId = -1;

  this->PCoords[0] = DEFAULT_VALUE;
  this->PCoords[1] = DEFAULT_VALUE;
  this->PCoords[2] = DEFAULT_VALUE;

  this->PickPosition[0] = DEFAULT_VALUE;
  this->PickPosition[1] = DEFAULT_VALUE;
  this->PickPosition[2] = DEFAULT_VALUE;

  this->PickNormal[0] = DEFAULT_VALUE;
  this->PickNormal[1] = DEFAULT_VALUE;
  this->PickNormal[2] = DEFAULT_VALUE;
  this->NormalFlipped = false;
}

//------------------------------------------------------------------------------
vtkHardwarePicker::~vtkHardwarePicker() = default;

//------------------------------------------------------------------------------
void vtkHardwarePicker::Initialize()
{
  this->vtkAbstractPropPicker::Initialize();

  this->NearRayPoint[0] = DEFAULT_VALUE;
  this->NearRayPoint[1] = DEFAULT_VALUE;
  this->NearRayPoint[2] = DEFAULT_VALUE;

  this->FarRayPoint[0] = DEFAULT_VALUE;
  this->FarRayPoint[1] = DEFAULT_VALUE;
  this->FarRayPoint[2] = DEFAULT_VALUE;

  this->Mapper = nullptr;
  this->DataSet = nullptr;
  this->CompositeDataSet = nullptr;
  this->FlatBlockIndex = -1;
  this->PointId = -1;
  this->CellId = -1;
  this->SubId = -1;

  this->PCoords[0] = DEFAULT_VALUE;
  this->PCoords[1] = DEFAULT_VALUE;
  this->PCoords[2] = DEFAULT_VALUE;

  this->PickPosition[0] = DEFAULT_VALUE;
  this->PickPosition[1] = DEFAULT_VALUE;
  this->PickPosition[2] = DEFAULT_VALUE;

  this->PickNormal[0] = DEFAULT_VALUE;
  this->PickNormal[1] = DEFAULT_VALUE;
  this->PickNormal[2] = DEFAULT_VALUE;
  this->NormalFlipped = false;
}

//------------------------------------------------------------------------------
int vtkHardwarePicker::TypeDecipher(vtkProp* propCandidate, vtkAbstractMapper3D** mapper)
{
  int pickable = 0;
  *mapper = nullptr;

  vtkActor* actor;
  vtkLODProp3D* prop3D;
  vtkProperty* tempProperty;
  vtkVolume* volume;
  vtkImageSlice* imageSlice;

  if (propCandidate->GetPickable() && propCandidate->GetVisibility())
  {
    pickable = 1;
    if ((actor = vtkActor::SafeDownCast(propCandidate)) != nullptr)
    {
      *mapper = actor->GetMapper();
      if (actor->GetProperty()->GetOpacity() <= 0.0)
      {
        pickable = 0;
      }
    }
    else if ((prop3D = vtkLODProp3D::SafeDownCast(propCandidate)) != nullptr)
    {
      int LODId = prop3D->GetPickLODID();
      *mapper = prop3D->GetLODMapper(LODId);

      // if the mapper is a vtkMapper (as opposed to a vtkVolumeMapper),
      // then check the transparency to see if the object is pickable
      if (vtkMapper::SafeDownCast(*mapper) != nullptr)
      {
        prop3D->GetLODProperty(LODId, &tempProperty);
        if (tempProperty->GetOpacity() <= 0.0)
        {
          pickable = 0;
        }
      }
    }
    else if ((volume = vtkVolume::SafeDownCast(propCandidate)) != nullptr)
    {
      *mapper = volume->GetMapper();
    }
    else if ((imageSlice = vtkImageSlice::SafeDownCast(propCandidate)) != nullptr)
    {
      *mapper = imageSlice->GetMapper();
    }
    else
    {
      pickable = 0; // only vtkProp3D's (actors and volumes) can be picked
    }
  }
  return pickable;
}

//----------------------------------------------------------------------------
void vtkHardwarePicker::FixNormalSign()
{
  vtkCamera* camera = this->Renderer->GetActiveCamera();
  double vpn[3];
  camera->GetViewPlaneNormal(vpn);
  if (vtkMath::AngleBetweenVectors(this->PickNormal, vpn) > PI_2)
  {
    this->PickNormal[0] *= -1;
    this->PickNormal[1] *= -1;
    this->PickNormal[2] *= -1;
    this->NormalFlipped = true;
  }
}

//----------------------------------------------------------------------------
int vtkHardwarePicker::ComputeSurfaceNormal(vtkDataSet* data, vtkCell* cell, double* weights)
{
  vtkDataArray* normals = data->GetPointData()->GetNormals();

  if (normals)
  {
    this->PickNormal[0] = this->PickNormal[1] = this->PickNormal[2] = 0.0;
    double pointNormal[3];
    const vtkIdType numPoints = cell->GetNumberOfPoints();
    for (vtkIdType k = 0; k < numPoints; k++)
    {
      normals->GetTuple(cell->PointIds->GetId(k), pointNormal);
      this->PickNormal[0] += pointNormal[0] * weights[k];
      this->PickNormal[1] += pointNormal[1] * weights[k];
      this->PickNormal[2] += pointNormal[2] * weights[k];
    }
    vtkMath::Normalize(this->PickNormal);
  }
  else
  {
    if (cell->GetCellDimension() == 3)
    {
      double t;
      int subId;
      double pcoord[3], x[3];

      int closestIntersectedFaceId = -1;
      double minDist2 = VTK_DOUBLE_MAX;
      // find the face that the ray intersected with that is closer to the intersection point
      for (int i = 0; i < cell->GetNumberOfFaces(); ++i)
      {
        if (cell->GetFace(i)->IntersectWithLine(this->NearRayPoint, this->FarRayPoint,
              IntersectionTolerance, t, x, pcoord, subId) != 0 &&
          t != VTK_DOUBLE_MAX)
        {
          double dist2 = vtkMath::Distance2BetweenPoints(x, this->PickPosition);
          if (dist2 < minDist2)
          {
            minDist2 = dist2;
            closestIntersectedFaceId = i;
          }
        }
      }
      // calculate the normal of the 2D face
      vtkPolygon::ComputeNormal(cell->GetFace(closestIntersectedFaceId)->Points, this->PickNormal);
      this->FixNormalSign();
    }
    else if (cell->GetCellDimension() == 2)
    {
      if (cell->GetCellType() != VTK_TRIANGLE_STRIP)
      {
        // calculate the normal of the 2D cell
        vtkPolygon::ComputeNormal(cell->Points, this->PickNormal);
        this->FixNormalSign();
      }
      else // cell->GetCellType() == VTK_TRIANGLE_STRIP
      {
        constexpr int idx[2][3] = { { 0, 1, 2 }, { 1, 0, 2 } };
        const int* order = idx[this->SubId & 1];
        vtkIdType pointIds[3];
        double points[3][3];

        pointIds[0] = cell->PointIds->GetId(this->SubId + order[0]);
        pointIds[1] = cell->PointIds->GetId(this->SubId + order[1]);
        pointIds[2] = cell->PointIds->GetId(this->SubId + order[2]);

        data->GetPoint(pointIds[0], points[0]);
        data->GetPoint(pointIds[1], points[1]);
        data->GetPoint(pointIds[2], points[2]);

        // calculate the normal of the subId triangle of the triangle strip cell
        vtkTriangle::ComputeNormal(points[0], points[1], points[2], this->PickNormal);
        this->FixNormalSign();
      }
    }
    else
    {
      return 0;
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkHardwarePicker::ComputeIntersectionFromDataSet(vtkDataSet* ds)
{
  if (this->SnapToMeshPoint) // if we are snapping
  {
    ds->GetPoint(this->PointId, this->PickPosition);
    vtkDataArray* normals = ds->GetPointData()->GetNormals();
    if (normals != nullptr) // if point normals exist
    {
      normals->GetTuple(this->PointId, this->PickNormal);
    }
    else
    {
      this->PickNormal[0] = this->PickNormal[1] = this->PickNormal[2] = DEFAULT_VALUE;
    }
  }
  else // if we are not snapping
  {
    double t, x[3];
    vtkCell* cell = ds->GetCell(this->CellId);

    int intersection = cell->IntersectWithLine(this->NearRayPoint, this->FarRayPoint,
      IntersectionTolerance, t, this->PickPosition, this->PCoords, this->SubId);
    if (intersection == 0 && t == VTK_DOUBLE_MAX)
    {
      this->PickPosition[0] = this->PickPosition[1] = this->PickPosition[2] = DEFAULT_VALUE;
      this->PickNormal[0] = this->PickNormal[1] = this->PickNormal[2] = DEFAULT_VALUE;
      vtkErrorMacro("The intersection was not properly found");
      return;
    }

    std::vector<double> weights;
    weights.resize(cell->GetNumberOfPoints());
    cell->EvaluateLocation(this->SubId, this->PCoords, x, weights.data());

    if (!vtkHardwarePicker::ComputeSurfaceNormal(ds, cell, weights.data()))
    {
      this->PickNormal[0] = this->PickNormal[1] = this->PickNormal[2] = DEFAULT_VALUE;
    }
  }
}

//------------------------------------------------------------------------------
int vtkHardwarePicker::Pick(
  double selectionX, double selectionY, double vtkNotUsed(selectionZ), vtkRenderer* renderer)
{
  //  initialize picking process
  this->Initialize();
  this->Renderer = renderer;
  this->SelectionPoint[0] = selectionX;
  this->SelectionPoint[1] = selectionY;
  this->SelectionPoint[2] = 0;

  // invoke start pick method if defined
  this->InvokeEvent(vtkCommand::StartPickEvent, nullptr);

  // choose which prop collection to use
  vtkPropCollection* props = this->PickFromList ? this->PickList : this->Renderer->GetViewProps();

  // preserve only the pickable props.
  this->PickableProps->RemoveAllItems();
  if (props->GetNumberOfItems() > 0)
  {
    vtkCollectionSimpleIterator pit;
    vtkProp* prop;
    for (props->InitTraversal(pit); (prop = props->GetNextProp(pit));)
    {
      vtkAssemblyPath* path;
      for (prop->InitPathTraversal(); (path = prop->GetNextPath());)
      {
        vtkProp* propCandidate = path->GetLastNode()->GetViewProp();
        vtkAbstractMapper3D* mapper = nullptr;
        int pickable = this->TypeDecipher(propCandidate, &mapper);
        if (pickable)
        {
          this->PickableProps->AddItem(propCandidate);
        }
      }
    }
  }

  if (this->SnapToMeshPoint) // snap to the closest point
  {
    // define the point picking area
    double area[4] = { selectionX - this->PixelTolerance, selectionY - this->PixelTolerance,
      selectionX + this->PixelTolerance, selectionY + this->PixelTolerance };
    // do the hardware point pick in the given area
    this->SetPath(renderer->PickPropFrom(area[0], area[1], area[2], area[3], this->PickableProps,
      vtkDataObject::FIELD_ASSOCIATION_POINTS, this->HardwareSelection));
  }
  else // pick a cell
  {
    // do the hardware cell pick
    this->SetPath(renderer->PickPropFrom(selectionX, selectionY, this->PickableProps,
      vtkDataObject::FIELD_ASSOCIATION_CELLS, this->HardwareSelection));
  }

  if (this->Path) // if there was a pick
  {
    vtkProp* propCandidate = this->Path->GetLastNode()->GetViewProp();
    vtkAbstractMapper3D* mapper = nullptr;

    // find the mapper and dataset corresponding to the picked prop
    int pickable = this->TypeDecipher(propCandidate, &mapper);
    if (pickable)
    {
      if (mapper)
      {
        this->Mapper = mapper;
        vtkMapper* map1;
        vtkAbstractVolumeMapper* vmap;
        vtkImageMapper3D* imap;
        if ((map1 = vtkMapper::SafeDownCast(mapper)) != nullptr)
        {
          this->Mapper = map1;
          this->DataSet = map1->GetInput();
          this->CompositeDataSet =
            vtkCompositeDataSet::SafeDownCast(map1->GetInputDataObject(0, 0));
        }
        else if ((vmap = vtkAbstractVolumeMapper::SafeDownCast(mapper)) != nullptr)
        {
          this->Mapper = vmap;
          this->DataSet = vmap->GetDataSetInput();
          this->CompositeDataSet =
            vtkCompositeDataSet::SafeDownCast(vmap->GetInputDataObject(0, 0));
        }
        else if ((imap = vtkImageMapper3D::SafeDownCast(mapper)) != nullptr)
        {
          this->Mapper = imap;
          this->DataSet = imap->GetDataSetInput();
          this->CompositeDataSet =
            vtkCompositeDataSet::SafeDownCast(imap->GetInputDataObject(0, 0));
        }
        else
        {
          this->DataSet = nullptr;
          this->CompositeDataSet = nullptr;
        }
      }
    }

    if (this->DataSet || this->CompositeDataSet)
    {
      // define FlatBlockIndex
      if (this->CompositeDataSet)
      {
        this->FlatBlockIndex = this->HardwareSelection->GetNode(0)->GetProperties()->Get(
          vtkSelectionNode::COMPOSITE_INDEX());
      }
      // define selected dataset
      vtkDataSet* selectedDataSet = this->DataSet
        ? this->DataSet
        : (this->CompositeDataSet ? (vtkDataSet::SafeDownCast(this->CompositeDataSet->GetDataSet(
                                      static_cast<unsigned int>(this->FlatBlockIndex))))
                                  : nullptr);
      // define PointId/CellId
      vtkIdType selectionId =
        vtkIdTypeArray::SafeDownCast(this->HardwareSelection->GetNode(0)->GetSelectionList())
          ->GetValue(0);
      // Note: the hardware selection may return a selectionId that does not correspond to a point
      // or cell in the dataset. If that happens, if we were to use vtkExtractSelection, the
      // resulted dataset would have 0 points/cells. Hence, we will set the PointId/CellId as -1.
      if (this->SnapToMeshPoint)
      {
        if (selectedDataSet)
        {
          this->PointId = selectionId < selectedDataSet->GetNumberOfPoints() ? selectionId : -1;
        }
      }
      else
      {
        if (selectedDataSet)
        {
          this->CellId = selectionId < selectedDataSet->GetNumberOfCells() ? selectionId : -1;
        }
      }

      // define ray points in display coordinates
      double nearDisplayPoint[3] = { selectionX, selectionY, 0.0 };
      double farDisplayPoint[3] = { selectionX, selectionY, 1.0 };

      // compute near ray point in world coordinates
      this->Renderer->SetDisplayPoint(nearDisplayPoint);
      this->Renderer->DisplayToWorld();
      const double* world = this->Renderer->GetWorldPoint();
      for (int i = 0; i < 3; i++)
      {
        this->NearRayPoint[i] = world[i] / world[3];
      }

      // compute far line point in world coordinates
      renderer->SetDisplayPoint(farDisplayPoint);
      renderer->DisplayToWorld();
      world = renderer->GetWorldPoint();
      for (int i = 0; i < 3; i++)
      {
        this->FarRayPoint[i] = world[i] / world[3];
      }

      if (this->PointId != -1 || this->CellId != -1)
      {
        if (selectedDataSet)
        {
          this->ComputeIntersectionFromDataSet(selectedDataSet);
        }
      }
    }
    else
    {
      vtkErrorMacro("Failed to find a dataset corresponding to the picked prop.");
    }
  }
  else // since a path was not found, return camera focal point and plane normal
  {
    vtkCamera* camera = this->Renderer->GetActiveCamera();
    double cameraFP[4];
    camera->GetFocalPoint(cameraFP);
    cameraFP[3] = 1.0;
    this->Renderer->SetWorldPoint(cameraFP);
    this->Renderer->WorldToDisplay();
    double* displayCoord = this->Renderer->GetDisplayPoint();

    // Handle display to world conversion
    double display[3] = { (double)selectionX, (double)selectionY, displayCoord[2] };
    this->Renderer->SetDisplayPoint(display);
    this->Renderer->DisplayToWorld();
    const double* world = this->Renderer->GetWorldPoint();

    // define PickPosition and PickNormal
    for (int i = 0; i < 3; i++)
    {
      this->PickPosition[i] = world[i] / world[3];
    }
    camera->GetViewPlaneNormal(this->PickNormal);
  }

  int picked = 0;
  if (this->Path)
  {
    // invoke pick method if one defined - prop goes first
    this->Path->GetFirstNode()->GetViewProp()->Pick();
    this->InvokeEvent(vtkCommand::PickEvent, nullptr);
    picked = 1;
  }

  // invoke end pick method if defined
  this->InvokeEvent(vtkCommand::EndPickEvent, nullptr);

  return picked;
}

//------------------------------------------------------------------------------
void vtkHardwarePicker::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "SnapToMeshPoint : " << (this->SnapToMeshPoint ? "yes" : "no") << endl;
  os << indent << "PixelTolerance : " << this->PixelTolerance << endl;

  os << indent << "NearRayPoint: (" << this->NearRayPoint[0] << "," << this->NearRayPoint[1] << ","
     << this->NearRayPoint[2] << ")" << endl;
  os << indent << "FarRayPoint: (" << this->FarRayPoint[0] << ", " << this->FarRayPoint[1] << ", "
     << this->FarRayPoint[2] << ")" << endl;

  if (this->Mapper)
  {
    os << indent << "Mapper: " << this->Mapper << endl;
  }
  else
  {
    os << indent << "Mapper: (none)" << endl;
  }
  if (this->DataSet)
  {
    os << indent << "DataSet: " << this->DataSet << endl;
  }
  else
  {
    os << indent << "DataSet: (none)" << endl;
  }
  if (this->CompositeDataSet)
  {
    os << indent << "CompositeDataSet: " << this->CompositeDataSet << endl;
  }
  else
  {
    os << indent << "CompositeDataSet: (none)" << endl;
  }
  if (this->FlatBlockIndex > -1)
  {
    os << indent << "FlatBlockIndex: " << this->FlatBlockIndex << "\n";
  }
  else
  {
    os << indent << "FlatBlockIndex: (none)\n";
  }
  os << indent << "PointId : " << this->PointId << endl;
  os << indent << "CellId : " << this->CellId << endl;
  os << indent << "SubId : " << this->SubId << endl;
  os << indent << "PickNormal: (" << this->PickNormal[0] << "," << this->PickNormal[1] << ","
     << this->PickNormal[2] << ")" << endl;
  os << indent << "PCoords: (" << this->PCoords[0] << ", " << this->PCoords[1] << ", "
     << this->PCoords[2] << ")" << endl;
}
