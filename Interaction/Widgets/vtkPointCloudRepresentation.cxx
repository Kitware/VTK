// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPointCloudRepresentation.h"
#include "vtkActor.h"
#include "vtkActor2D.h"
#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkDataSetMapper.h"
#include "vtkFloatArray.h"
#include "vtkGlyphSource2D.h"
#include "vtkHardwareSelector.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInteractorObserver.h"
#include "vtkMapper2D.h"
#include "vtkObjectFactory.h"
#include "vtkOutlineFilter.h"
#include "vtkPicker.h"
#include "vtkPickingManager.h"
#include "vtkPointData.h"
#include "vtkPointPicker.h"
#include "vtkPointSet.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkPropCollection.h"
#include "vtkProperty.h"
#include "vtkProperty2D.h"
#include "vtkRenderer.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkWindow.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkPointCloudRepresentation);

//------------------------------------------------------------------------------
// Abstract class to hide the details of picking
struct vtkPointCloudPicker
{
  vtkPointCloudRepresentation* Representation;
  vtkPointPicker* PointPicker;

  vtkPointCloudPicker(vtkPointCloudRepresentation* rep)
    : Representation(rep)
  {
    this->PointPicker = vtkPointPicker::New();
    this->PointPicker->PickFromListOn();
  }
  ~vtkPointCloudPicker() { this->PointPicker->Delete(); }

  // Need these to update the picking process
  void InitializePickList() { this->PointPicker->InitializePickList(); }
  void AddPickList(vtkProp* p) { this->PointPicker->AddPickList(p); }

  // Does the dirty work of picking
  vtkIdType Pick(int X, int Y, vtkRenderer* ren, double dispCoords[3], double worldCoords[4])
  {
    vtkPointCloudRepresentation* rep = this->Representation;
    vtkIdType ptId = (-1);

    // Depending on mode, pick appropriately
    if (this->Representation->PickingMode == vtkPointCloudRepresentation::SOFTWARE_PICKING)
    {
      double tol = rep->SoftwarePickingTolerance * rep->InitialLength;
      this->PointPicker->SetTolerance(tol);
      vtkAssemblyPath* path = rep->GetAssemblyPath(X, Y, 0., this->PointPicker);
      if (path != nullptr)
      {
        ptId = this->PointPicker->GetPointId();
        this->PointPicker->GetPickPosition(worldCoords);
        vtkInteractorObserver::ComputeWorldToDisplay(
          ren, worldCoords[0], worldCoords[1], worldCoords[2], dispCoords);
      }
    }
    else // HARDWARE_PICKING
    {
      // Setup pick process
      int tol = rep->HardwarePickingTolerance;
      vtkPoints* pcPoints = rep->PointCloud->GetPoints();
      vtkIdType numPts = rep->PointCloud->GetNumberOfPoints();
      int* winSize = ren->GetSize();
      double pos[3], x[3];
      ren->GetActiveCamera()->GetPosition(pos);

      // Have to instantiate the hardware selector every time or nasty memory leaks occur
      vtkNew<vtkHardwareSelector> selector;
      selector->UpdateMaximumPointId(numPts);
      selector->SetRenderer(ren);
      int xmin = ((X - tol) < 0 ? 0 : (X - tol));
      int ymin = ((Y - tol) < 0 ? 0 : (Y - tol));
      int xmax = ((X + tol) >= winSize[0] ? (winSize[0] - 1) : (X + tol));
      int ymax = ((Y + tol) >= winSize[1] ? (winSize[1] - 1) : (Y + tol));
      selector->SetArea(xmin, ymin, xmax, ymax);

      // We have to temporarily turn off the outline and selection actor
      // so they are not picked,
      rep->OutlineActor->VisibilityOff();
      rep->SelectionActor->VisibilityOff();
      vtkSmartPointer<vtkSelection> sel;
      sel.TakeReference(selector->Select());
      rep->SelectionActor->VisibilityOn();
      rep->OutlineActor->VisibilityOn();

      // Select points, compare with camera position
      double dist2, minDist2 = VTK_DOUBLE_MAX;
      const vtkIdType numNodes = sel->GetNumberOfNodes();
      for (vtkIdType nodeId = 0; nodeId < numNodes; ++nodeId)
      {
        vtkSelectionNode* node = sel->GetNode(nodeId);
        vtkIdTypeArray* selIds = vtkArrayDownCast<vtkIdTypeArray>(node->GetSelectionList());
        if (selIds)
        {
          vtkIdType numIds = selIds->GetNumberOfTuples();
          for (vtkIdType i = 0; i < numIds; ++i)
          {
            vtkIdType pid = selIds->GetValue(i);
            pcPoints->GetPoint(pid, x);
            dist2 = vtkMath::Distance2BetweenPoints(x, pos);
            if (dist2 < minDist2)
            {
              minDist2 = dist2;
              ptId = pid;
            }
          } // for all points in selection
        }   // if selection ids found
      }     // for all selection nodes
      if (ptId >= 0)
      {
        pcPoints->GetPoint(ptId, worldCoords);
        dispCoords[0] = static_cast<double>(X);
        dispCoords[1] = static_cast<double>(Y);
        dispCoords[2] = 0.0;
      }
    } // hardware selection

    return ptId;
  }
};

//------------------------------------------------------------------------------
vtkPointCloudRepresentation::vtkPointCloudRepresentation()
{
  // Internal state
  this->PointCloud = nullptr;
  this->PointCloudMapper = nullptr;
  this->PointCloudActor = nullptr;
  this->Highlighting = true;
  this->PointId = (-1);
  this->PointCoordinates[0] = this->PointCoordinates[1] = this->PointCoordinates[2] = 0.0;
  this->InteractionState = vtkPointCloudRepresentation::Outside;

  // Manage the picking stuff. Note that for huge point clouds the picking may not
  // be fast enough: TODO: use a rendering-based point picker, or further accelerate
  // vtkPointPicker with the use of a point locator.
  this->PickingMode = HARDWARE_PICKING;
  this->HardwarePickingTolerance = 2;      // in pixels
  this->SoftwarePickingTolerance = 0.0001; // fraction of bounding box
  this->OutlinePicker = vtkPicker::New();
  this->OutlinePicker->PickFromListOn();
  this->PointCloudPicker = new vtkPointCloudPicker(this);

  // The outline around the points
  this->OutlineFilter = vtkOutlineFilter::New();

  this->OutlineMapper = vtkPolyDataMapper::New();
  this->OutlineMapper->SetInputConnection(this->OutlineFilter->GetOutputPort());

  this->OutlineActor = vtkActor::New();
  this->OutlineActor->SetMapper(this->OutlineMapper);

  // Create the selection prop
  this->SelectionShape = vtkGlyphSource2D::New();
  this->SelectionShape->SetGlyphTypeToCircle();
  this->SelectionShape->SetResolution(32);
  this->SelectionShape->SetScale(10);

  this->SelectionMapper = vtkPolyDataMapper2D::New();
  this->SelectionMapper->SetInputConnection(this->SelectionShape->GetOutputPort());

  this->SelectionActor = vtkActor2D::New();
  this->SelectionActor->SetMapper(this->SelectionMapper);

  // Set up the initial selection properties
  this->CreateDefaultProperties();
  this->SelectionActor->SetProperty(this->SelectionProperty);
}

//------------------------------------------------------------------------------
vtkPointCloudRepresentation::~vtkPointCloudRepresentation()
{
  if (this->PointCloud)
  {
    this->PointCloud->Delete();
    this->PointCloudMapper->Delete();
    this->PointCloudActor->Delete();
  }
  this->OutlinePicker->Delete();
  delete this->PointCloudPicker;
  this->OutlineFilter->Delete();
  this->OutlineMapper->Delete();
  this->OutlineActor->Delete();
  this->SelectionShape->Delete();
  this->SelectionMapper->Delete();
  this->SelectionActor->Delete();
  this->SelectionProperty->Delete();
}

//------------------------------------------------------------------------------
void vtkPointCloudRepresentation::CreateDefaultProperties()
{
  this->SelectionProperty = vtkProperty2D::New();
  this->SelectionProperty->SetColor(1.0, 1.0, 1.0);
  this->SelectionProperty->SetLineWidth(1.0);
}

//------------------------------------------------------------------------------
void vtkPointCloudRepresentation::PlacePointCloud(vtkActor* a)
{
  // Return if nothing has changed
  if (a == this->PointCloudActor)
  {
    return;
  }

  // Make sure the prop has associated data of the proper type
  vtkPolyDataMapper* mapper = reinterpret_cast<vtkPolyDataMapper*>(a->GetMapper());
  vtkPointSet* pc =
    (mapper == nullptr ? nullptr : reinterpret_cast<vtkPointSet*>(mapper->GetInput()));
  if (mapper == nullptr || pc == nullptr)
  {
    if (this->PointCloud != nullptr)
    {
      this->PointCloud->Delete();
      this->PointCloud = nullptr;
    }
    if (this->PointCloudMapper != nullptr)
    {
      this->PointCloudMapper->Delete();
      this->PointCloudMapper = nullptr;
    }
    if (this->PointCloudActor != nullptr)
    {
      this->PointCloudActor->Delete();
      this->PointCloudActor = nullptr;
    }
    return;
  }

  // Restructure the pipeline
  if (this->PointCloud != nullptr)
  {
    this->PointCloud->Delete();
    this->PointCloudMapper->Delete();
    this->PointCloudActor->Delete();
  }
  this->PointCloud = pc;
  this->PointCloudMapper = mapper;
  this->PointCloudActor = a;
  this->PointCloud->Register(this);
  this->PointCloudMapper->Register(this);
  this->PointCloudActor->Register(this);
  this->PointCloudActor->GetProperty()->SetRepresentationToPoints();

  this->OutlinePicker->InitializePickList();
  this->OutlinePicker->AddPickList(a);

  this->PointCloudPicker->InitializePickList();
  this->PointCloudPicker->AddPickList(a);

  this->PlaceWidget(pc->GetBounds());

  this->OutlineFilter->SetInputData(pc);

  this->Modified();
}

//------------------------------------------------------------------------------
// If specifying point set, create our own actor and mapper
void vtkPointCloudRepresentation::PlacePointCloud(vtkPointSet* pc)
{
  // Return if nothing has changed
  if (pc == this->PointCloud)
  {
    return;
  }

  // Reconstruct the pipeline
  vtkNew<vtkActor> actor;

  if (vtkPolyData::SafeDownCast(pc))
  {
    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputData(vtkPolyData::SafeDownCast(pc));
    actor->SetMapper(mapper);
  }
  else
  {
    vtkNew<vtkDataSetMapper> mapper;
    mapper->SetInputData(pc);
    actor->SetMapper(mapper);
  }

  this->PlacePointCloud(actor);
}

//------------------------------------------------------------------------------
double* vtkPointCloudRepresentation::GetBounds()
{
  if (this->PointCloudActor)
  {
    return this->PointCloudActor->GetBounds();
  }
  else
  {
    return nullptr;
  }
}

//------------------------------------------------------------------------------
int vtkPointCloudRepresentation::ComputeInteractionState(int X, int Y, int vtkNotUsed(modify))
{
  if (!this->Renderer || !this->PointCloudActor || !this->PointCloud)
  {
    this->InteractionState = vtkPointCloudRepresentation::Outside;
    return this->InteractionState;
  }

  // First pick the bounding box to see if we should proceed further. If so,
  // perform a point pick.
  this->PointId = (-1);
  vtkAssemblyPath* path = this->GetAssemblyPath(X, Y, 0., this->OutlinePicker);
  if (path != nullptr)
  {
    this->OutlineActor->VisibilityOn();
    this->InteractionState = vtkPointCloudRepresentation::OverOutline;
    double pD[4], pW[4];
    this->PointId = this->PointCloudPicker->Pick(X, Y, this->Renderer, pD, pW);
    if (this->PointId >= 0)
    {
      this->InteractionState = vtkPointCloudRepresentation::Over;
      this->PointCoordinates[0] = pW[0];
      this->PointCoordinates[1] = pW[1];
      this->PointCoordinates[2] = pW[2];
      this->SelectionShape->SetCenter(pD);
      this->SelectionActor->VisibilityOn();
    }
    else
    {
      this->SelectionActor->VisibilityOff();
    }
  }
  else
  {
    this->InteractionState = vtkPointCloudRepresentation::Outside;
    this->OutlineActor->VisibilityOff();
  }

  return this->InteractionState;
}

//------------------------------------------------------------------------------
void vtkPointCloudRepresentation::GetActors2D(vtkPropCollection* pc)
{
  if (pc != nullptr && this->GetVisibility())
  {
    pc->AddItem(this->SelectionActor);
  }
  this->Superclass::GetActors2D(pc);
}

//------------------------------------------------------------------------------
void vtkPointCloudRepresentation::GetActors(vtkPropCollection* pc)
{
  if (pc != nullptr && this->GetVisibility())
  {
    if (this->PointCloudActor)
    {
      pc->AddItem(this->PointCloudActor);
    }
  }
  this->Superclass::GetActors(pc);
}

//------------------------------------------------------------------------------
void vtkPointCloudRepresentation::ReleaseGraphicsResources(vtkWindow* w)
{
  if (this->PointCloudActor)
  {
    this->PointCloudActor->ReleaseGraphicsResources(w);
  }
  this->OutlineActor->ReleaseGraphicsResources(w);
  this->SelectionActor->ReleaseGraphicsResources(w);
}

//------------------------------------------------------------------------------
int vtkPointCloudRepresentation::RenderOpaqueGeometry(vtkViewport* viewport)
{
  int count = 0;
  if (this->PointCloudActor && !this->Renderer->HasViewProp(this->PointCloudActor))
  {
    count += this->PointCloudActor->RenderOpaqueGeometry(viewport);
  }

  if (this->OutlineActor->GetVisibility())
  {
    count += this->OutlineActor->RenderOpaqueGeometry(viewport);
  }
  return count;
}

//------------------------------------------------------------------------------
int vtkPointCloudRepresentation::RenderTranslucentPolygonalGeometry(vtkViewport* viewport)
{
  int count = 0;
  if (this->PointCloudActor && !this->Renderer->HasViewProp(this->PointCloudActor))
  {
    count += this->PointCloudActor->RenderTranslucentPolygonalGeometry(viewport);
  }

  if (this->OutlineActor->GetVisibility())
  {
    count += this->OutlineActor->RenderTranslucentPolygonalGeometry(viewport);
  }
  return count;
}

//------------------------------------------------------------------------------
vtkTypeBool vtkPointCloudRepresentation::HasTranslucentPolygonalGeometry()
{
  int result = 0;
  if (this->PointCloudActor && !this->Renderer->HasViewProp(this->PointCloudActor))
  {
    result |= this->PointCloudActor->HasTranslucentPolygonalGeometry();
  }

  if (this->OutlineActor->GetVisibility())
  {
    result |= this->OutlineActor->HasTranslucentPolygonalGeometry();
  }

  return result;
}

//------------------------------------------------------------------------------
int vtkPointCloudRepresentation::RenderOverlay(vtkViewport* v)
{
  int count = 0;
  if (this->PointId != (-1) && this->Highlighting)
  {
    vtkRenderer* ren = vtkRenderer::SafeDownCast(v);
    if (ren)
    {
      count += this->SelectionActor->RenderOverlay(v);
    }
  }
  return count;
}

//------------------------------------------------------------------------------
void vtkPointCloudRepresentation::RegisterPickers()
{
  vtkPickingManager* pm = this->GetPickingManager();
  if (!pm)
  {
    return;
  }
  pm->AddPicker(this->OutlinePicker, this);
}

//------------------------------------------------------------------------------
void vtkPointCloudRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->PointCloudActor)
  {
    os << indent << "Point Cloud Actor: " << this->PointCloudActor << "\n";
  }
  else
  {
    os << indent << "Point Cloud Actor: (none)\n";
  }

  os << indent << "Point Id: " << this->PointId << "\n";
  os << indent << "Point Coordinates: (" << this->PointCoordinates[0] << ","
     << this->PointCoordinates[1] << "," << this->PointCoordinates[2] << ")\n";

  os << indent << "Highlighting: " << (this->Highlighting ? "On" : "Off") << "\n";

  os << indent << "Picking Mode: " << this->PickingMode << "\n";
  os << indent << "Hardware Picking Tolerance: " << this->HardwarePickingTolerance << "\n";
  os << indent << "Software Picking Tolerance: " << this->SoftwarePickingTolerance << "\n";

  if (this->SelectionProperty)
  {
    os << indent << "Selection Property: " << this->SelectionProperty << "\n";
  }
  else
  {
    os << indent << "Selection Property: (none)\n";
  }
}
VTK_ABI_NAMESPACE_END
