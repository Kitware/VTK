// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkMagnifierRepresentation.h"
#include "vtkActor2D.h"
#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkProperty2D.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkWindow.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkMagnifierRepresentation);

//------------------------------------------------------------------------------
vtkMagnifierRepresentation::vtkMagnifierRepresentation()
{
  this->MagnificationFactor = 10.0;
  this->Props = vtkPropCollection::New();
  this->Size[0] = this->Size[1] = 75;
  this->Border = false;

  this->InteractionState = Invisible;
  this->MagnificationRenderer = vtkRenderer::New();
  this->Coordinate = vtkCoordinate::New();
  this->Coordinate->SetCoordinateSystemToDisplay();
  this->InsideRenderer = false;

  // Create the geometry in canonical coordinates
  this->BorderPoints = vtkPoints::New();
  this->BorderPoints->SetDataTypeToDouble();
  this->BorderPoints->SetNumberOfPoints(4);
  this->BorderPoints->SetPoint(0, 0.0, 0.0, 0.0);
  this->BorderPoints->SetPoint(1, 1.0, 0.0, 0.0);
  this->BorderPoints->SetPoint(2, 1.0, 1.0, 0.0);
  this->BorderPoints->SetPoint(3, 0.0, 1.0, 0.0);

  vtkCellArray* outline = vtkCellArray::New();
  outline->InsertNextCell(5);
  outline->InsertCellPoint(0);
  outline->InsertCellPoint(1);
  outline->InsertCellPoint(2);
  outline->InsertCellPoint(3);
  outline->InsertCellPoint(0);

  this->BorderPolyData = vtkPolyData::New();
  this->BorderPolyData->SetPoints(this->BorderPoints);
  this->BorderPolyData->SetLines(outline);
  outline->Delete();

  this->BorderMapper = vtkPolyDataMapper2D::New();
  this->BorderMapper->SetInputData(this->BorderPolyData);
  this->BorderActor = vtkActor2D::New();
  this->BorderActor->SetMapper(this->BorderMapper);

  this->BorderProperty = vtkProperty2D::New();
  this->BorderProperty->SetLineWidth(2);
  this->BorderProperty->SetColor(1, 0, 0);
  this->BorderActor->SetProperty(this->BorderProperty);
}

//------------------------------------------------------------------------------
vtkMagnifierRepresentation::~vtkMagnifierRepresentation()
{
  this->Props->Delete();
  this->BorderProperty->Delete();

  if (this->Renderer && this->Renderer->GetRenderWindow())
  {
    this->Renderer->GetRenderWindow()->RemoveRenderer(this->MagnificationRenderer);
  }
  this->MagnificationRenderer->Delete();
  this->Coordinate->Delete();

  this->BorderPoints->Delete();
  this->BorderPolyData->Delete();
  this->BorderMapper->Delete();
  this->BorderActor->Delete();
}

//------------------------------------------------------------------------------
// Move the magnifier around. This method is invoked every time the mouse
// moves.
void vtkMagnifierRepresentation::WidgetInteraction(double eventPos[2])
{
  int XF = eventPos[0];
  int YF = eventPos[1];

  // Make sure the renderer and render window have been defined
  vtkRenderWindow* renWin;
  if (!this->Renderer || !(renWin = this->Renderer->GetRenderWindow()))
  {
    return;
  }

  // If event outside of this window, turn off magnify renderer
  this->InsideRenderer = (this->Renderer->IsInViewport(XF, YF) != 0);

  // Build the representation as necessary
  this->BuildRepresentation();

  // Move the viewport /renderer to the current mouse position
  int* winSize = this->Renderer->GetRenderWindow()->GetSize();
  int* vpSize = this->Renderer->GetSize();
  double vpxMax = static_cast<double>(vpSize[0]) / winSize[0];
  double vpyMax = static_cast<double>(vpSize[1]) / winSize[1];
  double viewPort[4];
  viewPort[0] = eventPos[0] / winSize[0];
  viewPort[1] = eventPos[1] / winSize[1];
  viewPort[2] = viewPort[0] + static_cast<double>(this->Size[0]) / winSize[0];
  viewPort[3] = viewPort[1] + static_cast<double>(this->Size[1]) / winSize[1];
  viewPort[2] = ((viewPort[2] - viewPort[0]) > vpxMax ? vpxMax : viewPort[2]);
  viewPort[3] = ((viewPort[3] - viewPort[1]) > vpyMax ? vpyMax : viewPort[3]);

  this->MagnificationRenderer->SetViewport(viewPort);

  // Update the magnificationcamera
  // Grab the containing renderer's camera and copy it. Adjust the view angle
  vtkCamera* cam = this->Renderer->GetActiveCamera();
  double viewAngle = cam->GetViewAngle();
  vtkCamera* magCam = this->MagnificationRenderer->GetActiveCamera();
  magCam->DeepCopy(cam);
  magCam->SetViewAngle(viewAngle / this->MagnificationFactor);

  // Specify the focal point
  this->Coordinate->SetValue(XF, YF);
  double* focal = this->Coordinate->GetComputedWorldValue(this->Renderer);
  this->MagnificationRenderer->GetActiveCamera()->SetFocalPoint(focal);

  // Setup the border if requested. We offset the border slightly to
  // accommodate the width of the line.
  if (this->Border)
  {
    this->BorderPoints->SetPoint(0, 1, 1, 0.0);
    this->BorderPoints->SetPoint(1, (this->Size[0] - 1), 1, 0.0);
    this->BorderPoints->SetPoint(2, (this->Size[0] - 1), (this->Size[1] - 1), 0.0);
    this->BorderPoints->SetPoint(3, 1, (this->Size[1] - 1), 0.0);
  }
}

//------------------------------------------------------------------------------
// This method is invoked when this class or dependent classes change (based
// on modified time).
void vtkMagnifierRepresentation::BuildRepresentation()
{
  // Make sure a renderer and render window are available
  if (!this->Renderer || !this->Renderer->GetRenderWindow())
  {
    return;
  }

  // If we've been turned off, make it so
  if (this->InteractionState == Invisible || !this->InsideRenderer)
  {
    this->Renderer->GetRenderWindow()->RemoveRenderer(this->MagnificationRenderer);
    return;
  }

  // Set up the renderer and build the border if requested.
  // Add the magnification renderer to the render window
  this->Renderer->GetRenderWindow()->AddRenderer(this->MagnificationRenderer);

  // If props are specifically listed, add them to the magnification
  // renderer. Otherwise, use the containing renderer's props.
  this->MagnificationRenderer->RemoveAllViewProps();
  vtkPropCollection* props;
  if (this->Props->GetNumberOfItems() > 0)
  {
    props = this->Props;
  }
  else
  {
    props = this->Renderer->GetViewProps();
  }
  int numItems = props->GetNumberOfItems();
  props->InitTraversal();
  for (auto i = 0; i < numItems; ++i)
  {
    this->MagnificationRenderer->AddViewProp(static_cast<vtkProp*>(props->GetNextItemAsObject()));
  }

  // Add the border if desired
  if (this->Border)
  {
    this->MagnificationRenderer->AddViewProp(this->BorderActor);
  }
}

//------------------------------------------------------------------------------
vtkMTimeType vtkMagnifierRepresentation::GetMTime()
{
  vtkMTimeType mTime = this->Superclass::GetMTime();
  mTime = std::max(mTime, this->BorderProperty->GetMTime());
  return mTime;
}

//------------------------------------------------------------------------------
// Optionally, use different view props than that used by the associated
// renderer. This enables special effects etc during magnification (or the
// ability to remove props from the scene like widgets etc).
void vtkMagnifierRepresentation::AddViewProp(vtkProp* prop)
{
  this->Props->AddItem(prop);
}

//------------------------------------------------------------------------------
int vtkMagnifierRepresentation::HasViewProp(vtkProp* prop)
{
  return (prop && this->Props->IsItemPresent(prop));
}

//------------------------------------------------------------------------------
void vtkMagnifierRepresentation::RemoveViewProp(vtkProp* prop)
{
  this->Props->RemoveItem(prop);
}

//------------------------------------------------------------------------------
void vtkMagnifierRepresentation::RemoveAllViewProps()
{
  this->Props->RemoveAllItems();
}

//------------------------------------------------------------------------------
void vtkMagnifierRepresentation::ReleaseGraphicsResources(vtkWindow* w)
{
  this->BorderActor->ReleaseGraphicsResources(w);
}

//------------------------------------------------------------------------------
int vtkMagnifierRepresentation::RenderOverlay(vtkViewport* w)
{
  if (this->Border && w == this->MagnificationRenderer)
  {
    this->BuildRepresentation();
    return this->BorderActor->RenderOverlay(w);
  }
  return 0;
}

//------------------------------------------------------------------------------
int vtkMagnifierRepresentation::RenderOpaqueGeometry(vtkViewport* w)
{
  if (this->Border && w == this->MagnificationRenderer)
  {
    this->BuildRepresentation();
    return this->BorderActor->RenderOpaqueGeometry(w);
  }
  return 0;
}

//------------------------------------------------------------------------------
int vtkMagnifierRepresentation::RenderTranslucentPolygonalGeometry(vtkViewport* w)
{
  if (this->Border && w == this->MagnificationRenderer)
  {
    this->BuildRepresentation();
    return this->BorderActor->RenderTranslucentPolygonalGeometry(w);
  }
  return 0;
}

//------------------------------------------------------------------------------
// Description:
// Does this prop have some translucent polygonal geometry?
vtkTypeBool vtkMagnifierRepresentation::HasTranslucentPolygonalGeometry()
{
  if (this->Border)
  {
    this->BuildRepresentation();
    return this->BorderActor->HasTranslucentPolygonalGeometry();
  }
  return false;
}

//------------------------------------------------------------------------------
void vtkMagnifierRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent
     << "Interaction State: " << (this->InteractionState == Visible ? "Visible\n" : "Invisible\n");

  os << indent << "Magnification Factor: " << this->MagnificationFactor << "\n";

  os << indent << "Props:\n";
  this->Props->PrintSelf(os, indent.GetNextIndent());

  os << indent << "Size: " << this->Size[0] << " " << this->Size[1] << endl;

  os << indent << "Border: " << (this->Border ? "On\n" : "Off\n");

  if (this->BorderProperty)
  {
    os << indent << "Border Property:\n";
    this->BorderProperty->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Border Property: (none)\n";
  }
}
VTK_ABI_NAMESPACE_END
