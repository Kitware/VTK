// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkChartRadViz.h"

#include "vtkContext2D.h"
#include "vtkContextMouseEvent.h"
#include "vtkContextScene.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPlotRadViz.h"
#include "vtkPoints2D.h"
#include "vtkStringArray.h"
#include "vtkTextProperty.h"
#include <sstream>
#include <vtk_fmt.h>
// clang-format off
#include VTK_FMT(fmt/format.h)
#include VTK_FMT(fmt/ranges.h)
// clang-format on

VTK_ABI_NAMESPACE_BEGIN
class vtkChartRadViz::vtkInternals
{
public:
  vtkInternals() = default;

  vtkSmartPointer<vtkPlotRadViz> Plot;
  /**
   * The index of the vertex dragged after left mouse press, -1 when no vertex
   * is dragged.
   */
  int DragVertexIndex = -1;
  /**
   * Set to true on left mouse press on the lens outline and set to false
   * on left mouse release.
   */
  bool ResizeLens = false;
  /**
   * Set to true on left mouse press on the lens interior and set to false
   * on mouse release.
   */
  bool DragLens = false;
  /**
   * Set to the mouse position on left mouse press.
   */
  float DragLensStart[2] = { 0.0f, 0.0f };
  /**
   * Rotation and mouse angle on left mouse press on a vertex.
   */
  double DragStartRotation = 0.0;
  double DragStartMouseAngle = 0.0;
};

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkChartRadViz);

//------------------------------------------------------------------------------
vtkChartRadViz::vtkChartRadViz()
{
  this->Internals = std::make_unique<vtkChartRadViz::vtkInternals>();
  this->AddPlot(0 /*type*/);
}

//------------------------------------------------------------------------------
vtkChartRadViz::~vtkChartRadViz() = default;

//------------------------------------------------------------------------------
void vtkChartRadViz::Update()
{
  if (this->Internals->Plot && this->Internals->Plot->GetVisible())
  {
    this->Internals->Plot->Update();
  }
}

//------------------------------------------------------------------------------
bool vtkChartRadViz::Paint(vtkContext2D* painter)
{
  vtkDebugMacro(<< "Paint event called.");

  int geometry[] = { this->GetScene()->GetSceneWidth(), this->GetScene()->GetSceneHeight() };
  if (geometry[0] == 0 || geometry[1] == 0 || !this->Visible)
  {
    // The geometry of the chart must be valid before anything can be drawn
    return false;
  }

  this->Update();

  if (this->LayoutStrategy == vtkChart::FILL_SCENE &&
    (geometry[0] != this->Geometry[0] || geometry[1] != this->Geometry[1]))
  {
    this->SetSize(vtkRectf(0.0, 0.0, geometry[0], geometry[1]));
  }

  vtkVector2i tileScale = this->Scene->GetLogicalTileScale();
  const int BORDER_SIZE = 20;
  const int TITLE_SPACE = 5;
  this->SetBorders(BORDER_SIZE * tileScale.GetX(), BORDER_SIZE * tileScale.GetY(),
    BORDER_SIZE * tileScale.GetX(), BORDER_SIZE * tileScale.GetY());

  int titleSize = 0;
  if (!this->Title.empty())
  {
    int fontSize = this->TitleProperties->GetFontSize();
    titleSize = fontSize + TITLE_SPACE;
    vtkPoints2D* rect = vtkPoints2D::New();
    rect->InsertNextPoint(this->Point1[0], this->Point2[1]);
    rect->InsertNextPoint(this->Point2[0] - this->Point1[0], fontSize);
    painter->ApplyTextProp(this->TitleProperties);
    painter->DrawStringRect(rect, this->Title);
    rect->Delete();
  }

  // Set the dimensions of the Plot
  if (this->Internals->Plot)
  {
    this->Internals->Plot->SetDimensions(this->Size.GetX() + BORDER_SIZE,
      this->Size.GetY() + BORDER_SIZE, this->Geometry[0] - 2 * BORDER_SIZE,
      this->Geometry[1] - 2 * BORDER_SIZE - titleSize);
    this->Internals->Plot->Paint(painter);
  }

  return true;
}

//------------------------------------------------------------------------------
void vtkChartRadViz::SetScene(vtkContextScene* scene)
{
  this->vtkAbstractContextItem::SetScene(scene);
}

//------------------------------------------------------------------------------
vtkPlot* vtkChartRadViz::AddPlot(int /* type */)
{
  if (!this->Internals->Plot)
  {
    this->Internals->Plot = vtkSmartPointer<vtkPlotRadViz>::New();
    this->Internals->Plot->SetParent(this);
    this->AddItem(this->Internals->Plot);
  }
  return this->Internals->Plot;
}

//------------------------------------------------------------------------------
void vtkChartRadViz::SetPlot(vtkPlotRadViz* plot)
{
  if (this->Internals->Plot)
  {
    this->RemoveItem(this->Internals->Plot);
  }
  this->Internals->Plot = plot;
  this->Internals->Plot->SetParent(this);
  this->AddItem(this->Internals->Plot);
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkChartRadViz::SetColumnVisibility(const vtkStdString& name, bool visible)
{
  if (this->Internals->Plot)
  {
    this->Internals->Plot->SetColumnVisibility(name, visible);
  }
}

//------------------------------------------------------------------------------
void vtkChartRadViz::SetColumnVisibilityAll(bool visible)
{
  if (this->Internals->Plot)
  {
    this->Internals->Plot->SetColumnVisibilityAll(visible);
  }
}

//------------------------------------------------------------------------------
bool vtkChartRadViz::GetColumnVisibility(const vtkStdString& name)
{
  return this->Internals->Plot ? this->Internals->Plot->GetColumnVisibility(name) : false;
}

//------------------------------------------------------------------------------
vtkStringArray* vtkChartRadViz::GetVisibleColumns()
{
  return this->Internals->Plot ? this->Internals->Plot->GetVisibleColumns() : nullptr;
}

//------------------------------------------------------------------------------
void vtkChartRadViz::SetVisibleColumns(vtkStringArray* visColumns)
{
  if (this->Internals->Plot)
  {
    this->Internals->Plot->SetVisibleColumns(visColumns);
  }
}

//------------------------------------------------------------------------------
vtkPlot* vtkChartRadViz::GetPlot(vtkIdType index)
{
  if (index == 0)
  {
    return this->Internals->Plot;
  }

  return nullptr;
}

//------------------------------------------------------------------------------
vtkIdType vtkChartRadViz::GetNumberOfPlots()
{
  if (this->Internals->Plot)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

//------------------------------------------------------------------------------
bool vtkChartRadViz::Hit(const vtkContextMouseEvent& mouse)
{
  vtkVector2f pos(mouse.GetScenePos());
  return pos[0] > this->Point1[0] - 10 && pos[0] < this->Point2[0] + 10 &&
    pos[1] > this->Point1[1] && pos[1] < this->Point2[1];
}

//------------------------------------------------------------------------------
bool vtkChartRadViz::MouseEnterEvent(const vtkContextMouseEvent&)
{
  return true;
}

//------------------------------------------------------------------------------
bool vtkChartRadViz::MouseMoveEvent(const vtkContextMouseEvent& mouse)
{
  if (this->Internals->Plot)
  {
    vtkVector2f pos{ mouse.GetScenePos() };
    if (this->Internals->DragVertexIndex >= 0)
    {
      // rotate vertices by the angle swept by the mouse since the press
      const double* center = this->Internals->Plot->GetCenter();
      double mouseAngle =
        vtkMath::DegreesFromRadians(std::atan2(pos[1] - center[1], pos[0] - center[0]));
      this->Internals->Plot->SetVerticesRotation(
        this->Internals->DragStartRotation + mouseAngle - this->Internals->DragStartMouseAngle);
      this->Scene->SetDirty(true);
    }
    else if (this->Internals->ResizeLens)
    {
      double* lensCenter = this->Internals->Plot->GetLensCenter();
      // distance between the lensCenter and the mouse point
      double r =
        std::sqrt(std::pow(pos[0] - lensCenter[0], 2) + std::pow(pos[1] - lensCenter[1], 2));
      this->Internals->Plot->SetLensRadius(r);
      this->Scene->SetDirty(true);
    }
    else if (this->Internals->DragLens)
    {
      double d[2];

      d[0] = pos[0] - this->Internals->DragLensStart[0];
      d[1] = pos[1] - this->Internals->DragLensStart[1];
      double newLensCenter[2];
      double* lensCenter = this->Internals->Plot->GetLensCenter();
      newLensCenter[0] = lensCenter[0] + d[0];
      ;
      newLensCenter[1] = lensCenter[1] + d[1];
      this->Internals->Plot->SetLensCenter(newLensCenter);
      std::copy(pos.GetData(), pos.GetData() + 2, this->Internals->DragLensStart);
      this->Scene->SetDirty(true);
    }
    else
    {
      int activeVertexIndex;
      bool activeLensOutline;
      bool activeLensInterior;
      this->GetActiveElement(pos, activeVertexIndex, activeLensOutline, activeLensInterior);
      if (activeVertexIndex != this->Internals->Plot->GetActiveVertexIndex() ||
        activeLensOutline != this->Internals->Plot->GetActiveLensOutline() ||
        activeLensInterior != this->Internals->Plot->GetActiveLensInterior())
      {
        this->Scene->SetDirty(true);
      }
      this->Internals->Plot->SetActiveVertexIndex(activeVertexIndex);
      this->Internals->Plot->SetActiveLensOutline(activeLensOutline);
      this->Internals->Plot->SetActiveLensInterior(activeLensInterior);
    }
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkChartRadViz::MouseLeaveEvent(const vtkContextMouseEvent&)
{
  return true;
}

//------------------------------------------------------------------------------
bool vtkChartRadViz::MouseButtonPressEvent(const vtkContextMouseEvent& mouse)
{
  vtkDebugMacro("vtkChartRadViz::MouseButtonPressEvent");
  if (this->Internals->Plot)
  {
    vtkVector2f pos{ mouse.GetScenePos() };
    this->GetActiveElement(pos, this->Internals->DragVertexIndex, this->Internals->ResizeLens,
      this->Internals->DragLens);
    if (this->Internals->DragVertexIndex >= 0)
    {
      const double* center = this->Internals->Plot->GetCenter();
      this->Internals->DragStartMouseAngle =
        vtkMath::DegreesFromRadians(std::atan2(pos[1] - center[1], pos[0] - center[0]));
      this->Internals->DragStartRotation = this->Internals->Plot->GetVerticesRotation();
    }
    if (this->Internals->DragLens)
    {
      std::copy(pos.GetData(), pos.GetData() + 2, this->Internals->DragLensStart);
    }
    return true;
  }
  else
  {
    return false;
  }
}

//------------------------------------------------------------------------------
void vtkChartRadViz::GetActiveElement(
  const vtkVector2f& pos, int& activeVertexIndex, bool& activeLensOutline, bool& activeLensInterior)
{
  activeVertexIndex = -1;
  activeLensOutline = false;
  activeLensInterior = false;
  const int SIZE = 5;
  auto vertices = this->Internals->Plot->GetVertices();
  if (!vertices || vertices->GetNumberOfPoints() < 3)
  {
    return;
  }
  for (int i = 0; i < vertices->GetNumberOfPoints(); ++i)
  {
    double xy[2];
    vertices->GetPoint(i, xy);
    if (pos[0] <= xy[0] + SIZE && pos[0] >= xy[0] - SIZE && pos[1] <= xy[1] + SIZE &&
      pos[1] >= xy[1] - SIZE)
    {
      activeVertexIndex = i;
      vtkDebugMacro("vtkChartRadViz::GetActiveElement: Vertex: " << i);
      break;
    }
  }
  if (activeVertexIndex < 0 && this->Internals->Plot->GetLensVisibility())
  {
    double* lensCenter = this->Internals->Plot->GetLensCenter();
    double lensRadius = this->Internals->Plot->GetLensRadius();
    // distance between the lensCenter and the mouse point
    double r = std::sqrt(std::pow(pos[0] - lensCenter[0], 2) + std::pow(pos[1] - lensCenter[1], 2));
    if (std::abs(lensRadius - r) < SIZE)
    {
      activeLensOutline = true;
      vtkDebugMacro("vtkChartRadViz::GetActiveElement: Lens Outline");
    }
    else if (lensRadius - r > 0)
    {
      activeLensInterior = true;
      vtkDebugMacro("vtkChartRadViz::GetActiveElement: Lens Interior");
    }
  }
}

//------------------------------------------------------------------------------
bool vtkChartRadViz::MouseButtonReleaseEvent(const vtkContextMouseEvent& vtkNotUsed(mouse))
{
  this->Internals->DragVertexIndex = -1;
  this->Internals->ResizeLens = false;
  this->Internals->DragLens = false;
  return true;
}

//------------------------------------------------------------------------------
bool vtkChartRadViz::MouseWheelEvent(const vtkContextMouseEvent&, int)
{
  return true;
}

//------------------------------------------------------------------------------
void vtkChartRadViz::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if (this->Internals->Plot)
  {
    os << indent << "Plot: " << endl;
    this->Internals->Plot->PrintSelf(os, indent.GetNextIndent());
  }
}

VTK_ABI_NAMESPACE_END
