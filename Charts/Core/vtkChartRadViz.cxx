// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkChartRadViz.h"

#include "vtkCommand.h"
#include "vtkContext2D.h"
#include "vtkContextMouseEvent.h"
#include "vtkContextScene.h"
#include "vtkObjectFactory.h"
#include "vtkPlotRadViz.h"
#include "vtkPoints2D.h"
#include "vtkTextProperty.h"
#include <sstream>

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
  float DragLensStart[2];
};

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkChartRadViz);

//------------------------------------------------------------------------------
vtkChartRadViz::vtkChartRadViz()
{
  this->Internals = new vtkChartRadViz::vtkInternals();
}

//------------------------------------------------------------------------------
vtkChartRadViz::~vtkChartRadViz()
{
  delete this->Internals;
}

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
  }

  this->PaintChildren(painter);

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
  this->AddItem(this->Internals->Plot);
  this->Modified();
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
  if (!this->Scene->GetDirty())
  {
    vtkVector2f pos{ mouse.GetScenePos() };
    if (this->Internals->DragVertexIndex >= 0)
    {
      // rotate vertices
      auto vertices = this->Internals->Plot->GetVertices();
      double vertex[2];
      vertices->GetPoint(this->Internals->DragVertexIndex, vertex);
      double mousePos[2] = { pos[0], pos[1] };
      double angle = this->Internals->Plot->GetAngle(vertex, mousePos);
      vtkDebugMacro("vtkChartRadViz::MouseMoveEvent, angle:"
        << angle << " VerticesRotation: " << this->Internals->Plot->GetVerticesRotation());
      this->Internals->Plot->SetVerticesRotation(
        angle + this->Internals->Plot->GetVerticesRotation());
      this->Scene->SetDirty(true);
    }
    else if (this->Internals->ResizeLens)
    {
      float* lensCenter = this->Internals->Plot->GetLensCenter();
      // distance between the lensCenter and the mouse point
      float r =
        std::sqrt(std::pow(pos[0] - lensCenter[0], 2) + std::pow(pos[1] - lensCenter[1], 2));
      this->Internals->Plot->SetLensRadius(r);
      this->Scene->SetDirty(true);
    }
    else if (this->Internals->DragLens)
    {
      float d[2];

      d[0] = pos[0] - this->Internals->DragLensStart[0];
      d[1] = pos[1] - this->Internals->DragLensStart[1];
      float newLensCenter[2];
      float* lensCenter = this->Internals->Plot->GetLensCenter();
      newLensCenter[0] = lensCenter[0] + d[0];
      ;
      newLensCenter[1] = lensCenter[1] + d[1];
      this->Internals->Plot->SetLensCenter(newLensCenter);
      std::copy(pos.GetData(), pos.GetData() + 2, this->Internals->DragLensStart);
      this->Scene->SetDirty(true);
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
  if (this->Internals->Plot)
  {
    const int SIZE = 5;
    auto vertices = this->Internals->Plot->GetVertices();
    vtkVector2f pos{ mouse.GetScenePos() };
    for (int i = 0; i < vertices->GetNumberOfPoints(); ++i)
    {
      double xy[2];
      vertices->GetPoint(i, xy);
      if (pos[0] <= xy[0] + SIZE && pos[0] >= xy[0] - SIZE && pos[1] <= xy[1] + SIZE &&
        pos[1] >= xy[1] - SIZE)
      {
        // drag a vertex
        this->Internals->DragVertexIndex = i;
        break;
      }
    }
    if (this->Internals->DragVertexIndex < 0)
    {
      float* lensCenter = this->Internals->Plot->GetLensCenter();
      float lensRadius = this->Internals->Plot->GetLensRadius();
      // distance between the lensCenter and the mouse point
      float r =
        std::sqrt(std::pow(pos[0] - lensCenter[0], 2) + std::pow(pos[1] - lensCenter[1], 2));
      if (std::abs(lensRadius - r) < SIZE)
      {
        // resize the lens
        this->Internals->ResizeLens = true;
      }
      else if (lensRadius - r > 0)
      {
        // drag the lens
        this->Internals->DragLens = true;
        std::copy(pos.GetData(), pos.GetData() + 2, this->Internals->DragLensStart);
      }
    }
    return true;
  }
  else
  {
    return false;
  }
}

//------------------------------------------------------------------------------
bool vtkChartRadViz::MouseButtonReleaseEvent(const vtkContextMouseEvent& mouse)
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
